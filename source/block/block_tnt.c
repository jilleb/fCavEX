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

#include "../cglm/types.h"
#include "../graphics/render_block.h"
#include "../graphics/render_item.h"
#include "../graphics/texture_atlas.h"
#include "../item/tool.h"
#include "../network/server_local.h"
#include "../network/server_world.h"
#include "../particle.h"
#include "aabb.h"
#include "blocks.h"
#include "blocks_data.h"
#include "face_occlusion.h"


//todo: cleanup
//todo: hide tnt block with explosion/particles
//todo: make explosion more efficient
//todo: limit the amount of particles that is spawned in some way
//todo: limit the amount of blocks that are dropped.

#define TNT_POWER 3.0f	  // blast radius 4 looks to be right, but crashes the game
#define TNT_FUSE_TICKS 15 // amount of ticks before TNT explodes
#define MAX_BLOCKS ((int)((2 * TNT_MAX_POWER + 1) * (2 * TNT_MAX_POWER + 1) * (2 * TNT_MAX_POWER + 1)))

static const int SIDE_VECTORS[6][3] = {
	{  0,  1,  0 }, // SIDE_TOP
	{  0, -1,  0 }, // SIDE_BOTTOM
	{  0,  0, -1 }, // SIDE_FRONT
	{  0,  0,  1 }, // SIDE_BACK
	{ -1,  0,  0 }, // SIDE_LEFT
	{  1,  0,  0 }  // SIDE_RIGHT
};

void tnt_explode(struct server_local* s, w_coord_t x, w_coord_t y, w_coord_t z, float power) {
	if (power > 4.0f) power = 4.0f;
	float power_squared = power * power;

	struct block_data tnt_blk;
	if (!server_world_get_block(&s->world, x, y, z, &tnt_blk)) return;
	if (tnt_blk.type == 0 || tnt_blk.type == 7) return;

	server_world_set_block(&s->world, x, y, z, (struct block_data){0});

	int radius = (int)(power + 0.5f);

	for (int dx = -radius; dx <= radius; dx++) {
		for (int dy = -radius; dy <= radius; dy++) {
			for (int dz = -radius; dz <= radius; dz++) {
				float dist2 = dx * dx + dy * dy + dz * dz;
				if (dist2 > power_squared) continue;

				w_coord_t bx = x + dx, by = y + dy, bz = z + dz;
				struct block_data blk;
				if (!server_world_get_block(&s->world, bx, by, bz, &blk)) continue;
				if (blk.type == 0 || blk.type == 7) continue;

				struct block_info info = {
					.x = bx,
					.y = by,
					.z = bz,
					.block = &blk
				};
				for (int i = 0; i < 6; i++) {
					w_coord_t nx = bx + SIDE_VECTORS[i][0];
					w_coord_t ny = by + SIDE_VECTORS[i][1];
					w_coord_t nz = bz + SIDE_VECTORS[i][2];
					server_world_get_block(&s->world, nx, ny, nz, &info.neighbours[i]);
				}

				if ((rand() % 3) == 0)
					server_local_spawn_block_drops(s, &info);

				server_world_set_block(&s->world, bx, by, bz, (struct block_data){ 0 });
			}
		}
	}
}




static void onWorldTick(struct server_local* s, struct block_info* info) {
	uint8_t fuse = info->block->metadata;

	if (fuse == 0) return; // not primed

	if (fuse > 1) {
		info->block->metadata--;
		server_world_set_block(&s->world, info->x, info->y, info->z, *info->block);
	} else {
		tnt_explode(s, info->x, info->y, info->z, TNT_POWER);
	}
}

static enum block_material getMaterial(struct block_info* this) {
	return MATERIAL_ORGANIC;
}

static size_t getBoundingBox(struct block_info* this, bool entity, struct AABB* x) {
	if (x) aabb_setsize(x, 1.0F, 1.0F, 1.0F);
	return 1;
}

static struct face_occlusion* getSideMask(struct block_info* this, enum side side, struct block_info* it) {
	return face_occlusion_full();
}

static uint8_t getTextureIndex(struct block_info* this, enum side side) {
	if (this->block->metadata > 0 && (this->block->metadata % 4 < 2)) {
		return tex_atlas_lookup(TEXAT_SNOW); // flashing
	}

	switch (side) {
		case SIDE_TOP:
		case SIDE_BOTTOM:
			return tex_atlas_lookup(TEXAT_TNT_TOP);
		default:
			return tex_atlas_lookup(TEXAT_TNT_SIDE);
	}
}

static void onRightClick(struct server_local* s, struct item_data* it,
						 struct block_info* where, struct block_info* on,
						 enum side on_side) {
	if (on->block->type == BLOCK_TNT && on->block->metadata == 0) {
		server_world_set_block(&s->world, on->x, on->y, on->z,
			(struct block_data){ .type = BLOCK_TNT, .metadata = TNT_FUSE_TICKS });
	}
}

static bool tnt_onItemPlace(struct server_local* s, struct item_data* it,
							struct block_info* where, struct block_info* on,
							enum side on_side) {
	struct block_data blk = (struct block_data) {
		.type = it->id,
		.metadata = 0,
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


struct block block_tnt = {
	.name = "TNT",
	.getSideMask = getSideMask,
	.getBoundingBox = getBoundingBox,
	.getMaterial = getMaterial,
	.getTextureIndex = getTextureIndex,
	.getDroppedItem = block_drop_default,
	.onRandomTick = NULL,
	.onWorldTick = onWorldTick,
	.onRightClick = onRightClick,
	.transparent = false,
	.renderBlock = render_block_full,
	.renderBlockAlways = NULL,
	.luminance = 0,
	.double_sided = false,
	.can_see_through = false,
	.ignore_lighting = false,
	.flammable = true,
	.place_ignore = false,
	.digging.hardness = 50,
	.digging.tool = TOOL_TYPE_ANY,
	.digging.min = TOOL_TIER_ANY,
	.digging.best = TOOL_TIER_ANY,
	.block_item = {
		.has_damage = false,
		.max_stack = 64,
		.renderItem = render_item_block,
		.onItemPlace = tnt_onItemPlace,
		.fuel = 0,
		.render_data.block.has_default = false,
		.armor.is_armor = false,
		.tool.type = TOOL_TYPE_ANY,
	},
};
