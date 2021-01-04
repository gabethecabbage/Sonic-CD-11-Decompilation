#include "RetroEngine.hpp"
#include <cmath>

int globalSFXCount = 0;
int stageSFXCount  = 0;

int masterVolume  = MAX_VOLUME;
int trackID     = -1;
int sfxVolume   = MAX_VOLUME;
int bgmVolume     = MAX_VOLUME;
bool audioEnabled = false;

int nextChannelPos;
bool musicEnabled;
int musicStatus;
TrackInfo musicTracks[TRACK_COUNT];
SFXInfo sfxList[SFX_COUNT];

ChannelInfo sfxChannels[CHANNEL_COUNT];

MusicPlaybackInfo musInfo;

#if RETRO_USING_SDL
SDL_AudioSpec audioDeviceFormat;

#define AUDIO_FREQUENCY (44100)
#define AUDIO_FORMAT    (0x8010) /**< Signed 16-bit samples */
#define AUDIO_SAMPLES   (0x800)
#define AUDIO_CHANNELS  (2)

#define ADJUST_VOLUME(s, v) (s = (s * v) / MAX_VOLUME)

#endif

#define AUDIO_BUFFERSIZE (0x4000)

int InitAudioPlayback()
{
    StopAllSfx(); //"init"
#if RETRO_USING_SDL
    SDL_AudioSpec want;
    want.freq     = AUDIO_FREQUENCY;
    want.format   = AUDIO_FORMAT;
    want.samples  = AUDIO_SAMPLES;
    want.channels = AUDIO_CHANNELS;
    want.callback = ProcessAudioPlayback;

    if (SDL_OpenAudio(&want, &audioDeviceFormat) >= 0) {
        audioEnabled = true;
        SDL_PauseAudio(0);
    }
    else {
        printLog("Unable to open audio device: %s", SDL_GetError());
        return false;
    }
    #endif

    FileInfo info;
    FileInfo infoStore;
    char strBuffer[0x100];
    int fileBuffer  = 0;
    int fileBuffer2 = 0;

    if (LoadFile("Data/Game/Gameconfig.bin", &info)) {
        infoStore = info;

        FileRead(&fileBuffer, 1);
        FileRead(strBuffer, fileBuffer);
        strBuffer[fileBuffer] = 0;

        FileRead(&fileBuffer, 1);
        FileRead(&strBuffer, fileBuffer); // Load 'Data'
        strBuffer[fileBuffer] = 0;

        FileRead(&fileBuffer, 1);
        FileRead(strBuffer, fileBuffer);
        strBuffer[fileBuffer] = 0;

        // Read Obect Names
        int objectCount = 0;
        FileRead(&objectCount, 1);
        for (int o = 0; o < objectCount; ++o) {
            FileRead(&fileBuffer, 1);
            FileRead(strBuffer, fileBuffer);
            strBuffer[fileBuffer] = 0;
        }

        // Read Script Paths
        for (int s = 0; s < objectCount; ++s) {
            FileRead(&fileBuffer, 1);
            FileRead(strBuffer, fileBuffer);
            strBuffer[fileBuffer] = 0;
        }

        int varCnt = 0;
        FileRead(&varCnt, 1);
        for (int v = 0; v < varCnt; ++v) {
            FileRead(&fileBuffer, 1);
            FileRead(strBuffer, fileBuffer);
            strBuffer[fileBuffer] = 0;

            // Read Variable Value
            FileRead(&fileBuffer2, 4);
        }

        // Read SFX
        globalSFXCount = 0;
        FileRead(&globalSFXCount, 1);
        for (int s = 0; s < globalSFXCount; ++s) {
            FileRead(&fileBuffer, 1);
            FileRead(strBuffer, fileBuffer);
            strBuffer[fileBuffer] = 0;

            GetFileInfo(&infoStore);
            LoadSfx(strBuffer, s);
            SetFileInfo(&infoStore);
        }

        CloseFile();
    }

    // sfxDataPosStage = sfxDataPos;
    nextChannelPos = 0;
    for (int i = 0; i < CHANNEL_COUNT; ++i) sfxChannels[i].sfxID = -1;

    return true;
}

