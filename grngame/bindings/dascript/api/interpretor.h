#pragma once

#include "../../../utils/c_cpp.h"

BEGIN_DECLARATIONS

bool DaScriptRun(const char *filename);
bool DaScriptRunFromMemory(const char *filename);

bool DaScriptReload(const char *filename);

void DaScriptFree();

END_DECLARATIONS