#pragma once
#include "../../../utils/c_cpp.h"

BEGIN_DECLARATIONS

typedef struct
{
    void *engine;
} AngelManager;

bool AngelInterpret(const char *filename);
bool AngelInterpretFromMemory(const char *filename);
bool AngelInit(const char *filename);
bool AngelInitFromMemory(const char *filename);
bool AngelReload(const char *filename);

bool AngelFree();

END_DECLARATIONS