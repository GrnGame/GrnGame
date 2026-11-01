#include "sqlite.h"
#include "SDL3/SDL_iostream.h"
#include "file.h"
#include "grngame/dev/logging.h"
#include "grngame/math/types.h"
#include "grngame/utils/attributes.h"
#include "grngame/utils/memory.h"
#include "grngame/utils/version.h"
#include "kvec.h"
#include <SDL3/SDL.h>
#include <grngame/utils/string_compat.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static sqlite3_stmt *DbPrepareInternal(sqlite3 *db, const char *sql)
{
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
        LOG_ERROR("Sqlite Prepare error: %s", sqlite3_errmsg(db));
    return stmt;
}

static void DbBindArgs(sqlite3_stmt *stmt, DbArg *args, int32 argc)
{
    for (int32 i = 0; i < argc; i++)
    {
        switch (args[i].type)
        {
        case INTEGER:
            sqlite3_bind_int64(stmt, i + 1, args[i].value.i);
            break;
        case FLOAT_:
            sqlite3_bind_double(stmt, i + 1, args[i].value.f);
            break;
        case TEXT:
            sqlite3_bind_text(stmt, i + 1, args[i].value.s, -1, SQLITE_TRANSIENT);
            break;
        case DATA:
            sqlite3_bind_blob(stmt, i + 1, args[i].value.blob.data, args[i].value.blob.size, SQLITE_TRANSIENT);
            break;
        default:
            sqlite3_bind_null(stmt, i + 1);
            break;
        }
    }
}

bool DbExists(const char *name)
{
    return FileExist(name);
}

sqlite3 *DbCreate(const char *name)
{
    sqlite3 *db = NULL;
    if (UNLIKELY(sqlite3_open(name, &db) != SQLITE_OK))
    {
        LOG_ERROR("Impossible to create DB named: %s (%s)", name, db ? sqlite3_errmsg(db) : "unknown error");
        if (db)
            sqlite3_close(db);
        return NULL;
    }
    return db;
}

void DbClose(sqlite3 *db)
{
    sqlite3_close(db);
}

bool DbBegin(sqlite3 *db)
{
    if (!DataWrite(db, "BEGIN TRANSACTION;"))
    {
        LOG_ERROR("Failed to begin transaction");
        return false;
    }
    return true;
}

bool DbCommit(sqlite3 *db)
{
    if (!DataWrite(db, "COMMIT;"))
    {
        LOG_ERROR("Failed to commit");
        return false;
    }
    return true;
}

bool DbRollback(sqlite3 *db)
{
    if (!DataWrite(db, "ROLLBACK;"))
    {
        LOG_ERROR("Failed to rollback");
        return false;
    }
    return true;
}

bool DataWrite(sqlite3 *db, const char *sql)
{
    sqlite3_stmt *stmt = DbPrepareInternal(db, sql);
    if (!stmt)
        return false;

    int32 rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
    {
        LOG_ERROR("Sqlite execution error: %s", sqlite3_errmsg(db));
        return false;
    }
    return true;
}

static DbResult DbResultGet(sqlite3_stmt *stmt)
{
    DbResult result;
    kv_init(result.rows);

    int32 colCount = sqlite3_column_count(stmt);
    int32 rc;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        DbRow row;
        kv_init(row.cols);

        for (int32 i = 0; i < colCount; i++)
        {
            DbValue v = {0};
            v.name = strdup(sqlite3_column_name(stmt, i));

            switch (sqlite3_column_type(stmt, i))
            {
            case SQLITE_INTEGER:
                v.type = INTEGER;
                v.value.i = sqlite3_column_int64(stmt, i);
                break;
            case SQLITE_FLOAT:
                v.type = FLOAT_;
                v.value.f = sqlite3_column_double(stmt, i);
                break;
            case SQLITE_TEXT:
                v.type = TEXT;
                v.value.s = strdup((const char *)sqlite3_column_text(stmt, i));
                break;
            case SQLITE_BLOB: {
                v.type = DATA;
                int32 size = sqlite3_column_bytes(stmt, i);
                v.value.blob.data = malloc(size);
                memcpy(v.value.blob.data, sqlite3_column_blob(stmt, i), size);
                v.value.blob.size = (uint32)size;
                break;
            }
            case SQLITE_NULL:
                v.type = VOID_;
                break;
            }

            kv_push(DbValue, row.cols, v);
        }

        kv_push(DbRow, result.rows, row);
    }

    if (rc != SQLITE_DONE)
        LOG_ERROR("SQL step error: %s", sqlite3_errmsg(sqlite3_db_handle(stmt)));

    return result;
}

