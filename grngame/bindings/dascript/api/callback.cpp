#include "interpretor.h"
#include <daScript/daScript.h>
#include "../../api/callback.h"
#include "grngame/assets/embedded_file_manager.h"
#include "grngame/platform/paths.h"
#include <cstdlib>


//todo make a class
static das::SimFunction *on_update = nullptr;
static das::SimFunction *on_start = nullptr;
static das::SimFunction *on_fixed_update = nullptr;
static das::SimFunction *on_render = nullptr;
static das::SimFunction *on_destroy = nullptr;
static das::SimFunction *callback[10] = {nullptr};


bool DaScriptRegisterCallback(uint16 name, const char *funcDecl)
{
    return true;
}

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