#if RETRO_USING_SDL
int readVorbisStream(void *dst, uint size)
{
    int tot = 0;
    int read;
    int to_read = size;
    char *buf   = (char *)dst;
    uint left   = musInfo.audioLen;
    while (to_read && (read = (int)ov_read(&musInfo.vorbisFile, buf, to_read, 0, 2, 1, &musInfo.vorbBitstream))) {
        if (read < 0) {
            return 0;
        }
        to_read -= read;
        buf += read;
        tot += read;
        left -= read;
        if (left <= 0)
            break;
    }
    return tot;
}


int trackRequestMoreData(uint samples, uint amount)
{
    int out   = amount / 2;
    int avail = SDL_AudioStreamAvailable(musInfo.stream);

    if (avail < out * 2) {

        int numSamples = 0;
        numSamples = readVorbisStream(musInfo.extraBuffer, (musInfo.spec.format & 0xFF) / 8 * samples * musInfo.spec.channels);

        if (numSamples == 0)
            return 0;

        int rc = SDL_AudioStreamPut(musInfo.stream, musInfo.extraBuffer, numSamples);
        if (rc == -1)
            return -1;
    }

    int get = SDL_AudioStreamGet(musInfo.stream, musInfo.buffer, out);
    if (get == -1) {
        return -1;
    }
    if (get == 0)
        get = -2;

    return get;
}
#endif


void ProcessMusicStream(void *data, Uint8 *stream, int len)
{
    if (!musInfo.loaded)
        return;
    switch (musicStatus) {
        case MUSIC_READY:
        case MUSIC_PLAYING: {
#if RETRO_USING_SDL
            int bytes        = trackRequestMoreData(AUDIO_SAMPLES, len * 2);
            if (bytes > 0) {
                int vol = (bgmVolume * masterVolume) / MAX_VOLUME;
                ProcessAudioMixing(NULL, stream, musInfo.buffer, audioDeviceFormat.format, len, vol, true);
            }

            switch (bytes) {
                case -2:
                case -1: break;
                case 0:
                    if (musInfo.currentTrack->trackLoop)
                        ov_pcm_seek(&musInfo.vorbisFile, musInfo.currentTrack->loopPoint);
                    else
                        musicStatus = MUSIC_STOPPED;
                    break;
            }
#endif
        } break;
        case MUSIC_STOPPED:
        case MUSIC_PAUSED:
        case MUSIC_LOADING:
            // dont play
            break;
    }
}

void ProcessAudioPlayback(void *data, Uint8 *stream, int len)
{
    memset(stream, 0, len);

    if (!audioEnabled)
        return;

    ProcessMusicStream(data, stream, len);

    for (byte i = 0; i < CHANNEL_COUNT; ++i) {
        ChannelInfo *sfx = &sfxChannels[i];
        if (sfx == NULL)
            continue;

        if (sfx->sfxID < 0)
            continue;

        if (sfx->samplePtr) {
            if (sfx->sampleLength > 0) {
                int sampleLen = (len > sfx->sampleLength) ? sfx->sampleLength : len;
#if RETRO_USING_SDL
                ProcessAudioMixing(sfx, stream, sfx->samplePtr, audioDeviceFormat.format, sampleLen, sfxVolume, false);
#endif

                sfx->samplePtr += sampleLen;
                sfx->sampleLength -= sampleLen;
            }

            if (sfx->sampleLength <= 0) {
                if (sfx->loopSFX) {
                    sfx->samplePtr    = sfxList[sfx->sfxID].buffer;
                    sfx->sampleLength = sfxList[sfx->sfxID].length;
                }
                else {
                    StopSfx(sfx->sfxID);
                }
            }
        }
    }
}

