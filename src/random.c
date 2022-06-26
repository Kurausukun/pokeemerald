#include "global.h"
#include "random.h"

EWRAM_DATA static u8 sUnknown = 0;
EWRAM_DATA static u32 sRandCount = 0;

// IWRAM common
struct PCG16 gPCGRng;
struct PCG16 gPCGRng2;

static u16 PCG16Random(struct PCG16 * PCGRng)
{
    u32 oldstate;
    u16 xorshifted;
    u16 rot;

    oldstate = PCGRng->state;
    // Advance internal state
    PCGRng->state = oldstate * 747796405U + (PCGRng->inc | 1u);
    // Calculate output function (XSH RR), uses old state for max ILP
    xorshifted = ((oldstate >> 10u) ^ oldstate) >> 12u;
    rot = oldstate >> 28u;
    return PCGRng->value = (xorshifted >> rot) | (xorshifted << ((-rot) & 15));
}

u16 Random(void)
{
    u16 temp = PCG16Random(&gPCGRng);
    sRandCount++;
    return temp;
}

void SeedRng(u32 seed)
{
    gPCGRng.state = seed;
    gPCGRng.inc = seed;
    sUnknown = 0;
}

void SeedRng2(u32 seed)
{
    gPCGRng2.state = seed;
    gPCGRng.inc = seed;
}

u16 Random2(void)
{
    u16 temp = PCG16Random(&gPCGRng2);
    return temp;
}
