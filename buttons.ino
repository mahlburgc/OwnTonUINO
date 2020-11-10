static bool buttonWasReleased(ButtonNr_t buttonNr)
{
    bool retVal = false;
    
    bool buttonState = button[buttonNr].wasReleased();
    
    //DEBUG_PRINT(F("Button "));
    //DEBUG_PRINT(buttonNr);
    //DEBUG_PRINT(F(" released: "));
    //DEBUG_PRINT(buttonState);
    //DEBUG_PRINT(F(", long pressed: "));
    //DEBUG_PRINT_LN(buttonLongPressed[buttonNr]);
    
    if (buttonState)
    {        
        if(buttonLongPressed[buttonNr] == true) /* ignore release if button up was pressed for long time */
        {
            buttonLongPressed[buttonNr] = false;
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
    
    if (button[buttonNr].pressedFor(ms))
    {
        buttonLongPressed[buttonNr] = true;
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