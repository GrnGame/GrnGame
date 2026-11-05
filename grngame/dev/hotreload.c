#if defined(GRNGAME_HOT_RELOAD_ENABLE)
#include "grngame/dev/hotreload.h"
#include "grngame/assets/load.h"
#include "grngame/bindings/api/interpretor.h"
#include "grngame/core/app.h"
#include "grngame/dev/logging.h"
#include "grngame/platform/paths.h"
#include "grngame/utils/string_compat.h"
#include "logging.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <string.h>

#define DMON_IMPL
#include "dmon.h"

static void HotReloadInitQueue()
{
    kv_init(g_app.queue);
}

static void HotReloadDestroyQueue()
{
    kv_destroy(g_app.queue);
}

static SDL_Mutex *g_queue_mutex = NULL;

static void watch_callback(dmon_watch_id watch_id, dmon_action action, const char *rootdir, const char *filepath,
                           const char *oldfilepath, void *user)
{
    if (!filepath)
        return;

    HotreloadQueueElement elem = {0};
    switch (action)
    {
    case DMON_ACTION_CREATE:
        elem.action = ADD;
        break;
    case DMON_ACTION_DELETE:
        elem.action = DELETE_;
        break;
    case DMON_ACTION_MODIFY:
        elem.action = MODIFIED;
        break;
    case DMON_ACTION_MOVE:
        elem.action = MOVED;
        break;
    }

    elem.new_file = strdup(filepath);
    if (oldfilepath)
        elem.old_file = strdup(oldfilepath);
    else
        elem.old_file = NULL;
    if (g_queue_mutex)
        LOCK_MUTEX(g_queue_mutex);

    kv_push(HotreloadQueueElement, g_app.queue, elem);

    if (g_queue_mutex)
        UNLOCK_MUTEX(g_queue_mutex);
}

void HotReloadInit(const char *folder)
{
    HotReloadInitQueue();
    dmon_init();
    dmon_watch(folder, watch_callback, DMON_WATCHFLAGS_RECURSIVE, NULL);
}

void HotReloadDestroy()
{
    dmon_deinit();
    HotReloadDestroyQueue();
}

