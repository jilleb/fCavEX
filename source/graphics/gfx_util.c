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

#include <assert.h>

#include "../daytime.h"
#include "../game/game_state.h"
#include "../platform/gfx.h"
#include "gfx_util.h"

static void gutil_render_stars(mat4 view_matrix, float time) {
	float fade = 0.0f;
	float day_ticks = fmodf(time, 24000.0f);

	if (day_ticks > 13000.0f && day_ticks < 14000.0f)
	    fade = (day_ticks - 13000.0f) / 1000.0f;  // fade-in
	else if (day_ticks >= 14000.0f && day_ticks <= 22000.0f)
	    fade = 1.0f;
	else if (day_ticks > 22000.0f && day_ticks < 23000.0f)
	    fade = (23000.0f - day_ticks) / 1000.0f;  // fade-out
	else
	    fade = 0.0f;

	float scale = 0.2f; // star size. 0.2 seems about right Bigger than this

	uint8_t star_alpha = (uint8_t)(fade * 255.0f);
	if (star_alpha == 0)
		return;

	gfx_texture(false);
	gfx_lighting(false);
	gfx_blending(MODE_BLEND2);
	gfx_alpha_test(false);
	gfx_cull_func(MODE_NONE);

	srand(42);

	uint8_t star_r = 180;
	uint8_t star_g = 190;
	uint8_t star_b = 255;

	for (int i = 0; i < 1000; i++) {
	    float theta = glm_rad(rand() % 360);
	    float phi = glm_rad(rand() % 180);
	    float radius = 90.0f;

	    float x = cosf(theta) * sinf(phi) * radius;
	    float y = cosf(phi) * radius;
	    float z = sinf(theta) * sinf(phi) * radius;

	    vec3 pos = {x, y, z};

	    // billboard-quads zoals bij particles
	    float vertices[12];
	    float texcoords[8] = {
	        0.0f, 0.0f,
	        1.0f, 0.0f,
	        1.0f, 1.0f,
	        0.0f, 1.0f
	    };
	    uint8_t colors[16] = {
	        star_r, star_g, star_b, star_alpha,
	        star_r, star_g, star_b, star_alpha,
	        star_r, star_g, star_b, star_alpha,
	        star_r, star_g, star_b, star_alpha
	    };

	    vec3 right = {
	        view_matrix[0][0],
	        view_matrix[1][0],
	        view_matrix[2][0]
	    };
	    vec3 up = {
	        view_matrix[0][1],
	        view_matrix[1][1],
	        view_matrix[2][1]
	    };

	    glm_vec3_scale(right, scale, right);
	    glm_vec3_scale(up, scale, up);

	    vec3 v0, v1, v2, v3;
	    glm_vec3_sub(pos, right, v0); glm_vec3_add(v0, up, v0); // top-left
	    glm_vec3_add(pos, right, v1); glm_vec3_add(v1, up, v1); // top-right
	    glm_vec3_add(pos, right, v2); glm_vec3_sub(v2, up, v2); // bottom-right
	    glm_vec3_sub(pos, right, v3); glm_vec3_sub(v3, up, v3); // bottom-left

	    memcpy(vertices +  0, v0, sizeof(vec3));
	    memcpy(vertices +  3, v1, sizeof(vec3));
	    memcpy(vertices +  6, v2, sizeof(vec3));
	    memcpy(vertices +  9, v3, sizeof(vec3));

	    gfx_draw_quads_flt(4, vertices, colors, texcoords);
	}

	gfx_alpha_test(true);
	gfx_blending(MODE_OFF);
	gfx_texture(true);
	gfx_cull_func(MODE_BACK);

}


