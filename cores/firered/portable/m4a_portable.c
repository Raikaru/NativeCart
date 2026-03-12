#include "../../../include/global.h"
#include "../../../include/m4a.h"

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
void m4aSongNumStop(u16 n) { (void)n; }
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
