static void adminMenu(void)
{
    DEBUG_TRACE;
    
    AdminMenuOptions_t menuOption = ADMIN_MENU_FIRST_OPTION;
    bool playMenuOption = true; /* used to play menu option track on first loop and if leaving submenu */

    mp3PlayMp3FolderTrack(MP3_ADMIN_MENU, DO_NOT_WAIT);
    delay(500);
        
    /* as long as intro is playing, it's possible to skip the intro with button up, down or play */
    readButtons();
    while (!buttonWasReleased(BUTTON_UP) && !buttonWasReleased(BUTTON_DOWN) && !buttonWasReleased(BUTTON_PLAY) && mp3IsPlaying())
    {
        /* DEBUG REMOVE THIS */
        DEBUG_PRINT(buttonWasReleased(BUTTON_UP));
        DEBUG_PRINT(buttonWasReleased(BUTTON_DOWN));
        DEBUG_PRINT(buttonWasReleased(BUTTON_PLAY));
        DEBUG_PRINT(mp3IsPlaying());
        DEBUG_PRINT_LN(!buttonWasReleased(BUTTON_UP) && !buttonWasReleased(BUTTON_DOWN) && !buttonWasReleased(BUTTON_PLAY) && mp3IsPlaying());
        delay(50);
        readButtons();
    }
   
    readButtons();
    while(!buttonPressedFor(BUTTON_PLAY, BUTTON_LONG_PRESS_TIME))
    {
        readButtons();
        mp3Loop();
          
        if (buttonWasReleased(BUTTON_UP) && (menuOption < (ADMIN_MENU_LAST_OPTION)))
        {
            menuOption = menuOption + 1;
            DEBUG_PRINT(F("menu option up to: "));
            DEBUG_PRINT_LN(menuOption);
        }
        else if (buttonWasReleased(BUTTON_DOWN) && (menuOption > ADMIN_MENU_FIRST_OPTION))
        {
            menuOption = menuOption - 1;
            DEBUG_PRINT(F("menu option down to: "));
            DEBUG_PRINT_LN(menuOption);
        }
        
        if (buttonWasReleased(BUTTON_UP) || buttonWasReleased(BUTTON_DOWN) || (playMenuOption == true))
        {    
            playMenuOption = false;
            mp3PlayMp3FolderTrack(menuOption + 1); /* play menu number (1, 2 ...) */
            mp3PlayMp3FolderTrack(MP3_ADMIN_MENU_OPTIONS_ARRAY[menuOption], DO_NOT_WAIT);             
            DEBUG_PRINT(F("menu option: "));
            DEBUG_PRINT_LN(menuOption);
        }
         
        if (buttonWasReleased(BUTTON_PLAY))
        {
            switch(menuOption)
            {
                case ADMIN_MENU_RESET_NFC_TAG:
                    resetNfcTag();
                    break;
                    
                case ADMIN_MENU_SET_VOL_MAX:
                    /* fall through */
                case ADMIN_MENU_SET_VOL_MIN:
                    /* fall through */
                case ADMIN_MENU_SET_VOL_INI:
                    adminMenu_setVolume(menuOption);
                    break;
                    
                case ADMIN_MENU_SET_SLEEP_TIMER:
                    adminMenu_setSleepTimer();
                    break;
                    
                case ADMIN_MENU_SETTINGS_RESET:
                    adminMenu_resetDeviceSettings();
                    break;
                    
                default:
                    /* nothing to do */
                    break;
            }
            
            playMenuOption = true;
        }
    }
    mp3PlayMp3FolderTrack(MP3_ADMIN_MENU_EXIT_SELECTED);
    DEBUG_PRINT_LN(F("BUTTON LONG PRESS"));
}

static void adminMenu_setVolume(AdminMenuOptions_t menuOption)
{
    uint8_t volTemp = 0;
    uint16_t voiceTrack = MP3_ADMIN_MENU_OPTIONS_ARRAY[menuOption];
    
    VolumeSetting_t volSelector = VOL_MAX;
    
    switch(menuOption)
    {
    default: /* fall through */
    case ADMIN_MENU_SET_VOL_MAX: volSelector = VOL_MAX; break;
    case ADMIN_MENU_SET_VOL_MIN: volSelector = VOL_MIN; break;
    case ADMIN_MENU_SET_VOL_INI: volSelector = VOL_INI; break;
    }
    
    volTemp = deviceSettings.volume[volSelector];   
    
    voiceTrack++;
    mp3PlayMp3FolderTrack(voiceTrack, DO_NOT_WAIT); /* MP3_VOL_..._SELECTED */
    
    readButtons();
    while (!buttonWasReleased(BUTTON_PLAY))
    {
        readButtons();
        mp3Loop();
        
        if (buttonWasReleased(BUTTON_UP) && (volTemp < DF_PLAYER_MAX_VOL))
        {
            volTemp++;                
        }
        else if (buttonWasReleased(BUTTON_DOWN) && (volTemp > 1)) /* it should not be possible to set max, min or init volume to zero (may cant here admin menu anymore on next startup) */
        {
            volTemp--;
        }
        
        if (buttonWasReleased(BUTTON_UP) || buttonWasReleased(BUTTON_DOWN))
        {
            mp3SetVolume(volTemp);
            mp3PlayMp3FolderTrack(volTemp, DO_NOT_WAIT);
        }
    }
    
    deviceSettings.volume[volSelector] = volTemp;
    
    writeSettingsToFlash();
    mp3SetVolume(volume);
    voiceTrack++;
    mp3PlayMp3FolderTrack((voiceTrack)); /* MP3_VOL_..._OK */
}

