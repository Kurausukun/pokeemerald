#include "global.h"

#define VOICE_DIRECT_SOUND_DEFAULT 0
#define VOICE_SQUARE_1 1
#define VOICE_SQUARE_2 2
#define VOICE_PROGRAMMABLE_WAVE 3
#define VOICE_NOISE 4
#define VOICE_DIRECT_SOUND_NO_RESAMPLE 8
#define VOICE_SQUARE_1_ALT 9
#define VOICE_SQUARE_2_ALT 10
#define VOICE_PROGRAMMABLE_WAVE_ALT 11
#define VOICE_NOISE_ALT 12
#define VOICE_DIRECT_SOUND_REVERSE 16
#define VOICE_KEYSPLIT 64
#define VOICE_KEYSPLIT_ALL 128

extern const u8 DirectSoundWaveData_sc88pro_piano1_48[];
extern const u8 DirectSoundWaveData_sc88pro_piano1_60[];
extern const u8 DirectSoundWaveData_sc88pro_piano1_72[];
extern const u8 DirectSoundWaveData_sc88pro_piano1_84[];

struct DirectSoundVoice
{
    u8 baseMidiKey;
    u8 pan;
    const u8 * const ALIGNED(4) samplePointer;
    u8 attack;
    u8 decay;
    u8 sustain;
    u8 release;
};

struct Square1Voice
{
    u8 baseMidiKey;
    u8 pan;
    u8 sweep;
    u8 dutyCycle;
    u8 ALIGNED(4) attack;
    u8 decay;
    u8 sustain;
    u8 release;
};

struct Square2Voice
{
    u8 baseMidiKey;
    u8 pan;
    u8 ALIGNED(2) dutyCycle;
    u8 ALIGNED(4) attack;
    u8 decay;
    u8 sustain;
    u8 release;
};

struct ProgrammableWaveVoice
{
    u8 baseMidiKey;
    u8 pan;
    const u8 * const ALIGNED(4) wavePointer;
    u8 attack;
    u8 decay;
    u8 sustain;
    u8 release;
};

struct NoiseVoice
{
    u8 baseMidiKey;
    u8 pan;
    u8 noisePeriod;
    u8 ALIGNED(4) attack;
    u8 decay;
    u8 sustain;
    u8 release;
};

struct KeySplitVoice
{
    const void * const ALIGNED(4) voiceGroupPointer;
    const u8 * const keySplitTablePointer;
};

struct Voice
{
    u8 __attribute__((packed)) type;
    union
    {
        struct DirectSoundVoice directSoundVoice;
        struct Square1Voice square1Voice;
        struct Square2Voice square2Voice;
        struct ProgrammableWaveVoice programmableWaveVoice;
        struct NoiseVoice noiseVoice;
        struct KeySplitVoice keySplitVoice;
        const void * const ALIGNED(8) keySplitAllVoiceGroupPointer;
    } temp;
};

const struct Voice voicegroup005[] =
{
    {
        .type = VOICE_DIRECT_SOUND_DEFAULT,
        .temp =
        {
            .directSoundVoice =
            {
                .baseMidiKey = 60,
                .pan = 0,
                .samplePointer = DirectSoundWaveData_sc88pro_piano1_48,
                .attack = 255,
                .decay = 252,
                .sustain = 0,
                .release = 239,
            },
        },
    },
    {
        .type = VOICE_DIRECT_SOUND_DEFAULT,
        .temp =
        {
            .directSoundVoice =
            {
                .baseMidiKey = 60,
                .pan = 0,
                .samplePointer = DirectSoundWaveData_sc88pro_piano1_60,
                .attack = 255,
                .decay = 250,
                .sustain = 0,
                .release = 221,
            },
        },
    },
    {
        .type = VOICE_DIRECT_SOUND_DEFAULT,
        .temp =
        {
            .directSoundVoice =
            {
                .baseMidiKey = 60,
                .pan = 0,
                .samplePointer = DirectSoundWaveData_sc88pro_piano1_72,
                .attack = 255,
                .decay = 250,
                .sustain = 0,
                .release = 221,
            },
        },
    },
    {
        .type = VOICE_DIRECT_SOUND_DEFAULT,
        .temp =
        {
            .directSoundVoice =
            {
                .baseMidiKey = 60,
                .pan = 0,
                .samplePointer = DirectSoundWaveData_sc88pro_piano1_84,
                .attack = 255,
                .decay = 247,
                .sustain = 0,
                .release = 221,
            },
        },
    },
};
