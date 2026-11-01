#pragma once
#include "grngame/math/types.h"
#include <grngame/utils/string_compat.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct
{
    void *base;
    uint32 size;      // total reserved size
    uint32 committed; // how much is currently committed
    uint32 offset;    // how much is currently used
} BumpAllocator;

/// Creates a bump allocator with the given virtual size reserved
/// (you can ask it for 1TB on a 8GB of RAM machine)
BumpAllocator BumpAllocatorInit(uint32 virtual_size);

/// Allocates size bytes, aligned to align
void *BumpAllocatorPushAligned(BumpAllocator *allocator, uint32 size, uint32 align);

/// Allocates size bytes
void *BumpAllocatorPush(BumpAllocator *allocator, uint32 size);

/// Makes the entire arena reusable without freeing the underlying memory
void BumpAllocatorReset(BumpAllocator *allocator);

/// Releases the virtual memory
void BumpAllocatorFree(BumpAllocator *allocator);

/// Gets the head of the bump allocator (base + offset)
void *BumpAllocatorHead(BumpAllocator *allocator);

/// Manually increments offset, returning false if we overran
bool BumpAllocatorIncrement(BumpAllocator *allocator, uint32 size, uint32 align);