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

#include "../block/blocks_data.h"
#include "../game/game_state.h"
#include "../graphics/render_monster.h"
#include "../network/client_interface.h"
#include "../network/server_local.h"
#include "../platform/gfx.h"
#include "entity.h"
#include "monsters_object.h"

struct monster_frame frames[256] = {
	// frame 0 is used for death
	{.x = 0,
	 .y = 0,
	 .length = 0,
	 .action = NULL,
	 .next_frame = 0
	},
	{.x = 0,
	 .y = 0,
	 .length = 10,
	 .action = NULL,
	 .next_frame = 2
	},
	{.x = 8,
	 .y = 4,
	 .length = 10,
	 .action = NULL,
	 .next_frame = 1
	},
};

struct monster* monsters[256] = {
	&monster_zombie,
};

static bool entity_client_tick(struct entity* e) {
	assert(e);

	glm_vec3_copy(e->pos, e->pos_old);
	glm_vec2_copy(e->orient, e->orient_old);

	for(int k = 0; k < 3; k++)
		if(fabsf(e->vel[k]) < 0.005F)
			e->vel[k] = 0.0F;

	struct AABB bbox;
	aabb_setsize_centered(&bbox, 0.25F, 0.25F, 0.25F);

	bool collision_xz = false;

	for(int k = 0; k < 3; k++)
		entity_try_move(e, e->pos, e->vel, &bbox, (size_t[]) {1, 0, 2}[k],
						&collision_xz, &e->on_ground);

	e->vel[1] -= 0.04F;
	e->vel[0] *= (e->on_ground ? 0.6F : 1.0F) * 0.98F;
	e->vel[2] *= (e->on_ground ? 0.6F : 1.0F) * 0.98F;
	e->vel[1] *= 0.98F;

	return false;
}

static bool entity_server_tick(struct entity* e, struct server_local* s) {
	assert(e);

	glm_vec3_copy(e->pos, e->pos_old);
	glm_vec2_copy(e->orient, e->orient_old);

	for(int k = 0; k < 3; k++)
		if(fabsf(e->vel[k]) < 0.005F)
			e->vel[k] = 0.0F;

	struct AABB bbox;
	aabb_setsize_centered(&bbox, 0.25F, 0.25F, 0.25F);

	bool collision_xz = false;

	for(int k = 0; k < 3; k++)
		entity_try_move(e, e->pos, e->vel, &bbox, (size_t[]) {1, 0, 2}[k],
						&collision_xz, &e->on_ground);

	e->vel[1] -= 0.04F;
	e->vel[0] *= (e->on_ground ? 0.6F : 1.0F) * 0.98F;
	e->vel[2] *= (e->on_ground ? 0.6F : 1.0F) * 0.98F;
	e->vel[1] *= 0.98F;

	return false;
}

static void entity_render(struct entity* e, mat4 view, float tick_delta) {
	int frame = e->data.monster.frame;

	vec3 pos_lerp;
	glm_vec3_lerp(e->pos_old, e->pos, tick_delta, pos_lerp);

	struct block_data in_block;
	entity_get_block(e, floorf(pos_lerp[0]), floorf(pos_lerp[1]),
					 floorf(pos_lerp[2]), &in_block);
	render_monster_update_light((in_block.torch_light << 4)
							 | in_block.sky_light);

	mat4 model;
	glm_translate_make(model, pos_lerp);
//		glm_translate_y(model, sinf(ticks / 30.0F * GLM_PIf) * 0.1F + 0.1F);
//		glm_rotate_y(model, glm_rad(ticks * 3.0F), model);
//		glm_scale_uni(model, 0.25F);
//		glm_translate(model, (vec3) {-0.5F, -0.5F, -0.5F});

	mat4 mv;
	glm_mat4_mul(view, model, mv);

	render_monster(frame, mv, false);

	struct AABB bbox;
	aabb_setsize_centered(&bbox, 0.25F, 0.25F, 0.25F);
	aabb_translate(&bbox, pos_lerp[0], pos_lerp[1] - 0.04F, pos_lerp[2]);
	entity_shadow(e, &bbox, view);
}

static size_t getBoundingBox(const struct entity *e, struct AABB *out) {
    assert(e && out);
    aabb_setsize_centered(out, 0.6f,  1.7f,  0.6f);
    aabb_translate(out, e->pos[0], e->pos[1], e->pos[2]);
    return 1;
}

void entity_monster(uint32_t id, struct entity* e, bool server, void* world,
				 int monster_id) {
	assert(e && world);
    e->name = "Monster";
	e->id = id;
	e->tick_server = entity_server_tick;
	e->tick_client = entity_client_tick;
	e->render = entity_render;
	e->teleport = entity_default_teleport;
	e->type = ENTITY_MONSTER;
	e->data.monster.id = monster_id;
	e->data.monster.frame = 1;
	e->getBoundingBox = getBoundingBox;
    e->leftClickText = "Attack";
//    e->onLeftClick   = onLeftClick;
    e->rightClickText = NULL;
//    e->onRightClick   = onRightClick;

	entity_default_init(e, server, world);
}
