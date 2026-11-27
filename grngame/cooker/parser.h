#pragma once
#include "grngame/math/types.h"

typedef struct AppInfo AppInfo;

typedef enum ArchiveKind
{
    ARCHIVE_KIND_NONE = -1,
    ARCHIVE_KIND_AUDIO = 0,
    ARCHIVE_KIND_IMAGE,
    ARCHIVE_KIND_DATA,   /* text + json */
    ARCHIVE_KIND_SCRIPT, /* scripts */
    ARCHIVE_KIND_COUNT
} ArchiveKind;

bool Cooker(AppInfo *app_info, int32 num_dirs, const char **dirs, const char *output_path);