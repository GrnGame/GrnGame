#include "json_read.h"
#include "file.h"
#include "grngame/dev/logging.h"
#include "grngame/math/types.h"
#include "grngame/platform/paths.h"
#include "grngame/utils/attributes.h"
#include "grngame/utils/clear.h"
#include "grngame/utils/string_compat.h"
#include "kvec.h"
#include "yyjson.h"
#include "grngame/core/app.h"
#include <stdlib.h>
#include <string.h>

static bool ReadConfigValue(const JsonDoc *json, const char *key, JsonValue *value)
{
    *value = JsonReadValue(json, key);
    return value->type != JSONV_VOID;
}

static bool ReadConfigNumber(const JsonDoc *json, const char *key, float64 *out)
{
    JsonValue value = JsonReadValue(json, key);
    if (value.type == JSONV_VOID)
    {
        LOG_ERROR("Missing required config parameter '%s' in %s", key, CONFIG_FILE);
        return false;
    }
    if (value.type != JSONV_INTEGER && value.type != JSONV_FLOAT)
    {
        LOG_ERROR("Invalid type for config parameter '%s' in %s: expected a number", key, CONFIG_FILE);
        JsonValueFree(&value);
        return false;
    }
    *out = (value.type == JSONV_INTEGER) ? (float64)value.value.i : value.value.f;
    JsonValueFree(&value);
    return true;
}

static bool ReadConfigBool(const JsonDoc *json, const char *key, bool *out)
{
    JsonValue value = JsonReadValue(json, key);
    if (value.type == JSONV_VOID)
    {
        LOG_ERROR("Missing required config parameter '%s' in %s", key, CONFIG_FILE);
        return false;
    }
    if (value.type != JSONV_BOOL)
    {
        LOG_ERROR("Invalid type for config parameter '%s' in %s: expected a boolean", key, CONFIG_FILE);
        JsonValueFree(&value);
        return false;
    }
    *out = value.value.b;
    JsonValueFree(&value);
    return true;
}

static bool ReadConfigText(const JsonDoc *json, const char *key, char **out)
{
    JsonValue value = JsonReadValue(json, key);
    if (value.type == JSONV_VOID)
    {
        LOG_ERROR("Missing required config parameter '%s' in %s", key, CONFIG_FILE);
        return false;
    }
    if (value.type != JSONV_TEXT)
    {
        LOG_ERROR("Invalid type for config parameter '%s' in %s: expected a string", key, CONFIG_FILE);
        JsonValueFree(&value);
        return false;
    }
    *out = value.value.s;
    return true;
}

static bool ReadConfigTextArray(const JsonDoc *json, const char *key, string_vec_t *out)
{
    JsonValue value = JsonReadValue(json, key);
    if (value.type == JSONV_VOID)
    {
        LOG_ERROR("Missing required config parameter '%s' in %s", key, CONFIG_FILE);
        return false;
    }
    if (value.type != JSONV_TEXT_ARRAY && value.type != JSONV_EMPTY_ARRAY)
    {
        LOG_ERROR("Invalid type for config parameter '%s' in %s: expected an array of strings", key, CONFIG_FILE);
        JsonValueFree(&value);
        return false;
    }
    if (value.type == JSONV_EMPTY_ARRAY)
        kv_init(*out);
    else
        *out = value.value.sa;
    return true;
}

AppInfo *ParseConfig(const unsigned char *memory, uint32 size)
{
    AppInfo *info = malloc(sizeof(AppInfo));
    CLEAR_PTR(info, 0);

    char *language = NULL;
    float64 num = 0.0;

    JsonDoc json = memory == NULL ? JsonOpenFromDirectory(CONFIG_FILE) : JsonOpenFromMemory(CONFIG_FILE, memory, size);

    if (UNLIKELY(json.doc == NULL))
    {
        LOG_ERROR("Failed to open or parse configuration file %s", CONFIG_FILE);
        free(info);
        return NULL;
    }

    if (!ReadConfigNumber(&json, "fps", &num))
        goto invalid;
    info->fps = (uint16)num;

    if (!ReadConfigNumber(&json, "windowWidth", &num))
        goto invalid;
    info->window_width = (float32)num;

    if (!ReadConfigNumber(&json, "windowHeight", &num))
        goto invalid;
    info->window_height = (float32)num;

    if (!ReadConfigNumber(&json, "universeWidth", &num))
        goto invalid;
    info->window_universe_width = (uint16)num;

    if (!ReadConfigNumber(&json, "universeHeight", &num))
        goto invalid;
    info->window_universe_height = (uint16)num;

    if (!ReadConfigBool(&json, "resizable", &info->window_resizable))
        goto invalid;

    if (!ReadConfigBool(&json, "fullscreen", &info->window_fullscreen))
        goto invalid;

    if (!ReadConfigBool(&json, "maximised", &info->window_maximised))
        goto invalid;

    if (!ReadConfigBool(&json, "bordered", &info->bordered))
        goto invalid;

    if (!ReadConfigBool(&json, "forceUniverseScale", &info->force_universe_scale))
        goto invalid;

    if (!ReadConfigBool(&json, "enable_palette", &info->enable_palette))
        goto invalid;

    if (!ReadConfigText(&json, "name", &info->name))
        goto invalid;

    if (!ReadConfigText(&json, "version", &info->version))
        goto invalid;

    if (!ReadConfigText(&json, "company", &info->company))
        goto invalid;

    if (!ReadConfigText(&json, "language", &language))
        goto invalid;

    if (strcmp("dascript", language) == 0)
    {
        info->scripting_langage = DAS_SCRIPT;
    }
    else if (strcmp("python", language) == 0)
    {
        info->scripting_langage = PY_SCRIPT;
    }
    else
    {
        LOG_WARNING("Unsupported scripting language '%s' in %s, defaulting to python", language, CONFIG_FILE);
        info->scripting_langage = PY_SCRIPT;
    }
    free(language);
    language = NULL;

    if (!ReadConfigTextArray(&json, "palette", &info->palette))
        goto invalid;

    JsonClose(&json);
    return info;

invalid:
    free(language);
    free(info->name);
    free(info->version);
    free(info->company);
    JsonClose(&json);
    free(info);
    return NULL;
}