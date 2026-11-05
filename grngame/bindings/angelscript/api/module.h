#pragma once
#include "../../../utils/c_cpp.h"
#include <angelscript.h>

BEGIN_DECLARATIONS

bool LoadModuleFn(const char *moduleName);
bool LoadModuleFnFromMemory(const char *moduleName);

END_DECLARATIONS