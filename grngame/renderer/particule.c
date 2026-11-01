#include "particule.h"

#include "SDL3/SDL_stdinc.h"
#include "grngame/core/app.h"
#include "grngame/math/math.h"
#include "grngame/utils/attributes.h"
#include "grngame/utils/clear.h"
#include "grngame/utils/random.h"
#include "kvec.h"
#include <math.h>

static bool IsAlive(const Particle *p)
{
    return p->lifetime > 0.0f && p->age < p->lifetime;
}

ParticleEmitter ParticleEmitterCreate(int32 capacity)
{
    ParticleEmitter emit = {0};

    if (capacity <= 0)
        capacity = 128;
    emit.capacity = capacity;

    emit.particles = malloc((uint32)capacity * sizeof(Particle));
    CLEAR_ARRAY(emit.particles, 0, capacity);

    emit.lifetime = 1.0f;
    emit.direction = 0.0f;
    emit.spread = 2.0f * (float32)pi;
    emit.initial_velocity = 60.0f;
    emit.damping = 1.0f;
    emit.scale_start = 4.0f;
    emit.scale_end = 1.0f;
    emit.scale_amount_curve = 1.0f;

    emit.color_start = 0;
    emit.color_end = 0;
    emit.alpha_start = 0;
    emit.alpha_end = 0;

    return emit;
}

void ParticleEmitterDestroy(ParticleEmitter *emit)
{
    if (!emit)
        return;

    free(emit->particles);
    emit->particles = NULL;
    emit->capacity = 0;
    emit->alive_count = 0;
}

static void SpawnParticle(ParticleEmitter *emit)
{
    if (emit->alive_count >= emit->capacity)
        return;

    for (int32 i = 0; i < emit->capacity; i++)
    {
        Particle *p = &emit->particles[i];
        if (IsAlive(p))
            continue;

        float32 off_x = 0.0f;
        float32 off_y = 0.0f;

        if (emit->spawn_radius > 0.0f)
        {
            float32 r = emit->spawn_radius * sqrtf(Random(0.0f, 1.0f));
            float32 theta = Random(0.0f, 2.0f * (float32)SDL_PI_F);
            off_x = cosf(theta) * r;
            off_y = sinf(theta) * r;
        }
        else if (emit->spawn_rect_w > 0.0f || emit->spawn_rect_h > 0.0f)
        {
            off_x = Random(-emit->spawn_rect_w * 0.5f, emit->spawn_rect_w * 0.5f);
            off_y = Random(-emit->spawn_rect_h * 0.5f, emit->spawn_rect_h * 0.5f);
        }

        p->position.x = emit->position.x + off_x;
        p->position.y = emit->position.y + off_y;

        float32 half_spread = emit->spread * 0.5f;
        float32 angle = emit->direction + Random(-half_spread, half_spread);
        float32 speed = emit->initial_velocity + Random(-emit->velocity_variation, emit->velocity_variation);
        if (speed < 0.0f)
            speed = 0.0f;

        p->velocity.x = cosf(angle) * speed;
        p->velocity.y = sinf(angle) * speed;

        p->lifetime = emit->lifetime + Random(-emit->lifetime_variation, emit->lifetime_variation);
        if (p->lifetime <= 0.05f)
            p->lifetime = 0.05f;
        p->age = 0.0f;

        p->size = emit->scale_start;
        p->seed = Random(0.0f, 100.0f);

        p->current_color_idx = emit->color_start;
        p->current_alpha_idx = emit->alpha_start;

        emit->alive_count++;
        return;
    }
}

static void UpdateParticle(ParticleEmitter *restrict emit, Particle *restrict p, float32 dt)
{
    if (!IsAlive(p))
        return;

    p->age += dt;

    if (p->age >= p->lifetime)
    {
        p->lifetime = 0.0f;
        emit->alive_count--;
        return;
    }
    if (emit->turbulence > 0.0f)
    {
        float32 wave = sinf(p->age * emit->turbulence_freq + p->seed);
        p->velocity.x += wave * emit->turbulence * dt;
    }

    p->velocity.y += emit->gravity * dt;
    p->velocity.x *= emit->damping;
    p->velocity.y *= emit->damping;
    p->position.x += p->velocity.x * dt;
    p->position.y += p->velocity.y * dt;

    float32 t = p->age / p->lifetime;
    float32 curve = emit->scale_amount_curve;
    float32 shaped_t = curve > 0.0f ? powf(t, curve) : t;
    p->size = Math_Lerp(emit->scale_start, emit->scale_end, shaped_t);
    if (p->size < 1.0f)
        p->size = 1.0f;

    p->current_color_idx = Math_LerpInt(emit->color_start, emit->color_end, t);
    p->current_alpha_idx = Math_LerpInt(emit->alpha_start, emit->alpha_end, shaped_t);
}

