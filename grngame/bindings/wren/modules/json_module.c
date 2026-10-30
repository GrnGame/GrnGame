#include "grngame/assets/asset_manager.h"
#include "grngame/assets/load.h"
#include "grngame/bindings/wren/wren_api.h"
#include "grngame/core/app.h"
#include "grngame/data/json.h"
#include "grngame/dev/logging.h"
#include "grngame/utils/memory.h"
#include "wren.h"

#define MAP_KEY_SIZE 255 * BUFFER_SPACE

// WREN -> C (read)
// Fixed slots (4) + WrenHandle to prevent VM memory bloat
// Handles prevent slot overwriting during recursion
#define SLOT_CONTAINER 1
#define SLOT_KEY 2
#define SLOT_VALUE 3

// C -> WREN (write)
// Slot stacking via baseslot
// Prevents Wren's GC from destroying parent objects
// Protects unlinked objects during allocation
#define JSON_MAP_REQ_SLOTS(base) ((base) + 3)
#define JSON_MAP_KEY_SLOT(base) ((base) + 1)
#define JSON_MAP_VAL_SLOT(base) ((base) + 2)

#define JSON_LIST_REQ_SLOTS(base) ((base) + 2)
#define JSON_LIST_VAL_SLOT(base) ((base) + 1)

static yyjson_mut_val *JsonParseValueFromSlot(WrenVM *vm, yyjson_mut_doc *doc, int32 valueSlot);
static void JsonPushValueToSlot(WrenVM *vm, yyjson_mut_val *item, int32 slot);

static yyjson_mut_val *JsonParseMap(WrenVM *vm, yyjson_mut_doc *doc, WrenHandle *mapHandle)
{
    yyjson_mut_val *object = yyjson_mut_obj(doc);
    if (object == NULL)
        return NULL;

    wrenEnsureSlots(vm, 4);
    wrenSetSlotHandle(vm, SLOT_CONTAINER, mapHandle);
    const int32 capacity = wrenGetMapCapacity(vm, SLOT_CONTAINER);

    for (int32 i = 0; i < capacity; i++)
    {
        wrenSetSlotHandle(vm, SLOT_CONTAINER, mapHandle);

        if (!wrenGetMapKey(vm, SLOT_CONTAINER, i, SLOT_KEY))
            continue;

        if (wrenGetSlotType(vm, SLOT_KEY) != WREN_TYPE_STRING)
            continue;

        char key[MAP_KEY_SIZE];
        snprintf(key, sizeof(key), "%s", wrenGetSlotString(vm, SLOT_KEY));

        wrenGetMapValue(vm, SLOT_CONTAINER, SLOT_KEY, SLOT_VALUE);
        yyjson_mut_val *value = JsonParseValueFromSlot(vm, doc, SLOT_VALUE);
        yyjson_mut_val *json_key = yyjson_mut_strcpy(doc, key);
        if (value == NULL || json_key == NULL || !yyjson_mut_obj_add(object, json_key, value))
            return NULL;
    }

    return object;
}

static yyjson_mut_val *JsonParseList(WrenVM *vm, yyjson_mut_doc *doc, WrenHandle *listHandle)
{
    yyjson_mut_val *array = yyjson_mut_arr(doc);
    if (array == NULL)
        return NULL;

    wrenEnsureSlots(vm, 4);

    wrenSetSlotHandle(vm, SLOT_CONTAINER, listHandle);
    const int32 count = wrenGetListCount(vm, SLOT_CONTAINER);

    for (int32 i = 0; i < count; i++)
    {
        wrenSetSlotHandle(vm, SLOT_CONTAINER, listHandle);

        wrenGetListElement(vm, SLOT_CONTAINER, i, SLOT_VALUE);
        yyjson_mut_val *value = JsonParseValueFromSlot(vm, doc, SLOT_VALUE);
        if (value == NULL || !yyjson_mut_arr_append(array, value))
            return NULL;
    }

    return array;
}

