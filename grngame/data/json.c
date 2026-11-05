#include "json.h"
#include "file.h"
#include "grngame/assets/embedded_file_manager.h"
#include "grngame/assets/load.h"
#include "grngame/bindings/api/callback.h"
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
        cJSON_Delete(object->json);
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
    bool result = WriteFileData(path, text, (uint32)strlen(text), false);
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
    uint32 size = (uint32)strlen(text);
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

bool OpenJsonObject(const char *path, uint32 min, uint32 max)
{
    char *path_ = PathFromExecutableDirectory(path);
    char *text = (char *)ReturnFileData(path_, NULL);
    free(path_);

    if (UNLIKELY(text == NULL))
    {
        LOG_ERROR("Impossible to get content from %s,did you create it?", path);
        return false;
    }

    cJSON *json = cJSON_Parse(text);
    free(text);

    if (UNLIKELY(json == NULL))
    {
        LOG_ERROR("Impossitble to parse %s, is it a correct json format ?", path);
        return false;
    }

    JsonObject *j = malloc(sizeof(JsonObject));
    CLEAR_PTR(j, 0);
    j->min = min;
    j->max = max;
    j->json = json;
    JsonObjectAdd(path, j);
    return 0;
}

// todo : add size parameter
bool OpenJsonObjectFromMemory(const char *path, const unsigned char *text, uint32 min, uint32 max)
{
#ifndef GRNGAME_WASM
    cJSON *json = cJSON_Parse((const char *)text);
    if (json == NULL)
    {
        LOG_ERROR("Impossitble to parse %s, is it a correct json format ?", path);
        return false;
    }

    JsonObject *j = malloc(sizeof(JsonObject));
    CLEAR_PTR(j, 0);
    j->json = json;
    j->min = min;
    j->max = max;
    JsonObjectAdd(path, j);
    return true;
#else
    LOG_WARNING("Json isnt supported on the web");
    return false;
#endif
}

typedef struct
{
    char *key;
    char *text;
} FileWriteJob;

static void FileWriteJobRun(void *user_data)
{
    FileWriteJob *job = user_data;

    WriteFileData(job->key, job->text, (uint32)strlen(job->key), false);

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

    job->key = PathFromExecutableDirectory(fileKey);
    job->text = cJSON_Print(entry->json);

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

    char *text = cJSON_Print(entry->json);

    bool result = EmbeddedFilePutDisk(fileKey, text, (uint32)strlen(text));

    free(text);

    return result;
#else
    LOG_WARNING("Json isnt supported on the web");
    return false;
#endif
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

    cJSON_Delete(entry->json);
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
    uint32 current_time_sec = g_app.info.frame_count / (uint32)g_app.info.fps;
    uint32 start_ticks = TimeNow();

    float64 max_time_ms = budget * 0.8;

    for (khiter_t it = kh_begin(manager); it != kh_end(manager); ++it)
    {
        if (!kh_exist(manager, it))
            continue;

        JsonObject *json = kh_value(manager, it);
        if ((json->min > json->max) || (json->min == 0 && json->max == 0))
            continue;
        uint32 elapsed_sec = current_time_sec - json->last_save_time;

        bool is_mandatory = elapsed_sec >= json->max;
        bool should_save = is_mandatory || elapsed_sec >= json->min;

        if (!should_save)
            continue;

        uint32 elapsed_ms = TimeNow() - start_ticks;

        if (!is_mandatory && elapsed_ms >= max_time_ms)
            continue;

        const char *key = kh_key(manager, it);
        CallbackArg args[1] = {{.type = CB_ARG_STRING, .as.string = key}};
        BindingsCallBack(args, JSON_SAVE);
        json->last_save_time = current_time_sec;
    }
}