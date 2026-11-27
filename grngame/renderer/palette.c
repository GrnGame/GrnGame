#include "palette.h"
#include "SDL3/SDL_pixels.h"
#include "cielab.h"
#include "grngame/assets/load.h"
#include "grngame/core/app.h"
#include "grngame/dev/logging.h"
#include "kvec.h"
#include <grngame/utils/string_compat.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>

// todo: move
void PaletteFreeStringVec(string_vec_t *vec)
{
    if (UNLIKELY(vec == NULL))
        return;

    for (uint32 i = 0; i < (uint32)kv_size(*vec); i++)
    {
        free(kv_A(*vec, i));
    }

    kv_destroy(*vec);
}

PaletteManager PaletteManagerCreate()
{
    PaletteManager manager = {0};

    kv_init(manager.palette_elements);
    kv_init(manager.palette_elements_lab);
    manager.palette_hex_hash = kh_init(ColorHex);

    return manager;
}

void PaletteManagerDestroy(PaletteManager *manager)
{
    if (UNLIKELY(manager == NULL))
        return;

    kv_destroy(manager->palette_elements);
    kv_destroy(manager->palette_elements_lab);

    if (manager->palette_hex_hash != NULL)
    {
        kh_destroy(ColorHex, manager->palette_hex_hash);
        manager->palette_hex_hash = NULL;
    }
}

void PaletteAddColor(PaletteManager *manager, SDL_Color color)
{
    kv_push(SDL_Color, manager->palette_elements, color);
    kv_push(ColorLAB, manager->palette_elements_lab, RgbToLab(&color));
}

static Uint8 HexToU8(const char *hex)
{
    char buf[3] = {hex[0], hex[1], '\0'};

    return (Uint8)strtol(buf, NULL, 16);
}

SDL_Color ColorFromHex(PaletteManager *manager, const char *hex)
{
    SDL_Color color = {255, 255, 255, 255};

    if (UNLIKELY(hex == NULL))
        return color;

    if (hex[0] == '#')
        hex++;

    if (UNLIKELY((uint32)strlen(hex) < 6))
    {
        LOG_ERROR("Invalid color hex: %s", hex);
        return color;
    }

    khash_t(ColorHex) *h = manager->palette_hex_hash;

    khiter_t k = kh_get(ColorHex, h, hex);

    if (k != kh_end(h))
    {
        return kh_value(h, k);
    }

    color.r = HexToU8(hex);
    color.g = HexToU8(hex + 2);
    color.b = HexToU8(hex + 4);
    color.a = 255;

    int32 ret;
    k = kh_put(ColorHex, h, hex, &ret);
    (void)ret;

    kh_value(h, k) = color;

    return color;
}

void PaletteParse(PaletteManager *manager, string_vec_t *text)
{
    if (kv_size(*text) == 0)
    {
        LOG_INFO("No color palette detected, set to black and white");

        PaletteAddColor(manager, (SDL_Color){.r = 0, .g = 0, .b = 0, .a = 255});

        PaletteAddColor(manager, (SDL_Color){.r = 255, .g = 255, .b = 255, .a = 255});

        return;
    }

    for (uint32 i = 0; i < (uint32)kv_size(*text); i++)
    {
        const char *color = kv_A(*text, i);

        if (LIKELY(color != NULL))
        {
            PaletteAddColor(manager, ColorFromHex(manager, color));
        }
    }
}