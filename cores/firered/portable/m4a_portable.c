/* Host-side wave load may run before game heap init; keep libc malloc. */
#define FIRERED_HOST_LIBC_MALLOC 1

#include "../../../include/global.h"
#include "../../../include/constants/songs.h"
#include "../../../include/m4a.h"
#include "../../../engine/core/engine_internal.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

#ifndef FIRERED_EXPERIMENTAL_AUDIO

struct MusicPlayerInfo gMPlayInfo_BGM = {0};
struct MusicPlayerInfo gMPlayInfo_SE1 = {0};
struct MusicPlayerInfo gMPlayInfo_SE2 = {0};
struct MusicPlayerInfo gMPlayInfo_SE3 = {0};
struct SoundInfo gSoundInfo = {0};
struct PokemonCrySong gPokemonCrySong = {0};
struct PokemonCrySong gPokemonCrySongs[MAX_POKEMON_CRIES] = {0};
struct MusicPlayerInfo gPokemonCryMusicPlayers[MAX_POKEMON_CRIES] = {0};
struct MusicPlayerTrack gPokemonCryTracks[MAX_POKEMON_CRIES] = {0};
u8 gMPlayMemAccArea[1] = {0};
struct CgbChannel gCgbChans[4] = {0};
MPlayFunc gMPlayJumpTable[64] = {0};
const struct MusicPlayer gMPlayTable[] = {
    { &gMPlayInfo_BGM, NULL, 0, 0 },
    { &gMPlayInfo_SE1, NULL, 0, 0 },
    { &gMPlayInfo_SE2, NULL, 0, 0 },
    { &gMPlayInfo_SE3, NULL, 0, 0 },
};
const struct Song gSongTable[] = { { NULL, 0, 0 } };
char SoundMainRAM[0x800] = {0};
char gNumMusicPlayers[] = {4, 0};
char gMaxLines[] = {12, 0, 0, 0};
const struct SongHeader mus_victory_gym_leader = {0};

void m4aSoundVSyncOn(void) {}
void m4aSoundVSyncOff(void) {}
void m4aSoundInit(void) {}
void m4aSoundMain(void) {}
void m4aSongNumStart(u16 n) { (void)n; }
void m4aSongNumStartOrChange(u16 n) { (void)n; }
void m4aSongNumStartOrContinue(u16 n) { (void)n; }
void m4aSongNumStop(u16 n) { (void)n; }
void m4aSongNumContinue(u16 n) { (void)n; }
void m4aMPlayAllStop(void) {}
void m4aMPlayContinue(struct MusicPlayerInfo *mplayInfo) { (void)mplayInfo; }
void m4aMPlayFadeOut(struct MusicPlayerInfo *mplayInfo, u16 speed) { (void)mplayInfo; (void)speed; }
void m4aMPlayFadeOutTemporarily(struct MusicPlayerInfo *mplayInfo, u16 speed) { (void)mplayInfo; (void)speed; }
void m4aMPlayFadeIn(struct MusicPlayerInfo *mplayInfo, u16 speed) { (void)mplayInfo; (void)speed; }
void m4aMPlayImmInit(struct MusicPlayerInfo *mplayInfo) { (void)mplayInfo; }
void m4aMPlayStop(struct MusicPlayerInfo *mplayInfo) { (void)mplayInfo; }
void FadeOutBody(struct MusicPlayerInfo *mplayInfo) { (void)mplayInfo; }
void TrkVolPitSet(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track) { (void)mplayInfo; (void)track; }
void MPlayContinue(struct MusicPlayerInfo *mplayInfo) { (void)mplayInfo; }
void MPlayStart(struct MusicPlayerInfo *mplayInfo, struct SongHeader *songHeader) { if (mplayInfo != NULL) mplayInfo->songHeader = songHeader; }
void ClearModM(struct MusicPlayerTrack *track) { (void)track; }
void m4aMPlayTempoControl(struct MusicPlayerInfo *mplayInfo, u16 tempo) { (void)mplayInfo; (void)tempo; }
void m4aMPlayVolumeControl(struct MusicPlayerInfo *mplayInfo, u16 trackBits, u16 volume) { (void)mplayInfo; (void)trackBits; (void)volume; }
void m4aMPlayPitchControl(struct MusicPlayerInfo *mplayInfo, u16 trackBits, s16 pitch) { (void)mplayInfo; (void)trackBits; (void)pitch; }
void m4aMPlayPanpotControl(struct MusicPlayerInfo *mplayInfo, u16 trackBits, s8 pan) { (void)mplayInfo; (void)trackBits; (void)pan; }
void m4aMPlayModDepthSet(struct MusicPlayerInfo *mplayInfo, u16 trackBits, u8 modDepth) { (void)mplayInfo; (void)trackBits; (void)modDepth; }
void m4aMPlayLFOSpeedSet(struct MusicPlayerInfo *mplayInfo, u16 trackBits, u8 lfoSpeed) { (void)mplayInfo; (void)trackBits; (void)lfoSpeed; }
struct MusicPlayerInfo *SetPokemonCryTone(struct ToneData *tone) { (void)tone; return NULL; }
void SetPokemonCryVolume(u8 val) { (void)val; }
void SetPokemonCryPanpot(s8 val) { (void)val; }
void SetPokemonCryPitch(s16 val) { (void)val; }
void SetPokemonCryLength(u16 val) { (void)val; }
void SetPokemonCryRelease(u8 val) { (void)val; }
void SetPokemonCryProgress(u32 val) { (void)val; }
bool32 IsPokemonCryPlaying(struct MusicPlayerInfo *mplayInfo) { (void)mplayInfo; return FALSE; }
void SetPokemonCryChorus(s8 val) { (void)val; }
void SetPokemonCryStereo(u32 val) { (void)val; }
void SetPokemonCryPriority(u8 val) { (void)val; }
void m4aSoundMode(u32 mode) { (void)mode; }
void SampleFreqSet(u32 freq) { (void)freq; }
void SoundInit(struct SoundInfo *soundInfo) { (void)soundInfo; }
void MPlayExtender(struct CgbChannel *cgbChans) { (void)cgbChans; }
u32 umul3232H32(u32 multiplier, u32 multiplicand) { u64 result = (u64)multiplier * (u64)multiplicand; return (u32)(result >> 32); }
void SoundMain(void) {}
void SoundMainBTM(void) {}
void TrackStop(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track) { (void)mplayInfo; (void)track; }
void MPlayMain(struct MusicPlayerInfo *mplayInfo) { (void)mplayInfo; }
void RealClearChain(void *x) { (void)x; }
void ClearChain(void *x) { (void)x; }
void Clear64byte(void *addr) { if (addr != NULL) memset(addr, 0, 64); }
void MPlayOpen(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track, u8 a3) { (void)track; (void)a3; if (mplayInfo != NULL) mplayInfo->status = 0; }
void CgbSound(void) {}
void CgbOscOff(u8 a0) { (void)a0; }
void CgbModVol(struct CgbChannel *chan) { (void)chan; }
u32 MidiKeyToCgbFreq(u8 a0, u8 a1, u8 a2) { (void)a0; (void)a1; (void)a2; return 0; }
void DummyFunc(void) {}
void MPlayJumpTableCopy(MPlayFunc *mplayJumpTable) { (void)mplayJumpTable; }
void ply_note(u32 note_cmd, struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track) { (void)note_cmd; (void)mplayInfo; (void)track; }
void ply_endtie(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track) { (void)mplayInfo; (void)track; }
void ply_lfos(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track) { (void)mplayInfo; (void)track; }
void ply_mod(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track) { (void)mplayInfo; (void)track; }
void firered_portable_m4aSoundVSync(void) {}
void firered_portable_m4aInitAssets(void) {}

