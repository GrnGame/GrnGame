#include "file.h"

#include "grngame/dev/logging.h"
#include <stdlib.h>

bool PathInfo(const char *path, SDL_PathInfo *info)
{
    return SDL_GetPathInfo(path, info);
}

bool PathRemove(const char *path)
{
    bool ok = SDL_RemovePath(path);
    if (!ok)
        LOG_ERROR("Cant remove path: %s error: %s", path, SDL_GetError());
    return ok;
}

SDL_IOStream *IOOpen(const char *path, const char *mode)
{
    SDL_IOStream *io = SDL_IOFromFile(path, mode);
    if (io == NULL)
        LOG_ERROR("Cant open: %s mode: %s error: %s", path, mode, SDL_GetError());
    return io;
}

bool IOClose(SDL_IOStream *io)
{
    bool ok = SDL_CloseIO(io);
    if (!ok)
        LOG_ERROR("Cant close io error: %s", SDL_GetError());
    return ok;
}

bool IORead(SDL_IOStream *io, void *data, uint32 size)
{
    bool ok = (SDL_ReadIO(io, data, size) == size);
    if (!ok && SDL_GetIOStatus(io) != SDL_IO_STATUS_EOF)
        LOG_ERROR("Cant read io error: %s", SDL_GetError());
    return ok;
}

bool IOWrite(SDL_IOStream *io, const void *data, uint32 size)
{
    bool ok = (SDL_WriteIO(io, data, size) == size);
    if (!ok)
        LOG_ERROR("Cant write io error: %s", SDL_GetError());
    return ok;
}

bool IOSeek(SDL_IOStream *io, uint32 offset, SDL_IOWhence whence)
{
    bool ok = (SDL_SeekIO(io, offset, whence) >= 0);
    if (!ok)
        LOG_ERROR("Cant seek io error: %s", SDL_GetError());
    return ok;
}

bool IOSize(SDL_IOStream *io, uint32 *size_out)
{
    int32 size = (int32)SDL_GetIOSize(io);
    if (size < 0)
    {
        LOG_ERROR("Invalid io size error: %s", SDL_GetError());
        return false;
    }
    *size_out = (uint32)size;
    return true;
}

bool IOTell(SDL_IOStream *io, uint32 *pos_out)
{
    int32 pos = SDL_TellIO(io);
    if (pos < 0)
    {
        LOG_ERROR("Invalid io position error: %s", SDL_GetError());
        return false;
    }
    *pos_out = (uint32)pos;
    return true;
}

bool IORemaining(SDL_IOStream *io, uint32 *remaining_out)
{
    uint32 size = 0;
    uint32 pos = 0;
    if (!IOSize(io, &size) || !IOTell(io, &pos) || pos > size)
        return false;

    *remaining_out = size - pos;
    return true;
}

bool FileExists(const char *path)
{
    SDL_PathInfo info;
    return PathInfo(path, &info) && info.type == SDL_PATHTYPE_FILE;
}

unsigned char *FileLoad(const char *path, uint32 *size_out)
{
    if (size_out != NULL)
        *size_out = 0;

    SDL_IOStream *io = IOOpen(path, "rb");
    if (io == NULL)
        return NULL;

    uint32 size = 0;
    if (!IOSize(io, &size) || size == UINT32_MAX)
    {
        IOClose(io);
        return NULL;
    }

    unsigned char *data = malloc(size + 1);
    if (data == NULL)
    {
        LOG_ERROR("Out of memory loading: %s", path);
        IOClose(io);
        return NULL;
    }

    if (!IORead(io, data, size))
    {
        LOG_ERROR("Cant read whole file: %s", path);
        free(data);
        IOClose(io);
        return NULL;
    }

    data[size] = '\0';
    IOClose(io);

    if (size_out != NULL)
        *size_out = size;
    return data;
}

bool FileSave(const char *path, const void *data, uint32 size)
{
    bool ok = SDL_SaveFile(path, data, size);
    if (!ok)
        LOG_ERROR("Cant save file: %s error: %s", path, SDL_GetError());
    return ok;
}

bool FileAppend(const char *path, const void *data, uint32 size)
{
    SDL_IOStream *io = IOOpen(path, "ab");
    if (io == NULL)
        return false;

    bool ok = IOWrite(io, data, size);
    return IOClose(io) && ok;
}

bool FileCopy(const char *src, const char *dst)
{
    uint32 size = 0;
    unsigned char *data = FileLoad(src, &size);
    if (data == NULL)
        return false;

    bool ok = FileSave(dst, data, size);
    free(data);

    if (!ok)
        PathRemove(dst);
    return ok;
}

bool FileMove(const char *src, const char *dst)
{
    if (SDL_RenamePath(src, dst))
        return true;

    // rename can fail across volumes: fall back to copy + remove
    return FileCopy(src, dst) && PathRemove(src);
}