#if RETRO_USING_SDL
void ProcessAudioMixing(void *sfx, Uint8 *dst, const byte *src, SDL_AudioFormat format, Uint32 len, int volume, bool music)
{
    if (volume == 0)
        return;

    if (volume > MAX_VOLUME)
        volume = MAX_VOLUME;

    ChannelInfo *snd = (ChannelInfo *)sfx;

    float panL     = 0;
    float panR     = 0;
    int i          = 0;
    if (!music)
        panL = snd->pan;
    else
        panL = 0;

    if (panL < 0) {
        panL = abs(panL / 100.0f);
        panR = 0;
    }
    else {
        panR = abs(panL / 100.0f);
        panL = 0;
    }

    switch (format) {
        case AUDIO_S16LSB: {
            Sint16 src1, src2;
            int dst_sample;
            const int max_audioval = ((1 << (16 - 1)) - 1);
            const int min_audioval = -(1 << (16 - 1));

            len /= 2;
            while (len--) {
                src1 = ((src[1]) << 8 | src[0]);
                ADJUST_VOLUME(src1, volume);

                if (panL != 0 || panR != 0) {
                    if ((i % 2) != 0) {
                        Sint16 swapr = (Sint16)(((float)(Sint16)SDL_SwapLE16(src1)) * panR);
                        src1         = (Sint16)SDL_SwapLE16(swapr);
                    }
                    else {
                        Sint16 swapl = (Sint16)(((float)(Sint16)SDL_SwapLE16(src1)) * panL);
                        src1         = (Sint16)SDL_SwapLE16(swapl);
                    }
                }

                src2 = ((dst[1]) << 8 | dst[0]);
                src += 2;
                dst_sample = src1 + src2;

                if (dst_sample > max_audioval) {
                    dst_sample = max_audioval;
                }
                else if (dst_sample < min_audioval) {
                    dst_sample = min_audioval;
                }
                dst[0] = dst_sample & 0xFF;
                dst_sample >>= 8;
                dst[1] = dst_sample & 0xFF;
                dst += 2;
                i++;
            }
        } break;
        case AUDIO_S16MSB: {
#if defined(__GNUC__) && defined(__M68000__) && !defined(__mcoldfire__) && defined(SDL_ASSEMBLY_ROUTINES)
            SDL_MixAudio_m68k_S16MSB((short *)dst, (short *)src, (unsigned long)len, (long)volume);
#else
            Sint16 src1, src2;
            int dst_sample;
            const int max_audioval = ((1 << (16 - 1)) - 1);
            const int min_audioval = -(1 << (16 - 1));

            len /= 2;
            while (len--) {
                src1 = ((src[0]) << 8 | src[1]);
                ADJUST_VOLUME(src1, volume);

                if (panL != 0 || panR != 0) {
                    if ((i % 2) != 0) {
                        Sint16 swapr = (Sint16)(((float)(Sint16)SDL_SwapBE16(src1)) * panR);
                        src1         = (Sint16)SDL_SwapBE16(swapr);
                    }
                    else {
                        Sint16 swapl = (Sint16)(((float)(Sint16)SDL_SwapBE16(src1)) * panL);
                        src1         = (Sint16)SDL_SwapBE16(swapl);
                    }
                }

                src2 = ((dst[0]) << 8 | dst[1]);
                src += 2;
                dst_sample = src1 + src2;

                if (dst_sample > max_audioval) {
                    dst_sample = max_audioval;
                }
                else if (dst_sample < min_audioval) {
                    dst_sample = min_audioval;
                }
                dst[1] = dst_sample & 0xFF;
                dst_sample >>= 8;
                dst[0] = dst_sample & 0xFF;
                dst += 2;
                i++;
            }
#endif
        } break;
        case AUDIO_U16LSB: {
            Uint16 src1, src2;
            int dst_sample;
            const int max_audioval = 0xFFFF;

            len /= 2;
            while (len--) {
                src1 = ((src[1]) << 8 | src[0]);
                ADJUST_VOLUME(src1, volume);

                if (panL != 0 || panR != 0) {
                    Sint16 samp = (Sint16)(SDL_SwapLE16(src1) - 32768);
                    if ((i % 2) != 0) {
                        Uint16 swapr = (Uint16)(Sint16)(((float)samp * panR) + 32768);
                        src1         = (Uint16)SDL_SwapLE16(swapr);
                    }
                    else {
                        Uint16 swapl = (Uint16)(Sint16)(((float)samp * panL) + 32768);
                        src1         = (Uint16)SDL_SwapLE16(swapl);
                    }
                }

                src2 = ((dst[1]) << 8 | dst[0]);
                src += 2;
                dst_sample = src1 + src2;
                if (dst_sample > max_audioval) {
                    dst_sample = max_audioval;
                }
                dst[0] = dst_sample & 0xFF;
                dst_sample >>= 8;
                dst[1] = dst_sample & 0xFF;
                dst += 2;
                i++;
            }
        } break;
        case AUDIO_U16MSB: {
            Uint16 src1, src2;
            int dst_sample;
            const int max_audioval = 0xFFFF;

            len /= 2;
            while (len--) {
                src1 = ((src[0]) << 8 | src[1]);
                ADJUST_VOLUME(src1, volume);

                if (panL != 0 || panR != 0) {
                    Sint16 samp = (Sint16)(SDL_SwapBE16(src1) - 32768);
                    if ((i % 2) != 0) {
                        Uint16 swapr = (Uint16)((Sint16)((float)samp * panR) + 32768);
                        src1         = (Uint16)SDL_SwapBE16(swapr);
                    }
                    else {
                        Uint16 swapl = (Uint16)((Sint16)((float)samp * panL) + 32768);
                        src1         = (Uint16)SDL_SwapBE16(swapl);
                    }
                }

                src2 = ((dst[0]) << 8 | dst[1]);
                src += 2;
                dst_sample = src1 + src2;
                if (dst_sample > max_audioval) {
                    dst_sample = max_audioval;
                }
                dst[1] = dst_sample & 0xFF;
                dst_sample >>= 8;
                dst[0] = dst_sample & 0xFF;
                dst += 2;
                i++;
            }
        } break;
        case AUDIO_S32LSB: {
            const Uint32 *src32 = (Uint32 *)src;
            Uint32 *dst32       = (Uint32 *)dst;
            Sint64 src1, src2;
            Sint64 dst_sample;
            const Sint64 max_audioval = ((((Sint64)1) << (32 - 1)) - 1);
            const Sint64 min_audioval = -(((Sint64)1) << (32 - 1));

            len /= 4;
            while (len--) {
                src1 = (Sint64)((Sint32)SDL_SwapLE32(*src32));
                src32++;
                ADJUST_VOLUME(src1, volume);

                if (panL != 0 || panR != 0) {
                    if ((i % 2) != 0) {
                        Sint32 swapr = (Sint32)(((float)(Sint32)SDL_SwapLE32(src1)) * panR);
                        src1         = (Sint32)SDL_SwapLE16(swapr);
                    }
                    else {
                        Sint32 swapl = (Sint32)(((float)(Sint32)SDL_SwapLE32(src1)) * panL);
                        src1         = (Sint32)SDL_SwapLE32(swapl);
                    }
                }

                src2       = (Sint64)((Sint32)SDL_SwapLE32(*dst32));
                dst_sample = src1 + src2;
                if (dst_sample > max_audioval) {
                    dst_sample = max_audioval;
                }
                else if (dst_sample < min_audioval) {
                    dst_sample = min_audioval;
                }
                *(dst32++) = SDL_SwapLE32((Uint32)((Sint32)dst_sample));
                i++;
            }
        } break;
        case AUDIO_S32MSB: {
            const Uint32 *src32 = (Uint32 *)src;
            Uint32 *dst32       = (Uint32 *)dst;
            Sint64 src1, src2;
            Sint64 dst_sample;
            const Sint64 max_audioval = ((((Sint64)1) << (32 - 1)) - 1);
            const Sint64 min_audioval = -(((Sint64)1) << (32 - 1));

            len /= 4;
            while (len--) {
                src1 = (Sint64)((Sint32)SDL_SwapBE32(*src32));
                src32++;
                ADJUST_VOLUME(src1, volume);

                if (panL != 0 || panR != 0) {
                    if ((i % 2) != 0) {
                        Sint32 swapr = (Sint32)(((float)(Sint32)SDL_SwapBE32((uint)src1)) * panR);
                        src1         = (Sint32)SDL_SwapBE16(swapr);
                    }
                    else {
                        Sint32 swapl = (Sint32)(((float)(Sint32)SDL_SwapBE32((uint)src1)) * panL);
                        src1         = (Sint32)SDL_SwapBE16(swapl);
                    }
                }

                src2       = (Sint64)((Sint32)SDL_SwapBE32(*dst32));
                dst_sample = src1 + src2;
                if (dst_sample > max_audioval) {
                    dst_sample = max_audioval;
                }
                else if (dst_sample < min_audioval) {
                    dst_sample = min_audioval;
                }
                *(dst32++) = SDL_SwapBE32((Uint32)((Sint32)dst_sample));
                i++;
            }
        } break;
        case AUDIO_F32LSB: {
            const float fmaxvolume = 1.0f / ((float)MAX_VOLUME);
            const float fvolume    = (float)volume;
            const float *src32     = (float *)src;
            float *dst32           = (float *)dst;
            float src1 /*, src2*/;
            float dst1;
            double dst_sample;

            /* !!! FIXME: are these right? */
            const double max_audioval = 3.402823466e+38F;
            const double min_audioval = -3.402823466e+38F;

            len /= 4;
            int i = 0;
            while (len--) {
                src1 = ((SDL_SwapFloatLE(*src32) * fvolume) * fmaxvolume);
                if (panL != 0 || panR != 0) {
                    if ((i % 2) == 0) {
                        src1 = (src1 * panR);
                    }
                    else {
                        src1 = (src1 * panL);
                    }
                }

                src32++;
                dst1       = SDL_SwapFloatLE(*dst32);
                dst_sample = ((double)src1) + ((double)dst1);

                if (dst_sample > max_audioval) {
                    dst_sample = max_audioval;
                }
                else if (dst_sample < min_audioval) {
                    dst_sample = min_audioval;
                }
                *(dst32++) = SDL_SwapFloatLE((float)dst_sample);
                i++;
            }
        } break;
        case AUDIO_F32MSB: {
            const float fmaxvolume = 1.0f / ((float)MAX_VOLUME);
            const float fvolume    = (float)volume;
            const float *src32     = (float *)src;
            float *dst32           = (float *)dst;
            float src1, src2;
            double dst_sample;
            const double max_audioval = 3.402823466e+38F;
            const double min_audioval = -3.402823466e+38F;

            len /= 4;
            while (len--) {
                src1 = ((SDL_SwapFloatBE(*src32) * fvolume) * fmaxvolume);

                if (panL != 0 || panR != 0) {
                    if ((i % 2) == 0) {
                        src1 = (src1 * panR);
                    }
                    else {
                        src1 = (src1 * panL);
                    }
                }

                src2 = SDL_SwapFloatBE(*dst32);
                src32++;

                dst_sample = ((double)src1) + ((double)src2);
                if (dst_sample > max_audioval) {
                    dst_sample = max_audioval;
                }
                else if (dst_sample < min_audioval) {
                    dst_sample = min_audioval;
                }
                *(dst32++) = SDL_SwapFloatBE((float)dst_sample);
                i++;
            }
        } break;
        default:
            printLog("Unknown audio format: %d", format);
            return;
    }
}
#endif