#else

#define PORTABLE_MPLAY_JUMP_TABLE_COUNT 36
#define PORTABLE_M4A_MIN_PRIORITY 0xFF
#define PORTABLE_SONG_TABLE_COUNT (MUS_TEACHY_TV_MENU + 1)
#define PORTABLE_SE1_PLAYER_INDEX 1

extern void *const gMPlayJumpTableTemplate[];
extern const u8 gClockTable[];

static struct MusicPlayerTrack sMPlayTrack_BGM[10];
static struct MusicPlayerTrack sMPlayTrack_SE1[3];
static struct MusicPlayerTrack sMPlayTrack_SE2[9];
static struct MusicPlayerTrack sMPlayTrack_SE3[1];

const struct ToneData voicegroup000 = {0};
static const u8 sPortableSilentTrack[] = {
    0xB1, // FINE
};
static const u8 sSeBikeBellTrack1[] = {
    0xBC, 0x00,       // KEYSH, 0
    0xBB, 0x3C,       // TEMPO, 60
    0xBD, 0x00,       // VOICE, 0
    0xBE, 0x5A,       // VOL, 90
    0xFE, 0x3C, 0x7F, // N96, Cn3, v127
    0xB0,             // W96
    0xB1,             // FINE
};

static struct WaveData *sBicycleBellWaveData;
static bool8 sPortableAssetsInitialized;
static bool8 sPortableAssetsReady;
static bool8 sPortableAudioSelfTestLogged;
static bool8 sPortableAudioSelfTestTriggered;
static bool8 sPortableSoundMainMarked;

static struct ToneData sVoicegroup128Minimal[] = {
    {
        .type = TONEDATA_TYPE_FIX,
        .key = 60,
        .length = 0,
        .pan_sweep = 0,
        .wav = NULL,
        .attack = 255,
        .decay = 249,
        .sustain = 0,
        .release = 165,
    },
};

const struct SongHeader mus_victory_gym_leader = {
    .trackCount = 1,
    .blockCount = 0,
    .priority = 0,
    .reverb = 0,
    .tone = (struct ToneData *)&voicegroup000,
    .part = { (u8 *)sPortableSilentTrack },
};

static struct SongHeader sSeBikeBellHeader = {
    .trackCount = 1,
    .blockCount = 0,
    .priority = 4,
    .reverb = SOUND_MODE_REVERB_SET | 50,
    .tone = sVoicegroup128Minimal,
    .part = { (u8 *)sSeBikeBellTrack1 },
};

const struct Song gSongTable[PORTABLE_SONG_TABLE_COUNT] = {
    [MUS_DUMMY] = { &mus_victory_gym_leader, 0, 0 },
    [SE_BIKE_BELL] = { &sSeBikeBellHeader, PORTABLE_SE1_PLAYER_INDEX, 0 },
    [MUS_VICTORY_GYM_LEADER] = { &mus_victory_gym_leader, 0, 0 },
};

const struct MusicPlayer gMPlayTable[] = {
    { &gMPlayInfo_BGM, sMPlayTrack_BGM, 10, 0 },
    { &gMPlayInfo_SE1, sMPlayTrack_SE1, 3, 1 },
    { &gMPlayInfo_SE2, sMPlayTrack_SE2, 9, 1 },
    { &gMPlayInfo_SE3, sMPlayTrack_SE3, 1, 0 },
};
char SoundMainRAM[0x800] = {0};
char gNumMusicPlayers[] = {4, 0};
char gMaxLines[] = {12, 0, 0, 0};

