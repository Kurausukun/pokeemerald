#include <string.h>
#include "gba/m4a_internal.h"

extern const u8 gCgb3Vol[];

#define BSS_CODE __attribute__((section(".bss.code")))

BSS_CODE ALIGNED(4) char SoundMainRAM_Buffer[0x800] = {0};

struct SoundInfo gSoundInfo;
struct PokemonCrySong gPokemonCrySongs[MAX_POKEMON_CRIES];
struct MusicPlayerInfo gPokemonCryMusicPlayers[MAX_POKEMON_CRIES];
void *gMPlayJumpTable[36];
struct CgbChannel gCgbChans[4];
struct MusicPlayerTrack gPokemonCryTracks[MAX_POKEMON_CRIES * 2];
struct PokemonCrySong gPokemonCrySong;
struct MusicPlayerInfo gMPlayInfo_BGM;
struct MusicPlayerInfo gMPlayInfo_SE1;
struct MusicPlayerInfo gMPlayInfo_SE2;
struct MusicPlayerInfo gMPlayInfo_SE3;
u8 gMPlayMemAccArea[0x10];
