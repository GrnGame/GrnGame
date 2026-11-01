#include "primitive.h"
#include "../utils/attributes.h"
#include "grngame/core/app.h"
#include "grngame/math/math.h"
#include "grngame/math/types.h"
#include "kvec.h"
#include "renderer.h"
#include <math.h>

static inline void SetColorFromPalette(int32 c_idx, uint8 a)
{
    int32 palette_size = (int32)kv_size(g_app.palette_manager.palette_elements);

    int32 safe_c = CLAMP(c_idx, 0, palette_size - 1);
    SDL_Color base_color = kv_A(g_app.palette_manager.palette_elements, safe_c);

    RendererSetColor(base_color.r, base_color.g, base_color.b, a);
}

static inline SDL_FRect make_rect(float32 x, float32 y, float32 w, float32 h)
{
    return (SDL_FRect){PIXEL_ALIGN(x + g_app.info.offset_x), PIXEL_ALIGN(y + g_app.info.offset_y), PIXEL_ALIGN(w),
                       PIXEL_ALIGN(h)};
}

void PixelDraw(float32 x, float32 y, int32 c_idx, uint8 a)
{
    SetColorFromPalette(c_idx, a);
    SDL_FRect rect = make_rect(x, y, 1.0f, 1.0f);
    RendererFillRect(&rect);
}

void LineDraw(float32 x0, float32 y0, float32 x1, float32 y1, int32 c_idx, uint8 a)
{
    SetColorFromPalette(c_idx, a);

    float32 dx = fabsf(x1 - x0);
    float32 dy = fabsf(y1 - y0);
    int32 n = (int32)((dx > dy ? dx : dy) + 1.0f);

    SDL_FRect *rects = STACK_ALLOC(SDL_FRect, n);

    float32 x = x0;
    float32 y = y0;
    float32 sx = x0 < x1 ? 1.0f : -1.0f;
    float32 sy = y0 < y1 ? 1.0f : -1.0f;
    float32 err = (dx > dy ? dx : -dy) / 2.0f;

    for (int32 i = 0; i < n; i++)
    {
        rects[i] = make_rect(x, y, 1.0f, 1.0f);
        float32 e2 = err;
        if (e2 > -dx)
        {
            err -= dy;
            x += sx;
        }
        if (e2 < dy)
        {
            err += dx;
            y += sy;
        }
    }

    RendererFillRects(rects, n);
}

void RectDraw(float32 x, float32 y, float32 w, float32 h, int32 c_idx, uint8 a)
{
    SetColorFromPalette(c_idx, a);
    SDL_FRect rect = make_rect(x, y, w, h);
    RendererRect(&rect);
}

void RectDrawFill(float32 x, float32 y, float32 w, float32 h, int32 c_idx, uint8 a)
{
    SetColorFromPalette(c_idx, a);
    SDL_FRect rect = make_rect(x, y, w, h);
    RendererFillRect(&rect);
}

void CircleDraw(float32 xc, float32 yc, float32 radius, int32 c_idx, uint8 a)
{
    SetColorFromPalette(c_idx, a);

    int32 rad = (int32)radius;
    int32 n = 2 + (rad * 4);

    SDL_FRect *rects = STACK_ALLOC(SDL_FRect, n);

    int32 count = 0;
    for (int32 dy = 0; dy <= rad; dy++)
    {
        int32 dx = (int32)sqrtf((float32)(rad * rad - dy * dy));
        int32 dx_next = (dy + 1 <= rad) ? (int32)sqrtf((float32)(rad * rad - (dy + 1) * (dy + 1))) : 0;
        float32 thick = (float32)(dx - dx_next);
        if (thick < 1.0f)
            thick = 1.0f;

        float32 left = xc - (float32)dx;
        float32 right = xc + (float32)dx - thick;

        if (dy == rad)
        {
            float32 w = (float32)(dx * 2);
            rects[count++] = make_rect(left, yc - (float32)dy, w, 1.0f);
            rects[count++] = make_rect(left, yc + (float32)dy, w, 1.0f);
        }
        else if (dy == 0)
        {
            rects[count++] = make_rect(left, yc, thick, 1.0f);
            rects[count++] = make_rect(right, yc, thick, 1.0f);
        }
        else
        {
            rects[count++] = make_rect(left, yc - (float32)dy, thick, 1.0f);
            rects[count++] = make_rect(right, yc - (float32)dy, thick, 1.0f);
            rects[count++] = make_rect(left, yc + (float32)dy, thick, 1.0f);
            rects[count++] = make_rect(right, yc + (float32)dy, thick, 1.0f);
        }
    }

    RendererFillRects(rects, count);
}

void CircleDrawFill(float32 xc, float32 yc, float32 radius, int32 c_idx, uint8 a)
{
    SetColorFromPalette(c_idx, a);

    int32 rad = (int32)radius;
    int32 n = 1 + (rad * 2);

    SDL_FRect *rects = STACK_ALLOC(SDL_FRect, n);

    int32 count = 0;
    for (int32 dy = 0; dy <= rad; dy++)
    {
        int32 dx = (int32)sqrtf((float32)(rad * rad - dy * dy));
        float32 w = (float32)(dx * 2);
        float32 xs = xc - (float32)dx;

        if (dy == 0)
        {
            rects[count++] = make_rect(xs, yc, w, 1.0f);
        }
        else
        {
            rects[count++] = make_rect(xs, yc - (float32)dy, w, 1.0f);
            rects[count++] = make_rect(xs, yc + (float32)dy, w, 1.0f);
        }
    }

    RendererFillRects(rects, count);
}
