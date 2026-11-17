#include "embedded_file_generator.h"
#include "SDL3/SDL_stdinc.h"
#include "grngame/data/file.h"
#include "grngame/data/sqlite.h"
#include "grngame/platform/directories.h"
#include "grngame/platform/paths.h"
#include "grngame/utils/attributes.h"
#include "sqlite3.h"
#include <grngame/utils/string_compat.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "qoi.h"
#include "SDL3_image/SDL_image.h"

static int32 IsConfigJson(const char *path)
{
    // we dont put json from user , only config
    return strstr(path, "config.json") != NULL;
}

static EmbedKind ClassifyFile(const char *path)
{
    if (FileIsLoadableScript(path))
        return EMBED_KIND_SCRIPT;

    if (FileIsLoadableAudio(path) || FileIsLoadableImage(path))
        return EMBED_KIND_ASSET;

    if (FileIsLoadableText(path) || IsConfigJson(path))
        return EMBED_KIND_DATA;

    return EMBED_KIND_NONE;
}

static sqlite3 *OpenFreshEmbeddedDb(const char *output_path)
{
    if (DbExists(output_path))
        RemovePath(output_path);

    return DbCreate(output_path);
}

static void CreateEmbedSchema(sqlite3 *db)
{
    char sql[256];

    for (int32 i = 0; i < EMBED_KIND_COUNT; ++i)
    {
        snprintf(sql, sizeof(sql), "CREATE TABLE IF NOT EXISTS %s (path TEXT PRIMARY KEY, content BLOB);",
                 EMBED_TABLE_NAMES[i]);
        DataWrite(db, sql);
    }
}

static bool PrepareEmbedStatements(sqlite3 *db, EmbedContext *ctx)
{
    char sql[128];
    for (int32 i = 0; i < EMBED_KIND_COUNT; ++i)
    {
        snprintf(sql, sizeof(sql), "INSERT INTO %s (path, content) VALUES (?, ?);", EMBED_TABLE_NAMES[i]);
        bool success = DbStmtPrepare(db, sql, &ctx->stmts[i]);
        if (!success)
        {
            return false;
        }
    }
    return true;
}

static void FreeEmbedStatements(EmbedContext *ctx)
{
    for (int32 i = 0; i < EMBED_KIND_COUNT; ++i)
        DbStmtFree(&ctx->stmts[i]);
}

static unsigned char *ContentToQoi(const unsigned char *content, uint32 in_size, uint32 *out_size)
{
    *out_size = 0;

    SDL_IOStream *io = SDL_IOFromConstMem(content, in_size);
    if (!io)
        return NULL;

    SDL_Surface *raw_surf = IMG_Load_IO(io, true);
    if (!raw_surf)
        return NULL;

    SDL_Surface *rgba = SDL_ConvertSurface(raw_surf, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(raw_surf);
    if (!rgba)
        return NULL;

    qoi_desc desc = {
        .width = (unsigned int)rgba->w, .height = (unsigned int)rgba->h, .channels = 4, .colorspace = QOI_SRGB};

    int32 encoded_len = 0;
    unsigned char *qoi_buf = (unsigned char *)qoi_encode(rgba->pixels, &desc, &encoded_len);

    SDL_DestroySurface(rgba);

    if (qoi_buf)
        *out_size = (uint32)encoded_len;

    return qoi_buf;
}

static void EmbedFileCallback(const char *path, void *userdata)
{
    EmbedContext *ctx = (EmbedContext *)userdata;

    EmbedKind kind = ClassifyFile(path);

    if (kind == EMBED_KIND_NONE)
        return;

    uint32 size;
    unsigned char *content = ReturnFileData(path, &size);
    if (UNLIKELY(!content))
    {
        fprintf(stderr, "[EmbeddedFile] Failed to load file: %s\n", path);
        return;
    }

    if (kind == EMBED_KIND_ASSET && FileIsLoadableImage(path) && !FileIsLoadableQOI(path))
    {
        uint32 new_size = 0;
        unsigned char *new_content = ContentToQoi(content, size, &new_size);
        free(content);

        if (UNLIKELY(!new_content))
        {
            fprintf(stderr, "[EmbeddedFile] Failed to convert image to QOI: %s\n", path);
            return;
        }

        content = new_content;
        size = new_size;
        path = SetExtensionToX(path, ".qoi");
    }

    DbArg args[2];
    args[0].type = TEXT;
    args[0].value.s = path;
    args[1].type = DATA;
    args[1].value.blob.data = content;
    args[1].value.blob.size = size;

    DbStmtRun(&ctx->stmts[kind], args, 2);

    free(content);
}

void CreateEmbeddedFileDb(int32 num_dirs, const char **dirs, const char *output_path, const char *version)
{
    sqlite3 *db = OpenFreshEmbeddedDb(output_path);
    if (UNLIKELY(db == NULL))
    {
        DbClose(db);
        fprintf(stderr, "[EmbeddedFile] Failed to create database: %s\n", output_path);
        return;
    }

    ApplyDbPragmas(db);
    if (!ApplyDbVersion(db, version))
    {
        DbClose(db);
        RemovePath(output_path);
        fprintf(stderr, "[EmbeddedFile] Failed to create database: %s\n", output_path);
        return;
    }

    CreateEmbedSchema(db);

    EmbedContext ctx;
    if (!PrepareEmbedStatements(db, &ctx))
    {
        fprintf(stderr, "[EmbeddedFile] Failed to insert data into database: %s\n", output_path);
        return;
    }

    DbBegin(db);
    for (int32 i = 0; i < num_dirs; ++i)
        DirWalk(dirs[i], EmbedFileCallback, &ctx);

    DbCommit(db);

    FreeEmbedStatements(&ctx);
    DbClose(db);
}