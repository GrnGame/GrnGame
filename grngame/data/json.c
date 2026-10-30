#include "json.h"
#include "SDL3/SDL_stdinc.h"
#include "file.h"
#include "grngame/assets/embedded_file_manager.h"
#include "grngame/assets/load.h"
#include "grngame/bindings/wren/wren_api.h"
#include "grngame/bindings/wren/wren_event.h"
#include "grngame/core/app.h"
#include "grngame/dev/logging.h"
#include "grngame/embedded/embedded_file_generator.h"
#include "grngame/input/input_data.h"
#include "grngame/math/types.h"
#include "grngame/platform/paths.h"
#include "grngame/utils/attributes.h"
#include "grngame/utils/clear.h"
#include "grngame/utils/string_compat.h"
#include "grngame/utils/time.h"
#include "kvec.h"
#include <stdlib.h>
#include <string.h>

COLD JsonManager JsonManagerCreate(void)
{
    return kh_init(JsonObjects);
}

static void JsonObjectDestroy(JsonObject *object)
{
    if (object->json != NULL)
    {
        yyjson_mut_doc_free(object->json);
        object->json = NULL;
    }
    free(object);
}

bool JsonExist(const char *key)
{
    char *key_ = PathFromExecutableDirectory(key);
    bool exist = FileExist(key_);
    free(key_);
    return exist;
}

bool JsonExistFromMemory(const char *key)
{
#ifndef GRNGAME_WASM
    return EmbeddedFileExists(key);
#else
    LOG_WARNING("Json isnt supported on the web");
    return false;
#endif
}

bool JsonCreate(const char *key, const char *text)
{
    if (!text)
        return false;

    char *path = PathFromExecutableDirectory(key);
    bool result = WriteFileData(path, text, strlen(text), false);
    if (!result)
    {
        LOG_ERROR("Failed to create json file :%s", key);
    }
    free(path);
    return result;
}
bool JsonCreateFromMemory(const char *key, const char *text)
{
#ifndef GRNGAME_WASM
    uint64 size = strlen(text);
    if (!EmbeddedFilePut(key, text, size))
    {
        LOG_ERROR("Failed to create json from memory :%s", key);
        return false;
    }
    return true;
#else
    LOG_WARNING("Json isnt supported on the web");
    return false;
#endif
}

static void JsonObjectAdd(const char *key, JsonObject *value)
{
    int32 ret;
    JsonManager manager = g_app.json_manager;
    char *dup_key = strdup(key);

    khiter_t it = kh_put(JsonObjects, manager, dup_key, &ret);

    if (ret == 0)
    {
        // free the existing key and value
        free(dup_key);
        JsonObject *old = kh_value(manager, it);
        JsonObjectDestroy(old);
    }

    kh_value(manager, it) = value;
}

JsonObject *JsonObjectGet(const char *key)
{
    JsonManager manager = g_app.json_manager;
    khiter_t it = kh_get(JsonObjects, manager, key);

    if (it == kh_end(manager))
        return NULL;

    return kh_value(manager, it);
}

bool JsonObjectContains(const char *key)
{
    JsonManager manager = g_app.json_manager;
    return kh_get(JsonObjects, manager, key) != kh_end(manager);
}

bool OpenJsonObject(const char *path, uint64 min, uint64 max)
{
    char *path_ = PathFromExecutableDirectory(path);
    char *text = (char *)ReturnFileData(path_, NULL);
    free(path_);

    if (UNLIKELY(text == NULL))
    {
        LOG_ERROR("Impossible to get content from %s,did you create it?", path);
        return false;
    }

    yyjson_doc *parsed = yyjson_read(text, strlen(text), 0);
    free(text);

    if (UNLIKELY(parsed == NULL))
    {
        LOG_ERROR("Impossitble to parse %s, is it a correct json format ?", path);
        return false;
    }

    yyjson_mut_doc *json = yyjson_doc_mut_copy(parsed, NULL);
    yyjson_doc_free(parsed);
    if (UNLIKELY(json == NULL))
    {
        LOG_ERROR("Impossible to allocate JSON document for %s", path);
        return false;
    }

    JsonObject *j = malloc(sizeof(JsonObject));
    CLEAR_PTR(j, 0);
    j->min = min;
    j->max = max;
    j->json = json;
    JsonObjectAdd(path, j);
    return true;
}

