#include "embedded_file_manager.h"
#include "grngame/core/app.h"
#include "grngame/data/sqlite.h"
#include "grngame/dev/logging.h"
#include "grngame/embedded/embedded_file_generator.h"
#include "grngame/utils/attributes.h"
#include "grngame/utils/clear.h"
#include "grngame/utils/string_compat.h"
#include "khash.h"
#include "kvec.h"
#include <stdlib.h>
#include <string.h>

static const EmbedKind GAME_DATA_KIND = EMBED_KIND_DATA;

const char *EmbedKindTable(EmbedKind kind)
{
    return EMBED_TABLE_NAMES[kind];
}

static EmbeddedFile *EmbeddedFileCreate(EmbedKind kind, const char *path, const void *data, uint32 size)
{
    EmbeddedFile *file = malloc(sizeof(*file));
    file->name = strdup(path);
    file->data = malloc(size);
    file->size = size;
    file->kind = kind;
    memcpy(file->data, data, size);
    return file;
}

static void EmbeddedFileFree(EmbeddedFile *file)
{
    if (!file)
        return;

    free((char *)file->name);
    free(file->data);
    free(file);
}

COLD EmbeddedFileManager EmbeddedFileManagerCreate(void)
{
    EmbeddedFileManager manager = {0};
    manager.hash = kh_init(EmbeddedFileHash);
    return manager;
}

COLD void EmbeddedFileManagerDestroy(EmbeddedFileManager *manager)
{
    for (khiter_t k = kh_begin(manager->hash); k != kh_end(manager->hash); ++k)
    {
        if (kh_exist(manager->hash, k))
            EmbeddedFileFree(kh_value(manager->hash, k));
    }

    kh_destroy(EmbeddedFileHash, manager->hash);
    manager->hash = NULL;

    CLEAR(manager->counts, 0);
    manager->total_count = 0;
}

static void CountAdd(EmbeddedFileManager *mgr, EmbedKind kind)
{
    mgr->counts[kind]++;
    mgr->total_count++;
}

static void CountRemove(EmbeddedFileManager *mgr, EmbedKind kind)
{
    mgr->counts[kind]--;
    mgr->total_count--;
}

static void MemoryPutOwned(EmbeddedFile *file)
{
    EmbeddedFileManager *mgr = &g_app.embedded_file_manager;
    int ret;
    khiter_t k = kh_put(EmbeddedFileHash, mgr->hash, file->name, &ret);

    if (ret == 0) /* replace if key already exist */
    {
        EmbeddedFile *old = kh_value(mgr->hash, k);
        CountRemove(mgr, old->kind);
        EmbeddedFileFree(old);
    }

    kh_key(mgr->hash, k) = file->name;
    kh_value(mgr->hash, k) = file;
    CountAdd(mgr, file->kind);
}

static bool MemoryRemove(const char *path)
{
    EmbeddedFileManager *mgr = &g_app.embedded_file_manager;
    khiter_t k = kh_get(EmbeddedFileHash, mgr->hash, path);
    if (k == kh_end(mgr->hash))
        return false;

    EmbeddedFile *file = kh_value(mgr->hash, k);
    CountRemove(mgr, file->kind);

    kh_del(EmbeddedFileHash, mgr->hash, k);
    EmbeddedFileFree(file);
    return true;
}

void EmbeddedFilePutMemory(EmbedKind kind, const char *path, const void *data, uint32 size)
{
    MemoryPutOwned(EmbeddedFileCreate(kind, path, data, size));
}

static bool LoadRow(DbRow *row, EmbedKind kind)
{
    if (UNLIKELY((uint32)kv_size(row->cols) < 2))
        return false;

    DbValue *path = &kv_A(row->cols, 0);
    DbValue *content = &kv_A(row->cols, 1);

    if (UNLIKELY(path->type != TEXT || content->type != DATA))
        return false;

    EmbeddedFilePutMemory(kind, path->value.s, content->value.blob.data, content->value.blob.size);
    return true;
}

static bool LoadTableFrom(sqlite3 *db, const char *label, EmbedKind kind)
{
    char sql[128 * BUFFER_SPACE];
    snprintf(sql, sizeof(sql), "SELECT path, content FROM %s;", EmbedKindTable(kind));

    bool success;
    DbResult res;
    success = DataFetch(db, sql, &res);

    bool ok = success;
    for (uint32 i = 0; ok && i < (uint32)kv_size(res.rows); ++i)
        ok = LoadRow(&kv_A(res.rows, i), kind);

    if (!ok)
        LOG_ERROR("%s: invalid structure '%s'", label, EmbedKindTable(kind));

    DbResultFree(&res);
    return ok;
}

