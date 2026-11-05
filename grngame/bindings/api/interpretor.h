#pragma once

#include "../../utils/c_cpp.h"

BEGIN_DECLARATIONS

bool BindingsInterpret(const char *filename);
bool BindingsInterpretFromMemory(const char *filename);
bool BindingsInit(const char *filename);
bool BindingsInitFromMemory(const char *filename);
bool BindingsReload(const char *filename);

bool BindingsFree();

END_DECLARATIONS