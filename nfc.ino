static void nfcHandler(void)
{
    DEBUG_TRACE;
    
    NfcTagObject_t nfcTag; /* my nfc object, e.g. nfc card or sticker */
    
    if (!mfrc522.PICC_ReadCardSerial())
    {
        return;
    }
    
    nfcConfigInProgess = true;

    if (readNfcTag(&nfcTag) == true) 
    {
        if (nfcTag.cookie != GOLDEN_COOKIE)
        {
            sleepTimerDisable();
            
            /* configure new card */  
            mp3PlayMp3FolderTrack(MP3_NEW_TAG);
            nfcTag = setupNfcTag();
            if (IS_LISTENMODE(nfcTag.folderSettings.mode))
            {
                folder = nfcTag.folderSettings;
                playFolder();
            }
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
                folder = nfcTag.folderSettings;
                playFolder();
                break;
                
            case MODE_KEYCARD:
                buttonsLocked = !buttonsLocked;
                DEBUG_PRINT(F("Buttons locked (0->unlocked, 1-> locked): "));
                DEBUG_PRINT_LN(buttonsLocked);
                mp3PlayAdvertisement(ADV_BUTTONS_UNLOCKED + buttonsLocked); /* 300 for buttonsLocked = false, 301 for buttonsLocked = true */             
                break;
                
            default:
                /* do nothing */
                break;
            }
        }
    }
    
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    
    nfcConfigInProgess = false;
}
    
static bool readNfcTag(NfcTagObject_t* nfcTag)
{
    DEBUG_TRACE;
    
    MFRC522::PICC_Type mifareType;
    const uint8_t BUFFER_SIZE    = 18;
    uint8_t buffer[BUFFER_SIZE]  = { 0 };
    uint8_t bufferSizeRetVal     = BUFFER_SIZE;
    uint32_t tempCookie = 0;

    if (nfcTag == NULL)
    {
        return false;
    }

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

static bool writeNfcTag(NfcTagObject_t nfcTag)
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
        mp3PlayMp3FolderTrack(MP3_TAG_CONFIG_ERROR);
        return false;
    }

    // Write data to the block
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
        mp3PlayMp3FolderTrack(MP3_TAG_CONFIG_ERROR);
        return false;
    }
    else
    {
        mp3PlayMp3FolderTrack(MP3_TAG_CONFIG_OK);
        return true;
    }
}

static NfcTagObject_t setupNfcTag(void)
{
    DEBUG_TRACE;
    
    FolderMode_t modeSelector = FIRST_MODE;
    uint16_t folderCount = mp3GetTotalFolderCount();
    uint16_t folderSelector = 1; /* can something between 1 and "number of folders" */

    NfcTagObject_t newNfcTag = 
    {
        .cookie = GOLDEN_COOKIE,
        .version = VERSION,
        .folderSettings = 
        {
            .number = 0,
            .mode   = MODE_UNSET,
        }
    };
    
    /* setup listen mode or keycard nfc tag */
    mp3PlayMp3FolderTrack(MP3_SELECT_MODE);
    mp3PlayMp3FolderTrack(MP3_MODE_ARRAY[FIRST_MODE - 1], DO_NOT_WAIT);
    
    DEBUG_PRINT_LN(modeSelector);
    
    readButtons();
    while (!buttonWasReleased(BUTTON_PLAY))
    {
        readButtons();
        mp3Loop();

        if (buttonWasReleased(BUTTON_UP) && (modeSelector < (LAST_MODE)))
        {
            modeSelector = modeSelector + 1;
            DEBUG_PRINT_LN(modeSelector);
        }
        else if (buttonWasReleased(BUTTON_DOWN) && (modeSelector > FIRST_MODE))
        {
            modeSelector = modeSelector - 1;
            DEBUG_PRINT_LN(modeSelector);
        }
            
        if (buttonWasReleased(BUTTON_UP) || buttonWasReleased(BUTTON_DOWN))
        {
            mp3PlayMp3FolderTrack(MP3_MODE_ARRAY[modeSelector - 1], DO_NOT_WAIT);
        }   
    }
    
    /* if selected mode is not a listen mode, the folder number is not needed */
    if (IS_LISTENMODE(modeSelector))
    {
        /* setup folder number */
        mp3PlayMp3FolderTrack(MP3_SELECT_FOLDER);
        mp3PlayMp3FolderTrack(folderSelector);
        mp3PlayFolderTrack(folderSelector, 1);

        readButtons();

        while (!buttonWasReleased(BUTTON_PLAY))
        {
            readButtons();
            mp3Loop();
            
            if (buttonWasReleased(BUTTON_UP) && (folderSelector < folderCount))
            {
                folderSelector++;
            }
            else if (buttonWasReleased(BUTTON_DOWN) && (folderSelector > 1))
            {
                folderSelector--;
            }
            
            if (buttonWasReleased(BUTTON_UP) || buttonWasReleased(BUTTON_DOWN))
            {
                mp3PlayMp3FolderTrack(folderSelector);
                mp3PlayFolderTrack(folderSelector, 1); 
            }
        }
    }

    newNfcTag.folderSettings.number = folderSelector;
    newNfcTag.folderSettings.mode = modeSelector;

    if (!writeNfcTag(newNfcTag))
    {
        newNfcTag.folderSettings.number = 0;
        newNfcTag.folderSettings.mode = MODE_UNSET;
    }
    
    return newNfcTag;
}

static void resetNfcTag(void)
{    
    DEBUG_TRACE;
    
    NfcTagObject_t nfcTag;
    bool abort = false;
    
    mp3PlayMp3FolderTrack(MP3_INSERT_TAG);
    
    while(!mfrc522.PICC_IsNewCardPresent() && !abort)
    {
        mp3Loop();
        readButtons();
        if (buttonPressedFor(BUTTON_PLAY, BUTTON_LONG_PRESS_TIME))
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
            readButtons();
            mp3Loop();
            DEBUG_PRINT_LN(F("cannot read card"));
            delay(100);
        }
        
        nfcTag = setupNfcTag();
        if (IS_LISTENMODE(nfcTag.folderSettings.mode))
        {
            folder = nfcTag.folderSettings;
        }
    }
    else
    {
        mp3PlayMp3FolderTrack(MP3_ACTION_ABORT_OK);
    }
    
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}

void dump_byte_array(uint8_t* buffer, uint8_t bufferSize)
{
    for (uint8_t i = 0; i < bufferSize; i++)
    {
        DEBUG_PRINT(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}