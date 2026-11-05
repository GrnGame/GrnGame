#pragma once

#include "grngame/math/types.h"
#include "grngame/utils/attributes.h"
#include "grngame/utils/macro.h"
#include "grngame/utils/memory.h"
#include <cJSON.h>
#include <khash.h>
#include <kvec.h>
#include <stdbool.h>

#define JSON_PATH_MAX_LEN 1024 * BUFFER_SPACE

typedef struct
{
    uint32 min;
    uint32 max;
    uint32 last_save_time;
    cJSON *json;
    bool embedded; // Present in embedded database, only available on target
} JsonObject;

KHASH_MAP_INIT_STR(JsonObjects, JsonObject *)

typedef khash_t(JsonObjects) * JsonManager;

COLD JsonManager JsonManagerCreate(void);
COLD void JsonManagerDestroy(JsonManager manager);

JsonObject *JsonObjectGet(const char *key);
bool JsonObjectContains(const char *key);

/* Open JSON from the filesystem. */
bool OpenJsonObject(const char *key, uint32 min, uint32 max);

/* Open JSON from embedded memory/database. */
bool OpenJsonObjectFromMemory(const char *path, const unsigned char *text, uint32 min, uint32 max);

/* Save JSON asynchronously to the filesystem. */
bool JsonSaveObject(const char *fileKey);

/* Save JSON objects using the configured time budget. */
void JsonSaveObjects(float64 budget);

/* Save JSON directly to the embedded database. */
bool JsonSaveObjectFromMemory(const char *fileKey);

/* Create a JSON file on the filesystem. */
bool JsonCreate(const char *key, const char *text);

/* Create or overwrite a JSON entry in the embedded database. */
bool JsonCreateFromMemory(const char *key, const char *text);

bool JsonExist(const char *key);

bool JsonExistFromMemory(const char *key);
