/********************************************************************************
 * @file    : menu.ino
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
 ********************************************************************************/

/********************************************************************************
 * private methods
 ********************************************************************************/
/**
 * @brief This method handles the admin menu.
 */
static void adminMenu_main(void)
{
    DEBUG_TRACE;
    
    AdminMenuOptions_t menuOption = ADMIN_MENU_FIRST_OPTION;
    bool playMenuOption = true; /* used to play menu option track on first loop and if leaving submenu */

    mp3_playMp3FolderTrack(MP3_ADMIN_MENU, DO_NOT_WAIT);
    delay(500);
        
    /* as long as intro is playing, it's possible to skip the intro with button up, down or play */
    button_readAll();
    while (!button_wasReleased(BUTTON_UP) && !button_wasReleased(BUTTON_DOWN) && !button_wasReleased(BUTTON_PLAY) && mp3_isPlaying())
    {
        button_readAll();
    }
   
    button_readAll();
    while(!button_pressedFor(BUTTON_PLAY, BUTTON_LONG_PRESS_TIME))
    {
        button_readAll();
        mp3_loop();
          
        if (button_wasReleased(BUTTON_UP) && (menuOption < (ADMIN_MENU_LAST_OPTION)))
        {
            menuOption = menuOption + 1;
        }
        else if (button_wasReleased(BUTTON_DOWN) && (menuOption > ADMIN_MENU_FIRST_OPTION))
        {
            menuOption = menuOption - 1;
        }
        
        if (button_wasReleased(BUTTON_UP) || button_wasReleased(BUTTON_DOWN) || (playMenuOption == true))
        {    
            playMenuOption = false;
            mp3_playMp3FolderTrack(menuOption + 1); /* play menu number (1, 2 ...) */
            mp3_playMp3FolderTrack(MP3_ADMIN_MENU_OPTIONS_ARRAY[menuOption], DO_NOT_WAIT);             
        }
         
        if (button_wasReleased(BUTTON_PLAY))
        {
            switch(menuOption)
            {
                case ADMIN_MENU_RESET_NFC_TAG:
                    nfc_resetTag();
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
    mp3_playMp3FolderTrack(MP3_ADMIN_MENU_EXIT_SELECTED);
}

/**
 * @brief This method handles the submenu for volume configuration.
 *        With this submenu, minimum, maximum and initial volume can be configured.       
 */
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
    mp3_playMp3FolderTrack(voiceTrack, DO_NOT_WAIT); /* MP3_VOL_..._SELECTED */
    
    button_readAll();
    while (!button_wasReleased(BUTTON_PLAY))
    {
        button_readAll();
        mp3_loop();
        
        if (button_wasReleased(BUTTON_UP) && (volTemp < DF_PLAYER_MAX_VOL))
        {
            volTemp++;                
        }
        else if (button_wasReleased(BUTTON_DOWN) && (volTemp > 1)) /* it should not be possible to set max, min or init volume to zero (may cant here admin menu anymore on next startup) */
        {
            volTemp--;
        }
        
        if (button_wasReleased(BUTTON_UP) || button_wasReleased(BUTTON_DOWN))
        {
            mp3_setVolume(volTemp);
            mp3_playMp3FolderTrack(volTemp, DO_NOT_WAIT);
        }
    }
    
    deviceSettings.volume[volSelector] = volTemp;
    
    settings_writeToEeprom();
    mp3_setVolume(volume);
    voiceTrack++;
    mp3_playMp3FolderTrack((voiceTrack)); /* MP3_VOL_..._OK */
}


/**
 * @brief This method handles the submenu for the sleep timer configuration.   
 */
static void adminMenu_setSleepTimer(void)
{
    /* TODO ASSERT deviceSettings.sleepTimerMinutes % 5 == 0 */
    mp3_playMp3FolderTrack(MP3_SLEEP_TIMER_SELECTED, DO_NOT_WAIT);
    
    button_readAll();
    while (!button_wasReleased(BUTTON_PLAY))
    {
        button_readAll();
        mp3_loop();
        
        if (button_wasReleased(BUTTON_UP) && (deviceSettings.sleepTimerMinutes < SLEEP_TIMER_MINUTES_MAX))
        {
            deviceSettings.sleepTimerMinutes += SLEEP_TIMER_MINUTES_INTERVAL;                
        }
        else if (button_wasReleased(BUTTON_DOWN) && (deviceSettings.sleepTimerMinutes > SLEEP_TIMER_MINUTES_INTERVAL))
        {
             deviceSettings.sleepTimerMinutes -= SLEEP_TIMER_MINUTES_INTERVAL;
        }
        
        if (button_wasReleased(BUTTON_UP) || button_wasReleased(BUTTON_DOWN))
        {
            mp3_playMp3FolderTrack(deviceSettings.sleepTimerMinutes, DO_NOT_WAIT);
        }
    }
    
    settings_writeToEeprom();
    mp3_playMp3FolderTrack((MP3_SLEEP_TIMER_OK));
}

/**
 * @brief This method handles the submenu for resetings all device settins. Configured nfc tags are still valid and usable.
 */
static void adminMenu_resetDeviceSettings(void)
{
    settings_reset();
    mp3_playMp3FolderTrack(MP3_SETTINGS_RESET_OK);
}

/**
 * @brief To enter the admin menu, this method should be called. Before entering the menu, 
 *        a pin must be entered or the keycard must be placed onto the nfc reader to open the admin menu.
 *
 *        TODO If five buttons are used, only on UP, DOWN and PLAY are used to detect pin code and only these pins trigger the button sound.
 */
static void adminMenu_enter(void)
{   
    DEBUG_TRACE;

    bool keyCardDetected = false;
    ButtonNr_t pinCodeEntered[PIN_CODE_LENGTH] = {};
    uint8_t pinCodeIndex = 0;
    
    mp3_pause();
    sleepTimer_disable();
    FastLED.showColor(CRGB::White);
    skipNextTrack = true;
    
    mp3_playMp3FolderTrack(MP3_ADMIN_MENU_PINCODE, DO_NOT_WAIT);
    
    /* wait for all buttons are released before next action */
    button_readAll();
    while (!button_allReleased())
    {
        button_readAll();
    }
    
    /* read pincode from button input and keycard */
    while (!keyCardDetected && (pinCodeIndex < PIN_CODE_LENGTH))
    {
        mp3_loop();
        button_readAll();
        
        if (checkForKeyCard() == true)
        {
            keyCardDetected = true;
        }
        else if (button_wasReleased(BUTTON_DOWN))
        {
            pinCodeEntered[pinCodeIndex] = BUTTON_DOWN;
        }
        else if (button_wasReleased(BUTTON_UP))
        {
            pinCodeEntered[pinCodeIndex] = BUTTON_UP;
        }
        else if (button_wasReleased(BUTTON_PLAY))
        {
            pinCodeEntered[pinCodeIndex] = BUTTON_PLAY;
        }
        
        if (button_wasReleased(BUTTON_DOWN) || button_wasReleased(BUTTON_UP) || button_wasReleased(BUTTON_PLAY))
        {
            pinCodeIndex++;
            mp3_playMp3FolderTrack(MP3_BEEP);
        }
    }
    
    if (keyCardDetected || adminMenu_pinCompare(pinCodeEntered, PIN_CODE))
    {
        adminMenu_main();
    }
    else
    {
        mp3_playMp3FolderTrack(MP3_PIN_CODE_WRONG);
    }
    
    FastLED.show();
    skipNextTrack = false;
}

/**
 * @brief This method is used to compare two pin codes.
 */
static bool adminMenu_pinCompare(const ButtonNr_t* enteredCode, const ButtonNr_t* pinCode)
{
    DEBUG_TRACE;
    
    ASSERT((enteredCode != NULL) && (pinCode == NULL));
    
    bool retVal = true;
     
    for (uint8_t i = 0; (i < PIN_CODE_LENGTH) && (retVal == true); i++)
    {
        if (enteredCode[i] != pinCode[i])
        {
            retVal = false;
        }
    }
    
    DEBUG_PRINT(F("pin code result (0 -> incorrect, 1-> correct): "));
    DEBUG_PRINT_LN(retVal);
    
    return retVal;
}