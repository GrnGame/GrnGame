#include "cooker_manager.h"

#include "grngame/cooker/parser.h"
#include "grngame/cooker/read.h"
#include "grngame/cooker/write.h"
#include "grngame/core/app.h"
#include "grngame/dev/logging.h"
#include "grngame/utils/clear.h"
#include <stdlib.h>

static void CookerFileFree(CookerFile *file)
{
    if (file == NULL)
        return;

    free((void *)file->name);
    free(file->data);
    free(file);
}

CookerFileManager CookerFileManagerCreate(void)
{
    CookerFileManager manager;
    CLEAR(manager, 0);
    manager.hash = kh_init(CookerFileHash);
    return manager;
}

void CookerFileManagerDestroy(CookerFileManager *manager)
{
    if (manager == NULL || manager->hash == NULL)
        return;

    for (khiter_t k = kh_begin(manager->hash); k != kh_end(manager->hash); ++k)
    {
        if (kh_exist(manager->hash, k))
            CookerFileFree(kh_value(manager->hash, k));
    }

    kh_destroy(CookerFileHash, manager->hash);
    manager->hash = NULL;

    CLEAR(manager->counts, 0);
    manager->total_count = 0;
}

static bool CookerFileInsert(CookerFileManager *mgr, ArchiveNode *node)
{
    CookerFile *file = malloc(sizeof(*file));

    file->name = node->name;
    file->data = node->content;
    file->size = node->content_size;
    file->kind = node->kind;

    // ownership moved, the node must not free them
    node->name = NULL;
    node->content = NULL;

    int ret = 0;
    khiter_t k = kh_put(CookerFileHash, mgr->hash, file->name, &ret);

    if (ret == 0)
    {
        LOG_ERROR("Duplicate file in archive: %s", file->name);
        CookerFileFree(file);
        return false;
    }

    kh_value(mgr->hash, k) = file;
    mgr->counts[file->kind]++;
    mgr->total_count++;
    return true;
}

bool CookerFileManagerLoadAssetsPak(void)
{
    CookerFileManager *mgr = &g_app.cooker_file_manager;

    uint32 count = 0;
    ArchiveNode *nodes = ReadArchiveNodes(ASSETS_PAK, &count);
    if (nodes == NULL)
    {
        LOG_ERROR("Cant read %s", ASSETS_PAK);
        return false;
    }

    bool ok = true;
    for (uint32 i = 0; ok && i < count; ++i)
    {
        if (nodes[i].kind == ARCHIVE_KIND_NONE || (uint32)nodes[i].kind >= ARCHIVE_KIND_COUNT)
        {
            LOG_ERROR("%s: invalid kind for '%s'", ASSETS_PAK, nodes[i].name);
            ok = false;
        }
        else
        {
            ok = CookerFileInsert(mgr, &nodes[i]);
        }
    }

    // frees what was not moved into the hash, and the array itself
    FreeArchiveNodes(nodes, count);
    return ok;
}

const CookerFile *CookerFileGet(const char *name)
{
    if (name == NULL)
        return NULL;

    CookerFileManager *mgr = &g_app.cooker_file_manager;
    if (mgr->hash == NULL)
        return NULL;

    khiter_t k = kh_get(CookerFileHash, mgr->hash, name);
    return (k == kh_end(mgr->hash)) ? NULL : kh_value(mgr->hash, k);
}

bool CookerFileExists(const char *name)
{
    return CookerFileGet(name) != NULL;
}

uint32 CookerFileManagerGetCounts(ArchiveKind kind)
{
    return g_app.cooker_file_manager.counts[kind];
}

uint32 CookerFileManagerGetTotalCount(void)
{
    return g_app.cooker_file_manager.total_count;
}