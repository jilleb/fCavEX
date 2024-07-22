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
	if(x) {
		uint8_t state = ((this->block->metadata & 0x03)
						 + ((this->block->metadata & 0x04) ? 1 : 0))
			% 4;
		switch(state) {
			case 0:
				aabb_setsize(x, 0.1875F, 1.0F, 1.0F);
				aabb_translate(x, -0.40625F, 0, 0);
				break;
			case 1:
				aabb_setsize(x, 1.0F, 1.0F, 0.1875F);
				aabb_translate(x, 0, 0, -0.40625F);
				break;
			case 2:
				aabb_setsize(x, 0.1875F, 1.0F, 1.0F);
				aabb_translate(x, 0.40625F, 0, 0);
				break;
			case 3:
				aabb_setsize(x, 1.0F, 1.0F, 0.1875F);
				aabb_translate(x, 0, 0, 0.40625F);
				break;
		}
	}

	return 1;
}

static struct face_occlusion*
getSideMask(struct block_info* this, enum side side, struct block_info* it) {
	// could be improved
	return face_occlusion_empty();
}

static uint8_t getTextureIndex1(struct block_info* this, enum side side) {
	return (this->block->metadata & 0x08) ?
		tex_atlas_lookup(TEXAT_DOOR_WOOD_TOP) :
		tex_atlas_lookup(TEXAT_DOOR_WOOD_BOTTOM);
}

static uint8_t getTextureIndex2(struct block_info* this, enum side side) {
	return (this->block->metadata & 0x08) ?
		tex_atlas_lookup(TEXAT_DOOR_IRON_TOP) :
		tex_atlas_lookup(TEXAT_DOOR_IRON_BOTTOM);
}

static size_t getDroppedItem(struct block_info* this, struct item_data* it,
							 struct random_gen* g, struct server_local* s) {
	if(it) {
		it->id = ITEM_DOOR_WOOD;
		it->durability = 0;
		it->count = 1;
	}

	//only drop item from bottom half
	if(this->block->metadata < 8) return 1;
	return 0;
}

static size_t getDroppedItem2(struct block_info* this, struct item_data* it,
							 struct random_gen* g, struct server_local* s) {
	if(it) {
		it->id = ITEM_DOOR_IRON;
		it->durability = 0;
		it->count = 1;
	}

	//only drop item from bottom half
	if(this->block->metadata < 8) return 1;
	return 0;
}

static void onRightClick(struct server_local* s, struct item_data* it,
						 struct block_info* where, struct block_info* on,
						 enum side on_side) {
	struct block_data blk;

	if(server_world_get_block(&s->world, on->x, on->y - 1, on->z, &blk) && blk.type == BLOCK_DOOR_WOOD) {
		server_world_set_block(&s->world, on->x, on->y - 1, on->z, (struct block_data) {
			.type = BLOCK_DOOR_WOOD,
			.metadata = blk.metadata ^ 1
		});
	}

	if(server_world_get_block(&s->world, on->x, on->y + 1, on->z, &blk) && blk.type == BLOCK_DOOR_WOOD) {
		server_world_set_block(&s->world, on->x, on->y + 1, on->z, (struct block_data) {
			.type = BLOCK_DOOR_WOOD,
			.metadata = blk.metadata ^ 1
		});
	}

	server_world_set_block(&s->world, on->x, on->y, on->z, (struct block_data) {
		.type = BLOCK_DOOR_WOOD,
		.metadata = on->block->metadata ^ 1
	});
}

static void onRightClick2(struct server_local* s, struct item_data* it,
						 struct block_info* where, struct block_info* on,
						 enum side on_side) {
	struct block_data blk;

	if(server_world_get_block(&s->world, on->x, on->y - 1, on->z, &blk) && blk.type == BLOCK_DOOR_IRON) {
		server_world_set_block(&s->world, on->x, on->y - 1, on->z, (struct block_data) {
			.type = BLOCK_DOOR_IRON,
			.metadata = blk.metadata ^ 1
		});
	}

	if(server_world_get_block(&s->world, on->x, on->y + 1, on->z, &blk) && blk.type == BLOCK_DOOR_IRON) {
		server_world_set_block(&s->world, on->x, on->y + 1, on->z, (struct block_data) {
			.type = BLOCK_DOOR_IRON,
			.metadata = blk.metadata ^ 1
		});
	}

	server_world_set_block(&s->world, on->x, on->y, on->z, (struct block_data) {
		.type = BLOCK_DOOR_IRON,
		.metadata = on->block->metadata ^ 1
	});
}

struct block block_wooden_door = {
	.name = "Wooden Door",
	.getSideMask = getSideMask,
	.getBoundingBox = getBoundingBox,
	.getMaterial = getMaterial1,
	.getTextureIndex = getTextureIndex1,
	.getDroppedItem = getDroppedItem,
	.onRandomTick = NULL,
	.onRightClick = onRightClick,
	.transparent = false,
	.renderBlock = render_block_door,
	.renderBlockAlways = NULL,
	.luminance = 0,
	.double_sided = false,
	.can_see_through = true,
	.opacity = 1,
	.ignore_lighting = false,
	.flammable = false,
	.place_ignore = false,
	.digging.hardness = 4500,
	.digging.tool = TOOL_TYPE_ANY,
	.digging.min = TOOL_TIER_ANY,
	.digging.best = TOOL_TIER_ANY,
	.block_item = {
		.has_damage = false,
		.max_stack = 1,
		.renderItem = render_item_flat,
		.onItemPlace = block_place_default,
		.fuel = 0,
		.armor.is_armor = false,
		.tool.type = TOOL_TYPE_ANY,
	},
};

struct block block_iron_door = {
	.name = "Iron Door",
	.getSideMask = getSideMask,
	.getBoundingBox = getBoundingBox,
	.getMaterial = getMaterial2,
	.getTextureIndex = getTextureIndex2,
	.getDroppedItem = getDroppedItem2,
	.onRandomTick = NULL,
	.onRightClick = onRightClick2,
	.transparent = false,
	.renderBlock = render_block_door,
	.renderBlockAlways = NULL,
	.luminance = 0,
	.double_sided = false,
	.can_see_through = true,
	.opacity = 1,
	.ignore_lighting = false,
	.flammable = false,
	.place_ignore = false,
	.digging.hardness = 7500,
	.digging.tool = TOOL_TYPE_PICKAXE,
	.digging.min = TOOL_TIER_WOOD,
	.digging.best = TOOL_TIER_WOOD,
	.block_item = {
		.has_damage = false,
		.max_stack = 1,
		.renderItem = render_item_flat,
		.onItemPlace = block_place_default,
		.fuel = 0,
		.armor.is_armor = false,
		.tool.type = TOOL_TYPE_ANY,
	},
};
