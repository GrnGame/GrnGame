#include "window.h"
#include "../math/types.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_video.h"
#include "app.h"
#include "cglm/types-struct.h"
#include "grngame/audio/sound.h"
#include "grngame/renderer/renderer.h"
#include "grngame/utils/attributes.h"
#include <math.h>

static void SetFullscreen(SDL_Window *window, bool fullscreen)
{
    if (UNLIKELY(!SDL_SetWindowFullscreen(window, fullscreen)))
        LOG_ERROR("Failed to set fullscreen: %s", SDL_GetError());
}

static void SetBordered(SDL_Window *window, bool bordered)
{
    if (UNLIKELY(!SDL_SetWindowBordered(window, bordered)))
        LOG_ERROR("Failed to set bordered: %s", SDL_GetError());
}

static void SetResizable(SDL_Window *window, bool resizable)
{
    if (UNLIKELY(!SDL_SetWindowResizable(window, resizable)))
        LOG_ERROR("Failed to set resizable: %s", SDL_GetError());
}

static void Maximize(SDL_Window *window)
{
    if (UNLIKELY(!SDL_MaximizeWindow(window)))
        LOG_ERROR("Failed to maximize window: %s", SDL_GetError());
}

static void SetSize(SDL_Window *window, float32 width, float32 height)
{
    if (UNLIKELY(!SDL_SetWindowSize(window, (int32)width, (int32)height)))
        LOG_ERROR("Failed to set window size: %s", SDL_GetError());
}

static void SetPosition(SDL_Window *window, int32 x, int32 y)
{
#ifdef GRNGAME_DESKTOP
    if (UNLIKELY(!SDL_SetWindowPosition(window, x, y)))
        LOG_ERROR("Failed to set window position: %s", SDL_GetError());
#endif
}

SDL_Window *WindowCreate(const AppInfo *app_info)
{
    SDL_WindowFlags flags = SDL_WINDOW_HIDDEN;
    if (app_info->window_fullscreen)
        flags |= SDL_WINDOW_FULLSCREEN;
    if (app_info->window_resizable)
        flags |= SDL_WINDOW_RESIZABLE;
    SDL_Window *window =
        SDL_CreateWindow(app_info->name, (int32)app_info->window_width, (int32)app_info->window_height, flags);
    if (!window)
        LOG_ERROR("Failed to create window: %s", SDL_GetError());
    return window;
}

bool SetAppMetadata(AppInfo *app_info, const char *appname, const char *appversion, const char *appidentifier)
{
    if (g_app.window)
        SDL_SetWindowTitle(g_app.window, appname);
    if (UNLIKELY(!SDL_SetAppMetadata(appname, appversion, appidentifier)))
    {
        LOG_ERROR("Impossible to set AppMetaData : %s", SDL_GetError());
        return false;
    }

    app_info->name = (char *)appname;
    app_info->version = (char*)appversion;

    return true;
}

void WindowInfoSetName(AppInfo *app_info, const char *name)
{
    app_info->name =(char*) name;
}

void WindowInfoSetVersion(AppInfo *app_info, const char *version)
{
    app_info->version = (char*)version;
}

void WindowInfoSetSize(AppInfo *app_info, float32 width, float32 height)
{
    app_info->window_width = width;
    app_info->window_height = height;
}

void WindowInfoSetUniverseSize(AppInfo *app_info, uint16 width, uint16 height)
{
    app_info->window_universe_width = width;
    app_info->window_universe_height = height;
}

void WindowInfoSetFullscreen(AppInfo *app_info, bool fullscreen)
{
    app_info->window_fullscreen = fullscreen;
}

void WindowInfoSetMaximised(AppInfo *app_info, bool maximised)
{
    app_info->window_maximised = maximised;
}

void WindowInfoSetResizable(AppInfo *app_info, bool resizable)
{
    app_info->window_resizable = resizable;
}

void WindowInfoSetBordered(AppInfo *app_info, bool bordered)
{
    app_info->bordered = bordered;
}

void WindowInfoSetForceUniverseScale(AppInfo *app_info, bool force)
{
    app_info->force_universe_scale = force;
}

bool WindowConfigureScale(uint8 scalex, uint8 scaley)
{
    return SDL_SetRenderScale(g_app.renderer.renderer, (float32)scalex, (float32)scaley);
}

uint8 WindowGetScale()
{
    float32 c;
    SDL_GetRenderScale(g_app.renderer.renderer, &c, &c);
    return (uint8)c;
}

