#include "../core/app.h"
#include "cglm/types-struct.h"
#include "controller.h"
#include "grngame/core/window.h"
#include "grngame/input/input_text.h"
#include "grngame/utils/attributes.h"
#include "grngame/utils/clear.h"
// dont moove string compat
#include "grngame/input/keyboard.h"
#include "grngame/utils/string_compat.h"
#include "mouse.h"
#include <SDL3/SDL_events.h>

static int16 FindControllerIndex(SDL_JoystickID id)
{
    for (int16 i = 0; i < MAX_CONTROLLERS; i++)
    {
        Controller *c = &g_app.input_manager.controllers[i];
        if (!c->id)
        {
            c->id = id;
            return i;
        }
        if (c->id && c->id == id)
        {
            return i;
        }
    }
    return -1;
}

static int16 FindKeyboardIndex(SDL_KeyboardID id)
{

    for (int16 i = 0; i < MAX_KEYBOARDS; i++)
    {
        Keyboard *k = &g_app.input_manager.keyboard[i];
        if (!k->id)
        {
            k->id = id;
            return i;
        }
        if (k->id && k->id == id)
        {
            return i;
        }
    }
    return -1;
}

static void ResetInputManagerKeys()
{
    InputManager *im = &g_app.input_manager;

    im->mouse.left_just_pressed = false;
    im->mouse.right_just_pressed = false;
    im->mouse.left_just_released = false;
    im->mouse.right_just_released = false;
    im->mouse.scroll_x = 0;
    im->mouse.scroll_y = 0;
    for (int16 i = 0; i < MAX_KEYBOARDS; i++)
    {
        CLEAR(im->keyboard[i].key_just_pressed, 0);
        CLEAR(im->keyboard[i].key_just_released, 0);
    }

    for (int32 i = 0; i < MAX_CONTROLLERS; i++)
    {
        CLEAR(im->controllers[i].just_pressed, 0);
        CLEAR(im->controllers[i].just_released, 0);
    }
}

static void UpdateMousePosition()
{
    vec2s i = GetMousePosition();
    uint8 coeff = WindowGetScale();
    g_app.input_manager.mouse.x = (int32)SDL_roundf(i.x / (float32)coeff - g_app.info.offset_x);
    g_app.input_manager.mouse.y = (int32)SDL_roundf(i.y / (float32)coeff - g_app.info.offset_y);
}

static void BackspaceTextInput()
{
    InputManager *im = &g_app.input_manager;
    if ((uint32)kv_size(im->text_input) > 0)
        kv_size(im->text_input)--;
}

