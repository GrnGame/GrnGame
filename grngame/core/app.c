#include "app.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"
#include "grngame/audio/sound.h"
#include "grngame/audio/sound_info.h"
#include "grngame/bindings/api/callback.h"
#include "grngame/bindings/api/interpretor.h"
#include "grngame/core/app.h"
#include "grngame/core/init.h"
#include "grngame/core/text.h"
#include "grngame/core/thread.h"
#include "grngame/core/window.h"
#include "grngame/data/json.h"
#include "grngame/data/sqlite.h"
#include "grngame/dev/hotreload.h"
#include "grngame/dev/logging.h"
#include "grngame/dev/tracy.h"
#include "grngame/input/input_data.h"
#include "grngame/input/poll_events.h"
#include "grngame/math/math.h"
#include "grngame/math/types.h"
#include "grngame/platform/check_type.h"
#include "grngame/renderer/palette.h"
#include "grngame/renderer/primitive.h"
#include "grngame/utils/attributes.h"
#include "grngame/utils/clear.h"
#include "grngame/utils/random.h"
#include "grngame/utils/time.h"
#include "kvec.h"
#include "message.h"

#ifdef GRNGAME_WASM
#include "grngame/web/web.h"
#endif

#include <stdbool.h>
#include <stdlib.h>

static bool s_is_running = false;

static HOT void MainLoopIteration(void *arg);
static COLD void MainLoop(void);
static COLD void EnsureInitSucceeded(InitResult res);

static HOT void RenderFrame(void);
static COLD void InitializeLoopState(void);
static COLD void CleanupAppResources(void);

static COLD void EnsureInitSucceeded(const InitResult res)
{
    if (UNLIKELY(res == INIT_SDL_FAILED))
    {
        MessageBox_("Failed to initialize SDL.");
        exit(1);
    }

    if (UNLIKELY(res == INIT_SOUND_FAILED))
    {
        MessageBox_("Failed to initialize SDL mixer");
        exit(1);
    }

    if (UNLIKELY(res == INIT_LOG_FAILED))
    {
        MessageBoxWindow("Failed to initialize the logging system.");
        exit(2);
    }

    if (UNLIKELY(res == INIT_CONFIG_FAILED))
    {
        MessageBoxWindow("Failed to load or parse the configuration file.");
        exit(3);
    }

    if (UNLIKELY(res == INIT_OPEN_GAME_DATA_FAILED))
    {
        MessageBoxWindow("Failed to open game data.");
        exit(4);
    }
    if (UNLIKELY(res == INIT_METADATA_FAILED))
    {
        MessageBoxWindow("Failed to read metadata.");
        exit(5);
    }
}

COLD void ShutdownScripts(void)
{
    BindingsCallOnDestroy();
    BindingsFree();
}

void EngineStart()
{
    SetSeed();
    CheckAllTypes();

    const InitResult res = InitAll();
    EnsureInitSucceeded(res);
    BindingsCallOnStart();
    MainLoop();
}

static void EngineStop(void)
{
    if (!s_is_running)
        return;

    LOG_INFO("Engine stopping");

    s_is_running = false;
    CleanupAppResources();
}

static void EngineRequestStop(void)
{
    if (UNLIKELY(g_app.request_stop))
    {
        EngineStop();
    }
}

void EngineSetStop()
{
    g_app.request_stop = true;
}

static void DestroyWindow_()
{
    if (g_app.window)
    {
        SDL_DestroyWindow(g_app.window);
        g_app.window = NULL;
    }
}

static void DestroyRenderer()
{
    if (g_app.renderer.renderer)
    {
        SDL_DestroyRenderer(g_app.renderer.renderer);
        g_app.renderer.renderer = NULL;
    }
}

static COLD void CleanupAppResources(void)
{
    PROFILE_FUNCTION("Cleanup");
    ShutdownScripts();
    DestroyWindow_();
    DestroyRenderer();
    PaletteFreeStringVec(&g_app.info.palette);
    PaletteManagerDestroy(&g_app.palette_manager);
    InputManagerDestroy(&g_app.input_manager);
    AssetManagerDestroy(&g_app.asset_manager);
#ifdef GRNGAME_EMBED_ASSETS
    EmbeddedFileManagerDestroy(&g_app.embedded_file_manager);
#endif
    SoundManagerDestroy(&g_app.sound_manager);
    ThreadManagerDestroy(&g_app.thread_manager);
    JsonManagerDestroy(g_app.json_manager);
    HotReloadDestroy();
    LogDestroy();
    DbClose(g_app.info.file_db);
    DbClose(g_app.info.data_db);

    // todo : factorise code
    if (g_app.info.name)
        free(g_app.info.name);
    if (g_app.info.asset_folder)
        free(g_app.info.asset_folder);
    if (g_app.info.version)
        free(g_app.info.version);
    if (g_app.info.company)
        free(g_app.info.company);
    g_app = (App){0};
}

static HOT void RenderFrame(void)
{
    if (g_app.info.window_occlusion_culled)
        return;

    PROFILE_ZONE_START(render_zone, "Render");

    RendererClear(&g_app.renderer);
    BindingsCallOnRender();
    ApplyBlackStripes();
    RendererPresent(&g_app.renderer);

    PROFILE_ZONE_END(render_zone);
}

static void UpdateEngineTime(const float64 frame_start_time)
{
    float64 frame_elapsed_time = TimeNow() - frame_start_time;
    const float64 target_frame_time = 1.0 / (float64)g_app.info.fps;
    if (frame_elapsed_time < target_frame_time)
    {
        float64 remaining_time = target_frame_time - frame_elapsed_time;
        JsonSaveObjects(remaining_time);
        frame_elapsed_time = TimeNow() - frame_start_time;

        if (frame_elapsed_time < target_frame_time)
        {
            const float64 delay_sec = target_frame_time - frame_elapsed_time;

            SDL_Delay((uint32)(delay_sec * 1000.0));
        }
    }

    g_app.info.dt = (float32)(TimeNow() - frame_start_time);
}

static HOT void MainLoopIteration(void *arg)
{
    (void)arg;

    if (!s_is_running)
        return;

    float64 frame_start_time = TimeNow();

    ProcessHotreloadQueue();
    PollEvents();

    BindingsCallOnUpdate(g_app.info.dt);

    // todo : call fixed update correctly
    BindingsCallOnFixedUpdate(FIXED_DT);

    RenderFrame();
    ClearAll();

    g_app.info.frame_count++;
    UpdateEngineTime(frame_start_time);

    EngineRequestStop();
}

static COLD void InitializeLoopState(void)
{
    s_is_running = true;
    g_app.info.frame_count = 0;
}

static COLD void MainLoop(void)
{
    InitializeLoopState();
    SDL_ShowWindow(g_app.window);

#ifdef GRNGAME_WASM
    WEB_LOOP(MainLoopIteration);
#else
    while (LIKELY(s_is_running))
    {
        MainLoopIteration(NULL);
    }
#endif
}

void ReloadConfig(void)
{

    PROFILE_FUNCTION("Reload");
    InitAppConfig();
    LogDestroy();
    LogInit(LOG_TO_CONSOLE);
    WindowApplyConfig(&g_app.info);
    if (g_app.info.enable_palette)
        PaletteReload();
    LOG_INFO("sucessfuly reload config");
}

App g_app;