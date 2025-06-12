/*
	Copyright (c) 2023 ByteBit/xtreme8000

	This file is part of CavEX.

	CavEX is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	CavEX is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with CavEX.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <math.h>
#include "entity.h"
#include "entity_monster.h"
#include "../block/blocks_data.h"
#include "../network/server_local.h"
#include "../network/client_interface.h"
#include "../world.h"
#include "../graphics/render_entity.h"
#include "../platform/gfx.h"

// Creeper movement constants
#define CREEPER_ACCEL            2.0f    // acceleration per tick
#define CREEPER_MAX_SPEED        2.0f    // max horizontal speed per tick
#define CREEPER_WANDER_INTERVAL  4      // ticks between direction changes
#define GRAVITY                  0.08f   // gravity per tick
#define UNSTUCK_MOVE             0.01f   // slight upward nudge if stuck
#define YAW_SMOOTH_FACTOR        0.2f    // smoothing factor [0..1]

// Local bounding box for collision (centered, half-height offset)
static void make_creeper_bbox(struct AABB* b) {
    const float sx = 0.6f, sy = 1.7f, sz = 0.6f;
    aabb_setsize_centered_offset(b, sx, sy, sz,
                                 0.0f, sy * 0.5f, 0.0f);
}

// Client-side tick: interpolate and smooth orientation
static bool client_tick_creeper(struct entity* e) {
    assert(e);
    // Position interpolation
    glm_vec3_copy(e->pos, e->pos_old);
    glm_vec3_copy(e->network_pos, e->pos);

    // Compute movement delta
    vec3 delta;
    glm_vec3_sub(e->pos, e->pos_old, delta);
    if (delta[0] != 0.0f || delta[2] != 0.0f) {
        float targetYaw = atan2f(delta[0], delta[2]);
        // smooth between old and target yaw
        e->orient[1] = e->orient_old[1] * (1.0f - YAW_SMOOTH_FACTOR)
                      + targetYaw * YAW_SMOOTH_FACTOR;
    }
    // No pitch for creeper
    e->orient[0] = 0.0f;

    // Copy for next frame
    glm_vec2_copy(e->orient, e->orient_old);
    return false;
}

// Server-side creeper logic
static bool server_tick_creeper(struct entity* e, struct server_local* s) {
    assert(e && s);
    // Save old for interpolation
    glm_vec3_copy(e->pos,    e->pos_old);
    glm_vec2_copy(e->orient, e->orient_old);
    // Damp tiny velocities
    for (int i = 0; i < 3; i++) if (fabsf(e->vel[i]) < 0.005f) e->vel[i] = 0.0f;

    // Random wander
    if (--e->ai_timer <= 0) {
        float ang = rand_gen_flt(&s->rand_src) * 2.0f * M_PI;
        e->vel[0] += CREEPER_ACCEL * cosf(ang);
        e->vel[2] += CREEPER_ACCEL * sinf(ang);
        float mag = sqrtf(e->vel[0]*e->vel[0] + e->vel[2]*e->vel[2]);
        if (mag > CREEPER_MAX_SPEED) {
            float scale = CREEPER_MAX_SPEED / mag;
            e->vel[0] *= scale;
            e->vel[2] *= scale;
        }
        e->ai_timer = CREEPER_WANDER_INTERVAL;
    }

    // Gravity
    e->vel[1] -= GRAVITY;
    e->on_ground = false;

    // Unstuck if embedded
    struct AABB b0, b1;
    make_creeper_bbox(&b0);
    b1 = b0;
    aabb_translate(&b1, 0.0f, UNSTUCK_MOVE, 0.0f);
    if (entity_aabb_intersection(e, &b0) && !entity_aabb_intersection(e, &b1)) {
        e->pos[1] += UNSTUCK_MOVE;
    }

    // Collision sweep: Y, X, Z
    bool collision_xz = false;
    size_t axes[3] = {1, 0, 2};
    for (int i = 0; i < 3; i++) {
        struct AABB bb;
        make_creeper_bbox(&bb);
        entity_try_move(e,
                        e->pos, e->vel,
                        &bb,
                        axes[i],
                        &collision_xz,
                        &e->on_ground);
        if (axes[i] == 1 && e->on_ground) e->vel[1] = 0.0f;
    }

    // Friction
    float slip = e->on_ground ? 0.6f : 1.0f;
    e->vel[0] *= slip * 0.91f;
    e->vel[2] *= slip * 0.91f;

    // Update orientation to face motion
    if (e->vel[0] != 0.0f || e->vel[2] != 0.0f) {
        e->orient[1] = atan2f(e->vel[0], e->vel[2]) + M_PI;
    }
    e->orient[0] = 0.0f;

    // Broadcast move
    clin_rpc_send(&(struct client_rpc){
        .type = CRPC_ENTITY_MOVE,
        .payload.entity_move.entity_id = e->id,
        .payload.entity_move.pos = {e->pos[0], e->pos[1], e->pos[2]}
    });
    return false;
}

static bool entity_server_tick(struct entity* e, struct server_local* s) {
    return (e->data.monster.id == MONSTER_CREEPER)
        ? server_tick_creeper(e, s)
        : false;
}
static bool entity_client_tick(struct entity* e) {
    return (e->data.monster.id == MONSTER_CREEPER)
        ? client_tick_creeper(e)
        : false;
}

// Render creeper
static void entity_render(struct entity* e, mat4 view, float td) {
    vec3 p;
    glm_vec3_lerp(e->pos_old, e->pos, td, p);
    struct block_data blk;
    entity_get_block(e,
        floorf(p[0]), floorf(p[1]), floorf(p[2]), &blk);
    render_entity_update_light((blk.torch_light << 4) | blk.sky_light);
    mat4 model, mv;
    glm_translate_make(model, p);
    glm_mat4_mul(view, model, mv);
    render_entity_creeper(mv, glm_deg(e->orient[1]));
    struct AABB shadow_bb;
    aabb_setsize_centered(&shadow_bb, 0.25f, 0.25f, 0.25f);
    aabb_translate(&shadow_bb, p[0], p[1] - 0.04f, p[2]);
    entity_shadow(e, &shadow_bb, view);
}

// Raycast & interaction bounding box (world-translated)
static size_t getBoundingBox_creeper(const struct entity* e, struct AABB* out) {
    assert(e && out);
    make_creeper_bbox(out);
    aabb_translate(out, e->pos[0], e->pos[1], e->pos[2]);
    return 1;
}

// Factory
void entity_monster(uint32_t id,
                    struct entity* e,
                    bool server,
                    void* world,
                    int monster_id) {
    assert(e && world);
    entity_default_init(e, server, world);
    e->id = id;
    e->type = ENTITY_MONSTER;
    e->name = "Monster";
    e->data.monster.id = monster_id;
    e->tick_server = entity_server_tick;
    e->tick_client = entity_client_tick;
    e->render      = entity_render;
    e->teleport    = entity_default_teleport;
    e->getBoundingBox = getBoundingBox_creeper;
    e->onLeftClick = NULL; // todo: attack
    e->leftClickText = "Attack";
    e->onRightClick = NULL;
    e->rightClickText = NULL;

    e->ai_timer     = CREEPER_WANDER_INTERVAL;
}
