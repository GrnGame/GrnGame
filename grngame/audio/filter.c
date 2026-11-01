#include "filter.h"
#include "grngame/math/math.h"
#include "grngame/math/types.h"
#include "grngame/utils/clear.h"
#include <SDL3/SDL.h>
#include <stdlib.h>

typedef struct
{
    FilterType type;
    float32 wet;
    float32 amount;
    float32 coeff;
    float32 *memory;
    int32 length;
    int32 pos;
} FilterNode;

struct FilterChain
{
    FilterNode *nodes;
    int32 count;
    int32 channels;
};

FilterDef FilterEcho(float32 delay, float32 decay, float32 wet)
{
    FilterDef def = {.type = FILTER_ECHO};
    def.echo.delay = delay;
    def.echo.decay = decay;
    def.echo.wet = wet;
    return def;
}

FilterDef FilterBassboost(float32 boost)
{
    FilterDef def = {.type = FILTER_BASSBOOST};
    def.bassboost.boost = boost;
    return def;
}

static void NodeInitEcho(FilterNode *node, const FilterDef *def, const SDL_AudioSpec *spec)
{
    float32 delay = CLAMP(def->echo.delay, ECHO_DELAY_MIN, ECHO_DELAY_MAX);
    node->amount = CLAMP(def->echo.decay, 0.0f, ECHO_DECAY_MAX);
    node->length = (int32)(delay * spec->freq);

    node->wet = def->echo.wet;

    node->memory = malloc((uint32)node->length * spec->channels * sizeof(float32));
    CLEAR_ARRAY(node->memory, 0, node->length * spec->channels);
}

static void NodeInitBass(FilterNode *node, const FilterDef *def, const SDL_AudioSpec *spec)
{
    node->amount = def->bassboost.boost;
    // bass formule
    node->coeff = 1.0f - EXP(-2.0f * pi * BASS_CUTOFF_HZ / (float32)spec->freq);
    node->memory = malloc(spec->channels * sizeof(float32));
    CLEAR_ARRAY(node->memory, 0, spec->channels);
}

static void NodeInit(FilterNode *node, const FilterDef *def, const SDL_AudioSpec *spec)
{
    node->type = def->type;
    switch (def->type)
    {
    case FILTER_ECHO:
        NodeInitEcho(node, def, spec);
        break;
    case FILTER_BASSBOOST:
        NodeInitBass(node, def, spec);
        break;
    }
}

static void NodeFree(FilterNode *node)
{
    free(node->memory);
}

static void NodeProcessEcho(FilterNode *node, float32 *pcm, int32 frames, int32 channels)
{
    for (int32 i = 0; i < frames; i++)
    {
        float32 *tap = &node->memory[node->pos * channels];
        for (int32 c = 0; c < channels; c++)
        {
            float32 in = pcm[i * channels + c];
            pcm[i * channels + c] = in + tap[c] * node->wet;
            tap[c] = in + tap[c] * node->amount;
        }
        node->pos = (node->pos + 1) % node->length;
    }
}

static void NodeProcessBass(FilterNode *node, float32 *pcm, int32 frames, int32 channels)
{
    for (int32 i = 0; i < frames; i++)
    {
        for (int32 c = 0; c < channels; c++)
        {
            float32 in = pcm[i * channels + c];
            node->memory[c] += node->coeff * (in - node->memory[c]);
            pcm[i * channels + c] = in + node->memory[c] * node->amount;
        }
    }
}

static void NodeProcess(FilterNode *node, float32 *pcm, int32 frames, int32 channels)
{
    switch (node->type)
    {
    case FILTER_ECHO:
        NodeProcessEcho(node, pcm, frames, channels);
        break;
    case FILTER_BASSBOOST:
        NodeProcessBass(node, pcm, frames, channels);
        break;
    }
}

FilterChain *FilterChainCreate(const FilterDef *defs, int32 count, const SDL_AudioSpec *spec)
{
    if (!defs || count <= 0 || !spec)
        return NULL;

    FilterChain *chain = malloc(sizeof(FilterChain));
    CLEAR_PTR(chain, 0);

    chain->nodes = malloc(count * sizeof(FilterNode));
    CLEAR_ARRAY(chain->nodes, 0, count);

    chain->channels = spec->channels;

    for (int32 i = 0; i < count; i++)
    {
        NodeInit(&chain->nodes[i], &defs[i], spec);
        chain->count++;
    }
    return chain;
}

void FilterChainDestroy(FilterChain *chain)
{
    if (!chain)
        return;
    for (int32 i = 0; i < chain->count; i++)
        NodeFree(&chain->nodes[i]);
    free(chain->nodes);
    free(chain);
}

void FilterChainProcess(FilterChain *chain, float32 *pcm, int32 samples, int32 channels)
{
    if (channels <= 0 || channels > chain->channels)
        return;

    int32 frames = samples / channels;
    for (int32 i = 0; i < chain->count; i++)
        NodeProcess(&chain->nodes[i], pcm, frames, channels);
}