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

#ifndef ENTITY_H
#define ENTITY_H

#include <m-lib/m-dict.h>
#include <stdbool.h>

#include "../cglm/cglm.h"
#include "../item/items.h"

struct camera;

enum entity_type {
	ENTITY_LOCAL_PLAYER,
	ENTITY_ITEM,
	ENTITY_MONSTER,
	ENTITY_MINECART
};

enum ai_state {
    AI_IDLE,
    AI_CHASE,
    AI_FUSE,
    AI_ATTACK,
    // todo: add more
};


struct server_local;

struct AABB;


struct entity {
	uint32_t id;
	bool on_server;
	void* world;
	int delay_destroy;
	short health;

	vec3 pos;
	vec3 pos_old;
	vec3 vel;
	vec2 orient;
	vec2 orient_old;
	bool on_ground;

	vec3 network_pos;

    float         detection_range;
    float         ai_timer;
    enum ai_state ai_state;

	bool (*tick_client)(struct entity*);
	bool (*tick_server)(struct entity*, struct server_local*);
	void (*render)(struct entity*, mat4, float);
	void (*teleport)(struct entity*, vec3);

	enum entity_type type;
    const char *name;
	size_t (*getBoundingBox)(const struct entity *e, struct AABB *out);
    const char *leftClickText;
    const char *rightClickText;
    bool (*onRightClick)(struct entity *e);
    bool (*onLeftClick)(struct entity *e);
	union entity_data {
		struct entity_local_player {
			int jump_ticks;
			bool capture_input;
		} local_player;
		struct entity_item {
			struct item_data item;
			int age;
		} item;
		struct entity_monster {
			int id;
			int frame;
			int frame_time_left;
		} monster;
        struct entity_minecart {
            // no extra fields needed for basic minecart
            // future data (e.g., rail_state, custom flags) can go here
			struct item_data item;   // houdt id, count, durability

        } minecart;


	} data;
};

struct monster {
	int max_health;
	int speed;
	int width;
	int height;
};




DICT_DEF2(dict_entity, uint32_t, M_BASIC_OPLIST, struct entity*, M_POD_OPLIST)

#include "../world.h"

void entity_local_player(uint32_t id, struct entity* e, struct world* w);
bool entity_local_player_block_collide(vec3 pos, struct block_info* blk_info);

void entity_item(uint32_t id, struct entity* e, bool server, void* world,
				 struct item_data it);

void entity_monster(uint32_t id, struct entity* e, bool server, void* world,
				 int monster_id);

void entity_minecart(uint32_t id, struct entity* e, bool server, void* world);

uint32_t entity_gen_id(dict_entity_t dict);
void entities_client_tick(dict_entity_t dict);
void entities_client_render(dict_entity_t dict, struct camera* c,
							float tick_delta);

void entity_default_init(struct entity* e, bool server, void* world);
void entity_default_teleport(struct entity* e, vec3 pos);
bool entity_default_client_tick(struct entity* e);

void entity_shadow(struct entity* e, struct AABB* a, mat4 view);

bool entity_get_block(struct entity* e, w_coord_t x, w_coord_t y, w_coord_t z,
					  struct block_data* blk);
bool entity_intersection_threshold(struct entity* e, struct AABB* aabb,
								   vec3 old_pos, vec3 new_pos,
								   float* threshold);
bool entity_intersection(struct entity* e, struct AABB* a,
						 bool (*test)(struct AABB* entity,
									  struct block_info* blk_info));
bool entity_block_aabb_test(struct AABB* entity, struct block_info* blk_info);
bool entity_aabb_intersection(struct entity* e, struct AABB* a);
void entity_try_move(struct entity* e, vec3 pos, vec3 vel, struct AABB* bbox,
					 size_t coord, bool* collision_xz, bool* on_ground);
bool entity_aabb_intersect_ray(const vec3 origin,
                               const vec3 dir,
                               const struct entity *e,
                               float *out_t);

struct entity *raycast_entity(dict_entity_t *entities,
                              const vec3 origin,
                              const vec3 dir,
                              float maxDist,
                              float *out_tNear);

#endif
