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

static uint8_t level_table_064[16] = {
	0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
};

static uint8_t level_table_080[16] = {
	0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
};

static inline uint8_t DIM_LIGHT(uint8_t l, uint8_t* table) {
	return (table[l >> 4] << 4) | table[l & 0x0F];
}

void render_item_init() {
	displaylist_init(&dl, 320, 3 * 2 + 2 * 1 + 1);
	memset(vertex_light, 0x0F, sizeof(vertex_light));
	memset(vertex_light_inv, 0xFF, sizeof(vertex_light_inv));
}

void render_item_update_light(uint8_t light) {
	memset(vertex_light, light, sizeof(vertex_light));
}

void render_item_flat(struct item* item, struct item_data* stack, mat4 view,
					  bool fullbright, enum render_item_env env) {
	assert(item && stack && view);

	uint8_t s, t;

	if(item_is_block(stack)) {
		struct block* b = blocks[stack->id];
		assert(b);

		struct block_data this_blk = (struct block_data) {
			.type = stack->id,
			.metadata = stack->durability,
			.sky_light = 15,
			.torch_light = 15,
		};

		uint8_t tex = b->getTextureIndex(
			&(struct block_info) {
				.block = &this_blk, .neighbours = NULL, .x = 0, .y = 0, .z = 0},
			SIDE_TOP);

		s = TEX_OFFSET(TEXTURE_X(tex));
		t = TEX_OFFSET(TEXTURE_Y(tex));

		gfx_bind_texture(b->transparent ? &texture_anim : &texture_terrain);
	} else {
		s = item->render_data.item.texture_x * 16;
		t = item->render_data.item.texture_y * 16;

		gfx_bind_texture(&texture_items);
	}

	if(env == R_ITEM_ENV_INVENTORY) {
		gfx_matrix_modelview(view);
		gutil_texquad(0, 0, s, t, 16, 16, 16 * GFX_GUI_SCALE, 16 * GFX_GUI_SCALE);
	} else {
		displaylist_reset(&dl);

		uint8_t light = fullbright ? *vertex_light_inv : *vertex_light;
		
		#ifdef GFX_3D_ELEMENTS
			// left layer
			displaylist_pos(&dl, 0, 256, -16);
			displaylist_color(&dl, light);
			displaylist_texcoord(&dl, s + 16, t);

			displaylist_pos(&dl, 0, 0, -16);
			displaylist_color(&dl, light);
			displaylist_texcoord(&dl, s + 16, t + 16);

			displaylist_pos(&dl, 256, 0, -16);
			displaylist_color(&dl, light);
			displaylist_texcoord(&dl, s, t + 16);

			displaylist_pos(&dl, 256, 256, -16);
			displaylist_color(&dl, light);
			displaylist_texcoord(&dl, s, t);
		#endif
		

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
		
		#ifdef GFX_3D_ELEMENTS
				for(int k = 0; k < 16; k++) {
					// front
					displaylist_pos(&dl, 256 - (k + 1) * 16, 256, 0);
					displaylist_color(&dl, DIM_LIGHT(light, level_table_080));
					displaylist_texcoord(&dl, s + k + 1, t);

					displaylist_pos(&dl, 256 - (k + 1) * 16, 0, 0);
					displaylist_color(&dl, DIM_LIGHT(light, level_table_080));
					displaylist_texcoord(&dl, s + k + 1, t + 16);

					displaylist_pos(&dl, 256 - (k + 1) * 16, 0, -16);
					displaylist_color(&dl, DIM_LIGHT(light, level_table_080));
					displaylist_texcoord(&dl, s + k, t + 16);

					displaylist_pos(&dl, 256 - (k + 1) * 16, 256, -16);
					displaylist_color(&dl, DIM_LIGHT(light, level_table_080));
					displaylist_texcoord(&dl, s + k, t);

					// back
					displaylist_pos(&dl, 256 - k * 16, 256, 0);
					displaylist_color(&dl, DIM_LIGHT(light, level_table_080));
					displaylist_texcoord(&dl, s + k + 1, t);

					displaylist_pos(&dl, 256 - k * 16, 256, -16);
					displaylist_color(&dl, DIM_LIGHT(light, level_table_080));
					displaylist_texcoord(&dl, s + k, t);
					displaylist_pos(&dl, 256 - k * 16, 0, -16);
					displaylist_color(&dl, DIM_LIGHT(light, level_table_080));
					displaylist_texcoord(&dl, s + k, t + 16);

					displaylist_pos(&dl, 256 - k * 16, 0, 0);
					displaylist_color(&dl, DIM_LIGHT(light, level_table_080));
					displaylist_texcoord(&dl, s + k + 1, t + 16);

					// top
					displaylist_pos(&dl, 0, 256 - k * 16, 0);
					displaylist_color(&dl, DIM_LIGHT(light, level_table_064));
					displaylist_texcoord(&dl, s + 16, t + k);

					displaylist_pos(&dl, 0, 256 - k * 16, -16);
					displaylist_color(&dl, DIM_LIGHT(light, level_table_064));
					displaylist_texcoord(&dl, s + 16, t + k + 1);

					displaylist_pos(&dl, 256, 256 - k * 16, -16);
					displaylist_color(&dl, DIM_LIGHT(light, level_table_064));
					displaylist_texcoord(&dl, s, t + k + 1);

					displaylist_pos(&dl, 256, 256 - k * 16, 0);
					displaylist_color(&dl, DIM_LIGHT(light, level_table_064));
					displaylist_texcoord(&dl, s, t + k);

					// bottom
					displaylist_pos(&dl, 0, 256 - (k + 1) * 16, 0);
					displaylist_color(&dl, DIM_LIGHT(light, level_table_064));
					displaylist_texcoord(&dl, s + 16, t + k);

					displaylist_pos(&dl, 256, 256 - (k + 1) * 16, 0);
					displaylist_color(&dl, DIM_LIGHT(light, level_table_064));
					displaylist_texcoord(&dl, s, t + k);

					displaylist_pos(&dl, 256, 256 - (k + 1) * 16, -16);
					displaylist_color(&dl, DIM_LIGHT(light, level_table_064));
					displaylist_texcoord(&dl, s, t + k + 1);

					displaylist_pos(&dl, 0, 256 - (k + 1) * 16, -16);
					displaylist_color(&dl, DIM_LIGHT(light, level_table_064));
					displaylist_texcoord(&dl, s + 16, t + k + 1);
				}
		#endif

		mat4 model;

		switch(env) {
			case R_ITEM_ENV_THIRDPERSON:
				glm_translate_make(model, (vec3) {-4.0F, -8.0F, -7.0F});
				glm_rotate_x(model, glm_rad(60.0F), model);
				glm_scale(model, (vec3) {9.0F, 9.0F, 9.0F});

				glm_translate(model, (vec3) {0.5F, 0.2F, 0.5F});
				glm_scale_uni(model, 1.5F);
				glm_rotate_y(model, glm_rad(90.0F), model);
				glm_rotate_z(model, glm_rad(335.0F), model);
				glm_translate(
					model, (vec3) {1.0F / 16.0F - 1.0F, -1.0F / 16.0F, 0.0F});
				break;
			case R_ITEM_ENV_ENTITY:
				glm_mat4_identity(model);
				#ifdef GFX_3D_ELEMENTS
					glm_scale_uni(model, 1.5F);
				#else
					if (item_is_block(stack)) glm_scale_uni(model, 0.25F);
				#endif
				glm_translate_make(model,
								   (vec3) {0.0F, 0.0F, 0.5F + 1.0F / 32.0F});
				break;
			case R_ITEM_ENV_FIRSTPERSON:
				glm_translate_make(model, (vec3) {0.5F, 0.2F, 0.5F});
				#ifdef GFX_3D_ELEMENTS
					glm_scale_uni(model, 1.5F);
					glm_rotate_y(model, glm_rad(50.0F), model);
					glm_rotate_z(model, glm_rad(335.0F), model);
					glm_translate(
						model, (vec3) {1.0F / 16.0F - 1.0F, -1.0F / 16.0F, 0.0F});			
				#else
					glm_scale_uni(model, 0.5F);
				#endif
				break;
			default: break;
		}

		mat4 modelview;
		glm_mat4_mul(view, model, modelview);
		#ifndef GFX_3D_ELEMENTS
			glm_mat4_ins3(GLM_MAT3_IDENTITY, modelview); //for billboarding
		#endif
		gfx_matrix_modelview(modelview);

		gfx_lighting(true);
		#ifdef GFX_3D_ELEMENTS
			displaylist_render_immediate(&dl, (2 + 16 * 4) * 4);
		#else
			//billboard item should need only 4 vertices
			displaylist_render_immediate(&dl, 4);
		#endif

		gfx_lighting(false);
	}
	gfx_matrix_modelview(GLM_MAT4_IDENTITY);
}

