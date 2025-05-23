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

#include "../network/client_interface.h"
#include "../network/inventory_logic.h"
#include "../network/server_local.h"
#include "blocks.h"

static enum block_material getMaterial(struct block_info* this) {
	return MATERIAL_STONE;
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
		case SIDE_TOP:
		case SIDE_BOTTOM:
			return tex_atlas_lookup(TEXAT_FURNACE_TOP);
		default:
			if (this->block->metadata == 0)
				return tex_atlas_lookup(TEXAT_FURNACE_FRONT);
			else
				return tex_atlas_lookup(TEXAT_FURNACE_FRONT_LIT);
	}
}

static uint8_t getTextureIndex2(struct block_info* this, enum side side) {
	switch(side) {
		case SIDE_FRONT:
			if(this->block->metadata == 2)
				return tex_atlas_lookup(TEXAT_FURNACE_FRONT_LIT);
			else
				return tex_atlas_lookup(TEXAT_FURNACE_SIDE);
		case SIDE_BACK:
			if(this->block->metadata == 3)
				return tex_atlas_lookup(TEXAT_FURNACE_FRONT_LIT);
			else
				return tex_atlas_lookup(TEXAT_FURNACE_SIDE);
		case SIDE_RIGHT:
			if(this->block->metadata == 5)
				return tex_atlas_lookup(TEXAT_FURNACE_FRONT_LIT);
			else
				return tex_atlas_lookup(TEXAT_FURNACE_SIDE);
		case SIDE_LEFT:
			if(this->block->metadata == 4)
				return tex_atlas_lookup(TEXAT_FURNACE_FRONT_LIT);
			else
				return tex_atlas_lookup(TEXAT_FURNACE_SIDE);
		default: return tex_atlas_lookup(TEXAT_FURNACE_TOP);
	}
}

static void onRightClick(struct server_local* s, struct item_data* it,
						 struct block_info* where, struct block_info* on,
						 enum side on_side) {
	if(items[it->id] && items[it->id]->fuel && on->block->metadata < 15) {
		uint8_t new_fuel = on->block->metadata + items[it->id]->fuel;
		if (new_fuel > 15) new_fuel = 15;
		server_world_set_block(s, on->x, on->y, on->z,
			(struct block_data) {
				.type = BLOCK_FURNACE,
				.metadata = new_fuel
			});

		size_t slot
			= inventory_get_hotbar(&s->player.inventory);
		inventory_consume(&s->player.inventory,
							slot + INVENTORY_SLOT_HOTBAR);

		clin_rpc_send(&(struct client_rpc) {
			.type = CRPC_INVENTORY_SLOT,
			.payload.inventory_slot.window = WINDOWC_INVENTORY,
			.payload.inventory_slot.slot
			= slot + INVENTORY_SLOT_HOTBAR,
			.payload.inventory_slot.item
			= s->player.inventory
					.items[slot + INVENTORY_SLOT_HOTBAR],
		});
	} else {
		if(on->block->metadata != 0) {
			if(s->player.active_inventory == &s->player.inventory) {
				clin_rpc_send(&(struct client_rpc) {
					.type = CRPC_OPEN_WINDOW,
					.payload.window_open.window = WINDOWC_FURNACE,
					.payload.window_open.type = WINDOW_TYPE_FURNACE,
					.payload.window_open.slot_count = FURNACE_SIZE,
				});

				struct inventory* inv = malloc(sizeof(struct inventory));
				inventory_create(inv, &inventory_logic_furnace, s, FURNACE_SIZE, on->x, on->y, on->z);
				s->player.active_inventory = inv;
			}
		}
	}
}

struct block block_furnaceoff = {
	.name = "Furnace",
	.getSideMask = getSideMask,
	.getBoundingBox = getBoundingBox,
	.getMaterial = getMaterial,
	.getTextureIndex = getTextureIndex1,
	.getDroppedItem = block_drop_default,
	.onRandomTick = NULL,
	.onRightClick = onRightClick,
	.transparent = false,
	.renderBlock = render_block_furnace,
	.renderBlockAlways = NULL,
	.luminance = 0, //depends on metadata
	.double_sided = false,
	.can_see_through = false,
	.ignore_lighting = false,
	.flammable = false,
	.place_ignore = false,
	.digging.hardness = 5250,
	.digging.tool = TOOL_TYPE_PICKAXE,
	.digging.min = TOOL_TIER_WOOD,
	.digging.best = TOOL_TIER_WOOD,
	.block_item = {
		.has_damage = false,
		.max_stack = 64,
		.renderItem = render_item_block,
		.onItemPlace = block_place_default,
		.fuel = 0,
		.render_data.block.has_default = true,
		.render_data.block.default_metadata = 0,
		.render_data.block.default_rotation = 0,
		.armor.is_armor = false,
		.tool.type = TOOL_TYPE_ANY,
	},
};

struct block block_furnaceon = {
	.name = "Furnace",
	.getSideMask = getSideMask,
	.getBoundingBox = getBoundingBox,
	.getMaterial = getMaterial,
	.getTextureIndex = getTextureIndex2,
	.getDroppedItem = block_drop_default,
	.onRandomTick = NULL,
	.onRightClick = onRightClick,
	.transparent = false,
	.renderBlock = render_block_full,
	.renderBlockAlways = NULL,
	.luminance = 13,
	.double_sided = false,
	.can_see_through = false,
	.ignore_lighting = false,
	.flammable = false,
	.place_ignore = false,
	.digging.hardness = 5250,
	.digging.tool = TOOL_TYPE_PICKAXE,
	.digging.min = TOOL_TIER_WOOD,
	.digging.best = TOOL_TIER_WOOD,
	.block_item = {
		.has_damage = false,
		.max_stack = 64,
		.renderItem = render_item_block,
		.onItemPlace = block_place_default,
		.fuel = 0,
		.render_data.block.has_default = true,
		.render_data.block.default_metadata = 2,
		.render_data.block.default_rotation = 0,
		.armor.is_armor = false,
		.tool.type = TOOL_TYPE_ANY,
	},
};