static bool AssetsPakTableExists(const char *name)
{
    char sql[256 * BUFFER_SPACE];
    snprintf(sql, sizeof(sql),
             "SELECT 1 FROM sqlite_master "
             "WHERE type = 'table' AND name = '%s' LIMIT 1;",
             name);

    DbResult res;
    bool success = DataFetch(g_app.info.file_db, sql, &res);
    bool exists = success && (uint32)kv_size(res.rows) > 0;
    DbResultFree(&res);
    return exists;
}

bool EmbeddedDbHasValidSchema()
{
    for (int32 i = 0; i < EMBED_KIND_COUNT; ++i)
    {
        if (!AssetsPakTableExists(EmbedKindTable((EmbedKind)i)))
        {
            LOG_ERROR("Assets.pak: missing table '%s'", EmbedKindTable((EmbedKind)i));
            return false;
        }
    }
    return true;
}

bool EmbeddedFileManagerLoadAssetsPak()
{
    if (!EmbeddedDbHasValidSchema())
    {
        LOG_ERROR("%s", "Assets.pak structure isn't correct");
        return false;
    }

    for (int32 i = 0; i < EMBED_KIND_COUNT; ++i)
    {
        if (!LoadTableFrom(g_app.info.file_db, "Assets.pak", (EmbedKind)i))
            return false;
    }
    return true;
}

bool EmbeddedFileManagerLoadGameData()
{
    return LoadTableFrom(g_app.info.data_db, "game.data", GAME_DATA_KIND);
}

const EmbeddedFile *EmbeddedFileGet(const char *path)
{
    EmbeddedFileManager *mgr = &g_app.embedded_file_manager;
    khiter_t k = kh_get(EmbeddedFileHash, mgr->hash, path);
    return (k == kh_end(mgr->hash)) ? NULL : kh_value(mgr->hash, k);
}

bool EmbeddedFileExists(const char *path)
{
    return EmbeddedFileGet(path) != NULL;
}

const char *EmbedKindTable(EmbedKind kind);

uint32 EmbeddedFileManagerGetCounts(EmbedKind kind)
{
    return g_app.embedded_file_manager.counts[kind];
}

uint32 EmbeddedFileManagerGetTotalCount()
{
    return g_app.embedded_file_manager.total_count;
}

static bool GameDataExec(const char *sql, DbArg *args, int32 arg_count)
{
    bool prepared;
    DbStmt stmt;
    prepared = DbStmtPrepare(g_app.info.data_db, sql, &stmt);
    if (!prepared)
    {
        LOG_ERROR("Cant prepare statement on game.data");
        return false;
    }

    bool ok = DbStmtRun(&stmt, args, arg_count);
    DbStmtFree(&stmt);

    if (!ok)
        LOG_ERROR("Cant run statement on game.data");
    return ok;
}

static bool GameDataDbWrite(const char *path, const void *data, uint32 size)
{
    char sql[255 * BUFFER_SPACE];
    snprintf(sql, sizeof(sql),
             "INSERT INTO %s (path, content) VALUES (?, ?) "
             "ON CONFLICT(path) DO UPDATE SET content = excluded.content;",
             EmbedKindTable(GAME_DATA_KIND));

    DbArg args[2] = {
        {.name = NULL, .type = TEXT, .value.s = path},
        {.name = NULL, .type = DATA, .value.blob = {.data = (void *)data, .size = size}},
    };
    return GameDataExec(sql, args, 2);
}

static bool GameDataDbDelete(const char *path)
{
    char sql[128 * BUFFER_SPACE];
    snprintf(sql, sizeof(sql), "DELETE FROM %s WHERE path = ?;", EmbedKindTable(GAME_DATA_KIND));

    DbArg arg = {.name = NULL, .type = TEXT, .value.s = path};
    return GameDataExec(sql, &arg, 1);
}

bool EmbeddedFilePutDisk(const char *path, const void *data, uint32 size)
{
    return GameDataDbWrite(path, data, size);
}

bool EmbeddedFilePut(const char *path, const void *data, uint32 size)
{
    if (!EmbeddedFilePutDisk(path, data, size))
        return false;

    EmbeddedFilePutMemory(GAME_DATA_KIND, path, data, size);
    return true;
}

bool EmbeddedFileRemove(const char *path)
{
    const EmbeddedFile *file = EmbeddedFileGet(path);
    if (!file || file->kind != GAME_DATA_KIND)
        return false;

    if (!GameDataDbDelete(path))
        return false;

    return MemoryRemove(path);
}