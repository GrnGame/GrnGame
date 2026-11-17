#pragma once

#include <SDL3_mixer/SDL_mixer.h>
#include <stdbool.h>

#pragma once

#include "filter.h"
#include "grngame/math/types.h"
#include "grngame/utils/c_cpp.h"
#include <SDL3_mixer/SDL_mixer.h>
#include <cglm/types-struct.h>

#define SOUND_MAX_SLOTS 255
#define SOUND_NAME_MAX 128

BEGIN_DECLARATIONS

typedef struct
{
    MIX_Track *track;
    FilterChain *filters;
    vec2s position;
    char name[SOUND_NAME_MAX];
} SoundSlot;

typedef struct
{
    MIX_Mixer *mixer;
    SoundSlot slots[SOUND_MAX_SLOTS];
    vec2s listener;
    vec2s half_view; // position of screen
} SoundManager;

bool SoundManagerTryCreate(SoundManager *result);
void SoundManagerDestroy(const SoundManager *sound_manager);

END_DECLARATIONS