void ProcessHotreloadQueue(void)
{
    if (g_queue_mutex)
        LOCK_MUTEX(g_queue_mutex);

    uint32 count = (uint32)kv_size(g_app.queue);

    for (uint32 i = 0; i < count; ++i)
    {
        HotreloadQueueElement elem = kv_A(g_app.queue, i);
        const char *cpath = elem.new_file;
        const char *oldCPath = elem.old_file;

        switch (elem.action)
        {
        case ADD: {
            LOG_DEBUG("File added '%s'", cpath);

            if (FileIsLoadableScript(cpath))
            {
                LOG_INFO("Detected new script '%s'", cpath);
                if (!(BindingsReload("scripts/main.as")))
                    LOG_WARNING("Failed to reload script '%s'", cpath);
            }

            if (FileIsLoadableAudio(cpath))
            {
                LOG_INFO("Detected new audio '%s'", cpath);
                bool load_result = LoadSoundFile(cpath);

                if (!load_result)
                    LOG_WARNING("Failed to load audio file '%s'", cpath);
                else
                    LOG_DEBUG("Loaded audio file '%s'", cpath);
            }

            if (FileIsLoadableImage(cpath))
            {
                LOG_INFO("Detected new image '%s'", cpath);
                bool load_result = LoadTextureFile(cpath);

                if (!load_result)
                    LOG_WARNING("Failed to load texture file '%s'", cpath);
                else
                    LOG_DEBUG("Loaded texture file '%s'", cpath);
            }
            break;
        }

        case DELETE_: {
            LOG_DEBUG("Asset deleted '%s'", cpath);

            if (FileIsLoadableScript(cpath))
            {
                LOG_INFO("Detected deleted script '%s'", cpath);
                if (!BindingsReload("scripts/main.as"))
                    LOG_WARNING("Failed to reload script '%s'", cpath);
            }

            if (FileIsLoadableAudio(cpath))
            {
                LOG_INFO("Detected deleted audio '%s'", cpath);
                bool unload_result = UnloadSoundFile(cpath);

                if (!unload_result)
                    LOG_WARNING("Failed to unload audio file '%s'", cpath);
                else
                    LOG_DEBUG("Unloaded audio file '%s'", cpath);
            }

            if (FileIsLoadableImage(cpath))
            {
                LOG_INFO("Detected deleted image '%s'", cpath);
                bool unload_result = UnloadTextureFile(cpath);

                if (!unload_result)
                    LOG_WARNING("Failed to unload texture file '%s'", cpath);
                else
                    LOG_DEBUG("Unloaded texture file '%s'", cpath);
            }
            break;
        }

        case MODIFIED: {
            // logs always modified
            if (strstr(cpath, "grngame.log") == NULL)
                LOG_DEBUG("File modified '%s'", cpath);

            if (FileIsLoadableScript(cpath))
            {
                LOG_INFO("Detected modified script '%s'", cpath);
                if (!BindingsReload("scripts/main.as"))
                    LOG_WARNING("Failed to reload script '%s'", cpath);
            }
            // we can reload the config
            if (strstr(cpath, "config.json") != NULL)
            {
                ReloadConfig();
            }

            if (FileIsLoadableAudio(cpath))
            {
                LOG_INFO("Detected modified audio '%s'", cpath);
                bool unload_result = UnloadSoundFile(cpath);

                if (!unload_result)
                    LOG_WARNING("Failed to unload audio file '%s'", cpath);

                bool load_result = LoadSoundFile(cpath);

                if (!load_result)
                    LOG_WARNING("Failed to reload audio file '%s'", cpath);
                else
                    LOG_DEBUG("Reloaded audio file '%s'", cpath);
            }

            if (FileIsLoadableImage(cpath))
            {
                LOG_INFO("Detected modified image '%s'", cpath);
                bool unload_result = UnloadTextureFile(cpath);

                if (!unload_result)
                    LOG_WARNING("Failed to unload texture file '%s'", cpath);

                bool load_result = LoadTextureFile(cpath);

                if (!load_result)
                    LOG_WARNING("Failed to reload texture file '%s'", cpath);
                else
                    LOG_DEBUG("Reloaded texture file '%s'", cpath);
            }
            break;
        }

        case MOVED: {
            LOG_DEBUG("Asset moved '%s' -> '%s'", oldCPath, cpath);

            if (FileIsLoadableAudio(oldCPath))
            {
                LOG_INFO("Detected moved audio '%s'", cpath);
                bool unload_result = UnloadSoundFile(oldCPath);

                if (!unload_result)
                    LOG_WARNING("Failed to unload moved audio file '%s'", oldCPath);
            }

            if (FileIsLoadableImage(oldCPath))
            {
                LOG_INFO("Detected moved image '%s'", cpath);
                bool unload_result = UnloadTextureFile(oldCPath);
                if (!unload_result)
                    LOG_WARNING("Failed to unload moved texture file '%s'", oldCPath);
            }

            if (FileIsLoadableScript(cpath))
            {
                LOG_INFO("Detected moved script '%s'", cpath);
                if (!BindingsReload("scripts/main.as"))
                    LOG_WARNING("Failed to reload script '%s'", cpath);
            }

            if (FileIsLoadableAudio(cpath))
            {
                bool load_result = LoadSoundFile(cpath);

                if (!load_result)
                    LOG_WARNING("Failed to load moved audio file '%s'", cpath);
                else
                    LOG_DEBUG("Loaded moved audio file '%s'", cpath);
            }

            if (FileIsLoadableImage(cpath))
            {
                bool load_result = LoadTextureFile(cpath);

                if (!load_result)
                    LOG_WARNING("Failed to load moved texture file '%s'", cpath);
                else
                    LOG_DEBUG("Loaded moved texture file '%s'", cpath);
            }
            break;
        }
        }

        if (elem.new_file)
            free((void *)elem.new_file);
        if (elem.old_file)
            free((void *)elem.old_file);
    }

    kv_size(g_app.queue) = 0;

    if (g_queue_mutex)
        UNLOCK_MUTEX(g_queue_mutex);
}

#else

void HotReloadInit(const char *folder)
{
    (void)folder;
}

void HotReloadDestroy()
{
    return;
}

void ProcessHotreloadQueue(void)
{
    return;
}

#endif