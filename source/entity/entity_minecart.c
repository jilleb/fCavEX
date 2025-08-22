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


#define MC_EPS 0.02f

static inline float tile_center(float v) {
    // Center of current tile
    return floorf(v) + 0.5f;
}

static inline int is_near_center_axis(float pos, int axis_is_x) {
    float c = axis_is_x ? tile_center(pos) : tile_center(pos);
    (void)c; // silence -O2 warning
    return fabsf((axis_is_x ? pos : pos) - (axis_is_x ? tile_center(pos) : tile_center(pos))) < MC_EPS;
}

static inline int is_near_center2(float x, float z) {
    return fabsf(x - tile_center(x)) < MC_EPS && fabsf(z - tile_center(z)) < MC_EPS;
}

// Turn table for curved rails (6..9) based on *effective* incoming dir (dx,dz).
// Meta meaning (MC Beta 1.7.3):
// 6: SE (connects South(+z) & East(+x))
// 7: SW (connects South(+z) & West(-x))
// 8: NW (connects North(-z) & West(-x))
// 9: NE (connects North(-z) & East(+x))
static inline void curve_turn(uint8_t meta, int in_dx, int in_dz, int *out_dx, int *out_dz) {
    *out_dx = in_dx; *out_dz = in_dz; // default: no change (safety)
    switch (meta) {
        case 6: // SE
            if (in_dz == -1) { *out_dx = +1; *out_dz = 0; }   // N -> E
            else if (in_dx == -1) { *out_dx = 0; *out_dz = +1; } // W -> S
            break;
        case 7: // SW
            if (in_dz == -1) { *out_dx = -1; *out_dz = 0; }   // N -> W
            else if (in_dx == +1) { *out_dx = 0; *out_dz = +1; } // E -> S
            break;
        case 8: // NW
            if (in_dz == +1) { *out_dx = -1; *out_dz = 0; }   // S -> W
            else if (in_dx == +1) { *out_dx = 0; *out_dz = -1; } // E -> N
            break;
        case 9: // NE
            if (in_dz == +1) { *out_dx = +1; *out_dz = 0; }   // S -> E
            else if (in_dx == -1) { *out_dx = 0; *out_dz = -1; } // W -> N
            break;
        default: break;
    }
}

// Seed initial heading (hx,hz) on a tile given metadata and current speed sign.
// Returns (hx,hz) with |hx|+|hz|==1. Straight/slopes pick axis; curves pick axis
// towards the tile center to avoid ambiguity when spawning on a curve.
static inline void seed_heading(uint8_t meta, float px, float pz, float spd, int8_t *hx, int8_t *hz) {
    int sgn = (spd >= 0.0f) ? +1 : -1;
    switch (meta) {
        case 0: case 4: case 5:  *hx = 0;     *hz = sgn; break; // NS (lock X)
        case 1: case 2: case 3:  *hx = sgn;   *hz = 0;   break; // EW (lock Z)
        case 6: case 7: case 8: case 9: {
            float cx = tile_center(px), cz = tile_center(pz);
            float dx = cx - px, dz = cz - pz;
            if (fabsf(dx) > fabsf(dz)) { *hx = (dx > 0 ? +1 : -1) * sgn; *hz = 0; }
            else { *hx = 0; *hz = (dz > 0 ? +1 : -1) * sgn; }
            break;
        }
        default: *hx = 0; *hz = (sgn != 0 ? sgn : +1); break;
    }
}



