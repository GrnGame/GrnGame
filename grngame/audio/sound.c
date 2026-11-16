#include "sound.h"
#include "cglm/util.h"
#include "filter.h"
#include "grngame/core/app.h"
#include "grngame/dev/logging.h"
#include "grngame/math/math.h"
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <math.h>
#include <string.h>

#define SOUND_STOP_FADE_MS 80
#define SOUND_PITCH_MIN 0.01f
#define SOUND_PITCH_MAX 100.0f
#define SOUND_DEPTH 1.0f

typedef void (*SlotAction)(SoundSlot *slot);

static void SlotRefreshPosition(SoundSlot *slot);

static SoundManager *Manager(void)
{
    return &g_app.sound_manager;
}

static bool HasPosition(vec2s position)
{
    return !isnan(position.x) && !isnan(position.y);
}

static bool SamePosition(vec2s a, vec2s b)
{
    return fabsf(a.x - b.x) < SOUND_POSITION_EPSILON && fabsf(a.y - b.y) < SOUND_POSITION_EPSILON;
}

static bool SlotIsActive(const SoundSlot *slot)
{
    return slot->track && MIX_TrackPlaying(slot->track);
}

static bool SlotIsFree(const SoundSlot *slot)
{
    return !slot->track || (!MIX_TrackPlaying(slot->track) && !MIX_TrackPaused(slot->track));
}

static bool SlotMatches(const SoundSlot *slot, const char *name)
{
    return SlotIsActive(slot) && strcmp(slot->name, name) == 0;
}

static void ForEachSlot(SlotAction action)
{
    SoundManager *manager = Manager();
    for (int32 i = 0; i < SOUND_MAX_SLOTS; i++)
        action(&manager->slots[i]);
}

static void ForEachNamed(const char *name, SlotAction action)
{
    SoundManager *manager = Manager();
    for (int32 i = 0; i < SOUND_MAX_SLOTS; i++)
        if (SlotMatches(&manager->slots[i], name))
            action(&manager->slots[i]);
}

static SoundSlot *AcquireSlot(void)
{
    SoundManager *manager = Manager();
    for (int32 i = 0; i < SOUND_MAX_SLOTS; i++)
        if (SlotIsFree(&manager->slots[i]))
            return &manager->slots[i];
    return NULL;
}

static bool SlotEnsureTrack(SoundSlot *slot)
{
    if (!slot->track)
        slot->track = MIX_CreateTrack(Manager()->mixer);
    if (slot->track != NULL)
        return true;

    LOG_ERROR("Failed to create sound track: %s", SDL_GetError());
    return false;
}

static void SDLCALL FilterCallback(void *userdata, MIX_Track *track, const SDL_AudioSpec *spec, float32 *pcm,
                                   int samples)
{
    FilterChainProcess((FilterChain *)userdata, pcm, samples, spec->channels);
}

static bool SlotClearFilters(SoundSlot *slot)
{
    bool success = MIX_SetTrackCookedCallback(slot->track, NULL, NULL);
    if (!success)
        LOG_WARNING("Failed to clear sound filter callback: %s", SDL_GetError());

    FilterChainDestroy(slot->filters);
    slot->filters = NULL;
    return success;
}

static bool SlotApplyFilters(SoundSlot *slot, const SoundInfo *info)
{
    if (!SlotClearFilters(slot))
        return false;

    if (info->filter_count <= 0)
        return true;

    SDL_AudioSpec spec;
    if (!MIX_GetMixerFormat(Manager()->mixer, &spec))
    {
        LOG_ERROR("Failed to get mixer format for sound filters: %s", SDL_GetError());
        return false;
    }

    slot->filters = FilterChainCreate(info->filters, info->filter_count, &spec);

    if (!MIX_SetTrackCookedCallback(slot->track, FilterCallback, slot->filters))
    {
        LOG_ERROR("Failed to apply sound filter callback: %s", SDL_GetError());
        FilterChainDestroy(slot->filters);
        slot->filters = NULL;
        return false;
    }

    return true;
}

static bool SlotApplyPan(SoundSlot *slot, float32 pan)
{
    if (pan == 0.0f)
    {
        if (!MIX_SetTrack3DPosition(slot->track, NULL))
        {
            LOG_WARNING("Failed to reset sound track position: %s", SDL_GetError());
            return false;
        }
        return true;
    }

    pan = glm_clamp(pan, -1.0f, 1.0f);
    MIX_StereoGains gains = {1.0f - glm_max(pan, 0.0f), 1.0f + glm_min(pan, 0.0f)};
    if (!MIX_SetTrackStereo(slot->track, &gains))
    {
        LOG_WARNING("Failed to apply sound pan: %s", SDL_GetError());
        return false;
    }
    return true;
}

static MIX_Point3D ToSoundSpace(vec2s world)
{
    SoundManager *manager = Manager();
    vec2s listener = manager->listener;
    float32 half_width = glm_max(manager->half_view.x, 1.0f);
    float32 half_height = glm_max(manager->half_view.y, 1.0f);

    MIX_Point3D point = {(world.x - listener.x) / half_width, (world.y - listener.y) / half_height, -SOUND_DEPTH};
    return point;
}

void SetSoundViewSize(float32 width, float32 height)
{
    Manager()->half_view = (vec2s){.x = width * 0.5f, .y = height * 0.5f};
    ForEachSlot(SlotRefreshPosition);
}

static bool SlotApplyPosition(SoundSlot *slot)
{
    MIX_Point3D point = ToSoundSpace(slot->position);
    if (!MIX_SetTrack3DPosition(slot->track, &point))
    {
        LOG_WARNING("Failed to apply sound position: %s", SDL_GetError());
        return false;
    }
    return true;
}

