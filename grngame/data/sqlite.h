#pragma once

#include "grngame/math/types.h"
#include "grngame/utils/c_cpp.h"
#include <kvec.h>
#include <sqlite3.h>
#include <stdbool.h>

BEGIN_DECLARATIONS

typedef enum
{
    INTEGER = 0,
    FLOAT_ = 1,
    TEXT = 2,
    DATA = 3,
    VOID_ = 4
} DbType;

typedef struct
{
    const char *name; // null for DbArg
    DbType type;
    union {
        int32 i;
        float64 f;
        const char *s;
        struct
        {
            void *data;
            uint32 size;
        } blob;
    } value;
} DbValue;

typedef struct
{
    kvec_t(DbValue) cols;
} DbRow;

typedef struct
{
    kvec_t(DbRow) rows;
} DbResult;

typedef struct
{
    sqlite3_stmt *stmt;
} DbStmt;

typedef DbValue DbArg;

sqlite3 *DbCreate(const char *name);
void DbClose(sqlite3 *db);
bool DbExists(const char *name);

// db result immutable
void DbResultPrint(DbResult *res);
void DbResultFree(DbResult *res);

bool DataWrite(sqlite3 *db, const char *sql);
bool DataFetch(sqlite3 *db, const char *sql, DbResult *out);

// wrap in begin/commit for bulk writes
bool DbBegin(sqlite3 *db);
bool DbCommit(sqlite3 *db);
bool DbRollback(sqlite3 *db);

bool DbStmtPrepare(sqlite3 *db, const char *sql, DbStmt *out);
bool DbStmtRun(DbStmt *s, DbArg *args, int32 argc);
void DbStmtFree(DbStmt *s);

// read without open it
bool ReadSqliteVersion(const char *path, int32 *out);

bool ApplyDbVersion(sqlite3 *db, const char *version);

bool ApplyDbPragmas(sqlite3 *db);

END_DECLARATIONS
