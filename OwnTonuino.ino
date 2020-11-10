/********************************************************************************
 * @file           : OwnTonuino
 * @author         : Christian Mahlburg
 * @date           : 07.11.2020
 * @brief          : This program is forked by original development TonUINO V 2.1
 *                   by Thorsten Voß.
 *                   
 *  _____         _____ _____ _____ _____
 * |_   _|___ ___|  |  |     |   | |     |
 *   | | | . |   |  |  |-   -| | | |  |  |
 *   |_| |___|_|_|_____|_____|_|___|_____|
 *   TonUINO Version 2.1
 *    
 *  created by Thorsten Voß and licensed under GNU/GPL.
 *  Information and contribution at https://tonuino.de.
 *    
 *  Changed by Christian Mahlburg
 *
 * TODO: 
 *  - status led for sleep timer
 *  - admin menu sleep timer configuration
 *  - modus sleep timer hinzufügen und sleep timer karte erstellen, kann dann eine Figur mit einer Uhr sein.
 *  - testen, ob nach button umbau noch alle funktionen gehen
 *
 ********************************************************************************
 * MIT License
 *
 * Copyright (c) 2020 CMA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 ********************************************************************************/

/********************************************************************************
 * includes
 ********************************************************************************/
#include <DFMiniMp3.h>
#include <EEPROM.h>
#include <JC_Button.h>
#include <MFRC522.h>
#include <SPI.h>
#include <SoftwareSerial.h>

/********************************************************************************
 * defines
 ********************************************************************************/
#define DEBUG /* COMMENT WHEN NOT IN DEBUG MODE */

#define VERSION             4 /* if number is changed, device settings in eeprom will be reseted */
/* these presets will be used on device settings reset in eeprom */
#define VOLUME_MIN_PRESET   0
#define VOLUME_MAX_PRESET   15
#define VOLUME_INIT_PRESET  6
#define SLEEP_TIMER_MINUTES_PRESET  5

/* Buttons */
#define BUTTON_PLAY_PIN     A0
#define BUTTON_UP_PIN       A2
#define BUTTON_DOWN_PIN     A1
#define BUTTON_NEXT_PIN     A4
#define BUTTON_PREV_PIN     A3
/* MFRC522 */
#define MFRC522_RST_PIN     9
#define MFRC522_SS_PIN      10
/* DF PLAYER */
#define DF_PLAYER_RX_PIN    2
#define DF_PLAYER_TX_PIN    3
#define DF_PLAYER_BUSY_PIN  4
/* ADC */
#define OPEN_ANALOG_PIN     A7  /* used for random number generation */
/* sleep timer led */
#define SLEEP_TIMER_LED_PIN A5

#define BUTTON_LONG_PRESS_TIME   1000 /* ms */

#ifdef  DEBUG
#define DEBUG_PRINT(x)      Serial.print(x)
#define DEBUG_PRINT_LN(x)   Serial.println(x)
#define DEBUG_TRACE         { Serial.print("==> "); Serial.println(__PRETTY_FUNCTION__); }
#else
#define DEBUG_PRINT(x)      ((void)0U)
#define DEBUG_PRINT_LN(x)   ((void)0U)
#define DEBUG_TRACE         ((void)0U)
#endif

#define UNUSED(x)           (void)x;

#define SERIAL_BAUD         115200

#define MAX_TRACKS_IN_FOLDER            255
#define MAX_FOLDER                      100

/*mp3 track number */
#define MP3_STARTUP_SOUND                   500
#define MP3_NEW_NFC_TAG                     300
#define MP3_SELECT_FOLDER                   301
#define MP3_SELECT_LISTEN_MODE              310
#define MP3_LISTEN_MODE_ALBUM               312
#define MP3_LISTEN_MODE_SHUFFLE             313
#define MP3_LISTEN_MODE_AUDIO_BOOK          315
#define MP3_LISTEN_MODE_ADMIN               316
#define MP3_OWEH_DAS_HAT_LEIDER             401
#define MP3_OK_ICH_HABE_DIE_KAR             400
#define MP3_RESET_OK                        999
#define MP3_ADMIN_MENU                      900
#define MP3_ADMIN_MENU_RESET_TAG            901
#define MP3_ADMIN_MENU_INSERT_TAG           800
#define MP3_ADMIN_MENU_VOL_MAX              902
#define MP3_ADMIN_MENU_VOL_MIN              903
#define MP3_ADMIN_MENU_VOL_INI              904
#define MP3_ADMIN_MENU_VOL_MAX_SELECTED     930
#define MP3_ADMIN_MENU_VOL_MIN_SELECTED     931
#define MP3_ADMIN_MENU_VOL_INI_SELECTED     932
#define MP3_ADMIN_MENU_SLEEP_TIMER          908
#define MP3_ABORT                           802

