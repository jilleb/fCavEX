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
	return MATERIAL_STONE;
}

static size_t getBoundingBox(struct block_info* this, bool entity,
							 struct AABB* x) {
	if(x)
		aabb_setsize(x, 1.0F, 0.5F, 1.0F);
	return 1;
}

static struct face_occlusion*
getSideMask(struct block_info* this, enum side side, struct block_info* it) {
	switch(side) {
		case SIDE_TOP: return face_occlusion_empty();
		case SIDE_BOTTOM: return face_occlusion_full();
		default: return face_occlusion_rect(8);
	}
}

static uint8_t getTextureIndex(struct block_info* this, enum side side) {
	switch(this->block->metadata) {
		default:
		case 0:
			return (side == SIDE_TOP || side == SIDE_BOTTOM) ?
				tex_atlas_lookup(TEXAT_SLAB_STONE_TOP) :
				tex_atlas_lookup(TEXAT_SLAB_STONE_SIDE);
		case 1:
			return (side == SIDE_TOP) ?
				tex_atlas_lookup(TEXAT_SANDSTONE_TOP) :
				((side == SIDE_BOTTOM) ?
					 tex_atlas_lookup(TEXAT_SANDSTONE_BOTTOM) :
					 tex_atlas_lookup(TEXAT_SANDSTONE_SIDE));
		case 2: return tex_atlas_lookup(TEXAT_PLANKS);
		case 3: return tex_atlas_lookup(TEXAT_COBBLESTONE);
	}
}

static bool onItemPlace(struct server_local* s, struct item_data* it,
						struct block_info* where, struct block_info* on,
						enum side on_side) {
	if(on_side == SIDE_TOP && on->block->type == it->id
	   && on->block->metadata == it->durability) {
		struct block_data blk = (struct block_data) {
			.type = BLOCK_DOUBLE_SLAB,
			.metadata = it->durability,
			.sky_light = 0,
			.torch_light = 0,
		};

		struct block_info blk_info = *on;
		blk_info.block = &blk;

		if(entity_local_player_block_collide(
			   (vec3) {s->player.x, s->player.y, s->player.z}, &blk_info))
			return false;

		server_world_set_block(&s->world, on->x, on->y, on->z, blk);
		return true;
	} else if(where->block->type == BLOCK_AIR) {
		struct block_data blk = (struct block_data) {
			.type = it->id,
			.metadata = it->durability,
			.sky_light = 0,
			.torch_light = 0,
		};

		struct block_info blk_info = *where;
		blk_info.block = &blk;

		if(entity_local_player_block_collide(
			   (vec3) {s->player.x, s->player.y, s->player.z}, &blk_info))
			return false;

		server_world_set_block(&s->world, where->x, where->y, where->z, blk);
		return true;
	}

	return false;
}

static size_t getDroppedItem(struct block_info* this, struct item_data* it,
							 struct random_gen* g) {
	if(it) {
		it->id = this->block->type;
		it->durability = this->block->metadata;
		it->count = 1;
	}

	return 1;
}

struct block block_slab = {
	.name = "Slab",
	.getSideMask = getSideMask,
	.getBoundingBox = getBoundingBox,
	.getMaterial = getMaterial,
	.getTextureIndex = getTextureIndex,
	.getDroppedItem = getDroppedItem,
	.onRandomTick = NULL,
	.onRightClick = NULL,
	.transparent = false,
	.renderBlock = render_block_slab,
	.renderBlockAlways = NULL,
	.luminance = 0,
	.double_sided = false,
	.can_see_through = true,
	.opacity = 15,
	.ignore_lighting = true,
	.flammable = false,
	.place_ignore = false,
	.digging.hardness = 3000,
	.digging.tool = TOOL_TYPE_PICKAXE,
	.digging.min = TOOL_TIER_WOOD,
	.digging.best = TOOL_TIER_MAX,
	.block_item = {
		.has_damage = false,
		.max_stack = 64,
		.renderItem = render_item_block,
		.onItemPlace = onItemPlace,
		.render_data.block.has_default = false,
		.armor.is_armor = false,
		.tool.type = TOOL_TYPE_ANY,
	},
};
