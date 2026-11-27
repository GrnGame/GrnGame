#include "parser.h"
#include "write.h"
#include "grngame/core/app.h"
#include "grngame/platform/directories.h"
#include "grngame/platform/paths.h"
#include "grngame/utils/clear.h"
#include "grngame/utils/version.h"
#include "kvec.h"
#include "qoi.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARCHIVE_MAGIC 0x4B504E47u // "GNPK"
#define ATLAS_NODE_NAME "atlas"
#define ATLAS_MIN_SIZE 256
#define ATLAS_MAX_SIZE 16384
#define ATLAS_PADDING 1

typedef struct
{
    SDL_Surface *surface;
    char *path;
    int32 x;
    int32 y;
} PackedImage;

typedef struct
{
    ArchiveNode *node;
    bool is_sdl_alloc;
} CookedNodeEntry;

typedef kvec_t(ArchiveAssetData *) asset_data_vec_t;

typedef struct
{
    kvec_t(CookedNodeEntry) nodes;
    kvec_t(PackedImage) images;
    uint32 audio_files;
    uint32 data_files;
    uint32 script_files;
    bool has_error;
} CookerContext;

static char *CopyPath(const char *path)
{
    size_t len = strlen(path) + 1;
    char *copy = (char *)malloc(len);
    memcpy(copy, path, len);
    return copy;
}

static int32 IsConfigJson(const char *path)
{
    return strstr(path, CONFIG_FILE_NAME) != NULL;
}

static ArchiveKind ClassifyFile(const char *path)
{
    if (FileIsLoadableScript(path))
        return ARCHIVE_KIND_SCRIPT;

    if (FileIsLoadableAudio(path))
        return ARCHIVE_KIND_AUDIO;

    if (FileIsLoadableImage(path))
        return ARCHIVE_KIND_IMAGE;

    if (FileIsLoadableText(path) || IsConfigJson(path))
        return ARCHIVE_KIND_DATA;

    return ARCHIVE_KIND_NONE;
}

static SDL_Surface *LoadRgbaSurface(const char *path)
{
    size_t file_size = 0;
    void *file_data = SDL_LoadFile(path, &file_size);
    if (!file_data)
    {
        fprintf(stderr, "Cooker: failed to read image '%s': %s\n", path, SDL_GetError());
        return NULL;
    }

    SDL_IOStream *io = SDL_IOFromConstMem(file_data, file_size);
    if (!io)
    {
        fprintf(stderr, "Cooker: failed to create IO stream for '%s': %s\n", path, SDL_GetError());
        SDL_free(file_data);
        return NULL;
    }

    SDL_Surface *raw = IMG_Load_IO(io, true);
    SDL_free(file_data);
    if (!raw)
    {
        fprintf(stderr, "Cooker: failed to decode image '%s': %s\n", path, SDL_GetError());
        return NULL;
    }

    SDL_Surface *rgba = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_RGBA32);
    if (!rgba)
        fprintf(stderr, "Cooker: failed to convert image '%s' to RGBA32: %s\n", path, SDL_GetError());
    SDL_DestroySurface(raw);
    return rgba;
}

static void CFileCallback(const char *path, void *userdata)
{
    CookerContext *ctx = (CookerContext *)userdata;
    if (ctx->has_error)
        return;

    ArchiveKind kind = ClassifyFile(path);
    if (kind == ARCHIVE_KIND_NONE)
        return;

    char *file_path = CopyPath(path);

    if (kind == ARCHIVE_KIND_IMAGE)
    {
        SDL_Surface *surf = LoadRgbaSurface(path);
        if (!surf)
        {
            free(file_path);
            ctx->has_error = true;
            return;
        }

        PackedImage img;
        CLEAR(img, 0);
        img.surface = surf;
        img.path = file_path;
        kv_push(PackedImage, ctx->images, img);
        return;
    }

    size_t raw_size = 0;
    void *raw_data = SDL_LoadFile(path, &raw_size);
    if (!raw_data)
    {
        fprintf(stderr, "Cooker: failed to read file '%s': %s\n", path, SDL_GetError());
        free(file_path);
        ctx->has_error = true;
        return;
    }

    ArchiveNode *node = (ArchiveNode *)calloc(1, sizeof(ArchiveNode));
    node->content = (unsigned char *)raw_data;
    node->content_size = (uint32)raw_size;
    node->name = file_path;
    node->kind = kind;

    CookedNodeEntry entry = {.node = node, .is_sdl_alloc = true};
    kv_push(CookedNodeEntry, ctx->nodes, entry);

    if (kind == ARCHIVE_KIND_AUDIO)
        ctx->audio_files++;
    else if (kind == ARCHIVE_KIND_DATA)
        ctx->data_files++;
    else if (kind == ARCHIVE_KIND_SCRIPT)
        ctx->script_files++;
}

#define STB_RECT_PACK_IMPLEMENTATION
// https://github.com/nothings/stb/tree/master
#include "stb_rect_pack.h"

