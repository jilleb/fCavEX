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

#include <string.h>

#include "../block/blocks.h"
#include "../platform/displaylist.h"
#include "../platform/gfx.h"
#include "gui_util.h"
#include "gfx_settings.h"
#include "render_item.h"

static struct displaylist dl;
static uint8_t vertex_light[24];
static uint8_t vertex_light_inv[24];

static inline uint8_t MAX_U8(uint8_t a, uint8_t b) {
	return a > b ? a : b;
}

static inline uint8_t DIM_LIGHT(uint8_t l, uint8_t* table) {
	return (table[l >> 4] << 4) | table[l & 0x0F];
}

void render_monster_init() {
	displaylist_init(&dl, 320, 3 * 2 + 2 * 1 + 1);
	memset(vertex_light, 0x0F, sizeof(vertex_light));
	memset(vertex_light_inv, 0xFF, sizeof(vertex_light_inv));
}

void render_monster_update_light(uint8_t light) {
	memset(vertex_light, light, sizeof(vertex_light));
}

void render_monster(int frame, mat4 view,
					  bool fullbright) {
	assert(frame && view);

	uint8_t s, t;

	s = frames[frame].x;
	t = frames[frame].y;

	gfx_bind_texture(&texture_mobs);

	displaylist_reset(&dl);

	uint8_t light = fullbright ? *vertex_light_inv : *vertex_light;

	// right layer
	displaylist_pos(&dl, 0, 256, 0);
	displaylist_color(&dl, light);
	displaylist_texcoord(&dl, s + 16, t);

	displaylist_pos(&dl, 256, 256, 0);
	displaylist_color(&dl, light);
	displaylist_texcoord(&dl, s, t);

	displaylist_pos(&dl, 256, 0, 0);
	displaylist_color(&dl, light);
	displaylist_texcoord(&dl, s, t + 16);

	displaylist_pos(&dl, 0, 0, 0);
	displaylist_color(&dl, light);
	displaylist_texcoord(&dl, s + 16, t + 16);
	mat4 model;

	glm_mat4_identity(model);
	//glm_scale_uni(model, 1.5F);
	glm_translate_make(model,
						 (vec3) {0.0F, 0.0F, 0.5F + 1.0F / 32.0F});

	mat4 modelview;
	glm_mat4_mul(view, model, modelview);
	glm_mat4_ins3(GLM_MAT3_IDENTITY, modelview); //for billboarding
	gfx_matrix_modelview(modelview);

	gfx_lighting(true);
	displaylist_render_immediate(&dl, 4);
	gfx_lighting(false);

	gfx_matrix_modelview(GLM_MAT4_IDENTITY);
}

