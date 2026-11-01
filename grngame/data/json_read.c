#include "json_read.h"
#include "file.h"
#include "grngame/dev/logging.h"
#include "grngame/input/input_data.h"
#include "grngame/math/types.h"
#include "grngame/platform/paths.h"
#include "grngame/utils/attributes.h"
#include "grngame/utils/clear.h"
#include "grngame/utils/string_compat.h"
#include "kvec.h"
#include "yyjson.h"
#include <stdlib.h>
#include <string.h>

static bool JsonFillArray(yyjson_val *arr, JsonValue *out)
{
    bool all_bool = true, all_int = true, all_num = true, all_str = true;
    uint32 idx, max;
    yyjson_val *item;

    yyjson_arr_foreach(arr, idx, max, item)
    {
        all_bool = all_bool && yyjson_is_bool(item);
        all_int = all_int && yyjson_is_int(item);
        all_num = all_num && yyjson_is_num(item);
        all_str = all_str && yyjson_is_str(item);
    }

    if (yyjson_arr_size(arr) == 0)
    {
        out->type = JSONV_EMPTY_ARRAY;
        return true;
    }

    if (all_int)
    {
        out->type = JSONV_INTEGER_ARRAY;
        kv_init(out->value.ia);
        yyjson_arr_foreach(arr, idx, max, item) kv_push(int32, out->value.ia, yyjson_get_sint(item));
        return true;
    }
    if (all_bool)
    {
        out->type = JSONV_BOOL_ARRAY;
        kv_init(out->value.ba);
        yyjson_arr_foreach(arr, idx, max, item) kv_push(bool, out->value.ba, yyjson_get_bool(item));
        return true;
    }
    if (all_num)
    {
        out->type = JSONV_FLOAT_ARRAY;
        kv_init(out->value.fa);
        yyjson_arr_foreach(arr, idx, max, item) kv_push(float64, out->value.fa, yyjson_get_num(item));
        return true;
    }
    if (all_str)
    {
        out->type = JSONV_TEXT_ARRAY;
        kv_init(out->value.sa);
        yyjson_arr_foreach(arr, idx, max, item) kv_push(char *, out->value.sa, strdup(yyjson_get_str(item)));
        return true;
    }
    return false;
}

static JsonValue JsonReadValueFromText(const char *file, const char *key, const char *text, uint32 size)
{
    JsonValue result;
    CLEAR_PTR(&result, 0);
    yyjson_read_err err;
    yyjson_doc *doc = yyjson_read_opts((char *)text, size, 0, NULL, &err);

    if (UNLIKELY(doc == NULL))
    {
        LOG_ERROR("Impossible to parse %s: %s (pos %zu)", file, err.msg, err.pos);
        return result;
    }
    yyjson_val *root = yyjson_doc_get_root(doc);

    yyjson_val *val = yyjson_obj_get(root, key);

    if (val != NULL)
    {

        if (yyjson_is_bool(val))
        {
            result.type = JSONV_BOOL;
            result.value.b = yyjson_get_bool(val);
        }
        else if (yyjson_is_int(val))
        {
            result.type = JSONV_INTEGER;
            result.value.i = yyjson_get_sint(val);
        }
        else if (yyjson_is_real(val))
        {
            result.type = JSONV_FLOAT;
            result.value.f = yyjson_get_real(val);
        }
        else if (yyjson_is_str(val))
        {
            result.type = JSONV_TEXT;
            result.value.s = strdup(yyjson_get_str(val));
        }
        else if (yyjson_is_arr(val))
        {
            if (!JsonFillArray(val, &result))
            {
                LOG_WARNING("Json array in %s is not homogeneous", file);
                CLEAR_PTR(&result, 0);
            }
        }
    }

    yyjson_doc_free(doc);
    return result;
}

JsonValue JsonReadValue(const char *file, const char *key)
{
    char *text = (char *)ReturnFileData(file, NULL);

    if (UNLIKELY(text == NULL))
    {
        LOG_ERROR("Impossible to get content from %s", file);
        JsonValue result;
        CLEAR_PTR(&result, 0);
        return result;
    }

    JsonValue result = JsonReadValueFromText(file, key, text, (uint32)strlen(text));
    free(text);
    return result;
}

JsonValue JsonReadValueFromDirectory(const char *path, const char *key)
{
    char *full_path = PathFromExecutableDirectory(path);
    JsonValue v = JsonReadValue(full_path, key);
    free(full_path);
    return v;
}

JsonValue JsonReadValueFromMemory(const char *file, const char *key, const unsigned char *text, uint32 size)
{
    if (UNLIKELY(text == NULL))
    {
        LOG_ERROR("Impossible to get content from %s", file);
        JsonValue result;
        CLEAR_PTR(&result, 0);
        return result;
    }
    return JsonReadValueFromText(file, key, (const char *)text, size);
}

void JsonValueFree(JsonValue *v)
{
    switch (v->type)
    {
    case JSONV_TEXT:
        free(v->value.s);
        break;
    case JSONV_INTEGER_ARRAY:
        kv_destroy(v->value.ia);
        break;
    case JSONV_FLOAT_ARRAY:
        kv_destroy(v->value.fa);
        break;
    case JSONV_BOOL_ARRAY:
        kv_destroy(v->value.ba);
        break;
    case JSONV_TEXT_ARRAY:
        for (uint32 i = 0; i < (uint32)kv_size(v->value.sa); ++i)
            free(kv_A(v->value.sa, i));
        kv_destroy(v->value.sa);
        break;
    default:
        break;
    }
    CLEAR_PTR(v, 0);
}