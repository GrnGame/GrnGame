#pragma once
#include "SDL3_mixer/SDL_mixer.h"
#include "asset_manager.h"
#include "grngame/math/math.h"
#include "grngame/utils/c_cpp.h"
#include <SDL3/SDL_surface.h>
#include <stdbool.h>

#define AUDIO_PREDECODE_SIZE_THRESHOLD (0.2f * MB)

BEGIN_DECLARATIONS

typedef struct LoadResult
{
    char *key;
    bool success;
    bool is_sound;

    SDL_Surface *pixels;
    SDL_Surface *surface_copy;
    MIX_Audio *stream;
} LoadResult;

typedef struct
{
    SDL_Surface *remapped; // NULL if failed
} ReloadResult;

typedef struct
{
    Texture *tex;
    ReloadResult *results;
    int32 index;
} ReloadTask;

bool LoadFileParallel(const char *file, LoadResult *result);
bool RegisterTextureResult(LoadResult *res);
bool RegisterSoundResult(LoadResult *res);
void InitPaletteRemapLUT(void);

bool LoadSoundFile(const char *file);
bool LoadTextureFile(const char *file);

bool UnloadSoundFile(const char *file);
bool UnloadTextureFile(const char *file);
bool UnloadAllSoundFiles(void);
bool UnloadAllTextureFiles(void);

bool ReloadAllTexturesWithPalette(void);
bool ReloadTextureWithPalette(const char *file);

END_DECLARATIONS
