#pragma once
#include "../../../utils/c_cpp.h"
#include "grngame/math/types.h"

#ifdef __cplusplus
class CScriptBuilder;
typedef CScriptBuilder *BuilderPtr;
#else
typedef void *BuilderPtr;
#endif

BEGIN_DECLARATIONS

int32 LoadModuleFn(const char *include, const char *from, BuilderPtr builder, void *userParam);
int32 LoadModuleFnFromMemory(const char *include, const char *from, BuilderPtr builder, void *userParam);

END_DECLARATIONS