#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <SDL2/SDL.h>

#define NO_UNDERSCORE_HACK

#include "global.h"
#include "platform.h"
#include "rtc.h"
#include "gba/defines.h"
#include "gba/m4a_internal.h"
//#include "gba/flash_internal.h"

extern void (*const gIntrTable[])(void);

u16 INTR_CHECK;
void *INTR_VECTOR;
unsigned char REG_BASE[0x400] __attribute__ ((aligned (4)));
unsigned char PLTT[PLTT_SIZE] __attribute__ ((aligned (4)));
unsigned char VRAM_[VRAM_SIZE] __attribute__ ((aligned (4)));
unsigned char OAM[OAM_SIZE] __attribute__ ((aligned (4)));
unsigned char FLASH_BASE[131072] __attribute__ ((aligned (4)));

#define DMA_COUNT 4

struct DMATransfer {
    union {
        const void *src;
        const u16 *src16;
        const u32 *src32;
    };
    union {
        void *dst;
        vu16 *dst16;
        vu32 *dst32;
    };
    u32 size;
    u16 control;
} DMAList[DMA_COUNT];

enum {
    DMA_NOW,
    DMA_VBLANK,
    DMA_HBLANK,
    DMA_SPECIAL
};

SDL_Thread *mainLoopThread;
SDL_Window *sdlWindow;
SDL_Renderer *sdlRenderer;
SDL_Texture *sdlTexture;
SDL_sem *vBlankSemaphore;
SDL_atomic_t isFrameAvailable;
bool speedUp = false;
unsigned int videoScale = 1;
bool videoScaleChanged = false;
bool isRunning = true;
bool paused = false;
double simTime = 0;
double lastGameTime = 0;
double curGameTime = 0;
double fixedTimestep = 1.0 / 60.0; // 16.666667ms
double timeScale = 1.0;
struct SiiRtcInfo internalClock;

extern void AgbMain(void);
extern void DoSoftReset(void);

int DoMain(void *param);
void ProcessEvents(void);
void VDraw(SDL_Texture *texture);
static void UpdateInternalClock(void);
static void RunDMAs(u32 type);

int main(int argc, char **argv)
{
    // Open an output console on Windows
#ifdef _WIN32
    AllocConsole() ;
    AttachConsole( GetCurrentProcessId() ) ;
    freopen( "CON", "w", stdout ) ;
#endif

    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    sdlWindow = SDL_CreateWindow("pokeemerald", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, DISPLAY_WIDTH * videoScale, DISPLAY_HEIGHT * videoScale, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (sdlWindow == NULL)
    {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_PRESENTVSYNC);
    if (sdlRenderer == NULL)
    {
        fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 255, 255);
    SDL_RenderClear(sdlRenderer);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_RenderSetLogicalSize(sdlRenderer, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    sdlTexture = SDL_CreateTexture(sdlRenderer,
                                   SDL_PIXELFORMAT_ARGB8888,
                                   SDL_TEXTUREACCESS_STREAMING,
                                   240, 160);
    if (sdlTexture == NULL)
    {
        fprintf(stderr, "Texture could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    simTime = curGameTime = lastGameTime = SDL_GetPerformanceCounter();

    isFrameAvailable.value = 0;
    vBlankSemaphore = SDL_CreateSemaphore(0);

    VDraw(sdlTexture);
    mainLoopThread = SDL_CreateThread(DoMain, "AgbMain", NULL);

    double accumulator = 0.0;

    memset(&internalClock, 0, sizeof(internalClock));
    internalClock.status = SIIRTCINFO_24HOUR;
    UpdateInternalClock();

    while (isRunning)
    {
        ProcessEvents();

        if (!paused)
        {
            double dt = fixedTimestep / timeScale; // TODO: Fix speedup

            curGameTime = SDL_GetPerformanceCounter();
            double deltaTime = (double)((curGameTime - lastGameTime) / (double)SDL_GetPerformanceFrequency());
            if (deltaTime > (dt * 5))
                deltaTime = dt;
            lastGameTime = curGameTime;

            accumulator += deltaTime;

            while (accumulator >= dt)
            {
                if (SDL_AtomicGet(&isFrameAvailable))
                {
                    VDraw(sdlTexture);
                    SDL_RenderClear(sdlRenderer);
                    SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
                    SDL_AtomicSet(&isFrameAvailable, 0);

                    REG_DISPSTAT |= INTR_FLAG_VBLANK;

                    RunDMAs(DMA_HBLANK);

                    if (REG_DISPSTAT & DISPSTAT_VBLANK_INTR)
                        gIntrTable[4]();

                    REG_DISPSTAT &= ~INTR_FLAG_VBLANK;

                    SDL_SemPost(vBlankSemaphore);

                    accumulator -= dt;
                }
            }
        }

        if (videoScaleChanged)
        {
            SDL_SetWindowSize(sdlWindow, DISPLAY_WIDTH * videoScale, DISPLAY_HEIGHT * videoScale);
            videoScaleChanged = false;
        }

        SDL_RenderPresent(sdlRenderer);
    }

    SDL_DestroyWindow(sdlWindow);
    SDL_Quit();
    return 0;
}

// Key mappings
#define KEY_A_BUTTON      SDLK_x
#define KEY_B_BUTTON      SDLK_z
#define KEY_START_BUTTON  SDLK_RSHIFT
#define KEY_SELECT_BUTTON SDLK_BACKSLASH
#define KEY_L_BUTTON      SDLK_a
#define KEY_R_BUTTON      SDLK_s
#define KEY_DPAD_UP       SDLK_UP
#define KEY_DPAD_DOWN     SDLK_DOWN
#define KEY_DPAD_LEFT     SDLK_LEFT
#define KEY_DPAD_RIGHT    SDLK_RIGHT

#define HANDLE_KEYUP(key) \
case KEY_##key:  keys &= ~key; break;

#define HANDLE_KEYDOWN(key) \
case KEY_##key:  keys |= key; break;

static u16 keys;

void ProcessEvents(void)
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            isRunning = false;
            break;
        case SDL_KEYUP:
            switch (event.key.keysym.sym)
            {
            HANDLE_KEYUP(A_BUTTON)
            HANDLE_KEYUP(B_BUTTON)
            HANDLE_KEYUP(START_BUTTON)
            HANDLE_KEYUP(SELECT_BUTTON)
            HANDLE_KEYUP(L_BUTTON)
            HANDLE_KEYUP(R_BUTTON)
            HANDLE_KEYUP(DPAD_UP)
            HANDLE_KEYUP(DPAD_DOWN)
            HANDLE_KEYUP(DPAD_LEFT)
            HANDLE_KEYUP(DPAD_RIGHT)
            case SDLK_SPACE:
                if (speedUp)
                {
                    speedUp = false;
                    timeScale = 1.0;
                }
                break;
            }
            break;
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            HANDLE_KEYDOWN(A_BUTTON)
            HANDLE_KEYDOWN(B_BUTTON)
            HANDLE_KEYDOWN(START_BUTTON)
            HANDLE_KEYDOWN(SELECT_BUTTON)
            HANDLE_KEYDOWN(L_BUTTON)
            HANDLE_KEYDOWN(R_BUTTON)
            HANDLE_KEYDOWN(DPAD_UP)
            HANDLE_KEYDOWN(DPAD_DOWN)
            HANDLE_KEYDOWN(DPAD_LEFT)
            HANDLE_KEYDOWN(DPAD_RIGHT)
            case SDLK_r:
                if (event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL))
                {
                    DoSoftReset();
                }
                break;
            case SDLK_p:
                if (event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL))
                {
                    paused = !paused;
                }
                break;
            case SDLK_SPACE:
                if (!speedUp)
                {
                    speedUp = true;
                    timeScale = 5.0;
                }
                break;
            }
            break;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                unsigned int w = event.window.data1;
                unsigned int h = event.window.data2;
                
                videoScale = 0;
                if (w / DISPLAY_WIDTH > videoScale)
                    videoScale = w / DISPLAY_WIDTH;
                if (h / DISPLAY_HEIGHT > videoScale)
                    videoScale = h / DISPLAY_HEIGHT;
                if (videoScale < 1)
                    videoScale = 1;

                videoScaleChanged = true;
            }
            break;
        }
    }
}

