#pragma once

#include "SDL3/SDL_pixels.h"
#include "grngame/math/types.h"
#include "grngame/utils/attributes.h"

typedef struct PaletteManager PaletteManager;

typedef struct
{
    float32 x, y, z;
} ColorXYZ;

typedef struct
{
    float32 l, a, b;
} ColorLAB;

void InitLinearLut();
HOT ColorLAB RgbToLab(SDL_Color *color);
int32 FindBestPaletteColorCIEDE2000(PaletteManager *manager, ColorLAB *target_pixel);
