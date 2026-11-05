#include "controller.h"
#include "../core/app.h"
#include "SDL3/SDL_gamepad.h"
#include "SDL3/SDL_joystick.h"
#include "SDL3/SDL_stdinc.h"
#include "grngame/bindings/api/callback.h"
#include "grngame/dev/logging.h"
#include "grngame/math/math.h"
#include "grngame/utils/attributes.h"
#include <SDL3/SDL_events.h>
#include <grngame/utils/string_compat.h>
#include <math.h>
#include <stdio.h>

// sdl wrappent
SDL_Gamepad *GamepadOpen(SDL_JoystickID id)
{
    SDL_Gamepad *gp = SDL_OpenGamepad(id);
    if (UNLIKELY(!gp))
        LOG_WARNING("Failed to open gamepad %d: %s", id, SDL_GetError());
    return gp;
}

SDL_Gamepad *GamepadFromID(SDL_JoystickID id)
{
    return SDL_GetGamepadFromID(id);
}

SDL_Joystick *GamepadGetJoystick(SDL_Gamepad *gp)
{
    SDL_Joystick *joy = SDL_GetGamepadJoystick(gp);
    if (UNLIKELY(!joy))
        LOG_WARNING("Failed to get joystick from gamepad: %s", SDL_GetError());
    return joy;
}

const char *GamepadGetName(SDL_Gamepad *gp)
{
    const char *name = SDL_GetGamepadName(gp);
    if (UNLIKELY(!name))
        LOG_WARNING("Failed to get gamepad name: %s", SDL_GetError());
    return name;
}

void GamepadClose(SDL_Gamepad *gp)
{
    SDL_CloseGamepad(gp);
}

int32 ControllerConnectedCountptr(SDL_JoystickID **ptr)
{
    int32 count = 0;
    *ptr = SDL_GetGamepads(&count);
    return count;
}

int32 ControllerConnectedCount(void)
{
    int32 count = 0;
    SDL_JoystickID *pads = SDL_GetGamepads(&count);
    if (LIKELY(pads))
        SDL_free(pads);
    return count;
}

bool ControllerOpen(SDL_JoystickID id, int16 index)
{
    SDL_JoystickID *pads = NULL;
    int32 count = ControllerConnectedCountptr(&pads);

    if (UNLIKELY(count == 0))
        return false;

    SDL_Gamepad *gp = GamepadOpen(id);
    if (UNLIKELY(!gp))
        return false;

    SDL_Joystick *joy = GamepadGetJoystick(gp);
    const char *name = GamepadGetName(gp) ? GamepadGetName(gp) : "Unknown";

    g_app.input_manager.controllers[index] = (Controller){.gamepad = gp, .joystick = joy, .id = id, .name = name};
    if (UNLIKELY(index == -1))
    {
        LOG_WARNING("Maximum controller capacity reached.");
        GamepadClose(gp);
        return false;
    }

    CallbackArg args[1] = {{.type = CB_ARG_NUM, .as.num = index}};

    BindingsCallBack(args, PAD_CONNECT);

    if (UNLIKELY(!SDL_GamepadHasAxis(gp, SDL_GAMEPAD_AXIS_LEFTX) || !SDL_GamepadHasAxis(gp, SDL_GAMEPAD_AXIS_LEFTY)))
        LOG_WARNING("Gamepad %d (%s) lacks a left analog stick.", index, name);

    if (UNLIKELY(!SDL_GamepadHasAxis(gp, SDL_GAMEPAD_AXIS_RIGHTX) || !SDL_GamepadHasAxis(gp, SDL_GAMEPAD_AXIS_RIGHTY)))
        LOG_WARNING("Gamepad %d (%s) lacks a right analog stick.", index, name);

    if (UNLIKELY(!SDL_GamepadHasAxis(gp, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) ||
                 !SDL_GamepadHasAxis(gp, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)))
        LOG_WARNING("Gamepad %d (%s) may lack analog triggers.", index, name);

    LOG_INFO("Gamepad %d successfully opened: %s", index, name);
    return true;
}

void ControllerClose(int16 index)
{
    if (UNLIKELY(index < 0 || index >= MAX_CONTROLLERS))
    {
        LOG_WARNING("Invalid controller index provided for closure: %d", index);
        return;
    }

    Controller *c = &g_app.input_manager.controllers[index];

    if (LIKELY(c->gamepad != NULL))
    {
        const char *name = GamepadGetName(c->gamepad);
        LOG_INFO("Closing gamepad %d: %s", index, name ? name : "Unknown");
        GamepadClose(c->gamepad);

        c->gamepad = NULL;
        c->joystick = NULL;
        c->id = 0;
        c->name = NULL;
        CallbackArg args[1] = {{.type = CB_ARG_NUM, .as.num = index}};
        BindingsCallBack(args, PAD_DISCONNECT);
    }
}

bool PadJustPressed(int32 button, int16 index)
{
    if (UNLIKELY(index >= MAX_CONTROLLERS || button < 0 || button >= SDL_GAMEPAD_BUTTON_COUNT))
    {
        LOG_WARNING("Invalid pad button %d or index %d", button, index);
        return false;
    }
    return g_app.input_manager.controllers[index].just_pressed[button];
}

bool PadPressed(int32 button, int16 index)
{
    if (UNLIKELY(index < 0 || index >= MAX_CONTROLLERS || button < 0 || button >= SDL_GAMEPAD_BUTTON_COUNT))
    {
        LOG_WARNING("Invalid pad button %d or index %d", button, index);
        return false;
    }

    Controller *c = &g_app.input_manager.controllers[index];

    if (UNLIKELY(c->gamepad == NULL))
        return false;

    return c->pressed[button];
}

