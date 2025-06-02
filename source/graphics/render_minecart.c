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
#include "render_item.h"
#include "render_block.h"


static struct displaylist dl;
static uint8_t vertex_light[24], vertex_light_inv[24];

/*
   Copies of level_table_* arrays from render_block.c,
   for top/bottom (0), north/south (1), east/west (2).
*/
static uint8_t level_table_0[16] = {
    0, 0, 0, 0, 1, 2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12,
};
static uint8_t level_table_1[16] = {
    0, 0, 0, 1, 2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13,
};
static uint8_t level_table_2[16] = {
    0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
};

static inline uint8_t DIM_LIGHT(uint8_t l, uint8_t* table, bool shade_sides,
                                uint8_t luminance) {
    if (shade_sides) {
        return (table[ MAX_U8(l >> 4, luminance) ] << 4)
             | table[ l & 0x0F ];
    } else {
        return (MAX_U8(l >> 4, luminance) << 4)
             | (l & 0x0F);
    }
}

/* UV regions on a 64×32 minecart texture */
#define UV_BOT_U   0
#define UV_BOT_V  10
#define UV_BOT_W  20
#define UV_BOT_H  16

#define UV_SIDE_U   0
#define UV_SIDE_V   0
#define UV_SIDE_W  16
#define UV_SIDE_H   8

#define UV_TOP_U    0
#define UV_TOP_V   26
#define UV_TOP_W   18
#define UV_TOP_H    1

/*
   emitFaceUV: emits 4 vertices with a single uniform light value.
   (Not used for the cube, but retained for completeness.)
*/
static inline void emitFaceUV(int x0,int y0,int z0,
                              int x1,int y1,int z1,
                              int x2,int y2,int z2,
                              int x3,int y3,int z3,
                              uint8_t light,
                              uint8_t u0, uint8_t v0,
                              uint8_t u1, uint8_t v1)
{
    displaylist_pos(&dl, x0, y0, z0);
    displaylist_color(&dl, light);
    displaylist_texcoord(&dl, u0,    v0   );

    displaylist_pos(&dl, x1, y1, z1);
    displaylist_color(&dl, light);
    displaylist_texcoord(&dl, u1,    v0   );

    displaylist_pos(&dl, x2, y2, z2);
    displaylist_color(&dl, light);
    displaylist_texcoord(&dl, u1,    v1   );

    displaylist_pos(&dl, x3, y3, z3);
    displaylist_color(&dl, light);
    displaylist_texcoord(&dl, u0,    v1   );
}

/*
   emitFaceUV4: emits 4 vertices, each with its own light value l0..l3,
   using the same UV rectangle (u0,v0)→(u1,v1).
*/
static inline void emitFaceUV4(
    int x0,int y0,int z0, uint8_t l0,
    int x1,int y1,int z1, uint8_t l1,
    int x2,int y2,int z2, uint8_t l2,
    int x3,int y3,int z3, uint8_t l3,
    uint8_t u0, uint8_t v0,
    uint8_t u1, uint8_t v1
) {
    displaylist_pos(&dl, x0, y0, z0); displaylist_color(&dl, l0); displaylist_texcoord(&dl, u0,    v0   );
    displaylist_pos(&dl, x1, y1, z1); displaylist_color(&dl, l1); displaylist_texcoord(&dl, u1,    v0   );
    displaylist_pos(&dl, x2, y2, z2); displaylist_color(&dl, l2); displaylist_texcoord(&dl, u1,    v1   );
    displaylist_pos(&dl, x3, y3, z3); displaylist_color(&dl, l3); displaylist_texcoord(&dl, u0,    v1   );
}

