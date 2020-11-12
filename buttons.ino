static bool buttonWasReleased(ButtonNr_t buttonNr)
{
    bool retVal = false;
    
    bool buttonState = button[buttonNr].wasReleased();
    
    //DEBUG_PRINT(F("Button "));
    //DEBUG_PRINT(buttonNr);
    //DEBUG_PRINT(F(" released: "));
    //DEBUG_PRINT(buttonState);
    //DEBUG_PRINT(F(", long pressed: "));
    //DEBUG_PRINT_LN(ignoreNextButtonRelease[buttonNr]);
    
    if (buttonState)
    {        
        if(ignoreNextButtonRelease[buttonNr] == true) /* ignore release if button up was pressed for long time or was used with isPressed */
        {
            ignoreNextButtonRelease[buttonNr] = false;
        }
        else
        {
            retVal = true;
        }
    }
    return retVal;
}

static bool buttonPressedFor(ButtonNr_t buttonNr, uint32_t ms)
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

static bool buttonWasPressed(ButtonNr_t buttonNr)
{
    return button[buttonNr].wasPressed();
}

static bool buttonIsPressed(ButtonNr_t buttonNr)
{
    return button[buttonNr].isPressed();
}

static void readButtons(void)
{
    for (uint8_t i = 0; i < NR_OF_BUTTONS; i++)
    {
        button[i].read();
    }
}

static bool allButtonsArePressed()
{
    bool retVal = false;
    
    if (buttonIsPressed(BUTTON_DOWN) && buttonIsPressed(BUTTON_UP) && buttonIsPressed(BUTTON_PLAY))
    {
        DEBUG_PRINT_LN(F("ALL BUTTONS ARE PRESSED!"));
        ignoreNextButtonRelease[BUTTON_DOWN] = true;
        ignoreNextButtonRelease[BUTTON_UP] = true;
        ignoreNextButtonRelease[BUTTON_PLAY] = true;
        retVal = true;
    }
    
    return retVal;
}
        