static bool32 portable_m4a_trim_path_components(char *path, size_t count)
{
    size_t i;

    for (i = 0; i < count; i++)
    {
        char *separator = strrchr(path, '\\');
        char *alt_separator = strrchr(path, '/');

        if (separator == NULL || (alt_separator != NULL && alt_separator > separator))
            separator = alt_separator;

        if (separator == NULL)
            return FALSE;

        *separator = '\0';
    }

    return TRUE;
}

#ifdef _WIN32
static bool32 portable_m4a_get_module_dir(char *path, size_t path_size)
{
    DWORD length = GetModuleFileNameA(NULL, path, (DWORD)path_size);

    if (length == 0 || length >= path_size)
        return FALSE;

    return portable_m4a_trim_path_components(path, 1);
}
#endif

static bool32 portable_m4a_get_repo_root(char *path, size_t path_size)
{
#ifdef _WIN32
    if (portable_m4a_get_module_dir(path, path_size))
    {
        if (portable_m4a_trim_path_components(path, 1))
            return TRUE;
    }
#endif

    if (snprintf(path, path_size, "%s", __FILE__) < 0)
        return FALSE;

    return portable_m4a_trim_path_components(path, 4);
}

static FILE *portable_m4a_open_wave_file(const char *relative_path)
{
    char path[PATH_MAX];

    if (!portable_m4a_get_repo_root(path, sizeof(path)))
        return NULL;

    if (snprintf(path, sizeof(path), "%s/%s", path, relative_path) < 0)
        return NULL;

    return fopen(path, "rb");
}

static void portable_m4a_write_repo_marker(const char *relative_path, const char *contents)
{
    char path[PATH_MAX];
    FILE *marker;

    if (!portable_m4a_get_repo_root(path, sizeof(path)))
        return;

    if (snprintf(path, sizeof(path), "%s/%s", path, relative_path) < 0)
        return;

    marker = fopen(path, "wb");
    if (marker == NULL)
        return;

    fputs(contents, marker);
    fclose(marker);
}

static struct WaveData *portable_m4a_load_wave_data(const char *relative_path)
{
    struct WaveData *wave = NULL;
    FILE *file = portable_m4a_open_wave_file(relative_path);
    long file_size;

    if (file == NULL)
        return NULL;

    if (fseek(file, 0, SEEK_END) != 0)
        goto cleanup;

    file_size = ftell(file);
    if (file_size < (long)sizeof(struct WaveData))
        goto cleanup;

    if (fseek(file, 0, SEEK_SET) != 0)
        goto cleanup;

    wave = malloc((size_t)file_size);
    if (wave == NULL)
        goto cleanup;

    if (fread(wave, 1, (size_t)file_size, file) != (size_t)file_size)
    {
        free(wave);
        wave = NULL;
        goto cleanup;
    }

cleanup:
    fclose(file);
    return wave;
}

static void portable_m4a_init_assets(void)
{
    if (sPortableAssetsInitialized)
        return;

    sPortableAssetsInitialized = TRUE;
    sPortableAssetsReady = FALSE;
    sSeBikeBellHeader.tone = (struct ToneData *)&voicegroup000;
    sSeBikeBellHeader.part[0] = (u8 *)sPortableSilentTrack;
    portable_m4a_write_repo_marker("build/portable_audio_init.ok",
                                   "portable audio init entered\n");

    sBicycleBellWaveData = portable_m4a_load_wave_data("sound/direct_sound_samples/bicycle_bell.bin");
    if (sBicycleBellWaveData == NULL)
        return;

    sVoicegroup128Minimal[0].wav = sBicycleBellWaveData;
    sSeBikeBellHeader.tone = sVoicegroup128Minimal;
    sSeBikeBellHeader.part[0] = (u8 *)sSeBikeBellTrack1;
    sPortableAssetsReady = TRUE;

    if (getenv("FIRERED_AUDIO_SELFTEST") != NULL)
        portable_m4a_write_repo_marker("build/portable_audio_asset.ok",
                                       "portable audio self-test: bicycle bell asset loaded\n");
}

void firered_portable_m4aInitAssets(void)
{
    portable_m4a_init_assets();
}

static u8 portable_m4a_read_byte_unchecked(struct MusicPlayerTrack *track)
{
    u8 value = *track->cmdPtr;
    track->cmdPtr++;
    return value;
}

static u32 portable_m4a_read_u32(const u8 *ptr)
{
    return (u32)ptr[0]
         | ((u32)ptr[1] << 8)
         | ((u32)ptr[2] << 16)
         | ((u32)ptr[3] << 24);
}

static void portable_m4a_clear_modm(struct MusicPlayerTrack *track)
{
    ClearModM(track);
}

static void portable_m4a_submit_current_pcm(struct SoundInfo *soundInfo)
{
    size_t sample_count;
    const int8_t *fifo_a;
    const int8_t *fifo_b;

    if (soundInfo == NULL)
        return;

    sample_count = (soundInfo->pcmSamplesPerVBlank > 0) ? (size_t)soundInfo->pcmSamplesPerVBlank : 0;
    if (sample_count > PCM_DMA_BUF_SIZE)
        sample_count = PCM_DMA_BUF_SIZE;

    fifo_a = (const int8_t *)soundInfo->pcmBuffer;
    fifo_b = (const int8_t *)soundInfo->pcmBuffer + PCM_DMA_BUF_SIZE;
    engine_audio_submit_gba_directsound(fifo_a, fifo_b, sample_count);
}

