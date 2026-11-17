#include "load.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_iostream.h"
#include "SDL3_image/SDL_image.h"
#include "SDL3_mixer/SDL_mixer.h"
#include "grngame/assets/asset_manager.h"
#include "grngame/core/app.h"
#include "grngame/core/thread.h"
#include "grngame/data/file.h"
#include "grngame/dev/logging.h"
#include "grngame/platform/paths.h"
#include "grngame/renderer/renderer.h"
#include "grngame/utils/attributes.h"
#include "grngame/utils/clear.h"

static bool LoadSoundStream(const char *file, MIX_Audio **stream);
static bool RegisterSound(char *key, MIX_Audio *stream);

static bool LoadTextureSurface(const char *file, SDL_Surface **surface)
{
#ifdef GRNGAME_EMBED_ASSETS
    const EmbeddedFile *asset = EmbeddedFileGet(file);
    if (UNLIKELY(!asset))
    {
        LOG_WARNING("Embedded texture file '%s' was not found", file);
        return false;
    }

    SDL_IOStream *io = SDL_IOFromConstMem(asset->data, asset->size);
    if (UNLIKELY(!io))
    {
        LOG_WARNING("Failed to create IO stream for texture file '%s', SDL error: '%s'", file, SDL_GetError());
        return false;
    }

    *surface = IMG_Load_IO(io, true);
#else
    *surface = IMG_Load(file);
#endif

    if (UNLIKELY(!*surface))
    {
        LOG_WARNING("Failed to load texture file '%s', SDL error: '%s'", file, SDL_GetError());
        return false;
    }

    return true;
}

#define LUT_EMPTY -1

static int16 lut_table[32 * 64 * 32];
static bool lut_table_init = false;

static inline uint16 RGB888TORGB565(const SDL_Color *c)
{
    return (uint16)(((uint16)c->r >> 3 << 11) | ((uint16)c->g >> 2 << 5) | ((uint16)c->b >> 3));
}

void InitPaletteRemapLUT(void)
{
    CLEAR(lut_table, LUT_EMPTY);
    lut_table_init = true;
}

static void ClearPaletteRemapCache(void)
{
    lut_table_init = false;
}

static HOT void ApplyPaletteRemap(SDL_Surface *restrict surface)
{
    const SDL_Color *restrict palette = g_app.palette_manager.palette_elements.a;
    const int32 palette_count = (int32)kv_size(g_app.palette_manager.palette_elements);

    const int32 w = surface->w;
    const int32 h = surface->h;
    const int32 pitch = surface->pitch;
    uint8 *restrict base_pixels = (uint8 *)surface->pixels;

    for (int32 y = 0; y < h; ++y)
    {
        SDL_Color *restrict row = (SDL_Color *)(base_pixels + y * pitch);

        for (int32 x = 0; x < w; ++x)
        {
            SDL_Color *restrict pixel = &row[x];

            if (UNLIKELY(pixel->a == 0))
                continue;

            uint16 key = RGB888TORGB565(pixel);

            int16 idx = lut_table[key];

            if (UNLIKELY(idx == LUT_EMPTY))
            {
                ColorLAB lab = RgbToLab(pixel);
                int32 best = FindBestPaletteColorCIEDE2000(&lab);

                if (UNLIKELY(best < 0 || best >= palette_count))
                    continue;

                idx = (int16)best;
                lut_table[key] = idx;
            }

            SDL_Color color = palette[idx];
            pixel->r = color.r;
            pixel->g = color.g;
            pixel->b = color.b;
        }
    }
}
static SDL_Texture *CreateConfiguredTexture(SDL_Surface *surface, const char *name_for_log)
{
    SDL_Texture *texture = SDL_CreateTextureFromSurface(g_app.renderer.renderer, surface);
    if (UNLIKELY(!texture))
    {
        LOG_WARNING("Failed to create texture for '%s', SDL error: '%s'", name_for_log, SDL_GetError());
        return NULL;
    }

    if (UNLIKELY(!SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST) ||
                 !SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND)))
    {
        LOG_WARNING("Failed to configure texture '%s', SDL error: '%s'", name_for_log, SDL_GetError());
        SDL_DestroyTexture(texture);
        return NULL;
    }

    return texture;
}