#define DF_PLAYER_COM_DELAY 100 /* ms, delay to make shure that communication was finished with df player before continuing in program */

/* For every folder the actual track can be stored in eeprom (audio book mode).
 * Therefore the track number (0 ... 255) is stored at folder number - 1.
 * Example: If folder number 5 is in audio book mode, the last played track is stored at eeprom address 5.
 * This results in eeprom adress 0 is always empty, because folder numeration starts with 1 to 99.
 */
#define EEPROM_DEVICE_SETTINGS_ADDR     MAX_FOLDER


/********************************************************************************
 * static memeber
 ********************************************************************************/

typedef enum
{
    BUTTON_PLAY,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_NEXT,
    BUTTON_PREV,
    NR_OF_BUTTONS,    /* DO NOT USE AS BUTTON */
} ButtonNr_t;

typedef enum 
{
    MODE_UNSET,       /* folder not configured */
    MODE_ALBUM,       /* play whole folder in normal sequence */
    MODE_SHUFFLE,     /* play whole folder in random sequence */
    MODE_AUDIO_BOOK,  /* play whole folder in normal sequence from last playes track */
    MODE_ADMIN,       /* admin mode */
    NR_OF_MODES,      /* DO NOT USE AS MODE */
} FolderMode_t;

typedef enum
{
    ADMIN_MENU_FIRST_OPTION     = 0,
    ADMIN_MENU_RESET_NFC_TAG    = 0,
    ADMIN_MENU_SET_VOL_MAX      = 1,
    ADMIN_MENU_SET_VOL_MIN      = 2,
    ADMIN_MENU_SET_VOL_INI      = 3,
    ADMIN_MENU_SET_SLEEP_TIMER  = 4,
    ADMIN_MENU_LAST_OPTION      = 4, /* DO NOT USE AS OPTION */
} AdminMenuOptions_t;

typedef struct 
{
    uint8_t      number;
    FolderMode_t mode;
} FolderSettings_t;

typedef struct 
{
    uint8_t version;
    uint8_t volumeMin;
    uint8_t volumeMax;
    uint8_t volumeInit;
    uint8_t sleepTimerMinutes;
} DeviceSettings_t;

typedef struct 
{
  uint32_t cookie;
  uint8_t version;
  FolderSettings_t folderSettings;
} NfcTagObject_t;
 
/* DFPlayer Mini */
static SoftwareSerial mySoftwareSerial(DF_PLAYER_RX_PIN, DF_PLAYER_TX_PIN); // RX, TX
static uint16_t numTracksInFolder;
static uint16_t currentTrack = 1;
static uint8_t queue[255];
static uint8_t volume;

static FolderSettings_t folder;
static DeviceSettings_t deviceSettings;

/* Buttons */
static Button button[NR_OF_BUTTONS] = 
{
    BUTTON_PLAY_PIN, 
    BUTTON_UP_PIN, 
    BUTTON_DOWN_PIN,
    BUTTON_NEXT_PIN, 
    BUTTON_PREV_PIN,
};

/* helper array to ignore button release after long press */
static bool buttonLongPressed[NR_OF_BUTTONS] = { false, false, false, false, false };

/* MFRC522 */
MFRC522 mfrc522(MFRC522_SS_PIN, MFRC522_RST_PIN); 
MFRC522::MIFARE_Key key;
bool successRead;
byte sector = 1;
byte blockAddr = 4;
byte trailerBlock = 7;
MFRC522::StatusCode status;
static const uint32_t GOLDEN_COOKIE = 0xDEADBEEF; /* this cookie is used to identify known cards */
static bool nfcConfigInProgess = false;

/* sleep timer */
static bool sleepTimerIsActive = false;
static unsigned long sleepTimerActivationTime = 0;