static yyjson_mut_val *JsonParseValueFromSlot(WrenVM *vm, yyjson_mut_doc *doc, int32 valueSlot)
{
    switch (wrenGetSlotType(vm, valueSlot))
    {
    case WREN_TYPE_NULL:
        return yyjson_mut_null(doc);

    case WREN_TYPE_BOOL:
        return yyjson_mut_bool(doc, wrenGetSlotBool(vm, valueSlot));

    case WREN_TYPE_NUM:
        return yyjson_mut_real(doc, wrenGetSlotDouble(vm, valueSlot));

    case WREN_TYPE_STRING:
        return yyjson_mut_strcpy(doc, wrenGetSlotString(vm, valueSlot));

    case WREN_TYPE_MAP: {
        WrenHandle *handle = wrenGetSlotHandle(vm, valueSlot);
        yyjson_mut_val *result = JsonParseMap(vm, doc, handle);
        wrenReleaseHandle(vm, handle);
        return result;
    }

    case WREN_TYPE_LIST: {
        WrenHandle *handle = wrenGetSlotHandle(vm, valueSlot);
        yyjson_mut_val *result = JsonParseList(vm, doc, handle);
        wrenReleaseHandle(vm, handle);
        return result;
    }

    case WREN_TYPE_FOREIGN:
    case WREN_TYPE_UNKNOWN:
    default:
        return yyjson_mut_null(doc);
    }
}

static yyjson_mut_doc *JsonDocumentFromSlot(WrenVM *vm, int32 valueSlot)
{
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    if (doc == NULL)
        return NULL;

    yyjson_mut_val *root = JsonParseValueFromSlot(vm, doc, valueSlot);
    if (root == NULL)
    {
        yyjson_mut_doc_free(doc);
        return NULL;
    }

    yyjson_mut_doc_set_root(doc, root);
    return doc;
}

static void JsonPushObjectToSlot(WrenVM *vm, yyjson_mut_val *object, int32 base_slot)
{
    wrenEnsureSlots(vm, JSON_MAP_REQ_SLOTS(base_slot));
    wrenSetSlotNewMap(vm, base_slot);

    const int32 keySlot = JSON_MAP_KEY_SLOT(base_slot);
    const int32 valueSlot = JSON_MAP_VAL_SLOT(base_slot);

    yyjson_mut_obj_iter iter = yyjson_mut_obj_iter_with(object);
    yyjson_mut_val *key;
    while ((key = yyjson_mut_obj_iter_next(&iter)) != NULL)
    {
        yyjson_mut_val *item = yyjson_mut_obj_iter_get_val(key);
        wrenSetSlotString(vm, keySlot, yyjson_mut_get_str(key));

        JsonPushValueToSlot(vm, item, valueSlot);

        wrenSetMapValue(vm, base_slot, keySlot, valueSlot);
    }
}

static void JsonPushArrayToSlot(WrenVM *vm, yyjson_mut_val *array, int32 base_slot)
{
    wrenEnsureSlots(vm, JSON_LIST_REQ_SLOTS(base_slot));
    wrenSetSlotNewList(vm, base_slot);

    const int32 valueSlot = JSON_LIST_VAL_SLOT(base_slot);

    int32 index = 0;
    const size_t count = yyjson_mut_arr_size(array);
    for (size_t i = 0; i < count; ++i)
    {
        yyjson_mut_val *item = yyjson_mut_arr_get(array, i);
        JsonPushValueToSlot(vm, item, valueSlot);

        wrenInsertInList(vm, base_slot, index, valueSlot);
        index++;
    }
}

static void JsonPushValueToSlot(WrenVM *vm, yyjson_mut_val *item, int32 slot)
{
    if (item == NULL)
    {
        wrenSetSlotNull(vm, slot);
        return;
    }
    switch (yyjson_mut_get_type(item))
    {
    case YYJSON_TYPE_NULL:
        wrenSetSlotNull(vm, slot);
        break;

    case YYJSON_TYPE_BOOL:
        wrenSetSlotBool(vm, slot, yyjson_mut_get_bool(item));
        break;

    case YYJSON_TYPE_NUM:
        wrenSetSlotDouble(vm, slot, yyjson_mut_get_num(item));
        break;

    case YYJSON_TYPE_STR:
        wrenSetSlotString(vm, slot, yyjson_mut_get_str(item));
        break;

    case YYJSON_TYPE_OBJ:
        JsonPushObjectToSlot(vm, item, slot);
        break;

    case YYJSON_TYPE_ARR:
        JsonPushArrayToSlot(vm, item, slot);
        break;

    default:
        wrenSetSlotNull(vm, slot);
        break;
    }
}

