#pragma once
#include "grngame/math/types.h"
#include "grngame/utils/macro.h"
#include "kvec.h"

static inline float32 Math_Lerp(float32 a, float32 b, float32 t)
{
    return a + (b - a) * t;
}

static inline int32 Math_LerpInt(int32 a, int32 b, float32 t)
{
    return a + (int32)((float32)(b - a) * t);
}

#define EXP(x) SDL_expf(x)
#define CLAMP(x, y, z) SDL_clamp(x, y, z)

static inline float64 Average(float64_vec_t vec)
{
    float64 sum = 0;
    for (uint32 i = 0; i < (uint32)kv_size(vec); i++)
    {
        sum += kv_A(vec, i);
    }
    return sum / (uint32)kv_size(vec);
}

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define DEG2RAD(x) ((x) * pi / 180.0)
#define RAD2DEG(x) ((x) * 180.0 / pi)

#define MB (1024 * 1024)