#pragma once
#include "grngame/math/types.h"
#include "parser.h"
#include "cglm/types-struct.h"
#include <stdbool.h>

#define PATH_SIZE 2048

    typedef struct
{
    uint32 magic;
    uint32 version;
    uint32 assets_files; // is 1+ audio because of atlas but use uint32 for alignement
    uint32 data_files;
    uint32 script_files;
} ArchiveHeader;

typedef struct
{
    unsigned char *content;
    uint32 content_size;
    const char *name;
    ArchiveKind kind;
} ArchiveNode;

typedef struct
{
    vec2s size;
    vec2s positions;
    const char *name;
} ArchiveAssetData;

bool WriteArchive(const char *destination, const ArchiveHeader *header, ArchiveNode **nodes, uint32 size_nodes,
                  ArchiveAssetData **assets_data, uint32 size_assets);
