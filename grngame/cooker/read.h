#pragma once
#include "grngame/math/types.h"
#include "write.h"

#include "grngame/utils/c_cpp.h"

BEGIN_DECLARATIONS

ArchiveHeader ReadArchiveHeader(const char *path);
ArchiveNode *ReadArchiveNodes(const char *path, uint32 *size_out);
ArchiveAssetData *ReadArchiveAssetsData(const char *path, uint32 *size_out);
void FreeArchiveNodes(ArchiveNode *nodes, uint32 count);
void FreeArchiveAssetsData(ArchiveAssetData *assets_data, uint32 count);

END_DECLARATIONS