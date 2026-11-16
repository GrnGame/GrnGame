#include "sound_info.h"
#include <math.h>

SoundInfo SoundInfoDefault()
{
    SoundInfo info = {0};
    info.volume = 1.0f;
    info.pitch = 1.0f;
    info.position = (vec2s){.x=NAN, .y=NAN};
    return info;
}

SoundInfo SoundInfoPositional(float32 x, float32 y)
{
    SoundInfo info = SoundInfoDefault();
    info.position = (vec2s){.x = x, .y = y};
    return info;
}

SoundInfo SoundInfoMusic()
{
    SoundInfo info = SoundInfoDefault();
    info.looping = true;
    info.fade_in = 1.0f;
    return info;
}