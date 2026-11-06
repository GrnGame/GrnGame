#include "grngame/bindings/angelscript/api/interpretor.h"
#include "grngame/bindings/dascript/api/interpretor.h"
#include "grngame/core/app.h"
#include "interpretor.h"

bool BindingsInterpret(const char *filename)
{
    if (g_app.info.scripting_langage == ANGEL_SCRIPT)
        return AngelInterpret(filename);
    else
    {
        return true;
    }
}

bool BindingsInterpretFromMemory(const char *filename)
{
    if (g_app.info.scripting_langage == ANGEL_SCRIPT)
        return AngelInterpretFromMemory(filename);
    else
    {
        return true;
    }
}

bool BindingsInit(const char *filename)
{
    if (g_app.info.scripting_langage == ANGEL_SCRIPT)
        return AngelInit(filename);
    else
    {
        return true;
    }
}

bool BindingsInitFromMemory(const char *filename)
{
    if (g_app.info.scripting_langage == ANGEL_SCRIPT)
        return AngelInitFromMemory(filename);
    else
    {
        return true;
    }
}

bool BindingsReload(const char *filename)
{
    if (g_app.info.scripting_langage == ANGEL_SCRIPT)
        return AngelReload(filename);
    else
    {
        return true;
    }
}

bool BindingsFree()
{
    if (g_app.info.scripting_langage == ANGEL_SCRIPT)
        return AngelFree();
    else
    {
        return true;
    }
}