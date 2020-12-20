/********************************************************************************
 * @file    : nfc.ino
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
 * @brief The nfc handler handles an nfc tag, if placed on the reader within the main loop. 
 *        Reading the keycard for unlock the main menu and setup a tag within the main menu 
 *        is handled seperately in the respective methods.
 */
 static void nfc_handler(void)
{
    NfcTagObject_t nfcTag; /* my nfc object, e.g. nfc card or sticker */
    
    if (!mfrc522.PICC_IsNewCardPresent())
    {
        return;
    }
    else if (!mfrc522.PICC_ReadCardSerial())
    {
        return;
    }
    
    if (nfc_readTag(&nfcTag) == true) 
    {
        if (nfcTag.cookie != GOLDEN_COOKIE)
        {
            skipNextTrack = true;
            sleepTimer_disable();
            FastLED.showColor(CRGB::White);
            
            /* configure new card */  
            mp3_playMp3FolderTrack(MP3_NEW_TAG);
            skipMp3WithButtonPress();
            nfcTag = nfc_setupTag();
            if (IS_LISTENMODE(nfcTag.folderSettings.mode))
            {
                folder = nfcTag.folderSettings;
                playFolder();
            }
            FastLED.show();

        }
        else
        {
            /* handle known card */
            switch (nfcTag.folderSettings.mode)
            {
            case MODE_ALBUM:
                /* fall through */
            case MODE_SHUFFLE:
                /* fall through */
            case MODE_AUDIO_BOOK:
                if (!buttonsLocked)
                {
                    skipNextTrack = true;
                    mp3_playMp3FolderTrack(MP3_NEW_KNOWN_TAG);
                    folder = nfcTag.folderSettings;
                    playFolder();
                }
                break;
                
            case MODE_KEYCARD:
                buttonsLocked = !buttonsLocked;
                ambientLed_keylockAnimation();
                DEBUG_PRINT(F("Buttons locked (0->unlocked, 1-> locked): "));
                DEBUG_PRINT_LN(buttonsLocked);
                //mp3_playAdvertisement(ADV_BUTTONS_UNLOCKED + buttonsLocked); /* 300 for buttonsLocked = false, 301 for buttonsLocked = true */             
                break;
                
            default:
                /* do nothing */
                break;
            }
        }
    }
    
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    
    skipNextTrack = false;
}

/**
 * @brief This method reads the actual nfc tag, write the tag informations to the given nfc tag object pointer 
 *        and returns the read status.
 */
static bool nfc_readTag(NfcTagObject_t* nfcTag)
{
    DEBUG_TRACE;
    
    ASSERT(nfcTag != NULL);
    
    MFRC522::PICC_Type mifareType;
    const uint8_t BUFFER_SIZE    = 18;
    uint8_t buffer[BUFFER_SIZE]  = { 0 };
    uint8_t bufferSizeRetVal     = BUFFER_SIZE;
    uint32_t tempCookie = 0;

    /* show some details of the PICC */
    DEBUG_PRINT(F("Card UID:"));
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    DEBUG_PRINT_LN();
    DEBUG_PRINT(F("PICC type: "));
    mifareType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    DEBUG_PRINT_LN(mfrc522.PICC_GetTypeName(mifareType));

    /* authenticate using key A */
    if ((mifareType == MFRC522::PICC_TYPE_MIFARE_MINI) ||
        (mifareType == MFRC522::PICC_TYPE_MIFARE_1K)   ||
        (mifareType == MFRC522::PICC_TYPE_MIFARE_4K))
    {
        DEBUG_PRINT_LN(F("authenticating classic using key A..."));
        status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    }
    else
    {
        DEBUG_PRINT_LN(F("Invalid PICC Type! This FW only works with MIFARE CLASSIC MINI, 1K and 4K!"));
        return false;
    }

    if (status != MFRC522::STATUS_OK)
    {
        DEBUG_PRINT(F("PCD_Authenticate() failed: "));
        DEBUG_PRINT_LN(mfrc522.GetStatusCodeName(status));
        return false;
    }

    /* read data from the block */
    if ((mifareType == MFRC522::PICC_TYPE_MIFARE_MINI) ||
        (mifareType == MFRC522::PICC_TYPE_MIFARE_1K)   ||
        (mifareType == MFRC522::PICC_TYPE_MIFARE_4K))
    {
        DEBUG_PRINT(F("Reading data from block "));
        DEBUG_PRINT(blockAddr);
        DEBUG_PRINT_LN(F(" ..."));
        status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(blockAddr, buffer, &bufferSizeRetVal);
        if (status != MFRC522::STATUS_OK)
        {
            DEBUG_PRINT(F("MIFARE_Read() failed: "));
            DEBUG_PRINT_LN(mfrc522.GetStatusCodeName(status));
            return false;
        }
    }
  
    DEBUG_PRINT(F("Data on Card "));
    DEBUG_PRINT_LN(F(":"));
    dump_byte_array(buffer, BUFFER_SIZE);
    DEBUG_PRINT_LN();
    DEBUG_PRINT_LN();

    tempCookie  = (uint32_t)buffer[0] << 24;
    tempCookie += (uint32_t)buffer[1] << 16;
    tempCookie += (uint32_t)buffer[2] << 8;
    tempCookie += (uint32_t)buffer[3];

    nfcTag->cookie                = tempCookie;
    nfcTag->version               = buffer[4];
    nfcTag->folderSettings.number = buffer[5];
    nfcTag->folderSettings.mode   = (FolderMode_t)buffer[6];

    return true;
}

