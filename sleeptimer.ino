/********************************************************************************
 * @file    : sleeptimer.ino
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
 * @brief The sleep timer handler is called cyclical by main loop to check if sleep timer elapsed.
 *        If so, the volume is decreased till zero and the sleep timer is deactivated.
 */
static void sleepTimer_handler(void)
{
    unsigned long currentTime    = 0;
    unsigned long sleepTime      = 0;
    static uint16_t muteCounter  = 0; /* used to mute music steadily */

    if (sleepTimerIsActive)
    {
        currentTime = millis();
        sleepTime = sleepTimerActivationTime + ((unsigned long)deviceSettings.sleepTimerMinutes  * 60000); /* x 60000 converts form minutes to ms */
        
#ifdef DEBUG
        static uint16_t printCounter = 0;
        if (printCounter >= 30) /* this value was choosen by trying to find a good serial print interval */
        {   
            DEBUG_PRINT(F("sleep timer time left: "));
            DEBUG_PRINT_LN((signed long)sleepTime - (signed long)currentTime);
            printCounter = 0;
        }
        else
        {
            printCounter++;
        }
#endif
            
        if (currentTime >= sleepTime)
        {
            if (muteCounter >= 50) /* this value was choosen by trying to find a good volume decrease interval */
            {
                muteCounter = 0;
                mp3_decreaseVolume();
                DEBUG_PRINT_LN(F("Decreasing volume"));
                
                if (mp3_getVolume() == 0)
                {
                    mp3_pause();
                    sleepTimer_disable();
                    DEBUG_PRINT_LN(F("sleeping now ...zzz"));
                }   
            }
            muteCounter++;
        }
    }
}

/**
 * @brief Enable the sleep timer. This method also enables the sleep timer indication led.
 */
static void sleepTimer_enable(void)
{
    DEBUG_TRACE;
    
    /* if sleepTimer is already active, do not reset */
    if (sleepTimerIsActive == false)
    {
        sleepTimerIsActive = true;
        sleepTimerActivationTime = millis();
        digitalWrite(SLEEP_TIMER_LED_PIN, HIGH);
        DEBUG_PRINT_LN(F("sleep timer enabled"));        
    }
}

/**
 * @brief Enable the sleep timer. This method also disables the sleep timer indication led.
 */
static void sleepTimer_disable(void)
{
    DEBUG_TRACE;
    
    sleepTimerIsActive = false;
    sleepTimerActivationTime = 0;
    mp3_setVolume(volume); /* reset volume if sleepTimer is deactivated while decreasing volume in sleepTimer_handler */
    digitalWrite(SLEEP_TIMER_LED_PIN, LOW);
    DEBUG_PRINT_LN(F("sleep timer disabled"));
}