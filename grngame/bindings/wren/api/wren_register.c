#include "grngame/assets/embedded_file_manager.h"
#include "grngame/bindings/wren/wren_api.h"
#include "grngame/data/file.h"
#include "grngame/dev/logging.h"
#include "grngame/platform/paths.h"
#include "grngame/utils/attributes.h"
#include "grngame/utils/memory.h"
#include "grngame/utils/string_compat.h"
#include "khash.h"
#include <stdio.h>
#include <stdlib.h>

#define KEY_SIZE (512 * BUFFER_SPACE)
#define MODULE_SIZE_MAX_NAME (256 * BUFFER_SPACE)

KHASH_MAP_INIT_STR(wren_method_map, WrenForeignMethodFn)
KHASH_MAP_INIT_STR(wren_class_map, WrenForeignClassMethods)

static khash_t(wren_method_map) *g_methods = NULL;
static khash_t(wren_class_map) *g_classes = NULL;

static void MakeMethodKey(char *buffer, uint32 max, const char *module, const char *className, bool isStatic,
                          const char *signature)
{
    snprintf(buffer, max, "%s:%s:%s:%s", module, className, isStatic ? "s" : "i", signature);
}

static void MakeClassKey(char *buffer, uint32 max, const char *module, const char *className)
{
    snprintf(buffer, max, "%s:%s", module, className);
}

void InitBindingSystem()
{
    if (!g_methods)
        g_methods = kh_init(wren_method_map);
    if (!g_classes)
        g_classes = kh_init(wren_class_map);
}

void RegisterMethod(const char *module, const char *className, bool isStatic, const char *signature,
                    WrenForeignMethodFn fn)
{
    char key[KEY_SIZE];
    MakeMethodKey(key, sizeof(key), module, className, isStatic, signature);

    int32 ret;
    char *dup_key = strdup(key);

    khint_t k = kh_put(wren_method_map, g_methods, dup_key, &ret);

    if (ret == 0)
    {
        free(dup_key);
    }

    kh_value(g_methods, k) = fn;
}

void RegisterClass_(const char *module, const char *className, WrenForeignMethodFn allocateFn,
                    WrenFinalizerFn finalizeFn)
{
    char key[KEY_SIZE];
    MakeClassKey(key, sizeof(key), module, className);

    int32 ret;
    char *dup_key = strdup(key);

    khint_t k = kh_put(wren_class_map, g_classes, dup_key, &ret);

    if (ret == 0)
    {
        free(dup_key);
    }

    WrenForeignClassMethods methods;
    methods.allocate = allocateFn;
    methods.finalize = finalizeFn;

    kh_value(g_classes, k) = methods;
}

WrenForeignMethodFn BindMethodFn(WrenVM *vm, const char *module, const char *className, bool isStatic,
                                 const char *signature)
{
    (void)vm;
    if (!g_methods)
        return NULL;

    char key[KEY_SIZE];
    MakeMethodKey(key, sizeof(key), module, className, isStatic, signature);

    khint_t k = kh_get(wren_method_map, g_methods, key);
    if (k != kh_end(g_methods))
    {
        return kh_value(g_methods, k);
    }

    return NULL;
}

WrenForeignClassMethods BindClassFn(WrenVM *vm, const char *module, const char *className)
{
    (void)vm;
    WrenForeignClassMethods fallback = {NULL, NULL};
    if (!g_classes)
        return fallback;

    char key[KEY_SIZE];
    MakeClassKey(key, sizeof(key), module, className);

    khint_t k = kh_get(wren_class_map, g_classes, key);
    if (k != kh_end(g_classes))
    {
        return kh_value(g_classes, k);
    }

    return fallback;
}

static void LoadModuleComplete(WrenVM *vm, const char *module, WrenLoadModuleResult result)
{
    (void)vm;
    (void)module;
    if (result.source)
    {
        free((void *)result.source);
    }
}

static char *AddEndOfString(char *data, uint32 size)
{
    uint32 sz = size;
    char *buf = malloc(sz + 1);
    memcpy(buf, data, sz);
    buf[sz] = '\0';
    return buf;
}

WrenLoadModuleResult LoadModuleFnFromMemory(WrenVM *vm, const char *name)
{
    (void)vm;
    WrenLoadModuleResult result = {0};
    result.onComplete = LoadModuleComplete;
    result.source = NULL;
    char filename[MODULE_SIZE_MAX_NAME];
    snprintf(filename, sizeof(filename), "%s.wren", name);

    const EmbeddedFile *asset = EmbeddedFileGet(filename);

    if (asset)
    {
        char *buf = AddEndOfString((char *)asset->data, asset->size);
        result.source = buf;
    }
    else
    {
        LOG_ERROR("Wren Import Error: Failed to find module '%s' in embedded files", name);
    }

    return result;
}

WrenLoadModuleResult LoadModuleFn(WrenVM *vm, const char *name)
{
    (void)vm;
    WrenLoadModuleResult result = {0};
    result.onComplete = LoadModuleComplete;
    result.source = NULL;

    char filename[MODULE_SIZE_MAX_NAME];
    snprintf(filename, sizeof(filename), "%s.wren", name);

    char script_path[MODULE_SIZE_MAX_NAME + 15];
    snprintf(script_path, sizeof(script_path), "%s", filename);

    char *path = PathFromExecutableDirectory(script_path);

    result.source = (char *)ReturnFileData(path, NULL);
    free(path);

    if (!result.source)
    {
        LOG_ERROR("Wren Import Error: Failed to find module '%s'", script_path);
    }

    return result;
}