/**
 * @brief This method writes the given nfc tag informations to the placed tag.
 */
static bool nfc_writeTag(NfcTagObject_t nfcTag)
{
    DEBUG_TRACE;
    
    MFRC522::PICC_Type mifareType;
    const uint8_t BUFFER_SIZE    = 16;
    uint8_t buffer[BUFFER_SIZE]  = { 0 };
     
    buffer[0] = (uint8_t)(nfcTag.cookie >> 24);
    buffer[1] = (uint8_t)(nfcTag.cookie >> 16);
    buffer[2] = (uint8_t)(nfcTag.cookie >> 8);
    buffer[3] = (uint8_t)(nfcTag.cookie >> 0);
    buffer[4] = nfcTag.version;
    buffer[5] = nfcTag.folderSettings.number;
    buffer[6] = nfcTag.folderSettings.mode;

    mifareType = mfrc522.PICC_GetType(mfrc522.uid.sak);

    /* authentificate with the nfc tag object and set object specific parameters */
    if ((mifareType == MFRC522::PICC_TYPE_MIFARE_MINI ) ||
        (mifareType == MFRC522::PICC_TYPE_MIFARE_1K )   ||
        (mifareType == MFRC522::PICC_TYPE_MIFARE_4K ))
    {
        DEBUG_PRINT_LN(F("authenticating again using key A..."));
        status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    }
    else
    {
        DEBUG_PRINT_LN(F("Invalid PICC Type! This FW only works with MIFARE CLASSIC MINI, 1K and 4K!"));
        return false;
    }

    if (status != MFRC522::STATUS_OK)
    {
        DEBUG_PRINT(F("PCD_Authenticate() failed: "));
        DEBUG_PRINT_LN(mfrc522.GetStatusCodeName(status));
        mp3_playMp3FolderTrack(MP3_TAG_CONFIG_ERROR);
        return false;
    }

    /* write data to the block */
    DEBUG_PRINT(F("Writing data into block "));
    DEBUG_PRINT(blockAddr);
    DEBUG_PRINT_LN(F(" ..."));
    dump_byte_array(buffer, BUFFER_SIZE);
    DEBUG_PRINT_LN();

    if ((mifareType == MFRC522::PICC_TYPE_MIFARE_MINI) ||
        (mifareType == MFRC522::PICC_TYPE_MIFARE_1K)   ||
        (mifareType == MFRC522::PICC_TYPE_MIFARE_4K))
    {
        status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(blockAddr, buffer, BUFFER_SIZE);
    }

    if (status != MFRC522::STATUS_OK)
    {
        DEBUG_PRINT(F("MIFARE_Write() failed: "));
        DEBUG_PRINT_LN(mfrc522.GetStatusCodeName(status));
        mp3_playMp3FolderTrack(MP3_TAG_CONFIG_ERROR);
        return false;
    }
    else
    {
        mp3_playMp3FolderTrack(MP3_TAG_CONFIG_OK);
        return true;
    }
}

/**
 * @brief This method is used to setup a new or a configured nfc tag.
 */
