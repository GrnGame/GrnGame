#pragma once
#include "grngame/math/types.h"
#include <stdbool.h>
#include <stddef.h>

/// reserves a contiguous virtual address range without allocating RAM
void *VirtualMemoryReserve(uint32 size);

/// commits pages in [ptr, ptr+size[, backing them with real RAM
bool VirtualMemoryCommit(void *ptr, uint32 size);

/// decommits pages, freeing RAM but keeping the address range reserved
bool VirtualMemoryDecommit(void *ptr, uint32 size);

/// releases the entire reserved range, ptr must be the original reserve base
bool VirtualMemoryRelease(void *ptr, uint32 size);

/// Page size
uint32 VirtualMemoryPageSize();