void open_json_file(WrenVM *vm)
{
    const char *key = wrenGetSlotString(vm, 1);
    uint64 min = (uint64)wrenGetSlotDouble(vm, 2);
    uint64 max = (uint64)wrenGetSlotDouble(vm, 3);
#ifndef GRNGAME_EMBED_ASSETS
    bool error = OpenJsonObject(key, min, max);
#else
    if (!EmbeddedFileExists(key))
    {
        LOG_ERROR("Failed to get %s in Assets.pak,you need to create the file first.", key);
        return;
    }
    const EmbeddedFile *asset = EmbeddedFileGet(key);
    bool error = OpenJsonObjectFromMemory(key, asset->data, min, max);
#endif
    wrenSetSlotBool(vm, 0, error);
}

void json_contains(WrenVM *vm)
{
    const char *key = wrenGetSlotString(vm, 1);

    bool contains = JsonObjectContains(key);
    wrenSetSlotBool(vm, 0, contains);
}

void json_get(WrenVM *vm)
{
    const char *key = wrenGetSlotString(vm, 1);

    JsonObject *object = JsonObjectGet(key);

    if (object == NULL)
    {
        LOG_ERROR("Failed to get Json content from %s.Did you forget to open Json ?", key);
        wrenSetSlotNull(vm, 0);
        return;
    }

    yyjson_mut_val *root = yyjson_mut_doc_get_root(object->json);

    JsonPushValueToSlot(vm, root, 0);
}

void json_set(WrenVM *vm)
{
    const char *key = wrenGetSlotString(vm, 1);
    yyjson_mut_doc *value = JsonDocumentFromSlot(vm, 2);
    if (value == NULL)
    {
        wrenSetSlotBool(vm, 0, false);
        return;
    }

    bool success = WriteInJsonObject(key, value);
    if (!success)
    {
        yyjson_mut_doc_free(value);
    }

    wrenSetSlotBool(vm, 0, success);
}

void json_save(WrenVM *vm)
{
    const char *key = wrenGetSlotString(vm, 1);
    bool success = false;
#ifndef GRNGAME_EMBED_ASSETS
    success = JsonSaveObject(key);
#else
    success = JsonSaveObjectFromMemory(key);

#endif
    wrenSetSlotBool(vm, 0, success);
}

void create_json_file(WrenVM *vm)
{
    const char *key = wrenGetSlotString(vm, 1);

    yyjson_mut_doc *value = JsonDocumentFromSlot(vm, 2);
    if (UNLIKELY(value == NULL))
    {
        wrenSetSlotBool(vm, 0, false);
        return;
    }

    char *text = yyjson_mut_write(value, YYJSON_WRITE_PRETTY, NULL);
    yyjson_mut_doc_free(value);
    if (UNLIKELY(text == NULL))
    {
        wrenSetSlotBool(vm, 0, false);
        return;
    }

#ifndef GRNGAME_EMBED_ASSETS
    bool success = JsonCreate(key, text);
#else
    bool success = JsonCreateFromMemory(key, text);
#endif

    free(text);

    wrenSetSlotBool(vm, 0, success);
}

void json_file_exist(WrenVM *vm)
{
    const char *key = wrenGetSlotString(vm, 1);
#ifndef GRNGAME_EMBED_ASSETS
    wrenSetSlotBool(vm, 0, JsonExist(key));
#else
    wrenSetSlotBool(vm, 0, JsonExistFromMemory(key));
#endif
}

void RegisterJsonModule()
{
    const char *module = "std/wren/data/json";
    const char *cls = "Json";
    const bool is_static = true;
    RegisterMethod(module, cls, is_static, "create(_,_)", create_json_file);
    RegisterMethod(module, cls, is_static, "open(_,_,_)", open_json_file);
    RegisterMethod(module, cls, is_static, "contains(_)", json_contains);
    RegisterMethod(module, cls, is_static, "exist(_)", json_file_exist);
    RegisterMethod(module, cls, is_static, "get(_)", json_get);
    RegisterMethod(module, cls, is_static, "set(_,_)", json_set);
    RegisterMethod(module, cls, is_static, "save(_)", json_save);
}