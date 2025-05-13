#include "global.h"
#include "malloc.h"

static void *sHeapStart;
static u32 sHeapSize;

ALIGNED(4) EWRAM_DATA u8 gHeap[HEAP_SIZE] = {0};
static EWRAM_DATA u32 sUsedHeap = 0; // amount of heap space currently in use

#define MALLOC_SYSTEM_ID 0xA3A3

struct MemBlock {
    // Whether this block is currently allocated.
    bool16 flag;

    // Magic number used for error checking. Should equal MALLOC_SYSTEM_ID.
    u16 magic;

    // Size of the block (not including this header struct).
    u32 size;

    // Previous block pointer. Equals sHeapStart if this is the first block.
    struct MemBlock *prev;

    // Next block pointer. Equals sHeapStart if this is the last block.
    struct MemBlock *next;

    // Data in the memory block. (Arrays of length 0 are a GNU extension.)
    u8 data[0];
};

void PutMemBlockHeader(void *block, struct MemBlock *prev, struct MemBlock *next, u32 size)
{
    struct MemBlock *header = (struct MemBlock *)block;

    header->flag = FALSE;
    header->magic = MALLOC_SYSTEM_ID;
    header->size = size;
    header->prev = prev;
    header->next = next;
    
    sUsedHeap += sizeof(* header);
}

void PutFirstMemBlockHeader(void *block, u32 size)
{
    PutMemBlockHeader(block, (struct MemBlock *)block, (struct MemBlock *)block, size - sizeof(struct MemBlock));
}

void *AllocInternal(void *heapStart, u32 size)
{
    struct MemBlock *pos = (struct MemBlock *)heapStart;
    struct MemBlock *head = pos;
    struct MemBlock *splitBlock;
    u32 foundBlockSize;

    // Alignment
    if (size & 3)
        size = 4 * ((size / 4) + 1);

    for (;;)
    {
        // Loop through the blocks looking for unused block that's big enough.

        if (!pos->flag)
        {
            foundBlockSize = pos->size;

            if (foundBlockSize >= size)
            {
                if (foundBlockSize - size < 2 * sizeof(struct MemBlock))
                {
                    // The block isn't much bigger than the requested size,
                    // so just use it.
                    pos->flag = TRUE;
                    sUsedHeap += size;
                }
                else
                {
                    // The block is significantly bigger than the requested
                    // size, so split the rest into a separate block.
                    foundBlockSize -= sizeof(struct MemBlock);
                    foundBlockSize -= size;

                    splitBlock = (struct MemBlock *)(pos->data + size);

                    pos->flag = TRUE;
                    pos->size = size;

                    PutMemBlockHeader(splitBlock, pos, pos->next, foundBlockSize);
                    sUsedHeap += size;

                    pos->next = splitBlock;

                    if (splitBlock->next != head)
                        splitBlock->next->prev = splitBlock;
                }

                return pos->data;
            }
        }

        if (pos->next == head)
            return NULL;

        pos = pos->next;
    }
    MgbaPrintf(2, "\nalloc'd %u (0x%X) bytes on heap.\nheap space in use: %u (0x%X)\nheap space free: %u (0x%X)", size + sizeof(struct MemBlock), size + sizeof(struct MemBlock), sUsedHeap, sUsedHeap, HEAP_SIZE - sUsedHeap, HEAP_SIZE - sUsedHeap);
}

void FreeInternal(void *heapStart, void *pointer)
{
    if (pointer)
    {
        struct MemBlock *head = (struct MemBlock *)heapStart;
        struct MemBlock *block = (struct MemBlock *)((u8 *)pointer - sizeof(struct MemBlock));
        block->flag = FALSE;
        sUsedHeap -= block->size;

        // If the freed block isn't the last one, merge with the next block
        // if it's not in use.
        if (block->next != head)
        {
            if (!block->next->flag)
            {
                block->size += sizeof(struct MemBlock) + block->next->size;
                block->next->magic = 0;
                block->next = block->next->next;
                if (block->next != head)
                    block->next->prev = block;
            }
        }

        // If the freed block isn't the first one, merge with the previous block
        // if it's not in use.
        if (block != head)
        {
            if (!block->prev->flag)
            {
                block->prev->next = block->next;

                if (block->next != head)
                    block->next->prev = block->prev;

                block->magic = 0;
                block->prev->size += sizeof(struct MemBlock) + block->size;
            }
        }
        MgbaPrintf(2, "\nfreed %u (0x%X) bytes on heap.\nheap space in use: %u (0x%X)\nheap space free: %u (0x%X)", block->size, block->size, sUsedHeap, sUsedHeap, HEAP_SIZE - sUsedHeap, HEAP_SIZE - sUsedHeap);
    }
}

void *AllocZeroedInternal(void *heapStart, u32 size)
{
    void *mem = AllocInternal(heapStart, size);

    if (mem != NULL)
    {
        if (size & 3)
            size = 4 * ((size / 4) + 1);

        CpuFill32(0, mem, size);
    }

    return mem;
}

bool32 CheckMemBlockInternal(void *heapStart, void *pointer)
{
    struct MemBlock *head = (struct MemBlock *)heapStart;
    struct MemBlock *block = (struct MemBlock *)((u8 *)pointer - sizeof(struct MemBlock));

    if (block->magic != MALLOC_SYSTEM_ID)
        return FALSE;

    if (block->next->magic != MALLOC_SYSTEM_ID)
        return FALSE;

    if (block->next != head && block->next->prev != block)
        return FALSE;

    if (block->prev->magic != MALLOC_SYSTEM_ID)
        return FALSE;

    if (block->prev != head && block->prev->next != block)
        return FALSE;

    if (block->next != head && block->next != (struct MemBlock *)(block->data + block->size))
        return FALSE;

    return TRUE;
}

void InitHeap(void *heapStart, u32 heapSize)
{
    sHeapStart = heapStart;
    sHeapSize = heapSize;
    PutFirstMemBlockHeader(heapStart, heapSize);
    MgbaPrintf(2, "\ninitialized heap\n");
}

void *Alloc(u32 size)
{
    return AllocInternal(sHeapStart, size);
}

void *AllocZeroed(u32 size)
{
    return AllocZeroedInternal(sHeapStart, size);
}

void Free(void *pointer)
{
    FreeInternal(sHeapStart, pointer);
}

bool32 CheckMemBlock(void *pointer)
{
    return CheckMemBlockInternal(sHeapStart, pointer);
}

bool32 CheckHeap()
{
    struct MemBlock *pos = (struct MemBlock *)sHeapStart;

    do {
        if (!CheckMemBlockInternal(sHeapStart, pos->data))
            return FALSE;
        pos = pos->next;
    } while (pos != (struct MemBlock *)sHeapStart);

    return TRUE;
}
