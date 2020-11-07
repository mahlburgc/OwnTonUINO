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
#include <avr/sleep.h>


/********************************************************************************
 * defines
 ********************************************************************************/
#define DEBUG /* COMMENT WHEN NOT IN DEBUG MODE */

/* Buttons */
#define BUTTON_PAUSE_PIN    A0
#define BUTTON_UP_PIN       A2
#define BUTTON_DOWN_PIN     A1
#define BUTTON_NEXT_PIN     A4
#define BUTTON_PREV_PIN     A3
/* MFRC522 */
#define RFID_RST_PIN        9
#define RFID_SS_PIN         10
/* DF PLAYER */
#define DF_PLAYER_RX_PIN    2
#define DF_PLAYER_TX_PIN    3
#define DF_PLAYER_BUSY_PIN  4
/* ADC */
#define OPEN_ANALOG_PIN     A7  /* used for random number generation */

#define BUTTON_LONG_PRESS   1000 /* ms */

#ifdef  DEBUG
#define DEBUG_PRINT(x)      Serial.print(x)
#define DEBUG_PRINT_LN(x)   Serial.println(x)
#else
#define DEBUG_PRINT(x)      ((void)0U)
#define DEBUG_PRINT_LN(x)   ((void)0U)
#endif

#define UNUSED(x)           (void)x;

#define SERIAL_BAUD         115200

/********************************************************************************
 * static memeber
 ********************************************************************************/

typedef enum 
{
    MODE_ALBUM,       /* play whole folder in normal sequence */
    MODE_SHUFFLE,     /* play whole folder in random sequence */
    MODE_AUDIO_BOOK,  /* play whole folder in normal sequence from last playes track */
} ListenMode_t;

typedef struct 
{
  uint8_t      number;
  ListenMode_t mode;
} FolderSettings_t;

typedef struct 
{
  uint8_t volumeMin;
  uint8_t volumeMax;
  uint8_t volumeInit;
  long    sleepTimer;
} DeviceSettings_t;
 
/* DFPlayer Mini */
static SoftwareSerial mySoftwareSerial(DF_PLAYER_RX_PIN, DF_PLAYER_TX_PIN); // RX, TX
static uint16_t numTracksInFolder;
static uint16_t currentTrack;
static uint8_t queue[255];
static uint8_t volume;

static FolderSettings_t folder;
static DeviceSettings_t deviceSettings;

/* Buttons */
Button buttonPause(BUTTON_PAUSE_PIN);
Button buttonUp(BUTTON_UP_PIN);
Button buttonDown(BUTTON_DOWN_PIN);
Button buttonNext(BUTTON_NEXT_PIN);
Button buttonPrev(BUTTON_PREV_PIN);

/* foreward decalarations */
static void nextTrack(void);
static void previousTrack(void);
static void volumeUp(void);
static void volumeDown(void);
static void readButtons(void);

class Mp3Notify 
{
public:
    static void OnError(uint16_t errorCode)
    {
      DEBUG_PRINT_LN();
      DEBUG_PRINT("Com Error ");       /* see DfMp3_Error for code meaning */
      DEBUG_PRINT_LN(errorCode);
    }
    
    static void PrintlnSourceAction(DfMp3_PlaySources source, const char* action) 
    {
      if (source & DfMp3_PlaySources_Sd) DEBUG_PRINT("SD Karte ");
      if (source & DfMp3_PlaySources_Usb) DEBUG_PRINT("USB ");
      if (source & DfMp3_PlaySources_Flash) DEBUG_PRINT("Flash ");
      DEBUG_PRINT_LN(action);
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
      PrintlnSourceAction(source, "bereit");
    }
    
    static void OnPlaySourceRemoved(DfMp3_PlaySources source)
    {
      PrintlnSourceAction(source, "entfernt");
    }
};

static DFMiniMp3<SoftwareSerial, Mp3Notify> mp3(mySoftwareSerial);

static void nextTrack(void)
{
    DEBUG_PRINT_LN(F("== nextTrack()"));
      
    if (folder.mode == MODE_ALBUM)
    {
        if (currentTrack < numTracksInFolder)
        {
            currentTrack = currentTrack + 1;
            mp3.playFolderTrack(folder.number, currentTrack);
            DEBUG_PRINT(F("Albummodus ist aktiv -> nächster Track: "));
            DEBUG_PRINT(currentTrack);
        } 
    }
    
    if (folder.mode == MODE_SHUFFLE)
    {
        if (currentTrack < numTracksInFolder)
        {
            DEBUG_PRINT(F("Shuffle -> weiter in der Queue "));
            currentTrack++;
        }
        else
        {
            DEBUG_PRINT_LN(F("Ende der Shuffle Queue -> beginne von vorne"));
            currentTrack = 1;
        }
        mp3.playFolderTrack(folder.number, queue[currentTrack - 1]);
    }

    if (folder.mode == MODE_AUDIO_BOOK)
    {
        if (currentTrack < numTracksInFolder)
        {
            currentTrack = currentTrack + 1;
            mp3.playFolderTrack(folder.number, currentTrack);
            /* EEPROM.update(folder.number, currentTrack); TODO */
            
            DEBUG_PRINT(F("Hörbuch Modus ist aktiv -> nächster Track und Fortschritt speichern"));
            DEBUG_PRINT_LN(currentTrack);
        } 
        else
        {
            /* TODO EEPROM.update(folder.number, 1); */
        }
    }
    delay(500);
}

