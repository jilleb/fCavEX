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

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "../cglm/cglm.h"

#include "../item/window_container.h"
#include "../platform/thread.h"
#include "client_interface.h"
#include "inventory_logic.h"
#include "server_interface.h"
#include "server_local.h"
#include "server_world.h"
#include "complex_block_archive.h"

#define CHUNK_DIST2(x1, x2, z1, z2)                                            \
	(((x1) - (x2)) * ((x1) - (x2)) + ((z1) - (z2)) * ((z1) - (z2)))


struct entity* server_local_spawn_minecart(vec3 pos, struct server_local* s) {
    uint32_t entity_id = entity_gen_id(s->entities);
    struct entity** e_ptr = dict_entity_safe_get(s->entities, entity_id);
    *e_ptr = malloc(sizeof(struct entity));
    struct entity* e = *e_ptr;
    assert(e);

    entity_minecart(entity_id, e, true, &s->world);
    e->teleport(e, pos);

    glm_vec3_copy(
        (vec3){ rand_gen_flt(&s->rand_src) - 0.5f,
               rand_gen_flt(&s->rand_src) - 0.5f,
               rand_gen_flt(&s->rand_src) - 0.5f },
        e->vel
    );
    glm_vec3_normalize(e->vel);
    glm_vec3_scale(
        e->vel,
        (2.0f * rand_gen_flt(&s->rand_src) + 0.5f) * 0.1f,
        e->vel
    );

    clin_rpc_send(&(struct client_rpc) {
        .type = CRPC_SPAWN_MINECART,
        .payload.spawn_minecart.entity_id = e->id,
        .payload.spawn_minecart.pos       = { pos[0], pos[1], pos[2] },
    });

    return e;
}



struct entity* server_local_spawn_item(vec3 pos, struct item_data* it,
									   bool throw, struct server_local* s) {
	uint32_t entity_id = entity_gen_id(s->entities);
	struct entity** e_ptr = dict_entity_safe_get(s->entities, entity_id);
	*e_ptr = malloc(sizeof(struct entity));
	struct entity* e = *e_ptr;
	assert(e);

	entity_item(entity_id, e, true, &s->world, *it);
	e->teleport(e, pos);

	if(throw) {
		float rx = glm_rad(-s->player.rx
						   + (rand_gen_flt(&s->rand_src) - 0.5F) * 22.5F);
		float ry = glm_rad(s->player.ry + 90.0F
						   + (rand_gen_flt(&s->rand_src) - 0.5F) * 22.5F);
		e->vel[0] = sinf(rx) * sinf(ry) * 0.25F;
		e->vel[1] = cosf(ry) * 0.25F;
		e->vel[2] = cosf(rx) * sinf(ry) * 0.25F;
	} else {
		glm_vec3_copy((vec3) {rand_gen_flt(&s->rand_src) - 0.5F,
							  rand_gen_flt(&s->rand_src) - 0.5F,
							  rand_gen_flt(&s->rand_src) - 0.5F},
					  e->vel);
		glm_vec3_normalize(e->vel);
		glm_vec3_scale(
			e->vel, (2.0F * rand_gen_flt(&s->rand_src) + 0.5F) * 0.1F, e->vel);
	}

	clin_rpc_send(&(struct client_rpc) {
		.type = CRPC_SPAWN_ITEM,
		.payload.spawn_item.entity_id = e->id,
		.payload.spawn_item.item = e->data.item.item,
		.payload.spawn_item.pos = {e->pos[0], e->pos[1], e->pos[2]},
		.payload.spawn_item.vel = {e->vel[0], e->vel[1], e->vel[2]},
	});

	return e;
}

struct entity* server_local_spawn_monster(vec3 pos, int monster_id,
									   struct server_local* s) {
	uint32_t entity_id = entity_gen_id(s->entities);

	struct entity** e_ptr = dict_entity_safe_get(s->entities, entity_id);
	*e_ptr = malloc(sizeof(struct entity));
	struct entity* e = *e_ptr;
	assert(e);


	entity_monster(entity_id, e, true, &s->world, monster_id);

	pos[0] = floorf(pos[0]) + 0.5f;
	pos[2] = floorf(pos[2]) + 0.5f;
	//pos[1] = pos[1] + 1.0f;
	e->teleport(e, pos);

	glm_vec3_copy((vec3) {rand_gen_flt(&s->rand_src) - 0.5F,
							rand_gen_flt(&s->rand_src) - 0.5F,
							rand_gen_flt(&s->rand_src) - 0.5F},
					e->vel);
	glm_vec3_normalize(e->vel);
	glm_vec3_scale(
		e->vel, (2.0F * rand_gen_flt(&s->rand_src) + 0.5F) * 0.1F, e->vel);

	clin_rpc_send(&(struct client_rpc) {
		.type = CRPC_SPAWN_MONSTER,
		.payload.spawn_monster.entity_id = e->id,
		.payload.spawn_monster.monster_id = monster_id,
		.payload.spawn_monster.pos = {e->pos[0], e->pos[1], e->pos[2]},
	});

	return e;
}

