/********************************************************************************
 * @file    : OwnTonuino.ino
 * @author  : Christian Mahlburg
 * @date    : 07.11.2020
 * @brief   : This program is forked by original development TonUINO V 2.1 
 *            by Thorsten Voß.
 *                   
 *          _____         _____ _____ _____ _____
 *         |_   _|___ ___|  |  |     |   | |     |
 *           | | | . |   |  |  |-   -| | | |  |  |
 *           |_| |___|_|_|_____|_____|_|___|_____|
 *           TonUINO Version 2.1
 *         
 *           created by Thorsten Voß and licensed under GNU/GPL.
 *           Information and contribution at https://tonuino.de.
 *         
 *           Changed by Christian Mahlburg and licensed under GNU/GPL.
 *
 *  TODO:
 * (- if a keycard is put on the player and there is no track playing, short parts of the
 *    track are played because music must be enabled to play advertisement 
 *    -> replace voice lines with red indicator led -> so the whole firmware does
 *       not use advertisement 
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
#include <FastLED.h>

/********************************************************************************
 * defines
 ********************************************************************************/
#define DEBUG /* COMMENT WHEN NOT IN DEBUG MODE */

#define FW_MAIN_VERSION  0          /* 0 ...16, if number is changed, device settings in eeprom will be reseted */
#define FW_SUB_VERSION   1          /* 0 ...16, if number is changed, device settings in eeprom will be reseted */
#define GOLDEN_COOKIE    0xAFFEAFFE /* this cookie is used to identify known cards, if changed, all configured cards are unconfigured */

/* device presets
 * these presets will be used on device settings reset in eeprom 
 */
#define VOL_MIN_PRESET              1
#define VOL_MAX_PRESET              15
#define VOL_INI_PRESET              6
#define SLEEP_TIMER_MINUTES_PRESET  15 /* The number should be an integer divisible by 5 */

/* pin assignment */
#define BUTTON_PLAY_PIN      A0  /* Buttons */
#define BUTTON_UP_PIN        A2
#define BUTTON_DOWN_PIN      A1
#define BUTTON_NEXT_PIN      A4
#define BUTTON_PREV_PIN      A3
#define MFRC522_RST_PIN      9   /* MFRC522 */
#define MFRC522_SS_PIN       10
#define DF_PLAYER_RX_PIN     2   /* DFPlayer Mini */
#define DF_PLAYER_TX_PIN     3
#define DF_PLAYER_BUSY_PIN   4
#define OPEN_ANALOG_PIN      A7  /* ADC, used for random number generation */
#define SLEEP_TIMER_LED_PIN  A5  /* LEDs */
#define WS2812B_LED_DATA_PIN 5  /* same as A6 */

/* WS2812B Ambient LEDs */
#define WS2812B_NR_OF_LEDS   2
#define WB2812B_BRIGHTNESS   200

/* DEBUG tools */
#ifdef  DEBUG
#define DEBUG_PRINT(x)              Serial.print(x)
#define DEBUG_PRINT_LN(x)           Serial.println(x)
#define DEBUG_PRINT_TWO_ARG(x, y)   Serial.print(x, y)
#define DEBUG_TRACE                 {Serial.print("==> "); Serial.println(__PRETTY_FUNCTION__);}
#define ASSERT(x)                   ((x) ? (void)0U : assertFailed(__FILE__, __LINE__))
#else
#define DEBUG_PRINT(x)              ((void)0U)
#define DEBUG_PRINT_LN(x)           ((void)0U)
#define DEBUG_PRINT_TWO_ARG(x, y)   ((void)0U)
#define DEBUG_TRACE                 ((void)0U)
#define ASSERT(x)                   ((void)0U)
#endif
#define UNUSED(x)                   (void)x;

