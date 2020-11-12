static void adminMenu(void)
{
    DEBUG_TRACE;
 
    AdminMenuOptions_t menuOption = ADMIN_MENU_FIRST_OPTION;
    
    nfcConfigInProgess = true;
    mp3Pause();
    mp3PlayMp3FolderTrack(MP3_ADMIN_MENU);
    mp3PlayMp3FolderTrack(MP3_ADMIN_MENU_RESET_TAG);    
    
    readButtons();
    while(!buttonPressedFor(BUTTON_PLAY, BUTTON_LONG_PRESS_TIME))
    {
        readButtons();
        mp3Loop();

        if (buttonWasReleased(BUTTON_UP) && (menuOption < (ADMIN_MENU_LAST_OPTION)))
        {
            menuOption = (AdminMenuOptions_t)((uint8_t)menuOption + 1);
        }
        else if (buttonWasReleased(BUTTON_DOWN) && (menuOption > ADMIN_MENU_FIRST_OPTION))
        {
            menuOption = (AdminMenuOptions_t)((uint8_t)menuOption - 1);
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
           //adminMenu_setSleeptimer();
            break;
            
        default:
            /* nothing to do */
            break;
        }
    }
    mp3PlayMp3FolderTrack(MP3_ABORT);
    DEBUG_PRINT_LN(F("BUTTON LONG PRESS"));
    nfcConfigInProgess = false;
}

static void adminMenu_resetNfcTag(void)
{
    if (buttonWasReleased(BUTTON_UP) || buttonWasReleased(BUTTON_DOWN))
    {
        mp3PlayMp3FolderTrack(MP3_ADMIN_MENU_RESET_TAG);
    }
    if (buttonWasReleased(BUTTON_PLAY))
    {
        resetNfcTag();
        mp3PlayMp3FolderTrack(MP3_ADMIN_MENU);
    }
}

static void adminMenu_setVolume(AdminMenuOptions_t menuOption)
{
    uint8_t volTemp = 0;
    uint16_t voiceTrack = 0;
    uint16_t voiceTrack2 = 0;
    
    sleepTimerDisable();
    
    switch(menuOption)
    {
    default: /* fall through */
    case ADMIN_MENU_SET_VOL_MAX:
        voiceTrack = MP3_ADMIN_MENU_VOL_MAX;
        voiceTrack2 = MP3_ADMIN_MENU_VOL_MAX_SELECTED;
        volTemp = deviceSettings.volumeMax;
        break;
        
    case ADMIN_MENU_SET_VOL_MIN: 
        voiceTrack = MP3_ADMIN_MENU_VOL_MIN;
        voiceTrack2 = MP3_ADMIN_MENU_VOL_MIN_SELECTED;
        volTemp = deviceSettings.volumeMin;
        break;
        
    case ADMIN_MENU_SET_VOL_INI:
        voiceTrack = MP3_ADMIN_MENU_VOL_INI;
        voiceTrack2 = MP3_ADMIN_MENU_VOL_INI_SELECTED;
        volTemp = deviceSettings.volumeInit;                                              
        break;
    }
    
    if (buttonWasReleased(BUTTON_UP) || buttonWasReleased(BUTTON_DOWN))
    {
        mp3PlayMp3FolderTrack(voiceTrack); 
    }
    
    if (buttonWasReleased(BUTTON_PLAY))
    {
        mp3PlayMp3FolderTrack(voiceTrack2); 
        
        readButtons();
        while (!buttonWasReleased(BUTTON_PLAY))
        {
            readButtons();
            mp3Loop();
            
            if (buttonWasReleased(BUTTON_UP) && (volTemp < 30)) /*30 is max by datasheet TODO MACRO */
            {
                volTemp++;                
            }
            else if (buttonWasReleased(BUTTON_DOWN) && (volTemp > 0))
            {
                volTemp--;
            }
            
            if (buttonWasReleased(BUTTON_UP) || buttonWasReleased(BUTTON_DOWN))
            {
                mp3SetVolume(volTemp);
                mp3PlayMp3FolderTrack(volTemp);
            }
        }

        switch(menuOption)
        {
        default: /* fall through */
        case ADMIN_MENU_SET_VOL_MAX: deviceSettings.volumeMax = volTemp; break;
        case ADMIN_MENU_SET_VOL_MIN: deviceSettings.volumeMin = volTemp; break;
        case ADMIN_MENU_SET_VOL_INI: deviceSettings.volumeInit = volTemp; break;
        }
        
        writeSettingsToFlash();
        mp3SetVolume(volume);
        /* TODO add voice track which says "Die Einstellungen wurden gespeichert" */
        mp3PlayMp3FolderTrack(MP3_ADMIN_MENU);
    }
}