#include "sprite.h"
#include "SDL3/SDL_surface.h"
#include "grngame/core/app.h"
#include "grngame/dev/logging.h"
#include "renderer.h"
#include "texture.h"
#include <math.h>

bool SpriteDraw(Sprite s, uint16 frame, float32 x, float32 y, uint8 c, int16 r, uint8 a)
{
    bool find = false;
    if (!s.find)
    {
        s.tex = FindImage(s.name, &find);

        if (!find)
        {
            LOG_WARNING("Impossible to get texture: %s,default is set to 16x16", s.name);
            return false;
        }
    }

    SDL_Texture *sdl_tex = NULL;
    int16 tex_w = 0;
    int16 tex_h = 0;
    float32 base_x = 0.0f;
    float32 base_y = 0.0f;

#ifndef GRNGAME_EMBED_ASSETS
    sdl_tex = s.tex.texture;
    tex_w = (int16)s.tex.w;
    tex_h = (int16)s.tex.h;
#else
    sdl_tex = g_app.asset_manager.atlas.texture;
    tex_w = (int16)s.tex.size.x;
    tex_h = (int16)s.tex.size.y;
    base_x = s.tex.positions.x;
    base_y = s.tex.positions.y;
#endif

    if (OffScreen(x, y, (float32)(tex_w * c), (float32)(tex_h * c)))
    {
        // LOG_DEBUG("Cant draw %s because it offscreen in :%f,%f", s.name, x, y);
        return true;
    }

    int16 cols = (int16)(tex_w / (int16)s.w);
    if (cols <= 0)
    {
        LOG_WARNING("Impossible to get case : %d ", frame);
        return false;
    }

    SDL_FRect src = {.x = base_x + (float32)((frame % cols) * s.w),
                     .y = base_y + (float32)((frame / cols) * s.h),
                     .w = (float32)s.w,
                     .h = (float32)s.h};
    SDL_FRect dst = {.x = PIXEL_ALIGN(x + g_app.info.offset_x),
                     .y = PIXEL_ALIGN(y + g_app.info.offset_y),
                     .w = (float32)(s.w * c),
                     .h = (float32)(s.h * c)};
    SDL_FPoint center = {.x = dst.w / 2.0f, .y = dst.h / 2.0f};

    RendererSetTextureAlpha(sdl_tex, a);
    if (r == 0)
    {
        SDL_RenderTexture(g_app.renderer.renderer, sdl_tex, &src, &dst);
    }
    else
    {
        RendererTextureRotated(sdl_tex, &src, &dst, (float64)r, &center, SDL_FLIP_NONE);
    }

    return true;
}