static s8 portable_m4a_clamp_sample(s32 sample)
{
    if (sample > 127)
        return 127;
    if (sample < -128)
        return -128;
    return (s8)sample;
}

static void portable_m4a_mix_directsound(struct SoundInfo *soundInfo)
{
    s8 *right_buffer;
    s8 *left_buffer;
    size_t sample_count;
    u8 channel_index;
    bool32 mixed_nonzero = FALSE;

    if (soundInfo == NULL)
        return;

    memset(soundInfo->pcmBuffer, 0, sizeof(soundInfo->pcmBuffer));

    sample_count = (soundInfo->pcmSamplesPerVBlank > 0) ? (size_t)soundInfo->pcmSamplesPerVBlank : 0;
    if (sample_count > PCM_DMA_BUF_SIZE)
        sample_count = PCM_DMA_BUF_SIZE;

    if (sample_count == 0)
        return;

    right_buffer = (s8 *)soundInfo->pcmBuffer;
    left_buffer = (s8 *)soundInfo->pcmBuffer + PCM_DMA_BUF_SIZE;

    for (channel_index = 0; channel_index < soundInfo->maxChans; channel_index++)
    {
        struct SoundChannel *channel = &soundInfo->chans[channel_index];
        struct WaveData *wave = channel->wav;
        s8 *src;
        s8 *loop_src = NULL;
        u32 count;
        u32 loop_count = 0;
        u32 frac;
        u32 step;
        u8 env;
        u8 status;
        u8 env_right;
        u8 env_left;
        bool32 channel_alive = TRUE;
        size_t sample_index;

        if (!(channel->statusFlags & SOUND_CHANNEL_SF_ON) || wave == NULL)
            continue;

        status = channel->statusFlags;
        if (status & SOUND_CHANNEL_SF_START)
        {
            if (status & SOUND_CHANNEL_SF_STOP)
            {
                channel->statusFlags = 0;
                continue;
            }

            status = SOUND_CHANNEL_SF_ENV_ATTACK;
            channel->currentPointer = wave->data + channel->count;
            channel->count = wave->size - channel->count;
            channel->envelopeVolume = 0;
            channel->fw = 0;
        }

        env = channel->envelopeVolume;
        if ((status & SOUND_CHANNEL_SF_START) == 0)
        {
            if (status & SOUND_CHANNEL_SF_IEC)
            {
                if (--channel->pseudoEchoLength == 0)
                {
                    channel->statusFlags = 0;
                    continue;
                }
            }
            else if (status & SOUND_CHANNEL_SF_STOP)
            {
                env = (u8)(((u32)env * channel->release) >> 8);
                if (env <= channel->pseudoEchoVolume)
                {
                    if (channel->pseudoEchoVolume == 0)
                    {
                        channel->statusFlags = 0;
                        continue;
                    }

                    status |= SOUND_CHANNEL_SF_IEC;
                }
            }
            else if ((status & SOUND_CHANNEL_SF_ENV) == SOUND_CHANNEL_SF_ENV_DECAY)
            {
                env = (u8)(((u32)env * channel->decay) >> 8);
                if (env <= channel->sustain)
                {
                    env = channel->sustain;
                    if (env == 0)
                    {
                        if (channel->pseudoEchoVolume == 0)
                        {
                            channel->statusFlags = 0;
                            continue;
                        }

                        status |= SOUND_CHANNEL_SF_IEC;
                    }
                    else
                    {
                        status--;
                    }
                }
            }
            else if ((status & SOUND_CHANNEL_SF_ENV) == SOUND_CHANNEL_SF_ENV_ATTACK)
            {
                u16 attack_env = env + channel->attack;

                if (attack_env >= 0xFF)
                {
                    env = 0xFF;
                    status--;
                }
                else
                {
                    env = (u8)attack_env;
                }
            }
        }

        channel->statusFlags = status;
        channel->envelopeVolume = env;

        {
            u32 mix_volume = ((soundInfo->masterVolume + 1) * env) >> 4;
            env_right = (u8)(((u32)channel->rightVolume * mix_volume) >> 8);
            env_left = (u8)(((u32)channel->leftVolume * mix_volume) >> 8);
            channel->envelopeVolumeRight = env_right;
            channel->envelopeVolumeLeft = env_left;
        }

        if ((status & SOUND_CHANNEL_SF_LOOP) && wave->loopStart < wave->size)
        {
            loop_src = wave->data + wave->loopStart;
            loop_count = wave->size - wave->loopStart;
        }

        src = channel->currentPointer;
        count = channel->count;
        frac = channel->fw;
        step = channel->frequency;

        for (sample_index = 0; sample_index < sample_count; sample_index++)
        {
            s32 sample;
            s32 mixed_right;
            s32 mixed_left;

            if (count == 0)
            {
                if (loop_src != NULL && loop_count != 0)
                {
                    src = loop_src;
                    count = loop_count;
                    frac = 0;
                }
                else
                {
                    channel_alive = FALSE;
                    break;
                }
            }

            sample = *src;
            mixed_right = right_buffer[sample_index] + ((sample * env_right) >> 8);
            mixed_left = left_buffer[sample_index] + ((sample * env_left) >> 8);
            right_buffer[sample_index] = portable_m4a_clamp_sample(mixed_right);
            left_buffer[sample_index] = portable_m4a_clamp_sample(mixed_left);
            if (right_buffer[sample_index] != 0 || left_buffer[sample_index] != 0)
                mixed_nonzero = TRUE;

            if (channel->type & TONEDATA_TYPE_FIX)
            {
                src++;
                count--;
            }
            else
            {
                u32 advance;

                frac += step;
                if (soundInfo->divFreq == 0)
                    advance = 1;
                else
                    advance = frac / soundInfo->divFreq;

                if (soundInfo->divFreq != 0)
                    frac %= soundInfo->divFreq;

                if (advance == 0)
                    continue;

                if (advance >= count)
                {
                    src += count;
                    count = 0;
                }
                else
                {
                    src += advance;
                    count -= advance;
                }
            }
        }

        if (!channel_alive)
        {
            channel->statusFlags = 0;
            channel->count = 0;
            channel->fw = 0;
            continue;
        }

        channel->currentPointer = src;
        channel->count = count;
        channel->fw = frac;
    }

    if (mixed_nonzero && getenv("FIRERED_AUDIO_SELFTEST") != NULL)
    {
        if (!sPortableAudioSelfTestLogged)
        {
            sPortableAudioSelfTestLogged = TRUE;
            portable_m4a_write_repo_marker("build/portable_audio_mixer_nonzero.ok",
                                           "portable audio mixer produced nonzero DirectSound samples\n");
        }
        portable_m4a_write_repo_marker("build/portable_audio_selftest.ok",
                                       "portable audio self-test: mixed nonzero DirectSound samples\n");
    }
}