u16 Platform_GetKeyInput(void)
{
    return keys;
}

// BIOS function implementations are based on the VBA-M source code.

static uint32_t CPUReadMemory(const void *src)
{
    return *(uint32_t *)src;
}

static void CPUWriteMemory(void *dest, uint32_t val)
{
    *(uint32_t *)dest = val;
}

static uint16_t CPUReadHalfWord(const void *src)
{
    return *(uint16_t *)src;
}

static void CPUWriteHalfWord(void *dest, uint16_t val)
{
    *(uint16_t *)dest = val;
}

static uint8_t CPUReadByte(const void *src)
{
    return *(uint8_t *)src;
}

static void CPUWriteByte(void *dest, uint8_t val)
{
    *(uint8_t *)dest = val;
}

static void RunDMAs(u32 type)
{
    for (int dmaNum = 0; dmaNum < DMA_COUNT; dmaNum++)
    {
        struct DMATransfer *dma = &DMAList[dmaNum];
        u32 dmaCntReg = (&REG_DMA0CNT)[dmaNum * 3];
        if (!((dmaCntReg >> 16) & DMA_ENABLE))
        {
            dma->control &= ~DMA_ENABLE;
        }
        
        if ( (dma->control & DMA_ENABLE) &&
           (((dma->control & DMA_START_MASK) >> 12) == type))
        {
            //printf("DMA%d src=%p, dest=%p, control=%d\n", dmaNum, dma->src, dma->dest, dma->control);
            for (int i = 0; i < (dma->size); i++)
            {
                if ((dma->control) & DMA_32BIT)
                     *dma->dst32 = *dma->src32;
                else *dma->dst16 = *dma->src16;

                // process destination pointer changes
                if (((dma->control) & DMA_DEST_MASK) == DMA_DEST_INC)
                {
                    if ((dma->control) & DMA_32BIT)
                            dma->dst32++;
                    else    dma->dst16++;
                }
                else if (((dma->control) & DMA_DEST_MASK) == DMA_DEST_DEC)
                {
                    if ((dma->control) & DMA_32BIT)
                            dma->dst32--;
                    else    dma->dst16--;
                }
                else if (((dma->control) & DMA_DEST_MASK) == DMA_DEST_RELOAD) // TODO
                {
                    if ((dma->control) & DMA_32BIT)
                            dma->dst32++;
                    else    dma->dst16++;
                }

                // process source pointer changes
                if (((dma->control) & DMA_SRC_MASK) == DMA_SRC_INC)
                {
                    if ((dma->control) & DMA_32BIT)
                            dma->src32++;
                    else    dma->src16++;
                }
                else if (((dma->control) & DMA_SRC_MASK) == DMA_SRC_DEC)
                {
                    if ((dma->control) & DMA_32BIT)
                            dma->src32--;
                    else    dma->src16--;
                }
            }

            if (dma->control & DMA_REPEAT)
            {
                dma->size = ((&REG_DMA0CNT)[dmaNum * 3] & 0x1FFFF);
                if (((dma->control) & DMA_DEST_MASK) == DMA_DEST_RELOAD)
                {
                    dma->dst = ((&REG_DMA0DAD)[dmaNum * 3]);
                }
            }
            else
            {
                dma->control &= ~DMA_ENABLE;
            }
        }
    }
}

void DmaSet(int dmaNum, const void *src, void *dest, u32 control)
{
    if (dmaNum >= DMA_COUNT)
    {
        fprintf(stderr, "DmaSet with invalid DMA number: dmaNum=%d, src=%p, dest=%p, control=%d\n", dmaNum, src, dest, control);
        return;
    }

    (&REG_DMA0SAD)[dmaNum * 3] = src;
    (&REG_DMA0DAD)[dmaNum * 3] = dest;
    (&REG_DMA0CNT)[dmaNum * 3] = control;

    struct DMATransfer *dma = &DMAList[dmaNum];
    dma->src = src;
    dma->dst = dest;
    dma->size = control & 0x1ffff;
    dma->control = control >> 16;

    RunDMAs(DMA_NOW);
}

void CpuSet(const void *src, void *dst, u32 cnt)
{
    if(dst == NULL)
    {
        puts("Attempted to CpuSet to NULL\n");
        return;
    }
    
    int count = cnt & 0x1FFFFF;

    const u8 *source = src;
    u8 *dest = dst;

    // 32-bit ?
    if ((cnt >> 26) & 1) {
        
        //assert(((uintptr_t)src & ~3) == (uintptr_t)src);
        //assert(((uintptr_t)dst & ~3) == (uintptr_t)dst);
        
        // needed for 32-bit mode!
        //source = (u8 *)((uint32_t )source & ~3);
        //dest = (u8 *)((uint32_t )dest & ~3);

        // fill ?
        if ((cnt >> 24) & 1) {
            uint32_t value = CPUReadMemory(source);
            while (count) {
                CPUWriteMemory(dest, value);
                dest += 4;
                count--;
            }
        } else {
            // copy
            while (count) {
                CPUWriteMemory(dest, CPUReadMemory(source));
                source += 4;
                dest += 4;
                count--;
            }
        }
    } else {
        // No align on 16-bit fill?
        //assert(((uintptr_t)src & ~1) == (uintptr_t)src);
        //assert(((uintptr_t)dst & ~1) == (uintptr_t)dst);

        // 16-bit fill?
        if ((cnt >> 24) & 1) {
            uint16_t value = CPUReadHalfWord(source);
            while (count) {
                CPUWriteHalfWord(dest, value);
                dest += 2;
                count--;
            }
        } else {
            // copy
            while (count) {
                CPUWriteHalfWord(dest, CPUReadHalfWord(source));
                source += 2;
                dest += 2;
                count--;
            }
        }
    }
}

