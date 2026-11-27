#pragma once
#include "SDL3/SDL.h"
#include "SDL3/SDL_iostream.h"
#include "grngame/math/types.h"
#include "grngame/utils/c_cpp.h"
#include <stdbool.h>

BEGIN_DECLARATIONS

bool PathInfo(const char *path, SDL_PathInfo *info);
bool PathRemove(const char *path);

bool FileExists(const char *path);

// need to free
unsigned char *FileLoad(const char *path, uint32 *size_out);

bool FileSave(const char *path, const void *data, uint32 size);
bool FileAppend(const char *path, const void *data, uint32 size);
bool FileCopy(const char *src, const char *dst);
bool FileMove(const char *src, const char *dst);

// sdl wrapper
SDL_IOStream *IOOpen(const char *path, const char *mode);
bool IOClose(SDL_IOStream *io);
bool IORead(SDL_IOStream *io, void *data, uint32 size);
bool IOWrite(SDL_IOStream *io, const void *data, uint32 size);
bool IOSeek(SDL_IOStream *io, uint32 offset, SDL_IOWhence whence);
bool IOSize(SDL_IOStream *io, uint32 *size_out);
bool IOTell(SDL_IOStream *io, uint32 *pos_out);
bool IORemaining(SDL_IOStream *io, uint32 *remaining_out);

END_DECLARATIONS