u32 umul3232H32(u32 multiplier, u32 multiplicand)
{
    u64 result = (u64)multiplier * (u64)multiplicand;
    return (u32)(result >> 32);
}

void RealClearChain(void *x)
{
    struct SoundChannel *channel = x;
    struct MusicPlayerTrack *track;

    if (channel == NULL)
        return;

    track = channel->track;
    if (track != NULL)
    {
        struct SoundChannel *next = channel->nextChannelPointer;
        struct SoundChannel *prev = channel->prevChannelPointer;

        if (prev != NULL)
            prev->nextChannelPointer = next;
        else
            track->chan = next;

        if (next != NULL)
            next->prevChannelPointer = prev;

        channel->track = NULL;
    }
}

void ply_fine(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    struct SoundChannel *channel = track->chan;

    (void)mplayInfo;

    while (channel != NULL)
    {
        struct SoundChannel *next = channel->nextChannelPointer;

        if (channel->statusFlags & SOUND_CHANNEL_SF_ON)
            channel->statusFlags |= SOUND_CHANNEL_SF_STOP;

        RealClearChain(channel);
        channel = next;
    }

    track->flags = 0;
}

void MPlayJumpTableCopy(MPlayFunc *mplayJumpTable)
{
    if (mplayJumpTable == NULL)
        return;

    memcpy(mplayJumpTable, gMPlayJumpTableTemplate, PORTABLE_MPLAY_JUMP_TABLE_COUNT * sizeof(*mplayJumpTable));
}

void ply_goto(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    (void)mplayInfo;
    track->cmdPtr = (u8 *)(uintptr_t)portable_m4a_read_u32(track->cmdPtr);
}

void ply_patt(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    if (track->patternLevel >= 3)
    {
        ply_fine(mplayInfo, track);
        return;
    }

    track->patternStack[track->patternLevel] = track->cmdPtr + 4;
    track->patternLevel++;
    ply_goto(mplayInfo, track);
}

void ply_pend(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    (void)mplayInfo;

    if (track->patternLevel == 0)
        return;

    track->patternLevel--;
    track->cmdPtr = track->patternStack[track->patternLevel];
}

void ply_rept(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    u8 *cmd_ptr = track->cmdPtr;
    u8 repeat_limit;

    (void)mplayInfo;

    repeat_limit = cmd_ptr[0];
    if (repeat_limit == 0)
    {
        track->cmdPtr = cmd_ptr + 1;
        ply_goto(mplayInfo, track);
        return;
    }

    track->repN++;
    if (track->repN < repeat_limit)
    {
        ply_goto(mplayInfo, track);
        return;
    }

    track->repN = 0;
    track->cmdPtr = cmd_ptr + 5;
}

void ply_prio(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    (void)mplayInfo;
    track->priority = portable_m4a_read_byte_unchecked(track);
}

void ply_tempo(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    u16 tempo = (u16)portable_m4a_read_byte_unchecked(track) << 1;

    mplayInfo->tempoD = tempo;
    mplayInfo->tempoI = (u16)((mplayInfo->tempoD * mplayInfo->tempoU) >> 8);
}

void ply_keysh(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    (void)mplayInfo;
    track->keyShift = (s8)portable_m4a_read_byte_unchecked(track);
    track->flags |= MPT_FLG_PITCHG;
}

void ply_voice(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    u8 voice = portable_m4a_read_byte_unchecked(track);
    const struct ToneData *tone = &mplayInfo->tone[voice];

    portable_m4a_init_assets();
    memcpy(&track->tone, tone, sizeof(*tone));
}

void ply_vol(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    (void)mplayInfo;
    track->vol = portable_m4a_read_byte_unchecked(track);
    track->flags |= MPT_FLG_VOLCHG;
}

void ply_pan(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    (void)mplayInfo;
    track->pan = (s8)portable_m4a_read_byte_unchecked(track) - C_V;
    track->flags |= MPT_FLG_VOLCHG;
}

void ply_bend(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    (void)mplayInfo;
    track->bend = (s8)portable_m4a_read_byte_unchecked(track) - C_V;
    track->flags |= MPT_FLG_PITCHG;
}

void ply_bendr(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    (void)mplayInfo;
    track->bendRange = portable_m4a_read_byte_unchecked(track);
    track->flags |= MPT_FLG_PITCHG;
}