static bool minecart_server_tick(struct entity* e, struct server_local* s) {
    // Keep old pos for interpolation
    glm_vec3_copy(e->pos, e->pos_old);

    // Find rail under cart
    int bx = (int)floorf(e->pos[0]);
    int by = (int)floorf(e->pos[1] - 0.1f);
    int bz = (int)floorf(e->pos[2]);

    struct block_data below = world_get_block(&gstate.world, bx, by, bz);
    if (below.type != BLOCK_RAIL && below.type != BLOCK_POWERED_RAIL) {
        e->data.minecart.speed = 0.0f;
        return false;
    }

    uint8_t meta = below.metadata & 0xF;

    // Powered rail simple boost/limit (unchanged behavior)
    if (below.type == BLOCK_POWERED_RAIL) {
        if (e->data.minecart.speed < 0.01f) {
            e->data.minecart.speed = 0.05f;
        } else {
            e->data.minecart.speed += 0.01f;
            if (e->data.minecart.speed > 0.2f)
                e->data.minecart.speed = 0.2f;
        }
    }

    // Friction
    e->data.minecart.speed *= 0.97f;
    if (fabsf(e->data.minecart.speed) < 0.001f)
        e->data.minecart.speed = 0.0f;

    float spd = e->data.minecart.speed;
    if (spd == 0.0f) {
        // Still update stored metadata; nothing else to do.
        e->data.minecart.rail_meta = meta;
        e->data.minecart.last_meta = meta;
        return false;
    }

    // Ensure heading exists; use the persistent hx,hz in entity.h
    // hx,hz describe the cart's "forward" orientation; the *effective* move
    // direction per tick is (hx,hz) * sign(spd).
    if (e->data.minecart.hx == 0 && e->data.minecart.hz == 0) {
        seed_heading(meta, e->pos[0], e->pos[2], spd, &e->data.minecart.hx, &e->data.minecart.hz);
    }

    int sgn = (spd >= 0.0f) ? +1 : -1;
    int cur_dx = e->data.minecart.hx * sgn;
    int cur_dz = e->data.minecart.hz * sgn;

    // Compute tile center and clamp the perpendicular axis to keep us on rails.
    float cx = tile_center(e->pos[0]);
    float cz = tile_center(e->pos[2]);

    // Helper lambdas (C99 inline style)
    #define MOVE_ALONG_X(amount) do { e->pos[0] += (amount) * (float)cur_dx; } while (0)
    #define MOVE_ALONG_Z(amount) do { e->pos[2] += (amount) * (float)cur_dz; } while (0)

    // For straights/slopes we always keep the perpendicular axis at center.
    // For curves we also force the perpendicular axis to the center while we
    // approach the middle of the tile.
    switch (meta) {
        // --- Straight NS ---
        case 0:
            e->pos[0] = cx; // lock X
            MOVE_ALONG_Z(fabsf(spd));
            break;

        // --- Straight EW ---
        case 1:
            e->pos[2] = cz; // lock Z
            MOVE_ALONG_X(fabsf(spd));
            break;

        // --- Slopes EW (2: up to W, 3: up to E) ---
        case 2: // ascend West (-x when going up)
        case 3: { // ascend East (+x when going up)
            e->pos[2] = cz; // lock Z
            // Move on X
            MOVE_ALONG_X(fabsf(spd));
            // Simple Y ramp: go up if moving toward the ascending direction
            int up_dir = (meta == 2) ? -1 : +1;           // up to W or up to E
            int going  = cur_dx;                          // current x move dir
            float dy   = (going == up_dir ? +1.0f : -1.0f) * 0.5f * fabsf(spd);
            e->pos[1] += dy;
            break;
        }

        // --- Slopes NS (4: up to N, 5: up to S) ---
        case 4:
        case 5: {
            e->pos[0] = cx; // lock X
            // Move on Z
            MOVE_ALONG_Z(fabsf(spd));
            int up_dir = (meta == 4) ? -1 : +1;           // up to N or up to S
            int going  = cur_dz;                          // current z move dir
            float dy   = (going == up_dir ? +1.0f : -1.0f) * 0.5f * fabsf(spd);
            e->pos[1] += dy;
            break;
        }

        // --- Curves (center -> turn -> out) ---
        case 6: case 7: case 8: case 9: {
            // Force perpendicular axis to center while approaching middle
            if (cur_dx != 0) e->pos[2] = cz; else e->pos[0] = cx;

            // Distance to center along current axis
            float dist_to_center = (cur_dx != 0) ? (cx - e->pos[0]) * (float)cur_dx
                                                 : (cz - e->pos[2]) * (float)cur_dz;
            float step = fabsf(spd);

            if (dist_to_center > MC_EPS) {
                // We are not yet at the center: move toward it (but not past it)
                float move = (step <= dist_to_center) ? step : dist_to_center;
                if (cur_dx != 0) MOVE_ALONG_X(move); else MOVE_ALONG_Z(move);

                // If we exactly hit center and have leftover, handle below
                if (step <= dist_to_center + 1e-6f) break;
                step -= dist_to_center;
            } else {
                // Already at/near center
            }

            // At center now: turn according to curve, using *effective* in-dir
            int out_dx = cur_dx, out_dz = cur_dz;
            curve_turn(meta, cur_dx, cur_dz, &out_dx, &out_dz);

            // Apply leftover step along the outgoing axis
            if (out_dx != 0) { e->pos[2] = cz; e->pos[0] += step * (float)out_dx; }
            else              { e->pos[0] = cx; e->pos[2] += step * (float)out_dz; }

            // Update persistent forward heading (hx,hz) so that
            // hx*sgn == out_dx and hz*sgn == out_dz
            e->data.minecart.hx = (int8_t)(out_dx * sgn);
            e->data.minecart.hz = (int8_t)(out_dz * sgn);
            break;
        }

        default:
            // Unknown meta: do nothing but stop drift
            e->pos[0] = cx;
            e->pos[2] = cz;
            break;
    }

    // Save meta for debugging/consistency
    e->data.minecart.rail_meta = meta;
    e->data.minecart.last_meta = meta;

    // Keep rider glued on top of cart (unchanged behavior)
    if (e->data.minecart.occupied && e->data.minecart.occupant_id != 0) {
        struct entity* rider = *dict_entity_get(gstate.entities, e->data.minecart.occupant_id);
        if (rider) {
            glm_vec3_copy(e->pos, rider->pos);
            glm_vec3_copy(e->pos, rider->pos_old);
        }
    }
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
        0.5f,   // sx
        0.5f,   // sy
        0.5f,   // sz
        0.0f,   // ox
        0.25f,  // oy
        0.0f    // oz
    );
    aabb_translate(out, e->pos[0], e->pos[1], e->pos[2]);
    return 1;
}

static bool onRightClick(struct entity* e) {
    assert(e);

    struct entity* player = gstate.local_player;
    if (!e->data.minecart.occupied && player) {
        // Mount minecart: set as occupied and record occupant id
        e->data.minecart.occupied = true;
        e->data.minecart.occupant_id = player->id;
        player->data.local_player.riding_entity_id = e->id;
        e->rightClickText = "Dismount";
        return true;
    } else if (e->data.minecart.occupied && player && e->data.minecart.occupant_id == player->id) {
        // Dismount: clear occupant
        e->data.minecart.occupied = false;
        e->data.minecart.occupant_id = 0;
        player->data.local_player.riding_entity_id = 0;
        // Move player a bit up to avoid stuck in cart
        player->pos[1] += 1.2f;
        e->rightClickText = "Ride";
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

    e->data.minecart.hx = 0;
    e->data.minecart.hz = 0;
    e->data.minecart.last_meta = 0xFF;

    e->data.minecart.item.id         = ITEM_MINECART;
    e->data.minecart.item.durability = 0;
    e->data.minecart.item.count      = 1;
    e->data.minecart.speed = 0.0f;
    e->data.minecart.occupied = false;
    e->data.minecart.occupant_id = 0;

    entity_default_init(e, server, world);
}
