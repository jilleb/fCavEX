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
#include "../platform/texture.h"
#include <math.h>
#include "../platform/displaylist.h"
#include "../graphics/render_block.h"
#include "../graphics/render_entity.h"
#include "../network/client_interface.h"
#include "../network/server_local.h"
#include "../platform/gfx.h"
#include "../item/items.h"


static bool minecart_server_tick(struct entity* e, struct server_local* s) {
    glm_vec3_copy(e->pos, e->pos_old);

    int bx = (int)floorf(e->pos[0]);
    int by = (int)floorf(e->pos[1] - 0.1f);
    int bz = (int)floorf(e->pos[2]);

    struct block_data below = world_get_block(&gstate.world, bx, by, bz);

    if (below.type != BLOCK_RAIL && below.type != BLOCK_POWERED_RAIL) {
        e->data.minecart.speed = 0.0f;
        return false;
    }

    uint8_t meta = below.metadata & 0xF;
    int dx = 0, dy = 0, dz = 0;

    switch (meta) {
        case 0:  dz =  1; break;            // NS
        case 1:  dx =  1; break;            // EW
        case 2:  dx = -1; dy = 1; break;    // slope W
        case 3:  dx =  1; dy = 1; break;    // slope E
        case 4:  dz = -1; dy = 1; break;    // slope N
        case 5:  dz =  1; dy = 1; break;    // slope S
        case 6:  dx =  1; dz =  1; break;   // SE
        case 7:  dx = -1; dz =  1; break;   // SW
        case 8:  dx = -1; dz = -1; break;   // NW
        case 9:  dx =  1; dz = -1; break;   // NE
        default: break;
    }

    bool powered = false;
    if (below.type == BLOCK_POWERED_RAIL) {
        powered = true;
    }

    if (powered) {
        if (e->data.minecart.speed < 0.01f) {
            e->data.minecart.speed = 0.05f;
        } else {
            e->data.minecart.speed += 0.01f;
            if (e->data.minecart.speed > 0.2f)
                e->data.minecart.speed = 0.2f;
        }
    }

    float spd = e->data.minecart.speed;
    e->pos[0] += dx * spd;
    e->pos[1] += dy * spd;
    e->pos[2] += dz * spd;

    e->data.minecart.rail_meta = meta;

    return false;
}

static bool minecart_client_tick(struct entity* e) {
    return minecart_server_tick(e, NULL);
}

static void entity_minecart_render(struct entity* e, mat4 view, float tick_delta) {
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

    struct block_data rail_block;
    int bx = (int)floorf(pos_lerp[0]);
    int by = (int)floorf(pos_lerp[1] - 0.1f);
    int bz = (int)floorf(pos_lerp[2]);
    entity_get_block(e, bx, by, bz, &rail_block);

    float yaw_deg = 0.0f;
    float pitch_deg = 0.0f;
    float y_offset = 0.0f;
    if (rail_block.type == BLOCK_RAIL) {
        uint8_t meta = rail_block.metadata & 0xF;

        switch (meta) {
            case 0:  yaw_deg =   0.0f; break;   // NS
            case 1:  yaw_deg =  90.0f; break;   // EW
            case 2:  yaw_deg =  90.0f; pitch_deg =  45.0f; y_offset = 0.5f; break; // slope W
            case 3:  yaw_deg =  90.0f; pitch_deg = -45.0f; y_offset = 0.5f; break; // slope E
            case 4:  yaw_deg =   0.0f; pitch_deg =  45.0f; y_offset = 0.5f; break; // slope S
            case 5:  yaw_deg =   0.0f; pitch_deg = -45.0f; y_offset = 0.5f; break; // slope N
            case 6:  yaw_deg =  135.0f; break; // SE curve
            case 7:  yaw_deg = -135.0f; break; // SW curve
            case 8:  yaw_deg =  -45.0f; break; // NW curve
            case 9:  yaw_deg =   45.0f; break; // NE curve
            default: break;
        }
    }

    mat4 model, mv;
    glm_translate_make(model, (vec3){
        pos_lerp[0],
        pos_lerp[1] + y_offset,
        pos_lerp[2]
    });

    glm_rotate_y(model, glm_rad(-yaw_deg), model);
    glm_rotate_x(model, glm_rad(pitch_deg), model);

    glm_mat4_mul(view, model, mv);

    render_entity_minecart(mv);

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
    aabb_setsize_centered_offset(
        out,
        1.0f,    // sx
        0.625f,  // sy
        1.25f,   // sz
        0.0f,    // ox
        0.3125f,    // oy
        0.1f    // oz
    );
    aabb_translate(out, e->pos[0], e->pos[1], e->pos[2]);
    return 1;
}

static bool onRightClick(struct entity* e) {
    assert(e);

    if (!e->data.minecart.occupied) {
        // TODO: add user as "rider"
        e->data.minecart.occupied = true;
        return true;
    }

    return false;
}

static bool onLeftClick(struct entity *e) {
    assert(e);
    // TODO: add breakdown logic.
    return true;
}

void entity_minecart(uint32_t id, struct entity* e, bool server, void* world) {
    e->name 	   = "Minecart";
    e->id          = id;
    e->tick_server = minecart_server_tick;
    e->tick_client = minecart_client_tick;
    e->render      = entity_minecart_render;
    e->teleport    = entity_default_teleport;
    e->type        = ENTITY_MINECART;
    e->on_ground   = true;
    e->getBoundingBox = getBoundingBox;
    e->leftClickText = NULL;
    e->onLeftClick   = onLeftClick;
    e->rightClickText = e->data.minecart.occupied ? "Dismount" : "Ride";
    e->onRightClick   = onRightClick;

    // zero vectors
    glm_vec3_zero(e->pos);
    glm_vec3_zero(e->pos_old);
    glm_vec3_zero(e->vel);
    glm_vec2_zero(e->orient);
    glm_vec2_zero(e->orient_old);

    e->data.minecart.item.id         = ITEM_MINECART;
    e->data.minecart.item.durability = 0;
    e->data.minecart.item.count      = 1;
    e->data.minecart.speed = 0.0f;


    entity_default_init(e, server, world);
}
