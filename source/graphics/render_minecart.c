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
#include "../cglm/types.h"
#include "../cglm/cglm.h"
#include "render_item.h"   // for texture_minecart

static struct displaylist dl;
static uint8_t vertex_light[24], vertex_light_inv[24];

/* UV regions on 64×32 minecart.png */
#define UV_BOT_U   0   /* bottom panel at (0,10) size 20×16 */
#define UV_BOT_V  10
#define UV_BOT_W  20
#define UV_BOT_H  16

#define UV_SIDE_U   0  /* side panels at (0,0) size 16×8 */
#define UV_SIDE_V   0
#define UV_SIDE_W  16
#define UV_SIDE_H   8

#define UV_TOP_U    0  /* top edge strip at (0,26) size 18×1 */
#define UV_TOP_V   26
#define UV_TOP_W   18
#define UV_TOP_H    1

/* emit a quad CCW with its own UV rectangle */
static inline void emitFaceUV(int x0,int y0,int z0,
                              int x1,int y1,int z1,
                              int x2,int y2,int z2,
                              int x3,int y3,int z3,
                              uint8_t light,
                              uint8_t u0, uint8_t v0,
                              uint8_t u1, uint8_t v1)
{
    displaylist_pos(&dl, x0,y0,z0); displaylist_color(&dl, light); displaylist_texcoord(&dl, u0,    v0   );
    displaylist_pos(&dl, x1,y1,z1); displaylist_color(&dl, light); displaylist_texcoord(&dl, u1,    v0   );
    displaylist_pos(&dl, x2,y2,z2); displaylist_color(&dl, light); displaylist_texcoord(&dl, u1,    v1   );
    displaylist_pos(&dl, x3,y3,z3); displaylist_color(&dl, light); displaylist_texcoord(&dl, u0,    v1   );
}

static inline uint8_t MAX_U8(uint8_t a, uint8_t b) {
    return a > b ? a : b;
}

void render_minecart_init(void) {
    displaylist_init(&dl, 320, 64);
    memset(vertex_light,     0x0F, sizeof(vertex_light));
    memset(vertex_light_inv, 0xFF, sizeof(vertex_light_inv));
}

void render_minecart_update_light(uint8_t light) {
    memset(vertex_light,     light,  sizeof(vertex_light));
    memset(vertex_light_inv, ~light, sizeof(vertex_light_inv));
}

void render_minecart(int frame, mat4 view, bool fullbright) {
    assert(frame >= 0 && view);

    uint8_t raw = fullbright ? vertex_light_inv[0] : vertex_light[0];

    const float SO = 0.80f, SI = 0.60f, ST = 1.00f, SB = 0.60f;
    uint8_t L_outer   = (uint8_t)(raw * SO);
    uint8_t L_inner   = (uint8_t)(raw * SI);
    uint8_t L_topedge = (uint8_t)(raw * ST);
    uint8_t L_bottom  = (uint8_t)(raw * SB);

    /* original Minecraft proportions (in world units) */
    const int W = 20*16, D = 16*16, H = 8*16, T = 2*16;

    /* build modelview: lift + rotate Y+90° */
    mat4 model, mv;
    glm_mat4_identity(model);
    glm_translate_make(model, (vec3){0.0F, 0.5F + 1.0F/32.0F, 0.0F});
    glm_mat4_mul(view, model, mv);
    glm_rotate_y(mv, GLM_PI_2, mv);
    gfx_matrix_modelview(mv);

    gfx_bind_texture(&texture_minecart);
    displaylist_reset(&dl);

    // 1) bottom (normal down, UV_BOT)
    emitFaceUV(
        0,   0,   D,
        0,   0,   0,
        W,   0,   0,
        W,   0,   D,
        L_bottom,
        UV_BOT_U, UV_BOT_V,
        UV_BOT_U + UV_BOT_W, UV_BOT_V + UV_BOT_H
    );

    // 2) outer walls (UV_SIDE)
    emitFaceUV(W,0,0,  W,H,0,  0,H,0,   0,0,0,
               L_outer,
               UV_SIDE_U, UV_SIDE_V,
               UV_SIDE_U+UV_SIDE_W, UV_SIDE_V+UV_SIDE_H);
    emitFaceUV(0,0,D,  0,H,D,  W,H,D,   W,0,D,
               L_outer,
               UV_SIDE_U, UV_SIDE_V,
               UV_SIDE_U+UV_SIDE_W, UV_SIDE_V+UV_SIDE_H);
    emitFaceUV(0,0,0,  0,H,0,  0,H,D,   0,0,D,
               L_outer,
               UV_SIDE_U, UV_SIDE_V,
               UV_SIDE_U+UV_SIDE_W, UV_SIDE_V+UV_SIDE_H);
    emitFaceUV(W,0,D,  W,H,D,  W,H,0,   W,0,0,
               L_outer,
               UV_SIDE_U, UV_SIDE_V,
               UV_SIDE_U+UV_SIDE_W, UV_SIDE_V+UV_SIDE_H);

    // 3) inner walls (UV_SIDE)
    emitFaceUV(0,0,T,   W,0,T,   W,H,T,   0,H,T,
               L_inner,
               UV_SIDE_U, UV_SIDE_V,
               UV_SIDE_U+UV_SIDE_W, UV_SIDE_V+UV_SIDE_H);
    emitFaceUV(W,0,D-T, 0,0,D-T, 0,H,D-T, W,H,D-T,
               L_inner,
               UV_SIDE_U, UV_SIDE_V,
               UV_SIDE_U+UV_SIDE_W, UV_SIDE_V+UV_SIDE_H);
    emitFaceUV(T,0,0,    T,0,D,    T,H,D,    T,H,0,
               L_inner,
               UV_SIDE_U, UV_SIDE_V,
               UV_SIDE_U+UV_SIDE_W, UV_SIDE_V+UV_SIDE_H);
    emitFaceUV(W-T,0,D,  W-T,0,0,  W-T,H,0,  W-T,H,D,
               L_inner,
               UV_SIDE_U, UV_SIDE_V,
               UV_SIDE_U+UV_SIDE_W, UV_SIDE_V+UV_SIDE_H);

    // 4) top edges (UV_TOP)
    emitFaceUV(0, H,   0,    0, H,   T,    W, H,   T,    W, H,   0,
               L_topedge,
               UV_TOP_U, UV_TOP_V,
               UV_TOP_U+UV_TOP_W, UV_TOP_V+UV_TOP_H);
    emitFaceUV(W, H,   D,    W, H,   D-T,  0, H,   D-T,  0, H,   D,
               L_topedge,
               UV_TOP_U, UV_TOP_V,
               UV_TOP_U+UV_TOP_W, UV_TOP_V+UV_TOP_H);
    emitFaceUV(0, H,   D,    T, H,   D,    T, H,   0,    0, H,   0,
               L_topedge,
               UV_TOP_U, UV_TOP_V,
               UV_TOP_U+UV_TOP_W, UV_TOP_V+UV_TOP_H);
    emitFaceUV(W, H,   0,    W, H,   D,    W-T, H, D,    W-T, H,   0,
               L_topedge,
               UV_TOP_U, UV_TOP_V,
               UV_TOP_U+UV_TOP_W, UV_TOP_V+UV_TOP_H);

    gfx_lighting(true);
    displaylist_render_immediate(&dl, 52);
    gfx_lighting(false);

    gfx_matrix_modelview(GLM_MAT4_IDENTITY);
}