void server_local_spawn_block_drops(struct server_local* s,
									struct block_info* blk_info) {
	assert(s && blk_info);

	if(!blocks[blk_info->block->type])
		return;

	struct random_gen tmp = s->rand_src;
	size_t count
		= blocks[blk_info->block->type]->getDroppedItem(blk_info, NULL, &tmp, s);

	if(count > 0) {
		struct item_data items[count];
		blocks[blk_info->block->type]->getDroppedItem(blk_info, items,
													  &s->rand_src, s);

		for(size_t k = 0; k < count; k++)
			server_local_spawn_item((vec3) {blk_info->x + 0.5F,
											blk_info->y + 0.5F,
											blk_info->z + 0.5F},
									items + k, false, s);
	}
}

void server_local_send_inv_changes(set_inv_slot_t changes,
								   struct inventory* inv, uint8_t window) {
	assert(changes && inv);

	set_inv_slot_it_t it;
	set_inv_slot_it(it, changes);

	while(!set_inv_slot_end_p(it)) {
		size_t slot = *set_inv_slot_ref(it);

		clin_rpc_send(&(struct client_rpc) {
			.type = CRPC_INVENTORY_SLOT,
			.payload.inventory_slot.window = window,
			.payload.inventory_slot.slot = slot,
			.payload.inventory_slot.item = (slot == SPECIAL_SLOT_PICKED_ITEM) ?
				inv->picked_item :
				inv->items[slot],
		});

		set_inv_slot_next(it);
	}
}

void server_local_set_player_health(struct server_local* s, short new_health) {
	s->player.health = new_health;
	if (s->player.health > MAX_PLAYER_HEALTH) s->player.health = MAX_PLAYER_HEALTH;
	if (s->player.health <= 0) {
		//player dead, drop all items and move to spawn position
		for (int i = 0; i < INVENTORY_SIZE; i++) {
			struct item_data item;
			inventory_get_slot(&s->player.inventory, i, &item);

			if (item.id != 0) {
				inventory_clear_slot(&s->player.inventory, i);
				clin_rpc_send(&(struct client_rpc) {
					.type = CRPC_INVENTORY_SLOT,
					.payload.inventory_slot.window = WINDOWC_INVENTORY,
					.payload.inventory_slot.slot = i,
					.payload.inventory_slot.item = s->player.inventory.items[i]
				});

				server_local_spawn_item(
					(vec3) {s->player.x, s->player.y, s->player.z}, &item, false, s);
			}
		}

		//respawn with half health
		s->player.health = MAX_PLAYER_HEALTH/2;
		s->player.x = s->player.spawn_x;
		s->player.y = s->player.spawn_y;
		s->player.z = s->player.spawn_z;
		clin_rpc_send(&(struct client_rpc) {
			.type = CRPC_PLAYER_POS,
			.payload.player_pos.position = {s->player.x, s->player.y, s->player.z},
			.payload.player_pos.rotation = {0, 0}
		});
	}

	//send updated health to client
	clin_rpc_send(&(struct client_rpc) {
		.type = CRPC_PLAYER_SET_HEALTH,
		.payload.player_set_health.health = s->player.health
	});
}