static void adminMenu_setSleepTimer(void)
{
    /* TODO ASSERT deviceSettings.sleepTimerMinutes % 5 == 0 */
    mp3PlayMp3FolderTrack(MP3_SLEEP_TIMER_SELECTED, DO_NOT_WAIT);
    
    readButtons();
    while (!buttonWasReleased(BUTTON_PLAY))
    {
        readButtons();
        mp3Loop();
        
        if (buttonWasReleased(BUTTON_UP) && (deviceSettings.sleepTimerMinutes < SLEEP_TIMER_MINUTES_MAX))
        {
            deviceSettings.sleepTimerMinutes += SLEEP_TIMER_MINUTES_INTERVAL;                
        }
        else if (buttonWasReleased(BUTTON_DOWN) && (deviceSettings.sleepTimerMinutes > SLEEP_TIMER_MINUTES_INTERVAL))
        {
             deviceSettings.sleepTimerMinutes -= SLEEP_TIMER_MINUTES_INTERVAL;
        }
        
        if (buttonWasReleased(BUTTON_UP) || buttonWasReleased(BUTTON_DOWN))
        {
            mp3PlayMp3FolderTrack(deviceSettings.sleepTimerMinutes, DO_NOT_WAIT);
        }
    }
    
    writeSettingsToFlash();
    mp3PlayMp3FolderTrack((MP3_SLEEP_TIMER_OK));
}

static void adminMenu_resetDeviceSettings(void)
{
    resetSettings();
    mp3PlayMp3FolderTrack(MP3_SETTINGS_RESET_OK);
}

static void enterAdminMenu(void)
{   
    DEBUG_TRACE;
    
    bool abort = false;
    bool keyCardDetected = false;
    ButtonNr_t pinCodeEntered[PIN_CODE_LENGTH] = {};
    uint8_t pinCodeIndex = 0;
    
    mp3Pause();
    sleepTimerDisable();
    nfcConfigInProgess = true;
    
    mp3PlayMp3FolderTrack(MP3_ADMIN_MENU_PINCODE, DO_NOT_WAIT);
    
    /* wait for all buttons are released before next action */
    while (buttonIsPressed(BUTTON_DOWN) || buttonIsPressed(BUTTON_UP) || buttonIsPressed(BUTTON_PLAY))
    {
        readButtons();
    }
    
    /* read pincode from button input and keycard */
    while (!keyCardDetected && (pinCodeIndex < PIN_CODE_LENGTH))
    {
        mp3Loop();
        readButtons();
        
        if (checkForKeyCard() == true)
        {
            keyCardDetected = true;
        }
        else if (buttonWasReleased(BUTTON_DOWN))
        {
            pinCodeEntered[pinCodeIndex] = BUTTON_DOWN;
            DEBUG_PRINT_LN(F("DOWN"));
        }
        else if (buttonWasReleased(BUTTON_UP))
        {
            pinCodeEntered[pinCodeIndex] = BUTTON_UP;
            DEBUG_PRINT_LN(F("UP"));
        }
        else if (buttonWasReleased(BUTTON_PLAY))
        {
            pinCodeEntered[pinCodeIndex] = BUTTON_PLAY;
            DEBUG_PRINT_LN(F("PLAY"));
        }
        
        if (buttonWasReleased(BUTTON_DOWN) || buttonWasReleased(BUTTON_UP) || buttonWasReleased(BUTTON_PLAY))
        {
            DEBUG_PRINT_LN(F("INCREASE INDEX"));
            pinCodeIndex++;
            mp3PlayMp3FolderTrack(MP3_BEEP);
        }
    }
    
    if (keyCardDetected || pinCompare(pinCodeEntered, PIN_CODE))
    {
        adminMenu();
    }
    else
    {
        mp3PlayMp3FolderTrack(MP3_PIN_CODE_WRONG);
    }
    
    nfcConfigInProgess = false;
}

static bool pinCompare(const ButtonNr_t* enteredCode, const ButtonNr_t* pinCode)
{
    DEBUG_TRACE
    
    bool retVal = true;
     
    for (uint8_t i = 0; (i < PIN_CODE_LENGTH) && (retVal == true); i++)
    {
        if (enteredCode[i] != pinCode[i])
        {
            retVal = false;
        }
    }
    
    DEBUG_PRINT(F(", pin code result (0 -> incorrect, 1-> correct): "));
    DEBUG_PRINT_LN(retVal);
    
    return retVal;
}

static bool checkForKeyCard(void)
{
    bool retVal = false;
    NfcTagObject_t nfcTag;
    
    if (!mfrc522.PICC_IsNewCardPresent())
    {
        return retVal;
    }
    
    if (!mfrc522.PICC_ReadCardSerial())
    {
        /* do nothing */
    }
    else if (!readNfcTag(&nfcTag))
    {
        /* do nothing */
    }
    else if ((nfcTag.cookie == GOLDEN_COOKIE) && (nfcTag.folderSettings.mode == MODE_KEYCARD))  /* rename mode to MODE_KEYCARD and remove locked and admin */
    {
        retVal = true;
    }
    
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    
    return retVal;
}     