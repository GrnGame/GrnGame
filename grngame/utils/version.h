#pragma once
#include "grngame/math/types.h"
#include <stdbool.h>

bool ParseVersionToInt(const char *version, int32 *out);
bool ParseVersionToString(int32 version_num, char *out, uint32 out_size);