bool PadJustReleased(int32 button, int16 index)
{
    if (UNLIKELY(index >= MAX_CONTROLLERS || button < 0 || button >= SDL_GAMEPAD_BUTTON_COUNT))
    {
        LOG_WARNING("Invalid pad button %d or index %d", button, index);
        return false;
    }
    return g_app.input_manager.controllers[index].just_released[button];
}

// todo test if function is call at frame 0 and call it at frame 1 and warn
bool PadRumble(int16 index, uint8 left_rumble, uint8 right_rumble, uint32 time)
{
    if (UNLIKELY(index < 0 || index >= MAX_CONTROLLERS))
    {
        LOG_WARNING("Invalid controller index for rumble: %d", index);
        return false;
    }

    SDL_Gamepad *gp = g_app.input_manager.controllers[index].gamepad;
    if (UNLIKELY(!gp))
    {
        LOG_WARNING("Controller %d is not connected or gamepad is null", index);
        return false;
    }

    left_rumble = CLAMP(0, left_rumble, 100);
    right_rumble = CLAMP(0, right_rumble, 100);

    Uint16 sdl_left = (Uint16)((left_rumble * 65535u) / 100u);
    Uint16 sdl_right = (Uint16)((right_rumble * 65535u) / 100u);

    if (!SDL_RumbleGamepad(gp, sdl_left, sdl_right, time))
    {
        LOG_WARNING("Failed to rumble controller %d: %s", index, SDL_GetError());
        return false;
    }

    return true;
}

bool PadHasButton(int16 index, int32 button)
{
    if (UNLIKELY(index < 0 || index >= MAX_CONTROLLERS))
    {
        LOG_WARNING("Invalid controller index for has_button: %d", index);
        return false;
    }

    SDL_Gamepad *gp = g_app.input_manager.controllers[index].gamepad;
    if (UNLIKELY(!gp))
    {
        LOG_WARNING("Controller %d is not connected or gamepad is null", index);
        return false;
    }

    return SDL_GamepadHasButton(gp, (SDL_GamepadButton)button);
}

bool PadHasAxis(int16 index, int32 axis)
{
    if (UNLIKELY(index < 0 || index >= MAX_CONTROLLERS))
    {
        LOG_WARNING("Invalid controller index for has_axis: %d", index);
        return false;
    }

    SDL_Gamepad *gp = g_app.input_manager.controllers[index].gamepad;
    if (UNLIKELY(!gp))
    {
        LOG_WARNING("Controller %d is not connected or gamepad is null", index);
        return false;
    }

    return SDL_GamepadHasAxis(gp, (SDL_GamepadAxis)axis);
}

bool PadHasSensor(int16 index, int32 sensor_type)
{
    if (UNLIKELY(index < 0 || index >= MAX_CONTROLLERS))
    {
        LOG_WARNING("Invalid controller index for has_sensor: %d", index);
        return false;
    }

    SDL_Gamepad *gp = g_app.input_manager.controllers[index].gamepad;
    if (UNLIKELY(!gp))
    {
        LOG_WARNING("Controller %d is not connected or gamepad is null", index);
        return false;
    }

    return SDL_GamepadHasSensor(gp, (SDL_SensorType)sensor_type);
}

int32 PadFirstPressedIndexForButton(int32 button)
{
    if (UNLIKELY(button < 0 || button >= SDL_GAMEPAD_BUTTON_COUNT))
    {
        LOG_WARNING("Invalid pad button %d", button);
        return -1;
    }

    for (int32 i = 0; i < MAX_CONTROLLERS; ++i)
    {
        Controller *c = &g_app.input_manager.controllers[i];
        if (!c->gamepad)
            continue;

        if (c->pressed[button])
            return i;
    }

    return -1;
}

float32 compute_axis(Sint16 value, float32 dead_zone)
{
    const float32 norm = 32767.0f;
    float32 res = (float32)value / norm;
    if (UNLIKELY(fabsf(res) < dead_zone))
    {
        return 0.0f;
    }
    return res;
}

float32 PadStickLX(uint16 index)
{
    if (UNLIKELY(index >= MAX_CONTROLLERS))
    {
        LOG_WARNING("Invalid controller index for stick lx: %d", index);
        return 0.0f;
    }
    return g_app.input_manager.controllers[index].stick_lx;
}

float32 PadStickLY(uint16 index)
{
    if (UNLIKELY(index >= MAX_CONTROLLERS))
    {
        LOG_WARNING("Invalid controller index for stick ly: %d", index);
        return 0.0f;
    }
    return g_app.input_manager.controllers[index].stick_ly;
}

float32 PadStickRX(uint16 index)
{
    if (UNLIKELY(index >= MAX_CONTROLLERS))
    {
        LOG_WARNING("Invalid controller index for stick rx: %d", index);
        return 0.0f;
    }
    return g_app.input_manager.controllers[index].stick_rx;
}

float32 PadStickRY(uint16 index)
{
    if (UNLIKELY(index >= MAX_CONTROLLERS))
    {
        LOG_WARNING("Invalid controller index for stick ry: %d", index);
        return 0.0f;
    }
    return g_app.input_manager.controllers[index].stick_ry;
}

float32 PadTriggerL(uint8 index)
{
    if (UNLIKELY(index >= MAX_CONTROLLERS))
    {
        LOG_WARNING("Invalid controller index for trigger l: %d", index);
        return 0.0f;
    }
    return g_app.input_manager.controllers[index].trigger_l;
}

float32 PadTriggerR(uint8 index)
{
    if (UNLIKELY(index >= MAX_CONTROLLERS))
    {
        LOG_WARNING("Invalid controller index for trigger r: %d", index);
        return 0.0f;
    }
    return g_app.input_manager.controllers[index].trigger_r;
}