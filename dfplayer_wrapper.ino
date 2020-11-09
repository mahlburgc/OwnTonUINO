static void mp3Start(void)
{
    mp3.start();
    delay(DF_PLAYER_COM_DELAY);
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

static void mp3Loop(void)
{
    mp3.loop();
}
static bool mp3IsPlaying(void)
{
  return !digitalRead(DF_PLAYER_BUSY_PIN);
}