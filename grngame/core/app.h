#pragma once
#include "../input/input_manager.h"
#include "../math/types.h"
#include "SDL3/SDL_pixels.h"
#include "grngame/assets/asset_manager.h"
#include "grngame/audio/sound_manager.h"
#include "grngame/bindings/angelscript/api/interpretor.h"
#include "grngame/core/text.h"
#include "grngame/data/json.h"
#include "grngame/data/sqlite.h"
#include "grngame/dev/hotreload.h"
#include "grngame/dev/logging.h"
#include "grngame/renderer/cielab.h"
#include "grngame/renderer/palette.h"
#include "grngame/renderer/renderer.h"
#include "grngame/utils/macro.h"
#include "khash.h"
#include "kvec.h"
#include "thread.h"

BEGIN_DECLARATIONS

#define FIXED_UPDATE_LOOP 30.0f

#define FIXED_DT (1.0 / FIXED_UPDATE_LOOP)

typedef enum
{
    ANGEL_SCRIPT,
    DAS_SCRIPT,
} SCRIPTING_LANGAGE;

typedef struct AppInfo
{
    sqlite3 *file_db;
    sqlite3 *data_db;
    bool update_event_data;
    char *name;
    char *company;
    char *version;
    char *old_version;
    char *asset_folder;

    SCRIPTING_LANGAGE scripting_langage;

    uint16 fps;
    float32 dt;
    uint32 frame_count;

    uint16 window_universe_width;
    uint16 window_universe_height;
    float32 window_width;
    float32 window_height;

    bool window_resizable;
    bool window_fullscreen;
    bool window_maximised;
    bool window_change;
    bool window_occlusion_culled;
    bool bordered;

    float32 offset_x;
    float32 offset_y;

    bool force_universe_scale; // apply black stripes to the window

    bool enable_palette;
    string_vec_t palette;

} AppInfo;

typedef struct
{
    SDL_Window *window;
    Renderer renderer;

    SoundManager sound_manager;
    AssetManager asset_manager;
    InputManager input_manager;
    JsonManager json_manager;
    AngelManager angel_manager;
    EmbeddedFileManager embedded_file_manager;
    PaletteManager palette_manager;
    ThreadManager thread_manager;
    kvec_t(HotreloadQueueElement) queue;
    AppInfo info;
    bool request_stop;

} App;

void EngineStart();
void EngineSetStop();
COLD void ShutdownScripts(void);
void ReloadConfig(void);

extern App g_app;
extern bool request_stop;

#define STD "std"

#define CONFIG "config/config.json"

#define METADATA "metadata.json"

#define ASSETS_PAK "Assets.pak"
#define SAVE_FILE "game.data"

END_DECLARATIONS