void CpuFastSet(const void *src, void *dst, u32 cnt)
{
    if(dst == NULL)
    {
        puts("Attempted to CpuFastSet to NULL\n");
        return;
    }
    
    int count = cnt & 0x1FFFFF;

    const u8 *source = src;
    u8 *dest = dst;
    
    //source = (u8 *)((uint32_t )source & ~3);
    //dest = (u8 *)((uint32_t )dest & ~3);

    // fill?
    if((cnt >> 24) & 1) {
        uint32_t value = CPUReadMemory(source);
        while(count > 0) {
            // BIOS always transfers 32 bytes at a time
            for(int i = 0; i < 8; i++) {
                CPUWriteMemory(dest, value);
                dest += 4;
            }
            count -= 8;
        }
    } else {
        // copy
        while(count > 0) {
            // BIOS always transfers 32 bytes at a time
            for(int i = 0; i < 8; i++) {
                uint32_t value = CPUReadMemory(source);
                CPUWriteMemory(dest, value);
                source += 4;
                dest += 4;
            }
            count -= 8;
        }
    }
}

void LZ77UnCompVram(const u32 *src_, void *dest_)
{
    const u8 *src = src_;
    u8 *dest = dest_;
	int destSize = (src[3] << 16) | (src[2] << 8) | src[1];
	int srcPos = 4;
	int destPos = 0;

	for (;;) {
		unsigned char flags = src[srcPos++];

		for (int i = 0; i < 8; i++) {
			if (flags & 0x80) {
				int blockSize = (src[srcPos] >> 4) + 3;
				int blockDistance = (((src[srcPos] & 0xF) << 8) | src[srcPos + 1]) + 1;

				srcPos += 2;

				int blockPos = destPos - blockDistance;

				// Some Ruby/Sapphire tilesets overflow.
				if (destPos + blockSize > destSize) {
					blockSize = destSize - destPos;
					//fprintf(stderr, "Destination buffer overflow.\n");
					puts("Destination buffer overflow.\n");
				}

				if (blockPos < 0)
					goto fail;

				for (int j = 0; j < blockSize; j++)
					dest[destPos++] = dest[blockPos + j];
			} else {
				if (destPos >= destSize)
					goto fail;

				dest[destPos++] = src[srcPos++];
			}

			if (destPos == destSize) {
				return;
			}

			flags <<= 1;
		}
	}

fail:
	puts("Fatal error while decompressing LZ file.\n");
}

/*
void LZ77UnCompVram(const void *src, void *dst)
{
    const uint8_t *source = src;
    uint8_t *dest = dst;

    uint32_t header = CPUReadMemory(source);
    source += 4;

    int byteCount = 0;
    int byteShift = 0;
    uint32_t writeValue = 0;

    int len = header >> 8;

    while (len > 0) {
        uint8_t d = CPUReadByte(source++);

        if (d) {
            for (int i = 0; i < 8; i++) {
                if (d & 0x80) {
                    uint16_t data = CPUReadByte(source++) << 8;
                    data |= CPUReadByte(source++);
                    int length = (data >> 12) + 3;
                    int offset = (data & 0x0FFF);
                    uint8_t *windowOffset = dest + byteCount - offset - 1;
                    for (int i2 = 0; i2 < length; i2++) {
                        writeValue |= (CPUReadByte(windowOffset++) << byteShift);
                        byteShift += 8;
                        byteCount++;

                        if (byteCount == 2) {
                            CPUWriteHalfWord(dest, writeValue);
                            dest += 2;
                            byteCount = 0;
                            byteShift = 0;
                            writeValue = 0;
                        }
                        len--;
                        if (len == 0)
                            return;
                    }
                } else {
                    writeValue |= (CPUReadByte(source++) << byteShift);
                    byteShift += 8;
                    byteCount++;
                    if (byteCount == 2) {
                        CPUWriteHalfWord(dest, writeValue);
                        dest += 2;
                        byteCount = 0;
                        byteShift = 0;
                        writeValue = 0;
                    }
                    len--;
                    if (len == 0)
                        return;
                }
                d <<= 1;
            }
        } else {
            for (int i = 0; i < 8; i++) {
                writeValue |= (CPUReadByte(source++) << byteShift);
                byteShift += 8;
                byteCount++;
                if (byteCount == 2) {
                    CPUWriteHalfWord(dest, writeValue);
                    dest += 2;
                    byteShift = 0;
                    byteCount = 0;
                    writeValue = 0;
                }
                len--;
                if (len == 0)
                    return;
            }
        }
    }
}
*/

void LZ77UnCompWram(const u32 *src, void *dst)
{
    const uint8_t *source = src;
    uint8_t *dest = dst;

    uint32_t header = CPUReadMemory(source);
    source += 4;

    int len = header >> 8;

    while (len > 0) {
        uint8_t d = CPUReadByte(source++);

        if (d) {
            for (int i = 0; i < 8; i++) {
                if (d & 0x80) {
                    uint16_t data = CPUReadByte(source++) << 8;
                    data |= CPUReadByte(source++);
                    int length = (data >> 12) + 3;
                    int offset = (data & 0x0FFF);
                    uint8_t *windowOffset = dest - offset - 1;
                    for (int i2 = 0; i2 < length; i2++) {
                        CPUWriteByte(dest++, CPUReadByte(windowOffset++));
                        len--;
                        if (len == 0)
                            return;
                    }
                } else {
                    CPUWriteByte(dest++, CPUReadByte(source++));
                    len--;
                    if (len == 0)
                        return;
                }
                d <<= 1;
            }
        } else {
            for (int i = 0; i < 8; i++) {
                CPUWriteByte(dest++, CPUReadByte(source++));
                len--;
                if (len == 0)
                    return;
            }
        }
    }
}


