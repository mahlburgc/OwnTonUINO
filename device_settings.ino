static void writeSettingsToFlash(void) 
{
    DEBUG_TRACE;
    
    EEPROM.put(EEPROM_DEVICE_SETTINGS_ADDR, deviceSettings);
}

static void loadSettingsFromFlash(void)
{
    DEBUG_TRACE;
    
    EEPROM.get(EEPROM_DEVICE_SETTINGS_ADDR, deviceSettings);
    
    if (deviceSettings.version != VERSION)
    {
        resetSettings();
    }
}

static void resetSettings(void)
{
    DEBUG_TRACE;
    
    deviceSettings.version      = VERSION;
    deviceSettings.volumeMin    = VOLUME_MIN_PRESET;
    deviceSettings.volumeMax    = VOLUME_MAX_PRESET;
    deviceSettings.volumeInit   = VOLUME_INIT_PRESET;
    deviceSettings.sleepTimer   = SLEEP_TIMER_PRESET;

    writeSettingsToFlash();
    mp3PlayMp3FolderTrack(MP3_RESET_OK);
}

static void printDeviceSettings(void)
{
    DEBUG_PRINT(F("version: "));
    DEBUG_PRINT_LN(deviceSettings.version);

    DEBUG_PRINT(F("volume max: "));
    DEBUG_PRINT_LN(deviceSettings.volumeMax);

    DEBUG_PRINT(F("volume min: "));
    DEBUG_PRINT_LN(deviceSettings.volumeMin);

    DEBUG_PRINT(F("volume initial: "));
    DEBUG_PRINT_LN(deviceSettings.volumeInit);

    DEBUG_PRINT(F("sleep timer: "));
    DEBUG_PRINT_LN(deviceSettings.sleepTimer);
}