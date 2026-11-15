#pragma once

#include "../../../utils/c_cpp.h"
#include "../../api/callback.h"

BEGIN_DECLARATIONS

bool DaScriptCallBack(CallbackArg *args, uint16 name);
bool DaScriptCallOnStart();
bool DaScriptCallOnUpdate(float32 delta);
bool DaScriptCallOnFixedUpdate(float32 delta);
bool DaScriptCallOnRender();
bool DaScriptCallOnDestroy();

END_DECLARATIONS