/* foreward decalarations */
static void nextTrack(void);
static void previousTrack(void);
static void volumeUp(void);
static void volumeDown(void);
static bool mp3IsPlaying(void);
static void shuffleQueue(void);
static void playFolder(void);
static void writeSettingsToFlash(void);
static void resetSettings(void);
static void loadSettingsFromFlash(void);
static void playStartupSound(void);
static void printDeviceSettings(void);
static void adminMenu(void);
/* wrapper functions for DFMiniMp3 Player with delay */
static void mp3Start(void);
static void mp3Pause(void);
static void mp3SetVolume(uint8_t volume);
static void mp3Begin(void);
static void mp3Loop(void);
static void mp3PlayFolderTrack(uint8_t folder, uint16_t track);
static uint16_t mp3GetFolderTrackCount(uint8_t folderNr);
static void mp3PlayMp3FolderTrack(uint16_t track);
static uint16_t mp3GetTotalFolderCount(void);
static uint8_t mp3GetVolume(void);

static void playAdvertisement(uint16_t advertTrack);
static void waitForTrackFinish(void);
/* rfid */
static void nfcHandler(void);
static bool readNfcTag(NfcTagObject_t* nfcTag);
static bool writeNfcTag(NfcTagObject_t nfcTag);
static NfcTagObject_t setupNfcTag(void);
static void resetNfcTag(void);

/* wrapper functions for JC_Buttons */
static bool buttonWasReleased(ButtonNr_t buttonNr);
static bool buttonPressedFor(ButtonNr_t buttonNr, uint32_t ms);
static bool buttonWasPressed(ButtonNr_t buttonNr);
static bool buttonIsPressed(ButtonNr_t buttonNr);
static void readButtons(void);

/* sleep timer */
static void sleepTimerHandler(void);
static void sleepTimerEnable(void);
static void sleepTimerDisable(void);


/********************************************************************************
 * classes
 ********************************************************************************/
class Mp3Notify 
{
public:
    static void OnError(uint16_t errorCode)
    {
      Serial.print("com error ");       /* see DfMp3_Error for code meaning */
      Serial.println(errorCode);
    }
    
    static void PrintlnSourceAction(DfMp3_PlaySources source, const char* action) 
    {
      if (source & DfMp3_PlaySources_Sd) DEBUG_PRINT("sd card ");
      if (source & DfMp3_PlaySources_Usb) DEBUG_PRINT("USB ");
      if (source & DfMp3_PlaySources_Flash) DEBUG_PRINT("flash ");
      Serial.println (action);
    }
    
    static void OnPlayFinished(DfMp3_PlaySources source, uint16_t track)
    {
        UNUSED(source);
        UNUSED(track);
        nextTrack();  
    }
    
    static void OnPlaySourceOnline(DfMp3_PlaySources source)
    {
      PrintlnSourceAction(source, "online");
    }
    
    static void OnPlaySourceInserted(DfMp3_PlaySources source)
    {
      PrintlnSourceAction(source, "ready");
    }
    
    static void OnPlaySourceRemoved(DfMp3_PlaySources source)
    {
      PrintlnSourceAction(source, "removed");
    }
};

static DFMiniMp3<SoftwareSerial, Mp3Notify> mp3(mySoftwareSerial);

/********************************************************************************
 * private functions
 ********************************************************************************/
static void nextTrack(void)
{
    DEBUG_TRACE;
    
    if (nfcConfigInProgess == true)
    {
        DEBUG_PRINT_LN(F("Return without action"));
        return;
    }
      
    if (folder.mode == MODE_ALBUM)
    {
        if (currentTrack < numTracksInFolder)
        {
            currentTrack = currentTrack + 1;
            mp3PlayFolderTrack(folder.number, currentTrack);
            DEBUG_PRINT(F("album mode -> next track: "));
            DEBUG_PRINT_LN(currentTrack);
            DEBUG_PRINT(F("folder: "));
            DEBUG_PRINT_LN(folder.number);
        }
        else
        {
            DEBUG_PRINT(F("album mode -> end"));
        }
    }
    
    if (folder.mode == MODE_SHUFFLE)
    {
        if (currentTrack < numTracksInFolder)
        {
            currentTrack++;
            DEBUG_PRINT(F("shuffle mode -> next track: "));
            DEBUG_PRINT_LN(queue[currentTrack-1]);
        }
        else
        {
            currentTrack = 1;
            DEBUG_PRINT(F("shuffle mode -> end of queue, start from beginning track: "));
            DEBUG_PRINT_LN(queue[currentTrack-1]);
        }
        mp3PlayFolderTrack(folder.number, queue[currentTrack - 1]);
    }

    if (folder.mode == MODE_AUDIO_BOOK)
    {
        if (currentTrack < numTracksInFolder)
        {
            currentTrack = currentTrack + 1;
            mp3PlayFolderTrack(folder.number, currentTrack);
            DEBUG_PRINT(F("audio book mode -> store progress, next track: "));
            DEBUG_PRINT_LN(currentTrack);
        } 
        else
        {
            currentTrack = 1;
            DEBUG_PRINT(F("audio book mode -> end, store progress, reset to first track"));
        }
        EEPROM.update(folder.number, currentTrack);            
    }
}

