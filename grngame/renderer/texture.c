#include "texture.h"
#include "SDL3/SDL_surface.h"
#include "cglm/types-struct.h"
#include "grngame/assets/asset_manager.h"
#include "grngame/core/app.h"
#include "grngame/dev/logging.h"
#include "grngame/math/types.h"
#include "grngame/utils/attributes.h"
#include "renderer.h"
#include <math.h>

bool TextureDraw(const char *name, float32 x, float32 y, uint8 c, int16 r, uint8 a)
{
    bool find = false;
    Texture tex = FindImage(name, &find);
    if (!find)
    {
        LOG_WARNING("Impossible to get texture: %s,default is set to 16x16", name);
        return false;
    }

    if (OffScreen(x, y, (float32)(tex.w * c), (float32)(tex.h * c)))
    {
        // LOG_DEBUG("Cant draw %s because it offscreen in :%f,%f", name, x, y);
        return true;
    }

    SDL_FRect dst = {PIXEL_ALIGN(x + g_app.info.offset_x), PIXEL_ALIGN(y + g_app.info.offset_y), (float32)(tex.w * c),
                     (float32)(tex.h * c)};

    SDL_FPoint center = {.x = dst.w / 2.0f, .y = dst.h / 2.0f};

    RendererSetTextureAlpha(tex.texture, a);
    if (r == 0)
    {

        SDL_RenderTexture(g_app.renderer.renderer, tex.texture, NULL, &dst);
    }
    else
    {
        RendererTextureRotated(tex.texture, NULL, &dst, (float64)r, &center, SDL_FLIP_NONE);
    }

    return true;
}

HOT ivec2s TextureGetSize(const char *name)
{
    bool find;
    Texture tex = FindImage(name, &find);
    if (!find)
    {
        LOG_WARNING("Impossible to get texture: %s,size will be 16x16", name);
        return (ivec2s){.raw = {0, 0}};
    }

    return (ivec2s){.raw = {(int32)tex.w, (int32)tex.h}};
}