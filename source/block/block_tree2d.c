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

static enum block_material getMaterial(struct block_info* this) {
	return MATERIAL_WOOD;
}

static size_t getBoundingBox(struct block_info* this, bool entity,
							 struct AABB* x) {
	if(x)
		aabb_setsize(x, 1.0F, 2.0F, 1.0F);
	return 1;
}

static struct face_occlusion*
getSideMask(struct block_info* this, enum side side, struct block_info* it) {
	return face_occlusion_empty();
}

static uint8_t getTextureIndex(struct block_info* this, enum side side) {
	switch(this->block->metadata >> 1) {
		case 0: return tex_atlas_lookup(TEXAT_SAPLING_OAK);
		case 1: return tex_atlas_lookup(TEXAT_SAPLING_SPRUCE);
		case 2: return tex_atlas_lookup(TEXAT_SAPLING_BIRCH);
		default: return tex_atlas_lookup(TEXAT_SAPLING_OAK);
	}
}

static size_t getDroppedItem(struct block_info* this, struct item_data* it,
							 struct random_gen* g, struct server_local* s) {
	if(it) {
		it[0].id = BLOCK_LOG;
		it[0].durability = this->block->metadata >> 1;
		it[0].count = 6 + 2 * (this->block->metadata & 1);
		it[1].id = BLOCK_SAPLING;
		it[1].durability = this->block->metadata >> 1;
		it[1].count = 2; //TODO: randomise
	}

	return 2;
}

static void onRightClick(struct server_local* s, struct item_data* it,
						 struct block_info* where, struct block_info* on,
						 enum side on_side) {
	server_local_spawn_item((vec3){where->x + 0.5F, where->y + 0.5F, where->z + 0.5F},
	(it->id == ITEM_SHEARS) ? &(struct item_data){
		.id = BLOCK_LEAVES,
		.durability = where->block->metadata >> 1,
		.count = 16
	}
	: &(struct item_data){
		.id = ITEM_STICK,
		.durability = 0,
		.count = 8
	}, false, s);
	server_world_set_block(s, on->x, on->y, on->z, (struct block_data) {
		.type = BLOCK_SAPLING,
		.metadata = on->block->metadata >> 1
	});
}

struct block block_tree2d = {
	.name = "Tree",
	.getSideMask = getSideMask,
	.getBoundingBox = getBoundingBox,
	.getMaterial = getMaterial,
	.getTextureIndex = getTextureIndex,
	.getDroppedItem = getDroppedItem,
	.onRandomTick = NULL,
	.onRightClick = onRightClick,
	.transparent = false,
	.renderBlock = render_block_tree2d,
	.renderBlockAlways = NULL,
	.luminance = 0,
	.double_sided = true,
	.can_see_through = true,
	.opacity = 0,
	.render_block_data.cross_random_displacement = false,
	.ignore_lighting = false,
	.flammable = false,
	.place_ignore = false,
	.digging.hardness = 30000,
	.digging.tool = TOOL_TYPE_AXE,
	.digging.min = TOOL_TIER_ANY,
	.digging.best = TOOL_TIER_MAX,
	.block_item = {
		.has_damage = false,
		.max_stack = 64,
		.renderItem = render_item_flat,
		.onItemPlace = block_place_default,
		.fuel = 0,
		.armor.is_armor = false,
		.tool.type = TOOL_TYPE_ANY,
	},
};
