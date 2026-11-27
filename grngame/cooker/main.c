#include "parser.h"
#include "grngame/core/app.h"
#include "grngame/data/json_read.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static void FreeAppInfo(AppInfo *info)
{
    if (!info)
        return;

    free((void *)info->version);
    for (size_t i = 0; i < kv_size(info->palette); ++i)
        free(kv_A(info->palette, i));
    kv_destroy(info->palette);
    free(info);
}

int main(int argc, char **argv)
{

    LogInit(LOG_TO_CONSOLE);
    if (argc > 1 && argc < 3)
    {
        fprintf(stderr, "%s output-pak dirs1 dirs2 ....\n", argv[0]);
        return 1;
    }

    AppInfo *app_info = ParseConfig(NULL, 0);
    if (!app_info)
    {
        fprintf(stderr, "Cooker: ParseConfig failed\n");
        return 1;
    }

    bool ok = false;
    if (argc == 1)
    {
        const char *dirs[] = {ASSET_FOLDER, DATA_FOLDER, STD, CONFIG_FOLDER};
        ok = Cooker(app_info, 4, dirs, ASSETS_PAK);
    }
    else
    {
        ok = Cooker(app_info, (int32)(argc - 2), (const char **)(argv + 2), argv[1]);
    }

    if (!ok)
        fprintf(stderr, "Cooker: failed to build archive\n");

    FreeAppInfo(app_info);
    return ok ? 0 : 1;
}