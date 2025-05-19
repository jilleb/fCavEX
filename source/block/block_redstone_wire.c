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
		aabb_setsize(x, 1.0F,
					 ((this->block->metadata & 0x7) > 1
					  && (this->block->metadata & 0x7) < 6) ?
						 0.625F :
						 0.125F,
					 1.0F);
	return entity ? 0 : 1;
}

static struct face_occlusion*
getSideMask(struct block_info* this, enum side side, struct block_info* it) {
	return face_occlusion_empty();
}

static uint8_t getTextureIndex(struct block_info* this, enum side side) {
	return tex_atlas_lookup(TEXAT_REDSTONE_WIRE_OFF);
}


static void onRightClick(struct server_local* s, struct item_data* it,
                         struct block_info* where, struct block_info* on,
                         enum side on_side) {
    // 1) Check we have an item in hand and it's redstone dust
    if (it && it->id == ITEM_REDSTONE) {
        struct block_data current;
        // 2) Read the block we clicked on
        if (server_world_get_block(&s->world,
                                   on->x, on->y, on->z,
                                   &current)
            && current.type == BLOCK_REDSTONE_WIRE)
        {
            // 3) Increment metadata (0–15), wrap at 16→0
            uint8_t level = current.metadata;
            level = (level + 1) & 0x0F;

            // 4) Write back the same block type with new metadata
            server_world_set_block(&s->world,
                                   on->x, on->y, on->z,
                                   (struct block_data){
                                       .type     = BLOCK_REDSTONE_WIRE,
                                       .metadata = level
                                   });

            // 5) Stop further processing
            return;
        }
    }

    // Fallback: default behavior (place block, use item, etc.)
    if (it && items[it->id] && items[it->id]->onItemPlace) {
        items[it->id]->onItemPlace(s, it, where, on, on_side);
    }
}


struct block block_redstone_wire = {
	.name = "Redstone wire",
	.getSideMask = getSideMask,
	.getBoundingBox = getBoundingBox,
	.getMaterial = getMaterial,
	.getTextureIndex = getTextureIndex,
	.getDroppedItem = block_drop_default,
	.onRandomTick = NULL,
	.onRightClick = onRightClick,
	.transparent = false,
	.renderBlock = render_block_redstone_wire,
	.renderBlockAlways = NULL,
	.luminance = 0,
	.double_sided = true,
	.can_see_through = true,
	.opacity = 0,
	.render_block_data.rail_curved_possible = true,
	.ignore_lighting = false,
	.flammable = false,
	.place_ignore = false,
	.digging.hardness = 1050,
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
