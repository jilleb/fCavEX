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

#ifndef BLOCKS_H
#define BLOCKS_H

#include <stdbool.h>
#include <stdint.h>

#include "../graphics/texture_atlas.h"
#include "../item/items.h"
#include "../platform/displaylist.h"
#include "../util.h"
#include "../world.h"
#include "aabb.h"
#include "blocks_data.h"
#include "face_occlusion.h"

struct block {
	char name[32];
	enum block_material (*getMaterial)(struct block_info*);
	uint8_t (*getTextureIndex)(struct block_info*, enum side);
	struct face_occlusion* (*getSideMask)(struct block_info*, enum side,
										  struct block_info*);
	size_t (*getBoundingBox)(struct block_info*, bool, struct AABB*);
	size_t (*renderBlock)(struct displaylist*, struct block_info*, enum side,
						  struct block_info*, uint8_t*, bool);
	size_t (*renderBlockAlways)(struct displaylist*, struct block_info*,
								enum side, struct block_info*, uint8_t*, bool);
	size_t (*getDroppedItem)(struct block_info*, struct item_data*,
							 struct random_gen*);
	void (*onRandomTick)(struct server_local*, struct block_info*);
	void (*onRightClick)(struct server_local*, struct item_data*,
						 struct block_info*, struct block_info*, enum side);
	bool transparent;
	uint8_t luminance : 4;
	uint8_t opacity : 4;
	bool double_sided;
	bool can_see_through;
	bool ignore_lighting;
	bool flammable;
	bool place_ignore;
	struct block_dig_data {
		int hardness;
		enum tool_type tool;
		enum tool_tier min;
		enum tool_tier best;
	} digging;
	union block_render_data {
		bool cross_random_displacement;
		bool rail_curved_possible;
	} render_block_data;
	struct item block_item;
};

extern struct block block_bedrock;
extern struct block block_slab;
extern struct block block_dirt;
extern struct block block_log;
extern struct block block_stone;
extern struct block block_leaves;
extern struct block block_grass;
extern struct block block_water_still;
extern struct block block_water_flowing;
extern struct block block_lava;
extern struct block block_sand;
extern struct block block_sandstone;
extern struct block block_gravel;
extern struct block block_ice;
extern struct block block_snow;
extern struct block block_snow_block;
extern struct block block_tallgrass;
extern struct block block_deadbush;
extern struct block block_flower;
extern struct block block_rose;
extern struct block block_furnaceoff;
extern struct block block_furnaceon;
extern struct block block_workbench;
extern struct block block_glass;
extern struct block block_clay;
extern struct block block_coalore;
extern struct block block_ironore;
extern struct block block_goldore;
extern struct block block_diamondore;
extern struct block block_redstoneore;
extern struct block block_redstoneore_lit;
extern struct block block_lapisore;
extern struct block block_wooden_stairs;
extern struct block block_stone_stairs;
extern struct block block_obsidian;
extern struct block block_spawner;
extern struct block block_cobblestone;
extern struct block block_mossstone;
extern struct block block_chest;
extern struct block block_locked_chest;
extern struct block block_cactus;
extern struct block block_pumpkin;
extern struct block block_pumpkin_lit;
extern struct block block_brown_mushroom;
extern struct block block_red_mushroom;
extern struct block block_reed;
extern struct block block_glowstone;
extern struct block block_torch;
extern struct block block_rail;
extern struct block block_powered_rail;
extern struct block block_detector_rail;
extern struct block block_redstone_torch;
extern struct block block_redstone_torch_lit;
extern struct block block_ladder;
extern struct block block_farmland;
extern struct block block_crops;
extern struct block block_planks;
extern struct block block_portal;
extern struct block block_iron;
extern struct block block_gold;
extern struct block block_diamond;
extern struct block block_lapis;
extern struct block block_cake;
extern struct block block_fire;
extern struct block block_double_slab;
extern struct block block_bed;
extern struct block block_sapling;
extern struct block block_bricks;
extern struct block block_wool;
extern struct block block_netherrack;
extern struct block block_soulsand;
extern struct block block_bookshelf;
extern struct block block_stone_pressure_plate;
extern struct block block_wooden_pressure_plate;
extern struct block block_jukebox;
extern struct block block_noteblock;
extern struct block block_sponge;
extern struct block block_dispenser;
extern struct block block_tnt;
extern struct block block_cobweb;
extern struct block block_fence;
extern struct block block_trapdoor;
extern struct block block_wooden_door;
extern struct block block_iron_door;

extern struct block* blocks[256];

#include "../graphics/render_block.h"
#include "../graphics/render_item.h"

void blocks_init(void);
enum side blocks_side_opposite(enum side s);
void blocks_side_offset(enum side s, int* x, int* y, int* z);
const char* block_side_name(enum side s);

bool block_place_default(struct server_local* s, struct item_data* it,
						 struct block_info* where, struct block_info* on,
						 enum side on_side);
size_t block_drop_default(struct block_info* this, struct item_data* it,
						  struct random_gen* g);

#endif
