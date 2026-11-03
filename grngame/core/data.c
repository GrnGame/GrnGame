#include "grngame/core/app.h"
#include "grngame/data/file.h"
#include "grngame/data/sqlite.h"
#include "grngame/dev/logging.h"
#include "grngame/platform/paths.h"
#include "grngame/utils/memory.h"
#include "grngame/utils/string_compat.h"
#include "grngame/utils/version.h"
#include <SDL3/SDL.h>
#include <stdlib.h>

static sqlite3 *OpenStorageDb(const char *name)
{
    char *path = PathFromStorage(name);
    if (!path)
        return NULL;

    sqlite3 *db = DbCreate(path);
    free(path);
    return db;
}

static bool CreateGameDataFile(const char *path, const char *version)
{
    sqlite3 *db = DbCreate(path);
    if (!db)
        return false;

    const bool ok = ApplyDbPragmas(db) && ApplyDbVersion(db, version) &&
                    DataWrite(db, "CREATE TABLE IF NOT EXISTS data (path TEXT PRIMARY KEY, content BLOB);");

    DbClose(db);

    if (!ok)
        RemovePath(path);
    return ok;
}

bool InstallGameData(void)
{
    char *path = PathFromStorage("game.data");
    if (!path)
        return false;

    bool ok = true;
    if (!FileExist(path))
    {
        LOG_DEBUG("Game.data does not exist, creating");
        ok = CreateGameDataFile(path, g_app.info.version);
    }

    free(path);
    return ok;
}

static bool DetectGameDataVersionChange(const char *path)
{
    int32 app_version = 0;
    if (!ParseVersionToInt(g_app.info.version, &app_version))
    {
        LOG_ERROR("Cannot parse application version");
        return false;
    }

    int32 stored_version = 0;
    if (!ReadSqliteVersion(path, &stored_version))
    {
        LOG_ERROR("Cannot read game.data version");
        return false;
    }

    const bool version_changed = (stored_version != app_version);

    if (version_changed)
    {
        char stored_str[64 * BUFFER_SPACE];
        if (!ParseVersionToString(stored_version, stored_str, sizeof(stored_str)))
        {
            LOG_ERROR("Cannot read game.data version");
            return false;
        }

        LOG_DEBUG("Changing game.data version from %s to %s", stored_str, g_app.info.version);
        g_app.info.old_version = strdup(stored_str);
    }

    g_app.info.update_event_data = version_changed;
    return true;
}

bool InitGameData(void)
{
    char *path = PathFromStorage("game.data");
    if (!path)
        return false;

    const bool ok = DetectGameDataVersionChange(path);
    free(path);
    if (!ok)
        return false;

    g_app.info.data_db = OpenStorageDb("game.data");
    if (!g_app.info.data_db)
    {
        LOG_ERROR("Cannot open game.data");
        return false;
    }

    return ApplyDbPragmas(g_app.info.data_db);
}