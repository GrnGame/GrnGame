#include "grngame/math/types.h"
#include <stdbool.h>
#include <stdio.h>

bool ParseVersionToInt(const char *version, int32 *out)
{
    int32 major = 0, minor = 0, patch = 0;
    if (sscanf(version, "%d.%d.%d", &major, &minor, &patch) < 1)
        return false;
    if (major < 0 || major > 2146 || minor < 0 || minor > 999 || patch < 0 || patch > 999)
        return 0;
    *out = (int32)(major * 1000000 + minor * 1000 + patch);
    return true;
}

bool ParseVersionToString(int32 version_num, char *out, uint32 out_size)
{
    if (out == NULL || out_size == 0 || version_num < 0)
        return false;

    int32 major = version_num / 1000000;
    int32 minor = (version_num / 1000) % 1000;
    int32 patch = version_num % 1000;

    int32 n = snprintf(out, out_size, "%d.%d.%d", (int32)major, (int32)minor, (int32)patch);

    return (n > 0 && (uint16)n < out_size);
}