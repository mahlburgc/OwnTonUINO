static void adminMenu(void)
{
    DEBUG_TRACE;
    
    AdminMenuOptions_t menuOption = ADMIN_MENU_FIRST_OPTION;
    
    mp3Pause();
    sleepTimerDisable();
    nfcConfigInProgess = true;

    mp3PlayMp3FolderTrack(MP3_ADMIN_MENU);
    mp3PlayMp3FolderTrack(MP3_ADMIN_MENU_CHOOSE_OPTION);
    mp3PlayMp3FolderTrack(MP3_ADMIN_MENU_EXIT_INTRO);
    delay(500);
    mp3PlayMp3FolderTrack(1);
    mp3PlayMp3FolderTrack(MP3_CARD_RESET_INTRO, DO_NOT_WAIT);    
    
    readButtons();
    while(!buttonPressedFor(BUTTON_PLAY, BUTTON_LONG_PRESS_TIME))
    {
        readButtons();
        mp3Loop();
        
        //TODO rebuild while with intro is recalled her after executing an option

        if (buttonWasReleased(BUTTON_UP) && (menuOption < (ADMIN_MENU_LAST_OPTION)))
        {
            menuOption = menuOption + 1;
        }
        else if (buttonWasReleased(BUTTON_DOWN) && (menuOption > ADMIN_MENU_FIRST_OPTION))
        {
            menuOption = menuOption - 1;
        }
        
        if (buttonWasReleased(BUTTON_UP) || buttonWasReleased(BUTTON_DOWN))
        {
            mp3PlayMp3FolderTrack(menuOption + 1); /* play menu number (1, 2 ...) */
            mp3PlayMp3FolderTrack(MP3_ADMIN_MENU_OPTIONS_ARRAY[menuOption], DO_NOT_WAIT);
        }
         
        switch(menuOption)
        {
        case ADMIN_MENU_RESET_NFC_TAG:
            adminMenu_resetNfcTag();
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
    }
    mp3PlayMp3FolderTrack(MP3_ADMIN_MENU_EXIT_SELECTED);
    DEBUG_PRINT_LN(F("BUTTON LONG PRESS"));
    nfcConfigInProgess = false;
}

static void adminMenu_resetNfcTag(void)
{
    if (buttonWasReleased(BUTTON_PLAY))
    {
        resetNfcTag();
        mp3PlayMp3FolderTrack(MP3_ADMIN_MENU_CHOOSE_OPTION);
    }
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
    
    if (buttonWasReleased(BUTTON_PLAY))
    {
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
                mp3PlayMp3FolderTrack(volTemp);
            }
        }
        
        deviceSettings.volume[volSelector] = volTemp;
        
        writeSettingsToFlash();
        mp3SetVolume(volume);
        voiceTrack++;
        mp3PlayMp3FolderTrack((voiceTrack)); /* MP3_VOL_..._OK */
        mp3PlayMp3FolderTrack(MP3_ADMIN_MENU_CHOOSE_OPTION);
    }
}

static void adminMenu_setSleepTimer(void)
{
    /* TODO ASSERT deviceSettings.sleepTimerMinutes % 5 == 0 */
    
    if (buttonWasReleased(BUTTON_PLAY))
    {
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
                mp3PlayMp3FolderTrack(deviceSettings.sleepTimerMinutes);
            }
        }
        
        writeSettingsToFlash();
        mp3PlayMp3FolderTrack((MP3_SLEEP_TIMER_OK));
        mp3PlayMp3FolderTrack(MP3_ADMIN_MENU_CHOOSE_OPTION);
    }
}

static void adminMenu_resetDeviceSettings(void)
{
    uint16_t newSleepTimerMinutes = deviceSettings.sleepTimerMinutes;
    
    if (buttonWasReleased(BUTTON_PLAY))
    {
        resetSettings();
        mp3PlayMp3FolderTrack(MP3_SETTINGS_RESET_OK);
        mp3PlayMp3FolderTrack(MP3_ADMIN_MENU_CHOOSE_OPTION);
    }
}