#pragma once

#include "../../../utils/c_cpp.h"
#include "grngame/bindings/api/callback.h"
#include "grngame/math/types.h"

BEGIN_DECLARATIONS

struct asSMessageInfo;

void MessageCallback(const struct asSMessageInfo *msg, void *param);

bool AngelRegisterCallback(uint16 name, const char *funcDecl);
bool AngelCallBack(CallbackArg *args, uint16 name);

void AngelCacheLifecycleFunctions(const char *moduleName);
bool AngelCallOnStart();
bool AngelCallOnUpdate(float32 delta);
bool AngelCallOnFixedUpdate(float32 delta);
bool AngelCallOnRender();
bool AngelCallOnDestroy();

void FreeCallbacks();

END_DECLARATIONS