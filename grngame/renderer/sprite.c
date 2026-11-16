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

    Texture tex = s.tex;

    int16 tex_w = tex.w;

    if (OffScreen(x, y, (float32)(tex_w * c), (float32)(tex.h * c)))
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

    SDL_FRect src = {.x = (float32)((frame % cols) * s.w),
                     .y = (float32)((frame / cols) * s.h),
                     .w = (float32)s.w,
                     .h = (float32)s.h};
    SDL_FRect dst = {.x = PIXEL_ALIGN(x + g_app.info.offset_x),
                     .y = PIXEL_ALIGN(y + g_app.info.offset_y),
                     .w = (float32)(s.w * c),
                     .h = (float32)(s.h * c)};
    SDL_FPoint center = {.x = dst.w / 2.0f, .y = dst.h / 2.0f};

    RendererSetTextureAlpha(tex.texture, a);
    if (r == 0)
    {

        SDL_RenderTexture(g_app.renderer.renderer, tex.texture, &src, &dst);
    }
    else
    {
        RendererTextureRotated(tex.texture, &src, &dst, (float64)r, &center, SDL_FLIP_NONE);
    }

    return true;
}