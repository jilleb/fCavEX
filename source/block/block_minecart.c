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

#include "blocks.h"

static enum block_material getMaterial(struct block_info* this) {
	return MATERIAL_WOOD;
}

static size_t getBoundingBox(struct block_info* this, bool entity,
							 struct AABB* x) {
	if(x)
		aabb_setsize(x, 1.0F, 1.0F, 1.0F);
	return 1;
}

static struct face_occlusion*
getSideMask(struct block_info* this, enum side side, struct block_info* it) {
	return face_occlusion_full();
}

static uint8_t getTextureIndex(struct block_info* this, enum side side) {
	uint8_t tex[SIDE_MAX] = {
		[SIDE_TOP] = tex_atlas_lookup(TEXAT_CHEST_TOP),
		[SIDE_BOTTOM] = tex_atlas_lookup(TEXAT_CHEST_TOP),
		[SIDE_BACK] = tex_atlas_lookup(TEXAT_CHEST_SIDE),
		[SIDE_FRONT] = tex_atlas_lookup(TEXAT_CHEST_SIDE),
		[SIDE_LEFT] = tex_atlas_lookup(TEXAT_CHEST_SIDE),
		[SIDE_RIGHT] = tex_atlas_lookup(TEXAT_CHEST_SIDE),
	};

	switch(this->block->metadata) {
		case 0: tex[SIDE_FRONT] = tex_atlas_lookup(TEXAT_CHEST_FRONT_SINGLE); break;
		case 1: tex[SIDE_RIGHT] = tex_atlas_lookup(TEXAT_CHEST_FRONT_SINGLE); break;
		case 2: tex[SIDE_BACK] = tex_atlas_lookup(TEXAT_CHEST_FRONT_SINGLE); break;
		case 3: tex[SIDE_LEFT] = tex_atlas_lookup(TEXAT_CHEST_FRONT_SINGLE); break;
	}

 	return tex[side];
}



struct block block_minecart = {
	.name = "Sand",
	.getSideMask = getSideMask,
	.getBoundingBox = getBoundingBox,
	.getMaterial = getMaterial,
	.getTextureIndex = getTextureIndex,
	.getDroppedItem = block_drop_default,
	.onRandomTick = NULL,
	.onRightClick = NULL,
	.transparent = false,
	.renderBlock = render_block_full,
	.renderBlockAlways = NULL,
	.luminance = 0,
	.double_sided = false,
	.can_see_through = false,
	.ignore_lighting = false,
	.flammable = false,
	.place_ignore = false,
	.digging.hardness = 750,
	.digging.tool = TOOL_TYPE_SHOVEL,
	.digging.min = TOOL_TIER_ANY,
	.digging.best = TOOL_TIER_MAX,
	.block_item = {
		.has_damage = false,
		.max_stack = 64,
		.renderItem = render_item_block,
		.onItemPlace = block_place_default,
		.fuel = 0,
		.render_data.block.has_default = false,
		.armor.is_armor = false,
		.tool.type = TOOL_TYPE_ANY,
	},
};

