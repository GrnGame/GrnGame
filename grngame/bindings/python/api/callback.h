#pragma once

#include "../../../utils/c_cpp.h"
#include "../../api/callback.h"

BEGIN_DECLARATIONS

bool PyScriptRegisterCallback(uint16 name, const char *funcDecl);
bool PyScriptCallBack(CallbackArg *args, uint16 name);
bool PyScriptCallOnStart();
bool PyScriptCallOnUpdate(float32 delta);
bool PyScriptCallOnFixedUpdate(float32 delta);
bool PyScriptCallOnRender();
bool PyScriptCallOnDestroy();

END_DECLARATIONS