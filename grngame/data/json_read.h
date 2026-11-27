#pragma once

#include "grngame/math/types.h"
#include "grngame/utils/c_cpp.h"
#include "grngame/utils/macro.h"
#include "kvec.h"
#include "stdbool.h"
#include "grngame/core/app.h"

#include <yyjson.h>

BEGIN_DECLARATIONS

typedef enum
{
    JSONV_VOID = 0, // not readable type
    JSONV_INTEGER,
    JSONV_FLOAT,
    JSONV_BOOL,
    JSONV_TEXT,
    JSONV_INTEGER_ARRAY,
    JSONV_FLOAT_ARRAY,
    JSONV_BOOL_ARRAY,
    JSONV_TEXT_ARRAY,
    JSONV_EMPTY_ARRAY
} JsonValueType;

typedef struct
{
    JsonValueType type;
    union {
        int32 i;
        float64 f;
        bool b;
        char *s;
        int32_vec_t ia;
        float64_vec_t fa;
        bool_vec_t ba;
        string_vec_t sa;
    } value;
} JsonValue;

typedef struct
{
    yyjson_doc *doc;
    char *file;
} JsonDoc;

// read only json ,can be use on web ios and android (arrays need to be homogenous)
JsonDoc JsonOpen(const char *file);
JsonDoc JsonOpenFromDirectory(const char *path);

// for config
JsonDoc JsonOpenFromMemory(const char *file, const unsigned char *text, uint32 size);

JsonValue JsonReadValue(const JsonDoc *json, const char *key);
void JsonClose(JsonDoc *json);

void JsonValueFree(JsonValue *v);

// used in grngame
AppInfo *ParseConfig(const unsigned char *memory, uint32 size);

END_DECLARATIONS