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
	return MATERIAL_STONE;
}

static enum block_material getMaterial2(struct block_info* this) {
	return MATERIAL_WOOD;
}

#define EYE_HEIGHT 1.62F

static size_t getBoundingBox(struct block_info* this, bool entity,
							 struct AABB* x) {
	if(x)
		aabb_setsize(x, 0.875F, 0.0625F, 0.875F);
	return entity ? 0 : 1;
}

struct face_occlusion side_bottom = {
	.mask = {0x00007FFE, 0x7FFE7FFE, 0x7FFE7FFE, 0x7FFE7FFE, 0x7FFE7FFE,
			 0x7FFE7FFE, 0x7FFE7FFE, 0x7FFE0000},
};

static struct face_occlusion*
getSideMask(struct block_info* this, enum side side, struct block_info* it) {
	return (side == SIDE_BOTTOM) ? &side_bottom : face_occlusion_empty();
}

static uint8_t getTextureIndex1(struct block_info* this, enum side side) {
	return tex_atlas_lookup(TEXAT_STONE);
}

static uint8_t getTextureIndex2(struct block_info* this, enum side side) {
	return tex_atlas_lookup(TEXAT_PLANKS);
}

static bool onItemPlace(struct server_local* s, struct item_data* it,
						struct block_info* where, struct block_info* on,
						enum side on_side) {
	struct block_data blk;
	if(!server_world_get_block(&s->world, where->x, where->y - 1, where->z,
							   &blk))
		return false;

	if(!blocks[blk.type] || blocks[blk.type]->can_see_through)
		return false;

	return block_place_default(s, it, where, on, on_side);
}


static void onWorldTick(struct server_local* s, struct block_info* info) {
    if (!info->neighbours) return;
	struct block_data cur = *info->block;
    float px       = s->player.x;
    float pz       = s->player.z;
    float footY    = s->player.y - EYE_HEIGHT;

    bool insideXZ = (px >= info->x && px < info->x + 1.0f)
                 && (pz >= info->z && pz < info->z + 1.0f);


    bool onPlate  = insideXZ
                 && (footY >= info->y && footY <= info->y + 0.1f);

    uint8_t newState = onPlate ? 1 : 0;

    if ((cur.metadata & 0x01) != newState) {
        cur.metadata = (cur.metadata & ~0x01) | newState;
        server_world_set_block(&s->world,
                               info->x, info->y, info->z,
                               cur);
    }
}

struct block block_stone_pressure_plate = {
	.name = "Pressure Plate",
	.getSideMask = getSideMask,
	.getBoundingBox = getBoundingBox,
	.getMaterial = getMaterial1,
	.getTextureIndex = getTextureIndex1,
	.getDroppedItem = block_drop_default,
	.onRandomTick = NULL,
	.onWorldTick = onWorldTick,
	.onRightClick = NULL,
	.transparent = false,
	.renderBlock = render_block_pressure_plate,
	.renderBlockAlways = NULL,
	.luminance = 0,
	.double_sided = false,
	.can_see_through = true,
	.opacity = 0,
	.ignore_lighting = false,
	.flammable = false,
	.place_ignore = false,
	.digging.hardness = 750,
	.digging.tool = TOOL_TYPE_PICKAXE,
	.digging.min = TOOL_TIER_WOOD,
	.digging.best = TOOL_TIER_WOOD,
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

struct block block_wooden_pressure_plate = {
	.name = "Pressure Plate",
	.getSideMask = getSideMask,
	.getBoundingBox = getBoundingBox,
	.getMaterial = getMaterial2,
	.getTextureIndex = getTextureIndex2,
	.getDroppedItem = block_drop_default,
	.onRandomTick = NULL,
	.onWorldTick = onWorldTick,
	.onRightClick = NULL,
	.transparent = false,
	.renderBlock = render_block_pressure_plate,
	.renderBlockAlways = NULL,
	.luminance = 0,
	.double_sided = false,
	.can_see_through = true,
	.opacity = 0,
	.ignore_lighting = false,
	.flammable = false,
	.place_ignore = false,
	.digging.hardness = 750,
	.digging.tool = TOOL_TYPE_PICKAXE,
	.digging.min = TOOL_TIER_WOOD,
	.digging.best = TOOL_TIER_WOOD,
	.block_item = {
		.has_damage = false,
		.max_stack = 64,
		.renderItem = render_item_block,
		.onItemPlace = onItemPlace,
		.fuel = 0,
		.render_data.block.has_default = false,
		.armor.is_armor = false,
		.tool.type = TOOL_TYPE_ANY,
	},
};
