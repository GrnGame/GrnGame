#pragma once

#include "../../../utils/c_cpp.h"
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
        const char *string;
    } as;
} CallbackArg;

#define MAX_CALLBACKS 10

#define PAD_CONNECT 0
#define PAD_DISCONNECT 1
#define JSON_SAVE 2
#define GAME_DATA_NEW_VERSION 3

BEGIN_DECLARATIONS

struct asSMessageInfo;

void MessageCallback(const struct asSMessageInfo *msg, void *param);

bool RegisterCallback(uint16 name, const char *funcDecl);
bool CallCallBack(CallbackArg *args, uint16 name);

void AngelCacheLifecycleFunctions(const char *moduleName);
bool AngelCallOnStart();
bool AngelCallOnUpdate(float32 delta);
bool AngelCallOnFixedUpdate(float32 delta);
bool AngelCallOnRender();
bool AngelCallOnDestroy();

void FreeCallbacks();

END_DECLARATIONS