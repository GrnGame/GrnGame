#pragma once
#include "grngame/math/types.h"
#include "grngame/utils/macro.h"
#include "kvec.h"

static inline float64 Average(float64_vec_t vec)
{
    float64 sum = 0;
    for (uint32 i = 0; i < (uint32)kv_size(vec); i++)
    {
        sum += kv_A(vec, i);
    }
    return sum / (uint32)kv_size(vec);
}

#define MB (1024.0f * 1024.0f)