HOT void PollEvents()
{
    InputManager *im = &g_app.input_manager;
    ResetInputManagerKeys();
    UpdateMousePosition();

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            EngineSetStop();
            break;

        case SDL_EVENT_WINDOW_RESIZED:
            ApplyResizing(&g_app.info, (int16)event.window.data1, (int16)event.window.data2);
            break;

        case SDL_EVENT_WINDOW_MAXIMIZED: {
            ivec2s i = WindowDimensions(g_app.window);
            ApplyResizing(&g_app.info, (int16)i.x, (int16)i.y);
            break;
        }
        case SDL_EVENT_WINDOW_MINIMIZED: {
            g_app.info.window_occlusion_culled = true;
            break;
        }
        case SDL_EVENT_WINDOW_RESTORED: {
            g_app.info.window_occlusion_culled = false;
            break;
        }
        case SDL_EVENT_KEY_DOWN:
            if (!event.key.repeat)
            {
                if (event.key.scancode == SDL_SCANCODE_BACKSPACE)
                    BackspaceTextInput();

                if (LIKELY(event.key.scancode < SDL_SCANCODE_COUNT))
                {
                    int16 index = FindKeyboardIndex(event.key.which);
                    im->keyboard[index].key_just_pressed[event.key.scancode] = true;
                    im->keyboard[index].key_pressed[event.key.scancode] = true;
                }
            }
            break;

        case SDL_EVENT_KEY_UP:
            if (LIKELY(event.key.scancode < SDL_SCANCODE_COUNT))
            {
                int16 index = FindKeyboardIndex(event.key.which);
                im->keyboard[index].key_just_released[event.key.scancode] = true;
                im->keyboard[index].key_pressed[event.key.scancode] = false;
            }
            break;
        case SDL_EVENT_TEXT_INPUT:
            InputTextPutText(event.text.text);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                im->mouse.left_pressed = true;
                im->mouse.left_just_pressed = true;
            }
            else if (event.button.button == SDL_BUTTON_RIGHT)
            {
                im->mouse.right_pressed = true;
                im->mouse.right_just_pressed = true;
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                im->mouse.left_pressed = false;
                im->mouse.left_just_released = true;
            }
            else if (event.button.button == SDL_BUTTON_RIGHT)
            {
                im->mouse.right_pressed = false;
                im->mouse.right_just_released = true;
            }
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            im->mouse.scroll_y = event.wheel.y;
            im->mouse.scroll_x = event.wheel.x;
            break;
        case SDL_EVENT_DROP_FILE:
            if (event.drop.data)
            {
                strncpy(im->drop_file, event.drop.data, DROP_FILE_PATH_MAX);
                im->drop_file[DROP_FILE_PATH_MAX - 1] = '\0';
            }
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN: {
            int16 idx = FindControllerIndex(event.gbutton.which);
            if (LIKELY(idx >= 0 && event.gbutton.button < SDL_GAMEPAD_BUTTON_COUNT))
            {
                bool *pressed = &im->controllers[idx].pressed[event.gbutton.button];
                if (!*pressed)
                    im->controllers[idx].just_pressed[event.gbutton.button] = true;
                *pressed = true;
            }
            break;
        }
        case SDL_EVENT_GAMEPAD_BUTTON_UP: {
            int16 idx = FindControllerIndex(event.gbutton.which);
            if (LIKELY(idx >= 0 && event.gbutton.button < SDL_GAMEPAD_BUTTON_COUNT))
            {
                im->controllers[idx].pressed[event.gbutton.button] = false;
                im->controllers[idx].just_released[event.gbutton.button] = true;
            }

            break;
        }

        case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
            int16 idx = FindControllerIndex(event.gaxis.which);
            if (UNLIKELY(idx < 0))
                break;
            switch (event.gaxis.axis)
            {
            case SDL_GAMEPAD_AXIS_LEFTX:
                im->controllers[idx].stick_lx = compute_axis(event.gaxis.value, DEFAULT_DEAD_ZONE);
                break;
            case SDL_GAMEPAD_AXIS_LEFTY:
                im->controllers[idx].stick_ly = compute_axis(event.gaxis.value, DEFAULT_DEAD_ZONE);
                break;
            case SDL_GAMEPAD_AXIS_RIGHTX:
                im->controllers[idx].stick_rx = compute_axis(event.gaxis.value, DEFAULT_DEAD_ZONE);
                break;
            case SDL_GAMEPAD_AXIS_RIGHTY:
                im->controllers[idx].stick_ry = compute_axis(event.gaxis.value, DEFAULT_DEAD_ZONE);
                break;
            case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
                im->controllers[idx].trigger_l = compute_axis(event.gaxis.value, DEFAULT_DEAD_ZONE);
                break;
            case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
                im->controllers[idx].trigger_r = compute_axis(event.gaxis.value, DEFAULT_DEAD_ZONE);
                break;
            }
            break;
        }
        case SDL_EVENT_GAMEPAD_ADDED: {
            int16 idx = FindControllerIndex(event.gdevice.which);
            ControllerOpen(event.gdevice.which, idx);
            break;
        }
        case SDL_EVENT_GAMEPAD_REMOVED: {
            int16 idx = FindControllerIndex(event.gdevice.which);
            if (LIKELY(idx >= 0))
                ControllerClose((int16)idx);
            break;
        }

        default:
            break;
        }
    }
}
