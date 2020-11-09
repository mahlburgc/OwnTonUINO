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
}