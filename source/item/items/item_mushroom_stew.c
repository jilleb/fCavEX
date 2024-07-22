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

#include "../../network/server_local.h"
#include "../../network/client_interface.h"
#include "../../network/server_interface.h"

static bool onItemPlace(struct server_local* s, struct item_data* it,
						struct block_info* where, struct block_info* on,
						enum side on_side) {
	if (s->player.health >= MAX_PLAYER_HEALTH) return false;

	server_local_set_player_health(s, s->player.health+5*HEALTH_PER_HEART);
	server_local_spawn_item((vec3) {where->x, where->y, where->z},
		&(struct item_data) {
			.id = ITEM_BOWL,
			.durability = 0,
			.count = 1
		}, false, s);
	return true;
}

struct item item_mushroom_stew = {
	.name = "Mushroom Stew",
	.has_damage = false,
	.max_stack = 1,
	.fuel = 0,
	.renderItem = render_item_flat,
	.onItemPlace = onItemPlace,
	.armor.is_armor = false,
	.tool.type = TOOL_TYPE_ANY,
	.render_data = {
		.item = {
			.texture_x = 8,
			.texture_y = 4,
		},
	},
};