/*mp3 track number */
#define MP3_INSERT_TAG                    300
#define MP3_NEW_TAG                       301
#define MP3_SELECT_MODE                   302
#define MP3_SELECT_FOLDER                 303
#define MP3_TAG_CONFIG_OK                 304
#define MP3_TAG_CONFIG_ERROR              305
#define MP3_PIN_CODE_WRONG                MP3_TAG_CONFIG_ERROR
#define MP3_MODE_ALBUM                    310
#define MP3_MODE_SHUFFLE                  311
#define MP3_MODE_AUDIO_BOOK               312
#define MP3_MODE_KEYCARD                  313
#define MP3_STARTUP_SOUND                 500
#define MP3_BEEP                          501
#define MP3_ACTION_ABORT_INTRO            800
#define MP3_ACTION_ABORT_OK               801
#define MP3_NO                            802
#define MP3_YES                           803
#define MP3_OK                            804
#define MP3_ADMIN_MENU_PINCODE            900
#define MP3_ADMIN_MENU                    901
#define MP3_ADMIN_MENU_EXIT_SELECTED      902
#define MP3_CARD_RESET_INTRO              910
#define MP3_VOL_MAX_INTRO                 920
#define MP3_VOL_MAX_SELECTED              921
#define MP3_VOL_MAX_OK                    922
#define MP3_VOL_MIN_INTRO                 930
#define MP3_VOL_MIN_SELECTED              931
#define MP3_VOl_MIN_OK                    932
#define MP3_VOL_INI_INTRO                 940
#define MP3_VOL_INI_SELECTED              941
#define MP3_VOL_INI_OK                    942
#define MP3_SLEEP_TIMER_INTRO             950
#define MP3_SLEEP_TIMER_SELECTED          951
#define MP3_SLEEP_TIMER_OK                952
#define MP3_SETTINGS_RESET_INTRO          960
#define MP3_SETTINGS_RESET_SELECTED       961
#define MP3_SETTINGS_RESET_OK             962
/* advert track number*/
#define ADV_BUTTONS_UNLOCKED              300
#define ADV_BUTTONS_LOCKED                301

#define SERIAL_BAUD         115200
#define DF_PLAYER_MAX_TRACKS_IN_FOLDER    255  /* from datasheet */
#define DF_PLAYER_MAX_FOLDER              100
#define DF_PLAYER_MAX_VOL                 30
#define DF_PLAYER_COM_DELAY               100  /* ms, delay to make shure that communication was finished with df player before continuing in program */
#define SLEEP_TIMER_MINUTES_MAX           60
#define SLEEP_TIMER_MINUTES_INTERVAL      5    /* configuration interval in admin menu ( 5 min, 10 min ...) */
#define BUTTON_LONG_PRESS_TIME            1000 /* ms */
#define STARTUP_DELAY                     500  /* ms, delay between startup sound and music, if tag is detected immediately after startup (sounds better with a short delay between them) */
/* For every folder the actual track can be stored in eeprom (audio book mode).
 * Therefore the track number (0 ... 255) is stored at folder number - 1.
 * Example: If folder number 5 is in audio book mode, the last played track is stored at eeprom address 5.
 * This results in eeprom adress 0 is always empty, because folder numeration starts with 1 to 99.
 */
#define EEPROM_DEVICE_SETTINGS_ADDR       DF_PLAYER_MAX_FOLDER

#define DO_NOT_WAIT                       false
#define WAIT                              true

#define IS_LISTENMODE(x)                  ((x == MODE_ALBUM) || (x == MODE_SHUFFLE) || (x == MODE_AUDIO_BOOK))

/********************************************************************************
 * typedefs and static member
 ********************************************************************************/
typedef enum : uint8_t
{
    BUTTON_PLAY,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_NEXT,
    BUTTON_PREV,
    NR_OF_BUTTONS,      /* DO NOT USE AS BUTTON */
} ButtonNr_t;

typedef enum : uint8_t
{
    VOL_MIN,
    VOL_MAX,
    VOL_INI,
    NR_OF_VOL_SETTINGS, /* DO NOT USE AS VOLUME SETTING */
} VolumeSetting_t;
    
typedef enum : uint8_t  /* if the order of the modes is changed, also the mp3 track array below (MP3_MODE_ARRAY) must be reordered */
{
    MODE_UNSET      = 0,                /* DO NOT USE AS MODE */
    FIRST_MODE,                         /* DO NOT USE AS MODE */
    MODE_ALBUM      = FIRST_MODE,       /* play whole folder in normal sequence */
    MODE_SHUFFLE,                       /* play whole folder in random sequence */
    MODE_AUDIO_BOOK,                    /* play whole folder in normal sequence from last playes track */
    MODE_KEYCARD,                       /* buttons can be locked / unlocked and admin menu can be entered */
    LAST_MODE       = MODE_KEYCARD,     /* DO NOT USE AS MODE */
    NR_OF_MODES,                        /* DO NOT USE AS MODE */
} FolderMode_t;

