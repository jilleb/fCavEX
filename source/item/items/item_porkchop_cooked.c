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

static bool onItemPlace(struct server_local* s, struct item_data* it,
						struct block_info* where, struct block_info* on,
						enum side on_side) {
	//health test - eating cooked porkchop adds 2 health
	if (s->player.health >= 10) return false;
	s->player.health += 2;
	
	//send updated health to client
	clin_rpc_send(&(struct client_rpc) {
		.type = CRPC_PLAYER_SET_HEALTH,
		.payload.player_set_health.health = s->player.health
	});

	return true;
}

struct item item_porkchop_cooked = {
	.name = "Cooked Porkchop",
	.has_damage = false,
	.max_stack = 64,
	.renderItem = render_item_flat,
	.onItemPlace = onItemPlace,
	.armor.is_armor = false,
	.tool.type = TOOL_TYPE_ANY,
	.render_data = {
		.item = {
			.texture_x = 7,
			.texture_y = 5,
		},
	},
};

