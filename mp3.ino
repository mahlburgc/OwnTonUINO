/********************************************************************************
 * @file    : mp3.ino
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
 * @brief The method should be called cyclical to ensure that incomming communication messages from DFPlayer Mini are processed.
 */
static void mp3_loop(void)
{
    mp3.loop();
}

/**
 * @brief The method should be called on startup to stat the player.
 */
static void mp3_begin(void)
{
    mp3.begin();
    delay(DF_PLAYER_COM_DELAY);   
}

/**
 * @brief Start playing music.
 */
static void mp3_start(void)
{
    if (!mp3_isPlaying())
    {
        mp3.start();
        delay(DF_PLAYER_COM_DELAY);
    }
}

/**
 * @brief Pause the music. Using mp3_start() after calling this method, the music continues within the track.
 */
static void mp3_pause(void)
{
    mp3.pause();
    delay(DF_PLAYER_COM_DELAY);
}

/**
 * @brief Set the music volume.
 */
static void mp3_setVolume(uint8_t volume)
{
    mp3.setVolume(volume);
    delay(DF_PLAYER_COM_DELAY);
}

/**
 * @brief Decrease the music volume.
 */
static void mp3_decreaseVolume(void)
{
    mp3.decreaseVolume();
    delay(DF_PLAYER_COM_DELAY);
}

/**
 * @brief Play a specific track from a specific folder.
 */
static void mp3_playFolderTrack(uint8_t folder, uint16_t track)
{
    mp3.playFolderTrack(folder, track);
    delay(DF_PLAYER_COM_DELAY);
}

/**
 * @brief Play a specific track from the mp3 folder (located on the sd-card).
 *        Tracks from the mp3 folder are handled like normal music tracks and stop the actual track.
 */
static void mp3_playMp3FolderTrack(uint16_t track, bool waitTillFinish) /* default: waitTillFinish == true */
{
    mp3.playMp3FolderTrack(track);
    if (waitTillFinish)
    {
        mp3_waitForTrackFinish();
    }
    else
    {
        delay(DF_PLAYER_COM_DELAY);
    }
}

/**
 * @brief Play a specific track from the advert folder (located on the sd-card).
 *        Advert tracks interrupting the actual track and resume at the interrupted point after the advert track finishes.
 *        To play an adver track, DFPlayer must be in playing mode (mp3_start()).
 *        IMPORTANT: If no music is playing and the player is in pause mode, calling this method results in playing a very short snipped
 *        of the paused music track before and after the advert track. Actually it seems like that cannot be prevented. So if possible, 
 *        avoid to use advertisement tracks.
 */
static void mp3_playAdvertisement(uint16_t track)
{
    if (mp3_isPlaying())
    {
        mp3.playAdvertisement(track);
        mp3_waitForTrackFinish();
    }
    else
    {
        mp3_start();
        delay(100); /* necessary to ensure that advertisement track can start */
        mp3.playAdvertisement(track);
        mp3_waitForTrackFinish();
        mp3.pause();
    }
}

/**
 * @brief Wait for a track to finish.
 */
static void mp3_waitForTrackFinish(void)
{
    DEBUG_TRACE;
    
    unsigned long timeStart = 0;
    unsigned long TRACK_FINISHED_TIMEOUT = 500; /* ms, timeout to start the track by DFPlayer and raise busy pin */
    
    /* If starting a new track while another track is playing, directly calling this function
     * may results in detecting the end of the old song. To avoid this, an initial delay is added.
     */
    delay(100); 
    timeStart = millis();
    
    /* Wait for track to finish. Therefore it must be checked that new track already started.
     * Otherwise it's possible that "track finished" is detected in the timeslice between the old and the new track.
     */
    while ((!mp3_isPlaying()) && (millis() < (timeStart + TRACK_FINISHED_TIMEOUT)))
    {
        /* wait for track start */
        mp3_loop();
    }
    DEBUG_PRINT_LN(F("track started"));
    
    while (mp3_isPlaying())
    {
        /* wait for track to finish */
        mp3_loop();
    }
    DEBUG_PRINT_LN(F("track finished"));
}

/**
 * @brief Wait for a track to finish.
 */
static bool mp3_isPlaying(void)
{
  return !digitalRead(DF_PLAYER_BUSY_PIN);
}

/**
 * @brief Get the actual volume from DFPlayer.
 */
static uint8_t mp3_getVolume(void)
{
    return mp3.getVolume();
}

/**
 * @brief Get the number of tracks contained in the given folder.
 */
static uint16_t mp3_getFolderTrackCount(uint8_t folderNr)
{
    return mp3.getFolderTrackCount(folderNr);
}

/**
 * @brief Get the total number of folders on the sc-card.
 *        TBD: What is the third offset folder?
 */
static uint16_t mp3_getTotalFolderCount(void)
{
    return (mp3.getTotalFolderCount() - 3); /* remove folder offset (advert, mp3 and ??) */
}