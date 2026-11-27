#pragma once

#include "grngame/cooker/parser.h"
#include "grngame/math/types.h"
#include "khash.h"
#include <stdbool.h>
#include "grngame/utils/c_cpp.h"

BEGIN_DECLARATIONS

typedef struct CookerFile
{
    const char *name;
    unsigned char *data;
    uint32 size;
    ArchiveKind kind;
} CookerFile;

KHASH_MAP_INIT_STR(CookerFileHash, CookerFile *)

typedef struct
{
    khash_t(CookerFileHash) * hash;
    uint32 counts[ARCHIVE_KIND_COUNT];
    uint32 total_count;
} CookerFileManager;

CookerFileManager CookerFileManagerCreate(void);
void CookerFileManagerDestroy(CookerFileManager *manager);

bool CookerFileManagerLoadAssetsPak(void);

const CookerFile *CookerFileGet(const char *name);
bool CookerFileExists(const char *name);

uint32 CookerFileManagerGetCounts(ArchiveKind kind);
uint32 CookerFileManagerGetTotalCount(void);

END_DECLARATIONS