static NfcTagObject_t nfc_setupTag(void)
{
    DEBUG_TRACE;
    
    FolderMode_t modeSelector = FIRST_MODE;
    uint16_t folderCount = mp3_getTotalFolderCount();
    uint16_t folderSelector = 1; /* can something between 1 and "number of folders" */

    NfcTagObject_t newNfcTag = 
    {
        .cookie = GOLDEN_COOKIE,
        .version = FW_VERSION,
        .folderSettings = 
        {
            .number = 0,
            .mode   = MODE_UNSET,
        }
    };
    
    /* setup listen mode or keycard nfc tag */
    mp3_playMp3FolderTrack(MP3_SELECT_MODE, DO_NOT_WAIT);
    skipMp3WithButtonPress();
    mp3_playMp3FolderTrack(MP3_MODE_ARRAY[FIRST_MODE - 1], DO_NOT_WAIT);
    
    DEBUG_PRINT_LN(modeSelector);
    DEBUG_PRINT(F("total folder count: "));
    DEBUG_PRINT(folderCount);
    
    button_readAll();
    while (!button_wasReleased(BUTTON_PLAY))
    {
        button_readAll();
        mp3_loop();

        if (button_wasReleased(BUTTON_UP) && (modeSelector < (LAST_MODE)))
        {
            modeSelector = modeSelector + 1;
            DEBUG_PRINT_LN(modeSelector);
        }
        else if (button_wasReleased(BUTTON_DOWN) && (modeSelector > FIRST_MODE))
        {
            modeSelector = modeSelector - 1;
            DEBUG_PRINT_LN(modeSelector);
        }
            
        if (button_wasReleased(BUTTON_UP) || button_wasReleased(BUTTON_DOWN))
        {
            mp3_playMp3FolderTrack(MP3_MODE_ARRAY[modeSelector - 1], DO_NOT_WAIT);
        }   
    }
    
    /* if selected mode is not a listen mode, the folder number is not needed */
    if (IS_LISTENMODE(modeSelector))
    {
        /* setup folder number */
        mp3_playMp3FolderTrack(MP3_SELECT_FOLDER, DO_NOT_WAIT);
        skipMp3WithButtonPress();
        mp3_playMp3FolderTrack(folderSelector, DO_NOT_WAIT);
        skipMp3WithButtonPress();
        mp3_playFolderTrack(folderSelector, 1);

        button_readAll();

        while (!button_wasReleased(BUTTON_PLAY))
        {
            button_readAll();
            mp3_loop();
            
            if (button_wasReleased(BUTTON_UP) && (folderSelector < folderCount))
            {
                folderSelector++;
            }
            else if (button_wasReleased(BUTTON_DOWN) && (folderSelector > 1))
            {
                folderSelector--;
            }
            
            if (button_wasReleased(BUTTON_UP) || button_wasReleased(BUTTON_DOWN))
            {
                mp3_playMp3FolderTrack(folderSelector, DO_NOT_WAIT);
                skipMp3WithButtonPress();
                mp3_playFolderTrack(folderSelector, 1);
            }
        }
    }

    newNfcTag.folderSettings.number = folderSelector;
    newNfcTag.folderSettings.mode = modeSelector;

    if (!nfc_writeTag(newNfcTag))
    {
        newNfcTag.folderSettings.number = 0;
        newNfcTag.folderSettings.mode = MODE_UNSET;
    }
    
    return newNfcTag;
}

/**
 * @brief This method is used by admin menu to reset a nfc tag. 
 *        This means, that the tag becomes a new configuration with nfc_setupTag().
 */
static void nfc_resetTag(void)
{    
    DEBUG_TRACE;
    
    NfcTagObject_t nfcTag;
    bool abort = false;
    
    mp3_playMp3FolderTrack(MP3_INSERT_TAG, DO_NOT_WAIT);
    
    while(!mfrc522.PICC_IsNewCardPresent() && !abort)
    {
        mp3_loop();
        button_readAll();
        if (button_pressedFor(BUTTON_PLAY, BUTTON_LONG_PRESS_TIME))
        {
            abort = true;
        }
        DEBUG_PRINT_LN(F("no card to reconfigure found"));
        delay(100);
    }
    
    if (!abort)
    {
        while(!mfrc522.PICC_ReadCardSerial())
        {
            mp3_loop();
            button_readAll();
            if (button_pressedFor(BUTTON_PLAY, BUTTON_LONG_PRESS_TIME))
            {
                abort = true;
            }
            DEBUG_PRINT_LN(F("cannot read card"));
            delay(100);
        }
        
        nfcTag = nfc_setupTag();
        if (IS_LISTENMODE(nfcTag.folderSettings.mode))
        {
            folder = nfcTag.folderSettings;
        }
    }
    else
    {
        mp3_playMp3FolderTrack(MP3_ACTION_ABORT_OK);
    }
    
    /* wait for all buttons are released before next action */
    button_readAll();
    while (!button_allReleased())
    {
        mp3_loop();
        button_readAll();
    }
    
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}

/**
 * @brief This method is used while entering the admin menu to check if the keycard was placed onto the nfc reader.
 */
static bool checkForKeyCard(void)
{
    bool retVal = false;
    NfcTagObject_t nfcTag;
    
    if (!mfrc522.PICC_IsNewCardPresent())
    {
        return retVal;
    }
    
    if (!mfrc522.PICC_ReadCardSerial())
    {
        /* do nothing */
    }
    else if (!nfc_readTag(&nfcTag))
    {
        /* do nothing */
    }
    else if ((nfcTag.cookie == GOLDEN_COOKIE) && (nfcTag.folderSettings.mode == MODE_KEYCARD))
    {
        retVal = true;
    }
    
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    
    return retVal;
}     

/**
 * @brief Helper method to pring array to terminal.
 */
void dump_byte_array(uint8_t* buffer, uint8_t bufferSize)
{
#ifdef DEBUG
    for (uint8_t i = 0; i < bufferSize; i++)
    {
        DEBUG_PRINT(buffer[i] < 0x10 ? " 0" : " ");
        DEBUG_PRINT_TWO_ARG(buffer[i], HEX);
    }
#else
    UNUSED(buffer);
    UNUSED(bufferSize);
#endif
}