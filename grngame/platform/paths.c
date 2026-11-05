#include "paths.h"
#include "grngame/core/app.h"
#include "grngame/dev/logging.h"
#include "grngame/math/types.h"
#include "grngame/platform/directories.h"
#include "grngame/utils/string_compat.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_system.h>
#include <stdio.h>
#include <stdlib.h>

char *FileStem(const char *path)
{
    const char *start = strrchr(path, '/');
    if (!start) // if we didnt find / that means we are on windows
        start = strrchr(path, '\\');
    start = start ? start + 1 : path;

    // last dot after the slash
    const char *dot = strrchr(start, '.');
    uint32 len = dot ? (uint32)(dot - start) : (uint32)strlen(start);

    char *stem = malloc(len + 1);
    memcpy(stem, start, len);
    stem[len] = '\0';
    return stem;
}

const char *FileExtension(const char *path)
{
    if (!path)
        return ""; // safeguard but could be
    const char *dot = strrchr(path, '.');
    return dot ? dot + 1 : "";
}

bool FileIsLoadableImage(const char *file)
{
    const char *ext = FileExtension(file);
    if (!*ext) // if the string is empty that means there is no extension and it cant be determined
        return false;

    // null terminated so we can loop on it in the following loop
    static const char *exts[] = {"bmp", "gif", "jpg", "jpeg", "lbm",  "pcx",  "png", "pnm", "pbm", "pgm", "ppm",
                                 "qoi", "svg", "tga", "tif",  "tiff", "webp", "xcf", "xpm", "xv",  NULL};

    // exts[i] will be false when it will reach NULL
    for (int32 i = 0; exts[i]; ++i)
        if (strcasecmp(ext, exts[i]) == 0)
            return true;

    return false;
}

bool FileIsLoadableScript(const char *file)
{
    const char *ext = FileExtension(file);
    if (!*ext) // if the string is empty that means there is no extension and it cant be determined
        return false;

    // null terminated so we can loop on it in the following loop
    static const char *exts[] = {"as", NULL};

    // exts[i] will be false when it will reach NULL
    for (int32 i = 0; exts[i]; ++i)
        if (strcasecmp(ext, exts[i]) == 0)
            return true;

    return false;
}

bool FileIsLoadableAudio(const char *file)
{
    const char *ext = FileExtension(file);
    if (!*ext) // same
        return false;

    // same
    static const char *exts[] = {"wav", "mp3", "ogg", NULL};

    for (int32 i = 0; exts[i]; ++i)
        if (strcasecmp(ext, exts[i]) == 0)
            return true;

    return false;
}

bool FileIsLoadableText(const char *file)
{
    const char *ext = FileExtension(file);
    if (!*ext) // same
        return false;

    // same
    static const char *exts[] = {"txt", NULL};

    for (int32 i = 0; exts[i]; ++i)
        if (strcasecmp(ext, exts[i]) == 0)
            return true;

    return false;
}

bool FileIsLoadableJson(const char *file)
{
    const char *ext = FileExtension(file);
    if (!*ext) // same
        return false;

    // same
    static const char *exts[] = {"json", NULL};

    for (int32 i = 0; exts[i]; ++i)
        if (strcasecmp(ext, exts[i]) == 0)
            return true;

    return false;
}

bool FileIsLoadableTtf(const char *file)
{
    const char *ext = FileExtension(file);
    if (!*ext) // same
        return false;

    // same
    static const char *exts[] = {"ttf", NULL};

    for (int32 i = 0; exts[i]; ++i)
        if (strcasecmp(ext, exts[i]) == 0)
            return true;

    return false;
}

char *PathFromExecutableDirectory(const char *relative)
{
#if defined(GRNGAME_ANDROID) || defined(GRNGAME_IOS)
    return strdup(relative);
#else
    const char *exe_dir = DirOfExecutable();

    // skip leading slashes on relative
    while (*relative == '/')
        relative++;

    uint32 exe_len = (uint32)strlen(exe_dir);

    // trim trailing slashes on exe_dir
    while (exe_len > 0 && exe_dir[exe_len - 1] == '/')
        exe_len--;

    uint32 len = exe_len + (uint32)strlen(relative) + 2; // +2 bytes for '/' and for '\0'
    char *buf = malloc(len);
    snprintf(buf, len, "%.*s/%s", (int32)exe_len, exe_dir, relative);

    return buf;
#endif
}

char *PathFromAndroidStorage(const char *file)
{
#ifdef GRNGAME_ANDROID
    const char *path = SDL_GetAndroidInternalStoragePath();
    uint32 n = (uint32)strlen(path) + (uint32)strlen(file) + 2;
    char *out = malloc(n);
    snprintf(out, n, "%s/%s", path, file);
    return out;
#else
    return strdup(file);
#endif
}

// need to free
char *PathFromStorage(const char *file)
{
#ifndef GRNGAME_WASM
    char *base = SDL_GetPrefPath(g_app.info.company, g_app.info.name);
    if (!base)
    {
        LOG_ERROR("Failed to get storage of the app");
        return NULL;
    }
    uint32 n = (uint32)strlen(base) + (uint32)strlen(file) + 1;
    char *out = (char *)malloc(n);
    snprintf(out, n, "%s%s", base, file);
    SDL_free(base);
    return out;
#else
    return strdup(file);
#endif
}