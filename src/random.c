#include "global.h"
#include "random.h"
#include "mgba.h"

EWRAM_DATA static u8 sUnknown = 0;
EWRAM_DATA static u32 sRandCount = 0;

// IWRAM common
struct PCG32 * gPCGRng;
struct PCG32 * gPCGRng2;

static u32 PCG32Random(struct PCG32 * PCGRng)
{
    u64 oldstate;
    u32 xorshifted;
    u32 rot;

    oldstate = PCGRng->state;
    // Advance internal state
    PCGRng->state = oldstate * 6364136223846793005ULL + (PCGRng->inc | 1);
    // Calculate output function (XSH RR), uses old state for max ILP
    xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

u16 Random(void)
{
    gPCGRng->state = PCG32Random(gPCGRng);
    sRandCount++;
    //MgbaPrintf(MGBA_LOG_INFO, "Rng1: %d", gPCGRng->state >> 16);
    return gPCGRng->state >> 16;
}

void SeedRng(u16 seed)
{
    gPCGRng->state = seed;
    gPCGRng->inc = REG_TM0CNT_L;
    sUnknown = 0;
}

void SeedRng2(u16 seed)
{
    gPCGRng2->state = seed;
    gPCGRng->inc = REG_TM1CNT_L;
}

u16 Random2(void)
{
    gPCGRng2->state = PCG32Random(gPCGRng2);
    //MgbaPrintf(MGBA_LOG_INFO, "Rng2: %d", gPCGRng2->state >> 16);
    return gPCGRng2->state >> 16;
}

u32 Random32(void)
{
    gPCGRng->state = PCG32Random(gPCGRng);
    sRandCount++;
    //MgbaPrintf(MGBA_LOG_INFO, "32: %d", gPCGRng->state);
    return gPCGRng->state;
}
