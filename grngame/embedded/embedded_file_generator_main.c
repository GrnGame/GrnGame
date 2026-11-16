#include "embedded_file_generator.h"
#include "grngame/core/app.h"
#include "grngame/data/json_read.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    JsonValue v = JsonReadValue(CONFIG_FILE, "version");
    if (v.type != JSONV_TEXT)
    {
        LOG_ERROR("Failed to read version section in %s", CONFIG_FILE);
        JsonValueFree(&v);
        return false;
    }
    const char *version = v.value.s;
    if (argc == 1)
    {
        const char *dirs[] = {ASSET_FOLDER, DATA_FOLDER, STD, CONFIG_FOLDER};
        CreateEmbeddedFileDb(4, dirs, ASSETS_PAK, version);
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