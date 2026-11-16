#pragma once

#include "grngame/math/types.h"
#include "grngame/utils/c_cpp.h"
#include <SDL3/SDL_audio.h>

BEGIN_DECLARATIONS

#define MAX_FILTERS 4

#define BASS_CUTOFF_HZ 150.0f
#define ECHO_DELAY_MIN 0.01f
#define ECHO_DELAY_MAX 2.0f
#define ECHO_DECAY_MAX 0.95f

typedef enum
{
    FILTER_ECHO,
    FILTER_BASSBOOST
} FilterType;

typedef struct
{
    FilterType type;
    union {
        struct
        {
            float32 delay, decay, wet;
        } echo;
        struct
        {
            float32 boost;
        } bassboost;
    };
} FilterDef;

typedef struct FilterChain FilterChain;

FilterDef FilterEcho(float32 delay, float32 decay, float32 wet);
FilterDef FilterBassboost(float32 boost);

FilterChain *FilterChainCreate(const FilterDef *defs, int32 count, const SDL_AudioSpec *spec);
void FilterChainDestroy(FilterChain *chain);
void FilterChainProcess(const FilterChain *chain, float32 *pcm, int32 samples, int32 channels);

END_DECLARATIONS