static bool TryPack(PackedImage *images, uint32 count, int32 atlas_w, int32 atlas_h)
{
    stbrp_node *nodes = malloc(sizeof(stbrp_node) * (uint32)(atlas_w + ATLAS_PADDING));
    stbrp_rect *rects = malloc(sizeof(stbrp_rect) * count);
    if (nodes == NULL || rects == NULL)
    {
        free(nodes);
        free(rects);
        return false;
    }

    for (uint32 i = 0; i < count; ++i)
    {
        rects[i].id = (int)i;
        rects[i].w = (stbrp_coord)(images[i].surface->w + ATLAS_PADDING);
        rects[i].h = (stbrp_coord)(images[i].surface->h + ATLAS_PADDING);
    }

    // the target is padded too, so an image touching the border still fits
    stbrp_context packer;
    stbrp_init_target(&packer, atlas_w + ATLAS_PADDING, atlas_h + ATLAS_PADDING, nodes, atlas_w + ATLAS_PADDING);

    bool ok = stbrp_pack_rects(&packer, rects, (int)count) != 0;
    if (ok)
    {
        // rects keep their original order after packing
        for (uint32 i = 0; i < count; ++i)
        {
            images[i].x = rects[i].x;
            images[i].y = rects[i].y;
        }
    }

    free(nodes);
    free(rects);
    return ok;
}

static void ApplyPaletteToSurface(SDL_Surface *surface, PaletteManager *manager)
{
    if (!surface || !manager)
        return;

    if (SDL_MUSTLOCK(surface) && !SDL_LockSurface(surface))
    {
        fprintf(stderr, "Cooker: failed to lock surface for palette conversion: %s\n", SDL_GetError());
        return;
    }

    for (int32 y = 0; y < surface->h; ++y)
    {
        uint8 *row = (uint8 *)surface->pixels + (size_t)y * (size_t)surface->pitch;
        for (int32 x = 0; x < surface->w; ++x)
        {
            uint8 *px = row + (size_t)x * 4;
            if (px[3] == 0)
                continue;

            SDL_Color src = {.r = px[0], .g = px[1], .b = px[2], .a = px[3]};
            ColorLAB lab = RgbToLab(&src);
            int32 best_idx = FindBestPaletteColorCIEDE2000(manager, &lab);
            if (best_idx >= 0)
            {
                SDL_Color dst = manager->palette_elements.a[best_idx];
                px[0] = dst.r;
                px[1] = dst.g;
                px[2] = dst.b;
            }
        }
    }

    if (SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);
}

static unsigned char *SurfaceToQoi(SDL_Surface *rgba, uint32 *out_size)
{
    *out_size = 0;
    if (!rgba)
        return NULL;

    qoi_desc desc = {
        .width = (unsigned int)rgba->w,
        .height = (unsigned int)rgba->h,
        .channels = 4,
        .colorspace = QOI_SRGB,
    };

    int32 encoded_len = 0;
    unsigned char *qoi_buf = (unsigned char *)qoi_encode(rgba->pixels, &desc, &encoded_len);
    if (!qoi_buf || encoded_len <= 0)
    {
        fprintf(stderr, "Cooker: failed to encode atlas (%dx%d) to QOI\n", rgba->w, rgba->h);
        free(qoi_buf);
        return NULL;
    }

    *out_size = (uint32)encoded_len;
    return qoi_buf;
}

static bool BuildAtlas(CookerContext *ctx, PaletteManager *manager, bool enable_palette, asset_data_vec_t *out_assets,
                       bool *out_has_atlas)
{
    *out_has_atlas = false;
    size_t count = kv_size(ctx->images);
    if (count == 0)
        return true;

    int32 atlas_w = ATLAS_MIN_SIZE;
    int32 atlas_h = ATLAS_MIN_SIZE;
    bool packed = false;

    while (atlas_w <= ATLAS_MAX_SIZE && atlas_h <= ATLAS_MAX_SIZE)
    {
        if (TryPack(ctx->images.a, count, atlas_w, atlas_h))
        {
            packed = true;
            break;
        }
        if (atlas_w <= atlas_h)
            atlas_w *= 2;
        else
            atlas_h *= 2;
    }

    if (!packed)
    {
        fprintf(stderr, "Cooker: failed to pack %zu images into max atlas size %dx%d\n", count, ATLAS_MAX_SIZE,
                ATLAS_MAX_SIZE);
        return false;
    }

    SDL_Surface *atlas = SDL_CreateSurface(atlas_w, atlas_h, SDL_PIXELFORMAT_RGBA32);
    if (!atlas)
    {
        fprintf(stderr, "Cooker: failed to create atlas surface (%dx%d): %s\n", atlas_w, atlas_h, SDL_GetError());
        return false;
    }

    for (size_t i = 0; i < count; ++i)
    {
        PackedImage *img = &kv_A(ctx->images, i);
        SDL_SetSurfaceBlendMode(img->surface, SDL_BLENDMODE_NONE);

        SDL_Rect dst_rect = {
            .x = img->x,
            .y = img->y,
            .w = img->surface->w,
            .h = img->surface->h,
        };
        SDL_BlitSurface(img->surface, NULL, atlas, &dst_rect);

        ArchiveAssetData *asset = (ArchiveAssetData *)calloc(1, sizeof(ArchiveAssetData));
        asset->size = (vec2s){.x = (float)img->surface->w, .y = (float)img->surface->h};
        asset->positions = (vec2s){.x = (float)img->x, .y = (float)img->y};
        asset->name = img->path;
        img->path = NULL;

        kv_push(ArchiveAssetData *, *out_assets, asset);
    }

    if (enable_palette && manager)
        ApplyPaletteToSurface(atlas, manager);

    uint32 qoi_size = 0;
    unsigned char *qoi_buf = SurfaceToQoi(atlas, &qoi_size);
    SDL_DestroySurface(atlas);
    if (!qoi_buf)
        return false;

    char *atlas_name = (char *)malloc(sizeof(ATLAS_NODE_NAME));
    memcpy(atlas_name, ATLAS_NODE_NAME, sizeof(ATLAS_NODE_NAME));

    ArchiveNode *atlas_node = (ArchiveNode *)calloc(1, sizeof(ArchiveNode));
    atlas_node->content = qoi_buf;
    atlas_node->content_size = qoi_size;
    atlas_node->name = atlas_name;
    atlas_node->kind = ARCHIVE_KIND_IMAGE;

    CookedNodeEntry entry = {.node = atlas_node, .is_sdl_alloc = false};
    kv_push(CookedNodeEntry, ctx->nodes, entry);
    *out_has_atlas = true;
    return true;
}

