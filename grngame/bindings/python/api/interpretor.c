#include "interpretor.h"
#include "callback.h"
#include "grngame/assets/embedded_file_manager.h"
#include "grngame/platform/paths.h"
bool PyScriptRun(const char *filename)
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

bool PyScriptRunFromMemory(const char *filename)
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

void PyScriptFree()
{
    PyScriptClearCallbacks();
}

bool PyScriptReload(const char *filename)
{
    PyScriptFree();
    return PyScriptRun(filename);
}
