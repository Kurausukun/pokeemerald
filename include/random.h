#ifndef GUARD_RANDOM_H
#define GUARD_RANDOM_H

struct PCG32
{
    u64 state;
    u64 inc;
    u32 value;
};

extern struct PCG32 gPCGRng;
extern struct PCG32 gPCGRng2;

//Returns a 16-bit pseudorandom number
u16 Random(void);
u16 Random2(void);

//Returns a 32-bit pseudorandom number
u32 Random32(void);

// The number 1103515245 comes from the example implementation of rand and srand
// in the ISO C standard.
#define ISO_RANDOMIZE1(val)(1103515245 * (val) + 24691)
#define ISO_RANDOMIZE2(val)(1103515245 * (val) + 12345)

//Sets the initial seed value of the pseudorandom number generator
void SeedRng(u16 seed);
void SeedRng2(u16 seed);

#endif // GUARD_RANDOM_H