#if RETRO_USING_SDL
size_t readVorbis(void *mem, size_t size, size_t nmemb, void *ptr)
{
    MusicPlaybackInfo *info = (MusicPlaybackInfo *)ptr;
    return FileRead2(&info->fileInfo, mem, (int)(size * nmemb));
}
int seekVorbis(void *ptr, ogg_int64_t offset, int whence)
{
    MusicPlaybackInfo *info = (MusicPlaybackInfo *)ptr;
    switch (whence) {
        case SEEK_SET: whence = 0; break;
        case SEEK_CUR: whence = (int)GetFilePosition2(&info->fileInfo); break;
        case SEEK_END: whence = info->fileInfo.fileSize; break;
        default: break;
    }
    SetFilePosition2(&info->fileInfo, (int)(whence + offset));
    return GetFilePosition2(&info->fileInfo) <= info->fileInfo.fileSize;
}
long tellVorbis(void *ptr)
{
    MusicPlaybackInfo *info = (MusicPlaybackInfo *)ptr;
    return GetFilePosition2(&info->fileInfo);
}
int closeVorbis(void *ptr)
{
    return CloseFile2();
}
#endif

void SetMusicTrack(char *filePath, byte trackID, bool loop, uint loopPoint)
{
    TrackInfo *track = &musicTracks[trackID];
    StrCopy(track->fileName, "Data/Music/");
    StrAdd(track->fileName, filePath);
    track->trackLoop = loop;
    track->loopPoint = loopPoint;
}
bool PlayMusic(int track)
{
    if (track < 0 || track >= TRACK_COUNT) {
        StopMusic();
        return false;
    }

    TrackInfo *trackPtr = &musicTracks[track];

    if (!trackPtr->fileName[0]) {
        StopMusic();
        return false;
    }

    if (LoadFile(trackPtr->fileName, &musInfo.fileInfo)) {
        if (musInfo.loaded)
            StopMusic();

        cFileHandleStream = cFileHandle;
        cFileHandle       = nullptr;

        musInfo.currentTrack = trackPtr;
        musInfo.loaded       = true;

#if RETRO_USING_SDL
        ov_callbacks callbacks;
        uint samples;

        callbacks.read_func  = readVorbis;
        callbacks.seek_func  = seekVorbis;
        callbacks.tell_func  = tellVorbis;
        callbacks.close_func = closeVorbis;

        int error = ov_open_callbacks(&musInfo, &musInfo.vorbisFile, NULL, 0, callbacks);
        if (error != 0) {
            return false;
        }

        musInfo.vorbBitstream = -1;
        musInfo.vorbisFile.vi   = ov_info(&musInfo.vorbisFile, -1);

        memset(&musInfo.spec, 0, sizeof(SDL_AudioSpec));

        musInfo.spec.format   = AUDIO_S16;
        musInfo.spec.channels = musInfo.vorbisFile.vi->channels;
        musInfo.spec.freq     = (int)musInfo.vorbisFile.vi->rate;
        musInfo.spec.samples  = 4096;

        samples = (uint)ov_pcm_total(&musInfo.vorbisFile, -1);

        musInfo.audioLen = musInfo.spec.size = (int)(samples * musInfo.spec.channels * 2);

        musInfo.stream = SDL_NewAudioStream(musInfo.spec.format, musInfo.spec.channels, musInfo.spec.freq, audioDeviceFormat.format,
                                      audioDeviceFormat.channels, audioDeviceFormat.freq);
        if (!musInfo.stream) {
            printLog("Failed to create stream: %s", SDL_GetError());
        }

        musInfo.buffer      = new byte[AUDIO_BUFFERSIZE];
        musInfo.extraBuffer = new byte[AUDIO_BUFFERSIZE];
#endif

        musicStatus = MUSIC_PLAYING;
        masterVolume = MAX_VOLUME;
        trackID      = track;
        return true;
    }
    return false;
}

