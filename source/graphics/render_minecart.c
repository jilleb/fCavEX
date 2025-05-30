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
/* render_minecart.c
   -----------------
   Draws the minecart entity as a simple 3D cube instead of a billboard.
*/

#include <assert.h>
#include <string.h>

#include "../block/blocks.h"
#include "../platform/displaylist.h"
#include "../platform/gfx.h"
#include "gui_util.h"
#include "gfx_settings.h"
#include "render_item.h"  // only for frames[] and texture_mobs

static struct displaylist dl;
static uint8_t vertex_light[24];
static uint8_t vertex_light_inv[24];

static inline uint8_t MAX_U8(uint8_t a, uint8_t b) {
    return a > b ? a : b;
}

static inline uint8_t DIM_LIGHT(uint8_t l, uint8_t* table) {
    return (table[l >> 4] << 4) | table[l & 0x0F];
}

void render_minecart_init(void) {
    displaylist_init(&dl, 320, /* max verts */ 24);
    memset(vertex_light,     0x0F, sizeof(vertex_light));
    memset(vertex_light_inv, 0xFF, sizeof(vertex_light_inv));
}

void render_minecart_update_light(uint8_t light) {
    memset(vertex_light,     light, sizeof(vertex_light));
    memset(vertex_light_inv, ~light, sizeof(vertex_light_inv));
}

void render_minecart(int frame, mat4 view, bool fullbright) {
    assert(frame >= 0 && view);

    // fetch the UV origin for this “frame” in the atlas
    uint8_t s = frames[frame].x;
    uint8_t t = frames[frame].y;

    // bind the same texture atlas you were using
    gfx_bind_texture(&texture_minecart);

    // pick a single lighting value for all vertices (you can expand this per-vertex if you like)
    uint8_t light = fullbright
        ? vertex_light_inv[0]
        : vertex_light[0];

    // build a simple model matrix to lift the cube a bit off the ground
    mat4 model, modelview;
    glm_mat4_identity(model);
    glm_translate_make(model, (vec3){0.0F, 0.0F, 0.5F + 1.0F/32.0F});
    glm_mat4_mul(view, model, modelview);
    gfx_matrix_modelview(modelview);

    // reset and emit all 6 faces (4 verts each = 24 verts)
    displaylist_reset(&dl);
    const int W = 256, H = 256, D = 256;

    // Front face (z = 0)
    displaylist_pos(&dl,  0, H, 0); displaylist_color(&dl, light); displaylist_texcoord(&dl, s,      t);
    displaylist_pos(&dl,  W, H, 0); displaylist_color(&dl, light); displaylist_texcoord(&dl, s + 16, t);
    displaylist_pos(&dl,  W, 0, 0); displaylist_color(&dl, light); displaylist_texcoord(&dl, s + 16, t + 32);
    displaylist_pos(&dl,  0, 0, 0); displaylist_color(&dl, light); displaylist_texcoord(&dl, s,      t + 32);

    // Back face (z = D)
    displaylist_pos(&dl,  W, H, D); displaylist_color(&dl, light); displaylist_texcoord(&dl, s,      t);
    displaylist_pos(&dl,  0, H, D); displaylist_color(&dl, light); displaylist_texcoord(&dl, s + 16, t);
    displaylist_pos(&dl,  0, 0, D); displaylist_color(&dl, light); displaylist_texcoord(&dl, s + 16, t + 32);
    displaylist_pos(&dl,  W, 0, D); displaylist_color(&dl, light); displaylist_texcoord(&dl, s,      t + 32);

    // Left face (x = 0)
    displaylist_pos(&dl,  0, H, D); displaylist_color(&dl, light); displaylist_texcoord(&dl, s,      t);
    displaylist_pos(&dl,  0, H, 0); displaylist_color(&dl, light); displaylist_texcoord(&dl, s + 16, t);
    displaylist_pos(&dl,  0, 0, 0); displaylist_color(&dl, light); displaylist_texcoord(&dl, s + 16, t + 32);
    displaylist_pos(&dl,  0, 0, D); displaylist_color(&dl, light); displaylist_texcoord(&dl, s,      t + 32);

    // Right face (x = W)
    displaylist_pos(&dl,  W, H, 0); displaylist_color(&dl, light); displaylist_texcoord(&dl, s,      t);
    displaylist_pos(&dl,  W, H, D); displaylist_color(&dl, light); displaylist_texcoord(&dl, s + 16, t);
    displaylist_pos(&dl,  W, 0, D); displaylist_color(&dl, light); displaylist_texcoord(&dl, s + 16, t + 32);
    displaylist_pos(&dl,  W, 0, 0); displaylist_color(&dl, light); displaylist_texcoord(&dl, s,      t + 32);

    // Top face (y = H)
    displaylist_pos(&dl,  0, H, D); displaylist_color(&dl, light); displaylist_texcoord(&dl, s,      t);
    displaylist_pos(&dl,  W, H, D); displaylist_color(&dl, light); displaylist_texcoord(&dl, s + 16, t);
    displaylist_pos(&dl,  W, H, 0); displaylist_color(&dl, light); displaylist_texcoord(&dl, s + 16, t + 32);
    displaylist_pos(&dl,  0, H, 0); displaylist_color(&dl, light); displaylist_texcoord(&dl, s,      t + 32);

    // Bottom face (y = 0)
    displaylist_pos(&dl,  0, 0, 0); displaylist_color(&dl, light); displaylist_texcoord(&dl, s,      t);
    displaylist_pos(&dl,  W, 0, 0); displaylist_color(&dl, light); displaylist_texcoord(&dl, s + 16, t);
    displaylist_pos(&dl,  W, 0, D); displaylist_color(&dl, light); displaylist_texcoord(&dl, s + 16, t + 32);
    displaylist_pos(&dl,  0, 0, D); displaylist_color(&dl, light); displaylist_texcoord(&dl, s,      t + 32);

    // draw all 24 verts in one go
    gfx_lighting(true);
    displaylist_render_immediate(&dl, 24);
    gfx_lighting(false);

    // restore identity for next object
    gfx_matrix_modelview(GLM_MAT4_IDENTITY);
}