static bool SlotApplySpatial(SoundSlot *slot, const SoundInfo *info)
{
    slot->position = info->position;
    return HasPosition(slot->position) ? SlotApplyPosition(slot) : SlotApplyPan(slot, info->pan);
}

static void SlotRefreshPosition(SoundSlot *slot)
{
    if (SlotIsActive(slot) && HasPosition(slot->position))
    {
        if (!SlotApplyPosition(slot))
            LOG_WARNING("Failed to refresh active sound position");
    }
}

static bool SlotConfigure(SoundSlot *slot, MIX_Audio *audio, const char *name, const SoundInfo *info)
{
    SDL_strlcpy(slot->name, name, sizeof(slot->name));
    if (!MIX_SetTrackAudio(slot->track, audio))
    {
        LOG_WARNING("Failed to set audio for sound '%s': %s", name, SDL_GetError());
        return false;
    }
    if (!MIX_SetTrackGain(slot->track, info->volume))
    {
        LOG_WARNING("Failed to set volume for sound '%s': %s", name, SDL_GetError());
        return false;
    }
    if (!MIX_SetTrackFrequencyRatio(slot->track, glm_clamp(info->pitch, SOUND_PITCH_MIN, SOUND_PITCH_MAX)))
    {
        LOG_WARNING("Failed to set pitch for sound '%s': %s", name, SDL_GetError());
        return false;
    }
    if (!SlotApplySpatial(slot, info))
    {
        LOG_WARNING("Failed to set spatialization for sound '%s'", name);
        return false;
    }
    if (!SlotApplyFilters(slot, info))
    {
        LOG_WARNING("Failed to set filters for sound '%s'", name);
        return false;
    }
    return true;
}

static bool PlayOptions(const SoundInfo *info, SDL_PropertiesID *options)
{
    *options = SDL_CreateProperties();

    if (!SDL_SetNumberProperty(*options, MIX_PROP_PLAY_LOOPS_NUMBER, info->looping ? -1 : 0))
    {
        LOG_ERROR("Failed to set sound loop properties: %s", SDL_GetError());
        SDL_DestroyProperties(*options);
        *options = 0;
        return false;
    }

    if (info->fade_in > 0.0f)
    {
        if (!SDL_SetNumberProperty(*options, MIX_PROP_PLAY_FADE_IN_MILLISECONDS_NUMBER,
                                   (Sint64)(info->fade_in * 1000.0f)))
        {
            LOG_ERROR("Failed to set sound fade-in properties: %s", SDL_GetError());
            SDL_DestroyProperties(*options);
            *options = 0;
            return false;
        }
    }

    return true;
}

static bool SlotStart(SoundSlot *slot, const SoundInfo *info)
{
    SDL_PropertiesID options = 0;
    if (!PlayOptions(info, &options))
        return false;

    bool ok = MIX_PlayTrack(slot->track, options);
    SDL_DestroyProperties(options);
    if (!ok)
    {
        LOG_WARNING("Failed to start sound track: %s", SDL_GetError());
        return false;
    }

    return true;
}

static void SlotFadeOut(SoundSlot *slot)
{
    if (!MIX_StopTrack(slot->track, MIX_TrackMSToFrames(slot->track, SOUND_STOP_FADE_MS)))
        LOG_WARNING("Failed to fade out sound track: %s", SDL_GetError());
}

static void SlotCut(SoundSlot *slot)
{
    if (!MIX_StopTrack(slot->track, 0))
        LOG_WARNING("Failed to stop sound track: %s", SDL_GetError());
}

static void SlotDestroy(SoundSlot *slot)
{
    if (!slot->track)
        return;
    SlotClearFilters(slot);
    MIX_DestroyTrack(slot->track);
    slot->track = NULL;
}

bool SoundPlay(const char *name, const SoundInfo *info)
{
    SoundInfo fallback = SoundInfoDefault();
    if (!info)
        info = &fallback;

    MIX_Audio *audio = FindAudio(name);
    if (!audio)
    {
        LOG_WARNING("Sound not found: %s", name);
        return false;
    }

    SoundSlot *slot = AcquireSlot();
    if (!slot)
    {
        LOG_WARNING("No free sound track available for '%s'", name);
        return false;
    }
    if (!SlotEnsureTrack(slot))
        return false;

    if (!SlotConfigure(slot, audio, name, info))
        return false;
    return SlotStart(slot, info);
}

void SoundStop(const char *name)
{
    ForEachNamed(name, SlotFadeOut);
}

void SoundBreak(const char *name)
{
    ForEachNamed(name, SlotCut);
}

bool SoundIsPlaying(const char *name)
{
    SoundManager *manager = Manager();
    for (int32 i = 0; i < SOUND_MAX_SLOTS; i++)
        if (SlotMatches(&manager->slots[i], name))
            return true;
    return false;
}

bool SoundIsPlayingAt(const char *name, float32 x, float32 y)
{
    SoundManager *manager = Manager();
    vec2s position = {.x = x, .y = y};
    for (int32 i = 0; i < SOUND_MAX_SLOTS; i++)
        if (SlotMatches(&manager->slots[i], name) && SamePosition(manager->slots[i].position, position))
            return true;
    return false;
}

void SetListenerPosition(float32 x, float32 y)
{
    Manager()->listener = (vec2s){.x = x, .y = y};
    ForEachSlot(SlotRefreshPosition);
}

void SoundShutdown(void)
{
    ForEachSlot(SlotDestroy);
}