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

#include "../block/blocks_data.h"
#include "../game/game_state.h"
#include "../graphics/render_entity.h"
#include "../network/client_interface.h"
#include "../network/server_local.h"
#include "../platform/gfx.h"
#include "entity.h"
#include "entity_monster.h"
#include "../world.h"


#define CREEPER_ACCEL         0.1f   // gelijk aan player‐accel
#define CREEPER_MAX_SPEED     0.23f  // blok/tick (20 tps → ~4.6 b/s)
#define CREEPER_WANDER_INTERVAL 40   // random dir elke 40 ticks = 2s
#define GRAVITY               0.08f  // vanilla gravity


static bool client_tick_creeper(struct entity* e) {
    // 0) bewaar oude pos voor interpolation
    glm_vec3_copy(e->pos, e->pos_old);

    // 1) kopieer server-positie in je render-pos
    glm_vec3_copy(e->network_pos, e->pos);

    // 2) zet pos op network_pos voor interpolation
    glm_vec3_copy(e->network_pos, e->pos);

    // 3) bepaal delta-beweging (XZ) en update yaw
    vec3 delta;
    glm_vec3_sub(e->pos, e->pos_old, delta);
    if (delta[0] != 0.0f || delta[2] != 0.0f) {
        e->orient[1] = atan2f(delta[0], delta[2]);
    }

    return false;
}

static bool server_tick_creeper(struct entity* e, struct server_local* s) {
    assert(e && s);

    glm_vec3_copy(e->pos,    e->pos_old);
    glm_vec2_copy(e->orient, e->orient_old);

    for (int i = 0; i < 3; i++)
        if (fabsf(e->vel[i]) < 0.005f)
            e->vel[i] = 0.0f;

    e->ai_timer -= 1;
    if (e->ai_timer <= 0) {
        float ang = rand_gen_flt(&s->rand_src) * 2.0f * (float)M_PI;
        float dx  = cosf(ang), dz = sinf(ang);
        e->vel[0] += 0.1f * dx;
        e->vel[2] += 0.1f * dz;
        float mag = sqrtf(e->vel[0]*e->vel[0] + e->vel[2]*e->vel[2]);
        const float MAX = 0.23f;
        if (mag > MAX) {
            float r = MAX / mag;
            e->vel[0] *= r;
            e->vel[2] *= r;
        }
        e->ai_timer = 40;
    }

    e->vel[1] -= 0.08f;

    e->on_ground = false;

    bool collision_xz = false;
    size_t axes[3] = {1, 0, 2};
    for (int k = 0; k < 3; k++) {
        int axis = axes[k];
        struct AABB bb;
        e->getBoundingBox(e, &bb);

        entity_try_move(
          e, e->pos, e->vel, &bb,
          axis,
          &collision_xz,
          &e->on_ground
        );

        if (axis == 1 && e->on_ground)
            e->vel[1] = 0.0f;
    }

    float slip = e->on_ground ? 0.6f : 1.0f;
    e->vel[0] *= slip * 0.91f;
    e->vel[2] *= slip * 0.91f;

    if (e->vel[0] != 0.0f || e->vel[2] != 0.0f)
        e->orient[1] = atan2f(e->vel[0], e->vel[2]) + M_PI;

    clin_rpc_send(&(struct client_rpc){
        .type = CRPC_ENTITY_MOVE,
        .payload.entity_move.entity_id = e->id,
        .payload.entity_move.pos = {e->pos[0], e->pos[1], e->pos[2]}
    });

    return false;
}
//todo: find_closest_player() zoekt lineair door actieve spelers.

	//todo: move_towards() rekent een eenvoudige velocity-vector uit (geen A*).

