#pragma once

#include "../../utils/c_cpp.h"
#include "grngame/math/types.h"

typedef enum
{
    CB_ARG_NUM,
    CB_ARG_BOOL,
    CB_ARG_STRING,
} CallbackArgType;

typedef struct
{
    CallbackArgType type;
    union {
        float64 num;
        bool boolean;
        void *string;
    } as;
} CallbackArg;

#define MAX_CALLBACKS 10

#define PAD_CONNECT 0
#define PAD_DISCONNECT 1
#define JSON_SAVE 2
#define GAME_DATA_NEW_VERSION 3

BEGIN_DECLARATIONS

bool RegisterCallback(uint16 name, const char *funcDecl);
bool CallBack(CallbackArg *args, uint16 name);

bool CallOnStart();
bool CallOnUpdate(float32 delta);
bool CallOnFixedUpdate(float32 delta);
bool CallOnRender();
bool CallOnDestroy();

END_DECLARATIONS