bool DataFetch(sqlite3 *db, const char *sql, DbResult *out)
{
    sqlite3_stmt *stmt = DbPrepareInternal(db, sql);
    if (!stmt)
    {
        *out = (DbResult){0};
        return false;
    }
    DbResult res = DbResultGet(stmt);
    sqlite3_finalize(stmt);
    *out = res;
    return true;
}

void DbResultPrint(DbResult *res)
{
    for (uint32 r = 0; r < (uint32)kv_size(res->rows); r++)
    {
        DbRow row = kv_A(res->rows, r);
        printf("Row %u:\n", r);
        for (uint32 c = 0; c < (uint32)kv_size(row.cols); c++)
        {
            DbValue v = kv_A(row.cols, c);
            printf("  %s = ", v.name);
            switch (v.type)
            {
            case INTEGER:
                printf("%lld\n", v.value.i);
                break;
            case FLOAT_:
                printf("%f\n", v.value.f);
                break;
            case TEXT:
                printf("%s\n", v.value.s);
                break;
            case DATA:
                printf("[BLOB size=%u]\n", v.value.blob.size);
                break;
            case VOID_:
                printf("NULL\n");
                break;
            }
        }
    }
}

void DbResultFree(DbResult *res)
{
    for (uint32 r = 0; r < (uint32)kv_size(res->rows); r++)
    {
        DbRow row = kv_A(res->rows, r);
        for (uint32 c = 0; c < (uint32)kv_size(row.cols); c++)
        {
            DbValue v = kv_A(row.cols, c);
            free((void *)v.name);
            if (v.type == TEXT)
                free((void *)v.value.s);
            if (v.type == DATA)
                free(v.value.blob.data);
        }
        kv_destroy(row.cols);
    }
    kv_destroy(res->rows);
}

bool DbStmtPrepare(sqlite3 *db, const char *sql, DbStmt *out)
{
    DbStmt s = {0};
    if (sqlite3_prepare_v2(db, sql, -1, &s.stmt, NULL) != SQLITE_OK)
    {
        *out = s;
        LOG_ERROR("Sqlite prepare error: %s", sqlite3_errmsg(db));
        return false;
    }
    *out = s;
    return true;
}

bool DbStmtRun(DbStmt *s, DbArg *args, int32 argc)
{
    sqlite3_clear_bindings(s->stmt);
    DbBindArgs(s->stmt, args, argc);

    int32 rc = sqlite3_step(s->stmt);
    sqlite3_reset(s->stmt);

    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
    {
        LOG_ERROR("Sqlite step error: %s", sqlite3_errmsg(sqlite3_db_handle(s->stmt)));
        return false;
    }
    return true;
}

void DbStmtFree(DbStmt *s)
{
    if (s->stmt)
    {
        sqlite3_finalize(s->stmt);
        s->stmt = NULL;
    }
}

bool ReadSqliteVersion(const char *path, int32 *out)
{
    SDL_IOStream *io = IOFromFile(path, "rb");
    if (!io)
        return false;

    uint8 h[100];
    bool ok = ReadIO(io, h, sizeof(h));
    CloseIO(io);

    if (!ok || memcmp(h, "SQLite format 3\0", 16) != 0)
        return false;

    *out = (int32)(((uint32)h[60] << 24) | ((uint32)h[61] << 16) | ((uint32)h[62] << 8) | (uint32)h[63]);
    return true;
}

bool ApplyDbPragmas(sqlite3 *db)
{
    if (!DataWrite(db, "PRAGMA journal_mode = OFF;"))
    {
        LOG_ERROR("Can't write database journal_mode");
        return false;
    }

    if (!DataWrite(db, "PRAGMA synchronous = OFF;"))
    {
        LOG_ERROR("Can't write database synchronous");
        return false;
    }

    if (!DataWrite(db, "PRAGMA cache_size = -65536;"))
    {
        LOG_ERROR("Can't write database cache_size");
        return false;
    }

    return true;
}

bool ApplyDbVersion(sqlite3 *db, const char *version)
{
    int32 ver = 0;
    if (!ParseVersionToInt(version, &ver))
    {
        fprintf(stdout, "Format version is incorrect please use major.minor.fix");
        return false;
    }
    char sql[64 * BUFFER_SPACE];
    snprintf(sql, sizeof(sql), "PRAGMA user_version = %d;", ver);
    if (!DataWrite(db, sql))
    {
        LOG_ERROR("Can't write database version :%s", version);
        return false;
    }
    return true;
}
