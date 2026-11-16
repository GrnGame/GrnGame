#include "asset_manager.h"
#include "grngame/assets/embedded_file_manager.h"
#include "grngame/assets/load.h"
#include "grngame/core/app.h"
#include "grngame/core/thread.h"
#include "grngame/dev/logging.h"
#include "grngame/dev/tracy.h"
#include "grngame/embedded/embedded_file_generator.h"
#include "grngame/platform/directories.h"
#include "grngame/platform/paths.h"
#include "grngame/utils/attributes.h"
#include "grngame/utils/clear.h"
#include "grngame/utils/string_compat.h"
#include "khash.h"
#include "kvec.h"
#include <stdlib.h>

static void AddAssetFileToArray(const char *path, void *user_data);
static void LoadTaskWorker(void *data);
static void LoadFilesMultithreaded(void);

static Texture CreateDefaultTexture()
{
    Texture texture = {0};
    texture.w = 16;
    texture.h = 16;

    texture.texture =
        SDL_CreateTexture(g_app.renderer.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 16, 16);

    uint32 pixels[16 * 16];

    for (int y = 0; y < 16; y++)
    {
        for (int x = 0; x < 16; x++)
        {
            bool white = ((x / 4) + (y / 4)) % 2;
            pixels[y * 16 + x] = white ? 0xFFFFFFFF : 0xFF000000;
        }
    }

    if (!SDL_UpdateTexture(texture.texture, NULL, pixels, 16 * sizeof(uint32)))
    {
        LOG_WARNING("Cant update default texture of engine");
    }
    return texture;
}

COLD AssetManager AssetManagerCreate()
{
    AssetManager manager = {
        .sound_map = kh_init(SoundMap), .texture_map = kh_init(TextureMap), .default_texture = CreateDefaultTexture()};

    kv_init(manager.assets_list);

    return manager;
}

static void LoadTaskWorker(void *data)
{
    LoadTask *task = (LoadTask *)data;

    LoadFileParallel(task->path, &task->results[task->index]);

    free(task);
}

static void LoadFilesMultithreaded(void)
{
    string_vec_t *list = &g_app.asset_manager.assets_list;
    int32 count = (int32)kv_size(*list);

    if (count == 0)
        return;

    if (g_app.info.enable_palette)
    {
        InitPaletteRemapLUT();
    }

    LoadResult *results = malloc(count * sizeof(LoadResult));
    CLEAR_ARRAY(results, 0, count);

    for (int32 i = 0; i < count; ++i)
    {
        LoadTask *task = malloc(sizeof(LoadTask));

        task->path = kv_A(*list, i);
        task->results = results;
        task->index = i;

        ThreadManagerPush(LoadTaskWorker, task);
    }

    ThreadManagerWait();

    // main thread because gpu is not safe thread
    for (int32 i = 0; i < count; ++i)
    {
        LoadResult *res = &results[i];

        if (!res->success)
            continue;

        if (res->is_sound)
            RegisterSoundResult(res);
        else
            RegisterTextureResult(res);
    }

    free(results);
}

static void AddAssetFileToArray(const char *path, void *user_data)
{
    int32 *count = (int32 *)user_data;

    if (!FileIsLoadableImage(path) && !FileIsLoadableAudio(path))
        return;

    if (count)
        (*count)++;

    kv_push(char *, g_app.asset_manager.assets_list, strdup(path));
}

void AssetManagerLoadFolder(const char *folder)
{
    PROFILE_FUNCTION("LoadFolder");
    int32 asset_count = 0;
    DirWalk(folder, AddAssetFileToArray, &asset_count);

    if (asset_count == 0)
    {
        LOG_WARNING("No assets files in asset folder '%s'", folder);
        return;
    }
    LoadFilesMultithreaded();
}

void AssetManagerLoadFolderFromMemory(const char *folder)
{
    PROFILE_FUNCTION("LoadFolderFromMemory");
    int32 asset_count = 0;
    LOG_DEBUG(
        "assets count: %d,scripts count: %d,data count: %d", (int32)EmbeddedFileManagerGetCounts(EMBED_KIND_ASSET),
        (int32)EmbeddedFileManagerGetCounts(EMBED_KIND_SCRIPT), (int32)EmbeddedFileManagerGetCounts(EMBED_KIND_DATA));

    if (UNLIKELY(EmbeddedFileManagerGetCounts(EMBED_KIND_ASSET)) == 0)
    {
        LOG_WARNING("No assets files in embedded assets folder '%s'", folder);
        return;
    }
    khash_t(EmbeddedFileHash) *hash = g_app.embedded_file_manager.hash;
    for (khint_t k = kh_begin(hash); k != kh_end(hash); ++k)
    {
        if (kh_exist(hash, k))
        {
            EmbeddedFile *asset = kh_value(hash, k);
            AddAssetFileToArray(asset->name, &asset_count);
        }
    }
    if (asset_count == 0)
    {
        LOG_WARNING("No assets files in asset folder '%s'", folder);
        return;
    }
    LoadFilesMultithreaded();
}

void AssetManagerDestroy(AssetManager *manager)
{

    UnloadAllTextureFiles();
    UnloadAllSoundFiles();
    // Keys are owned by assets_list.
    PaletteFreeStringVec(&manager->assets_list);
    kh_destroy(TextureMap, manager->texture_map);
    kh_destroy(SoundMap, manager->sound_map);
    manager->texture_map = NULL;
    manager->sound_map = NULL;
}

MIX_Audio *FindAudio(const char *name)
{
    khash_t(SoundMap) *map = g_app.asset_manager.sound_map;
    khiter_t it = kh_get(SoundMap, map, name);
    return it == kh_end(map) ? NULL : kh_value(map, it);
}

Texture FindImage(const char *name, bool *found)
{
    khash_t(TextureMap) *map = g_app.asset_manager.texture_map;
    khiter_t it = kh_get(TextureMap, map, name);

    if (it != kh_end(map))
    {
        if (found)
            *found = true;

        return kh_value(map, it);
    }

    if (found)
        *found = false;

    return g_app.asset_manager.default_texture;
}
