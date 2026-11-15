#include "interpretor.h"
#include <daScript/daScript.h>

#include "grngame/assets/embedded_file_manager.h"
#include "grngame/bindings/dascript/api/callback.h"
#include "grngame/platform/paths.h"
#include <cstdlib>

bool DaScriptRun(const char *filename)
{
    char *name = PathFromExecutableDirectory(filename);
    char *module = FileStem(filename);

    // create interpretor

    // register bindings

    // register main callback

    // run script from disk

free:
    free(name);
    free(module);
}

bool DaScriptRunFromMemory(const char *filename)
{

    const EmbeddedFile *file = EmbeddedFileGet(filename);
    const char *name = file->name;
    uint32 size = file->size;
    char *module = FileStem(filename);

    // create intepretor

    // register bindings

    // register main callback

    // run script from memory

free:
    free(module);
}

void DaScriptFree()
{
    // free the static callback
}

bool DaScriptReload(const char *filename)
{
    DaScriptFree();
    return DaScriptRun(filename);
}