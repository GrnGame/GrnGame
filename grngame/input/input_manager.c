#include "input_manager.h"
#include "grngame/utils/attributes.h"

InputManager InputManagerCreate()
{
    InputManager m = {
        .mouse = {.x = 0,
                  .y = 0,
                  .left_pressed = false,
                  .left_just_pressed = false,
                  .left_just_released = false,
                  .right_pressed = false,
                  .right_just_pressed = false,
                  .right_just_released = false,
                  .scroll_x = 0,
                  .scroll_y = 0},
        .drop_file = {0},
        .keyboard = {0},
        .controllers = {0},
    };
    kv_init(m.text_input);
    return m;
}

void InputManagerDestroy(InputManager *manager)
{
    if (manager == NULL)
        return;

    for (int16 i = 0; i < MAX_CONTROLLERS; ++i)
    {
        if (manager->controllers[i].gamepad != NULL)
        {
            GamepadClose(manager->controllers[i].gamepad);

            manager->controllers[i].gamepad = NULL;
            manager->controllers[i].joystick = NULL;
            manager->controllers[i].id = 0;
            manager->controllers[i].name = NULL;
        }
    }

    kv_destroy(manager->text_input);
}
