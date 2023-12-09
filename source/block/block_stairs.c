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

static enum block_material getMaterial1(struct block_info* this) {
	return MATERIAL_WOOD;
}

static enum block_material getMaterial2(struct block_info* this) {
	return MATERIAL_STONE;
}

static size_t getBoundingBox(struct block_info* this, bool entity,
							 struct AABB* x) {
	if(entity) {
		if(x) {
			aabb_setsize(x + 0, 1.0F, 0.5F, 1.0F);

			enum side facing
				= (enum side[4]) {SIDE_RIGHT, SIDE_LEFT, SIDE_BACK,
								  SIDE_FRONT}[this->block->metadata & 3];

			switch(facing) {
				default:
				case SIDE_FRONT:
					aabb_setsize(x + 1, 1.0F, 0.5F, 0.5F);
					aabb_translate(x + 1, 0, 0.5F, -0.25F);
					break;
				case SIDE_BACK:
					aabb_setsize(x + 1, 1.0F, 0.5F, 0.5F);
					aabb_translate(x + 1, 0, 0.5F, 0.25F);
					break;
				case SIDE_RIGHT:
					aabb_setsize(x + 1, 0.5F, 0.5F, 1.0F);
					aabb_translate(x + 1, 0.25F, 0.5F, 0);
					break;
				case SIDE_LEFT:
					aabb_setsize(x + 1, 0.5F, 0.5F, 1.0F);
					aabb_translate(x + 1, -0.25F, 0.5F, 0);
					break;
			}
		}

		return 2;
	} else {
		if(x)
			aabb_setsize(x, 1.0F, 1.0F, 1.0F);
		return 1;
	}
}

static struct face_occlusion side_mask = {
	.mask = {0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFFFFFFFF,
			 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF},
};

static struct face_occlusion side_mask_mirrored = {
	.mask = {0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0xFF00FF00, 0xFFFFFFFF,
			 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF},
};

static struct face_occlusion side_top_mask_1 = {
	.mask = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
			 0x00000000, 0x00000000, 0x00000000},
};

static struct face_occlusion side_top_mask_2 = {
	.mask = {0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF,
			 0x00FF00FF, 0x00FF00FF, 0x00FF00FF},
};

static struct face_occlusion side_top_mask_3 = {
	.mask = {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF,
			 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF},
};

static struct face_occlusion side_top_mask_4 = {
	.mask = {0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00,
			 0xFF00FF00, 0xFF00FF00, 0xFF00FF00},
};

static struct face_occlusion*
getSideMask(struct block_info* this, enum side side, struct block_info* it) {
	enum side facing = (enum side[4]) {SIDE_RIGHT, SIDE_LEFT, SIDE_BACK,
									   SIDE_FRONT}[this->block->metadata & 3];

	if(side == facing || side == SIDE_BOTTOM) {
		return face_occlusion_full();
	} else if(side == blocks_side_opposite(facing)) {
		return face_occlusion_rect(8);
	}

	if(side == SIDE_TOP) {
		switch(facing) {
			case SIDE_FRONT: return &side_top_mask_1;
			case SIDE_BACK: return &side_top_mask_3;
			case SIDE_RIGHT: return &side_top_mask_2;
			case SIDE_LEFT: return &side_top_mask_4;
			default:
				return face_occlusion_empty();
				// case never reached, just for -pedantic
		}
	} else {
		switch(facing) {
			case SIDE_FRONT: return &side_mask;
			case SIDE_BACK: return &side_mask_mirrored;
			case SIDE_RIGHT: return &side_mask;
			case SIDE_LEFT: return &side_mask_mirrored;
			default:
				return face_occlusion_empty();
				// case never reached, just for -pedantic
		}
	}
}

static uint8_t getTextureIndex1(struct block_info* this, enum side side) {
	return tex_atlas_lookup(TEXAT_PLANKS);
}

static uint8_t getTextureIndex2(struct block_info* this, enum side side) {
	return tex_atlas_lookup(TEXAT_COBBLESTONE);
}

static bool onItemPlace(struct server_local* s, struct item_data* it,
						struct block_info* where, struct block_info* on,
						enum side on_side) {
	int metadata = 0;
	double dx = s->player.x - (where->x + 0.5);
	double dz = s->player.z - (where->z + 0.5);

	if(fabs(dx) > fabs(dz)) {
		metadata = (dx >= 0) ? 1 : 0;
	} else {
		metadata = (dz >= 0) ? 3 : 2;
	}

	struct block_data blk = (struct block_data) {
		.type = it->id,
		.metadata = metadata,
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

static size_t drop_wood(struct block_info* this, struct item_data* it,
						struct random_gen* g) {
	if(it) {
		it->id = BLOCK_PLANKS;
		it->durability = 0;
		it->count = 1;
	}

	return 1;
}

static size_t drop_cobblestone(struct block_info* this, struct item_data* it,
							   struct random_gen* g) {
	if(it) {
		it->id = BLOCK_COBBLESTONE;
		it->durability = 0;
		it->count = 1;
	}

	return 1;
}

struct block block_wooden_stairs = {
	.name = "Stairs",
	.getSideMask = getSideMask,
	.getBoundingBox = getBoundingBox,
	.getMaterial = getMaterial1,
	.getTextureIndex = getTextureIndex1,
	.getDroppedItem = drop_wood,
	.onRandomTick = NULL,
	.onRightClick = NULL,
	.transparent = false,
	.renderBlock = render_block_stairs,
	.renderBlockAlways = render_block_stairs_always,
	.luminance = 0,
	.double_sided = false,
	.can_see_through = true,
	.opacity = 15,
	.ignore_lighting = true,
	.flammable = true,
	.place_ignore = false,
	.digging.hardness = 3000,
	.digging.tool = TOOL_TYPE_ANY,
	.digging.min = TOOL_TIER_ANY,
	.digging.best = TOOL_TIER_ANY,
	.block_item = {
		.has_damage = false,
		.max_stack = 64,
		.renderItem = render_item_block,
		.onItemPlace = onItemPlace,
		.render_data.block.has_default = true,
		.render_data.block.default_metadata = 2,
		.render_data.block.default_rotation = 0,
		.armor.is_armor = false,
		.tool.type = TOOL_TYPE_ANY,
	},
};

struct block block_stone_stairs = {
	.name = "Stairs",
	.getSideMask = getSideMask,
	.getBoundingBox = getBoundingBox,
	.getMaterial = getMaterial2,
	.getTextureIndex = getTextureIndex2,
	.getDroppedItem = drop_cobblestone,
	.onRandomTick = NULL,
	.onRightClick = NULL,
	.transparent = false,
	.renderBlock = render_block_stairs,
	.renderBlockAlways = render_block_stairs_always,
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
	.digging.best = TOOL_TIER_WOOD,
	.block_item = {
		.has_damage = false,
		.max_stack = 64,
		.renderItem = render_item_block,
		.onItemPlace = onItemPlace,
		.render_data.block.has_default = true,
		.render_data.block.default_metadata = 2,
		.render_data.block.default_rotation = 0,
		.armor.is_armor = false,
		.tool.type = TOOL_TYPE_ANY,
	},
};