static void previousTrack(void)
{
    DEBUG_TRACE;

    if (folder.mode == MODE_ALBUM)
    {
        if (currentTrack > 1)
        {
          currentTrack = currentTrack - 1;
          
        }
        mp3PlayFolderTrack(folder.number, currentTrack);
        DEBUG_PRINT(F("album mode -> prev track: "));
        DEBUG_PRINT_LN(currentTrack);
    }
    
    if (folder.mode == MODE_SHUFFLE)
    {
        if (currentTrack > 1)
        {
            currentTrack--;
            DEBUG_PRINT(F("shuffle mode -> prev track: "));
            DEBUG_PRINT_LN(queue[currentTrack-1]);
        }
        else
        {
            currentTrack = numTracksInFolder;
            DEBUG_PRINT(F("shuffle mode -> beginning of queue, jump to last track in queue: "));
            DEBUG_PRINT_LN(queue[currentTrack-1]);
        }
        DEBUG_PRINT_LN(queue[currentTrack - 1]);
        mp3PlayFolderTrack(folder.number, queue[currentTrack - 1]);
    }
    
    if (folder.mode == MODE_AUDIO_BOOK)
    {
        if (currentTrack > 1)
        {
          currentTrack = currentTrack - 1;
        }
        mp3PlayFolderTrack(folder.number, currentTrack);
        EEPROM.update(folder.number, currentTrack);
        DEBUG_PRINT(F("audio book mode -> store progress, prev track: "));
        DEBUG_PRINT_LN(currentTrack);
    }
}

static void volumeUp(void)
{
    DEBUG_TRACE;
    
    if (volume < deviceSettings.volumeMax) 
    {
        volume++;
        mp3SetVolume(volume);
    }
    DEBUG_PRINT_LN(F("new volume: "));
    DEBUG_PRINT_LN(volume);
}

static void volumeDown(void)
{
    DEBUG_TRACE;
    
    if (volume > deviceSettings.volumeMin)
    {
        volume--;
        mp3SetVolume(volume);
    }
    DEBUG_PRINT_LN(F("new volume: "));
    DEBUG_PRINT_LN(volume);
}

static void buttonHandler(void)
{
    readButtons();
    
    if (buttonIsPressed(BUTTON_DOWN) && buttonIsPressed(BUTTON_UP) && buttonIsPressed(BUTTON_PLAY))
    {
        DEBUG_PRINT_LN(F("BUTTON PLAY UP DOWN PRESSED"));
        adminMenu();
        playFolder();
    }
        
    
    if (buttonWasReleased(BUTTON_PLAY))
    {   
        DEBUG_PRINT_LN(F("BUTTON PLAY RELEASED"));
        if (mp3IsPlaying())
        {
            mp3Pause();
        }
        else
        {
            mp3Start();
        }
    } 
    
    if (buttonPressedFor(BUTTON_PLAY, BUTTON_LONG_PRESS_TIME))
    {
        if (sleepTimerIsActive)
        {
            sleepTimerDisable();
        }
        else
        {
            sleepTimerEnable();
        }
    }
    
    if (buttonWasReleased(BUTTON_UP)) 
    {
        DEBUG_PRINT_LN(F("BUTTON UP RELEASED"));
        if (mp3IsPlaying())
        {
            volumeUp();
        }
    }

    if (buttonWasReleased(BUTTON_DOWN))
    {
        DEBUG_PRINT_LN(F("BUTTON DOWN RELEASED"));
        if (mp3IsPlaying())
        {
            volumeDown();
        }
    }

    if (buttonWasReleased(BUTTON_NEXT))
    {
        DEBUG_PRINT_LN(F("BUTTON NEXT RELEASED"));
        if (mp3IsPlaying())
        {
            nextTrack();
        }
    }
    
    if (buttonWasReleased(BUTTON_PREV))
    {
        DEBUG_PRINT_LN(F("BUTTON PREV RELEASED"));
        if (mp3IsPlaying())
        {
            previousTrack();
        }
    }
    
    if (buttonPressedFor(BUTTON_PREV, BUTTON_LONG_PRESS_TIME))
    {
        /* reset to first track / mix shuffle queue */
        DEBUG_PRINT_LN(F("BUTTON PLAY LONG PRESS"));
        if (folder.mode == MODE_AUDIO_BOOK)
        {
            EEPROM.update(folder.number, 1);
        }
        playFolder();
    }
}

