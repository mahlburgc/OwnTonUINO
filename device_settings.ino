static void writeSettingsToFlash(void) 
{
    DEBUG_TRACE;
    
    EEPROM.put(EEPROM_DEVICE_SETTINGS_ADDR, deviceSettings);
}

static void loadSettingsFromFlash(bool reset)
{
    DEBUG_TRACE;
    
    EEPROM.get(EEPROM_DEVICE_SETTINGS_ADDR, deviceSettings);
    
    if ((deviceSettings.version != VERSION) || (reset == true))
    {
        resetSettings();
    }
}

static void resetSettings(void)
{
    DEBUG_TRACE;
    
    deviceSettings.version           = VERSION;
    deviceSettings.volume[VOL_MAX]   = VOL_MAX_PRESET;
    deviceSettings.volume[VOL_MIN]   = VOL_MIN_PRESET;
    deviceSettings.volume[VOL_INI]   = VOL_INI_PRESET;
    deviceSettings.sleepTimerMinutes = SLEEP_TIMER_MINUTES_PRESET;

    writeSettingsToFlash();
}

static void printDeviceSettings(void)
{
    DEBUG_PRINT(F("version: "));
    DEBUG_PRINT_LN(deviceSettings.version);

    DEBUG_PRINT(F("volume max: "));
    DEBUG_PRINT_LN(deviceSettings.volume[VOL_MAX]);

    DEBUG_PRINT(F("volume min: "));
    DEBUG_PRINT_LN(deviceSettings.volume[VOL_MIN]);

    DEBUG_PRINT(F("volume initial: "));
    DEBUG_PRINT_LN(deviceSettings.volume[VOL_INI]);

    DEBUG_PRINT(F("sleep timer: "));
    DEBUG_PRINT_LN(deviceSettings.sleepTimerMinutes);
}
