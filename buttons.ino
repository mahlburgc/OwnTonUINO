/********************************************************************************
 * @file    : buttons.ino
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
 * @brief Check if button was released. If button was pressed for long before, 
 *        the next button release method call is ignored.
 */
static bool button_wasReleased(ButtonNr_t buttonNr)
{
    bool retVal = false;

    if (button[buttonNr].wasReleased())
    {        
        if(ignoreNextButtonRelease[buttonNr] == true) /* ignore release if button up was pressed for long time */
        {
            DEBUG_PRINT_LN(F("this button release is ignored"));
            ignoreNextButtonRelease[buttonNr] = false;
        }
        else
        {
            retVal = true;
        }
    }
    return retVal;
}

/**
 * @brief Check if button was pressed for an amount of millis. 
 */
static bool button_pressedFor(ButtonNr_t buttonNr, uint32_t ms)
{
    bool retVal = false;
    static uint32_t lastLongPressDetect[NR_OF_BUTTONS] = { 0 };
    uint32_t currentTime = millis();
    
    if (button[buttonNr].pressedFor(ms) && (currentTime > (lastLongPressDetect[buttonNr] + ms)))
    {
        lastLongPressDetect[buttonNr] = currentTime;
        ignoreNextButtonRelease[buttonNr] = true;
        retVal = true;
    }
    
    return retVal;
}

/**
 * @brief Read all buttons.
 */
static void button_readAll(void)
{
    for (uint8_t i = 0; i < NR_OF_BUTTONS; i++)
    {
        button[i].read();
    }
}

/**
 * @brief Check if all buttons are pressed. 
 *
 *        TODO More precisely, UP, DOWN and PLAY are checked. Actually NEXT and PREV are ignored because 
 *        there should be the possibility to enable three Button mode.
 */
static bool button_allPressed(void)
{
    bool retVal = false;
    
    if (button[BUTTON_UP].isPressed() && button[BUTTON_DOWN].isPressed() && button[BUTTON_PLAY].isPressed())
    {
        DEBUG_PRINT_LN(F("BUTTONS UP DOWN AND PLAY ARE PRESSED!"));
        retVal = true;
    }
    
    return retVal;
}

/**
 * @brief Check if all buttons are released. 
 *
 *        TODO More precisely, UP, DOWN and PLAY are checked. Actually NEXT and PREV are ignored because 
 *        there should be the possibility to enable three Button mode.
 */
static bool button_allReleased(void)
{
    bool retVal = false;
    
    if (button[BUTTON_UP].isReleased() && button[BUTTON_DOWN].isReleased() && button[BUTTON_PLAY].isReleased())
    {
        DEBUG_PRINT_LN(F("BUTTONS UP DOWN AND PLAY ARE RELEASED!"));
        retVal = true;
    }
    
    return retVal;
}