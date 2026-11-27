#pragma once
#include "app.h"
#include <stdbool.h>

typedef enum
{
    INIT_OK,
    INIT_SDL_FAILED,
    INIT_LOG_FAILED,
    INIT_SOUND_FAILED,
    INIT_CONFIG_FAILED,
    INIT_DATA_FAILED,
} InitResult;

InitResult InitAll();

InitResult InitAppConfig(void);