static void shuffleQueue(void)
{
    DEBUG_TRACE;
    
    /* create queue for shuffle */
    for (uint8_t i = 0; i < numTracksInFolder; i++)
    {
        queue[i] = i + 1; /* track number starts with 1 */
    }
    
    /* DEBUG */
    DEBUG_PRINT_LN(F("Queue unmixed:"));
    for (uint8_t i = 0; i < numTracksInFolder; i++)
    {
        DEBUG_PRINT_LN(queue[i]);
    }
  
    /* fill queue with zeros */
    for (uint8_t i = numTracksInFolder; i < MAX_TRACKS_IN_FOLDER; i++)
    {
        queue[i] = 0;
    }
  
    /* mix queue */
    for (uint8_t i = 0; i < numTracksInFolder; i++)
    {
        uint8_t j = random(0, numTracksInFolder - 1);
        uint8_t temp = queue[i];
        queue[i] = queue[j];
        queue[j] = temp;
    }
    
    /* DEBUG */
    DEBUG_PRINT_LN(F("Queue mixed:"));
    for (uint8_t i = 0; i < numTracksInFolder; i++)
    {
        DEBUG_PRINT_LN(queue[i]);
    }
}

static void playFolder(void)
{
    DEBUG_TRACE;
   
    numTracksInFolder = mp3GetFolderTrackCount(folder.number);
    DEBUG_PRINT(numTracksInFolder);
    DEBUG_PRINT(F(" files in directory "));
    DEBUG_PRINT_LN(folder.number);

    if (folder.mode == MODE_ALBUM)
    {
        DEBUG_PRINT_LN(F("album mode -> play whole folder"));
        currentTrack = 1;
        mp3PlayFolderTrack(folder.number, currentTrack);
    }
    else if (folder.mode == MODE_SHUFFLE)
    {
        DEBUG_PRINT_LN(F("shuffle mode -> play folder in random order"));
        shuffleQueue();
        currentTrack = 1;
        mp3PlayFolderTrack(folder.number, queue[currentTrack - 1]);
    }
    else if (folder.mode == MODE_AUDIO_BOOK)
    {
        DEBUG_PRINT_LN(F("audio book mode -> play whole folder and remember progress"));
        currentTrack = EEPROM.read(folder.number);
   
        if ((currentTrack == 0) || (currentTrack > numTracksInFolder))
        {
            currentTrack = 1;
        }
        mp3PlayFolderTrack(folder.number, currentTrack);
    }
}

static void playStartupSound(void)
{
    DEBUG_TRACE;
    mp3PlayMp3FolderTrack(MP3_STARTUP_SOUND);
}

static void playAdvertisement(uint16_t advertTrack)
{
    DEBUG_TRACE;
    
    if (mp3IsPlaying())
    {
        mp3.playAdvertisement(advertTrack);
        waitForTrackFinish();
        delay(50); /* delay for dfplayer to start the interrupted track before continuing in program */
    }
    else
    {
        /* TBD if it's realy necessary to play advert tracks if music is not playing (therefore mp3 folder is usable)
         * possible solution is to add same mp3 files into advert dir and mp3 dir on sd card an call mp3.playMp3FolderTrack(advertTrack); here
         */
        mp3Start();
        mp3.playAdvertisement(advertTrack);
        waitForTrackFinish();
        mp3Pause();
    }
}

static void waitForTrackFinish(void)
{
    DEBUG_TRACE;
    
    unsigned long timeStart = 0;
    unsigned long TRACK_FINISHED_TIMEOUT = 500; /* ms, timeout to start the track by dfplayer and raise busy pin */
    
    /* If starting a new song while another song is playing and directily calling this function, 
     * it's possible that the old song is still playing when calling this function. To avoid detecting 
     * "track finished" of the old song, a initial delay is added.
     */
    delay(100); 
    timeStart = millis();
    
    /* Wait for track to finish. Therefore it must be checked that new track already started.
     * Otherwise it's possible that "track finished" is detected in the timeslice, in which the dfplayer needs to start the new track.
     */
    while ((!mp3IsPlaying()) && (millis() < (timeStart + TRACK_FINISHED_TIMEOUT)))
    {
        /* wait for track starting */
        mp3Loop();
    }
    DEBUG_PRINT_LN(F("track started"));
    
    while (mp3IsPlaying())
    {
        /* wait for track to finish */
        mp3Loop();
    }
    DEBUG_PRINT_LN(F("track finished"));
}

