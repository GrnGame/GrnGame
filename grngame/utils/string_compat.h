#pragma once

#include "grngame/math/types.h"
#include <stdlib.h>
#include <string.h>
#if defined(HAVE_STRCASECMP)
#include <strings.h>
#endif

#if !defined(HAVE_STRDUP)
#if defined(HAVE__STRDUP)
#define strdup _strdup
#else
static inline char *grn_strdup(const char *s)
{
    uint64 len = strlen(s) + 1;
    char *p = malloc(len);
    memcpy(p, s, len);
    return p;
}
#define strdup grn_strdup
#endif
#endif

#if !defined(HAVE_STRCASECMP)
#if defined(HAVE__STRICMP)
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#else
static inline int grn_strcasecmp(const char *a, const char *b)
{
    while (*a && *b)
    {
        unsigned char c1 = (unsigned char)*a++;
        unsigned char c2 = (unsigned char)*b++;

        if (c1 >= 'A' && c1 <= 'Z')
            c1 += 'a' - 'A';
        if (c2 >= 'A' && c2 <= 'Z')
            c2 += 'a' - 'A';

        if (c1 != c2)
            return c1 - c2;
    }

    return (unsigned char)*a - (unsigned char)*b;
}
#define strcasecmp grn_strcasecmp
#endif
#endif