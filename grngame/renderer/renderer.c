#include "renderer.h"
#include "../math/types.h"
#include "SDL3/SDL_error.h"
#include "grngame/core/app.h"
#include "grngame/data/file.h"
#include "grngame/dev/logging.h"
#include "grngame/utils/attributes.h"
#include <math.h>
#include <stdbool.h>

bool RendererTryCreate(SDL_Window *window, Renderer *renderer)
{
    renderer->renderer = SDL_CreateRenderer(window, NULL);
    if (UNLIKELY(!renderer->renderer))
    {
        LOG_ERROR("Failed to create renderer: %s", SDL_GetError());
        return false;
    }

    SDL_SetRenderVSync(renderer->renderer, 0);

    LOG_INFO("Renderer initialized. Driver=\"%s\" Renderer=\"%s\"", SDL_GetCurrentVideoDriver(),
             SDL_GetRendererName(renderer->renderer));

    return true;
}

bool RendererClear(const Renderer *renderer)
{
    RendererSetColor(255, 255, 255, 255);
    if (UNLIKELY(!SDL_RenderClear(renderer->renderer)))
    {

        LOG_ERROR("Failed to clear renderer: %s", SDL_GetError());
        return false;
    }
    return true;
}

bool RendererPresent(const Renderer *renderer)
{
    if (UNLIKELY(!SDL_RenderPresent(renderer->renderer)))
    {
        LOG_ERROR("Failed to present renderer: %s", SDL_GetError());
        return false;
    }

    return true;
}

bool RendererSetColor(uint8 r, uint8 g, uint8 b, uint8 a)
{
    if (UNLIKELY(!SDL_SetRenderDrawBlendMode(g_app.renderer.renderer, SDL_BLENDMODE_BLEND)))
    {
        LOG_ERROR("Failed to set blend mode: %s", SDL_GetError());
        return false;
    }
    if (UNLIKELY(!SDL_SetRenderDrawColor(g_app.renderer.renderer, r, g, b, a)))
    {
        LOG_ERROR("Failed to set draw color: %s", SDL_GetError());
        return false;
    }
    return true;
}
bool RendererFillRect(const SDL_FRect *restrict rect)
{
    if (UNLIKELY(!SDL_RenderFillRect(g_app.renderer.renderer, rect)))
    {
        LOG_ERROR("Failed to fill rect: %s", SDL_GetError());
        return false;
    }
    return true;
}

bool RendererFillRects(const SDL_FRect *restrict rects, int32 count)
{
    if (UNLIKELY(!SDL_RenderFillRects(g_app.renderer.renderer, rects, count)))
    {
        LOG_ERROR("Failed to fill rects: %s", SDL_GetError());
        return false;
    }
    return true;
}
bool RendererRect(const SDL_FRect *restrict rect)
{
    if (UNLIKELY(!SDL_RenderRect(g_app.renderer.renderer, rect)))
    {
        LOG_ERROR("Failed to draw rect: %s", SDL_GetError());
        return false;
    }
    return true;
}
bool RendererTextureRotated(SDL_Texture *restrict texture, const SDL_FRect *restrict src, const SDL_FRect *restrict dst,
                            float64 angle, const SDL_FPoint *restrict center, SDL_FlipMode flip)
{
    if (UNLIKELY(!SDL_RenderTextureRotated(g_app.renderer.renderer, texture, src, dst, angle, center, flip)))
    {
        LOG_ERROR("Failed to render texture: %s", SDL_GetError());
        return false;
    }
    return true;
}
bool RendererSetTextureAlpha(SDL_Texture *restrict texture, uint8 a)
{
    if (UNLIKELY(!SDL_SetTextureAlphaMod(texture, a)))
    {
        LOG_ERROR("Failed to set texture alpha: %s", SDL_GetError());
        return false;
    }
    return true;
}

bool OffScreen(float32 x, float32 y, float32 w, float32 h)
{
    float32 view_w = (float32)g_app.info.window_universe_width;
    float32 view_h = (float32)g_app.info.window_universe_height;

    if (g_app.info.force_universe_scale)
        return (x + w <= 0.0f) || (x >= view_w) || (y + h <= 0.0f) || (y >= view_h);

    float32 off_x = g_app.info.offset_x;
    float32 off_y = g_app.info.offset_y;
    return (x + w <= -off_x) || (x >= view_w + off_x) || (y + h <= -off_y) || (y >= view_h + off_y);
}

void SetPixelArtFilter(SDL_Texture *tex)
{
    if (tex != NULL && !SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST))
        LOG_WARNING("Cant set nearest scale mode: %s", SDL_GetError());
}