void gutil_clouds(mat4 view_matrix, float brightness) {
	assert(view_matrix);

	float cloud_pos = (gstate.world_time
					   + time_diff_s(gstate.world_time_start, time_get())
						   * 1000.0F / DAY_TICK_MS)
		* 12.0F / 400.0F;

	vec3 shift = {roundf(gstate.camera.x / 12.0F) * 12.0F
					  - (cloud_pos - roundf(cloud_pos / 12.0F) * 12.0F),
				  108.5F, roundf(gstate.camera.z / 12.0F) * 12.0F};

	mat4 model_view;
	glm_translate_to(view_matrix, shift, model_view);
	gfx_matrix_modelview(model_view);

	gfx_fog(true);
	gfx_fog_pos(shift[0] - gstate.camera.x, shift[2] - gstate.camera.z,
				gstate.config.fog_distance * 1.5F);
	gfx_texture(false);
	gfx_blending(MODE_BLEND);
	gfx_lighting(false);
	gfx_cull_func(MODE_NONE);
	gfx_write_buffers(false, true, true);

	int ox = roundf(gstate.camera.x / 12.0F) + roundf(cloud_pos / 12.0F);
	int oy = roundf(gstate.camera.z / 12.0F);

	uint8_t shade[6] = {
		roundf(brightness * 1.0F * 255),  roundf(brightness * 0.75F * 255),
		roundf(brightness * 0.84F * 255), roundf(brightness * 0.84F * 255),
		roundf(brightness * 0.92F * 255), roundf(brightness * 0.92F * 255),
	};

	for(int k = 0; k < 2; k++) {
		if(k == 1)
			gfx_write_buffers(true, false, true);

		for(int y = -16; y <= 16; y++) {
			for(int x = -16; x <= 16; x++) {
				uint8_t color[4];
				tex_gfx_lookup(&texture_clouds, ox + x, oy + y, color);

				if(color[3] >= 128) {
					int16_t min[] = {x * 12, y * 12, 0};
					int16_t box[] = {x * 12 + 12, y * 12 + 12, 4};

					// top, bottom, back, front, right, left
					gfx_draw_quads(
						24,
						(int16_t[]) {
							min[0], box[2], min[1], box[0], box[2], min[1],
							box[0], box[2], box[1], min[0], box[2], box[1],

							min[0], min[2], min[1], min[0], min[2], box[1],
							box[0], min[2], box[1], box[0], min[2], min[1],

							min[0], box[2], min[1], min[0], min[2], min[1],
							box[0], min[2], min[1], box[0], box[2], min[1],

							min[0], box[2], box[1], box[0], box[2], box[1],
							box[0], min[2], box[1], min[0], min[2], box[1],

							box[0], min[2], min[1], box[0], min[2], box[1],
							box[0], box[2], box[1], box[0], box[2], min[1],

							min[0], min[2], min[1], min[0], box[2], min[1],
							min[0], box[2], box[1], min[0], min[2], box[1],
						},
						(uint8_t[]) {
							shade[0], shade[0], shade[0], 0xBF,
							shade[0], shade[0], shade[0], 0xBF,
							shade[0], shade[0], shade[0], 0xBF,
							shade[0], shade[0], shade[0], 0xBF,

							shade[1], shade[1], shade[1], 0xBF,
							shade[1], shade[1], shade[1], 0xBF,
							shade[1], shade[1], shade[1], 0xBF,
							shade[1], shade[1], shade[1], 0xBF,

							shade[2], shade[2], shade[2], 0xBF,
							shade[2], shade[2], shade[2], 0xBF,
							shade[2], shade[2], shade[2], 0xBF,
							shade[2], shade[2], shade[2], 0xBF,

							shade[3], shade[3], shade[3], 0xBF,
							shade[3], shade[3], shade[3], 0xBF,
							shade[3], shade[3], shade[3], 0xBF,
							shade[3], shade[3], shade[3], 0xBF,

							shade[4], shade[4], shade[4], 0xBF,
							shade[4], shade[4], shade[4], 0xBF,
							shade[4], shade[4], shade[4], 0xBF,
							shade[4], shade[4], shade[4], 0xBF,

							shade[5], shade[5], shade[5], 0xBF,
							shade[5], shade[5], shade[5], 0xBF,
							shade[5], shade[5], shade[5], 0xBF,
							shade[5], shade[5], shade[5], 0xBF,
						},
						(uint16_t[]) {
							0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
							0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
							0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						});
				}
			}
		}
	}

	gfx_write_buffers(true, true, true);
	gfx_blending(MODE_OFF);
	gfx_texture(true);
	gfx_cull_func(MODE_BACK);
	gfx_fog(false);
}

