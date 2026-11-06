#include "../angelscript/api/callback.h"
#include "callback.h"
#include "grngame/bindings/angelscript/api/callback.h"
#include "grngame/core/app.h"

bool BindingsRegisterCallback(uint16 name, const char *funcDecl)
{
    if (g_app.info.scripting_langage == ANGEL_SCRIPT)
        return AngelRegisterCallback(name, funcDecl);
    else
    {
        return true;
    }
}
bool BindingsCallBack(CallbackArg *args, uint16 name)
{
    if (g_app.info.scripting_langage == ANGEL_SCRIPT)
        return AngelCallBack(args, name);
    else
    {
        return true;
    }
}
bool BindingsCallOnStart()
{
    if (g_app.info.scripting_langage == ANGEL_SCRIPT)
        return AngelCallOnStart();
    else
    {
        return true;
    }
}
bool BindingsCallOnUpdate(float32 delta)
{
    if (g_app.info.scripting_langage == ANGEL_SCRIPT)
        return AngelCallOnUpdate(delta);
    else
    {
        return true;
    }
}
bool BindingsCallOnFixedUpdate(float32 delta)
{
    if (g_app.info.scripting_langage == ANGEL_SCRIPT)
        return AngelCallOnFixedUpdate(delta);
    else
    {
        return true;
    }
}
bool BindingsCallOnRender()
{
    if (g_app.info.scripting_langage == ANGEL_SCRIPT)
        return AngelCallOnRender();
    else
    {
        return true;
    }
}
bool BindingsCallOnDestroy()
{
    if (g_app.info.scripting_langage == ANGEL_SCRIPT)
        return AngelCallOnDestroy();
    else
    {
        return true;
    }
}