void ply_lfodl(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    (void)mplayInfo;
    track->lfoDelay = portable_m4a_read_byte_unchecked(track);
}

void ply_modt(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    u8 mod_type;

    (void)mplayInfo;

    mod_type = portable_m4a_read_byte_unchecked(track);
    if (track->modT != mod_type)
    {
        track->modT = mod_type;
        track->flags |= MPT_FLG_VOLCHG | MPT_FLG_PITCHG;
    }
}

void ply_tune(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    (void)mplayInfo;
    track->tune = (s8)portable_m4a_read_byte_unchecked(track) - C_V;
    track->flags |= MPT_FLG_PITCHG;
}

void ply_port(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    u8 reg_offset;

    (void)mplayInfo;

    reg_offset = portable_m4a_read_byte_unchecked(track);
    *(vu8 *)((uintptr_t)REG_SOUND1CNT_L + reg_offset) = reg_offset;
}

void firered_portable_m4aSoundVSync(void)
{
    struct SoundInfo *soundInfo = SOUND_INFO_PTR;

    if (soundInfo == NULL)
        return;

    if (soundInfo->ident != ID_NUMBER && soundInfo->ident != ID_NUMBER + 1)
        return;

    soundInfo->pcmDmaCounter--;
    if (soundInfo->pcmDmaCounter > 0)
        return;

    soundInfo->pcmDmaCounter = soundInfo->pcmDmaPeriod;

    if (REG_DMA1CNT & (DMA_REPEAT << 16))
        REG_DMA1CNT = ((DMA_ENABLE | DMA_START_NOW | DMA_32BIT | DMA_SRC_INC | DMA_DEST_FIXED) << 16) | 4;
    if (REG_DMA2CNT & (DMA_REPEAT << 16))
        REG_DMA2CNT = ((DMA_ENABLE | DMA_START_NOW | DMA_32BIT | DMA_SRC_INC | DMA_DEST_FIXED) << 16) | 4;

    REG_DMA1CNT_H = DMA_32BIT;
    REG_DMA2CNT_H = DMA_32BIT;
    REG_DMA1CNT_H = DMA_ENABLE | DMA_START_SPECIAL | DMA_32BIT | DMA_REPEAT;
    REG_DMA2CNT_H = DMA_ENABLE | DMA_START_SPECIAL | DMA_32BIT | DMA_REPEAT;
}

void TrackStop(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    struct SoundChannel *channel;

    (void)mplayInfo;

    if (!(track->flags & MPT_FLG_EXIST))
        return;

    channel = track->chan;
    while (channel != NULL)
    {
        struct SoundChannel *next = channel->nextChannelPointer;

        if (channel->statusFlags != 0 && (channel->type & TONEDATA_TYPE_CGB))
            gSoundInfo.CgbOscOff(channel->type & TONEDATA_TYPE_CGB);

        channel->statusFlags = 0;
        channel->track = NULL;
        channel = next;
    }

    track->chan = NULL;
}

void ChnVolSetAsm(struct SoundChannel *channel, struct MusicPlayerTrack *track)
{
    s32 pan = channel->rhythmPan;
    s32 velocity = channel->velocity;
    s32 right = (((0x80 + pan) * velocity) * track->volMR) >> 14;
    s32 left = (((0x7F - pan) * velocity) * track->volML) >> 14;

    if (right > 0xFF)
        right = 0xFF;
    if (left > 0xFF)
        left = 0xFF;

    channel->rightVolume = (u8)right;
    channel->leftVolume = (u8)left;
}

