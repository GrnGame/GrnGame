#include "asset_manager.h"
#include "SDL3_image/SDL_image.h"
#include "grngame/assets/load.h"
#include "cooker_manager.h"
#include "grngame/cooker/read.h"
#include "grngame/core/app.h"
#include "grngame/dev/logging.h"
#include "grngame/dev/tracy.h"
#include "grngame/platform/directories.h"
#include "grngame/platform/paths.h"
#include "grngame/utils/attributes.h"
#include "grngame/utils/clear.h"
#include "khash.h"
#include "grngame/utils/time.h"

#include <stdlib.h>

#define ATLAS_NODE_NAME "atlas"

static Texture CreateDefaultTexture(void)
{
    Texture texture = {0};
    texture.w = 16;
    texture.h = 16;

    texture.texture =
        SDL_CreateTexture(g_app.renderer.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 16, 16);
    SetPixelArtFilter(texture.texture);
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

COLD AssetManager AssetManagerCreate(void)
{
    AssetManager manager = {
        .sound_map = kh_init(SoundMap),
        .texture_map = kh_init(TextureMap),
        .default_texture = CreateDefaultTexture(),
        .atlas = {0},
    };

    return manager;
}

#ifndef GRNGAME_EMBED_ASSETS
static void LoadAssetCallback(const char *path, void *userdata)
{
    int32 *asset_count = (int32 *)userdata;

    if (FileIsLoadableImage(path))
    {
        if (LoadTextureFile(path))
            (*asset_count)++;
    }
    else if (FileIsLoadableAudio(path))
    {
        if (LoadSoundFile(path))
            (*asset_count)++;
    }
}
#endif

void AssetManagerLoadFolder(const char *folder)
{
#ifndef GRNGAME_EMBED_ASSETS
    // need it for palette on assets
    InitLinearLut();
    int32 asset_count = 0;
    DirWalk(folder, LoadAssetCallback, &asset_count);

    if (asset_count == 0)
    {
        LOG_WARNING("No assets files in asset folder '%s'", folder);
        return;
    }
#else
    (void)folder;
#endif
}

#ifdef GRNGAME_EMBED_ASSETS

// can be long maybe need to profile it
static bool LoadEmbeddedAtlas(void)
{
    const CookerFile *atlas_file = CookerFileGet(ATLAS_NODE_NAME);
    if (!atlas_file)
    {
        LOG_WARNING("No atlas node found in %s", ASSETS_PAK);
        return true;
    }

    SDL_IOStream *io = SDL_IOFromConstMem(atlas_file->data, atlas_file->size);
    if (!io)
    {
        LOG_ERROR("Failed to create IO stream for atlas: %s", SDL_GetError());
        return false;
    }
    SDL_Surface *surf = IMG_Load_IO(io, true);
    if (!surf)
    {
        LOG_ERROR("Failed to decode atlas surface: %s", SDL_GetError());
        return false;
    }

    SDL_Texture *tex = SDL_CreateTextureFromSurface(g_app.renderer.renderer, surf);
    if (!tex)
    {
        LOG_ERROR("Failed to create atlas SDL_Texture: %s", SDL_GetError());
        SDL_DestroySurface(surf);
        return false;
    }
    SetPixelArtFilter(tex);
    g_app.asset_manager.atlas.texture = tex;
    g_app.asset_manager.atlas.w = (uint16)surf->w;
    g_app.asset_manager.atlas.h = (uint16)surf->h;
    SDL_DestroySurface(surf);
    return true;
}

static bool LoadEmbeddedAtlasEntries(void)
{
    uint32 count = 0;
    ArchiveAssetData *assets_data = ReadArchiveAssetsData(ASSETS_PAK, &count);
    if (!assets_data || count == 0)
        return true;

    khash_t(TextureMap) *map = g_app.asset_manager.texture_map;

    for (uint32 i = 0; i < count; ++i)
    {
        TextureMemory entry = {
            .positions = assets_data[i].positions,
            .size = assets_data[i].size,
        };

        // same key as the dev path: the file stem
        char *key = FileStem(assets_data[i].name);
        if (UNLIKELY(key == NULL))
            continue;

        int32 ret = 0;
        khiter_t k = kh_put(TextureMap, map, key, &ret);
        if (UNLIKELY(ret == 0))
        {
            // the map keeps its existing key, the new one is not needed
            LOG_WARNING("Duplicate atlas entry: %s", key);
            free(key);
        }

        kh_value(map, k) = entry;
    }

    FreeArchiveAssetsData(assets_data, count);
    return true;
}
#endif

void AssetManagerLoadFolderFromMemory(const char *folder)
{
    (void)folder;
#ifdef GRNGAME_EMBED_ASSETS
    PROFILE_FUNCTION("LoadFolderFromMemory");

    if (!LoadEmbeddedAtlas())
        return;

    if (!LoadEmbeddedAtlasEntries())
        return;

    khash_t(CookerFileHash) *files = g_app.cooker_file_manager.hash;
    if (!files)
        return;

    for (khiter_t k = kh_begin(files); k != kh_end(files); ++k)
    {
        if (!kh_exist(files, k))
            continue;

        const CookerFile *file = kh_value(files, k);
        if (file && file->kind == ARCHIVE_KIND_AUDIO)
            LoadSoundFile(file->name);
    }
#endif
}

void AssetManagerDestroy(AssetManager *manager)
{
    UnloadAllTextureFiles();
    UnloadAllSoundFiles();

    if (manager->atlas.texture)
    {
        SDL_DestroyTexture(manager->atlas.texture);
        manager->atlas.texture = NULL;
    }

    if (manager->default_texture.texture)
    {
        SDL_DestroyTexture(manager->default_texture.texture);
        manager->default_texture.texture = NULL;
    }

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

#ifndef GRNGAME_EMBED_ASSETS
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
#else
TextureMemory FindImage(const char *name, bool *found)
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

    TextureMemory fallback = {
        .positions = {.x = 0.0f, .y = 0.0f},
        .size = {.x = (float)g_app.asset_manager.default_texture.w, .y = (float)g_app.asset_manager.default_texture.h},
    };
    return fallback;
}
#endif