static bool RegisterTexture(char *key, SDL_Texture *texture, SDL_Surface *surface, int16 width, int16 height)
{
    khash_t(TextureMap) *map = g_app.asset_manager.texture_map;

    int32 ret;
    khiter_t k = kh_put(TextureMap, map, key, &ret);

    if (UNLIKELY(ret == 0))
    {
        free((char *)kh_key(map, k));
        kh_key(map, k) = key;

        SDL_DestroyTexture(kh_value(map, k).texture);
        SDL_DestroySurface(kh_value(map, k).surface);
    }

    kh_value(map, k) = (Texture){.texture = texture, .surface = surface, .w = width, .h = height};
    return true;
}

bool LoadFileParallel(const char *file, LoadResult *result)
{
    *result = (LoadResult){0};

    if (FileIsLoadableAudio(file))
    {
        result->is_sound = true;
        result->key = FileStem(file);

        if (UNLIKELY(!LoadSoundStream(file, &result->stream)))
        {
            free(result->key);
            result->key = NULL;
            return false;
        }

        result->success = true;
        return true;
    }

    if (FileIsLoadableImage(file) || FileIsLoadableQOI(file))
    {
        result->is_sound = false;
        result->key = FileStem(file);

        SDL_Surface *raw_surface = NULL;
        if (UNLIKELY(!LoadTextureSurface(file, &raw_surface)))
        {
            free(result->key);
            result->key = NULL;
            return false;
        }

        SDL_Surface *rgba = NULL;

        // everys files in embedded is at rgba32 for optimisation
        if (raw_surface->format == SDL_PIXELFORMAT_RGBA32)
        {
            rgba = raw_surface;
        }
        else
        {
            rgba = SDL_ConvertSurface(raw_surface, SDL_PIXELFORMAT_RGBA32);
            SDL_DestroySurface(raw_surface);
            if (UNLIKELY(!rgba))
            {
                LOG_WARNING("Failed to convert texture file '%s', SDL error: '%s'", file, SDL_GetError());
                free(result->key);
                result->key = NULL;
                return false;
            }
        }

        if (g_app.info.enable_palette)
        {
            result->surface_copy = rgba;
            SDL_Surface *remapped = SDL_DuplicateSurface(rgba);
            if (UNLIKELY(!remapped))
            {
                SDL_DestroySurface(rgba);
                free(result->key);
                result->key = NULL;
                return false;
            }

            ApplyPaletteRemap(remapped);
            result->pixels = remapped;
        }
        else
        {
            result->surface_copy = rgba;
            result->pixels = rgba;
        }

        result->success = true;
        return true;
    }

    LOG_WARNING("Unknown file type in asset folder: '%s'", FileExtension(file));
    return false;
}

bool RegisterTextureResult(LoadResult *res)
{
    int16 tex_w = (int16)res->pixels->w;
    int16 tex_h = (int16)res->pixels->h;

    SDL_Texture *texture = CreateConfiguredTexture(res->pixels, res->key);

    if (res->pixels != res->surface_copy)
    {
        SDL_DestroySurface(res->pixels);
    }

    if (UNLIKELY(!texture))
    {
        SDL_DestroySurface(res->surface_copy);
        free(res->key);
        return false;
    }

    if (UNLIKELY(!RegisterTexture(res->key, texture, res->surface_copy, tex_w, tex_h)))
    {
        SDL_DestroyTexture(texture);
        SDL_DestroySurface(res->surface_copy);
        free(res->key);
        return false;
    }

    LOG_DEBUG("Loaded texture file '%s'", res->key);
    return true;
}

bool RegisterSoundResult(LoadResult *res)
{
    if (UNLIKELY(!RegisterSound(res->key, res->stream)))
    {
        MIX_DestroyAudio(res->stream);
        free(res->key);
        return false;
    }

    LOG_DEBUG("Loaded sound file '%s'", res->key);
    return true;
}

