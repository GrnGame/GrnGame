#include "init.h"
#include "data.h"
#include "grngame/assets/asset_manager.h"
#include "grngame/assets/embedded_file_manager.h"
#include "grngame/bindings/api/callback.h"
#include "grngame/bindings/api/interpretor.h"
#include "grngame/core/app.h"
#include "grngame/core/window.h"
#include "grngame/data/file.h"
#include "grngame/data/json.h"
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
#include "grngame/utils/time.h"
#include "grngame/utils/version.h"
#include "kvec.h"
#include "text.h"
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

static bool ReadConfigValue(const char *key, const unsigned char *memory, JsonValue *value, uint32 size)
{
    *value =
        memory == NULL ? JsonReadValueFromDirectory(CONFIG, key) : JsonReadValueFromMemory(CONFIG, key, memory, size);
    return value->type != JSONV_VOID;
}

static bool ParseConfig(const unsigned char *memory, uint32 size)
{
    JsonValue value;

    if (!ReadConfigValue("fps", memory, &value, size) || (value.type != JSONV_INTEGER && value.type != JSONV_FLOAT))
        goto invalid;
    g_app.info.fps = (uint16)(value.type == JSONV_INTEGER ? value.value.i : value.value.f);
    JsonValueFree(&value);

    if (!ReadConfigValue("windowWidth", memory, &value, size) ||
        (value.type != JSONV_INTEGER && value.type != JSONV_FLOAT))
        goto invalid;
    g_app.info.window_width = (float32)(value.type == JSONV_INTEGER ? value.value.i : value.value.f);
    JsonValueFree(&value);

    if (!ReadConfigValue("windowHeight", memory, &value, size) ||
        (value.type != JSONV_INTEGER && value.type != JSONV_FLOAT))
        goto invalid;
    g_app.info.window_height = (float32)(value.type == JSONV_INTEGER ? value.value.i : value.value.f);
    JsonValueFree(&value);

    if (!ReadConfigValue("universeWidth", memory, &value, size) ||
        (value.type != JSONV_INTEGER && value.type != JSONV_FLOAT))
        goto invalid;
    g_app.info.window_universe_width = (uint16)(value.type == JSONV_INTEGER ? value.value.i : value.value.f);
    JsonValueFree(&value);

    if (!ReadConfigValue("universeHeight", memory, &value, size) ||
        (value.type != JSONV_INTEGER && value.type != JSONV_FLOAT))
        goto invalid;
    g_app.info.window_universe_height = (uint16)(value.type == JSONV_INTEGER ? value.value.i : value.value.f);
    JsonValueFree(&value);

    if (!ReadConfigValue("resizable", memory, &value, size) || value.type != JSONV_BOOL)
        goto invalid;
    g_app.info.window_resizable = value.value.b;
    JsonValueFree(&value);

    if (!ReadConfigValue("fullscreen", memory, &value, size) || value.type != JSONV_BOOL)
        goto invalid;
    g_app.info.window_fullscreen = value.value.b;
    JsonValueFree(&value);

    if (!ReadConfigValue("maximised", memory, &value, size) || value.type != JSONV_BOOL)
        goto invalid;
    g_app.info.window_maximised = value.value.b;
    JsonValueFree(&value);

    if (!ReadConfigValue("bordered", memory, &value, size) || value.type != JSONV_BOOL)
        goto invalid;
    g_app.info.bordered = value.value.b;
    JsonValueFree(&value);

    if (!ReadConfigValue("forceUniverseScale", memory, &value, size) || value.type != JSONV_BOOL)
        goto invalid;
    g_app.info.force_universe_scale = value.value.b;
    JsonValueFree(&value);

    if (!ReadConfigValue("enable_palette", memory, &value, size) || value.type != JSONV_BOOL)
        goto invalid;
    g_app.info.enable_palette = value.value.b;
    JsonValueFree(&value);

    if (!ReadConfigValue("name", memory, &value, size) || value.type != JSONV_TEXT)
        goto invalid;
    g_app.info.name = value.value.s;

    if (!ReadConfigValue("assetFolder", memory, &value, size) || value.type != JSONV_TEXT)
        goto invalid;
    g_app.info.asset_folder = value.value.s;

    if (!ReadConfigValue("version", memory, &value, size) || value.type != JSONV_TEXT)
        goto invalid;
    g_app.info.version = value.value.s;

    if (!ReadConfigValue("company", memory, &value, size) || value.type != JSONV_TEXT)
        goto invalid;
    g_app.info.company = value.value.s;

    if (!ReadConfigValue("language", memory, &value, size) || value.type != JSONV_TEXT)
        goto invalid;

    if (strcmp("angelscript", value.value.s) == 0)
    {
        g_app.info.scripting_langage = ANGEL_SCRIPT;
    }
    else
    {
        LOG_WARNING("scripting language : %s is not supported set angelscript to default", value.value.s);
    }

    if (!ReadConfigValue("palette", memory, &value, size) ||
        (value.type != JSONV_TEXT_ARRAY && value.type != JSONV_EMPTY_ARRAY))
        goto invalid;

    if (value.type == JSONV_EMPTY_ARRAY)
        kv_init(g_app.info.palette);
    else
        g_app.info.palette = value.value.sa;

    return true;

// first time of my life used a goto , but best choice here because used specific code error will be bloated the code
invalid:
    JsonValueFree(&value);
    LOG_ERROR("Failed to read configuration value from %s", CONFIG);
    return false;
}

