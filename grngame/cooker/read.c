#include "read.h"

#include "grngame/data/file.h"
#include "grngame/dev/logging.h"
#include "grngame/utils/attributes.h"
#include "grngame/utils/clear.h"
#include <stdlib.h>
#include <string.h>

#define NODE_MIN_SIZE ((uint32)(sizeof(ArchiveKind) + sizeof(uint32) + PATH_SIZE))
#define ASSET_DATA_SIZE ((uint32)(sizeof(vec2s) + sizeof(vec2s) + PATH_SIZE))

static bool ReadHeaderIO(SDL_IOStream *io, ArchiveHeader *h)
{
    return IORead(io, &h->magic, sizeof(uint32)) && IORead(io, &h->version, sizeof(uint32)) &&
           IORead(io, &h->assets_files, sizeof(uint32)) && IORead(io, &h->data_files, sizeof(uint32)) &&
           IORead(io, &h->script_files, sizeof(uint32));
}

ArchiveHeader ReadArchiveHeader(const char *path)
{
    ArchiveHeader header;
    CLEAR(header, 0);

    SDL_IOStream *io = IOOpen(path, "rb");
    if (UNLIKELY(io == NULL))
        return header;

    if (!ReadHeaderIO(io, &header))
    {
        LOG_ERROR("Invalid archive header: %s", path);
        CLEAR(header, 0);
    }

    IOClose(io);
    return header;
}

void FreeArchiveNodes(ArchiveNode *nodes, uint32 count)
{
    if (nodes == NULL)
        return;

    for (uint32 i = 0; i < count; i++)
    {
        free(nodes[i].content);
        free((void *)nodes[i].name);
    }
    free(nodes);
}

void FreeArchiveAssetsData(ArchiveAssetData *assets_data, uint32 count)
{
    if (assets_data == NULL)
        return;

    for (uint32 i = 0; i < count; i++)
        free((void *)assets_data[i].name);

    free(assets_data);
}

static bool ReadArchiveNode(SDL_IOStream *io, ArchiveNode *node)
{
    char name[PATH_SIZE];
    CLEAR(name, 0);

    if (!IORead(io, &node->kind, sizeof(ArchiveKind)) || !IORead(io, &node->content_size, sizeof(uint32)) ||
        !IORead(io, name, PATH_SIZE))
    {
        return false;
    }
    name[PATH_SIZE - 1] = '\0';

    uint32 remaining = 0;
    if (!IORemaining(io, &remaining) || node->content_size > remaining)
    {
        LOG_ERROR("Corrupted archive node: %s", name);
        return false;
    }

    uint32 name_len = (uint32)strlen(name);
    char *name_copy = malloc(name_len + 1);
    if (name_copy == NULL)
        return false;
    memcpy(name_copy, name, name_len + 1);
    node->name = name_copy;

    // +1 for a null terminator (scripts / text)
    node->content = malloc(node->content_size + 1);
    if (node->content == NULL)
        return false;

    if (!IORead(io, node->content, node->content_size))
        return false;

    node->content[node->content_size] = '\0';
    return true;
}

static bool ReadArchiveAssetData(SDL_IOStream *io, ArchiveAssetData *asset)
{
    char name[PATH_SIZE];
    CLEAR(name, 0);

    if (!IORead(io, &asset->size, sizeof(vec2s)) || !IORead(io, &asset->positions, sizeof(vec2s)) ||
        !IORead(io, name, PATH_SIZE))
    {
        return false;
    }
    name[PATH_SIZE - 1] = '\0';

    uint32 name_len = (uint32)strlen(name);
    char *name_copy = malloc(name_len + 1);
    if (name_copy == NULL)
        return false;
    memcpy(name_copy, name, name_len + 1);
    asset->name = name_copy;

    return true;
}