bool OpenJsonObjectFromMemory(const char *path, const unsigned char *text, uint64 min, uint64 max)
{
    yyjson_doc *parsed = yyjson_read((const char *)text, strlen((const char *)text), 0);
    if (parsed == NULL)
    {
        LOG_ERROR("Impossitble to parse %s, is it a correct json format ?", path);
        return false;
    }

    yyjson_mut_doc *json = yyjson_doc_mut_copy(parsed, NULL);
    yyjson_doc_free(parsed);
    if (UNLIKELY(json == NULL))
    {
        LOG_ERROR("Impossible to allocate JSON document for %s", path);
        return false;
    }

    JsonObject *j = malloc(sizeof(JsonObject));
    CLEAR_PTR(j, 0);
    j->json = json;
    j->min = min;
    j->max = max;
    JsonObjectAdd(path, j);
    return true;
}

static yyjson_mut_val *JsonNavigateToParent(yyjson_mut_doc *doc, char *path_copy, char **out_leaf, bool create)
{
    char *saveptr = NULL;
    char *token = strtok_r(path_copy, ".", &saveptr);
    if (UNLIKELY(token == NULL))
    {
        return NULL;
    }

    yyjson_mut_val *current = yyjson_mut_doc_get_root(doc);
    char *next_token = NULL;

    while ((next_token = strtok_r(NULL, ".", &saveptr)) != NULL)
    {
        yyjson_mut_val *child = yyjson_mut_obj_get(current, token);

        if (child == NULL)
        {
            if (!create)
            {
                return NULL;
            }
            child = yyjson_mut_obj(doc);
            yyjson_mut_val *child_key = yyjson_mut_strcpy(doc, token);
            if (child == NULL || child_key == NULL ||
                !yyjson_mut_obj_add(current, child_key, child))
            {
                return NULL;
            }
        }
        else if (UNLIKELY(yyjson_mut_get_type(child) != YYJSON_TYPE_OBJ))
        {
            return NULL;
        }

        current = child;
        token = next_token;
    }

    *out_leaf = token;
    return current;
}

static yyjson_mut_val *JsonResolve(const char *fileKey, const char *key, char **out_leaf, bool create)
{
    JsonObject *entry = JsonObjectGet(fileKey);
    if (UNLIKELY(entry == NULL))
    {
        return NULL;
    }

    static THREAD_LOCAL char path_copy[JSON_PATH_MAX_LEN];
    if (UNLIKELY(strlen(key) >= JSON_PATH_MAX_LEN))
    {
        return NULL;
    }
    strncpy(path_copy, key, JSON_PATH_MAX_LEN - 1);
    path_copy[JSON_PATH_MAX_LEN - 1] = '\0';

    return JsonNavigateToParent(entry->json, path_copy, out_leaf, create);
}

bool JsonGetNumber(const char *fileKey, const char *key, float64 *out)
{
    char *leaf = NULL;
    yyjson_mut_val *parent = JsonResolve(fileKey, key, &leaf, false);
    if (UNLIKELY(parent == NULL))
        return false;

    yyjson_mut_val *item = yyjson_mut_obj_get(parent, leaf);
    if (UNLIKELY(item == NULL || !yyjson_mut_is_num(item)))
    {
        return false;
    }

    *out = yyjson_mut_get_num(item);
    return true;
}

bool JsonGetBool(const char *fileKey, const char *key, bool *out)
{
    char *leaf = NULL;
    yyjson_mut_val *parent = JsonResolve(fileKey, key, &leaf, false);
    if (UNLIKELY(parent == NULL))
        return false;

    yyjson_mut_val *item = yyjson_mut_obj_get(parent, leaf);
    if (UNLIKELY(item == NULL || !yyjson_mut_is_bool(item)))
    {
        return false;
    }

    *out = yyjson_mut_get_bool(item);
    return true;
}

