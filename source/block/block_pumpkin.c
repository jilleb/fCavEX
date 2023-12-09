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
		aabb_setsize(x, 1.0F, 1.0F, 1.0F);
	return 1;
}

static struct face_occlusion*
getSideMask(struct block_info* this, enum side side, struct block_info* it) {
	return face_occlusion_full();
}

static uint8_t getTextureIndex1(struct block_info* this, enum side side) {
	switch(side) {
		case SIDE_FRONT:
			if(this->block->metadata == 2)
				return tex_atlas_lookup(TEXAT_PUMPKIN_FRONT);
			else
				return tex_atlas_lookup(TEXAT_PUMPKIN_SIDE);
		case SIDE_BACK:
			if(this->block->metadata == 0)
				return tex_atlas_lookup(TEXAT_PUMPKIN_FRONT);
			else
				return tex_atlas_lookup(TEXAT_PUMPKIN_SIDE);
		case SIDE_RIGHT:
			if(this->block->metadata == 3)
				return tex_atlas_lookup(TEXAT_PUMPKIN_FRONT);
			else
				return tex_atlas_lookup(TEXAT_PUMPKIN_SIDE);
		case SIDE_LEFT:
			if(this->block->metadata == 1)
				return tex_atlas_lookup(TEXAT_PUMPKIN_FRONT);
			else
				return tex_atlas_lookup(TEXAT_PUMPKIN_SIDE);
		default: return tex_atlas_lookup(TEXAT_PUMPKIN_TOP);
	}
}

static uint8_t getTextureIndex2(struct block_info* this, enum side side) {
	switch(side) {
		case SIDE_FRONT:
			if(this->block->metadata == 2)
				return tex_atlas_lookup(TEXAT_PUMPKIN_FRONT_LIT);
			else
				return tex_atlas_lookup(TEXAT_PUMPKIN_SIDE);
		case SIDE_BACK:
			if(this->block->metadata == 0)
				return tex_atlas_lookup(TEXAT_PUMPKIN_FRONT_LIT);
			else
				return tex_atlas_lookup(TEXAT_PUMPKIN_SIDE);
		case SIDE_RIGHT:
			if(this->block->metadata == 3)
				return tex_atlas_lookup(TEXAT_PUMPKIN_FRONT_LIT);
			else
				return tex_atlas_lookup(TEXAT_PUMPKIN_SIDE);
		case SIDE_LEFT:
			if(this->block->metadata == 1)
				return tex_atlas_lookup(TEXAT_PUMPKIN_FRONT_LIT);
			else
				return tex_atlas_lookup(TEXAT_PUMPKIN_SIDE);
		default: return tex_atlas_lookup(TEXAT_PUMPKIN_TOP);
	}
}

static bool onItemPlace(struct server_local* s, struct item_data* it,
						struct block_info* where, struct block_info* on,
						enum side on_side) {
	int metadata = 0;
	double dx = s->player.x - (where->x + 0.5);
	double dz = s->player.z - (where->z + 0.5);

	if(fabs(dx) > fabs(dz)) {
		metadata = (dx >= 0) ? 3 : 1;
	} else {
		metadata = (dz >= 0) ? 0 : 2;
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

struct block block_pumpkin = {
	.name = "Pumpkin",
	.getSideMask = getSideMask,
	.getBoundingBox = getBoundingBox,
	.getMaterial = getMaterial,
	.getTextureIndex = getTextureIndex1,
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
	.digging.hardness = 1500,
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
		.armor.is_armor = true,
		.armor.type = ARMOR_TYPE_HELMET,
		.armor.tier = ARMOR_TIER_BLOCK,
		.tool.type = TOOL_TYPE_ANY,
	},
};

struct block block_pumpkin_lit = {
	.name = "Jack o'Lantern",
	.getSideMask = getSideMask,
	.getBoundingBox = getBoundingBox,
	.getMaterial = getMaterial,
	.getTextureIndex = getTextureIndex2,
	.getDroppedItem = block_drop_default,
	.onRandomTick = NULL,
	.onRightClick = NULL,
	.transparent = false,
	.renderBlock = render_block_full,
	.renderBlockAlways = NULL,
	.luminance = 15,
	.double_sided = false,
	.can_see_through = false,
	.ignore_lighting = false,
	.flammable = false,
	.place_ignore = false,
	.digging.hardness = 1500,
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
