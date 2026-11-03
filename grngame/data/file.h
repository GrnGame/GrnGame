#pragma once
#include "SDL3/SDL.h"
#include "SDL3/SDL_iostream.h"
#include "grngame/math/types.h"
#include "grngame/utils/c_cpp.h"
#include "grngame/utils/memory.h"
#include <stdbool.h>

#define MAX_PATH_ 1024 * BUFFER_SPACE

BEGIN_DECLARATIONS

bool PathInfo(const char *path, SDL_PathInfo *info);
void *LoadFile(const char *name, uint32 *size);
bool SaveFile(const char *name, const void *data, uint32 size);
SDL_IOStream *IOFromFile(const char *file, const char *mode);
bool WriteIO(SDL_IOStream *io, const void *data, uint32 size);
bool ReadIO(SDL_IOStream *io, void *data, uint32 size);
bool CloseIO(SDL_IOStream *io);
bool RemovePath(const char *path);

bool FileExist(const char *name);
// dont work on android
unsigned char *ReturnFileData(const char *name, uint32 *size_out);
bool WriteFileData(const char *name, const void *data, uint32 size, bool append);
bool MoveFile_(const char *dst, const char *name);
END_DECLARATIONS
