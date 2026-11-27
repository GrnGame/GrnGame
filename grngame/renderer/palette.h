#pragma once

#include "cielab.h"
#include "grngame/utils/c_cpp.h"
#include "kvec.h"
#include "grngame/utils/macro.h"

#include <SDL3/SDL_pixels.h>
#include <khash.h>

BEGIN_DECLARATIONS

KHASH_MAP_INIT_STR(ColorHex, SDL_Color);

typedef struct PaletteManager
{
    kvec_t(SDL_Color) palette_elements;
    kvec_t(ColorLAB) palette_elements_lab;
    khash_t(ColorHex) * palette_hex_hash;
} PaletteManager;

PaletteManager PaletteManagerCreate(void);
void PaletteManagerDestroy(PaletteManager *manager);
void PaletteFreeStringVec(string_vec_t *vec);

void PaletteAddColor(PaletteManager *manager, SDL_Color color);
SDL_Color ColorFromHex(PaletteManager *manager, const char *hex);
void PaletteParse(PaletteManager *manager, string_vec_t *text);

END_DECLARATIONS