static void sleepTimerHandler(void)
{
    unsigned long currentTime    = 0;
    unsigned long sleepTime      = 0;
    static uint16_t muteCounter  = 0; /* used to mute music steadily */
    static uint16_t printCounter = 0; /* DEBUG REMOVE THIS */
    
    if (sleepTimerIsActive)
    {
        currentTime = millis();
        sleepTime = sleepTimerActivationTime + ((unsigned long)deviceSettings.sleepTimerMinutes  * 60000); /* x 60000 converts form minutes to ms */
        
        /* DEBUG */
        if (printCounter >= 30) /* this value is choosed by trying to find a good serial print interval */
        {   
            DEBUG_PRINT(F("sleep timer time left: "));
            DEBUG_PRINT_LN((signed long)sleepTime - (signed long)currentTime);
            printCounter = 0;
        }
        else
        {
            printCounter++;
        }
            
        if (currentTime >= sleepTime)
        {
            if (muteCounter >= 50) /* this value is choosed by trying to find a good volume decrease interval */
            {
                muteCounter = 0;
                mp3DecreaseVolume();
                DEBUG_PRINT_LN(F("Decreasing volume"));
                
                if (mp3GetVolume() == 0)
                {
                    mp3Pause();
                    sleepTimerDisable();
                    DEBUG_PRINT_LN(F("sleeping now ...zzz"));
                }   
            }
            muteCounter++;
        }
    }
}

static void sleepTimerEnable(void)
{
    DEBUG_TRACE
    
    /* if sleepTimer is already active, do not reset */
    if (sleepTimerIsActive == false)
    {
        sleepTimerIsActive = true;
        sleepTimerActivationTime = millis();
        digitalWrite(SLEEP_TIMER_LED_PIN, HIGH);
        DEBUG_PRINT_LN(F("sleep timer enabled"));        
    }
}

static void sleepTimerDisable(void)
{
    DEBUG_TRACE
    
    sleepTimerIsActive = false;
    sleepTimerActivationTime = 0;
    mp3SetVolume(volume); /* reset volume if sleepTimer is deactivated while decreasing volume in sleepTimerHandler */
    digitalWrite(SLEEP_TIMER_LED_PIN, LOW);
    DEBUG_PRINT_LN(F("sleep timer disabled"));
}
        
        

/********************************************************************************
 * main program
 ********************************************************************************/
void setup(void)
{
    uint32_t ADC_LSB = 0;
    uint32_t ADCSeed = 0;

    /* create value for randomSeed() through multiple sampling of the LSB on open analog input */
    for (uint8_t i = 0; i < 128; i++)
    {
        ADC_LSB = analogRead(OPEN_ANALOG_PIN) & 0x1;
        ADCSeed ^= ADC_LSB << (i % 32);
    }
    randomSeed(ADCSeed); /* initialize random generator */
   
    Serial.begin(SERIAL_BAUD);
    loadSettingsFromFlash();
    /* DEBUG REMOVE THIS */
    deviceSettings.sleepTimerMinutes = 1;
    
    printDeviceSettings();

    volume = deviceSettings.volumeInit;

    mp3Begin();
    mp3SetVolume(volume);
    
    numTracksInFolder = mp3GetFolderTrackCount(folder.number);

    pinMode(BUTTON_PLAY_PIN, INPUT_PULLUP);
    pinMode(BUTTON_UP_PIN,   INPUT_PULLUP);
    pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
    pinMode(BUTTON_NEXT_PIN, INPUT_PULLUP);
    pinMode(BUTTON_PREV_PIN, INPUT_PULLUP);
    pinMode(SLEEP_TIMER_LED_PIN, OUTPUT);
    pinMode(DF_PLAYER_BUSY_PIN, INPUT_PULLUP);
       
    /* NFC Leser init */
    SPI.begin();        // Init SPI bus
    mfrc522.PCD_Init(); // Init MFRC522
    mfrc522
    .PCD_DumpVersionToSerial(); // Show details of PCD - MFRC522 Card Reader
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
    
    playStartupSound();
    delay(200);
    playFolder();
}
 
void loop(void)
{
    while(!mfrc522.PICC_IsNewCardPresent())
    {
        mp3Loop();
        buttonHandler();
        sleepTimerHandler();
        delay(1);
    }

    mp3Pause();
    nfcHandler();
    playFolder();
}
