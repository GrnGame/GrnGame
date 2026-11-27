#pragma once

#include "../assets/asset_manager.h"
#include "../math/types.h"
#include "grngame/utils/c_cpp.h"

BEGIN_DECLARATIONS

typedef struct
{
    uint16 w;
    uint16 h;
#ifndef GRNGAME_EMBED_ASSETS
    Texture tex; // for optimisation
#else
    TextureMemory tex;
#endif
    bool find;
    const char *name;
} Sprite;

bool SpriteDraw(Sprite s, uint16 frame, float32 x, float32 y, uint8 c, int16 r, uint8 a);

END_DECLARATIONS