static void server_local_process(struct server_rpc* call, void* user) {
	assert(call && user);

	struct server_local* s = user;

	switch(call->type) {
		case SRPC_TOGGLE_PAUSE:
			s->paused = !s->paused;
			clin_rpc_send(&(struct client_rpc) {
				.type = CRPC_TIME_SET,
				.payload.time_set = s->world_time,
			});
			break;
		case SRPC_PLAYER_POS:
			if(s->player.finished_loading) {
				s->player.x = call->payload.player_pos.x;
				s->player.y = call->payload.player_pos.y;
				s->player.z = call->payload.player_pos.z;
				s->player.rx = call->payload.player_pos.rx;
				s->player.ry = call->payload.player_pos.ry;
				s->player.old_vel_y = s->player.vel_y;
				s->player.vel_y = call->payload.player_pos.vel_y;
				s->player.has_pos = true;
			}
			break;
		case SRPC_HOTBAR_SLOT:
			if(s->player.has_pos
			   && call->payload.hotbar_slot.slot < INVENTORY_SIZE_HOTBAR)
				inventory_set_hotbar(&s->player.inventory,
									 call->payload.hotbar_slot.slot);
			break;
		case SRPC_WINDOW_CLICK: {
			set_inv_slot_t changes;
			set_inv_slot_init(changes);

			bool accept = inventory_action(
				s->player.active_inventory, call->payload.window_click.slot,
				call->payload.window_click.right_click, changes);

			clin_rpc_send(&(struct client_rpc) {
				.type = CRPC_WINDOW_TRANSACTION,
				.payload.window_transaction.accepted = accept,
				.payload.window_transaction.action_id
				= call->payload.window_click.action_id,
				.payload.window_transaction.window
				= call->payload.window_click.window,
			});

			server_local_send_inv_changes(changes, s->player.active_inventory,
										  call->payload.window_click.window);
			set_inv_slot_clear(changes);
			break;
		}
		case SRPC_WINDOW_CLOSE: {
			if(s->player.active_inventory->logic
			   && s->player.active_inventory->logic->on_close)
				s->player.active_inventory->logic->on_close(
					s->player.active_inventory);

			s->player.active_inventory = &s->player.inventory;
			break;
		}
		case SRPC_BLOCK_DIG:
			if(s->player.has_pos && call->payload.block_dig.y >= 0
			   && call->payload.block_dig.y < WORLD_HEIGHT
			   && call->payload.block_dig.finished) {
				struct block_data blk;
				if(server_world_get_block(&s->world, call->payload.block_dig.x,
										  call->payload.block_dig.y,
										  call->payload.block_dig.z, &blk)) {
					server_world_set_block(s, call->payload.block_dig.x,
										   call->payload.block_dig.y,
										   call->payload.block_dig.z,
										   (struct block_data) {
											   .type = BLOCK_AIR,
											   .metadata = 0,
										   });

					struct item_data it_data;
					bool has_tool = inventory_get_hotbar_item(
						&s->player.inventory, &it_data);
					struct item* it = has_tool ? item_get(&it_data) : NULL;

					if(blocks[blk.type]
					   && ((it
							&& it->tool.type == blocks[blk.type]->digging.tool
							&& it->tool.tier >= blocks[blk.type]->digging.min)
						   || blocks[blk.type]->digging.min == TOOL_TIER_ANY
						   || blocks[blk.type]->digging.tool == TOOL_TYPE_ANY))
						server_local_spawn_block_drops(
							s,
							&(struct block_info) {
								.block = &blk,
								.neighbours = NULL,
								.x = call->payload.block_dig.x,
								.y = call->payload.block_dig.y,
								.z = call->payload.block_dig.z,
							});
				}
			}
			break;
		case SRPC_BLOCK_PLACE:
			if(s->player.has_pos && call->payload.block_place.y >= 0
			   && call->payload.block_place.y < WORLD_HEIGHT) {
				int x, y, z;
				blocks_side_offset(call->payload.block_place.side, &x, &y, &z);

				struct block_data blk_where, blk_on;
				if(server_world_get_block(
					   &s->world, call->payload.block_place.x + x,
					   call->payload.block_place.y + y,
					   call->payload.block_place.z + z, &blk_where)
				   && server_world_get_block(
					   &s->world, call->payload.block_place.x,
					   call->payload.block_place.y, call->payload.block_place.z,
					   &blk_on)) {
					struct block_info where = (struct block_info) {
						.block = &blk_where,
						.neighbours = NULL,
						.x = call->payload.block_place.x + x,
						.y = call->payload.block_place.y + y,
						.z = call->payload.block_place.z + z,
					};

					struct block_info on = (struct block_info) {
						.block = &blk_on,
						.neighbours = NULL,
						.x = call->payload.block_place.x,
						.y = call->payload.block_place.y,
						.z = call->payload.block_place.z,
					};

					struct item_data it_data;
					inventory_get_hotbar_item(&s->player.inventory, &it_data);
					struct item* it = item_get(&it_data);

					if(blocks[blk_on.type]
					   && blocks[blk_on.type]->onRightClick) {
						blocks[blk_on.type]->onRightClick(
							s, &it_data, &where, &on,
							call->payload.block_place.side);
					} else if((!blocks[blk_where.type]
							   || blocks[blk_where.type]->place_ignore)
							  && it && it->onItemPlace
							  && it->onItemPlace(
								  s, &it_data, &where, &on,
								  call->payload.block_place.side)) {
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
					}
				}
			}
			break;
		case SRPC_UNLOAD_WORLD:
			// save chunks here, then destroy all
			clin_rpc_send(&(struct client_rpc) {
				.type = CRPC_WORLD_RESET,
				.payload.world_reset.dimension = s->player.dimension,
				.payload.world_reset.local_entity = 0,
			});

			level_archive_write_player(
				&s->level, (vec3) {s->player.x, s->player.y, s->player.z},
				(vec2) {s->player.rx, s->player.ry}, NULL, s->player.dimension);

			level_archive_write_inventory(&s->level, &s->player.inventory);
			level_archive_write(&s->level, LEVEL_TIME, &s->world_time);

			level_archive_write(&s->level, LEVEL_PLAYER_HEALTH, &s->player.health);

			chest_archive_write(s->chest_pos, s->chest_items[0], s->level_name);
			sign_archive_write(s->sign_pos, s->sign_texts[0], s->level_name);

			dict_entity_it_t it;
			dict_entity_it(it, s->entities);

			while(!dict_entity_end_p(it)) {
				free(dict_entity_ref(it)->value);
				dict_entity_next(it);
			}
			dict_entity_reset(s->entities);
			server_world_destroy(&s->world);
			level_archive_destroy(&s->level);

			s->player.has_pos = false;
			s->player.finished_loading = false;
			string_reset(s->level_name);
			break;

		case SRPC_ENTITY_ATTACK:
		  uint32_t id = call->payload.entity_attack.entity_id;
		  struct entity **ptr = dict_entity_get(s->entities, id);
		  if (ptr && *ptr) {
		    (*ptr)->health -= 5;    // of welk DAMAGE‐getal je wilt
		    if ((*ptr)->health <= 0) {
		      (*ptr)->data.monster.fuse = 30;
		      (*ptr)->ai_state = AI_FUSE;
		    }
		  }
		  break;
		case SRPC_LOAD_WORLD:
			assert(!s->player.has_pos);

			string_set(s->level_name, call->payload.load_world.name);
			string_clear(call->payload.load_world.name);

			if(level_archive_create(&s->level, s->level_name)) {
				vec3 pos;
				vec2 rot;
				enum world_dim dim;
				if(level_archive_read_player(&s->level, pos, rot, NULL, &dim)) {
					server_world_create(&s->world, s->level_name, dim);
					s->player.x = pos[0];
					s->player.y = pos[1];
					s->player.z = pos[2];
					s->player.rx = rot[0];
					s->player.ry = rot[1];
					s->player.dimension = dim;
					s->player.fall_y = s->player.y;
					s->player.old_vel_y = 0;
					s->player.vel_y = 0;
					s->player.has_pos = true;
				}

				level_archive_read(&s->level, LEVEL_TIME, &s->world_time, 0);

				level_archive_read(&s->level, LEVEL_PLAYER_HEALTH, &s->player.health, 0);
				if (s->player.health > MAX_PLAYER_HEALTH) s->player.health = MAX_PLAYER_HEALTH;
				level_archive_read(&s->level, LEVEL_PLAYER_SPAWNX, &s->player.spawn_x, 0);
				level_archive_read(&s->level, LEVEL_PLAYER_SPAWNY, &s->player.spawn_y, 0);
				level_archive_read(&s->level, LEVEL_PLAYER_SPAWNZ, &s->player.spawn_z, 0);

				chest_archive_read(s->chest_pos, s->chest_items[0], s->level_name);
				sign_archive_read(s->sign_pos, s->sign_texts[0], s->level_name);

				s->player.oxygen = MAX_OXYGEN;

				dict_entity_reset(s->entities);
				s->player.active_inventory = &s->player.inventory;

				clin_rpc_send(&(struct client_rpc) {
					.type = CRPC_WORLD_RESET,
					.payload.world_reset.dimension = dim,
					.payload.world_reset.local_entity = 0,
				});

				clin_rpc_send(&(struct client_rpc) {
					.type = CRPC_PLAYER_SET_HEALTH,
					.payload.player_set_health.health = s->player.health
				});
			}
			break;
	}
}

