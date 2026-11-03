#include "embedded_file_generator.h"
#include "grngame/core/app.h"
#include "grngame/data/json_read.h"
#include "grngame/utils/memory.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    JsonValue v = JsonReadValue("config/config.json", "version");
    if (v.type != JSONV_TEXT)
    {
        LOG_ERROR("Failed to read version section in %s", "config/config.json");
        JsonValueFree(&v);
        return false;
    }
    const char *version = v.value.s;
    if (argc == 1)
    {
        const char *dirs[] = {"assets", "data", "std", "config"};
        CreateEmbeddedFileDb(4, dirs, "Assets.pak", version);
    }
    else if (argc < 3)
    {
        fprintf(stdout, "%s output-header dirs1 dirs2 ....\n", argv[0]);
        return 1;
    }
    else
    {
        CreateEmbeddedFileDb(argc - 2, (const char **)(argv + 2), argv[1], version);
    }
    JsonValueFree(&v);

    return 0;
}