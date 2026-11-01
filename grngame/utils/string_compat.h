#pragma once

#include "grngame/math/types.h"
#include "stdio.h"
#include "stdlib.h"
#include <string.h>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

#ifdef _MSC_VER
#define strdup _strdup
#endif

#ifdef GRNGAME_WASM

static inline char *strdup(const char *s)
{
    uint64 len = strlen(s) + 1;
    char *p = malloc(len);
    memcpy(p, s, len);
    return p;
}

static inline int32 strcasecmp(const char *s1, const char *s2)
{
    while (*s1 && *s2)
    {
        unsigned char c1 = (unsigned char)*s1++;
        unsigned char c2 = (unsigned char)*s2++;

        if (c1 >= 'A' && c1 <= 'Z')
            c1 += 'a' - 'A';
        if (c2 >= 'A' && c2 <= 'Z')
            c2 += 'a' - 'A';

        if (c1 != c2)
            return c1 - c2;
    }

    return (unsigned char)*s1 - (unsigned char)*s2;
}

#endif

#include <SDL3/SDL_stdinc.h>

#ifndef strtok_r
#define strtok_r SDL_strtok_r
#endif
