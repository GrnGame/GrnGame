#include <daScript/daScript.h>
#include "grngame/bindings/dascript/api/callback.h"
#include "grngame/assets/cooker_manager.h"
#include "grngame/platform/paths.h"
#include "interpretor.h"
#include <cstdlib>

// todo make a class
static das::SimFunction *on_update = nullptr;
static das::SimFunction *on_start = nullptr;
static das::SimFunction *on_fixed_update = nullptr;
static das::SimFunction *on_render = nullptr;
static das::SimFunction *on_destroy = nullptr;
static das::SimFunction *callback[10] = {nullptr};

bool DaScriptCallBack(CallbackArg *args, uint16 name)
{
    return true;
}

bool DaScriptCallOnStart()
{
    return true;
}

bool DaScriptCallOnUpdate(float32 delta)
{
    return true;
}

bool DaScriptCallOnFixedUpdate(float32 delta)
{
    return true;
}

bool DaScriptCallOnRender()
{
    return true;
}

bool DaScriptCallOnDestroy()
{
    return true;
}