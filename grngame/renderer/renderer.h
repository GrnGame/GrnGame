#pragma once
#include "../math/types.h"
#include "grngame/utils/attributes.h"
#include <SDL3/SDL_render.h>

BEGIN_DECLARATIONS

#if PIXEL_INT_ONLY
#define PIXEL_ALIGN(x) floor((float64)x)
#else
#define PIXEL_ALIGN(x) (x)
#endif

typedef struct
{
    SDL_Renderer *renderer;
} Renderer;

bool RendererTryCreate(SDL_Window *window, Renderer *renderer);
void SetPixelArtFilter(SDL_Texture *tex);
bool RendererClear(const Renderer *renderer);
bool RendererPresent(const Renderer *renderer);
bool RendererSetColor(uint8 r, uint8 g, uint8 b, uint8 a);
bool RendererFillRect(const SDL_FRect *RESTRICT rect);
bool RendererFillRects(const SDL_FRect *RESTRICT rects, int32 count);
bool RendererRect(const SDL_FRect *RESTRICT rect);
bool RendererTextureRotated(SDL_Texture *RESTRICT texture, const SDL_FRect *RESTRICT src, const SDL_FRect *RESTRICT dst,
                            float64 angle, const SDL_FPoint *RESTRICT center, SDL_FlipMode flip);
bool RendererSetTextureAlpha(SDL_Texture *RESTRICT texture, uint8 a);
bool OffScreen(float32 x, float32 y, float32 w, float32 h);

END_DECLARATIONS
