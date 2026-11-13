#include "grngame/bindings/api/callback.h"

#include "grngame/assets/embedded_file_manager.h"
#include "grngame/platform/paths.h"
#include "interpretor.h"
#include "pocketpy.h"
#include <cstdlib>

static py_Ref *on_update = nullptr;
static py_Ref *on_start = nullptr;
static py_Ref *on_fixed_update = nullptr;
static py_Ref *on_render = nullptr;
static py_Ref *on_destroy = nullptr;
static py_Ref *on_callback[10] = { nullptr };

bool PyScriptRegisterCallback(uint16 name, const char *funcDecl)
{
    return true;
}

bool PyScriptCallBack(CallbackArg *args, uint16 name)
{
    return true;
}

bool PyScriptCallOnStart()
{
    return true;
}

bool PyScriptCallOnUpdate(float32 delta)
{
    return true;
}

bool PyScriptCallOnFixedUpdate(float32 delta)
{
    return true;
}

bool PyScriptCallOnRender()
{
    return true;
}

bool PyScriptCallOnDestroy()
{
    return true;
}