void LoadSfx(char *filePath, byte sfxID) {
    FileInfo info;
    char fullPath[0x80];

    StrCopy(fullPath, "Data/SoundFX/");
    StrAdd(fullPath, filePath);

    if (LoadFile(fullPath, &info)) {
        byte* sfx = new byte[info.fileSize];
        FileRead(sfx, info.fileSize);
        CloseFile();

        SDL_RWops *src = SDL_RWFromMem(sfx, info.fileSize);
        if (src == NULL) {
            printLog("Unable to open sfx: %s", info.fileName);
        }
        else {
            SDL_AudioSpec wav_spec;
            uint wav_length;
            byte *wav_buffer;
            SDL_AudioSpec *wav = SDL_LoadWAV_RW(src, 0, &wav_spec, &wav_buffer, &wav_length);

            SDL_RWclose(src);
            delete[] sfx;
            if (wav == NULL) {
                printLog("Unable to read sfx: %s", info.fileName);
            }
            else {
                SDL_AudioCVT convert;
                if (SDL_BuildAudioCVT(&convert, wav->format, wav->channels, wav->freq, audioDeviceFormat.format, audioDeviceFormat.channels,
                                      audioDeviceFormat.freq)
                    > 0) {
                    convert.buf = (byte *)malloc(wav_length * convert.len_mult);
                    convert.len = wav_length;
                    memcpy(convert.buf, wav_buffer, wav_length);
                    SDL_ConvertAudio(&convert);

                    StrCopy(sfxList[sfxID].name, filePath);
                    sfxList[sfxID].buffer = convert.buf;
                    sfxList[sfxID].length = convert.len_cvt;
                    sfxList[sfxID].loaded = true;
                    SDL_FreeWAV(wav_buffer);
                }
                else {
                    StrCopy(sfxList[sfxID].name, filePath);
                    sfxList[sfxID].buffer = wav_buffer;
                    sfxList[sfxID].length = wav_length;
                    sfxList[sfxID].loaded = true;
                }
            }

        
        }
    }
}
void PlaySfx(int sfx, bool loop)
{
    int sfxChannelID = nextChannelPos++;
    for (int c = 0; c < CHANNEL_COUNT; ++c) {
        if (sfxChannels[c].sfxID == sfx) {
            sfxChannelID = c;
            break;
        }
    }

    ChannelInfo *sfxInfo      = &sfxChannels[sfxChannelID];
    sfxInfo->sfxID        = sfx;
    sfxInfo->samplePtr        = sfxList[sfx].buffer;
    sfxInfo->sampleLength     = sfxList[sfx].length;
    sfxInfo->loopSFX      = loop;
    sfxInfo->pan          = 0;
    if (nextChannelPos == CHANNEL_COUNT)
        nextChannelPos = 0;
}
void SetSfxAttributes(int sfx, int loopCount, char pan)
{
    int sfxChannel = -1;
    for (int i = 0; i < CHANNEL_COUNT; ++i) {
        if (sfxChannels[i].sfxID == sfx || sfxChannels[i].sfxID == -1) {
            nextChannelPos = i;
            sfxChannel     = i;
            break;
        }
    }
    if (sfxChannel == -1)
        return; // wasn't found

    ChannelInfo *sfxInfo  = &sfxChannels[nextChannelPos++];
    if (sfxInfo->sfxID != -1)
        StopSfx(sfxInfo->sfxID);
    sfxInfo->samplePtr  = sfxList[sfx].buffer;
    sfxInfo->sampleLength = sfxList[sfx].length;
    sfxInfo->loopSFX      = loopCount == -1 ? sfxInfo->loopSFX : loopCount;
    sfxInfo->pan  = pan;
    sfxInfo->sfxID   = sfx;

    if (nextChannelPos == CHANNEL_COUNT)
        nextChannelPos = 0;
}