static const uint16_t MP3_MODE_ARRAY[NR_OF_MODES - 1] =  /* -1 because mode unset is not a real mode */
{ 
    MP3_MODE_ALBUM, 
    MP3_MODE_SHUFFLE, 
    MP3_MODE_AUDIO_BOOK, 
    MP3_MODE_KEYCARD,
};

typedef enum : uint8_t  /* if the order of the menu options is changed, also the mp3 track array below (MP3_ADMIN_MENU_OPTIONS_ARRAY) must be reordered */
{
    ADMIN_MENU_FIRST_OPTION     = 0,                          /* DO NOT USE AS OPTION */
    ADMIN_MENU_SET_SLEEP_TIMER  = ADMIN_MENU_FIRST_OPTION,    /* configure the sleeptimer in minutes */
    ADMIN_MENU_RESET_NFC_TAG,                                 /* reconfigure a new or a configured nfc tag */
    ADMIN_MENU_SET_VOL_MAX,                                   /* set the maximum volume level */
    ADMIN_MENU_SET_VOL_MIN,                                   /* set the minimum volume level */
    ADMIN_MENU_SET_VOL_INI,                                   /* set the startup volume level */
    ADMIN_MENU_SETTINGS_RESET,                                /* reset all settings, tags are still configured */
    ADMIN_MENU_LAST_OPTION      = ADMIN_MENU_SETTINGS_RESET,  /* DO NOT USE AS OPTION */
    NR_OF_MENU_OPTIONS,                                       /* DO NOT USE AS OPTION */
} AdminMenuOptions_t;
    
const uint16_t MP3_ADMIN_MENU_OPTIONS_ARRAY[NR_OF_MENU_OPTIONS] = 
{ 
    MP3_SLEEP_TIMER_INTRO, 
    MP3_CARD_RESET_INTRO, 
    MP3_VOL_MAX_INTRO, 
    MP3_VOL_MIN_INTRO, 
    MP3_VOL_INI_INTRO, 
    MP3_SETTINGS_RESET_INTRO,
};

typedef struct 
{
    uint8_t   number;
    FolderMode_t mode;
} FolderSettings_t;

typedef struct 
{
    uint8_t version;
    uint8_t volume[NR_OF_VOL_SETTINGS];
    uint8_t sleepTimerMinutes;
} DeviceSettings_t;

typedef struct 
{
  uint32_t cookie;
  uint8_t version;
  FolderSettings_t folderSettings;
} NfcTagObject_t;
 
 static const uint8_t FW_VERSION = (FW_MAIN_VERSION << 4) + (FW_SUB_VERSION);
 
/* DFPlayer Mini */
static SoftwareSerial mySoftwareSerial(DF_PLAYER_RX_PIN, DF_PLAYER_TX_PIN); /* RX, TX */
static uint16_t numTracksInFolder                    = 0;
static uint16_t currentTrack                         = 0;
static uint8_t queue[DF_PLAYER_MAX_TRACKS_IN_FOLDER] = { 0 };
static uint8_t volume                                = VOL_INI_PRESET;
static bool skipNextTrack                            = false;            /* skip next track if configuring nfc tag or using admin menu */

/* MFRC522 */
MFRC522 mfrc522(MFRC522_SS_PIN, MFRC522_RST_PIN); 
MFRC522::MIFARE_Key key                              = { 0 };
bool successRead                                     = false;
uint8_t sector                                       = 1;
uint8_t blockAddr                                    = 4;
uint8_t trailerBlock                                 = 7;
MFRC522::StatusCode status                           = MFRC522::STATUS_OK;

static FolderSettings_t folder                       = { 0, MODE_UNSET };
static DeviceSettings_t deviceSettings               = {};

