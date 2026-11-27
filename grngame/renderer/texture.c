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
    SDL_Texture *sdl_tex = NULL;
    SDL_FRect src = {0};
    const SDL_FRect *src_ptr = NULL;
    float32 w = 0.0f;
    float32 h = 0.0f;

#ifndef GRNGAME_EMBED_ASSETS
    Texture tex = FindImage(name, &find);
    if (!find)
    {
        LOG_WARNING("Impossible to get texture: %s,default is set to 16x16", name);
        return false;
    }

    sdl_tex = tex.texture;
    w = (float32)tex.w;
    h = (float32)tex.h;
#else
    TextureMemory tex = FindImage(name, &find);
    if (!find)
    {
        LOG_WARNING("Impossible to get texture: %s,default is set to 16x16", name);
        return false;
    }

    sdl_tex = g_app.asset_manager.atlas.texture;
    src = (SDL_FRect){.x = tex.positions.x, .y = tex.positions.y, .w = tex.size.x, .h = tex.size.y};
    src_ptr = &src;
    w = tex.size.x;
    h = tex.size.y;
#endif

    if (OffScreen(x, y, w * c, h * c))
    {
        // LOG_DEBUG("Cant draw %s because it offscreen in :%f,%f", name, x, y);
        return true;
    }

    SDL_FRect dst = {
        .x = PIXEL_ALIGN(x + g_app.info.offset_x), .y = PIXEL_ALIGN(y + g_app.info.offset_y), .w = w * c, .h = h * c};
    SDL_FPoint center = {.x = dst.w / 2.0f, .y = dst.h / 2.0f};

    RendererSetTextureAlpha(sdl_tex, a);
    if (r == 0)
    {
        SDL_RenderTexture(g_app.renderer.renderer, sdl_tex, src_ptr, &dst);
    }
    else
    {
        RendererTextureRotated(sdl_tex, src_ptr, &dst, (float64)r, &center, SDL_FLIP_NONE);
    }

    return true;
}

HOT ivec2s TextureGetSize(const char *name)
{
    bool find = false;
#ifndef GRNGAME_EMBED_ASSETS
    Texture tex = FindImage(name, &find);
    if (!find)
    {
        LOG_WARNING("Impossible to get texture: %s,size will be 16x16", name);
        return (ivec2s){.raw = {0, 0}};
    }

    return (ivec2s){.raw = {(int32)tex.w, (int32)tex.h}};
#else
    TextureMemory tex = FindImage(name, &find);
    if (!find)
    {
        LOG_WARNING("Impossible to get texture: %s,size will be 16x16", name);
        return (ivec2s){.raw = {0, 0}};
    }

    return (ivec2s){.raw = {(int32)tex.size.x, (int32)tex.size.y}};
#endif
}