//    struct entity* player = find_closest_player(e, e->detection_range);
//    if (!player) {
//        e->ai_state = AI_IDLE;
//        return;
//    }
//
//    float dist = distance(e->pos, s->player.x);
//    if (e->ai_state == AI_IDLE) {
//        e->ai_state = AI_CHASE;
//    }
//    if (e->ai_state == AI_CHASE) {
//        // beweeg richting speler
//        move_towards(e, player->pos, /*speed=*/0.5f);
//        if (dist < 3.0f) {
//            e->ai_state = AI_FUSE;
//            e->ai_timer = 1.5f;  // 1.5s fuse
//        }
//    }
//    else if (e->ai_state == AI_FUSE) {
//        e->ai_timer -= delta_time(s);
//        if (e->ai_timer <= 0.0f) {
//            explode_creeper(e, s);
//        }
//        // tijdens fuse stil blijven staan
//    }
//}


static bool entity_server_tick(struct entity* e, struct server_local* s) {
	assert(e);
    switch (e->data.monster.id) {
        case 1:
            server_tick_creeper(e, s);
            break;
        default:
            break;
    }

	return false;
}


static bool entity_client_tick(struct entity* e) {
	assert(e);
    switch (e->data.monster.id) {
        case 1:
        	client_tick_creeper(e);
            break;
        default:
            break;
    }

	return false;
}

static void entity_render(struct entity* e, mat4 view, float tick_delta) {
		vec3 pos_lerp;
	    glm_vec3_lerp(e->pos_old, e->pos, tick_delta, pos_lerp);
	    struct block_data in_block;
	    entity_get_block(e,
	        floorf(pos_lerp[0]),
	        floorf(pos_lerp[1]),
	        floorf(pos_lerp[2]),
	        &in_block
	    );

	    render_entity_update_light(
	        (in_block.torch_light << 4) | (in_block.sky_light)
	    );

	    mat4 model, mv;
	    glm_translate_make(model, pos_lerp);
	    glm_mat4_mul(view, model, mv);

	    float yawDeg = glm_deg(e->orient[1]);
	    switch (e->data.monster.id) {
	        case 1:
	        	render_entity_creeper(mv, yawDeg);
	            break;
	        // todo: add more entities in the future
	        default:
	            render_entity_creeper(mv, yawDeg);
	            break;
	    }
	    // 6) (Optional) if you have a shadow routine, set up a small AABB and call it:
	    struct AABB bbox;
	    aabb_setsize_centered(&bbox, 0.25F, 0.25F, 0.25F);
	    aabb_translate(&bbox,
	        pos_lerp[0],
	        pos_lerp[1] - 0.04F,
	        pos_lerp[2]
	    );
	    entity_shadow(e, &bbox, view);
}


static size_t getBoundingBox(const struct entity *e, struct AABB *out) {
    assert(e && out);

    const float sx = 0.6f;
    const float sy = 1.7f;
    const float sz = 0.6f;
    aabb_setsize_centered_offset(out, sx, sy, sz, 0.0f, sy * 0.5f, 0.0f);
    aabb_translate(out,e->pos[0], e->pos[1],e->pos[2]);

    printf("[BB] pos=(%.2f,%.2f,%.2f) → x1=%.2f..%.2f y1=%.2f..%.2f z1=%.2f..%.2f\n",
           e->pos[0], e->pos[1], e->pos[2],
           out->x1, out->x2,
           out->y1, out->y2,
           out->z1, out->z2);

    return 1;
}

void entity_monster(uint32_t id, struct entity* e, bool server,
                    void* world, int monster_id) {
    assert(e && world);

    entity_default_init(e, server, world);

    e->name  = "Monster";
    e->id    = id;
    e->type  = ENTITY_MONSTER;
    e->data.monster.id = monster_id;
    e->tick_server = entity_server_tick;
    e->tick_client = entity_client_tick;
    e->render     = entity_render;
    e->on_ground   = true;
    e->vel[1]    = 0.0f;
    e->getBoundingBox = getBoundingBox;
    e->teleport    = entity_default_teleport;

    e->leftClickText = "Attack";
    e->onLeftClick = NULL;
    e->rightClickText = "Attack";
    e->onRightClick = NULL;

    e->ai_state        = AI_IDLE;
    e->ai_timer        = 0.0f;
    e->detection_range = 16.0f;


    switch (monster_id) {
        case 1:
            e->detection_range = 16.0f;
            break;
    }






}
