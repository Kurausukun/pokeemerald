#ifndef GUARD_RANDOM_H
#define GUARD_RANDOM_H

struct PCG16
{
    u32 state;
    u32 inc;
    u16 value;
};

extern struct PCG16 gPCGRng;
extern struct PCG16 gPCGRng2;

//Returns a 16-bit pseudorandom number
u16 Random(void);
u16 Random2(void);

//Returns a 32-bit pseudorandom number
#define Random32() (Random() | (Random() << 16))

// The number 1103515245 comes from the example implementation of rand and srand
// in the ISO C standard.
#define ISO_RANDOMIZE1(val)(1103515245 * (val) + 24691)
#define ISO_RANDOMIZE2(val)(1103515245 * (val) + 12345)

//Sets the initial seed value of the pseudorandom number generator
void SeedRng(u32 seed);
void SeedRng2(u32 seed);

#endif // GUARD_RANDOM_H
