#include "bump_allocator.h"
#include "grngame/math/types.h"
#include "grngame/platform/virtual_memory.h"

static uint32 RoundUpToPage(uint32 size);
static bool EnsureCommitted(BumpAllocator *allocator, uint32 needed);

BumpAllocator BumpAllocatorInit(uint32 virtual_size)
{
    void *base = VirtualMemoryReserve(virtual_size);
    return (BumpAllocator){
        .base = base,
        .size = virtual_size,
        .committed = 0,
        .offset = 0,
    };
}

void *BumpAllocatorPushAligned(BumpAllocator *allocator, uint32 size, uint32 align)
{
    uint32 aligned = (allocator->offset + (align - 1)) & ~(align - 1);
    if (!EnsureCommitted(allocator, aligned + size))
        return NULL;

    allocator->offset = aligned + size;
    return (char *)allocator->base + aligned;
}

void *BumpAllocatorPush(BumpAllocator *allocator, uint32 size)
{
    return BumpAllocatorPushAligned(allocator, size, 8);
}

void BumpAllocatorReset(BumpAllocator *allocator)
{
    allocator->offset = 0; // no need to decommit
}

void BumpAllocatorFree(BumpAllocator *allocator)
{
    VirtualMemoryRelease(allocator->base, allocator->size);
    allocator->base = NULL;
    allocator->committed = 0;
    allocator->offset = 0;
}

void *BumpAllocatorHead(BumpAllocator *allocator)
{
    return (char *)allocator->base + allocator->offset;
}

bool BumpAllocatorIncrement(BumpAllocator *allocator, uint32 size, uint32 align)
{
    uint32 aligned = (allocator->offset + (align - 1)) & ~(align - 1);
    if (!EnsureCommitted(allocator, aligned + size))
        return false;

    allocator->offset = aligned + size;
    return true;
}

static uint32 RoundUpToPage(uint32 size)
{
    return (size + VirtualMemoryPageSize() - 1) & ~(VirtualMemoryPageSize() - 1);
}

static bool EnsureCommitted(BumpAllocator *allocator, uint32 needed)
{
    if (needed <= allocator->committed)
        return true;
    if (needed > allocator->size)
        return false;

    uint32 to_commit = RoundUpToPage(needed - allocator->committed);
    void *ptr = (char *)allocator->base + allocator->committed;
    if (!VirtualMemoryCommit(ptr, to_commit))
        return false;

    allocator->committed += to_commit;
    return true;
}