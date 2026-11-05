#include "interpretor.h"
#include "callback.h"
#include "grngame/assets/embedded_file_manager.h"
#include "grngame/core/app.h"
#include "grngame/data/file.h"
#include "grngame/platform/paths.h"
#include "grngame/utils/string_compat.h"
#include "module.h"
#include <cstdlib>
#include <scriptarray/scriptarray.h>
#include <scriptbuilder/scriptbuilder.h>
#include <scriptdictionary/scriptdictionary.h>
#include <scriptstdstring/scriptstdstring.h>

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

    auto *engine = static_cast<asIScriptEngine *>(g_app.angel_manager.engine);

    // 1. On utilise le Builder ici !
    CScriptBuilder builder;
    builder.SetIncludeCallback(LoadModuleFnFromMemory, nullptr);

    int result = builder.StartNewModule(engine, module_name);
    if (result < 0)
    {
        LOG_ERROR("AngelScript StartNewModule failed for '%s'", module_name);
        free(module_name);
        return false;
    }

    // 2. On ajoute la section via le builder
    result = builder.AddSectionFromMemory(filename, static_cast<const char *>(asset->data), asset->size);
    if (result < 0)
    {
        LOG_ERROR("AngelScript AddSection failed for embedded '%s'", filename);
        free(module_name);
        return false;
    }

    // 3. On build via le builder
    result = builder.BuildModule();
    if (result < 0)
    {
        LOG_ERROR("AngelScript Build failed for embedded '%s'", filename);
        free(module_name);
        return false;
    }
    AngelCacheLifecycleFunctions(module_name);
    free(module_name);
    return true;
}

bool Interpret(const char *filename)
{
    char *module_name = FileStem(filename);
    char *path = PathFromExecutableDirectory(filename);

    uint32 file_size = 0;
    char *file_content = reinterpret_cast<char *>(ReturnFileData(path, &file_size));
    free(path);

    if (!file_content)
    {
        LOG_ERROR("Failed to read script '%s'", filename);
        free(module_name);
        return false;
    }

    auto *engine = static_cast<asIScriptEngine *>(g_app.angel_manager.engine);

    // 1. On utilise le Builder ici !
    CScriptBuilder builder;
    builder.SetIncludeCallback(LoadModuleFn, nullptr);

    int result = builder.StartNewModule(engine, module_name);
    if (result < 0)
    {
        LOG_ERROR("AngelScript StartNewModule failed for '%s'", module_name);
        free(file_content);
        free(module_name);
        return false;
    }

    // 2. On passe la taille exacte au builder pour éviter les bugs de lecture
    result = builder.AddSectionFromMemory(filename, file_content, file_size);
    free(file_content);

    if (result < 0)
    {
        LOG_ERROR("AngelScript AddSection failed for '%s'", filename);
        free(module_name);
        return false;
    }

    // 3. On build via le builder
    result = builder.BuildModule();
    if (result < 0)
    {
        LOG_ERROR("AngelScript Build failed for '%s'", filename);
        free(module_name);
        return false;
    }
    AngelCacheLifecycleFunctions(module_name);
    free(module_name);
    return true;
}

// temp for testing
static void AS_PrintCallback(const std::string &msg)
{
    LOG_INFO("%s", msg.c_str());
}
static void AS_EngineStop()
{
    EngineSetStop();
}

bool AngelInit(const char *filename)
{
    asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
    if (!engine)
    {
        LOG_ERROR("Failed to create AngelScript engine");
        return false;
    }

    RegisterStdString(engine);
    RegisterScriptArray(engine, true);
    RegisterScriptDictionary(engine);

    // temp for test
    int r = engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(AS_PrintCallback), asCALL_CDECL);
    if (r < 0)
        LOG_ERROR("AngelScript : impossible to save print function");

    engine->SetDefaultNamespace("Exit");
    int r1 = engine->RegisterGlobalFunction("void engine_stop()", asFUNCTION(AS_EngineStop), asCALL_CDECL);
    if (r1 < 0)
        LOG_ERROR("AngelScript : impossible to save engine stop function");
    engine->SetDefaultNamespace("");

    // Message callback globale au moteur
    engine->SetMessageCallback(asFUNCTION(MessageCallback), nullptr, asCALL_CDECL);

    g_app.angel_manager.engine = reinterpret_cast<void *>(engine);

    // Plus de trace de CScriptBuilder ici, il est géré dans Interpret()
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

    RegisterStdString(engine);
    RegisterScriptArray(engine, true);
    RegisterScriptDictionary(engine);

    int r = engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(AS_PrintCallback), asCALL_CDECL);
    if (r < 0)
        LOG_ERROR("AngelScript : impossible to save print function");

    engine->SetDefaultNamespace("Exit");
    int r1 = engine->RegisterGlobalFunction("void engine_stop()", asFUNCTION(AS_EngineStop), asCALL_CDECL);
    if (r1 < 0)
        LOG_ERROR("AngelScript : impossible to save engine stop function");
    engine->SetDefaultNamespace("");

    engine->SetMessageCallback(asFUNCTION(MessageCallback), nullptr, asCALL_CDECL);

    g_app.angel_manager.engine = reinterpret_cast<void *>(engine);

    // Pareil, le Builder est géré dans InterpretFromMemory()
    return InterpretFromMemory(filename);
}

// ... AngelFree et AngelReload inchangés ...
bool AngelFree()
{
    FreeCallbacks();
    if (auto *engine = static_cast<asIScriptEngine *>(g_app.angel_manager.engine))
    {
        engine->ShutDownAndRelease();
        g_app.angel_manager.engine = nullptr;
    }
    return true;
}

bool AngelReload(const char *filename)
{
    AngelFree();
    return AngelInit(filename);
}