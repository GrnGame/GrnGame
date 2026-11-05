#include "module.h"
#include "../../../utils/c_cpp.h"
#include "grngame/assets/embedded_file_manager.h"
#include "grngame/core/app.h"
#include "grngame/data/file.h"
#include "grngame/platform/paths.h"
#include "grngame/utils/string_compat.h"
#include "scriptbuilder/scriptbuilder.h"
#include <angelscript.h>
#include <cstdlib>

#define MODULE_SIZE_MAX_NAME 512

int32 LoadModuleFnFromMemory(const char *include, const char *from, CScriptBuilder *builder, void *userParam)
{
    char filename[MODULE_SIZE_MAX_NAME];

    // we can put .as in include
    if (strstr(include, ".as") != nullptr)
    {
        snprintf(filename, sizeof(filename), "%s", include);
    }
    else
    {
        snprintf(filename, sizeof(filename), "%s.as", include);
    }

    const EmbeddedFile *asset = EmbeddedFileGet(filename);
    if (!asset)
    {
        LOG_ERROR("AngelScript Include Error: Failed to find '%s' in embedded files (included from '%s')", filename,
                  from);
        return -1;
    }

    int32 result = builder->AddSectionFromMemory(filename, static_cast<const char *>(asset->data), asset->size);
    if (result < 0)
    {
        LOG_ERROR("AngelScript AddSectionFromMemory failed for embedded include '%s'", filename);
        return -1;
    }

    return 0;
}

int32 LoadModuleFn(const char *include, const char *from, CScriptBuilder *builder, void *userParam)
{
    char filename[MODULE_SIZE_MAX_NAME];

    if (strstr(include, ".as") != nullptr)
    {
        snprintf(filename, sizeof(filename), "%s", include);
    }
    else
    {
        snprintf(filename, sizeof(filename), "%s.as", include);
    }

    char *path = PathFromExecutableDirectory(filename);

    uint32 file_size = 0;
    const auto file_content = reinterpret_cast<char *>(ReturnFileData(path, &file_size));
    free(path);

    if (!file_content)
    {
        LOG_ERROR("AngelScript Include Error: Failed to find '%s' on disk (included from '%s')", filename, from);
        return -1;
    }

    int32 result = builder->AddSectionFromMemory(filename, file_content, file_size);
    free(file_content);

    if (result < 0)
    {
        LOG_ERROR("AngelScript AddSectionFromMemory failed for include '%s'", filename);
        return -1;
    }

    return 0;
}