static bool SkipArchiveNode(SDL_IOStream *io)
{
    ArchiveKind kind;
    uint32 content_size = 0;
    char name[PATH_SIZE];

    if (!IORead(io, &kind, sizeof(ArchiveKind)) || !IORead(io, &content_size, sizeof(uint32)) ||
        !IORead(io, name, PATH_SIZE))
    {
        return false;
    }

    uint32 remaining = 0;
    if (!IORemaining(io, &remaining) || content_size > remaining)
        return false;

    return IOSeek(io, content_size, SDL_IO_SEEK_CUR);
}

ArchiveNode *ReadArchiveNodes(const char *path, uint32 *size_out)
{
    if (size_out != NULL)
        *size_out = 0;

    SDL_IOStream *io = IOOpen(path, "rb");
    if (UNLIKELY(io == NULL))
        return NULL;

    ArchiveHeader header;
    CLEAR(header, 0);

    if (!ReadHeaderIO(io, &header))
    {
        LOG_ERROR("Invalid archive header: %s", path);
        IOClose(io);
        return NULL;
    }

    uint32 count = header.assets_files + header.data_files + header.script_files;
    uint32 remaining = 0;
    if (!IORemaining(io, &remaining) || count > remaining / NODE_MIN_SIZE)
    {
        LOG_ERROR("Archive header does not match file size: %s", path);
        IOClose(io);
        return NULL;
    }

    uint32 alloc_count = count > 0 ? count : 1;
    ArchiveNode *nodes = malloc((uint32)sizeof(ArchiveNode) * alloc_count);
    if (nodes == NULL)
    {
        IOClose(io);
        return NULL;
    }
    CLEAR_ARRAY(nodes, 0, alloc_count);

    for (uint32 i = 0; i < count; i++)
    {
        if (!ReadArchiveNode(io, &nodes[i]))
        {
            LOG_ERROR("Failed to read node %u in %s", i, path);
            FreeArchiveNodes(nodes, count);
            IOClose(io);
            return NULL;
        }
    }

    IOClose(io);
    if (size_out != NULL)
        *size_out = count;
    return nodes;
}

ArchiveAssetData *ReadArchiveAssetsData(const char *path, uint32 *size_out)
{
    if (size_out != NULL)
        *size_out = 0;

    SDL_IOStream *io = IOOpen(path, "rb");
    if (UNLIKELY(io == NULL))
        return NULL;

    ArchiveHeader header;
    CLEAR(header, 0);

    if (!ReadHeaderIO(io, &header))
    {
        LOG_ERROR("Invalid archive header: %s", path);
        IOClose(io);
        return NULL;
    }

    uint32 nodes_count = header.assets_files + header.data_files + header.script_files;
    for (uint32 i = 0; i < nodes_count; i++)
    {
        if (!SkipArchiveNode(io))
        {
            LOG_ERROR("Failed to skip node %u in %s", i, path);
            IOClose(io);
            return NULL;
        }
    }

    uint32 remaining = 0;
    if (!IORemaining(io, &remaining) || remaining % ASSET_DATA_SIZE != 0)
    {
        LOG_ERROR("Corrupted archive asset data section in %s", path);
        IOClose(io);
        return NULL;
    }

    uint32 count = remaining / ASSET_DATA_SIZE;
    uint32 alloc_count = count > 0 ? count : 1;
    ArchiveAssetData *assets_data = malloc((uint32)sizeof(ArchiveAssetData) * alloc_count);
    if (assets_data == NULL)
    {
        IOClose(io);
        return NULL;
    }
    CLEAR_ARRAY(assets_data, 0, alloc_count);

    for (uint32 i = 0; i < count; i++)
    {
        if (!ReadArchiveAssetData(io, &assets_data[i]))
        {
            LOG_ERROR("Failed to read asset data %u in %s", i, path);
            FreeArchiveAssetsData(assets_data, count);
            IOClose(io);
            return NULL;
        }
    }

    IOClose(io);
    if (size_out != NULL)
        *size_out = count;
    return assets_data;
}