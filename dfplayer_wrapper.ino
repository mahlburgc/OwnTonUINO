static void mp3Start(void)
{
    if (!mp3IsPlaying())
    {
        mp3.start();
        delay(DF_PLAYER_COM_DELAY);
    }
}

static void mp3Pause(void)
{
    mp3.pause();
    delay(DF_PLAYER_COM_DELAY);
}

static void mp3SetVolume(uint8_t volume)
{
    mp3.setVolume(volume);
    delay(DF_PLAYER_COM_DELAY);
}

static void mp3IncreaseVolume()
{
    mp3.increaseVolume();
    delay(DF_PLAYER_COM_DELAY);
}

static void mp3DecreaseVolume()
{
    mp3.decreaseVolume();
    delay(DF_PLAYER_COM_DELAY);
}

static void mp3PlayFolderTrack(uint8_t folder, uint16_t track)
{
    mp3.playFolderTrack(folder, track);
    delay(DF_PLAYER_COM_DELAY);
}

static void mp3Begin(void)
{
    mp3.begin();
    delay(DF_PLAYER_COM_DELAY);   
}

static uint16_t mp3GetFolderTrackCount(uint8_t folderNr)
{
    return mp3.getFolderTrackCount(folderNr);
}

static uint16_t mp3GetTotalFolderCount(void)
{
    return (mp3.getTotalFolderCount() - 3); /* remove folder offset (advert, mp3 and ??) */
}

static void mp3Loop(void)
{
    mp3.loop();
    delay(1);
}
static bool mp3IsPlaying(void)
{
  return !digitalRead(DF_PLAYER_BUSY_PIN);
}

static void mp3PlayMp3FolderTrack(uint16_t track, bool waitTillFinish) /* default: waitTillFinish == true */
{
    mp3.playMp3FolderTrack(track);
    if (waitTillFinish)
    {
        waitForTrackFinish();
    }
    else
    {
        delay(DF_PLAYER_COM_DELAY);
    }
}

static uint8_t mp3GetVolume(void)
{
    return mp3.getVolume();
}

static void mp3PlayAdvertisement(uint16_t track)
{
    if (mp3IsPlaying())
    {
        mp3.playAdvertisement(track);
        waitForTrackFinish();
    }
    else
    {
        mp3Start();
        delay(100);
        mp3.playAdvertisement(track);
        waitForTrackFinish();
        mp3.pause();
    }
}
