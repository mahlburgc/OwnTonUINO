static bool buttonWasReleased(ButtonNr_t buttonNr)
{
    bool retVal = false;
    
    button[buttonNr].read();
    
    if (button[buttonNr].wasReleased())
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
    
    button[buttonNr].read();
    
    if (button[buttonNr].pressedFor(ms))
    {
        buttonLongPressed[buttonNr] = true;
        retVal = true;
    }
    return retVal;
}

static bool buttonWasPressed(ButtonNr_t buttonNr)
{
    button[buttonNr].read();
    return button[buttonNr].wasPressed();
}

static bool buttonIsPressed(ButtonNr_t buttonNr)
{
    button[buttonNr].read();
    return button[buttonNr].isPressed();
}

static void readButtons(void)
{
    for (uint8_t i = 0; i < NR_OF_BUTTONS; i++)
    {
        button[i].read();
    }
}