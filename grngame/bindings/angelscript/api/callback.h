#pragma once

#include "../../../utils/c_cpp.h"
#include "grngame/math/types.h"

#include <angelscript.h>

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
        const char *string;
    } as;
} CallbackArg;

#define PAD_CONNECT 0
#define PAD_DISCONNECT 1
#define JSON_SAVE 2
#define GAME_DATA_NEW_VERSION 3

BEGIN_DECLARATIONS

void MessageCallback(const asSMessageInfo *msg, void *param);

void RegisterCallback(uint16 name);
void CallCallBack(CallbackArg *args, uint16 name);

bool AngelCallOnStart();
bool AngelCallOnUpdate(float32 delta);
bool AngelCallOnFixedUpdate(float32 delta);
bool AngelCallOnRender();
bool AngelCallOnDestroy();

END_DECLARATIONS