const s16 sineTable[256] = {
  (s16)0x0000, (s16)0x0192, (s16)0x0323, (s16)0x04B5, (s16)0x0645, (s16)0x07D5, (s16)0x0964, (s16)0x0AF1,
  (s16)0x0C7C, (s16)0x0E05, (s16)0x0F8C, (s16)0x1111, (s16)0x1294, (s16)0x1413, (s16)0x158F, (s16)0x1708,
  (s16)0x187D, (s16)0x19EF, (s16)0x1B5D, (s16)0x1CC6, (s16)0x1E2B, (s16)0x1F8B, (s16)0x20E7, (s16)0x223D,
  (s16)0x238E, (s16)0x24DA, (s16)0x261F, (s16)0x275F, (s16)0x2899, (s16)0x29CD, (s16)0x2AFA, (s16)0x2C21,
  (s16)0x2D41, (s16)0x2E5A, (s16)0x2F6B, (s16)0x3076, (s16)0x3179, (s16)0x3274, (s16)0x3367, (s16)0x3453,
  (s16)0x3536, (s16)0x3612, (s16)0x36E5, (s16)0x37AF, (s16)0x3871, (s16)0x392A, (s16)0x39DA, (s16)0x3A82,
  (s16)0x3B20, (s16)0x3BB6, (s16)0x3C42, (s16)0x3CC5, (s16)0x3D3E, (s16)0x3DAE, (s16)0x3E14, (s16)0x3E71,
  (s16)0x3EC5, (s16)0x3F0E, (s16)0x3F4E, (s16)0x3F84, (s16)0x3FB1, (s16)0x3FD3, (s16)0x3FEC, (s16)0x3FFB,
  (s16)0x4000, (s16)0x3FFB, (s16)0x3FEC, (s16)0x3FD3, (s16)0x3FB1, (s16)0x3F84, (s16)0x3F4E, (s16)0x3F0E,
  (s16)0x3EC5, (s16)0x3E71, (s16)0x3E14, (s16)0x3DAE, (s16)0x3D3E, (s16)0x3CC5, (s16)0x3C42, (s16)0x3BB6,
  (s16)0x3B20, (s16)0x3A82, (s16)0x39DA, (s16)0x392A, (s16)0x3871, (s16)0x37AF, (s16)0x36E5, (s16)0x3612,
  (s16)0x3536, (s16)0x3453, (s16)0x3367, (s16)0x3274, (s16)0x3179, (s16)0x3076, (s16)0x2F6B, (s16)0x2E5A,
  (s16)0x2D41, (s16)0x2C21, (s16)0x2AFA, (s16)0x29CD, (s16)0x2899, (s16)0x275F, (s16)0x261F, (s16)0x24DA,
  (s16)0x238E, (s16)0x223D, (s16)0x20E7, (s16)0x1F8B, (s16)0x1E2B, (s16)0x1CC6, (s16)0x1B5D, (s16)0x19EF,
  (s16)0x187D, (s16)0x1708, (s16)0x158F, (s16)0x1413, (s16)0x1294, (s16)0x1111, (s16)0x0F8C, (s16)0x0E05,
  (s16)0x0C7C, (s16)0x0AF1, (s16)0x0964, (s16)0x07D5, (s16)0x0645, (s16)0x04B5, (s16)0x0323, (s16)0x0192,
  (s16)0x0000, (s16)0xFE6E, (s16)0xFCDD, (s16)0xFB4B, (s16)0xF9BB, (s16)0xF82B, (s16)0xF69C, (s16)0xF50F,
  (s16)0xF384, (s16)0xF1FB, (s16)0xF074, (s16)0xEEEF, (s16)0xED6C, (s16)0xEBED, (s16)0xEA71, (s16)0xE8F8,
  (s16)0xE783, (s16)0xE611, (s16)0xE4A3, (s16)0xE33A, (s16)0xE1D5, (s16)0xE075, (s16)0xDF19, (s16)0xDDC3,
  (s16)0xDC72, (s16)0xDB26, (s16)0xD9E1, (s16)0xD8A1, (s16)0xD767, (s16)0xD633, (s16)0xD506, (s16)0xD3DF,
  (s16)0xD2BF, (s16)0xD1A6, (s16)0xD095, (s16)0xCF8A, (s16)0xCE87, (s16)0xCD8C, (s16)0xCC99, (s16)0xCBAD,
  (s16)0xCACA, (s16)0xC9EE, (s16)0xC91B, (s16)0xC851, (s16)0xC78F, (s16)0xC6D6, (s16)0xC626, (s16)0xC57E,
  (s16)0xC4E0, (s16)0xC44A, (s16)0xC3BE, (s16)0xC33B, (s16)0xC2C2, (s16)0xC252, (s16)0xC1EC, (s16)0xC18F,
  (s16)0xC13B, (s16)0xC0F2, (s16)0xC0B2, (s16)0xC07C, (s16)0xC04F, (s16)0xC02D, (s16)0xC014, (s16)0xC005,
  (s16)0xC000, (s16)0xC005, (s16)0xC014, (s16)0xC02D, (s16)0xC04F, (s16)0xC07C, (s16)0xC0B2, (s16)0xC0F2,
  (s16)0xC13B, (s16)0xC18F, (s16)0xC1EC, (s16)0xC252, (s16)0xC2C2, (s16)0xC33B, (s16)0xC3BE, (s16)0xC44A,
  (s16)0xC4E0, (s16)0xC57E, (s16)0xC626, (s16)0xC6D6, (s16)0xC78F, (s16)0xC851, (s16)0xC91B, (s16)0xC9EE,
  (s16)0xCACA, (s16)0xCBAD, (s16)0xCC99, (s16)0xCD8C, (s16)0xCE87, (s16)0xCF8A, (s16)0xD095, (s16)0xD1A6,
  (s16)0xD2BF, (s16)0xD3DF, (s16)0xD506, (s16)0xD633, (s16)0xD767, (s16)0xD8A1, (s16)0xD9E1, (s16)0xDB26,
  (s16)0xDC72, (s16)0xDDC3, (s16)0xDF19, (s16)0xE075, (s16)0xE1D5, (s16)0xE33A, (s16)0xE4A3, (s16)0xE611,
  (s16)0xE783, (s16)0xE8F8, (s16)0xEA71, (s16)0xEBED, (s16)0xED6C, (s16)0xEEEF, (s16)0xF074, (s16)0xF1FB,
  (s16)0xF384, (s16)0xF50F, (s16)0xF69C, (s16)0xF82B, (s16)0xF9BB, (s16)0xFB4B, (s16)0xFCDD, (s16)0xFE6E
};

void BgAffineSet(struct BgAffineSrcData *src, struct BgAffineDstData *dest, s32 count)
{
    for(s32 i=0; i<count; i++)
    {
        s32 cx = src[i].texX;
        s32 cy = src[i].texY;
        s16 dispx = src[i].scrX;
        s16 dispy = src[i].scrY;
        s16 rx = src[i].sx;
        s16 ry = src[i].sy;
        u16 theta = src[i].alpha>>8;
        s32 a = sineTable[(theta+0x40)&255];
        s32 b = sineTable[theta];

        s16 dx =  (rx * a)>>14;
        s16 dmx = (rx * b)>>14;
        s16 dy =  (ry * b)>>14;
        s16 dmy = (ry * a)>>14;

        dest[i].pa = dx;
        dest[i].pb = -dmx;
        dest[i].pc = dy;
        dest[i].pd = dmy;

        s32 startx = cx - dx * dispx + dmx * dispy;
        s32 starty = cy - dy * dispx - dmy * dispy;

        dest[i].dx = startx;
        dest[i].dy = starty;
    }
}