void gutil_sky_box(mat4 view_matrix, float celestial_angle, vec3 color_top,
				   vec3 color_bottom) {
	assert(view_matrix && color_top && color_bottom);

	gfx_alpha_test(false);
	gfx_write_buffers(true, false, false);
	gfx_blending(MODE_OFF);
	gfx_lighting(false);
	gfx_texture(false);
	gfx_fog(true);

	mat4 model_view;
	glm_translate_to(view_matrix,
					 (vec3) {gstate.camera.x, gstate.camera.y, gstate.camera.z},
					 model_view);
	gfx_matrix_modelview(model_view);
	gfx_fog_pos(0, 0, gstate.config.fog_distance);

	// render a bit larger for possible inaccuracy
	float size = gstate.config.fog_distance + 4;

	gfx_draw_quads(4,
				   (int16_t[]) {-size, 16, -size, -size, 16, size, size, 16,
								size, size, 16, -size},
				   (uint8_t[]) {color_top[0], color_top[1], color_top[2], 0xFF,
								color_top[0], color_top[1], color_top[2], 0xFF,
								color_top[0], color_top[1], color_top[2], 0xFF,
								color_top[0], color_top[1], color_top[2], 0xFF},
				   (uint16_t[]) {0, 0, 0, 0, 0, 0, 0, 0});

	gfx_draw_quads(
		4,
		(int16_t[]) {-size, -32, -size, size, -32, -size, size, -32, size,
					 -size, -32, size},
		(uint8_t[]) {color_bottom[0], color_bottom[1], color_bottom[2], 0xFF,
					 color_bottom[0], color_bottom[1], color_bottom[2], 0xFF,
					 color_bottom[0], color_bottom[1], color_bottom[2], 0xFF,
					 color_bottom[0], color_bottom[1], color_bottom[2], 0xFF},
		(uint16_t[]) {0, 0, 0, 0, 0, 0, 0, 0});

	gfx_fog(false);
	gfx_texture(true);

	gutil_render_stars(view_matrix, daytime_get_time());

	gfx_blending(MODE_BLEND2);

	mat4 tmp;
	glm_translate_to(view_matrix,
					 (vec3) {gstate.camera.x, gstate.camera.y, gstate.camera.z},
					 tmp);
	glm_rotate_x(tmp, glm_rad(celestial_angle * 360.0F), model_view);
	gfx_matrix_modelview(model_view);

	gfx_bind_texture(&texture_sun);
	gfx_draw_quads(
		4, (int16_t[]) {-30, 100, -30, -30, 100, 30, 30, 100, 30, 30, 100, -30},
		(uint8_t[]) {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
		(uint16_t[]) {0, 0, 0, 256, 256, 256, 256, 0});

	gfx_bind_texture(&texture_moon);
	gfx_draw_quads(4,
				   (int16_t[]) {-20, -100, -20, 20, -100, -20, 20, -100, 20,
								-20, -100, 20},
				   (uint8_t[]) {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
								0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
				   (uint16_t[]) {0, 0, 0, 256, 256, 256, 256, 0});


	gfx_blending(MODE_OFF);
	gfx_write_buffers(true, true, true);
	gfx_alpha_test(true);
}

void gutil_block_selection(mat4 view_matrix, struct block_info* this) {
	assert(view_matrix && this);

	int pad = 1;

	if(!blocks[this->block->type])
		return;

	size_t count = blocks[this->block->type]->getBoundingBox(this, false, NULL);

	if(!count)
		return;

	struct AABB bbox[count];
	blocks[this->block->type]->getBoundingBox(this, false, bbox);

	gfx_fog(false);
	gfx_lighting(false);
	gfx_blending(MODE_BLEND);
	gfx_texture(false);

	mat4 model_view;
	glm_translate_to(view_matrix, (vec3) {this->x, this->y, this->z},
					 model_view);
	gfx_matrix_modelview(model_view);

	for(size_t k = 0; k < count; k++) {
		struct AABB* box = bbox + k;
		gfx_draw_lines(
			24,
			(int16_t[]) {
				// bottom
				-pad + box->x1 * 256, -pad + box->y1 * 256,
				-pad + box->z1 * 256, box->x2 * 256 + pad, -pad + box->y1 * 256,
				-pad + box->z1 * 256,

				-pad + box->x1 * 256, -pad + box->y1 * 256,
				-pad + box->z1 * 256, -pad + box->x1 * 256,
				-pad + box->y1 * 256, box->z2 * 256 + pad,

				box->x2 * 256 + pad, -pad + box->y1 * 256, box->z2 * 256 + pad,
				box->x2 * 256 + pad, -pad + box->y1 * 256, -pad + box->z1 * 256,

				box->x2 * 256 + pad, -pad + box->y1 * 256, box->z2 * 256 + pad,
				-pad + box->x1 * 256, -pad + box->y1 * 256, box->z2 * 256 + pad,

				// top
				-pad + box->x1 * 256, box->y2 * 256 + pad, -pad + box->z1 * 256,
				box->x2 * 256 + pad, box->y2 * 256 + pad, -pad + box->z1 * 256,

				-pad + box->x1 * 256, box->y2 * 256 + pad, -pad + box->z1 * 256,
				-pad + box->x1 * 256, box->y2 * 256 + pad, box->z2 * 256 + pad,

				box->x2 * 256 + pad, box->y2 * 256 + pad, box->z2 * 256 + pad,
				box->x2 * 256 + pad, box->y2 * 256 + pad, -pad + box->z1 * 256,

				box->x2 * 256 + pad, box->y2 * 256 + pad, box->z2 * 256 + pad,
				-pad + box->x1 * 256, box->y2 * 256 + pad, box->z2 * 256 + pad,

				// vertical
				-pad + box->x1 * 256, -pad + box->y1 * 256,
				-pad + box->z1 * 256, -pad + box->x1 * 256, box->y2 * 256 + pad,
				-pad + box->z1 * 256,

				box->x2 * 256 + pad, -pad + box->y1 * 256, -pad + box->z1 * 256,
				box->x2 * 256 + pad, box->y2 * 256 + pad, -pad + box->z1 * 256,

				-pad + box->x1 * 256, -pad + box->y1 * 256, box->z2 * 256 + pad,
				-pad + box->x1 * 256, box->y2 * 256 + pad, box->z2 * 256 + pad,

				box->x2 * 256 + pad, -pad + box->y1 * 256, box->z2 * 256 + pad,
				box->x2 * 256 + pad, box->y2 * 256 + pad, box->z2 * 256 + pad},
			(uint8_t[]) {0, 0,	 0, 153, 0, 0,	 0, 153, 0, 0,	 0, 153, 0, 0,
						 0, 153, 0, 0,	 0, 153, 0, 0,	 0, 153, 0, 0,	 0, 153,
						 0, 0,	 0, 153, 0, 0,	 0, 153, 0, 0,	 0, 153, 0, 0,
						 0, 153, 0, 0,	 0, 153, 0, 0,	 0, 153, 0, 0,	 0, 153,
						 0, 0,	 0, 153, 0, 0,	 0, 153, 0, 0,	 0, 153, 0, 0,
						 0, 153, 0, 0,	 0, 153, 0, 0,	 0, 153, 0, 0,	 0, 153,
						 0, 0,	 0, 153, 0, 0,	 0, 153, 0, 0,	 0, 153});
	}

	gfx_texture(true);
	gfx_lighting(true);
}

void gutil_entity_selection(mat4 view_matrix, const struct entity *e) {
    assert(view_matrix && e);
    assert(e->getBoundingBox != NULL);

    struct AABB box;
    size_t count = e->getBoundingBox(e, &box);
    if (count == 0) {
        return;
    }

    gfx_fog(false);
    gfx_lighting(false);
    gfx_blending(MODE_BLEND);
    gfx_texture(false);


    int base_x = (int)floorf(box.x1);
    int base_y = (int)floorf(box.y1);
    int base_z = (int)floorf(box.z1);

    float rel_x1 = box.x1 - (float)base_x;
    float rel_y1 = box.y1 - (float)base_y;
    float rel_z1 = box.z1 - (float)base_z;
    float rel_x2 = box.x2 - (float)base_x;
    float rel_y2 = box.y2 - (float)base_y;
    float rel_z2 = box.z2 - (float)base_z;

    mat4 model_view;
    glm_translate_to(view_matrix,
                     (vec3){(float)base_x, (float)base_y, (float)base_z},
                     model_view);
    gfx_matrix_modelview(model_view);

    int pad = 1;
    int16_t sx1 = (int16_t)(rel_x1 * 256.0f) - pad;
    int16_t sy1 = (int16_t)(rel_y1 * 256.0f) - pad;
    int16_t sz1 = (int16_t)(rel_z1 * 256.0f) - pad;
    int16_t sx2 = (int16_t)(rel_x2 * 256.0f) + pad;
    int16_t sy2 = (int16_t)(rel_y2 * 256.0f) + pad;
    int16_t sz2 = (int16_t)(rel_z2 * 256.0f) + pad;

    int16_t pts[24 * 3];
    int idx = 0;
    #define ADD_EDGE(x0,y0,z0, x1,y1,z1) \
        do {                             \
            pts[idx++] = (x0);           \
            pts[idx++] = (y0);           \
            pts[idx++] = (z0);           \
            pts[idx++] = (x1);           \
            pts[idx++] = (y1);           \
            pts[idx++] = (z1);           \
        } while (0)

    // Bottom face (y = sy1)
    ADD_EDGE(sx1, sy1, sz1,  sx2, sy1, sz1);
    ADD_EDGE(sx2, sy1, sz1,  sx2, sy1, sz2);
    ADD_EDGE(sx2, sy1, sz2,  sx1, sy1, sz2);
    ADD_EDGE(sx1, sy1, sz2,  sx1, sy1, sz1);

    // Top face (y = sy2)
    ADD_EDGE(sx1, sy2, sz1,  sx2, sy2, sz1);
    ADD_EDGE(sx2, sy2, sz1,  sx2, sy2, sz2);
    ADD_EDGE(sx2, sy2, sz2,  sx1, sy2, sz2);
    ADD_EDGE(sx1, sy2, sz2,  sx1, sy2, sz1);

    // Vertical edges
    ADD_EDGE(sx1, sy1, sz1,  sx1, sy2, sz1);
    ADD_EDGE(sx2, sy1, sz1,  sx2, sy2, sz1);
    ADD_EDGE(sx2, sy1, sz2,  sx2, sy2, sz2);
    ADD_EDGE(sx1, sy1, sz2,  sx1, sy2, sz2);

    #undef ADD_EDGE

    uint8_t colors[24 * 4];
    for (int i = 0; i < 24; i++) {
        colors[i * 4 + 0] = 0;   // R
        colors[i * 4 + 1] = 0;   // G
        colors[i * 4 + 2] = 0;   // B
        colors[i * 4 + 3] = 153; // A
    }

    gfx_draw_lines(24, pts, colors);

    gfx_texture(true);
    gfx_lighting(true);
}
