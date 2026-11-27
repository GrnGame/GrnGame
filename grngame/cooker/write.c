#include "write.h"

#include "grngame/data/file.h"
#include "grngame/utils/attributes.h"
#include "grngame/utils/clear.h"
#include <string.h>

static bool WriteArchiveHeader(SDL_IOStream *io, const ArchiveHeader *h)
{
    return IOWrite(io, &h->magic, sizeof(uint32)) && IOWrite(io, &h->version, sizeof(uint32)) &&
           IOWrite(io, &h->assets_files, sizeof(uint32)) && IOWrite(io, &h->data_files, sizeof(uint32)) &&
           IOWrite(io, &h->script_files, sizeof(uint32));
}

static bool WriteArchiveNode(SDL_IOStream *io, const ArchiveNode *node)
{
    char name[PATH_SIZE];
    CLEAR(name, 0);
    strncpy(name, node->name, PATH_SIZE - 1);

    return IOWrite(io, &node->kind, sizeof(ArchiveKind)) && IOWrite(io, &node->content_size, sizeof(uint32)) &&
           IOWrite(io, name, PATH_SIZE) && IOWrite(io, node->content, node->content_size);
}

static bool WriteArchiveAssetData(SDL_IOStream *io, const ArchiveAssetData *asset)
{
    char name[PATH_SIZE];
    CLEAR(name, 0);
    strncpy(name, asset->name, PATH_SIZE - 1);

    return IOWrite(io, &asset->size, sizeof(vec2s)) && IOWrite(io, &asset->positions, sizeof(vec2s)) &&
           IOWrite(io, name, PATH_SIZE);
}

bool WriteArchive(const char *destination, const ArchiveHeader *header, ArchiveNode **nodes, uint32 size_nodes,
                  ArchiveAssetData **assets_data, uint32 size_assets)
{
    SDL_IOStream *io = IOOpen(destination, "wb");
    if (UNLIKELY(io == NULL))
        return false;

    bool ok = WriteArchiveHeader(io, header);

    for (uint32 i = 0; ok && i < size_nodes; i++)
        ok = WriteArchiveNode(io, nodes[i]);

    for (uint32 i = 0; ok && i < size_assets; i++)
        ok = WriteArchiveAssetData(io, assets_data[i]);

    ok = IOClose(io) && ok;

    // remove it if archive writer fail
    if (!ok)
        PathRemove(destination);

    return ok;
}