void ObjAffineSet(struct ObjAffineSrcData *src, void *dest, s32 count, s32 offset)
{
    for(s32 i=0; i<count; i++)
    {
        s16 rx = src[i].xScale;
        s16 ry = src[i].yScale;
        u16 theta = src[i].rotation>>8;

        s32 a = (s32)sineTable[(theta + 64) & 255];
        s32 b = (s32)sineTable[theta];

        s16 dx =  ((s32)rx * a)>>14;
        s16 dmx = ((s32)rx * b)>>14;
        s16 dy =  ((s32)ry * b)>>14;
        s16 dmy = ((s32)ry * a)>>14;

        CPUWriteHalfWord(dest, dx);
        dest += offset;
        CPUWriteHalfWord(dest, -dmx);
        dest += offset;
        CPUWriteHalfWord(dest, dy);
        dest += offset;
        CPUWriteHalfWord(dest, dmy);
        dest += offset;
    }
}

void SoftReset(u32 resetFlags)
{
    puts("Soft Reset called. Exiting.");
    exit(0);
}

/*
static inline uint16_t SwapPixel(uint16_t pixel)
{
    uint16_t result = 0;

    result |= (pixel >> 10) & 0x1F;
    result |= pixel & (0x1F << 5);
    result |= (pixel & 0x1F) << 10;
}
*/

static inline uint32_t ConvertPixel(uint16_t pixel)
{
    unsigned int r = (((pixel >> 0) & 0x1F) << 3) ;
    unsigned int g = (((pixel >> 5) & 0x1F) << 3) ;
    unsigned int b = (((pixel >> 10) & 0x1F) << 3);

    return b | (g << 8) | (r << 16) | (0xFF << 24);
}

static const uint16_t bgMapSizes[][2] =
{
    {32, 32},
    {64, 32},
    {32, 64},
    {64, 64},
};

static void RenderBGScanline(int bgNum, uint16_t control, uint16_t hoffs, uint16_t voffs, int lineNum, uint32_t *line)
{
    unsigned int charBaseBlock = (control >> 2) & 3;
    unsigned int screenBaseBlock = (control >> 8) & 0x1F;
    //bool is8bit = (control >> 7) & 1;
    unsigned int mapWidth = bgMapSizes[control >> 14][0];
    unsigned int mapHeight = bgMapSizes[control >> 14][1];
    unsigned int mapWidthInPixels = mapWidth * 8;
    unsigned int mapHeightInPixels = mapHeight * 8;

    uint8_t *bgtiles = (uint8_t *)(VRAM_ + charBaseBlock * 0x4000);
    uint16_t *bgmap = (uint16_t *)(VRAM_ + screenBaseBlock * 0x800);
    uint16_t *pal = (uint16_t *)PLTT;

    hoffs &= 0x1FF;
    voffs &= 0x1FF;

    for (unsigned int x = 0; x < DISPLAY_WIDTH; x++)
    {
        // adjust for scroll
        unsigned int xx = (x + hoffs) & 0x1FF;
        unsigned int yy = (lineNum + voffs) & 0x1FF;

        if (xx > mapWidthInPixels || yy > mapHeightInPixels)
        {
            //if (!(control & (1 << 13)))
            //    continue;
        }
            
        xx %= mapWidthInPixels;
        yy %= mapHeightInPixels;

        unsigned int mapX = xx / 8;
        unsigned int mapY = yy / 8;
        uint16_t entry = bgmap[mapY * 32 + mapX];

        unsigned int tileNum = entry & 0x3FF;
        unsigned int paletteNum = (entry >> 12) & 0xF;
        
        unsigned int tileX = xx % 8;
        unsigned int tileY = yy % 8;

        // Flip if necessary
        if (entry & (1 << 10))
            tileX = 7 - tileX;
        if (entry & (1 << 11))
            tileY = 7 - tileY;

        uint8_t pixel = bgtiles[(tileNum * 32) + (tileY * 4) + (tileX / 2)];
        if (tileX & 1)
            pixel >>= 4;
        else
            pixel &= 0xF;

        if (pixel != 0 /*&& !(line[x] & (0xFF << 24))*/)
            line[x] = ConvertPixel(pal[16 * paletteNum + pixel]);
    }
}

static inline uint32_t getBgX(int bgNumber)
{
    if (bgNumber == 2)
    {
        return REG_BG2X;
    }
    else if (bgNumber == 3)
    {
        return REG_BG3X;
    }
}

static inline uint32_t getBgY(int bgNumber)
{
    if (bgNumber == 2)
    {
        return REG_BG2Y;
    }
    else if (bgNumber == 3)
    {
        return REG_BG3Y;
    }
}

static inline uint16_t getBgPA(int bgNumber)
{
    if (bgNumber == 2)
    {
        return REG_BG2PA;
    }
    else if (bgNumber == 3)
    {
        return REG_BG3PA;
    }
}

static inline uint16_t getBgPB(int bgNumber)
{
    if (bgNumber == 2)
    {
        return REG_BG2PB;
    }
    else if (bgNumber == 3)
    {
        return REG_BG3PB;
    }
}

static inline uint16_t getBgPC(int bgNumber)
{
    if (bgNumber == 2)
    {
        return REG_BG2PC;
    }
    else if (bgNumber == 3)
    {
        return REG_BG3PC;
    }
}

static inline uint16_t getBgPD(int bgNumber)
{
    if (bgNumber == 2)
    {
        return REG_BG2PD;
    }
    else if (bgNumber == 3)
    {
        return REG_BG3PD;
    }
}

static void RenderRotScaleBGScanline(int bgNum, uint16_t control, uint16_t x, uint16_t y, int lineNum, uint32_t *line)
{
    vBgCnt *bgcnt = (vBgCnt *)&control;
    unsigned int charBaseBlock = bgcnt->charBaseBlock;
    unsigned int screenBaseBlock = bgcnt->screenBaseBlock;
    unsigned int mapWidth = 1 << (4 + (bgcnt->screenSize)); // number of tiles

    uint8_t *bgtiles = (uint8_t *)(VRAM_ + charBaseBlock * 0x4000);
    uint8_t *bgmap = (uint8_t *)(VRAM_ + screenBaseBlock * 0x800);
    uint16_t *pal = (uint16_t *)PLTT;
    int prio = ((bgcnt->priority) << 25) + 0x1000000;

    s16 pa = getBgPA(bgNum);
    s16 pb = getBgPB(bgNum);
    s16 pc = getBgPC(bgNum);
    s16 pd = getBgPD(bgNum);

    int sizeX = 128;
    int sizeY = 128;

    switch (bgcnt->screenSize)
    {
    case 0:
        break;
    case 1:
        sizeX = sizeY = 256;
        break;
    case 2:
        sizeX = sizeY = 512;
        break;
    case 3:
        sizeX = sizeY = 1024;
        break;
    }

    int maskX = sizeX - 1;
    int maskY = sizeY - 1;

    int yshift = ((control >> 14) & 3) + 4;

    /*int dx = pa & 0x7FFF;
    if (pa & 0x8000)
        dx |= 0xFFFF8000;
    int dmx = pb & 0x7FFF;
    if (pb & 0x8000)
        dmx |= 0xFFFF8000;
    int dy = pc & 0x7FFF;
    if (pc & 0x8000)
        dy |= 0xFFFF8000;
    int dmy = pd & 0x7FFF;
    if (pd & 0x8000)
        dmy |= 0xFFFF8000;*/

    int currentX = getBgX(bgNum);
    int currentY = getBgY(bgNum);

    currentX += lineNum * pb;
    currentY += lineNum * pd;

    int realX = currentX;
    int realY = currentY;

    if (bgcnt->areaOverflowMode)
    {
        for (int x = 0; x < 240; x++)
        {
            int xxx = (realX >> 8) & maskX;
            int yyy = (realY >> 8) & maskY;

            int tile = bgmap[(xxx >> 3) + ((yyy >> 3) << yshift)];

            int tileX = xxx & 7;
            int tileY = yyy & 7;

            uint8_t pixel = bgtiles[(tile << 6) + (tileY << 3) + tileX];

            if (pixel != 0)
                line[x] = ConvertPixel(pal[pixel]) | prio;

            realX += pa;
            realY += pc;
        }
    }
    else
    {
        for (int x = 0; x < 240; x++)
        {
            int xxx = (realX >> 8);
            int yyy = (realY >> 8);

            if (xxx < 0 || yyy < 0 || xxx >= sizeX || yyy >= sizeY)
            {
                //line[x] = 0x80000000;
            }
            else
            {
                int tile = bgmap[(xxx >> 3) + ((yyy >> 3) << yshift)];

                int tileX = xxx & 7;
                int tileY = yyy & 7;

                uint8_t pixel = bgtiles[(tile << 6) + (tileY << 3) + tileX];

                if (pixel != 0)
                    line[x] = ConvertPixel(pal[pixel]) | prio;
            }
            realX += pa;
            realY += pc;
        }
    }
}

