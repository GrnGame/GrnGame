#include "load.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_iostream.h"
#include "SDL3_image/SDL_image.h"
#include "SDL3_mixer/SDL_mixer.h"
#include "grngame/assets/asset_manager.h"
#include "cooker_manager.h"
#include "grngame/core/app.h"
#include "grngame/core/thread.h"
#include "grngame/data/file.h"
#include "grngame/dev/logging.h"
#include "grngame/platform/paths.h"
#include "grngame/renderer/renderer.h"
#include "grngame/utils/attributes.h"
#include "grngame/utils/clear.h"
#include <stdlib.h>

static bool LoadSoundStream(const char *file, MIX_Audio **stream)
{
#ifdef GRNGAME_EMBED_ASSETS
    const CookerFile *asset = CookerFileGet(file);
    if (UNLIKELY(!asset))
    {
        LOG_WARNING("Embedded sound file '%s' was not found", file);
        return false;
    }

    SDL_IOStream *io = SDL_IOFromConstMem(asset->data, asset->size);
    if (UNLIKELY(!io))
    {
        LOG_WARNING("Failed to create IO stream for sound file '%s', SDL error: '%s'", file, SDL_GetError());
        return false;
    }

    bool predecode = (asset->size < (int32)AUDIO_PREDECODE_SIZE_THRESHOLD);
    *stream = MIX_LoadAudio_IO(g_app.sound_manager.mixer, io, predecode, true);
#else
    *stream = MIX_LoadAudio(g_app.sound_manager.mixer, file, false);
#endif

    if (UNLIKELY(!*stream))
    {
        LOG_WARNING("Failed to load sound file '%s', SDL error: '%s'", file, SDL_GetError());
        return false;
    }

    return true;
}

static bool RegisterSound(char *key, MIX_Audio *stream)
{
    khash_t(SoundMap) *map = g_app.asset_manager.sound_map;

    int32 ret;
    khiter_t k = kh_put(SoundMap, map, key, &ret);

    if (UNLIKELY(ret == 0))
    {
        free((char *)kh_key(map, k));
        kh_key(map, k) = key;
        MIX_DestroyAudio(kh_value(map, k));
    }

    kh_value(map, k) = stream;
    return true;
}

bool LoadSoundFile(const char *file)
{
    char *key = FileStem(file);
    MIX_Audio *stream = NULL;

    if (UNLIKELY(!LoadSoundStream(file, &stream)))
    {
        free(key);
        return false;
    }

    if (UNLIKELY(!RegisterSound(key, stream)))
    {
        MIX_DestroyAudio(stream);
        free(key);
        return false;
    }

    return true;
}

bool UnloadSoundFile(const char *file)
{
    khash_t(SoundMap) *map = g_app.asset_manager.sound_map;

    char *key = FileStem(file);
    khiter_t k = kh_get(SoundMap, map, key);
    free(key);

    if (UNLIKELY(k == kh_end(map)))
    {
        LOG_WARNING("Sound file '%s' is not loaded", file);
        return false;
    }

    MIX_DestroyAudio(kh_value(map, k));
    free((char *)kh_key(map, k));
    kh_del(SoundMap, map, k);

    return true;
}

bool UnloadAllSoundFiles(void)
{
    khash_t(SoundMap) *map = g_app.asset_manager.sound_map;

    for (khiter_t k = kh_begin(map); k != kh_end(map); ++k)
    {
        if (!kh_exist(map, k))
            continue;

        MIX_DestroyAudio(kh_value(map, k));
        free((char *)kh_key(map, k));
    }

    kh_clear(SoundMap, map);
    return true;
}

