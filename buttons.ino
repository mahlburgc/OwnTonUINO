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
    bool buttonState = button[buttonNr].wasReleased();

    if (buttonState)
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
 * @brief Check if button was for an amount of millis. Next button long press can just be detected 
 *        if button was released before and button_wasReleased() method was called.
 */
static bool button_pressedFor(ButtonNr_t buttonNr, uint32_t ms)
{
    bool retVal = false;
    
    /* avoid detecting more than one long button press if button is pressed for very long time without release */
    if (button[buttonNr].pressedFor(ms) && (ignoreNextButtonRelease[buttonNr] == false))
    {
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