const u8 spriteSizes[][2] =
{
    {8, 16},
    {8, 32},
    {16, 32},
    {32, 64},
};

// Parts of this code heavily borrowed from NanoboyAdvance.
static void DrawSprites(uint32_t layers[4][DISPLAY_WIDTH], uint16_t vcount)
{
    int i;
    unsigned int x;
    unsigned int y;
    void *objtiles = VRAM_ + 0x10000;
    unsigned int blendMode = (REG_BLDCNT >> 6) & 3;

    int16_t matrix[2][2] = {};

    if (!(REG_DISPCNT & (1 << 6)))
    {
        puts("2-D OBJ Character mapping not supported.");
    }

    for (i = 127; i >= 0; i--)
    {
        struct OamData *oam = &((struct OamData *)OAM)[i];
        unsigned int width;
        unsigned int height;
        uint32_t *pixels;

        bool isAffine  = oam->affineMode & 1;
        bool doubleSizeOrDisabled = (oam->affineMode >> 1) & 1;

        if (!(isAffine) && doubleSizeOrDisabled) // disable for non-affine
        {
            continue;
        }
        //if (oam->objMode != 0)
        //    continue;  // Don't know how to handle other modes yet

        if (oam->shape == 0)
        {
            width = (1 << oam->size) * 8;
            height = (1 << oam->size) * 8;
        }
        else if (oam->shape == 1) // wide
        {
            width = spriteSizes[oam->size][1];
            height = spriteSizes[oam->size][0];
        }
        else if (oam->shape == 2) // tall
        {
            width = spriteSizes[oam->size][0];
            height = spriteSizes[oam->size][1];
        }
        else
        {
            continue; // prohibited, do not draw
        }

        int rect_width = width;
        int rect_height = height;

        int half_width = width / 2;
        int half_height = height / 2;

        pixels = layers[oam->priority];

        int32_t x = oam->x;
        int32_t y = oam->y;

        if (x >= 240)
            x -= 512;
        if (y >= 160)
            y -= 256;

        if (isAffine)
        {
            //TODO: there is probably a better way to do this
            u8 matrixNum = oam->matrixNum * 4;

            struct OamData *oam1 = &((struct OamData *)OAM)[matrixNum];
            struct OamData *oam2 = &((struct OamData *)OAM)[matrixNum + 1];
            struct OamData *oam3 = &((struct OamData *)OAM)[matrixNum + 2];
            struct OamData *oam4 = &((struct OamData *)OAM)[matrixNum + 3];

            matrix[0][0] = oam1->affineParam;
            matrix[0][1] = oam2->affineParam;
            matrix[1][0] = oam3->affineParam;
            matrix[1][1] = oam4->affineParam;

            if (doubleSizeOrDisabled) // double size for affine
            {
                rect_width *= 2;
                rect_height *= 2;
                half_width *= 2;
                half_height *= 2;
            }
        }
        else
        {
            // Identity
            matrix[0][0] = 0x100;
            matrix[0][1] = 0;
            matrix[1][0] = 0;
            matrix[1][1] = 0x100;
        }

        x += half_width;
        y += half_height;

        // Does this sprite actually draw on this scanline?
        if (vcount >= (y - half_height) && vcount < (y + half_height))
        {
            int local_y = vcount - y;
            int number  = oam->tileNum;
            int palette = oam->paletteNum;
            bool flipX  = !isAffine && ((oam->matrixNum >> 3) & 1);
            bool flipY  = !isAffine && ((oam->matrixNum >> 4) & 1);
            bool is8BPP  = oam->bpp & 1;

            for (int local_x = -half_width; local_x <= half_width; local_x++)
            {
                uint8_t *tiledata = (uint8_t *)objtiles;
                uint16_t *palette = (uint16_t *)(PLTT + 0x200);

                unsigned int global_x = local_x + x;

                if (global_x < 0 || global_x >= 240)
                    continue;

                int tex_x = ((matrix[0][0] * local_x + matrix[0][1] * local_y) >> 8) + (width / 2);
                int tex_y = ((matrix[1][0] * local_x + matrix[1][1] * local_y) >> 8) + (height / 2);

                /* Check if transformed coordinates are inside bounds. */

                if (tex_x >= width || tex_y >= height || tex_x < 0 || tex_y < 0)
                    continue;

                if (flipX)
                    tex_x = width  - tex_x - 1;
                if (flipY)
                    tex_y = height - tex_y - 1;

                int tile_x = tex_x % 8;
                int tile_y = tex_y % 8;
                int block_x = tex_x / 8;
                int block_y = tex_y / 8;

                int block_offset = ((block_y * (REG_DISPCNT & 0x40 ? (width / 8) : 16)) + block_x);
                uint16_t pixel = 0;

                if (!is8BPP)
                {
                    pixel = tiledata[(block_offset + oam->tileNum) * 32 + (tile_y * 4) + (tile_x / 2)];
                    if (tile_x & 1)
                        pixel >>= 4;
                    else
                        pixel &= 0xF;
                    palette += oam->paletteNum * 16;
                }
                else
                {
                    pixel = tiledata[(block_offset * 2 + oam->tileNum) * 32 + (tile_y * 8) + tile_x];
                }

                if (pixel != 0)
                {
                    uint32_t color;
                    // u8 disHeightBot = REG_WIN0V ? REG_WIN0V : DISPLAY_HEIGHT;
                    // u8 disWidthBot = REG_WIN0H ? REG_WIN0H : DISPLAY_WIDTH;
                    // u8 disHeightTop = REG_WIN0V ? REG_WIN0V >> 8 : 0;
                    // u8 disWidthTop = REG_WIN0H ? REG_WIN0H >> 8 : 0;

                    color = ConvertPixel(palette[pixel]);

                    if (global_x < DISPLAY_WIDTH && global_x >= 0)
                        pixels[global_x] = color;
                }
            }
        }
    }
}