bool JsonGetString(const char *fileKey, const char *key, const char **out)
{
    char *leaf = NULL;
    yyjson_mut_val *parent = JsonResolve(fileKey, key, &leaf, false);
    if (UNLIKELY(parent == NULL))
        return false;

    yyjson_mut_val *item = yyjson_mut_obj_get(parent, leaf);
    if (UNLIKELY(item == NULL || !yyjson_mut_is_str(item)))
    {
        return false;
    }

    *out = yyjson_mut_get_str(item);
    return true;
}

bool JsonGetNumberArray(const char *fileKey, const char *key, float64_vec_t *out)
{
    kv_init(*out);

    char *leaf = NULL;
    yyjson_mut_val *parent = JsonResolve(fileKey, key, &leaf, false);
    if (UNLIKELY(parent == NULL))
        return false;

    yyjson_mut_val *array = yyjson_mut_obj_get(parent, leaf);
    if (UNLIKELY(array == NULL || yyjson_mut_get_type(array) != YYJSON_TYPE_ARR))
    {
        return false;
    }

    float64_vec_t vec;
    kv_init(vec);

    int64 count = (int64)yyjson_mut_arr_size(array);
    kv_resize(float64, vec, count);

    for (int64 i = 0; i < count; ++i)
    {
        yyjson_mut_val *item = yyjson_mut_arr_get(array, (size_t)i);
        if (UNLIKELY(!yyjson_mut_is_num(item)))
        {
            kv_destroy(vec);
            return false;
        }

        kv_push(float64, vec, yyjson_mut_get_num(item));
    }

    *out = vec;
    return true;
}

typedef struct
{
    char *key;
    char *text;
} FileWriteJob;

static void FileWriteJobRun(void *user_data)
{
    FileWriteJob *job = user_data;

    WriteFileData(job->key, job->text, strlen(job->text), false);

    free(job->key);
    free(job->text);
    free(job);
}

bool JsonSaveObject(const char *fileKey)
{
    JsonObject *entry = JsonObjectGet(fileKey);
    if (UNLIKELY(entry == NULL))
    {
        LOG_ERROR("Cant save json because %s doesnt exist", fileKey);
        return false;
    }

    FileWriteJob *job = malloc(sizeof(*job));
    if (job == NULL)
    {
        LOG_ERROR("Failed to allocate JSON save job for %s", fileKey);
        return false;
    }

    job->key = PathFromExecutableDirectory(fileKey);
    job->text = yyjson_mut_write(entry->json, YYJSON_WRITE_PRETTY, NULL);
    if (job->key == NULL || job->text == NULL)
    {
        LOG_ERROR("Failed to serialize JSON file %s", fileKey);
        free(job->key);
        free(job->text);
        free(job);
        return false;
    }

    // Safe asynchronous operation: the file is read once during initialization,
    // and writes occur at most once per second.
    ThreadManagerPush(FileWriteJobRun, job);

    return true;
}

bool JsonSaveObjectFromMemory(const char *fileKey)
{
#ifndef GRNGAME_WASM
    JsonObject *entry = JsonObjectGet(fileKey);
    if (UNLIKELY(entry == NULL))
    {
        LOG_ERROR("Cant save json because isnt open :%s", fileKey);
        return false;
    }

    char *text = yyjson_mut_write(entry->json, YYJSON_WRITE_PRETTY, NULL);
    if (text == NULL)
    {
        LOG_ERROR("Failed to serialize JSON file %s", fileKey);
        return false;
    }

    bool result = EmbeddedFilePutDisk(fileKey, text, strlen(text));

    free(text);

    return result;
#else
    LOG_WARNING("Json isnt supported on the web");
    return false;
#endif
}

bool JsonGetBoolArray(const char *fileKey, const char *key, bool_vec_t *out)
{
    kv_init(*out);

    char *leaf = NULL;
    yyjson_mut_val *parent = JsonResolve(fileKey, key, &leaf, false);
    if (UNLIKELY(parent == NULL))
        return false;

    yyjson_mut_val *array = yyjson_mut_obj_get(parent, leaf);
    if (UNLIKELY(array == NULL || yyjson_mut_get_type(array) != YYJSON_TYPE_ARR))
    {
        return false;
    }

    bool_vec_t vec;
    kv_init(vec);

    int64 count = (int64)yyjson_mut_arr_size(array);
    kv_resize(bool, vec, count);

    for (int64 i = 0; i < count; ++i)
    {
        yyjson_mut_val *item = yyjson_mut_arr_get(array, (size_t)i);
        if (UNLIKELY(!yyjson_mut_is_bool(item)))
        {
            kv_destroy(vec);
            return false;
        }

        kv_push(bool, vec, yyjson_mut_get_bool(item));
    }

    *out = vec;
    return true;
}

