#pragma once
#include "../../../utils/c_cpp.h"
#include "angelscript.h"

BEGIN_DECLARATIONS

typedef struct
{
    void *engine;
} AngelManager;

bool Interpret(const char *filename);
bool InterpretFromMemory(const char *filename);
bool AngelInit(const char *filename);
bool AngelInitFromMemory(const char *filename);

bool AngelFree();

END_DECLARATIONS