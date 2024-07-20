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

#include "../item/recipe.h"
#include "../item/window_container.h"
#include "inventory_logic.h"
#include "server_local.h"

static bool inv_pre_action(struct inventory* inv, size_t slot, bool right,
						   set_inv_slot_t changes) {
	struct item_data chr;
	
	// text chars are stored as item counts
	if(inventory_get_slot(inv, slot, &chr)) {
		if(right) {
			chr.count++;
			if (chr.count > '~') chr.count = ' ';
		} else {
			chr.count--;
			if (chr.count < ' ') chr.count = '~';
		}
		inventory_set_slot(inv, slot, chr);
		set_inv_slot_push(changes, slot);
	}

	// do not move "char items" around
	return false;
}

static void inv_post_action(struct inventory* inv, size_t slot, bool right,
							bool accepted, set_inv_slot_t changes) {
}

static void inv_on_close(struct inventory* inv) {
	struct server_local* s = inv->user;

	set_inv_slot_t changes;
	set_inv_slot_init(changes);

	// TODO: save sign text

	server_local_send_inv_changes(changes, inv, WINDOWC_SIGN);
	set_inv_slot_clear(changes);

	inventory_destroy(inv);
}

static bool inv_on_collect(struct inventory* inv, struct item_data* item) {
	return false;
}

static void inv_on_create(struct inventory* inv) {
	struct server_local* s = inv->user;

	set_inv_slot_t changes;
	set_inv_slot_init(changes);

	// TODO: restore sign text

	for(size_t k = 0; k < SIGN_SIZE; k++) {
		inv->items[k].id = 1;
		inv->items[k].count = ' ';
		set_inv_slot_push(changes, k);
	}
	
	server_local_send_inv_changes(changes, inv, WINDOWC_SIGN);
	set_inv_slot_clear(changes);
}

static bool inv_on_destroy(struct inventory* inv) {
	struct server_local* s = inv->user;

	set_inv_slot_t changes;
	set_inv_slot_init(changes);

	server_local_send_inv_changes(changes, &s->player.inventory,
								  WINDOWC_INVENTORY);
	set_inv_slot_clear(changes);
	return true;
}

struct inventory_logic inventory_logic_sign = {
	.pre_action = inv_pre_action,
	.post_action = inv_post_action,
	.on_collect = inv_on_collect,
	.on_create = inv_on_create,
	.on_destroy = inv_on_destroy,
	.on_close = inv_on_close,
};