// need to free every string + the array
bool JsonGetStringArray(const char *fileKey, const char *key, string_vec_t *out)
{
    kv_init(*out);

    char *leaf = NULL;
    yyjson_mut_val *parent = JsonResolve(fileKey, key, &leaf, false);
    if (UNLIKELY(parent == NULL))
        return false;

    yyjson_mut_val *array = yyjson_mut_obj_get(parent, leaf);
    if (UNLIKELY(array == NULL || yyjson_mut_get_type(array) != YYJSON_TYPE_ARR))
    {
        return false;
    }

    int64 count = (int64)yyjson_mut_arr_size(array);
    kv_resize(char *, *out, count);

    for (int64 i = 0; i < count; ++i)
    {
        yyjson_mut_val *item = yyjson_mut_arr_get(array, (size_t)i);
        if (UNLIKELY(!yyjson_mut_is_str(item)))
        {

            for (uint64 j = 0; j < kv_size(*out); ++j)
                free(kv_A(*out, j));

            kv_destroy(*out);
            kv_init(*out);

            return false;
        }

        kv_push(char *, *out, strdup(yyjson_mut_get_str(item)));
    }

    return true;
}

static bool JsonCloseObject(const char *fileKey)
{
    JsonManager manager = g_app.json_manager;
    khiter_t it = kh_get(JsonObjects, manager, fileKey);

    if (UNLIKELY(it == kh_end(manager)))
    {
        LOG_ERROR("Cant close json object from memory :%s", fileKey);
        return false;
    }

    JsonObject *entry = kh_value(manager, it);

    yyjson_mut_doc_free(entry->json);
    entry->json = NULL;

    free((char *)kh_key(manager, it));

    kh_del(JsonObjects, manager, it);

    return true;
}

COLD void JsonManagerDestroy(JsonManager manager)
{
    for (khiter_t it = kh_begin(manager); it != kh_end(manager); ++it)
    {
        if (kh_exist(manager, it))
            JsonCloseObject(kh_key(manager, it));
    }

    kh_destroy(JsonObjects, manager);
}

void JsonSaveObjects(float64 budget)
{
    JsonManager manager = g_app.json_manager;
    uint64 current_time_sec = g_app.info.frame_count / (uint64)g_app.info.fps;
    uint64 start_ticks = TimeNow();

    float64 max_time_ms = budget * 0.8;

    for (khiter_t it = kh_begin(manager); it != kh_end(manager); ++it)
    {
        if (!kh_exist(manager, it))
            continue;

        JsonObject *json = kh_value(manager, it);
        if ((json->min > json->max) || (json->min == 0 && json->max == 0))
            continue;
        uint64 elapsed_sec = current_time_sec - json->last_save_time;

        bool is_mandatory = elapsed_sec >= json->max;
        bool should_save = is_mandatory || elapsed_sec >= json->min;

        if (!should_save)
            continue;

        uint64 elapsed_ms = TimeNow() - start_ticks;

        if (!is_mandatory && elapsed_ms >= max_time_ms)
            continue;

        const char *key = kh_key(manager, it);
        CallbackArg args[1] = {{.type = CB_ARG_STRING, .as.string = key}};
        CallWrenCallback(JSON_SAVE, args, 1);

        json->last_save_time = current_time_sec;
    }
}

bool WriteInJsonObject(const char *key, yyjson_mut_doc *object)
{
#ifndef GRNGAME_WASM
    JsonObject *entry = JsonObjectGet(key);
    if (UNLIKELY(entry == NULL))
    {
        LOG_ERROR("Cant write in json object because %s doesnt exist.did you forget to open it?", key);
        return false;
    }
    yyjson_mut_doc_free(entry->json);
    entry->json = object;
    return true;
#else
    LOG_WARNING("Json isnt supported on the web");
    return false;
#endif
}
