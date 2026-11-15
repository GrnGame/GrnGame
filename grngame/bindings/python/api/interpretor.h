#pragma once

#include "../../../utils/c_cpp.h"
#include <stdbool.h>

BEGIN_DECLARATIONS

bool PyScriptRun(const char *filename);
bool PyScriptRunFromMemory(const char *filename);

bool PyScriptReload(const char *filename);

void PyScriptFree();

END_DECLARATIONS