/* WB2812B Ambient LEDs */
CRGB ambientLeds[WS2812B_NR_OF_LEDS]                 = {};
static bool ambientLedEnabled                        = false;


/* Buttons */
static Button button[NR_OF_BUTTONS]                  = { BUTTON_PLAY_PIN, BUTTON_UP_PIN, BUTTON_DOWN_PIN, BUTTON_NEXT_PIN, BUTTON_PREV_PIN };
static bool ignoreNextButtonRelease[NR_OF_BUTTONS]   = { false, false, false, false, false };

/* sleep timer */
static bool sleepTimerIsActive                       = false;
static unsigned long sleepTimerActivationTime        = 0;
static bool buttonsLocked                            = false;

/* pincode */
static const uint8_t PIN_CODE_LENGTH                 = 6;
static const ButtonNr_t PIN_CODE[PIN_CODE_LENGTH]    = { BUTTON_DOWN, BUTTON_PLAY, BUTTON_UP, BUTTON_UP, BUTTON_PLAY, BUTTON_DOWN };


/********************************************************************************
 * foreward declarations
 ********************************************************************************/
static void nextTrack(void);
static void previousTrack(void);
static void volumeUp(void);
static void volumeDown(void);
static void shuffleQueue(void);
static void playFolder(void);
static void buttonHandler(void);

/* device settings */
static void settings_writeToEeprom(void);
static void settings_reset(void);
static void settings_loadFromEeprom(bool reset = false);
static void settings_print(void);

/* admin menu */
static void adminMenu_main(void);
static void adminMenu_setVolume(AdminMenuOptions_t menuOption);
static void adminMenu_setSleepTimer(void);
static void adminMenu_resetDeviceSettings(void);
static void adminMenu_enter(void);
static bool adminMenu_pinCompare(const ButtonNr_t* enteredCode, const ButtonNr_t* pinCode);

/* DFPlayer Mini */
static void mp3_loop(void);
static void mp3_begin(void);
static void mp3_start(void);
static void mp3_pause(void);
static void mp3_setVolume(uint8_t volume);
static void mp3_decreaseVolume(void);
static void mp3_playFolderTrack(uint8_t folder, uint16_t track);
static void mp3_playMp3FolderTrack(uint16_t track, bool waitTillFinish = true);
static void mp3_playAdvertisement(uint16_t advertTrack);
static void mp3_waitForTrackFinish(void);
static bool mp3_isPlaying(void);
static uint8_t mp3_getVolume(void);
static uint16_t mp3_getFolderTrackCount(uint8_t folderNr);
static uint16_t mp3_getTotalFolderCount(void);

/* nfc */
static void nfc_handler(void);
static void nfc_resetTag(void);
static bool nfc_readTag(NfcTagObject_t* nfcTag);
static bool nfc_writeTag(NfcTagObject_t nfcTag);
static NfcTagObject_t nfc_setupTag(void);

/* buttons */
static bool button_wasReleased(ButtonNr_t buttonNr);
static bool button_pressedFor(ButtonNr_t buttonNr, uint32_t ms);
static void button_readAll(void);
static bool button_allPressed(void);
static bool button_allReleased(void);

/* sleep timer */
static void sleepTimer_handler(void);
static void sleepTimer_enable(void);
static void sleepTimer_disable(void);

/* WS2812B ambient LED */
static void ambientLed_handler(void);

/* DEBUG */
#ifdef DEBUG
static void assertFailed(const char* file, const uint32_t line);
#endif

/********************************************************************************
 * classes
 ********************************************************************************/
/**
 * @brief This is the class definition for the DFPlayer Mini mp3 object.
 */
