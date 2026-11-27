#include "init.h"
#include "grngame/assets/asset_manager.h"
#include "grngame/bindings/api/callback.h"
#include "grngame/bindings/api/interpretor.h"
#include "grngame/core/app.h"
#include "grngame/core/window.h"
#include "grngame/data/file.h"
#include "grngame/data/json_read.h"
#include "grngame/data/sqlite.h"
#include "grngame/dev/hotreload.h"
#include "grngame/dev/logging.h"
#include "grngame/dev/tracy.h"
#include "grngame/platform/paths.h"
#include "grngame/renderer/cielab.h"
#include "grngame/renderer/palette.h"
#include "grngame/utils/attributes.h"
#include "grngame/utils/taskbar_icon.h"
#include "kvec.h"
#include "grngame/utils/time.h"
#include "grngame/math/types.h"

#include <SDL3/SDL.h>
#include <stddef.h>
#include <stdlib.h>

static InitResult InitializeLogging(void)
{
#ifdef GRNGAME_EMBED_ASSETS
    if (!LogInit(LOG_TO_CONSOLE)) // temp
    {
        LOG_ERROR("Failed to initialize logging");
        return INIT_LOG_FAILED;
    }
#else
    if (!LogInit(LOG_TO_CONSOLE))
    {
        LOG_ERROR("Failed to initialize logging");
        return INIT_LOG_FAILED;
    }
#endif

    return INIT_OK;
}

static void ConfigureSDLHints(void)
{
#ifndef GRNGAME_SOFTWARE
    SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "1");

#if defined(GRNGAME_WINDOWS)
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "vulkan,opengl,software");
#elif defined(GRNGAME_WASM)
    SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#window");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2,software");
#elif defined(GRNGAME_MACOS)
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal,opengl,software");
#elif defined(GRNGAME_LINUX)
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "vulkan,opengl,software");
#endif
#else
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
#endif
}

static InitResult InitializeSDL(void)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO))
    {
        LOG_ERROR("SDL initialization failed: %s", SDL_GetError());
        return INIT_SDL_FAILED;
    }

    SDL_SetEventEnabled(SDL_EVENT_MOUSE_MOTION, false);

    return INIT_OK;
}

static InitResult SetSDLMetadata(void)
{
    if (!SetAppMetadata(&g_app.info, g_app.info.name, g_app.info.version, g_app.info.name))
        return INIT_SDL_FAILED;

    return INIT_OK;
}

static InitResult LoadAppConfig()
{
    AppInfo *info = ParseConfig(NULL, 0);
    if (info == NULL)
    {
        LOG_ERROR("%s", "Failed to parse : somes parameters arent here");
        return INIT_CONFIG_FAILED;
    }
    g_app.info = *info;
    free(info);
    return INIT_OK;
}

static InitResult LoadAppConfigEmbedded()
{
    const CookerFile *asset = CookerFileGet(CONFIG_FILE);
    if (!asset)
    {
        LOG_ERROR("Failed to get %s in %s.Did you delete it before package your app ?", CONFIG_FILE, ASSETS_PAK);
        return INIT_CONFIG_FAILED;
    }

    AppInfo *info = ParseConfig(asset->data, asset->size);
    if (info == NULL)
    {

        LOG_ERROR("Failed to parse %s : somes parameters arent here", CONFIG_FILE);
        return INIT_CONFIG_FAILED;
    }
    g_app.info = *info;
    free(info);
    return INIT_OK;
}

InitResult InitAppConfig(void)
{
#ifdef GRNGAME_EMBED_ASSETS
    return LoadAppConfigEmbedded();
#else
    return LoadAppConfig();
#endif
}

static SDL_IOStream *LoadControllerDatabase(void)
{
#ifdef GRNGAME_EMBED_ASSETS
    {
        const CookerFile *asset = CookerFileGet(DATA_GAME_DB);
        if (!asset)
        {
            LOG_ERROR("Failed to get %s in Assets.pak,did you delete it before package your app?", DATA_GAME_DB);
            return NULL;
        }
        return SDL_IOFromConstMem(asset->data, asset->size);
    }
#else

    char *path = PathFromExecutableDirectory(DATA_GAME_DB);
    SDL_IOStream *stream = SDL_IOFromFile(path, "rb");
    free(path);
    return stream;
#endif
}