void ply_note(u32 note_cmd, struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    struct SoundInfo *soundInfo = SOUND_INFO_PTR;
    struct ToneData *tone = &track->tone;
    struct SoundChannel *best = NULL;
    struct SoundChannel *channel;
    u8 priority;
    s8 rhythm_pan = 0;
    u8 key = track->key;
    u8 velocity = track->velocity;
    s32 pitch_key;

    track->gateTime = gClockTable[note_cmd];
    if (*track->cmdPtr < 0x80)
    {
        key = *track->cmdPtr++;
        track->key = key;
        if (*track->cmdPtr < 0x80)
        {
            velocity = *track->cmdPtr++;
            track->velocity = velocity;
            if (*track->cmdPtr < 0x80)
                track->gateTime += *track->cmdPtr++;
        }
    }

    priority = mplayInfo->priority + track->priority;
    if (priority < mplayInfo->priority)
        priority = PORTABLE_M4A_MIN_PRIORITY;

    if (tone->type & (TONEDATA_TYPE_SPL | TONEDATA_TYPE_RHY))
    {
        const struct ToneData *tone_table = (const struct ToneData *)tone->wav;
        u8 tone_index = key;

        if (tone->type & TONEDATA_TYPE_SPL)
        {
            const u8 *key_split_table = (const u8 *)tone->wav;
            tone_index = key_split_table[key];
        }

        tone = (struct ToneData *)&tone_table[tone_index];
        if (tone->type & TONEDATA_TYPE_RHY)
            rhythm_pan = (tone->pan_sweep & 0x80) ? (s8)((tone->pan_sweep - TONEDATA_P_S_PAN) << 1) : 0;
        key = tone->key;
    }

    if ((tone->type & TONEDATA_TYPE_CGB) != 0)
    {
        u8 cgb_index = (tone->type & TONEDATA_TYPE_CGB) - 1;

        if (soundInfo == NULL || soundInfo->cgbChans == NULL)
            return;

        best = (struct SoundChannel *)&soundInfo->cgbChans[cgb_index];
        if ((best->statusFlags & SOUND_CHANNEL_SF_ON)
         && !(best->statusFlags & SOUND_CHANNEL_SF_STOP)
         && (best->priority > priority || (best->priority == priority && best->track <= track)))
        {
            return;
        }
    }
    else
    {
        u8 i;
        u8 best_priority = PORTABLE_M4A_MIN_PRIORITY;
        struct MusicPlayerTrack *best_track = NULL;

        channel = soundInfo->chans;
        for (i = 0; i < soundInfo->maxChans; i++, channel++)
        {
            if (!(channel->statusFlags & SOUND_CHANNEL_SF_ON))
            {
                best = channel;
                break;
            }

            if ((channel->statusFlags & SOUND_CHANNEL_SF_STOP) != 0)
            {
                if (best == NULL)
                {
                    best = channel;
                    best_priority = channel->priority;
                    best_track = channel->track;
                }
                continue;
            }

            if (best == NULL
             || channel->priority < best_priority
             || (channel->priority == best_priority && channel->track > best_track))
            {
                best = channel;
                best_priority = channel->priority;
                best_track = channel->track;
            }
        }

        if (best == NULL)
            return;
    }

    RealClearChain(best);
    best->prevChannelPointer = NULL;
    best->nextChannelPointer = track->chan;
    if (track->chan != NULL)
        track->chan->prevChannelPointer = best;
    track->chan = best;
    best->track = track;

    track->lfoDelayC = track->lfoDelay;
    if (track->lfoDelayC != 0)
        portable_m4a_clear_modm(track);

    TrkVolPitSet(mplayInfo, track);
    best->gateTime = track->gateTime;
    best->priority = priority;
    best->key = key;
    best->rhythmPan = rhythm_pan;
    best->midiKey = track->key;
    best->type = tone->type;
    best->wav = tone->wav;
    best->attack = tone->attack;
    best->decay = tone->decay;
    best->sustain = tone->sustain;
    best->release = tone->release;
    best->pseudoEchoVolume = track->pseudoEchoVolume;
    best->pseudoEchoLength = track->pseudoEchoLength;
    best->velocity = track->velocity;

    ChnVolSetAsm(best, track);

    pitch_key = (s32)best->key + track->keyM;
    if (pitch_key < 0)
        pitch_key = 0;

    if (tone->type & TONEDATA_TYPE_CGB)
    {
        best->frequency = soundInfo->MidiKeyToCgbFreq(
            tone->type & TONEDATA_TYPE_CGB,
            (u8)pitch_key,
            track->pitM);
    }
    else
    {
        best->count = track->unk_3C;
        best->frequency = MidiKeyToFreq(best->wav, (u8)pitch_key, track->pitM);
    }

    best->statusFlags = SOUND_CHANNEL_SF_START;
    track->flags &= 0xF0;
}

void ply_endtie(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    struct SoundChannel *channel;
    u8 key;

    (void)mplayInfo;

    if (*track->cmdPtr < 0x80)
    {
        key = *track->cmdPtr;
        track->key = key;
        track->cmdPtr++;
    }
    else
    {
        key = track->key;
    }

    channel = track->chan;
    while (channel != NULL)
    {
        if ((channel->statusFlags & (SOUND_CHANNEL_SF_START | SOUND_CHANNEL_SF_ENV)) != 0
         && !(channel->statusFlags & SOUND_CHANNEL_SF_STOP)
         && channel->midiKey == key)
        {
            channel->statusFlags |= SOUND_CHANNEL_SF_STOP;
            break;
        }
        channel = channel->nextChannelPointer;
    }
}

void ply_lfos(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    (void)mplayInfo;
    track->lfoSpeed = portable_m4a_read_byte_unchecked(track);
    if (track->lfoSpeed == 0)
        portable_m4a_clear_modm(track);
}

void ply_mod(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    (void)mplayInfo;
    track->mod = portable_m4a_read_byte_unchecked(track);
    if (track->mod == 0)
        portable_m4a_clear_modm(track);
}

