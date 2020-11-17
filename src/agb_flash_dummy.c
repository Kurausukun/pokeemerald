#include "global.h"
#include "gba/gba.h"
#include "gba/flash_internal.h"
#include <stdio.h>

const u16 dummyMaxTime[] =
{
      10, 65469, TIMER_ENABLE | TIMER_INTR_ENABLE | TIMER_256CLK,
      10, 65469, TIMER_ENABLE | TIMER_INTR_ENABLE | TIMER_256CLK,
    2000, 65469, TIMER_ENABLE | TIMER_INTR_ENABLE | TIMER_256CLK,
    2000, 65469, TIMER_ENABLE | TIMER_INTR_ENABLE | TIMER_256CLK,
};

const struct FlashSetupInfo DUMMY_SAVE =
{
    ProgramFlashByte_DUMMY,
    ProgramFlashSector_DUMMY,
    EraseFlashChip_DUMMY,
    EraseFlashSector_DUMMY,
    WaitForFlashWrite_DUMMY,
    dummyMaxTime,
    {
        131072, // ROM size
        {
            4096, // sector size
              12, // bit shift to multiply by sector size (4096 == 1 << 12)
              32, // number of sectors
               0  // appears to be unused
        },
        { 3, 1 }, // wait state setup data
        { { 0xCC, 0xCC } } // ID
    }
};

u16 WaitForFlashWrite_DUMMY(u8 phase, u8 *addr, u8 lastData)
{
    puts("function WaitForFlashWrite_DUMMY is a stub");
    return 0;
}

u16 EraseFlashChip_DUMMY(void)
{
    puts("EraseFlashChip_DUMMY");
    FILE * savefile = fopen("pokeemerald.sav", "w+b");
    fclose(savefile);
    return 0;
}

u16 EraseFlashSector_DUMMY(u16 sectorNum)
{
    printf("EraseFlashSector_DUMMY(0x%04X)\n",sectorNum);
    FILE * savefile = fopen("pokeemerald.sav", "r+b");
    if (savefile == NULL)
    {
        savefile = fopen("pokeemerald.sav", "w+b");
    }
    if (fseek(savefile, (sectorNum << gFlash->sector.shift), SEEK_SET))
    {
        fclose(savefile);
        return 1;
    }
    u8 buf[0x1000] = {0xFF};
    if (fwrite(buf, 1, sizeof(buf), savefile) != sizeof(buf))
    {
        fclose(savefile);
        return 1;
    }
    fclose(savefile);
    return 0;
}

u16 ProgramFlashByte_DUMMY(u16 sectorNum, u32 offset, u8 data)
{
    printf("ProgramFlashByte_DUMMY(0x%04X,0x%08X,0x%02X)\n",sectorNum,offset,data);
    u8 val = data;
    FILE * savefile = fopen("pokeemerald.sav", "r+b");
    if (savefile == NULL)
    {
        savefile = fopen("pokeemerald.sav", "w+b");
    }
    if (fseek(savefile, (sectorNum << gFlash->sector.shift) + offset, SEEK_SET))
    {
        fclose(savefile);
        return 1;
    }
    if (fwrite(&val, 1, 1, savefile) != 1)
    {
        fclose(savefile);
        return 1;
    }
    fclose(savefile);
    return 0;
}


u16 ProgramFlashSector_DUMMY(u16 sectorNum, void *src)
{
    printf("ProgramFlashSector_DUMMY(0x%04X)\n",sectorNum);
    FILE * savefile = fopen("pokeemerald.sav", "r+b");
    if (savefile == NULL)
    {
        savefile = fopen("pokeemerald.sav", "w+b");
    }
    if (fseek(savefile, (sectorNum << gFlash->sector.shift), SEEK_SET))
    {
        fclose(savefile);
        return 1;
    }
    if (fwrite(src, 1, 0x1000, savefile) != 0x1000)
    {
        fclose(savefile);
        return 1;
    }
    fclose(savefile);
    return 0;
}
