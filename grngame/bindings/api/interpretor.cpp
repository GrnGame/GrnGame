#include "grngame/bindings/dascript/api/interpretor.h"
#include "grngame/bindings/python/api/interpretor.h"
#include "grngame/core/app.h"
#include "interpretor.h"

bool ScriptRun(const char *filename)
{
    if (g_app.info.scripting_langage == DAS_SCRIPT)
        return DaScriptRun(filename);
    else if (g_app.info.scripting_langage == PY_SCRIPT)
    {
        return PyScriptRun(filename);
    }
    else
    {
        LOG_ERROR("Cannot run script in unknown scripting language");
        return false;
    }
}

bool ScriptRunFromMemory(const char *filename)
{
    if (g_app.info.scripting_langage == DAS_SCRIPT)
        return DaScriptRunFromMemory(filename);
    else if (g_app.info.scripting_langage == PY_SCRIPT)
    {
        return PyScriptRunFromMemory(filename);
    }
    else
    {
        LOG_ERROR("Cannot run script from memory in unknown scripting language");
        return false;
    }
}
bool ScriptReload(const char *filename)
{
    if (g_app.info.scripting_langage == DAS_SCRIPT)
        return DaScriptReload(filename);
    else if (g_app.info.scripting_langage == PY_SCRIPT)
    {
        return PyScriptReload(filename);
    }
    else
    {
        LOG_ERROR("Cannot reload script from memory in unknown scripting language");
        return false;
    }
}

void ScriptFree()
{
    if (g_app.info.scripting_langage == DAS_SCRIPT)
        DaScriptFree();
    else if (g_app.info.scripting_langage == PY_SCRIPT)
    {
        PyScriptFree();
    }
    else
    {
        LOG_ERROR("Cannot free script in unknown scripting language");
    }
}
