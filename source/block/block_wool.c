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
	return MATERIAL_WOOL;
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
	switch(this->block->metadata) {
		case 0: return tex_atlas_lookup(TEXAT_WOOL_0);
		case 1: return tex_atlas_lookup(TEXAT_WOOL_1);
		case 2: return tex_atlas_lookup(TEXAT_WOOL_2);
		case 3: return tex_atlas_lookup(TEXAT_WOOL_3);
		case 4: return tex_atlas_lookup(TEXAT_WOOL_4);
		case 5: return tex_atlas_lookup(TEXAT_WOOL_5);
		case 6: return tex_atlas_lookup(TEXAT_WOOL_6);
		case 7: return tex_atlas_lookup(TEXAT_WOOL_7);
		case 8: return tex_atlas_lookup(TEXAT_WOOL_8);
		case 9: return tex_atlas_lookup(TEXAT_WOOL_9);
		case 10: return tex_atlas_lookup(TEXAT_WOOL_10);
		case 11: return tex_atlas_lookup(TEXAT_WOOL_11);
		case 12: return tex_atlas_lookup(TEXAT_WOOL_12);
		case 13: return tex_atlas_lookup(TEXAT_WOOL_13);
		case 14: return tex_atlas_lookup(TEXAT_WOOL_14);
		case 15: return tex_atlas_lookup(TEXAT_WOOL_15);
	}
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

struct block block_wool = {
	.name = "Wool",
	.getSideMask = getSideMask,
	.getBoundingBox = getBoundingBox,
	.getMaterial = getMaterial,
	.getTextureIndex = getTextureIndex,
	.getDroppedItem = getDroppedItem,
	.onRandomTick = NULL,
	.onRightClick = NULL,
	.transparent = false,
	.renderBlock = render_block_full,
	.renderBlockAlways = NULL,
	.luminance = 0,
	.double_sided = false,
	.can_see_through = false,
	.ignore_lighting = false,
	.flammable = true,
	.place_ignore = false,
	.digging.hardness = 1200,
	.digging.tool = TOOL_TYPE_ANY,
	.digging.min = TOOL_TIER_ANY,
	.digging.best = TOOL_TIER_ANY,
	.block_item = {
		.has_damage = false,
		.max_stack = 64,
		.renderItem = render_item_block,
		.onItemPlace = block_place_default,
		.render_data.block.has_default = false,
		.armor.is_armor = false,
		.tool.type = TOOL_TYPE_ANY,
	},
};
