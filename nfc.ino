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
            /* configure new card */  
            mp3PlayMp3FolderTrack(MP3_NEW_NFC_TAG);
            nfcTag = setupNfcTag();
            folder = nfcTag.folderSettings;
            
        }
        else
        {
            /* handle known card */
            switch (nfcTag.folderSettings.mode)
            {
            case MODE_UNSET:
                /* TBD setupCard(); */
                break;
                
            case MODE_ADMIN:
                mfrc522.PICC_HaltA();
                mfrc522.PCD_StopCrypto1();
                adminMenu();
                break;
                
            case MODE_SHUFFLE:
            case MODE_ALBUM:
            case MODE_AUDIO_BOOK:
                folder = nfcTag.folderSettings;
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
        mp3PlayMp3FolderTrack(MP3_OWEH_DAS_HAT_LEIDER);
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
        mp3PlayMp3FolderTrack(MP3_OWEH_DAS_HAT_LEIDER);
        return false;
    }
    else
    {
        mp3PlayMp3FolderTrack(MP3_OK_ICH_HABE_DIE_KAR);
        return true;
    }
}

static NfcTagObject_t setupNfcTag(void)
{
    DEBUG_TRACE;
    
    FolderMode_t modeSelector = MODE_ALBUM;
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
    
    /* setup folder number */
    mp3PlayMp3FolderTrack(MP3_SELECT_FOLDER);
    mp3PlayMp3FolderTrack(folderSelector);
    mp3PlayFolderTrack(folderSelector, 1);
    readButtons();
    while (!buttonPause.wasReleased())
    {
        readButtons();
        if (buttonUp.wasReleased() || buttonDown.wasReleased())
        {
            if (buttonUp.wasReleased() && (folderSelector < folderCount))
            {
                folderSelector++;
            }
            else if (buttonDown.wasReleased() && (folderSelector > 1))
            {
                folderSelector--;
            }
            mp3PlayMp3FolderTrack(folderSelector);
            mp3PlayFolderTrack(folderSelector, 1);
        }
    }

    newNfcTag.folderSettings.number = folderSelector;
    
    /* setup listen mode or admin nfc tag */
    mp3PlayMp3FolderTrack(MP3_SELECT_LISTEN_MODE);
    mp3PlayMp3FolderTrack(MP3_LISTEN_MODE_ALBUM);
    readButtons();
    while (!buttonPause.wasReleased())
    {
        readButtons();
        if (buttonUp.wasReleased() || buttonDown.wasReleased())
        {
            if (buttonUp.wasReleased() && (modeSelector < (MODE_NR_OF_MODES - 1)))
            {
                modeSelector = (FolderMode_t)((uint8_t)modeSelector + 1);
            }
            else if (buttonDown.wasReleased() && (modeSelector > 1))
            {
                modeSelector = (FolderMode_t)((uint8_t)modeSelector - 1);
            }
            
            switch(modeSelector)
            {                
            case MODE_SHUFFLE:
                mp3PlayMp3FolderTrack(MP3_LISTEN_MODE_SHUFFLE);
                break;
                
            case MODE_AUDIO_BOOK:
                mp3PlayMp3FolderTrack(MP3_LISTEN_MODE_AUDIO_BOOK);
                break;
                
            case MODE_ADMIN:
                mp3PlayMp3FolderTrack(MP3_LISTEN_MODE_ADMIN);
                break;
                
            case MODE_ALBUM:
                /* fall through */
            default:
                mp3PlayMp3FolderTrack(MP3_LISTEN_MODE_ALBUM);
                break;
            }
        }
    }
    newNfcTag.folderSettings.mode = modeSelector;

    writeNfcTag(newNfcTag);
    
    return newNfcTag;
}

static void resetNfcTag(void)
{    
    DEBUG_TRACE;
    
    while(!mfrc522.PICC_IsNewCardPresent())
    {
        DEBUG_PRINT_LN(F("no card to reconfigure found"));
        delay(100);
    }
    
    while(!mfrc522.PICC_ReadCardSerial())
    {
        DEBUG_PRINT_LN(F("cannot read card"));
        delay(100);
    }
    
    setupNfcTag();
}

void dump_byte_array(uint8_t* buffer, uint8_t bufferSize)
{
    for (uint8_t i = 0; i < bufferSize; i++)
    {
        DEBUG_PRINT(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}