static void server_local_update(struct server_local* s) {
	assert(s);

	// print TPS
	#ifndef NDEBUG
	ptime_t this_tick = time_get();
	float dt = time_diff_s(s->last_tick, this_tick);
	float tps = 1.0F / dt;
	s->last_tick = this_tick;
	printf("%f\n", tps);
	#endif

	svin_process_messages(server_local_process, s, false);

	if(!s->player.has_pos || s->paused)
		return;

	s->world_time++;

	dict_entity_it_t it;
	dict_entity_it(it, s->entities);

	while(!dict_entity_end_p(it)) {
		uint32_t key = dict_entity_ref(it)->key;
		struct entity* e = dict_entity_ref(it)->value;

		if(e->tick_server) {
			bool remove = (e->delay_destroy == 0) || e->tick_server(e, s);
			dict_entity_next(it);

			if(remove) {
				clin_rpc_send(&(struct client_rpc) {
					.type = CRPC_ENTITY_DESTROY,
					.payload.entity_destroy.entity_id = key,
				});

				free(e);
				dict_entity_erase(s->entities, key);
			} else if(e->delay_destroy < 0) {
				// TODO: find a more optimized way of moving entities on both client and server
				/*
				clin_rpc_send(&(struct client_rpc) {
					.type = CRPC_ENTITY_MOVE,
					.payload.entity_move.entity_id = key,
					.payload.entity_move.pos
					= {e->pos[0], e->pos[1], e->pos[2]},
				});
				*/
			}
		} else {
			dict_entity_next(it);
		}
	}

	w_coord_t px = WCOORD_CHUNK_OFFSET(floor(s->player.x));
	w_coord_t pz = WCOORD_CHUNK_OFFSET(floor(s->player.z));

	server_world_random_tick(&s->world, &s->rand_src, s, px, pz,
							 MAX_VIEW_DISTANCE - 2);
	server_world_tick(&s->world, s);

	w_coord_t cx, cz;
	if(server_world_furthest_chunk(&s->world, MAX_VIEW_DISTANCE, px, pz, &cx,
								   &cz)) {
		// unload just one chunk
		server_world_save_chunk(&s->world, true, cx, cz);
		clin_rpc_send(&(struct client_rpc) {
			.type = CRPC_UNLOAD_CHUNK,
			.payload.unload_chunk.x = cx,
			.payload.unload_chunk.z = cz,
		});
	}

	// iterate over all chunks that should be loaded
	bool c_nearest = false;
	w_coord_t c_nearest_x, c_nearest_z;
	w_coord_t c_nearest_dist2;
	for(w_coord_t z = pz - MAX_VIEW_DISTANCE; z <= pz + MAX_VIEW_DISTANCE;
		z++) {
		for(w_coord_t x = px - MAX_VIEW_DISTANCE; x <= px + MAX_VIEW_DISTANCE;
			x++) {
			w_coord_t d = CHUNK_DIST2(px, x, pz, z);
			if(!server_world_is_chunk_loaded(&s->world, x, z)
			   && (d < c_nearest_dist2 || !c_nearest)
			   && server_world_disk_has_chunk(&s->world, x, z)) {
				c_nearest_dist2 = d;
				c_nearest_x = x;
				c_nearest_z = z;
				c_nearest = true;
			}
		}
	}

	// load just one chunk
	struct server_chunk* sc;
	if(c_nearest
	   && server_world_load_chunk(&s->world, c_nearest_x, c_nearest_z, &sc)) {
		size_t sz = CHUNK_SIZE * CHUNK_SIZE * WORLD_HEIGHT;
		void* ids = malloc(sz);
		void* metadata = malloc(sz / 2);
		void* lighting_sky = malloc(sz / 2);
		void* lighting_torch = malloc(sz / 2);

		memcpy(ids, sc->ids, sz);
		memcpy(metadata, sc->metadata, sz / 2);
		memcpy(lighting_sky, sc->lighting_sky, sz / 2);
		memcpy(lighting_torch, sc->lighting_torch, sz / 2);

		clin_rpc_send(&(struct client_rpc) {
			.type = CRPC_CHUNK,
			.payload.chunk.x = c_nearest_x * CHUNK_SIZE,
			.payload.chunk.y = 0,
			.payload.chunk.z = c_nearest_z * CHUNK_SIZE,
			.payload.chunk.sx = CHUNK_SIZE,
			.payload.chunk.sy = WORLD_HEIGHT,
			.payload.chunk.sz = CHUNK_SIZE,
			.payload.chunk.ids = ids,
			.payload.chunk.metadata = metadata,
			.payload.chunk.lighting_sky = lighting_sky,
			.payload.chunk.lighting_torch = lighting_torch,
		});
	} else if(!s->player.finished_loading) {
		struct client_rpc pos;
		pos.type = CRPC_PLAYER_POS;
		if(level_archive_read_player(&s->level, pos.payload.player_pos.position,
									 pos.payload.player_pos.rotation, NULL,
									 NULL))
			clin_rpc_send(&pos);

		clin_rpc_send(&(struct client_rpc) {
			.type = CRPC_TIME_SET,
			.payload.time_set = s->world_time,
		});

		if(level_archive_read_inventory(&s->level, &s->player.inventory)) {
			for(size_t k = 0; k < INVENTORY_SIZE; k++) {
				if(s->player.inventory.items[k].id > 0) {
					clin_rpc_send(&(struct client_rpc) {
						.type = CRPC_INVENTORY_SLOT,
						.payload.inventory_slot.window = WINDOWC_INVENTORY,
						.payload.inventory_slot.slot = k,
						.payload.inventory_slot.item
						= s->player.inventory.items[k],
					});
				}
			}
		}

		s->player.finished_loading = true;
	}

	// check if player is underwater
	// server side X off by one?
	struct block_data blk;
	server_world_get_block(&s->world, s->player.x-1, s->player.y, s->player.z, &blk);
	bool in_water = (blk.type == BLOCK_WATER_STILL || blk.type == BLOCK_WATER_FLOW);
	bool in_lava = (blk.type == BLOCK_LAVA_STILL || blk.type == BLOCK_LAVA_FLOW);
	if(s->player.y != 0) {
		server_world_get_block(&s->world, s->player.x-1, s->player.y-1, s->player.z, &blk);
		if(blk.type == BLOCK_LAVA_STILL || blk.type == BLOCK_LAVA_FLOW) in_lava = true;
	}

	// check if player is falling
	// reset falling height if player is underwater
	if((s->player.old_vel_y >= -0.079f && s->player.vel_y < -0.079f) || in_water) {
		s->player.fall_y = s->player.y;
	}
	if(s->player.old_vel_y < -0.079f && s->player.vel_y >= -0.079f) {
		int fall_distance = s->player.fall_y - s->player.y;
		if(fall_distance >= 4) {
			server_local_set_player_health(s, s->player.health-HEALTH_PER_HEART*(fall_distance-3));
		}
		s->player.fall_y = s->player.y;
	}

	if(in_lava) {
		// damage player in lava every 8 ticks
		if((s->player.oxygen & 7) == 0) {
			server_local_set_player_health(s, s->player.health-HEALTH_PER_HEART*2);
		}
		s->player.oxygen--;
	} else if(in_water) {
		// damage drowning player every 32 ticks
		if(s->player.oxygen <= OXYGEN_THRESHOLD && (s->player.oxygen&31) == 0) {
			server_local_set_player_health(s, s->player.health-HEALTH_PER_HEART);
		}
		s->player.oxygen--;
	} else s->player.oxygen = MAX_OXYGEN;
}

static void* server_local_thread(void* user) {
	while(1) {
		server_local_update(user);
		thread_msleep(50);
	}

	return NULL;
}

void server_local_create(struct server_local* s) {
	assert(s);
	rand_gen_seed(&s->rand_src);
	s->paused = false;
	s->world_time = 0;
	s->player.has_pos = false;
	s->player.finished_loading = false;
	s->last_tick = time_get();
	string_init(s->level_name);

	inventory_create(&s->player.inventory, &inventory_logic_player, s,
					 INVENTORY_SIZE, 0, 0, 0);
	s->player.active_inventory = &s->player.inventory;
	dict_entity_init(s->entities);
	memset(s->chest_pos, -1, MAX_CHESTS*3*sizeof(int));
	memset(s->sign_pos, -1, MAX_SIGNS*3*sizeof(int));

	struct thread t;
	thread_create(&t, server_local_thread, s, 8);
}
