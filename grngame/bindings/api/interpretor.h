#pragma once

#include "../../utils/c_cpp.h"
#include <stdbool.h>

BEGIN_DECLARATIONS

typedef enum
{
    PY_SCRIPT,
    DAS_SCRIPT,
} SCRIPTING_LANGUAGE;

bool ScriptRun(const char *filename);
bool ScriptRunFromMemory(const char *filename);

bool ScriptReload(const char *filename);

void ScriptFree();

END_DECLARATIONS