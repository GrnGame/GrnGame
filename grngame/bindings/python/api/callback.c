#include "grngame/bindings/python/api/callback.h"

#include "grngame/dev/logging.h"
#include "grngame/utils/attributes.h"
#include "interpretor.h"
#include "pocketpy/objects/base.h"
#include "pocketpy/pocketpy.h"

typedef struct
{
    bool set;
    py_TValue func;
} PyCallbackSlot;

static PyCallbackSlot on_callback[MAX_CALLBACKS] = {0};

bool PyScriptStoreCallback(uint16 name, py_Ref func)
{
    if (UNLIKELY(name >= MAX_CALLBACKS))
    {
        LOG_ERROR("Python: callback id %u out of range (max %d)", (unsigned)name, MAX_CALLBACKS - 1);
        return false;
    }
    if (func == NULL || !py_callable(func))
    {
        LOG_ERROR("Python: callback id %u: value is not callable", (unsigned)name);
        return false;
    }
    on_callback[name].func = *func;
    on_callback[name].set = true;
    return true;
}

void PyScriptClearCallbacks(void)
{
    for (int i = 0; i < MAX_CALLBACKS; i++)
        on_callback[i].set = false;
}

static bool CallFunc(py_Ref func, int argc, py_Ref argv)
{
    if (!py_call(func, argc, argv))
    {
        py_printexc();
        py_clearexc(NULL);
        return false;
    }
    return true;
}

static bool CallOptional(const char *func_name, int argc, py_Ref argv)
{
    py_GlobalRef func = py_getglobal(py_name(func_name));
    if (func == NULL || !py_callable(func))
        return true;
    return CallFunc(func, argc, argv);
}

static bool PushArg(const CallbackArg *arg)
{
    py_Ref slot = py_pushtmp();
    switch (arg->type)
    {
    case CB_ARG_NUM:
        py_newfloat(slot, (double)arg->as.num);
        return true;
    case CB_ARG_BOOL:
        py_newbool(slot, arg->as.boolean);
        return true;
    case CB_ARG_STRING:
        if (arg->as.string != NULL)
            py_newstr(slot, (const char *)arg->as.string);
        else
            py_newnone(slot);
        return true;
    default:
        py_newnone(slot);
        return false;
    }
}

bool PyScriptRegisterCallback(uint16 name, const char *funcDecl)
{
    if (funcDecl == NULL)
        return false;

    py_GlobalRef func = py_getglobal(py_name(funcDecl));
    if (func == NULL)
    {
        LOG_ERROR("Python: cannot register callback '%s', function not found in script", funcDecl);
        return false;
    }
    return PyScriptStoreCallback(name, func);
}

bool PyScriptCallBack(CallbackArg *args, uint16 name)
{
    if (UNLIKELY(name >= MAX_CALLBACKS || !on_callback[name].set))
    {
        LOG_ERROR("Python: callback id %u is not registered", (unsigned)name);
        return false;
    }

    int argc = 0;
    py_Ref argv = NULL;
    bool ok = true;

    if (args != NULL)
    {
        while (args[argc].type != CB_ARG_END)
        {

            bool pushed_ok = PushArg(&args[argc]);
            if (argc == 0)
                argv = py_peek(-1);
            argc++;
            if (UNLIKELY(!pushed_ok))
            {
                LOG_ERROR("Python: callback id %u: unknown argument type at index %d", (unsigned)name, argc - 1);
                ok = false;
                break;
            }
        }
    }

    if (ok)
        ok = CallFunc(&on_callback[name].func, argc, argv);

    for (int i = 0; i < argc; i++)
        py_pop();

    return ok;
}

bool PyScriptCallOnStart()
{
    return CallOptional("on_start", 0, NULL);
}

bool PyScriptCallOnUpdate(float32 delta)
{
    py_Ref arg = py_getreg(0);
    py_newfloat(arg, (double)delta);
    return CallOptional("on_update", 1, arg);
}

bool PyScriptCallOnFixedUpdate(float32 delta)
{
    py_Ref arg = py_getreg(0);
    py_newfloat(arg, (double)delta);
    return CallOptional("on_fixed_update", 1, arg);
}

bool PyScriptCallOnRender()
{
    return CallOptional("on_render", 0, NULL);
}

bool PyScriptCallOnDestroy()
{
    return CallOptional("on_destroy", 0, NULL);
}