static void previousTrack(void)
{
    DEBUG_PRINT_LN(F("== previousTrack()"));

    if (folder.mode == MODE_ALBUM)
    {
        DEBUG_PRINT_LN(F("Albummodus ist aktiv -> vorheriger Track"));
        if (currentTrack > 1)
        {
          currentTrack = currentTrack - 1;
        }
        mp3.playFolderTrack(folder.number, currentTrack);
    }
    
    if (folder.mode == MODE_SHUFFLE)
    {
        if (currentTrack > 1)
        {
          DEBUG_PRINT(F("Shuffle Modus ist aktiv -> zurück in der Qeueue "));
          currentTrack--;
        }
        else
        {
            DEBUG_PRINT(F("Anfang der Queue -> springe ans Ende "));
            currentTrack = numTracksInFolder;
        }
        DEBUG_PRINT_LN(queue[currentTrack - 1]);
        mp3.playFolderTrack(folder.number, queue[currentTrack - 1]);
    }
    
    if (folder.mode == MODE_AUDIO_BOOK)
    {
        DEBUG_PRINT_LN(F("Hörbuch Modus ist aktiv -> vorheriger Track und Fortschritt speichern"));
        if (currentTrack > 1)
        {
          currentTrack = currentTrack - 1;
        }
        mp3.playFolderTrack(folder.number, currentTrack);
        /*EEPROM.update(folder.number, currentTrack);  TODO */
    }
    delay(500);
}

static void volumeUp(void)
{
    DEBUG_PRINT_LN(F("== volumeUp()"));
    
    if (volume < deviceSettings.volumeMax) 
    {
        mp3.increaseVolume();
        volume++;
    }
    DEBUG_PRINT_LN(volume);
}

static void volumeDown(void)
{
    DEBUG_PRINT_LN(F("== volumeDown()"));
    
    if (volume > deviceSettings.volumeMin)
    {
        mp3.decreaseVolume();
        volume--;
    }
    DEBUG_PRINT_LN(volume);
}

static void readButtons(void)
{
    buttonPause.read();
    buttonUp.read();
    buttonDown.read();
    buttonNext.read();
    buttonPrev.read();
}

static void buttonHandler(void)
{
    readButtons();
    
    if (buttonPause.wasReleased())
    {
        if (isPlaying())
        {
            mp3.pause();
        }
        else
        {
            mp3.start();
        }
    } 
    
    if (buttonUp.wasReleased()) 
    {
        volumeUp();
    }

    if (buttonDown.wasReleased())
    {
        volumeDown();
    }

    if (buttonNext.wasReleased())
    {
        if (isPlaying())
        {
            nextTrack();
        }
    }
    
    if (buttonPrev.wasReleased())
    {
        if (isPlaying())
        {
            previousTrack();
        }
    }
}

bool isPlaying()
{
  return !digitalRead(DF_PLAYER_BUSY_PIN);
}

void setup(void)
{
  Serial.begin(SERIAL_BAUD);

  DEBUG_PRINT_LN(F("initializing..."));

  folder =
  {
    .number = 1,
    .mode   = MODE_ALBUM,
  };

  deviceSettings =
  {
    .volumeMin = 0,
    .volumeMax = 15,
    .volumeInit = 6,
    .sleepTimer = 0,
  };
  
  volume = deviceSettings.volumeInit;
  currentTrack = 1;
  
  mp3.begin();
  delay(2000);
  mp3.setVolume(volume);
  numTracksInFolder = mp3.getFolderTrackCount(folder.number);

  pinMode(BUTTON_PAUSE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_NEXT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PREV_PIN, INPUT_PULLUP);
  pinMode(DF_PLAYER_BUSY_PIN, INPUT);

  DEBUG_PRINT_LN(F("starting..."));
  
  mp3.playFolderTrack(folder.number, currentTrack); // random of all folders on sd
}

void loop(void)
{
    mp3.loop();
    buttonHandler();
}