class Mp3Notify 
{
public:
    static void OnError(uint16_t errorCode)
    {
      Serial.print("com error "); /* see DfMp3_Error for code meaning */
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
        
        /* There is a problem in DFPlayer mini that sometimes,if a track is finished, this callback is called twice from DFPlayer.
         * This is a hardware bug and can not be solved clearly -> so the solution is to skip the track if the callback is called twice with same track number
         * withing the first 200ms. If a song should be played twice, it's possible if the song is longer than 0.2s ;). -> https://github.com/Makuna/DFMiniMp3/issues/34
         */
        uint32_t currentTime                        = millis();
        static uint16_t lastTrackFinished           = 0;
        static uint32_t lastTrackFinishedTime       = 0; 
        const uint32_t PLAY_SAME_SONG_TWICE_TIMEOUT = 200 /* ms */;
        
        /* check if this function call is valid or not (not valid if callback is triggered twice from dfplayer mini) */
        if ((currentTime > (lastTrackFinishedTime + PLAY_SAME_SONG_TWICE_TIMEOUT)) || (track != lastTrackFinished))
        {
            /* DEBUG_PRINT_LN(F("valid callback, function call is proceed")); */
            lastTrackFinished = track;
            lastTrackFinishedTime = millis();
        }
        else
        {
            /* DEBUG_PRINT(F("invalid callback, skip double invocation within ms: ")); */
            /* DEBUG_PRINT_LN(currentTime - lastTrackFinishedTime); */
            return;
        }
        
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
 * private methods
 ********************************************************************************/
 /**
 * @brief If an assertion fails, file and line is transmitted to debug usart
 *        and program is halted.
 */
#ifdef DEBUG
static void assertFailed(const char* file, const uint32_t line)
{
    DEBUG_PRINT(F("ASSERT: file "));
    DEBUG_PRINT(file); 
    DEBUG_PRINT(F(", line  "));
    DEBUG_PRINT_LN(line); 
}
#endif

/**
 * @brief This method plays the next track depending on the actual listen mode.
 */
static void nextTrack(void)
{
    DEBUG_TRACE;
    
    if (skipNextTrack == true)
    {
        DEBUG_PRINT_LN(F("Return without action"));
        return;
    }
      
    if (folder.mode == MODE_ALBUM)
    {
        if (currentTrack < numTracksInFolder)
        {
            currentTrack = currentTrack + 1;
            mp3_playFolderTrack(folder.number, currentTrack);
            DEBUG_PRINT(F("album mode -> next track: "));
            DEBUG_PRINT(currentTrack);
            DEBUG_PRINT(F(", folder: "));
            DEBUG_PRINT_LN(folder.number);
        }
        else
        {
            DEBUG_PRINT_LN(F("album mode -> end"));
        }
    }
    else if (folder.mode == MODE_SHUFFLE)
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
        mp3_playFolderTrack(folder.number, queue[currentTrack - 1]);
    }
    else if (folder.mode == MODE_AUDIO_BOOK)
    {
        if (currentTrack < numTracksInFolder)
        {
            currentTrack = currentTrack + 1;
            mp3_playFolderTrack(folder.number, currentTrack);
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

/**
 * @brief This method plays the previous track depending on the actual listen mode.
 */
static void previousTrack(void)
{
    DEBUG_TRACE;

    if (folder.mode == MODE_ALBUM)
    {
        if (currentTrack > 1)
        {
          currentTrack = currentTrack - 1;
          
        }
        mp3_playFolderTrack(folder.number, currentTrack);
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
        mp3_playFolderTrack(folder.number, queue[currentTrack - 1]);
    }
    
    if (folder.mode == MODE_AUDIO_BOOK)
    {
        if (currentTrack > 1)
        {
          currentTrack = currentTrack - 1;
        }
        mp3_playFolderTrack(folder.number, currentTrack);
        EEPROM.update(folder.number, currentTrack);
        DEBUG_PRINT(F("audio book mode -> store progress, prev track: "));
        DEBUG_PRINT_LN(currentTrack);
    }
}

/**
 * @brief This method increases the volume and transmits the volume to the DFPlayer Mini.
 */
static void volumeUp(void)
{
    DEBUG_TRACE;
    
    if (volume < deviceSettings.volume[VOL_MAX]) 
    {
        volume++;
        mp3_setVolume(volume);
    }
    DEBUG_PRINT_LN(F("new volume: "));
    DEBUG_PRINT_LN(volume);
}

/**
 * @brief This method decreases the volume and transmits the volume to the DFPlayer Mini.
 */
static void volumeDown(void)
{
    DEBUG_TRACE;
    
    if (volume > deviceSettings.volume[VOL_MIN])
    {
        volume--;
        mp3_setVolume(volume);
    }
    DEBUG_PRINT_LN(F("new volume: "));
    DEBUG_PRINT_LN(volume);
}

/**
 * @brief This method creaetes a shuffle queue with randomized track order for shuffle mode.
 *        To randomize the track order, every iteration a track is changing position with another track.
 *        This ensures that each track occurs exactly once in the queue.
 */
static void shuffleQueue(void)
{
    DEBUG_TRACE;
    
    /* create queue for shuffle */
    for (uint8_t i = 0; i < numTracksInFolder; i++)
    {
        queue[i] = i + 1; /* track number starts with 1 */
    }
    
#ifdef DEBUG
    DEBUG_PRINT_LN(F("Queue unmixed:"));
    for (uint8_t i = 0; i < numTracksInFolder; i++)
    {
        DEBUG_PRINT_LN(queue[i]);
    }
#endif
  
    /* fill queue with zeros */
    for (uint8_t i = numTracksInFolder; i < DF_PLAYER_MAX_TRACKS_IN_FOLDER; i++)
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
    
#ifdef DEBUG
    DEBUG_PRINT_LN(F("Queue mixed:"));
    for (uint8_t i = 0; i < numTracksInFolder; i++)
    {
        DEBUG_PRINT_LN(queue[i]);
    }
#endif
}

/**
 * @brief This method plays a folder depending on the actual listen mode.
 */
static void playFolder(void)
{
    DEBUG_TRACE;
   
    numTracksInFolder = mp3_getFolderTrackCount(folder.number);
    DEBUG_PRINT(numTracksInFolder);
    DEBUG_PRINT(F(" files in directory "));
    DEBUG_PRINT_LN(folder.number);

    if (folder.mode == MODE_ALBUM)
    {
        DEBUG_PRINT_LN(F("album mode -> play whole folder"));
        currentTrack = 1;
        mp3_playFolderTrack(folder.number, currentTrack);
    }
    else if (folder.mode == MODE_SHUFFLE)
    {
        DEBUG_PRINT_LN(F("shuffle mode -> play folder in random order"));
        shuffleQueue();
        currentTrack = 1;
        mp3_playFolderTrack(folder.number, queue[currentTrack - 1]);
    }
    else if (folder.mode == MODE_AUDIO_BOOK)
    {
        DEBUG_PRINT_LN(F("audio book mode -> play whole folder and remember progress"));
        currentTrack = EEPROM.read(folder.number);
   
        if ((currentTrack == 0) || (currentTrack > numTracksInFolder))
        {
            currentTrack = 1;
        }
        mp3_playFolderTrack(folder.number, currentTrack);
    }
}

/**
 * @brief The button handler manages the button interactions in the main loop. 
 *         Button actions within the admin menu or the nfc setup are handled seperately in the respective methods.
 */
static void buttonHandler(void)
{
    if (buttonsLocked)
    {
        return;
    }
        
    button_readAll();
    
    if (button_allPressed())
    {  
        adminMenu_enter();
        playFolder();
    }
    
    if (button_wasReleased(BUTTON_PLAY))
    {   
        DEBUG_PRINT_LN(F("BUTTON PLAY RELEASED"));
        if (IS_LISTENMODE(folder.mode))
        {
            if (mp3_isPlaying())
            {
                mp3_pause();
            }
            else
            {
                mp3_start();
            }
        }            
    } 
    
    if (button_pressedFor(BUTTON_PLAY, BUTTON_LONG_PRESS_TIME))
    {
        if (sleepTimerIsActive)
        {
            sleepTimer_disable();
        }
        else
        {
            sleepTimer_enable();
        }
    }
    
    if (button_wasReleased(BUTTON_UP)) 
    {
        DEBUG_PRINT_LN(F("BUTTON UP RELEASED"));
        if (mp3_isPlaying())
        {
            volumeUp();
        }
    }

    if (button_wasReleased(BUTTON_DOWN))
    {
        DEBUG_PRINT_LN(F("BUTTON DOWN RELEASED"));
        if (mp3_isPlaying())
        {
            volumeDown();
        }
    }

    if (button_wasReleased(BUTTON_NEXT))
    {
        DEBUG_PRINT_LN(F("BUTTON NEXT RELEASED"));
        if (mp3_isPlaying())
        {
            nextTrack();
        }
    }
    
    if (button_wasReleased(BUTTON_PREV))
    {
        DEBUG_PRINT_LN(F("BUTTON PREV RELEASED"));
        if (mp3_isPlaying())
        {
            previousTrack();
        }
    }
}

static void ambientLed_handler(void)
{
    static uint8_t color = random(0, 0xFF);;
    static uint8_t counter = 0;
    
    if (mp3_isPlaying())
    {
        ambientLedEnabled = true;
        for (uint8_t i = 0; i < WS2812B_NR_OF_LEDS; i++)
        {
            ambientLeds[i].setHSV(color, 255, 255);
        }
        FastLED.show();
        
        counter++;
        if (counter >= 10)
        {
            color++; /* if overflowing, color starts from 0 */
            counter = 0;
        }
    }
    else
    {
        ambientLed_disable();
    }
}

static void ambientLed_disable(void)
{
    if (ambientLedEnabled)
    { 
        for (uint8_t i = 0; i < WS2812B_NR_OF_LEDS; i++)
        {
            ambientLeds[i] = CRGB::Black;
        }
        FastLED.show();
        ambientLedEnabled = false;
    }
}    

/********************************************************************************
 * main program
 ********************************************************************************/
/**
 * @brief This method initializes all necessary hardware and software for runtime program.
 */
void setup(void)
{
    bool reset = false;
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
    Serial.println();
    Serial.println("Booting...");
    
    pinMode(BUTTON_PLAY_PIN, INPUT_PULLUP);
    pinMode(BUTTON_UP_PIN,   INPUT_PULLUP);
    pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
    pinMode(BUTTON_NEXT_PIN, INPUT_PULLUP);
    pinMode(BUTTON_PREV_PIN, INPUT_PULLUP);
    pinMode(SLEEP_TIMER_LED_PIN, OUTPUT);
    pinMode(DF_PLAYER_BUSY_PIN, INPUT_PULLUP);
    
    /* WB2812B Ambient LEDs */
    FastLED.addLeds<WS2812B, WS2812B_LED_DATA_PIN, RGB>(ambientLeds, WS2812B_NR_OF_LEDS);
    FastLED.setBrightness(WB2812B_BRIGHTNESS);
    
    /* DEBUG REMOVE THIS */
    // for (uint8_t i = 0; i < WS2812B_NR_OF_LEDS; i++)
    // {
        // ambientLeds[i].setHSV(127, 220, 190);
    // }
    // FastLED.show();
       
    /* NFC reader init */
    SPI.begin();
    mfrc522.PCD_Init();
    Serial.println("MFRC522 Device Information:");
    mfrc522.PCD_DumpVersionToSerial();
    for (uint8_t i = 0; i < 6; i++)
    {
        key.keyByte[i] = 0xFF;
    }
    
    /* reset device settings on startup, if up down and play button is pressed */
    button_readAll();
    if (button_allPressed())
    {
        reset = true;
    }
    
    settings_loadFromEeprom(reset);
    volume = deviceSettings.volume[VOL_INI];
    mp3_begin();
    mp3_setVolume(volume);

    if (reset == true)
    {
        mp3_playMp3FolderTrack(MP3_SETTINGS_RESET_OK);
    }
    else
    {
        mp3_playMp3FolderTrack(MP3_STARTUP_SOUND);
    }
    
    settings_print();
    delay(STARTUP_DELAY);
    playFolder();
    
    /* if reset was done, wait till all buttons are released before proceeding with main loop */
    button_readAll();
    while (!button_allReleased())
    {
        button_readAll();
    }
}

/**
 * @brief The main loop is handle the button inputs and the sleep timer. 
 *        It also calls the mp3_loop cyclical to ensure that all incomming communication messages from DFPlayer Mini are processed as soon as possible.
 *        If a new nfc tag is detected, the nfc handler is called.
 */
void loop(void)
{    
    while (!mfrc522.PICC_IsNewCardPresent())
    {
        mp3_loop();
        buttonHandler();
        sleepTimer_handler();
        ambientLed_handler();
        delay(1);
    }
    nfc_handler();
}
