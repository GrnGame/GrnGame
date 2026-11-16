#include "mouse.h"
#include "../core/app.h"
#include "SDL3/SDL_error.h"
#include "cglm/types-struct.h"
#include "grngame/core/window.h"
#include "grngame/dev/logging.h"
#include "grngame/math/types.h"
#include <SDL3/SDL_mouse.h>

vec2s GetMousePosition()
{
    float32 mx, my;
    SDL_GetMouseState(&mx, &my);
    return (vec2s){.raw = {mx, my}};
}

bool MouseLeftPressed()
{
    return g_app.input_manager.mouse.left_pressed;
}
bool MouseLeftJustPressed()
{
    return g_app.input_manager.mouse.left_just_pressed;
}
bool MouseLeftJustReleased()
{
    return g_app.input_manager.mouse.left_just_released;
}
bool MouseRightPressed()
{
    return g_app.input_manager.mouse.right_pressed;
}
bool MouseRightJustPressed()
{
    return g_app.input_manager.mouse.right_just_pressed;
}
bool MouseRightJustReleased()
{
    return g_app.input_manager.mouse.right_just_released;
}
int32 MouseX()
{
    return g_app.input_manager.mouse.x;
}
int32 MouseY()
{
    return g_app.input_manager.mouse.y;
}
float32 MouseScrollX()
{
    return g_app.input_manager.mouse.scroll_x;
}
float32 MouseScrollY()
{
    return g_app.input_manager.mouse.scroll_y;
}

void MoveMouse(int16 x, int16 y)
{
    uint8 coeff = WindowGetScale();
    float32 mx = (((float32)x + g_app.info.offset_x) * (float32)coeff);
    float32 my = (((float32)y + g_app.info.offset_y) * (float32)coeff);
    SDL_WarpMouseInWindow(g_app.window, mx, my);
}

bool MouseHideCursor()
{
    if (!SDL_HideCursor())
    {
        LOG_WARNING("Impossible to hide cursor :%s", SDL_GetError());
        return false;
    }
    return true;
}

bool MouseShowCursor()
{
    if (!SDL_ShowCursor())
    {
        LOG_WARNING("Impossible to show cursor :%s", SDL_GetError());
        return false;
    }
    return true;
}