#ifndef GRNGAME_EMBED_ASSETS
static void ApplyPaletteToSurface(SDL_Surface *surface, PaletteManager *manager)
{
    if (SDL_MUSTLOCK(surface) && !SDL_LockSurface(surface))
        return;

    for (int32 y = 0; y < surface->h; ++y)
    {
        uint8 *row = (uint8 *)surface->pixels + (size_t)y * (size_t)surface->pitch;
        for (int32 x = 0; x < surface->w; ++x)
        {
            uint8 *px = row + (size_t)x * 4;
            if (px[3] == 0)
                continue;

            SDL_Color src = {.r = px[0], .g = px[1], .b = px[2], .a = px[3]};
            ColorLAB lab = RgbToLab(&src);
            int32 best_idx = FindBestPaletteColorCIEDE2000(manager, &lab);
            if (best_idx >= 0)
            {
                SDL_Color dst = manager->palette_elements.a[best_idx];
                px[0] = dst.r;
                px[1] = dst.g;
                px[2] = dst.b;
            }
        }
    }

    if (SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);
}

bool LoadTextureFile(const char *file)
{
    SDL_Surface *raw = IMG_Load(file);
    if (UNLIKELY(!raw))
    {
        LOG_WARNING("Failed to load image file '%s', SDL error: '%s'", file, SDL_GetError());
        return false;
    }

    SDL_Surface *surf = raw;
    if (g_app.info.enable_palette)
    {
        surf = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(raw);
        if (UNLIKELY(!surf))
            return false;

        ApplyPaletteToSurface(surf, &g_app.palette_manager);
    }

    SDL_Texture *sdl_tex = SDL_CreateTextureFromSurface(g_app.renderer.renderer, surf);
    uint16 w = (uint16)surf->w;
    uint16 h = (uint16)surf->h;
    SDL_DestroySurface(surf);

    if (UNLIKELY(!sdl_tex))
    {
        LOG_WARNING("Failed to create texture for '%s', SDL error: '%s'", file, SDL_GetError());
        return false;
    }
    SetPixelArtFilter(sdl_tex);
    char *key = FileStem(file);
    khash_t(TextureMap) *map = g_app.asset_manager.texture_map;

    int32 ret = 0;
    khiter_t k = kh_put(TextureMap, map, key, &ret);
    if (UNLIKELY(ret == 0))
    {
        free((char *)kh_key(map, k));
        kh_key(map, k) = key;
        SDL_DestroyTexture(kh_value(map, k).texture);
    }

    kh_value(map, k) = (Texture){.texture = sdl_tex, .w = w, .h = h};
    return true;
}

bool UnloadTextureFile(const char *file)
{
    khash_t(TextureMap) *map = g_app.asset_manager.texture_map;

    char *key = FileStem(file);
    khiter_t k = kh_get(TextureMap, map, key);
    free(key);

    if (UNLIKELY(k == kh_end(map)))
    {
        LOG_WARNING("Texture file '%s' is not loaded", file);
        return false;
    }

    SDL_DestroyTexture(kh_value(map, k).texture);
    free((char *)kh_key(map, k));
    kh_del(TextureMap, map, k);

    return true;
}

bool UnloadAllTextureFiles(void)
{
    khash_t(TextureMap) *map = g_app.asset_manager.texture_map;

    for (khiter_t k = kh_begin(map); k != kh_end(map); ++k)
    {
        if (!kh_exist(map, k))
            continue;

        SDL_DestroyTexture(kh_value(map, k).texture);
        free((char *)kh_key(map, k));
    }

    kh_clear(TextureMap, map);
    return true;
}
#else
bool LoadTextureFile(const char *file)
{
    (void)file;
    return false;
}

bool UnloadTextureFile(const char *file)
{
    khash_t(TextureMap) *map = g_app.asset_manager.texture_map;

    char *key = FileStem(file);
    khiter_t k = kh_get(TextureMap, map, key);
    free(key);

    if (UNLIKELY(k == kh_end(map)))
    {
        LOG_WARNING("Texture entry '%s' is not loaded", file);
        return false;
    }

    free((char *)kh_key(map, k));
    kh_del(TextureMap, map, k);
    return true;
}

bool UnloadAllTextureFiles(void)
{
    khash_t(TextureMap) *map = g_app.asset_manager.texture_map;

    for (khiter_t k = kh_begin(map); k != kh_end(map); ++k)
    {
        if (!kh_exist(map, k))
            continue;

        free((char *)kh_key(map, k));
    }

    kh_clear(TextureMap, map);
    return true;
}
#endif