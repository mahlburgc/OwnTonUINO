/********************************************************************************
 * @file    : settings.ino
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
 * @brief Write device settings to eeprom.
 */
static void settings_writeToEeprom(void) 
{
    DEBUG_TRACE;
    
    EEPROM.put(EEPROM_DEVICE_SETTINGS_ADDR, deviceSettings);
}

/**
 * @brief Load device settings from eeprom.
 */
static void settings_loadFromEeprom(bool reset)
{
    DEBUG_TRACE;
    
    EEPROM.get(EEPROM_DEVICE_SETTINGS_ADDR, deviceSettings);
    
    if ((deviceSettings.version != FW_VERSION) || (reset == true))
    {
        settings_reset();
    }
}

/**
 * @brief Reset all device settings to presets.
 */
static void settings_reset(void)
{
    DEBUG_TRACE;
    
    deviceSettings.version           = FW_VERSION;
    deviceSettings.volume[VOL_MAX]   = VOL_MAX_PRESET;
    deviceSettings.volume[VOL_MIN]   = VOL_MIN_PRESET;
    deviceSettings.volume[VOL_INI]   = VOL_INI_PRESET;
    deviceSettings.sleepTimerMinutes = SLEEP_TIMER_MINUTES_PRESET;
    deviceSettings.ambientLedEnable  = AMBIENT_LED_ENABLE_PRESET;

    settings_writeToEeprom();
}

/**
 * @brief Print device settings.
 */
static void settings_print(void)
{
    Serial.println(F("TonUINO Device Information:"));
    Serial.print(F("  Firmware Version:     v"));
    Serial.print(deviceSettings.version >> 4);     /* MSB 4 bit -> main version */
    Serial.print(F("."));
    Serial.println(deviceSettings.version & 0x0F); /* LSB 4 bit -> sub version */

    Serial.print(F("  Volume Max:           "));
    Serial.println(deviceSettings.volume[VOL_MAX]);

    Serial.print(F("  Volume Min:           "));
    Serial.println(deviceSettings.volume[VOL_MIN]);

    Serial.print(F("  Volume Initial:       "));
    Serial.println(deviceSettings.volume[VOL_INI]);

    Serial.print(F("  Sleeptimer (minutes): "));
    Serial.println(deviceSettings.sleepTimerMinutes);
    
    Serial.print(F("  Ambient LED: "));
    if (deviceSettings.ambientLedEnable == true)
    {
        Serial.println(F("enabled"));
    }
    else 
    {
        Serial.println(F("disabled"));
    }
}
