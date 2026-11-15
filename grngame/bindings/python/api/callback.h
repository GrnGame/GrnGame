#pragma once

#include "../../../utils/c_cpp.h"
#include "../../api/callback.h"
#include "pocketpy/pocketpy.h"

BEGIN_DECLARATIONS

bool PyScriptCallBack(CallbackArg *args, uint16 name);
bool PyScriptCallOnStart();
bool PyScriptCallOnUpdate(float32 delta);
bool PyScriptCallOnFixedUpdate(float32 delta);
bool PyScriptCallOnRender();
bool PyScriptCallOnDestroy();

// set a python function in  array of callbacks
bool PyScriptStoreCallback(uint16 name, py_Ref func);

// clear callbacks
void PyScriptClearCallbacks(void);

END_DECLARATIONS