static uint32_t *target1layer;

#if 0
static void ProcessBGBlending(uint32_t *layer, int bg)
{
    unsigned int effect = (REG_BLDCNT >> 6) & 3;
    unsigned int evy;
    unsigned int eva;
    unsigned int evb;
    int i;

    switch (effect)
    {
    case 0:  // none
        break;
    case 1:  // alpha blending
        if (REG_BLDCNT & (1 << bg))  // BG is in first target
        {
            eva = REG_BLDALPHA & 0x1F;
            for (i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++)
            {
                unsigned int r = (layer[i] >> 0) & 0xFF;
                unsigned int g = (layer[i] >> 8) & 0xFF;
                unsigned int b = (layer[i] >> 16) & 0xFF;
                unsigned int a = (layer[i] >> 24) & 0xFF;

                r = r * eva / 31;
                g = g * eva / 31;
                b = b * eva / 31;
                if (r > 255)
                    r = 255;
                if (g > 255)
                    g = 255;
                if (b > 255)
                    b = 255;
                layer[i] = r | (g << 8) | (b << 16) | (a << 24);
            }
            target1layer = layer;
        }
        if (REG_BLDCNT & (1 << (8 + bg)))  // BG is in second target
        {
            evb = (REG_BLDALPHA >> 8) & 0x1F;
            for (i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++)
            {
                unsigned int r = (layer[i] >> 0) & 0xFF;
                unsigned int g = (layer[i] >> 8) & 0xFF;
                unsigned int b = (layer[i] >> 16) & 0xFF;
                unsigned int a = (layer[i] >> 24) & 0xFF;

                r = r * evb / 31;
                g = g * evb / 31;
                b = b * evb / 31;
                if (r > 255)
                    r = 255;
                if (g > 255)
                    g = 255;
                if (b > 255)
                    b = 255;
                layer[i] = r | (g << 8) | (b << 16) | (a << 24);
            }
        }
        break;
    case 2:  // brightness increase
    #if 0
        if (REG_BLDCNT & (1 << bg))
        {
            evy = REG_BLDY & 0x1F;
            //evy <<= 3;  // we are working with 8-bit instead of 5-bit
            for (i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++)
            {
                unsigned int r = (layer[i] >> 0) & 0xFF;
                unsigned int g = (layer[i] >> 8) & 0xFF;
                unsigned int b = (layer[i] >> 16) & 0xFF;
                unsigned int a = (layer[i] >> 24) & 0xFF;

                /*
                r += ((31 << 3) - r) * evy;
                g += ((31 << 3) - g) * evy;
                b += ((31 << 3) - b) * evy;
                if (r > 255)
                    r = 255;
                if (g > 255)
                    g = 255;
                if (b > 255)
                    b = 255;
                */
                r >>= 3;
                g >>= 3;
                b >>= 3;
                r += evy;
                g += evy;
                b += evy;
                r <<= 3;
                g <<= 3;
                b <<= 3;
                if (r > 255)
                    r = 255;
                if (g > 255)
                    g = 255;
                if (b > 255)
                    b = 255;
                layer[i] = r | (g << 8) | (b << 16) | (a << 24);
            }
        }
    #endif
        break;
    // TODO: support the rest
    }
}
#endif

static void DrawScanline(uint32_t *pixels, uint16_t vcount)
{
    unsigned int mode = REG_DISPCNT & 3;
    unsigned int bgEnabled = (REG_DISPCNT >> 8) & 0xF;
    int i;
    int j;
    static uint32_t layers[4][DISPLAY_WIDTH];

    int bgnum;
    // I have no clue how blending is supposed to work.
    unsigned int blendMode = (REG_BLDCNT >> 6) & 3;
    int blendPriority1 = -5;
    int blendPriority2 = -5;

    memset(layers, 0, sizeof(layers));

    if (blendMode == 1)
    {
        for (bgnum = 0; bgnum <= 3; bgnum++)
        {
            if (bgEnabled & (1 << bgnum))
            {
                uint16_t bgcnt = *(uint16_t *)(REG_ADDR_BG0CNT + bgnum * 2);
                unsigned int priority = bgcnt & 3;

                if (REG_BLDCNT & (1 << bgnum))
                    blendPriority1 = priority;
                if (REG_BLDCNT & (1 << (8 + bgnum)))
                    blendPriority2 = priority;
            }
        }
    }

    switch (mode)
    {
    case 0:
        // All backgrounds are text mode
        for (bgnum = 3; bgnum >= 0; bgnum--)
        {
            if (bgEnabled & (1 << bgnum))
            {
                uint16_t bgcnt = *(uint16_t *)(REG_ADDR_BG0CNT + bgnum * 2);
                uint16_t bghoffs = *(uint16_t *)(REG_ADDR_BG0HOFS + bgnum * 4);
                uint16_t bgvoffs = *(uint16_t *)(REG_ADDR_BG0VOFS + bgnum * 4);
                unsigned int priority = bgcnt & 3;

                RenderBGScanline(bgnum, bgcnt, bghoffs, bgvoffs, vcount, layers[priority]);
                //ProcessBGBlending(layers[priority], bgnum);
            }
        }
        break;
    case 1:
        // BG2 is affine
        bgnum = 2;
        if (bgEnabled & (1 << bgnum))
        {
            uint16_t bgcnt = *(uint16_t *)(REG_ADDR_BG0CNT + bgnum * 2);
            unsigned int priority = bgcnt & 3;

            RenderRotScaleBGScanline(bgnum, bgcnt, REG_BG2X, REG_BG2Y, vcount, layers[priority]);
            //ProcessBGBlending(layers[priority], bgnum);
        }
        // BG0 and BG1 are text mode
        for (bgnum = 1; bgnum >= 0; bgnum--)
        {
            if (bgEnabled & (1 << bgnum))
            {
                uint16_t bgcnt = *(uint16_t *)(REG_ADDR_BG0CNT + bgnum * 2);
                uint16_t bghoffs = *(uint16_t *)(REG_ADDR_BG0HOFS + bgnum * 4);
                uint16_t bgvoffs = *(uint16_t *)(REG_ADDR_BG0VOFS + bgnum * 4);
                unsigned int priority = bgcnt & 3;

                RenderBGScanline(bgnum, bgcnt, bghoffs, bgvoffs, vcount, layers[priority]);
                //RenderTextBGLayer(bgcnt, bghoffs, bgvoffs, layers[priority]);
                //ProcessBGBlending(layers[priority], bgnum);
            }
        }
        break;
    default:
        printf("Video mode %u is unsupported.\n", mode);
        break;
    }

    if (REG_DISPCNT & DISPCNT_OBJ_ON)
        DrawSprites(layers, vcount);

    // Copy to screen
    
    for (i = 3; i >= 0; i--)
    {
        uint32_t *src = layers[i];
        uint32_t *dest = pixels;

        if (i == blendPriority2 && i - 1 == blendPriority1)
        {
            uint32_t *target1 = layers[i - 1];
            uint32_t *target2 = layers[i];
            unsigned int eva = REG_BLDALPHA & 0x1F;
            unsigned int evb = (REG_BLDALPHA >> 8) & 0x1F;

            for (j = 0; j < DISPLAY_WIDTH; j++)
            {
                if ((target1[j] & (0xFF << 24)) && (target2[j] & (0xFF << 24)))
                {
                    unsigned int r = ((target1[j] >>  0) & 0xFF) * eva / 16 + ((target2[j] >>  0) & 0xFF) * evb / 16;
                    unsigned int g = ((target1[j] >>  8) & 0xFF) * eva / 16 + ((target2[j] >>  8) & 0xFF) * evb / 16;
                    unsigned int b = ((target1[j] >> 16) & 0xFF) * eva / 16 + ((target2[j] >> 16) & 0xFF) * evb / 16;
                    unsigned int a = (target1[j] >> 24) & 0xFF;
                    
                    if (r > 255)
                        r = 255;
                    if (g > 255)
                        g = 255;
                    if (b > 255)
                        b = 255;
                    target2[j] = r | (g << 8) | (b << 16) | (a << 24);
                    //target2[j] = target1[j];
                }
            }
            i--;
        }

        for (j = 0; j < DISPLAY_WIDTH; j++)
        {
            if ((src[j] & (0xFF << 24)))
                dest[j] = src[j];
        }
    }
}

