
#include "callback.h"
#include "grngame/core/app.h"
#include "grngame/dev/logging.h"
#include "grngame/utils/attributes.h"
#include <angelscript.h>

// dont use AngelManager for moment
// todo : set callbacks in AngelManger

static asIScriptFunction *g_callbacks[MAX_CALLBACKS] = {nullptr};
static asIScriptFunction *g_onStart = nullptr;
static asIScriptFunction *g_onUpdate = nullptr;
static asIScriptFunction *g_onFixedUpdate = nullptr;
static asIScriptFunction *g_onRender = nullptr;
static asIScriptFunction *g_onDestroy = nullptr;

void MessageCallback(const asSMessageInfo *msg, void *param)
{
    if (msg->type == asMSGTYPE_ERROR)
        LOG_ERROR("%s", msg->message);
    else if (msg->type == asMSGTYPE_WARNING)
        LOG_WARNING("%s", msg->message);
    else
    {
        LOG_INFO("%s", msg->message);
    }
}

bool AngelRegisterCallback(const uint16 name, const char *funcDecl)
{
    if (UNLIKELY(name >= MAX_CALLBACKS))
        return false;

    auto *engine = static_cast<asIScriptEngine *>(g_app.angel_manager.engine);

    const asIScriptModule *mod = engine->GetModule("main");

    asIScriptFunction *func = mod->GetFunctionByDecl(funcDecl);
    if (!func)
    {
        LOG_ERROR("AngelScript: Cannot find callback function '%s'", funcDecl);
        return false;
    }

    if (g_callbacks[name])
        g_callbacks[name]->Release();

    g_callbacks[name] = func;

    g_callbacks[name]->AddRef();

    return true;
}

bool AngelCallBack(CallbackArg *args, uint16 name)
{
    if (UNLIKELY(name >= MAX_CALLBACKS))
        return false;

    if (!g_callbacks[name])
    {
        LOG_WARNING("AngelScript: Cannot call callback function '%u'", name);
        return false;
    }

    auto *engine = static_cast<asIScriptEngine *>(g_app.angel_manager.engine);

    asIScriptContext *ctx = engine->RequestContext();
    ctx->Prepare(g_callbacks[name]);
    if (args)
    {
        switch (args->type)
        {
        case CB_ARG_NUM:
            ctx->SetArgDouble(0, args->as.num);
            break;
        case CB_ARG_BOOL:
            ctx->SetArgByte(0, args->as.boolean);
            break;
        case CB_ARG_STRING:
            ctx->SetArgObject(0, args->as.string);
            break;
        }
    }

    if (const int32 r = ctx->Execute(); r != asEXECUTION_FINISHED)
    {
        LOG_ERROR("AngelScript Error in CallCallBack for event ID %d", name);
    }

    engine->ReturnContext(ctx);

    return true;
}

void AngelCacheLifecycleFunctions(const char *moduleName)
{
    auto *engine = static_cast<asIScriptEngine *>(g_app.angel_manager.engine);
    asIScriptModule *mod = engine->GetModule(moduleName);

    g_onStart = mod->GetFunctionByDecl("void OnStart()");
    g_onUpdate = mod->GetFunctionByDecl("void OnUpdate(float)");
    g_onFixedUpdate = mod->GetFunctionByDecl("void OnFixedUpdate(float)");
    g_onRender = mod->GetFunctionByDecl("void OnRender()");
    g_onDestroy = mod->GetFunctionByDecl("void OnDestroy()");
}

static bool AngelCall(asIScriptFunction *g, const char *name, const float32 delta, const bool have_delta)
{
    if (!g)
    {
        LOG_WARNING("AngelScript: Cannot find main function '%s'", name);
        return false;
    }

    auto *engine = static_cast<asIScriptEngine *>(g_app.angel_manager.engine);
    asIScriptContext *ctx = engine->RequestContext();
    ctx->Prepare(g);
    if (have_delta)
    {
        ctx->SetArgFloat(0, delta);
    }
    int32 r = ctx->Execute();

    engine->ReturnContext(ctx);

    if (r != asEXECUTION_FINISHED)
    {
        LOG_ERROR("AngelScript Error executing OnRender");
        return false;
    }
    return true;
}

bool AngelCallOnStart()
{
    return AngelCall(g_onStart, "OnStart()", 0, false);
}

bool AngelCallOnUpdate(const float32 delta)
{
    return AngelCall(g_onUpdate, "OnUpdate()", delta, true);
}

bool AngelCallOnFixedUpdate(const float32 delta)
{
    return AngelCall(g_onFixedUpdate, "OnFixedUpdate()", delta, true);
}

bool AngelCallOnRender()
{
    return AngelCall(g_onRender, "OnRender()", 0, false);
}

bool AngelCallOnDestroy()
{
    return AngelCall(g_onDestroy, "OnDestroy()", 0, false);
}

void FreeCallbacks()
{
    for (auto &g_callback : g_callbacks)
    {
        if (g_callback)
        {
            g_callback->Release();
            g_callback = nullptr;
        }
    }
    if (g_onStart)
    {
        g_onStart->Release();
        g_onStart = nullptr;
    }
    if (g_onUpdate)
    {
        g_onUpdate->Release();
        g_onUpdate = nullptr;
    }
    if (g_onFixedUpdate)
    {
        g_onFixedUpdate->Release();
        g_onFixedUpdate = nullptr;
    }
    if (g_onRender)
    {
        g_onRender->Release();
        g_onRender = nullptr;
    }
    if (g_onDestroy)
    {
        g_onDestroy->Release();
        g_onDestroy = nullptr;
    }
}
