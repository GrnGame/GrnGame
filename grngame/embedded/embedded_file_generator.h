#pragma once

#include "grngame/data/sqlite.h"
#include "grngame/math/types.h"

typedef enum EmbedKind
{
    EMBED_KIND_NONE = -1, // not saved
    EMBED_KIND_ASSET = 0, /* audio + images */
    EMBED_KIND_DATA,      /* text + json */
    EMBED_KIND_SCRIPT,    /* scripts */
    EMBED_KIND_COUNT
} EmbedKind;

static const char *const EMBED_TABLE_NAMES[EMBED_KIND_COUNT] = {
    "assets",
    "data",
    "scripts",
};

typedef struct EmbedContext
{
    DbStmt stmts[EMBED_KIND_COUNT];
} EmbedContext;

void CreateEmbeddedFileDb(int32 num_dirs, const char **dirs, const char *output_path, const char *version);
