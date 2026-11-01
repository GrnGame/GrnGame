#include "grngame/math/types.h"
#include "grngame/utils/attributes.h"
#include <SDL3/SDL.h>

float64 TimeNow(void)
{
    static uint32 freq = 0;

    if (freq == 0)
    {
        freq = SDL_GetPerformanceFrequency();

        if (UNLIKELY(freq == 0))
            return 0.0;
    }

    return (float64)SDL_GetPerformanceCounter() / (float64)freq;
}