static bool LoadSoundStream(const char *file, MIX_Audio **stream)
{
#ifdef GRNGAME_EMBED_ASSETS
    const EmbeddedFile *asset = EmbeddedFileGet(file);
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

bool LoadTextureFile(const char *file)
{
    LoadResult res = {0};
    if (UNLIKELY(!LoadFileParallel(file, &res) || res.is_sound))
    {
        return false;
    }
    return RegisterTextureResult(&res);
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

bool ReloadTextureWithPalette(const char *file)
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

    Texture *tex = &kh_value(map, k);
    if (UNLIKELY(!tex->surface))
    {
        LOG_WARNING("Texture file '%s' has no source surface", file);
        return false;
    }

    SDL_Surface *rgba = SDL_DuplicateSurface(tex->surface);
    if (UNLIKELY(!rgba))
    {
        LOG_WARNING("Failed to duplicate texture surface '%s', SDL error: '%s'", file, SDL_GetError());
        return false;
    }

    ApplyPaletteRemap(rgba);

    SDL_Texture *new_texture = CreateConfiguredTexture(rgba, file);
    SDL_DestroySurface(rgba);

    if (UNLIKELY(!new_texture))
        return false;

    SDL_DestroyTexture(tex->texture);
    tex->texture = new_texture;

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
    SDL_DestroySurface(kh_value(map, k).surface);
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
        SDL_DestroySurface(kh_value(map, k).surface);
        free((char *)kh_key(map, k));
    }

    kh_clear(TextureMap, map);
    return true;
}

static void ReloadTaskWorker(void *data)
{
    ReloadTask *task = (ReloadTask *)data;
    SDL_Surface *rgba = NULL;

    if (LIKELY(task->tex->surface != NULL))
    {
        rgba = SDL_DuplicateSurface(task->tex->surface);
        if (LIKELY(rgba != NULL))
            ApplyPaletteRemap(rgba);
    }

    task->results[task->index].remapped = rgba;
}

bool ReloadAllTexturesWithPalette(void)
{
    ClearPaletteRemapCache();
    InitPaletteRemapLUT();

    khash_t(TextureMap) *map = g_app.asset_manager.texture_map;
    int32 count = (int32)kh_size(map);

    if (count == 0)
        return true;

    Texture **texture_ptrs = malloc((uint32)count * sizeof(Texture *));
    ReloadResult *results = malloc((uint32)count * sizeof(ReloadResult));
    CLEAR_ARRAY(texture_ptrs, 0, count);
    CLEAR_ARRAY(results, 0, count);

    ReloadTask *tasks = malloc((uint32)count * sizeof(ReloadTask));

    if (UNLIKELY(!texture_ptrs || !results || !tasks))
    {
        free(texture_ptrs);
        free(results);
        free(tasks);
        return false;
    }

    int32 idx = 0;
    for (khiter_t k = kh_begin(map); k != kh_end(map); ++k)
    {
        if (!kh_exist(map, k))
            continue;

        Texture *tex = &kh_value(map, k);
        texture_ptrs[idx] = tex;

        tasks[idx] = (ReloadTask){.tex = tex, .results = results, .index = idx};

        ThreadManagerPush(ReloadTaskWorker, &tasks[idx]);
        idx++;
    }

    ThreadManagerWait();

    bool success = true;

    for (int32 i = 0; i < idx; ++i)
    {
        SDL_Surface *rgba = results[i].remapped;
        Texture *tex = texture_ptrs[i];

        if (UNLIKELY(!rgba || !tex))
        {
            LOG_WARNING("Failed to remap texture at index %d", i);
            if (rgba)
                SDL_DestroySurface(rgba);
            success = false;
            continue;
        }

        if (LIKELY(tex->texture != NULL))
        {
            if (UNLIKELY(!SDL_UpdateTexture(tex->texture, NULL, rgba->pixels, rgba->pitch)))
            {
                LOG_WARNING("Failed to update texture while reloading palette, SDL error: '%s'", SDL_GetError());
                success = false;
            }
            SDL_DestroySurface(rgba);
        }
        else
        {
            SDL_Texture *new_texture = CreateConfiguredTexture(rgba, "reload_palette");
            SDL_DestroySurface(rgba);

            if (UNLIKELY(!new_texture))
            {
                success = false;
                continue;
            }
            tex->texture = new_texture;
        }
    }

    free(tasks);
    free(texture_ptrs);
    free(results);

    return success;
}