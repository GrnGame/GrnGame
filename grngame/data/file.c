#include "SDL3/SDL.h"
#include "SDL3/SDL_iostream.h"
#include "grngame/dev/logging.h"
#include "grngame/math/types.h"
#include <grngame/utils/string_compat.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool PathInfo(const char *path, SDL_PathInfo *info)
{
    return SDL_GetPathInfo(path, info);
}

// todo return a bool
void *LoadFile(const char *name, uint32 *size)
{
    size_t loaded_size = 0;
    void *data = SDL_LoadFile(name, &loaded_size);
    if (size != NULL)
        *size = (uint32)loaded_size;
    if (!data)
        LOG_ERROR("Cant load file :%s error:", name, SDL_GetError());
    return data;
}

bool SaveFile(const char *name, const void *data, uint32 size)
{
    bool ok = SDL_SaveFile(name, data, size);
    if (!ok)
        LOG_ERROR("Cant save file: %s error:", name, SDL_GetError());
    return ok;
}

// todo : return a bool
SDL_IOStream *IOFromFile(const char *file, const char *mode)
{
    SDL_IOStream *io = SDL_IOFromFile(file, mode);
    if (!io)
        LOG_ERROR("Cant get io from file :%s in mode %s error :", file, mode, SDL_GetError());
    return io;
}

bool WriteIO(SDL_IOStream *io, const void *data, uint32 size)
{
    bool ok = (SDL_WriteIO(io, data, size) == size);
    if (!ok)
        LOG_ERROR("Cant write io error: %s", SDL_GetError());
    return ok;
}

bool ReadIO(SDL_IOStream *io, void *data, uint32 size)
{
    bool ok = (SDL_ReadIO(io, data, size) == size);
    if (!ok && SDL_GetIOStatus(io) != SDL_IO_STATUS_EOF)
        LOG_ERROR("Cant read io error: %s", SDL_GetError());
    return ok;
}

bool CloseIO(SDL_IOStream *io)
{
    bool ok = SDL_CloseIO(io);
    if (!ok)
        LOG_ERROR("Cant close io error:", SDL_GetError());
    return ok;
}

bool RemovePath(const char *path)
{
    if (!SDL_RemovePath(path))
    {
        LOG_ERROR("Cant remove path of: %s error:%s", path, SDL_GetError());
        return false;
    }
    return true;
}

bool FileExist(const char *name)
{
    SDL_PathInfo info;
    return PathInfo(name, &info) && info.type == SDL_PATHTYPE_FILE;
}

// dont use sdl api because we need to free data with sdl_free
unsigned char *ReturnFileData(const char *name, uint32 *size_out)
{
    FILE *file = fopen(name, "rb");
    if (file == NULL)
    {
        LOG_ERROR("File %s doesn't exist", name);
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        LOG_ERROR("Unable to seek in file %s", name);
        fclose(file);
        return NULL;
    }

    long size = ftell(file);
    if (size < 0)
    {
        LOG_ERROR("Invalid file size for %s", name);
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        LOG_ERROR("Unable to rewind file %s", name);
        fclose(file);
        return NULL;
    }

    uint32 file_size = (uint32)size;
    unsigned char *buffer = malloc(file_size + 1);

    uint32 read_count = (uint32)fread(buffer, 1, file_size, file);
    fclose(file);

    buffer[read_count] = '\0';

    if (size_out != NULL)
        *size_out = (uint32)read_count;

    return buffer;
}

bool WriteFileData(const char *name, const void *data, uint32 size, bool append)
{
    if (!append)
        return SaveFile(name, data, size);

    SDL_IOStream *io = IOFromFile(name, "ab");
    if (!io)
    {
        return false;
    }

    bool ok = WriteIO(io, data, size);
    return CloseIO(io) && ok;
}

bool MoveFile_(const char *dst, const char *src)
{
    uint32 size = 0;
    void *data = LoadFile(src, &size);
    if (!data)
    {
        LOG_ERROR("impossible to get data from %s", src);
        return false;
    }

    bool ok = SaveFile(dst, data, size);
    SDL_free(data);

    if (!ok)
    {
        LOG_ERROR("Failed to save file :%s", dst);
        if (!RemovePath(dst))
        {
            LOG_ERROR("Failed to remove path of %s", dst);
        }
    }

    return ok;
}