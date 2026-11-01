#pragma once

#include "grngame/math/types.h"
#include "grngame/utils/c_cpp.h"
#include "grngame/utils/macro.h"
#include "kvec.h"
#include "stdbool.h"

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

// read only json ,can be use on web ios and android (arrays need to be homogenous)
JsonValue JsonReadValue(const char *file, const char *path);
JsonValue JsonReadValueFromDirectory(const char *path, const char *key);

// for config
JsonValue JsonReadValueFromMemory(const char *file, const char *path, const unsigned char *text, uint32 size);

void JsonValueFree(JsonValue *v);

END_DECLARATIONS