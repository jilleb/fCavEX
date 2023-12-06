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
	return MATERIAL_STONE;
}

static size_t getBoundingBox(struct block_info* this, bool entity,
							 struct AABB* x) {
	return 0;
}

static struct face_occlusion*
getSideMask(struct block_info* this, enum side side, struct block_info* it) {
	int block_height = (this->block->metadata & 0x8) ?
		16 :
		(8 - this->block->metadata) * 2 * 7 / 8;
	switch(side) {
		case SIDE_TOP:
			return (it->block->type == this->block->type) ?
				face_occlusion_full() :
				face_occlusion_empty();
		case SIDE_BOTTOM: return face_occlusion_full();
		default: return face_occlusion_rect(block_height);
	}
}

static uint8_t getTextureIndex(struct block_info* this, enum side side) {
	return TEXTURE_INDEX(2, 0);
}

static size_t getDroppedItem(struct block_info* this, struct item_data* it,
							 struct random_gen* g) {
	return 0;
}

struct block block_lava = {
	.name = "Lava",
	.getSideMask = getSideMask,
	.getBoundingBox = getBoundingBox,
	.getMaterial = getMaterial,
	.getTextureIndex = getTextureIndex,
	.getDroppedItem = getDroppedItem,
	.onRandomTick = NULL,
	.onRightClick = NULL,
	.transparent = true,
	.renderBlock = render_block_fluid,
	.renderBlockAlways = NULL,
	.luminance = 15,
	.double_sided = false,
	.can_see_through = true,
	.opacity = 15,
	.ignore_lighting = false,
	.flammable = false,
	.place_ignore = true,
	.digging.hardness = 150000,
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