void UpdateEmitter(ParticleEmitter *emit, float32 dt)
{
    if (emit->capacity <= 0 || !emit->particles || dt <= 0.0f)
        return;

    if (emit->local_coords)
    {
        vec2s delta = {{emit->position.x - emit->previous_position.x, emit->position.y - emit->previous_position.y}};
        if (delta.x != 0.0f || delta.y != 0.0f)
        {
            for (int32 i = 0; i < emit->capacity; i++)
            {
                Particle *p = &emit->particles[i];
                if (IsAlive(p))
                {
                    p->position.x += delta.x;
                    p->position.y += delta.y;
                }
            }
        }
    }
    emit->previous_position = emit->position;

    int32 spawn_count = 0;
    float32 life = emit->lifetime > 0.0001f ? emit->lifetime : 0.0001f;
    float32 explosiveness = SDL_clamp(emit->explosiveness, 0.0f, 1.0f);

    if (emit->one_shot)
    {
        if (!emit->one_shot_done)
        {
            spawn_count = emit->capacity;
            emit->one_shot_done = true;
        }
    }
    else
    {
        float32 stream_rate = ((float32)emit->capacity / life) * (1.0f - explosiveness);
        emit->emit_accumulator += stream_rate * dt;
        spawn_count += (int32)emit->emit_accumulator;
        emit->emit_accumulator -= (float32)((int32)emit->emit_accumulator);

        emit->emit_cycle_time += dt;
        int32 burst_size = (int32)((float32)emit->capacity * explosiveness);
        while (emit->emit_cycle_time >= life)
        {
            emit->emit_cycle_time -= life;
            spawn_count += burst_size;
        }
    }

    int32 available = emit->capacity - emit->alive_count;
    if (spawn_count > available)
        spawn_count = available;

    for (int32 i = 0; i < spawn_count; i++)
        SpawnParticle(emit);

    for (int32 i = 0; i < emit->capacity; i++)
        UpdateParticle(emit, &emit->particles[i], dt);
}

void RenderEmitter(ParticleEmitter *emit)
{
    if (emit->capacity <= 0 || !emit->particles || emit->alive_count == 0)
        return;

    SDL_Renderer *renderer = g_app.renderer.renderer;

    int32 palette_size = (int32)kv_size(g_app.palette_manager.palette_elements);

    SDL_Vertex *verts = STACK_ALLOC(SDL_Vertex, emit->alive_count * 4);
    int32 *indices = STACK_ALLOC(int32, emit->alive_count * 6);
    if (UNLIKELY(!verts || !indices))
        return;

    int32 vcount = 0;
    int32 icount = 0;
    const int32 base_indices[6] = {0, 1, 2, 2, 3, 0};

    for (int32 i = 0; i < emit->capacity; i++)
    {
        Particle *p = &emit->particles[i];
        if (!IsAlive(p))
            continue;

        float32 x = p->position.x + g_app.info.offset_x;
        float32 y = p->position.y + g_app.info.offset_y;
        float32 w = p->size;
        float32 h = p->size;

        SDL_Color base_color = {0, 0, 0, 255};
        if (LIKELY(palette_size > 0))
        {
            int32 safe_idx = CLAMP(p->current_color_idx, 0, palette_size - 1);
            base_color = kv_A(g_app.palette_manager.palette_elements, safe_idx);
        }

        uint8 alpha_val = p->current_alpha_idx;
        SDL_FColor color = {base_color.r / 255.0f, base_color.g / 255.0f, base_color.b / 255.0f, alpha_val / 255.0f};

        int32 base = vcount;

        verts[vcount++] =
            (SDL_Vertex){.position = {PIXEL_ALIGN(x), PIXEL_ALIGN(y)}, .color = color, .tex_coord = {0, 0}};
        verts[vcount++] =
            (SDL_Vertex){.position = {PIXEL_ALIGN(x + w), PIXEL_ALIGN(y)}, .color = color, .tex_coord = {0, 0}};
        verts[vcount++] =
            (SDL_Vertex){.position = {PIXEL_ALIGN(x + w), PIXEL_ALIGN(y + h)}, .color = color, .tex_coord = {0, 0}};
        verts[vcount++] =
            (SDL_Vertex){.position = {PIXEL_ALIGN(x), PIXEL_ALIGN(y + h)}, .color = color, .tex_coord = {0, 0}};

        for (int32 j = 0; j < 6; j++)
            indices[icount++] = base + base_indices[j];
    }

    if (LIKELY(vcount > 0 && icount > 0))
    {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderGeometry(renderer, NULL, verts, vcount, indices, icount);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}