static void LoadControllerMappings(void)
{
    SDL_IOStream *stream = LoadControllerDatabase();
    if (!stream)
    {
        LOG_WARNING("Unable to load controller database: %s", SDL_GetError());
        return;
    }

    int32 mapped = SDL_AddGamepadMappingsFromIO(stream, true);
    if (mapped < 0)
    {
        LOG_WARNING("Failed to load gamepad mappings: %s", SDL_GetError());
        return;
    }

    LOG_INFO("Loaded %d controller mappings", mapped);
}

static void HandleBindingsFailure(void)
{
    const SDL_Color red = {255, 0, 0, 255};
    RendererSetColor(red.r, red.g, red.b, red.a);
    SetTaskBarIconErrorProgress(100.0f);
}

static InitResult InitializeWindow(void)
{
    g_app.info.offset_x = 0;
    g_app.info.offset_y = 0;
    g_app.info.window_occlusion_culled = false;

    g_app.window = WindowCreate(&g_app.info);
    if (UNLIKELY(!g_app.window))
        return INIT_SDL_FAILED;

    if (UNLIKELY(!RendererTryCreate(g_app.window, &g_app.renderer)))
        return INIT_SDL_FAILED;

    WindowApplyConfig(&g_app.info);

    return INIT_OK;
}

static InitResult InitializeManagers(void)
{
    g_app.asset_manager = AssetManagerCreate();
    g_app.input_manager = InputManagerCreate();

    if (UNLIKELY(!SoundManagerTryCreate(&g_app.sound_manager)))
        return INIT_SOUND_FAILED;

    return INIT_OK;
}
static void InitializeAssets(void)
{
    char *asset_path = PathFromExecutableDirectory(ASSET_FOLDER);
#ifndef GRNGAME_EMBED_ASSETS
    AssetManagerLoadFolder(asset_path);
#else
    AssetManagerLoadFolderFromMemory(asset_path);
#endif
    free(asset_path);
}

// only in dev mode
static void InitializePalette(void)
{
    if (g_app.info.enable_palette)
    {
        InitLinearLut();
        g_app.palette_manager = PaletteManagerCreate();
        PaletteParse(&g_app.palette_manager, &g_app.info.palette);
    }
}

static void InitializeScripts(void)
{
    const char *main_file = GetMainFile();

#ifdef GRNGAME_EMBED_ASSETS
    bool success = ScriptRunFromMemory(main_file);
#else
    bool success = ScriptRun(main_file);
#endif

    if (!success)
    {
        HandleBindingsFailure();
        return;
    }

    LOG_INFO("Runtime initialized successfully with script: %s", main_file);
}

InitResult InitAll(void)
{

    float64 start = TimeNow();

    g_app = (App){0};

    InitResult result = InitializeLogging();
    if (result != INIT_OK)
        return result;

#ifdef GRNGAME_EMBED_ASSETS
    g_app.cooker_file_manager = CookerFileManagerCreate();
    if (!CookerFileManagerLoadAssetsPak())
    {
        return INIT_DATA_FAILED;
    }

#endif

    result = InitAppConfig();
    if (result != INIT_OK)
        return result;

#ifndef GRNGAME_EMBED_ASSETS
    InitializePalette();
#endif

    result = InitializeSDL();
    if (result != INIT_OK)
        return result;

    ThreadManagerCreate();
    ConfigureSDLHints();

    result = SetSDLMetadata();
    if (result != INIT_OK)
        return result;

    result = InitializeWindow();
    if (result != INIT_OK)
        return result;

    result = InitializeManagers();
    if (result != INIT_OK)
        return result;

    LoadControllerMappings();

    InitializeAssets();

    InitializeScripts();

    // todo callback when game data update

    HotReloadInit(PathFromExecutableDirectory("."));

    LOG_INFO("All engine subsystems initialized");

    float64 end = TimeNow();

    LOG_DEBUG("Init in :%f", end - start);

    return INIT_OK;
}
