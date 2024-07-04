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
	return true;
}

static void inv_post_action(struct inventory* inv, size_t slot, bool right,
							bool accepted, set_inv_slot_t changes) {
}

static void inv_on_close(struct inventory* inv) {
	struct server_local* s = inv->user;

	set_inv_slot_t changes;
	set_inv_slot_init(changes);

	for(int i = 0; i < MAX_CHESTS; i++) {
		if(s->chest_pos[i].x == inv->x && s->chest_pos[i].y == inv->y && s->chest_pos[i].z == inv->z) {
			for(size_t k = 0; k < IRON_CHEST_SIZE_STORAGE; k++) {
				s->chest_items[i][k]
					= inv->items[k + IRON_CHEST_SLOT_STORAGE];

				struct item_data item;
				inventory_get_slot(inv, k, &item);

				if (item.id != 0) {
					inventory_clear_slot(inv, k);
					set_inv_slot_push(changes, k);
				}
			}
			break;
		}
	}

	struct item_data picked_item;
	if(inventory_get_picked_item(inv, &picked_item)) {
		inventory_clear_picked_item(inv);
		set_inv_slot_push(changes, SPECIAL_SLOT_PICKED_ITEM);
		server_local_spawn_item((vec3) {s->player.x, s->player.y, s->player.z},
								&picked_item, true, s);
	}

	server_local_send_inv_changes(changes, inv, WINDOWC_IRON_CHEST);
	set_inv_slot_clear(changes);

	inventory_destroy(inv);
}

static bool inv_on_collect(struct inventory* inv, struct item_data* item) {
	uint8_t priorities[INVENTORY_SIZE_HOTBAR + INVENTORY_SIZE_MAIN];

	for(size_t k = 0; k < INVENTORY_SIZE_HOTBAR; k++)
		priorities[k] = k + IRON_CHEST_SLOT_HOTBAR;

	for(size_t k = 0; k < INVENTORY_SIZE_MAIN; k++)
		priorities[k + INVENTORY_SIZE_HOTBAR] = k + IRON_CHEST_SLOT_MAIN;

	set_inv_slot_t changes;
	set_inv_slot_init(changes);

	bool success
		= inventory_collect(inv, item, priorities,
							sizeof(priorities) / sizeof(*priorities), changes);
	server_local_send_inv_changes(changes, inv, WINDOWC_IRON_CHEST);
	set_inv_slot_clear(changes);

	return success;
}

static void inv_on_create(struct inventory* inv) {
	struct server_local* s = inv->user;

	set_inv_slot_t changes;
	set_inv_slot_init(changes);

	for(size_t k = 0; k < INVENTORY_SIZE_HOTBAR; k++) {
		inv->items[k + IRON_CHEST_SLOT_HOTBAR]
			= s->player.inventory.items[k + INVENTORY_SLOT_HOTBAR];
		set_inv_slot_push(changes, k + IRON_CHEST_SLOT_HOTBAR);
	}

	for(size_t k = 0; k < INVENTORY_SIZE_MAIN; k++) {
		inv->items[k + IRON_CHEST_SLOT_MAIN]
			= s->player.inventory.items[k + INVENTORY_SLOT_MAIN];
		set_inv_slot_push(changes, k + IRON_CHEST_SLOT_MAIN);
	}

	for(int i = 0; i < MAX_CHESTS; i++) {
		if(s->chest_pos[i].x == inv->x && s->chest_pos[i].y == inv->y && s->chest_pos[i].z == inv->z) {
			for(size_t k = 0; k < IRON_CHEST_SIZE_STORAGE; k++) {
				inv->items[k + IRON_CHEST_SLOT_STORAGE]
					= s->chest_items[i][k];
				set_inv_slot_push(changes, k + IRON_CHEST_SLOT_STORAGE);
			}
			break;
		}
	}
	
	server_local_send_inv_changes(changes, inv, WINDOWC_IRON_CHEST);
	set_inv_slot_clear(changes);
}

static bool inv_on_destroy(struct inventory* inv) {
	struct server_local* s = inv->user;

	set_inv_slot_t changes;
	set_inv_slot_init(changes);

	for(size_t k = 0; k < INVENTORY_SIZE_HOTBAR; k++) {
		s->player.inventory.items[k + INVENTORY_SLOT_HOTBAR]
			= inv->items[k + IRON_CHEST_SLOT_HOTBAR];
		set_inv_slot_push(changes, k + INVENTORY_SLOT_HOTBAR);
	}

	for(size_t k = 0; k < INVENTORY_SIZE_MAIN; k++) {
		s->player.inventory.items[k + INVENTORY_SLOT_MAIN]
			= inv->items[k + IRON_CHEST_SLOT_MAIN];
		set_inv_slot_push(changes, k + INVENTORY_SLOT_MAIN);
	}



	server_local_send_inv_changes(changes, &s->player.inventory,
								  WINDOWC_INVENTORY);
	set_inv_slot_clear(changes);
	return true;
}

struct inventory_logic inventory_logic_iron_chest = {
	.pre_action = inv_pre_action,
	.post_action = inv_post_action,
	.on_collect = inv_on_collect,
	.on_create = inv_on_create,
	.on_destroy = inv_on_destroy,
	.on_close = inv_on_close,
};