void render_item_block(struct item* item, struct item_data* stack, mat4 view,
					   bool fullbright, enum render_item_env env) {
	assert(item && stack && view);
	assert(item_is_block(stack));
	#ifdef GFX_3D_ELEMENTS
		struct block* b = blocks[stack->id];
		assert(b);

		struct block_data neighbour_blk = (struct block_data) {
			.type = BLOCK_AIR,
			.metadata = 0,
			.sky_light = 15,
			.torch_light = 0,
		};

		struct block_info neighbour = (struct block_info) {
			.block = &neighbour_blk,
			.neighbours = NULL,
			.x = 0,
			.y = 0,
			.z = 0,
		};

		struct block_data this_blk = (struct block_data) {
			.type = stack->id,
			.metadata = item->render_data.block.has_default ?
				item->render_data.block.default_metadata :
				stack->durability,
			.sky_light = 15,
			.torch_light = env == R_ITEM_ENV_INVENTORY ? 15 : b->luminance,
		};

		struct block_data n[6] = {
			neighbour_blk, neighbour_blk, neighbour_blk,
			neighbour_blk, neighbour_blk, neighbour_blk,
		};

		struct block_info this = (struct block_info) {
			.block = &this_blk,
			.neighbours = n,
			.x = 0,
			.y = 0,
			.z = 0,
		};

		displaylist_reset(&dl);

		size_t vertices = 0;

		for(int k = 0; k < 6; k++) {
			vertices += b->renderBlock(&dl, &this, (enum side)k, &neighbour,
									   fullbright ? vertex_light_inv : vertex_light,
									   false);
			if(b->renderBlockAlways)
				vertices += b->renderBlockAlways(
					&dl, &this, (enum side)k, &neighbour,
					fullbright ? vertex_light_inv : vertex_light, false);
		}

		mat4 model;

		if(env == R_ITEM_ENV_INVENTORY) {
			glm_translate_make(model, (vec3) {3 * 2, 3 * 2, -16});
			glm_scale(model, (vec3) {20, 20, -20});
			glm_translate(model, (vec3) {0.5F, 0.5F, 0.5F});
			glm_rotate_z(model, glm_rad(180.0F), model);
			glm_rotate_x(model, glm_rad(-30.0F), model);
			glm_rotate_y(
				model,
				glm_rad((item->render_data.block.has_default ?
							 item->render_data.block.default_rotation * 90.0F :
							 0)
						- 45.0F),
				model);
			glm_translate(model, (vec3) {-0.5F, -0.5F, -0.5F});
		} else if(env == R_ITEM_ENV_THIRDPERSON) {
			glm_translate_make(model, (vec3) {-4.0F, -14.0F, 3.0F});
			glm_rotate_x(model, glm_rad(22.5F), model);
			glm_rotate_y(model, glm_rad(45.0F), model);
			glm_scale(model, (vec3) {6.0F, 6.0F, 6.0F});
		} else {
			glm_mat4_identity(model);
		}

		mat4 modelview;
		glm_mat4_mul(view, model, modelview);
		gfx_matrix_modelview(modelview);

		gfx_bind_texture(b->transparent ? &texture_anim : &texture_terrain);
		gfx_lighting(true);
		displaylist_render_immediate(&dl, vertices * 4);
		gfx_matrix_modelview(GLM_MAT4_IDENTITY);
		gfx_lighting(false);
	#else
		render_item_flat(item, stack, view, fullbright, env);
	#endif
}
