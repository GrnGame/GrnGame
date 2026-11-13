#include "callback.h"
#include "grngame/bindings/dascript/api/callback.h"
#include "grngame/bindings/python/api/callback.h"
#include "grngame/core/app.h"

bool RegisterCallback(uint16 name, const char *funcDecl)
{
     if (g_app.info.scripting_langage == DAS_SCRIPT)
        return DaScriptRegisterCallback(name, funcDecl);
    else if (g_app.info.scripting_langage == PY_SCRIPT)
        return PyScriptRegisterCallback(name, funcDecl);
    else
    {
        LOG_ERROR("Cannot register callback in unknown scripting language");
        return false;
    }
}

bool CallBack(CallbackArg *args, uint16 name)
{
     if (g_app.info.scripting_langage == DAS_SCRIPT)
        return DaScriptCallBack(args, name);
    else if (g_app.info.scripting_langage == PY_SCRIPT)
        return PyScriptCallBack(args, name);
    else
    {
        LOG_ERROR("Cannot execute callback in unknown scripting language");
        return false;
    }
}

bool CallOnStart()
{
     if (g_app.info.scripting_langage == DAS_SCRIPT)
        return DaScriptCallOnStart();
    else if (g_app.info.scripting_langage == PY_SCRIPT)
        return PyScriptCallOnStart();
    else
    {
        LOG_ERROR("Cannot call OnStart in unknown scripting language");
        return false;
    }
}

bool CallOnUpdate(float32 delta)
{
     if (g_app.info.scripting_langage == DAS_SCRIPT)
        return DaScriptCallOnUpdate(delta);
    else if (g_app.info.scripting_langage == PY_SCRIPT)
        return PyScriptCallOnUpdate(delta);
    else
    {
        LOG_ERROR("Cannot call OnUpdate in unknown scripting language");
        return false;
    }
}

bool CallOnFixedUpdate(float32 delta)
{
     if (g_app.info.scripting_langage == DAS_SCRIPT)
        return DaScriptCallOnFixedUpdate(delta);
    else if (g_app.info.scripting_langage == PY_SCRIPT)
        return PyScriptCallOnFixedUpdate(delta);
    else
    {
        LOG_ERROR("Cannot call OnFixedUpdate in unknown scripting language");
        return false;
    }
}

bool CallOnRender()
{
     if (g_app.info.scripting_langage == DAS_SCRIPT)
        return DaScriptCallOnRender();
    else if (g_app.info.scripting_langage == PY_SCRIPT)
        return PyScriptCallOnRender();
    else
    {
        LOG_ERROR("Cannot call OnRender in unknown scripting language");
        return false;
    }
}

bool CallOnDestroy()
{
     if (g_app.info.scripting_langage == DAS_SCRIPT)
        return DaScriptCallOnDestroy();
    else if (g_app.info.scripting_langage == PY_SCRIPT)
        return PyScriptCallOnDestroy();
    else
    {
        LOG_ERROR("Cannot call OnDestroy in unknown scripting language");
        return false;
    }
}