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


static enum block_material getMaterial(struct block_info* this) {
	return MATERIAL_WOOD;
}

static size_t getBoundingBox(struct block_info* this, bool entity,
							 struct AABB* x) {
	if(x) {
		aabb_setsize(x, 0.3125F, 0.625F, 0.3125F);

		switch(this->block->metadata) {
			case 1: aabb_translate(x, -0.34375F, 0.1875F, 0); break;
			case 2: aabb_translate(x, 0.34375F, 0.1875F, 0); break;
			case 3: aabb_translate(x, 0, 0.1875F, -0.34375F); break;
			case 4: aabb_translate(x, 0, 0.1875F, 0.34375F); break;
			default: aabb_setsize(x, 0.2F, 0.6F, 0.2F); break;
		}
	}

	return entity ? 0 : 1;
}

static struct face_occlusion*
getSideMask(struct block_info* this, enum side side, struct block_info* it) {
	return face_occlusion_empty();
}

static uint8_t getTextureIndex1(struct block_info* this, enum side side) {
	return tex_atlas_lookup(TEXAT_TORCH);
}

static uint8_t getTextureIndex2(struct block_info* this, enum side side) {
	return tex_atlas_lookup(TEXAT_REDSTONE_TORCH);
}

static uint8_t getTextureIndex3(struct block_info* this, enum side side) {
	return tex_atlas_lookup(TEXAT_REDSTONE_TORCH_LIT);
}

static bool onItemPlace(struct server_local* s, struct item_data* it,
						struct block_info* where, struct block_info* on,
						enum side on_side) {
	if(on_side == SIDE_BOTTOM || !blocks[on->block->type]
	   || blocks[on->block->type]->can_see_through)
		return false;

	int metadata = 0;
	switch(on_side) {
		case SIDE_LEFT: metadata = 2; break;
		case SIDE_RIGHT: metadata = 1; break;
		case SIDE_FRONT: metadata = 4; break;
		case SIDE_BACK: metadata = 3; break;
		default: metadata = 5; break;
	}

	server_world_set_block(s, where->x, where->y, where->z,
						   (struct block_data) {
							   .type = it->id,
							   .metadata = metadata,
							   .sky_light = 0,
							   .torch_light = 0,
						   });
    notifyNeighbours(s, where->x, where->y, where->z);

	return true;
}

static size_t drop_redstone_torch(struct block_info* this, struct item_data* it,
								  struct random_gen* g, struct server_local* s) {
	if(it) {
		it->id = BLOCK_REDSTONE_TORCH_LIT;
		it->durability = 0;
		it->count = 1;
	}

	return 1;
}

static void onWorldTick(struct server_local* s, struct block_info* blk) {
    if (rand_gen_flt(&gstate.rand_src) > (1.0f/3.0f)) return;

    // determine the position of the flame/smoke effect
    struct AABB box;
    blocks[blk->block->type]
      ->getBoundingBox(blk, false, &box);

    float spawnX, spawnZ;
    switch (blk->block->metadata) {
        case 1:
            spawnX = box.x2;
            spawnZ = 0.5f * (box.z1 + box.z2);
            break;
        case 2:
            spawnX = box.x1;
            spawnZ = 0.5f * (box.z1 + box.z2);
            break;
        case 3:
            spawnX = 0.5f * (box.x1 + box.x2);
            spawnZ = box.z2;
            break;
        case 4:
            spawnX = 0.5f * (box.x1 + box.x2);
            spawnZ = box.z1;
            break;
        default:
            spawnX = 0.5f * (box.x1 + box.x2);
            spawnZ = 0.5f * (box.z1 + box.z2);
    }

    vec3 pos = {
        blk->x + spawnX,
        blk->y + box.y2 + 0.02f,
        blk->z + spawnZ
    };

    // spawn appropriate effect based on torch type
    if (blk->block->type == 50) {
    	particle_generate_torch(pos);
    }
    else {
    	particle_generate_redstone_torch(pos);
    }

}

struct block block_torch = {
	.name = "Torch",
	.getSideMask = getSideMask,
	.getBoundingBox = getBoundingBox,
	.getMaterial = getMaterial,
	.getTextureIndex = getTextureIndex1,
	.getDroppedItem = block_drop_default,
	.onRandomTick = NULL,
	.onWorldTick = onWorldTick,
	.onRightClick = NULL,
	.transparent = false,
	.renderBlock = render_block_torch,
	.renderBlockAlways = NULL,
	.luminance = 14,
	.double_sided = false,
	.can_see_through = true,
	.opacity = 0,
	.ignore_lighting = false,
	.flammable = false,
	.place_ignore = false,
	.digging.hardness = 50,
	.digging.tool = TOOL_TYPE_ANY,
	.digging.min = TOOL_TIER_ANY,
	.digging.best = TOOL_TIER_ANY,
	.block_item = {
		.has_damage = false,
		.max_stack = 64,
		.renderItem = render_item_flat,
		.onItemPlace = onItemPlace,
		.fuel = 0,
		.armor.is_armor = false,
		.tool.type = TOOL_TYPE_ANY,
	},
};

struct block block_redstone_torch = {
	.name = "Redstone Torch",
	.getSideMask = getSideMask,
	.getBoundingBox = getBoundingBox,
	.getMaterial = getMaterial,
	.getTextureIndex = getTextureIndex2,
	.getDroppedItem = drop_redstone_torch,
	.onWorldTick = onWorldTick,
	.onRandomTick = NULL,
	.onRightClick = NULL,
	.transparent = false,
	.renderBlock = render_block_torch,
	.renderBlockAlways = NULL,
	.luminance = 7,
	.double_sided = true,
	.can_see_through = true,
	.opacity = 0,
	.ignore_lighting = false,
	.flammable = false,
	.place_ignore = false,
	.digging.hardness = 50,
	.digging.tool = TOOL_TYPE_ANY,
	.digging.min = TOOL_TIER_ANY,
	.digging.best = TOOL_TIER_ANY,
	.block_item = {
		.has_damage = false,
		.max_stack = 64,
		.renderItem = render_item_flat,
		.onItemPlace = onItemPlace,
		.fuel = 0,
		.armor.is_armor = false,
		.tool.type = TOOL_TYPE_ANY,
	},
};

struct block block_redstone_torch_lit = {
	.name = "Redstone Torch",
	.getSideMask = getSideMask,
	.getBoundingBox = getBoundingBox,
	.getMaterial = getMaterial,
	.getTextureIndex = getTextureIndex3,
	.getDroppedItem = drop_redstone_torch,
	.onRandomTick = NULL,
	.onRightClick = NULL,
	.onWorldTick = onWorldTick,
	.transparent = false,
	.renderBlock = render_block_torch,
	.renderBlockAlways = NULL,
	.luminance = 7,
	.double_sided = true,
	.can_see_through = true,
	.opacity = 0,
	.ignore_lighting = false,
	.flammable = false,
	.place_ignore = false,
	.digging.hardness = 50,
	.digging.tool = TOOL_TYPE_ANY,
	.digging.min = TOOL_TIER_ANY,
	.digging.best = TOOL_TIER_ANY,
	.block_item = {
		.has_damage = false,
		.max_stack = 64,
		.renderItem = render_item_flat,
		.onItemPlace = onItemPlace,
		.fuel = 0,
		.armor.is_armor = false,
		.tool.type = TOOL_TYPE_ANY,
	},
};