void MPlayMain(struct MusicPlayerInfo *mplayInfo)
{
    struct SoundInfo *soundInfo;
    u16 tempo_counter;

    if (mplayInfo->ident != ID_NUMBER)
        return;

    mplayInfo->ident++;

    if ((s32)mplayInfo->status < 0)
        goto done;

    soundInfo = SOUND_INFO_PTR;
    FadeOutBody(mplayInfo);
    if ((s32)mplayInfo->status < 0)
        goto done;

    tempo_counter = mplayInfo->tempoC + mplayInfo->tempoI;
    do
    {
        u8 track_count = mplayInfo->trackCount;
        struct MusicPlayerTrack *track = mplayInfo->tracks;
        u32 status = 0;
        u32 bit = 1;

        while (track_count-- > 0)
        {
            if (track->flags & MPT_FLG_EXIST)
            {
                struct SoundChannel *channel = track->chan;

                status |= bit;

                while (channel != NULL)
                {
                    struct SoundChannel *next = channel->nextChannelPointer;

                    if (channel->statusFlags & SOUND_CHANNEL_SF_ON)
                    {
                        if (channel->gateTime != 0)
                        {
                            channel->gateTime--;
                            if (channel->gateTime == 0)
                                channel->statusFlags |= SOUND_CHANNEL_SF_STOP;
                        }
                    }
                    else
                    {
                        RealClearChain(channel);
                    }

                    channel = next;
                }

                if (track->flags & MPT_FLG_START)
                {
                    memset(track, 0, 64);
                    track->flags = MPT_FLG_EXIST;
                    track->bendRange = 2;
                    track->volX = 64;
                    track->lfoSpeed = 22;
                    track->tone.type = 1;
                }

                while (track->wait == 0)
                {
                    u8 cmd = *track->cmdPtr;

                    if (cmd < 0x80)
                        cmd = track->runningStatus;
                    else
                    {
                        track->cmdPtr++;
                        if (cmd >= 0xBD)
                            track->runningStatus = cmd;
                    }

                    if (cmd >= 0xCF)
                    {
                        soundInfo->plynote(cmd - 0xCF, mplayInfo, track);
                        break;
                    }

                    if (cmd > 0xB0)
                    {
                        mplayInfo->cmd = cmd - 0xB1;
                        soundInfo->MPlayJumpTable[mplayInfo->cmd](mplayInfo, track);
                        if (track->flags == 0)
                            break;
                    }
                    else
                    {
                        track->wait = gClockTable[cmd - 0x80];
                    }
                }

                if (track->wait != 0)
                {
                    s32 mod;

                    track->wait--;
                    if (track->lfoSpeed != 0 && track->mod != 0)
                    {
                        if (track->lfoDelayC != 0)
                        {
                            track->lfoDelayC--;
                        }
                        else
                        {
                            u8 prev = track->modM;

                            track->lfoSpeedC += track->lfoSpeed;
                            if (track->lfoSpeedC < 0x40)
                                mod = (s8)track->lfoSpeedC;
                            else
                                mod = 0x80 - track->lfoSpeedC;

                            mod = (track->mod * mod) >> 6;
                            if ((u8)(prev ^ mod) != 0)
                            {
                                track->modM = (u8)mod;
                                track->flags |= (track->modT == 0) ? MPT_FLG_PITCHG : MPT_FLG_VOLCHG;
                            }
                        }
                    }
                }
            }

            track++;
            bit <<= 1;
        }

        mplayInfo->clock++;
        if (status == 0)
        {
            mplayInfo->status = MUSICPLAYER_STATUS_PAUSE;
            goto done;
        }

        mplayInfo->status = status;
        tempo_counter -= 150;
    } while (tempo_counter >= 150);

    mplayInfo->tempoC = tempo_counter;

    {
        u8 track_count = mplayInfo->trackCount;
        struct MusicPlayerTrack *track = mplayInfo->tracks;

        while (track_count-- > 0)
        {
            if ((track->flags & MPT_FLG_EXIST) != 0
             && (track->flags & (MPT_FLG_VOLCHG | MPT_FLG_PITCHG)) != 0)
            {
                struct SoundChannel *channel;

                TrkVolPitSet(mplayInfo, track);
                channel = track->chan;
                while (channel != NULL)
                {
                    struct SoundChannel *next = channel->nextChannelPointer;

                    if (!(channel->statusFlags & SOUND_CHANNEL_SF_ON))
                    {
                        RealClearChain(channel);
                        channel = next;
                        continue;
                    }

                    if (track->flags & MPT_FLG_VOLCHG)
                    {
                        ChnVolSetAsm(channel, track);
                        if (channel->type & TONEDATA_TYPE_CGB)
                            ((struct CgbChannel *)channel)->modify |= CGB_CHANNEL_MO_VOL;
                    }

                    if (track->flags & MPT_FLG_PITCHG)
                    {
                        s32 key = (s32)channel->key + track->keyM;

                        if (key < 0)
                            key = 0;

                        if (channel->type & TONEDATA_TYPE_CGB)
                        {
                            ((struct CgbChannel *)channel)->frequency = soundInfo->MidiKeyToCgbFreq(
                                channel->type & TONEDATA_TYPE_CGB,
                                (u8)key,
                                track->pitM);
                            ((struct CgbChannel *)channel)->modify |= CGB_CHANNEL_MO_PIT;
                        }
                        else
                        {
                            channel->frequency = MidiKeyToFreq(channel->wav, (u8)key, track->pitM);
                        }
                    }

                    channel = next;
                }

                track->flags &= 0xF0;
            }

            track++;
        }
    }

done:
    mplayInfo->ident = ID_NUMBER;
}

void SoundMain(void)
{
    struct SoundInfo *soundInfo = SOUND_INFO_PTR;
    struct MusicPlayerInfo *player;
    MPlayMainFunc player_tick;

    if (soundInfo == NULL || soundInfo->ident != ID_NUMBER)
        return;

    portable_m4a_init_assets();
    soundInfo->ident++;
    if (!sPortableSoundMainMarked)
    {
        sPortableSoundMainMarked = TRUE;
        portable_m4a_write_repo_marker("build/portable_audio_soundmain.ok",
                                       "portable audio SoundMain entered\n");
    }

    if (getenv("FIRERED_AUDIO_SELFTEST") != NULL && sPortableAssetsReady && !sPortableAudioSelfTestTriggered)
    {
        sPortableAudioSelfTestTriggered = TRUE;
        m4aSongNumStart(SE_BIKE_BELL);
    }

    player_tick = soundInfo->MPlayMainHead;
    if (player_tick != NULL)
    {
        player = soundInfo->musicPlayerHead;
        while (player != NULL)
        {
            struct MusicPlayerInfo *next = player->musicPlayerNext;

            player_tick(player);
            player = next;
        }
    }

    if (soundInfo->CgbSound != NULL)
        soundInfo->CgbSound();

    portable_m4a_mix_directsound(soundInfo);
    portable_m4a_submit_current_pcm(soundInfo);
    soundInfo->ident = ID_NUMBER;
}

void SoundMainBTM(void)
{
}

#endif
