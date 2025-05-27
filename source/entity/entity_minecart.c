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
#include "../network/client_interface.h"
#include "../network/server_local.h"
#include "entity.h"
#include "../graphics/gfx_settings.h"
#include "entity_minecart.h"
#include "../platform/gfx.h"
#include "../graphics/texture_atlas.h"
#include <math.h>

// ---- Server-side tick: simple rail following + gravity ----
static bool minecart_server_tick(struct entity* e, struct server_local* s) {
    // copy old pos
    glm_vec3_copy(e->pos, e->pos_old);

    // determine block below
    int bx = (int)floorf(e->pos[0]);
    int by = (int)floorf(e->pos[1] - 0.1f);
    int bz = (int)floorf(e->pos[2]);
    struct block_data below = world_get_block(&gstate.world, bx, by, bz);

    if (below.type == BLOCK_RAIL) {
        uint8_t meta = below.metadata & 0x7;
        int dx = 0, dz = 0;
        switch (meta) {
            case 0: dz =  1; break;
            case 1: dx =  1; break;
            case 2: dz = -1; break;
            case 3: dx = -1; break;
        }
        e->vel[0] += dx * 0.02f;
        e->vel[2] += dz * 0.02f;
        e->vel[1]  = 0.0f;
    } else {
        e->vel[1] -= 0.04f;
    }

    // friction
    e->vel[0] *= 0.98f;
    e->vel[1] *= 0.98f;
    e->vel[2] *= 0.98f;

    // apply movement
    glm_vec3_add(e->pos, e->vel, e->pos);
    return false; // never destroy
}

// ---- Client-side tick just mirrors server logic ----
static bool minecart_client_tick(struct entity* e) {
    return minecart_server_tick(e, NULL);
}

// ---- Render using existing item_render API ----
static void minecart_render(struct entity* e, mat4 view, float tick_delta) {
    // interpolate position
    vec3 pos_lerp;
    glm_vec3_lerp(e->pos_old, e->pos, tick_delta, pos_lerp);

    // update lighting from block below
    struct block_data in_block;
    entity_get_block(e,
        (int)floorf(pos_lerp[0]),
        (int)floorf(pos_lerp[1]),
        (int)floorf(pos_lerp[2]),
        &in_block);
    render_item_update_light((in_block.torch_light << 4) | in_block.sky_light);

    // build model matrix: position, scale, center
    mat4 model;
    glm_translate_make(model, pos_lerp);
    glm_scale(model, (vec3){1.0f, 0.5f, 1.0f});
    glm_translate(model, (vec3){-0.5f, -0.25f, -0.5f});

    // compose view*model
    mat4 mv;
    glm_mat4_mul(view, model, mv);

    // correct item_get call
    struct item* it = item_get(&e->data.minecart.item);
    it->renderItem(it, &e->data.minecart.item, mv, false, R_ITEM_ENV_ENTITY);

    // draw shadow
    struct AABB bbox;
    aabb_setsize_centered(&bbox, 0.5f, 0.25f, 0.5f);
    aabb_translate(&bbox, pos_lerp[0], pos_lerp[1] - 0.04f, pos_lerp[2]);
    entity_shadow(e, &bbox, view);
}

// ---- Factory: initialize entity fields, including item_data ----
void entity_minecart(uint32_t id, struct entity* e, bool server, void* world) {
    e->id          = id;
    e->tick_server = minecart_server_tick;
    e->tick_client = minecart_client_tick;
    e->render      = minecart_render;
    e->teleport    = entity_default_teleport;
    e->type        = ENTITY_MINECART;
    e->on_ground   = true;

    // zero vectors
    glm_vec3_zero(e->pos);
    glm_vec3_zero(e->pos_old);
    glm_vec3_zero(e->vel);
    glm_vec2_zero(e->orient);
    glm_vec2_zero(e->orient_old);

    // setup item_data for render
    e->data.minecart.item.id         = ITEM_MINECART;
    e->data.minecart.item.durability = 0;
    e->data.minecart.item.count      = 1;

    // default initialization (world registration, networking)
    entity_default_init(e, server, world);
}