static void CleanupCooker(CookerContext *ctx, asset_data_vec_t *assets)
{
    for (size_t i = 0; i < kv_size(ctx->images); ++i)
    {
        PackedImage *img = &kv_A(ctx->images, i);
        if (img->surface)
            SDL_DestroySurface(img->surface);
        free(img->path);
    }
    kv_destroy(ctx->images);

    for (size_t i = 0; i < kv_size(ctx->nodes); ++i)
    {
        CookedNodeEntry *entry = &kv_A(ctx->nodes, i);
        if (entry->node)
        {
            if (entry->is_sdl_alloc)
                SDL_free(entry->node->content);
            else
                free(entry->node->content);
            free((void *)entry->node->name);
            free(entry->node);
        }
    }
    kv_destroy(ctx->nodes);

    for (size_t i = 0; i < kv_size(*assets); ++i)
    {
        ArchiveAssetData *asset = kv_A(*assets, i);
        if (asset)
        {
            free((void *)asset->name);
            free(asset);
        }
    }
    kv_destroy(*assets);
}

bool Cooker(AppInfo *app_info, int32 num_dirs, const char **dirs, const char *output_path)
{
    int32 version_num = 0;
    if (!ParseVersionToInt(app_info->version, &version_num))
    {
        fprintf(stderr, "Cooker: invalid version string '%s'\n", app_info->version ? app_info->version : "(null)");
        return false;
    }

    PaletteManager manager = {0};
    if (app_info->enable_palette)
    {
        InitLinearLut();
        manager = PaletteManagerCreate();
        PaletteParse(&manager, &app_info->palette);
    }

    CookerContext ctx;
    CLEAR(ctx, 0);

    kv_init(ctx.nodes);
    kv_init(ctx.images);

    asset_data_vec_t assets_data;
    kv_init(assets_data);

    for (int32 i = 0; i < num_dirs; ++i)
    {
        DirWalk(dirs[i], CFileCallback, &ctx);
        if (ctx.has_error)
            break;
    }

    bool ok = !ctx.has_error;
    bool has_atlas = false;

    if (ok)
        ok = BuildAtlas(&ctx, &manager, app_info->enable_palette, &assets_data, &has_atlas);

    if (ok)
    {
        ArchiveHeader header;
        CLEAR(header, 0);
        header.magic = ARCHIVE_MAGIC;
        header.version = (uint32)version_num;
        header.assets_files = ctx.audio_files + (has_atlas ? 1u : 0u);
        header.data_files = ctx.data_files;
        header.script_files = ctx.script_files;

        uint32 total_nodes = (uint32)kv_size(ctx.nodes);
        ArchiveNode **node_ptrs = NULL;

        if (total_nodes > 0)
        {
            node_ptrs = (ArchiveNode **)malloc(sizeof(ArchiveNode *) * total_nodes);
            for (uint32 i = 0; i < total_nodes; ++i)
                node_ptrs[i] = kv_A(ctx.nodes, i).node;
        }

        ok = WriteArchive(output_path, &header, node_ptrs, total_nodes, assets_data.a, (uint32)kv_size(assets_data));
        if (!ok)
            fprintf(stderr, "Cooker: WriteArchive failed for '%s'\n", output_path);
        free(node_ptrs);
    }

    CleanupCooker(&ctx, &assets_data);
    if (app_info->enable_palette)
        PaletteManagerDestroy(&manager);

    return ok;
}