/*
    emitCubeWithSideUV:
    ===================
    Draws an axis‐aligned box from (x0,y0,z0) to (x0+Sx, y0+Sy, z0+Sz),
    using per‐vertex lighting driven by the single raw value in vertex_light[0],
    but creating a vertical gradient on each side by darkening bottom corners.

    level_table_0 is for top/bottom faces,
    level_table_1 for north/south faces,
    level_table_2 for east/west faces.
    We compute for vertical faces:
      • raw_top    = base_raw
      • raw_bottom = max(0, base_raw − 16)
    so that the bottom two corners appear slightly darker.
*/
static void emitCubeWithSideUV(int x0, int y0, int z0,
                               int Sx, int Sy, int Sz)
{
    int x1 = x0 + Sx;
    int y1 = y0 + Sy;
    int z1 = z0 + Sz;

    // UV coords (all faces use the UV_SIDE rectangle)
    const uint8_t u0 = UV_SIDE_U;
    const uint8_t v0 = UV_SIDE_V;
    const uint8_t u1 = UV_SIDE_U + UV_SIDE_W;
    const uint8_t v1 = UV_SIDE_V + UV_SIDE_H;

    // The “base” raw‐light is stored in vertex_light[0] by render_minecart_update_light.
    uint8_t base_raw = vertex_light[0];
    uint8_t raw_top    = base_raw;
    uint8_t raw_bottom = (base_raw > 8 ? base_raw - 8 : 0);

    // --------------------------------------------------------
    // 1) Bottom face (y=y0), normal => −Y
    //    Winding: (x1,y0,z0)->(x0,y0,z0)->(x0,y0,z1)->(x1,y0,z1)
    // --------------------------------------------------------
    {
        int vx0 = x1, vy0 = y0, vz0 = z0;
        int vx1 = x0, vy1 = y0, vz1 = z0;
        int vx2 = x0, vy2 = y0, vz2 = z1;
        int vx3 = x1, vy3 = y0, vz3 = z1;

        // All four bottom‐face corners use raw = base_raw
        uint8_t l0 = DIM_LIGHT(base_raw, level_table_0, true, 0);
        uint8_t l1 = DIM_LIGHT(base_raw, level_table_0, true, 0);
        uint8_t l2 = DIM_LIGHT(base_raw, level_table_0, true, 0);
        uint8_t l3 = DIM_LIGHT(base_raw, level_table_0, true, 0);

        emitFaceUV4(
            vx0, vy0, vz0, l0,
            vx1, vy1, vz1, l1,
            vx2, vy2, vz2, l2,
            vx3, vy3, vz3, l3,
            u0, v0, u1, v1
        );
    }

    // --------------------------------------------------------
    // 2) Top face (y=y1), normal => +Y
    //    Winding: (x1,y1,z1)->(x0,y1,z1)->(x0,y1,z0)->(x1,y1,z0)
    // --------------------------------------------------------
    {
        int vx0 = x1, vy0 = y1, vz0 = z1;
        int vx1 = x0, vy1 = y1, vz1 = z1;
        int vx2 = x0, vy2 = y1, vz2 = z0;
        int vx3 = x1, vy3 = y1, vz3 = z0;

        // All four top‐face corners use raw = base_raw
        uint8_t l0 = DIM_LIGHT(base_raw, level_table_0, true, 0);
        uint8_t l1 = DIM_LIGHT(base_raw, level_table_0, true, 0);
        uint8_t l2 = DIM_LIGHT(base_raw, level_table_0, true, 0);
        uint8_t l3 = DIM_LIGHT(base_raw, level_table_0, true, 0);

        emitFaceUV4(
            vx0, vy0, vz0, l0,
            vx1, vy1, vz1, l1,
            vx2, vy2, vz2, l2,
            vx3, vy3, vz3, l3,
            u0, v0, u1, v1
        );
    }

    // --------------------------------------------------------
    // 3) North face (z=z1), normal => +Z
    //    Winding: (x0,y0,z1)->(x0,y1,z1)->(x1,y1,z1)->(x1,y0,z1)
    // --------------------------------------------------------
    {
        int vx0 = x0, vy0 = y0, vz0 = z1; // bottom-left
        int vx1 = x0, vy1 = y1, vz1 = z1; // top-left
        int vx2 = x1, vy2 = y1, vz2 = z1; // top-right
        int vx3 = x1, vy3 = y0, vz3 = z1; // bottom-right

        // top corners use raw_top, bottom corners raw_bottom
        uint8_t l0 = DIM_LIGHT(raw_bottom, level_table_1, true, 0);
        uint8_t l1 = DIM_LIGHT(raw_top,    level_table_1, true, 0);
        uint8_t l2 = DIM_LIGHT(raw_top,    level_table_1, true, 0);
        uint8_t l3 = DIM_LIGHT(raw_bottom, level_table_1, true, 0);

        emitFaceUV4(
            vx0, vy0, vz0, l0,
            vx1, vy1, vz1, l1,
            vx2, vy2, vz2, l2,
            vx3, vy3, vz3, l3,
            u0, v0, u1, v1
        );
    }

    // --------------------------------------------------------
    // 4) South face (z=z0), normal => −Z
    //    Winding: (x1,y0,z0)->(x1,y1,z0)->(x0,y1,z0)->(x0,y0,z0)
    // --------------------------------------------------------
    {
        int vx0 = x1, vy0 = y0, vz0 = z0; // bottom-left
        int vx1 = x1, vy1 = y1, vz1 = z0; // top-left
        int vx2 = x0, vy2 = y1, vz2 = z0; // top-right
        int vx3 = x0, vy3 = y0, vz3 = z0; // bottom-right

        uint8_t l0 = DIM_LIGHT(raw_bottom, level_table_1, true, 0);
        uint8_t l1 = DIM_LIGHT(raw_top,    level_table_1, true, 0);
        uint8_t l2 = DIM_LIGHT(raw_top,    level_table_1, true, 0);
        uint8_t l3 = DIM_LIGHT(raw_bottom, level_table_1, true, 0);

        emitFaceUV4(
            vx0, vy0, vz0, l0,
            vx1, vy1, vz1, l1,
            vx2, vy2, vz2, l2,
            vx3, vy3, vz3, l3,
            u0, v0, u1, v1
        );
    }

    // --------------------------------------------------------
    // 5) West face (x=x0), normal => −X
    //    Winding: (x0,y0,z0)->(x0,y1,z0)->(x0,y1,z1)->(x0,y0,z1)
    // --------------------------------------------------------
    {
        int vx0 = x0, vy0 = y0, vz0 = z0; // bottom-left
        int vx1 = x0, vy1 = y1, vz1 = z0; // top-left
        int vx2 = x0, vy2 = y1, vz2 = z1; // top-right
        int vx3 = x0, vy3 = y0, vz3 = z1; // bottom-right

        uint8_t l0 = DIM_LIGHT(raw_bottom, level_table_2, true, 0);
        uint8_t l1 = DIM_LIGHT(raw_top,    level_table_2, true, 0);
        uint8_t l2 = DIM_LIGHT(raw_top,    level_table_2, true, 0);
        uint8_t l3 = DIM_LIGHT(raw_bottom, level_table_2, true, 0);

        emitFaceUV4(
            vx0, vy0, vz0, l0,
            vx1, vy1, vz1, l1,
            vx2, vy2, vz2, l2,
            vx3, vy3, vz3, l3,
            u0, v0, u1, v1
        );
    }

    // --------------------------------------------------------
    // 6) East face (x=x1), normal => +X
    //    Winding: (x1,y0,z1)->(x1,y1,z1)->(x1,y1,z0)->(x1,y0,z0)
    // --------------------------------------------------------
    {
        int vx0 = x1, vy0 = y0, vz0 = z1; // bottom-left
        int vx1 = x1, vy1 = y1, vz1 = z1; // top-left
        int vx2 = x1, vy2 = y1, vz2 = z0; // top-right
        int vx3 = x1, vy3 = y0, vz3 = z0; // bottom-right

        uint8_t l0 = DIM_LIGHT(raw_bottom, level_table_2, true, 0);
        uint8_t l1 = DIM_LIGHT(raw_top,    level_table_2, true, 0);
        uint8_t l2 = DIM_LIGHT(raw_top,    level_table_2, true, 0);
        uint8_t l3 = DIM_LIGHT(raw_bottom, level_table_2, true, 0);

        emitFaceUV4(
            vx0, vy0, vz0, l0,
            vx1, vy1, vz1, l1,
            vx2, vy2, vz2, l2,
            vx3, vy3, vz3, l3,
            u0, v0, u1, v1
        );
    }
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

void render_minecart(mat4 view, bool fullbright) {
    assert(view);

    // Per-vertex lighting is handled inside emitCubeWithSideUV,
    // so no single L_outer or similar is needed here.

    /* Build modelview: translate up and rotate by 90° around Y */
    mat4 model, mv;
    glm_mat4_identity(model);
    glm_translate_make(model, (vec3){0.0F, 0.5F + 1.0F/32.0F, 0.0F});
    glm_mat4_mul(view, model, mv);
    glm_rotate_y(mv, GLM_PI_2, mv);
    gfx_matrix_modelview(mv);

    // Bind the minecart texture (atlas) and reset the displaylist
    gfx_bind_texture(&texture_minecart);
    displaylist_reset(&dl);

    /*
       Draw a single cube of size 1×1×1 block.
       Since 1 block = 256 internal units, we set S = 256 at origin (0,0,0).
       Per-vertex lighting is applied inside emitCubeWithSideUV.
    */
    const int S = 256;  // internal units for 1 block
    emitCubeWithSideUV(
        0,              // x0
        0,              // y0
        0,              // z0
        2 * S,       	 // Sx = 2 blocks wide
        1 * S	,        // Sy = 1 block tall
        128              // Sz = 0.5 block deep
    );

    // Flush: 6 faces × 4 vertices = 24
    gfx_lighting(true);
    displaylist_render_immediate(&dl, 24);
    gfx_lighting(false);

    // Reset modelview to identity
    gfx_matrix_modelview(GLM_MAT4_IDENTITY);
}
