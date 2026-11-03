#pragma once
#include "grngame/utils/c_cpp.h"
#include <stdbool.h>
#include <stddef.h>

BEGIN_DECLARATIONS

// Returns a malloc'd string that is the filename stem of the path
// If you give it assets/boss.wav, it returns boss
// Remember to free
char *FileStem(const char *path);

const char *FileExtension(const char *path);

/// Returns true if the file is an image that COULD be loaded with SDL_Image
bool FileIsLoadableImage(const char *file);

/// Returns true if the file is an image that COULd be loaded with Soloud's Wav_stream
bool FileIsLoadableAudio(const char *file);

/// Returns true if the file can be loaded as a script
bool FileIsLoadableScript(const char *file);

/// Returns true if the file can be loaded as a text
bool FileIsLoadableText(const char *file);

/// Returns true if the file can be loaded as a json
bool FileIsLoadableJson(const char *file);

/// Returns true if the file can loaded as a tff
bool FileIsLoadableTtf(const char *file);

/// Returns the path starting from the path of the executable where we added relative. You must free it.
/// For example , if your exe is in ~/chouffe and relative is assets/folder, it returns ~/chouffe/assets/folder
char *PathFromExecutableDirectory(const char *relative);

/// need to free
char *PathFromStorage(const char *file);

/// need to free
char *PathFromAndroidStorage(const char *file);

END_DECLARATIONS