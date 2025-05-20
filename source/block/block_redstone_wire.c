/*
	Copyright (c) 2022 ByteBit/xtreme8000

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

#include "../network/server_local.h"
#include "blocks.h"
#include "../particle.h"
#include "../game/game_state.h"

typedef struct {
    bool    present;   // true if neighbor is a redstone wire
    uint8_t power;     // its metadata (0–15)
} NeighborInfo;

static enum block_material getMaterial(struct block_info* this) {
	return MATERIAL_STONE;
}

static size_t getBoundingBox(struct block_info* this, bool entity,
                             struct AABB* x) {
    if (x) {
        aabb_setsize(x,
                     1.0F,
                     0.125F,
                     1.0F);
    }
    return entity ? 0 : 1;
}

static struct face_occlusion*
getSideMask(struct block_info* this, enum side side, struct block_info* it) {
	return face_occlusion_empty();
}

static uint8_t getTextureIndex(struct block_info* this, enum side side) {
	uint8_t lvl = this->block->metadata & 0x0F;
	if (lvl == 0) {
		return tex_atlas_lookup(TEXAT_REDSTONE_WIRE_OFF);
	}
	return tex_atlas_lookup(TEXAT_REDSTONE_WIRE_L1 + (lvl - 1));
}

// Check the four horizontal neighbors for wire power levels
static void getAdjacentWirePower(struct server_local* s,
                                 int x, int y, int z,
                                 NeighborInfo neighbors[4])
{
    static const int dx[4] = { 1, -1,  0,  0 };
    static const int dz[4] = { 0,  0,  1, -1 };
    struct block_data bd;
    for (int i = 0; i < 4; ++i) {
        int nx = x + dx[i], ny = y, nz = z + dz[i];
        bool ok = server_world_get_block(&s->world, nx, ny, nz, &bd);
        if (ok && bd.type == BLOCK_REDSTONE_WIRE) {
            neighbors[i].present = true;
            neighbors[i].power   = bd.metadata & 0x0F;
        } else {
            neighbors[i].present = false;
            neighbors[i].power   = 0;
        }
    }
}

// Check six directions for "strong" power sources (e.g. torch and lever, which is still todo)
static uint8_t getStrongPower(struct server_local* s,
                              int x, int y, int z)
{
    struct block_data bd;

    // check in all 6 directions
    static const int dx6[6] = { 1, -1,  0,  0,  0,  0 };
    static const int dy6[6] = { 0,  0,  1, -1,  0,  0 };
    static const int dz6[6] = { 0,  0,  0,  0,  1, -1 };
    for (int i = 0; i < 6; ++i) {
        int nx = x + dx6[i], ny = y + dy6[i], nz = z + dz6[i];
        if (!server_world_get_block(&s->world, nx, ny, nz, &bd))
            continue;
        if (bd.type == 76 )// || (bd.type == BLOCK_LEVER && (bd.metadata & 0x01)))
        {
            return 15;
        }
    }

    // Extra: torches in y+1 horizontal neighbours
    static const int dx4[4] = {  1, -1,  0,  0 };
    static const int dz4[4] = {  0,  0,  1, -1 };
    for (int i = 0; i < 4; ++i) {
        int nx = x + dx4[i];
        int ny = y + 1;
        int nz = z + dz4[i];
        if (server_world_get_block(&s->world, nx, ny, nz, &bd) &&
            bd.type == 76)
        {
            return 15;
        }
    }

    return 0;
}


// World‐tick handler: propagate power from strong sources and neighbors
static void onWorldTick(struct server_local* s, struct block_info* blk)
{
    int x = blk->x;
    int y = blk->y;
    int z = blk->z;


    struct block_data current;
    if (!server_world_get_block(&s->world, x, y, z, &current)
        || current.type != BLOCK_REDSTONE_WIRE)
    {
        return;
    }

    // Check for strong power (level 15)
    uint8_t strong = getStrongPower(s, x, y, z);

    // Check neighboring wires
    NeighborInfo neighbors[4];
    getAdjacentWirePower(s, x, y, z, neighbors);
    uint8_t max_neighbor = 0;
    for (int i = 0; i < 4; ++i) {
        if (neighbors[i].present && neighbors[i].power > max_neighbor) {
            max_neighbor = neighbors[i].power;
        }
    }

    // Determine desired level
    uint8_t desired;
    if (strong > 0) {
        desired = 15;
    } else if (max_neighbor > 0) {
        desired = max_neighbor - 1;
    } else {
        desired = 0;
    }

   vec3 center = { blk->x + 0.5f,
                        blk->y,
                        blk->z + 0.5f };
   particle_generate_redstone_wire(center, desired);


    if ((current.metadata & 0x0F) != desired) {
    	server_world_set_block(&s->world,
                               x, y, z,
                               (struct block_data){
                                 .type     = BLOCK_REDSTONE_WIRE,
                                 .metadata = desired
                               });
    }

}

static size_t getDroppedItem(struct block_info* this,
                             struct item_data* it,
                             struct random_gen* g,
                             struct server_local* s)
{
    if (it) {
        it->id = ITEM_REDSTONE;
        it->durability = 0;
        it->count = 1;
    }
    return 1;
}

struct block block_redstone_wire = {
	.name = "Redstone wire",
	.getSideMask = getSideMask,
	.getBoundingBox = getBoundingBox,
	.getMaterial = getMaterial,
	.getTextureIndex = getTextureIndex,
	.getDroppedItem = getDroppedItem,
	.onRandomTick = NULL,
	.onRightClick = NULL,
	.onWorldTick = onWorldTick,
	.transparent = false,
	.renderBlock = render_block_redstone_wire,
	.renderBlockAlways = NULL,
	.luminance = 0,
	.double_sided = true,
	.can_see_through = true,
	.opacity = 0,
	.render_block_data.rail_curved_possible = true,
	.ignore_lighting = false,
	.flammable = false,
	.place_ignore = false,
	.digging.hardness = 1050,
	.digging.tool = TOOL_TYPE_ANY,
	.digging.min = TOOL_TIER_ANY,
	.digging.best = TOOL_TIER_ANY,
	.block_item = {
		.has_damage = false,
		.max_stack = 64,
		.renderItem = render_item_flat,
		.onItemPlace = block_place_default,
		.armor.is_armor = false,
		.tool.type = TOOL_TYPE_ANY,
	},
};