float32 WindowGetWidth()
{
    return g_app.info.window_width;
}

float32 WindowGetHeight()
{
    return g_app.info.window_height;
}

int32 WindowGetUniverseWidth()
{
    return (int32)g_app.info.window_universe_width;
}

int32 WindowGetUniverseHeight()
{
    return (int32)g_app.info.window_universe_height;
}

float32 WindowGetOffsetX()
{
    return g_app.info.offset_x;
}

float32 WindowGetOffsetY()
{
    return g_app.info.offset_y;
}

bool WindowGetChange()
{
    return g_app.info.window_change;
}

ivec2s WindowDimensions(SDL_Window *window)
{
    ivec2s result;
    if (!SDL_GetWindowSize(window, &result.x, &result.y))
        LOG_ERROR("Failed to get window dimensions: %s", SDL_GetError());
    return result;
}

void ApplyResizing(AppInfo *app_info, float32 width, float32 height)
{
    int8 coeff_w = (int8)(width / (float32)app_info->window_universe_width);
    int8 coeff_h = (int8)(height / (float32)app_info->window_universe_height);
    int8 coeff = (coeff_w < coeff_h) ? coeff_w : coeff_h;
    if (coeff < 1)
        coeff = 1;

    float32 real_w = (float32)app_info->window_universe_width * (float32)coeff;
    float32 real_h = (float32)app_info->window_universe_height * (float32)coeff;

    float32 offset_x = PIXEL_ALIGN((width - real_w) / 2.0f / (float32)coeff);
    float32 offset_y = PIXEL_ALIGN((height - real_h) / 2.0f / (float32)coeff);

    if (offset_x < 0.0f)
        offset_x = 0.0f;
    if (offset_y < 0.0f)
        offset_y = 0.0f;

    app_info->window_width = real_w;
    app_info->window_height = real_h;
    app_info->offset_x = offset_x;
    app_info->offset_y = offset_y;

    WindowConfigureScale(coeff, coeff);

    g_app.info.window_change = true;
}

void ApplyBlackStripes()
{
    if (!g_app.info.force_universe_scale)
        return;

    const float32 off_x = PIXEL_ALIGN(g_app.info.offset_x);
    const float32 off_y = PIXEL_ALIGN(g_app.info.offset_y);
    const float32 uw = (float32)g_app.info.window_universe_width;
    const float32 uh = (float32)g_app.info.window_universe_height;
    const float32 big = 4096.0f;

    SDL_FRect rects[4] = {
        {-big, -big, big + off_x, 2.0f * big},
        {off_x + uw, -big, big, 2.0f * big},
        {-big, -big, 2.0f * big, big + off_y},
        {-big, off_y + uh, 2.0f * big, big},
    };

    RendererSetColor(10, 10, 10, 255);
    RendererFillRects(rects, 4);
}

void WindowApplyConfig(AppInfo *app_info)
{
    SDL_Window *window = g_app.window;
    SetAppMetadata(app_info, app_info->name, app_info->version, app_info->name);
    SetBordered(window, app_info->bordered);
    SetResizable(window, app_info->window_resizable);

    if (app_info->window_fullscreen)
    {
        SetFullscreen(window, true);
        ivec2s vec = WindowDimensions(window);
        ApplyResizing(app_info, (float32)vec.x, (float32)vec.y);
        app_info->window_maximised = false;
    }
    else if (app_info->window_maximised)
    {
        SetFullscreen(window, false);
        Maximize(window);
        ivec2s vec = WindowDimensions(window);
        ApplyResizing(app_info, (float32)vec.x, (float32)vec.y);
    }
    else
    {
        SDL_DisplayID display_id = SDL_GetDisplayForWindow(window);
        if (display_id == 0)
        {
            LOG_ERROR("Impossible to get display information: %s", SDL_GetError());
            return;
        }
        SDL_Rect bounds;
        if (!SDL_GetDisplayUsableBounds(display_id, &bounds))
        {
            LOG_ERROR("Impossible to get usable display bounds: %s", SDL_GetError());
            return;
        }
        SetFullscreen(window, false);
        SetSize(window, app_info->window_width, app_info->window_height);
        SetPosition(window, bounds.x + (bounds.w - (int32)app_info->window_width) / 2,
                    bounds.y + (bounds.h - (int32)app_info->window_height) / 2);
        ApplyResizing(app_info, app_info->window_width, app_info->window_height);
    }

    SetSoundViewSize(app_info->window_universe_width, app_info->window_universe_height);
}