static InitResult LoadAppConfig()
{
    if (!ParseConfig(NULL, 0))
    {
        LOG_ERROR("%s", "Failed to parse config.json : somes parameters arent here");
        return INIT_CONFIG_FAILED;
    }
    return INIT_OK;
}

static InitResult LoadAppConfigEmbedded()
{
    const EmbeddedFile *asset = EmbeddedFileGet("config/config.json");
    if (!asset)
    {
        LOG_ERROR("%s", "Failed to get config.json in Assets.pak.Did you delete it before package your app ?");
        return INIT_CONFIG_FAILED;
    }

    if (!ParseConfig(asset->data, asset->size))
    {
        LOG_ERROR("%s", "Failed to parse config.json : somes parameters arent here");
        return INIT_CONFIG_FAILED;
    }
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
        const EmbeddedFile *asset = EmbeddedFileGet("data/gamecontrollerdb.txt");
        if (!asset)
        {
            LOG_ERROR("Failed to get gamecontrollerdb.txt in Assets.pak,did you delete it before package your app?");
            return NULL;
        }
        return SDL_IOFromConstMem(asset->data, asset->size);
    }
#else

    char *path = PathFromExecutableDirectory("data/gamecontrollerdb.txt");
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
    SetTaskBarIconErrorProgress(100.0);
}

InitResult InitializeWindow(void)
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

InitResult InitializeManagers(void)
{
    g_app.asset_manager = AssetManagerCreate();
    g_app.input_manager = InputManagerCreate();

    if (UNLIKELY(!SoundManagerTryCreate(&g_app.sound_manager)))
        return INIT_SOUND_FAILED;

    return INIT_OK;
}

void InitializeJson(void)
{
    g_app.json_manager = JsonManagerCreate();
}

void InitializePalette(void)
{
    InitLinearLut();
    g_app.palette_manager = PaletteManagerCreate();
    if (g_app.info.enable_palette)
        PaletteParse(&g_app.info.palette);
}

void InitializeAssets(void)
{
    char *asset_path = PathFromExecutableDirectory(g_app.info.asset_folder);
#ifndef GRNGAME_EMBED_ASSETS
    AssetManagerLoadFolder(asset_path);
#else
    AssetManagerLoadFolderFromMemory(asset_path);
#endif
    free(asset_path);
}

void InitializeScripts(void)
{
#ifndef GRNGAME_EMBED_ASSETS
    if (!BindingsInit("scripts/main.as"))
    {
        HandleBindingsFailure();
        return;
    }
    if (g_app.info.scripting_langage == ANGEL_SCRIPT)
        LOG_INFO("Angel runtime initialized successfully with script 'main.as'");
    else
    {
    }
#else
    if (!BindingsInitFromMemory("scripts/main.as"))
    {
        HandleBindingsFailure();
        return;
    }
    if (g_app.info.scripting_langage == ANGEL_SCRIPT)
        LOG_INFO("Angel runtime initialized successfully with script 'main.as'");
    else
    {
    }
#endif
}

InitResult InitAll(void)
{
    PROFILE_FUNCTION("initialization");

    g_app = (App){0};

    InitResult result = InitializeLogging();
    if (result != INIT_OK)
        return result;

    InitializeJson();

#if GRNGAME_WASM
    g_app.embedded_file_manager = EmbeddedFileManagerCreate();
    if (!EmbeddedFileManagerLoadAssetsPak())
        return INIT_OPEN_GAME_DATA_FAILED;
#elif GRNGAME_EMBED_ASSETS
#ifdef GRNGAME_ANDROID
    g_app.embedded_file_manager = EmbeddedFileManagerCreate();
    char *storage = PathFromAndroidStorage("Assets.pak");
    if (!MoveFile_(storage, ASSETS_PAK))
    {
        free(storage);
        return INIT_OPEN_GAME_DATA_FAILED;
    }

    g_app.info.file_db = DbCreate(storage);
    free(storage);
#else
    g_app.embedded_file_manager = EmbeddedFileManagerCreate();
    g_app.info.file_db = DbCreate("Assets.pak");

#endif

    if (!EmbeddedFileManagerLoadAssetsPak())
        return INIT_OPEN_GAME_DATA_FAILED;
#endif

    result = InitAppConfig();
    if (result != INIT_OK)
        return result;

#ifdef GRNGAME_EMBED_ASSETS
#ifndef GRNGAME_WASM
    if (!InstallGameData() || !InitGameData())
        return INIT_OPEN_GAME_DATA_FAILED;

    if (!EmbeddedFileManagerLoadGameData())
        return INIT_OPEN_GAME_DATA_FAILED;
#endif
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

    InitializePalette();

    InitializeAssets();

    InitializeScripts();

#ifdef GRNGAME_EMBED_ASSETS
    if (g_app.info.update_event_data)
    {
        if (!ApplyDbVersion(g_app.info.data_db, g_app.info.version))
        {
            LOG_ERROR("Failed to store new version for game.data :'%s'", g_app.info.version);
            g_app.info.update_event_data = false;
        }
    }
#endif

    // todo : refactor
    if (g_app.info.update_event_data)
    {
        CallbackArg args[2] = {
            {.type = CB_ARG_STRING, .as.string = g_app.info.old_version},
            {.type = CB_ARG_STRING, .as.string = g_app.info.version},
        };

        BindingsCallBack(args, GAME_DATA_NEW_VERSION);
    }

    HotReloadInit(PathFromExecutableDirectory("."));

    LOG_INFO("All engine subsystems initialized");

    return INIT_OK;
}