uint32_t *memsetu32(uint32_t *dst, uint32_t fill, size_t count)
{
    for (int i = 0; i < count; i++)
    {
        *dst++ = fill;
    }
}

static void DrawFrame(uint32_t *pixels)
{
    int i;
    int j;
    static uint32_t scanlines[DISPLAY_HEIGHT][DISPLAY_WIDTH];

    memsetu32(scanlines, ConvertPixel(*(uint16_t *)PLTT), DISPLAY_WIDTH * DISPLAY_HEIGHT);

    for (i = 0; i < DISPLAY_HEIGHT; i++)
    {
        REG_VCOUNT = i;
        DrawScanline(scanlines[i], i);

        REG_DISPSTAT |= INTR_FLAG_HBLANK;

        RunDMAs(DMA_HBLANK);

        if (REG_DISPSTAT & DISPSTAT_HBLANK_INTR)
            gIntrTable[3]();

        REG_DISPSTAT &= ~INTR_FLAG_HBLANK;
    }

    // Copy to screen
    for (i = 0; i < DISPLAY_HEIGHT; i++)
    {
        uint32_t *src = scanlines[i];
        for (j = 0; j < DISPLAY_WIDTH; j++)
        {
            pixels[i * DISPLAY_WIDTH + j] = src[j];
        }
    }
}

void VDraw(SDL_Texture *texture)
{
    static uint32_t image[DISPLAY_WIDTH * DISPLAY_HEIGHT];

    memset(image, 0, sizeof(image));
    DrawFrame(image);
    SDL_UpdateTexture(texture, NULL, image, DISPLAY_WIDTH * sizeof (Uint32));
    REG_VCOUNT = 161; // prep for being in VBlank period
}

int DoMain(void *data)
{
    AgbMain();
}

void VBlankIntrWait(void)
{
    SDL_AtomicSet(&isFrameAvailable, 1);
    SDL_SemWait(vBlankSemaphore);
}

u8 BinToBcd(u8 bin)
{
    int placeCounter = 1;
    u8 out = 0;
    do
    {
        out |= (bin % 10) * placeCounter;
        placeCounter *= 16;
    }
    while ((bin /= 10) > 0);

    return out;
}

void Platform_GetStatus(struct SiiRtcInfo *rtc)
{
    rtc->status = internalClock.status;
}

void Platform_SetStatus(struct SiiRtcInfo *rtc)
{
    internalClock.status = rtc->status;
}

static void UpdateInternalClock(void)
{
    time_t rawTime = time(NULL);
    struct tm *time = localtime(&rawTime);

    internalClock.year = BinToBcd(time->tm_year - 100);
    internalClock.month = BinToBcd(time->tm_mon) + 1;
    internalClock.day = BinToBcd(time->tm_mday);
    internalClock.dayOfWeek = BinToBcd(time->tm_wday);
    internalClock.hour = BinToBcd(time->tm_hour);
    internalClock.minute = BinToBcd(time->tm_min);
    internalClock.second = BinToBcd(time->tm_sec);
}

void Platform_GetDateTime(struct SiiRtcInfo *rtc)
{
    UpdateInternalClock();

    rtc->year = internalClock.year;
    rtc->month = internalClock.month;
    rtc->day = internalClock.day;
    rtc->dayOfWeek = internalClock.dayOfWeek;
    rtc->hour = internalClock.hour;
    rtc->minute = internalClock.minute;
    rtc->second = internalClock.second;
    printf("GetDateTime: %d-%02d-%02d %02d:%02d:%02d\n", ConvertBcdToBinary(rtc->year),
                                                         ConvertBcdToBinary(rtc->month),
                                                         ConvertBcdToBinary(rtc->day),
                                                         ConvertBcdToBinary(rtc->hour),
                                                         ConvertBcdToBinary(rtc->minute),
                                                         ConvertBcdToBinary(rtc->second));
}

void Platform_SetDateTime(struct SiiRtcInfo *rtc)
{
    internalClock.month = rtc->month;
    internalClock.day = rtc->day;
    internalClock.dayOfWeek = rtc->dayOfWeek;
    internalClock.hour = rtc->hour;
    internalClock.minute = rtc->minute;
    internalClock.second = rtc->second;
}

void Platform_GetTime(struct SiiRtcInfo *rtc)
{
    UpdateInternalClock();

    rtc->hour = internalClock.hour;
    rtc->minute = internalClock.minute;
    rtc->second = internalClock.second;
    printf("GetTime: %02d:%02d:%02d\n", ConvertBcdToBinary(rtc->hour),
                                        ConvertBcdToBinary(rtc->minute),
                                        ConvertBcdToBinary(rtc->second));
}

void Platform_SetTime(struct SiiRtcInfo *rtc)
{
    internalClock.hour = rtc->hour;
    internalClock.minute = rtc->minute;
    internalClock.second = rtc->second;
}

void Platform_SetAlarm(u8 *alarmData)
{
    // TODO
}
