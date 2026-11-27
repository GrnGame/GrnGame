#pragma once
#include "../math/types.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_surface.h"
#include "cglm/types-struct.h"
#include "cooker_manager.h"
#include "grngame/renderer/cielab.h"
#include "grngame/utils/attributes.h"
#include "grngame/utils/c_cpp.h"
#include <SDL3_mixer/SDL_mixer.h>
#include <khash.h>

typedef struct LoadResult LoadResult;

BEGIN_DECLARATIONS

typedef struct
{
    SDL_Texture *texture;
    uint16 w;
    uint16 h;
} Texture;

typedef struct
{
    vec2s positions;
    vec2s size;
} TextureMemory;

KHASH_MAP_INIT_STR(SoundMap, MIX_Audio *);

#ifndef GRNGAME_EMBED_ASSETS
KHASH_MAP_INIT_STR(TextureMap, Texture);
#else
KHASH_MAP_INIT_STR(TextureMap, TextureMemory);
#endif

struct AppInfo;

typedef struct
{
    khash_t(SoundMap) * sound_map;
    khash_t(TextureMap) * texture_map;
    Texture default_texture;
    Texture atlas;
} AssetManager;

COLD AssetManager AssetManagerCreate(void);
COLD void AssetManagerDestroy(AssetManager *manager);

void AssetManagerLoadFolder(const char *folder);
void AssetManagerLoadFolderFromMemory(const char *folder);

#ifndef GRNGAME_EMBED_ASSETS
Texture FindImage(const char *name, bool *found);
#else
TextureMemory FindImage(const char *name, bool *found);
#endif

MIX_Audio *FindAudio(const char *name);

END_DECLARATIONS