#include "interpretor.h"

#include "callback.h"
#include "grngame/assets/embedded_file_manager.h"
#include "grngame/core/app.h"
#include "grngame/data/file.h"
#include "grngame/platform/paths.h"
#include "grngame/utils/string_compat.h"
#include <stdlib.h>

bool InterpretFromMemory(const char *filename)
{
    char *module_name = FileStem(filename);

    const EmbeddedFile *asset = EmbeddedFileGet(filename);
    if (!asset)
    {
        LOG_ERROR("Failed to find script '%s' in embedded files", filename);
        free(module_name);
        return false;
    }

    char *source = AddEndOfString(static_cast<char *>(asset->data), asset->size);

    auto *engine = static_cast<asIScriptEngine *>(g_app.angel_manager.engine);
    asIScriptModule *mod = engine->GetModule(module_name, asGM_ALWAYS_CREATE);

    int result = mod->AddScriptSection(filename, source);
    free(source);

    if (result < 0)
    {
        LOG_ERROR("AngelScript AddScriptSection failed for embedded '%s'", filename);
        free(module_name);
        return false;
    }

    result = mod->Build();
    if (result < 0)
    {
        LOG_ERROR("AngelScript Build failed for embedded '%s'", filename);
        free(module_name);
        return false;
    }

    free(module_name);
    return true;
}

bool Interpret(const char *filename)
{
    char *module_name = FileStem(filename);
    char *path = PathFromExecutableDirectory(filename);
    char *file_content = reinterpret_cast<char *>(ReturnFileData(path, nullptr));
    free(path);

    if (!file_content)
    {
        LOG_ERROR("Failed to read script '%s'", filename);
        free(module_name);
        return false;
    }

    auto *engine = static_cast<asIScriptEngine *>(g_app.angel_manager.engine);
    asIScriptModule *mod = engine->GetModule(module_name, asGM_ALWAYS_CREATE);

    int result = mod->AddScriptSection(filename, file_content);
    free(file_content);

    if (result < 0)
    {
        LOG_ERROR("AngelScript AddScriptSection failed for '%s'", filename);
        free(module_name);
        return false;
    }

    result = mod->Build();
    if (result < 0)
    {
        LOG_ERROR("AngelScript Build failed for '%s'", filename);
        free(module_name);
        return false;
    }

    free(module_name);
    return true;
}

bool AngelInit(const char *filename)
{
    asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
    if (!engine)
    {
        LOG_ERROR("Failed to create AngelScript engine");
        return false;
    }

    engine->SetMessageCallback(asFUNCTION(MessageCallback), nullptr, asCALL_CDECL);

    g_app.angel_manager.engine = reinterpret_cast<void *>(engine);

    return Interpret(filename);
}

bool AngelInitFromMemory(const char *filename)
{
    asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
    if (!engine)
    {
        LOG_ERROR("Failed to create AngelScript engine");
        return false;
    }

    engine->SetMessageCallback(asFUNCTION(MessageCallback), nullptr, asCALL_CDECL);

    g_app.angel_manager.engine = reinterpret_cast<void *>(engine);

    return InterpretFromMemory(filename);
}

bool AngelFree()
{
    if (auto *engine = static_cast<asIScriptEngine *>(g_app.angel_manager.engine))
    {
        engine->ShutDownAndRelease();
        g_app.angel_manager.engine = nullptr;
    }
    return true;
}