#include "virtual_memory.h"

#ifdef GRNGAME_WINDOWS
#include <Windows.h>

void *VirtualMemoryReserve(uint32 size)
{
    void *ptr = VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
    return ptr;
}

bool VirtualMemoryCommit(void *ptr, uint32 size)
{
    void *result = VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
    return result != NULL;
}

bool VirtualMemoryDecommit(void *ptr, uint32 size)
{
    return VirtualFree(ptr, size, MEM_DECOMMIT);
}

bool VirtualMemoryRelease(void *ptr, uint32 size)
{
    return VirtualFree(ptr, 0, MEM_RELEASE);
}

uint32 VirtualMemoryPageSize()
{
    static uint32 page_size = 0;
    if (page_size == 0)
    {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        page_size = si.dwPageSize;
    }
    return page_size;
}

#elif defined(GRNGAME_LINUX) || defined(GRNGAME_MACOS)
#include <sys/mman.h>
#include <unistd.h>

void *VirtualMemoryReserve(uint32 size)
{
    void *ptr = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return ptr;
}

bool VirtualMemoryCommit(void *ptr, uint32 size)
{
    return mprotect(ptr, size, PROT_READ | PROT_WRITE) == 0;
}

bool VirtualMemoryDecommit(void *ptr, uint32 size)
{
    mprotect(ptr, size, PROT_NONE);
    madvise(ptr, size, MADV_FREE);
    return true;
}

bool VirtualMemoryRelease(void *ptr, uint32 size)
{
    return munmap(ptr, size) == 0;
}

uint32 VirtualMemoryPageSize(void)
{
    static uint32 page_size = 0;
    if (page_size == 0)
        page_size = sysconf(_SC_PAGESIZE);
    return page_size;
}

#endif
