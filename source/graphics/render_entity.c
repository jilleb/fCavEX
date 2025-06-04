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

/*
    DIM_LIGHT: produces the final 8-bit brightness from raw (0–255),
    using the chosen level_table, shade_sides=true, luminance=0.
*/
static inline uint8_t DIM_LIGHT(uint8_t l, uint8_t* table, bool shade_sides,
                                uint8_t luminance) {
    // Local MAX_U8 to compare high 4 bits against luminance
    inline uint8_t MAX_U8_LOCAL(uint8_t a, uint8_t b) {
        return a > b ? a : b;
    }
    if (shade_sides) {
        return (table[MAX_U8_LOCAL(l >> 4, luminance)] << 4)
             | table[l & 0x0F];
    } else {
        return (MAX_U8_LOCAL(l >> 4, luminance) << 4)
             | (l & 0x0F);
    }
}



// Define a helper struct for one face’s UV rectangle
typedef struct {
    uint8_t u, v, w, h;
} UVRect;

/*
    Helper function to compute the four UV corners (in order BL, BR, TR, TL)
    and then rotate them by one of {0, 90, 180, -90} degrees.

    Input:
      u0, v0, u1, v1 : original rectangle corners in texture space
      rotDegrees     : must be one of {0, 90, 180, -90}

    Output:
      outUVs[8] = { uBL, vBL, uBR, vBR, uTR, vTR, uTL, vTL }
      after applying the specified rotation.
*/
static void render_entity_fill_rotated_UVs(
    uint8_t u0, uint8_t v0,
    uint8_t u1, uint8_t v1,
    int rotDegrees,
    uint8_t outUVs[8]
) {
    // Original corners in this order:
    //   0 = bottom-left  (u0, v0)
    //   1 = bottom-right (u1, v0)
    //   2 = top-right    (u1, v1)
    //   3 = top-left     (u0, v1)
    uint8_t uvCorners[4][2] = {
        { u0, v0 },  // 0
        { u1, v0 },  // 1
        { u1, v1 },  // 2
        { u0, v1 }   // 3
    };

    int rotIdx;
    switch (rotDegrees) {
        case   0: rotIdx = 0; break;
        case  90: rotIdx = 1; break;
        case 180: rotIdx = 2; break;
        case -90: rotIdx = 3; break;
        default:  rotIdx = 0; break;  // fallback to no rotation if invalid
    }

    // After rotation, the new order of corners (BL, BR, TR, TL) is:
    //   newIndex(i) = (i + rotIdx) % 4
    for (int i = 0; i < 4; i++) {
        int src = (i + rotIdx) & 3;  // mod 4
        outUVs[i * 2 + 0] = uvCorners[src][0];
        outUVs[i * 2 + 1] = uvCorners[src][1];
    }
}

/*
    emitCubeWithSideUVRot:
    ======================
    Draws an axis-aligned box from (x0,y0,z0) to (x0+Sx, y0+Sy, z0+Sz),
    using per-vertex lighting from vertex_light[0], and sampling six independent
    UV rectangles—one per face—in this order:

      0 = bottom, 1 = top, 2 = north (+Z), 3 = south (−Z),
      4 = west  (−X), 5 = east  (+X)

    Additionally, each face i can rotate its UVs by faceRotations[i] degrees,
    where faceRotations[i] ∈ {0, 90, 180, -90}.

    faceUVs:       array of 6 UVRect structs (u, v, w, h), one per face.
    faceRotations: array of 6 ints, each 0/90/180/−90.
*/
static void render_entity_create_cube(
    int x0_px, int y0_px, int z0_px, // position in px
    int sx_px, int sy_px, int sz_px, // dimensons in px
    const UVRect faceUVs[6], //  u, v, w, h per face
    const int faceRotations[6] // rotate texture mapping per face, 0, 90, -90, 180 degrees.
) {
    int x0 = x0_px * 16;
    int y0 = y0_px * 16;
    int z0 = z0_px * 16;
    int x1 = x0 + sx_px * 16;
    int y1 = y0 + sy_px * 16;
    int z1 = z0 + sz_px * 16;

    // sw, sh are scale factors from UVRect units to actual texture‐space units
    const int sw = 256 / 64;
    const int sh = 256 / 32;

    // Precompute raw UV corners for each face (u0,v0 → u1,v1):
    uint8_t ub0 = faceUVs[0].u * sw;
    uint8_t vb0 = faceUVs[0].v * sh;
    uint8_t ub1 = faceUVs[0].u * sw + faceUVs[0].w * sw;
    uint8_t vb1 = faceUVs[0].v * sh + faceUVs[0].h * sh;

    uint8_t ut0 = faceUVs[1].u * sw;
    uint8_t vt0 = faceUVs[1].v * sh;
    uint8_t ut1 = faceUVs[1].u * sw + faceUVs[1].w * sw;
    uint8_t vt1 = faceUVs[1].v * sh + faceUVs[1].h * sh;

    uint8_t un0 = faceUVs[2].u * sw;
    uint8_t vn0 = faceUVs[2].v * sh;
    uint8_t un1 = faceUVs[2].u * sw + faceUVs[2].w * sw;
    uint8_t vn1 = faceUVs[2].v * sh + faceUVs[2].h * sh;

    uint8_t us0 = faceUVs[3].u * sw;
    uint8_t vs0 = faceUVs[3].v * sh;
    uint8_t us1 = faceUVs[3].u * sw + faceUVs[3].w * sw;
    uint8_t vs1 = faceUVs[3].v * sh + faceUVs[3].h * sh;

    uint8_t uw0 = faceUVs[4].u * sw;
    uint8_t vw0 = faceUVs[4].v * sh;
    uint8_t uw1 = faceUVs[4].u * sw + faceUVs[4].w * sw;
    uint8_t vw1 = faceUVs[4].v * sh + faceUVs[4].h * sh;

    uint8_t ue0 = faceUVs[5].u * sw;
    uint8_t ve0 = faceUVs[5].v * sh;
    uint8_t ue1 = faceUVs[5].u * sw + faceUVs[5].w * sw;
    uint8_t ve1 = faceUVs[5].v * sh + faceUVs[5].h * sh;

    // Compute base lighting values (same as original emitCubeWithSideUV):
    uint8_t base_raw   = vertex_light[0];
    uint8_t raw_top    = base_raw;
    uint8_t raw_bottom = (base_raw > 16 ? base_raw - 16 : 0);

    // Each face will need an array of 8 bytes for rotated UVs:
    uint8_t uv4[8];

    // ---- Bottom face (Y−, index 0) ----
    {
        int vx0 = x1, vy0 = y0, vz0 = z0;
        int vx1 = x0, vy1 = y0, vz1 = z0;
        int vx2 = x0, vy2 = y0, vz2 = z1;
        int vx3 = x1, vy3 = y0, vz3 = z1;
        uint8_t l0 = DIM_LIGHT(base_raw, level_table_0, true, 0);

        render_entity_fill_rotated_UVs(ub0, vb0, ub1, vb1, faceRotations[0], uv4);
        // Vertex 0 (BL)
        displaylist_pos(&dl, vx0, vy0, vz0);
        displaylist_color(&dl, l0);
        displaylist_texcoord(&dl, uv4[0], uv4[1]);
        // Vertex 1 (BR)
        displaylist_pos(&dl, vx1, vy1, vz1);
        displaylist_color(&dl, l0);
        displaylist_texcoord(&dl, uv4[2], uv4[3]);
        // Vertex 2 (TR)
        displaylist_pos(&dl, vx2, vy2, vz2);
        displaylist_color(&dl, l0);
        displaylist_texcoord(&dl, uv4[4], uv4[5]);
        // Vertex 3 (TL)
        displaylist_pos(&dl, vx3, vy3, vz3);
        displaylist_color(&dl, l0);
        displaylist_texcoord(&dl, uv4[6], uv4[7]);
    }

    // ---- Top face (Y+, index 1) ----
    {
        int vx0 = x1, vy0 = y1, vz0 = z1;
        int vx1 = x0, vy1 = y1, vz1 = z1;
        int vx2 = x0, vy2 = y1, vz2 = z0;
        int vx3 = x1, vy3 = y1, vz3 = z0;
        uint8_t l0 = DIM_LIGHT(base_raw, level_table_0, true, 0);

        render_entity_fill_rotated_UVs(ut0, vt0, ut1, vt1, faceRotations[1], uv4);
        displaylist_pos(&dl, vx0, vy0, vz0);
        displaylist_color(&dl, l0);
        displaylist_texcoord(&dl, uv4[0], uv4[1]);
        displaylist_pos(&dl, vx1, vy1, vz1);
        displaylist_color(&dl, l0);
        displaylist_texcoord(&dl, uv4[2], uv4[3]);
        displaylist_pos(&dl, vx2, vy2, vz2);
        displaylist_color(&dl, l0);
        displaylist_texcoord(&dl, uv4[4], uv4[5]);
        displaylist_pos(&dl, vx3, vy3, vz3);
        displaylist_color(&dl, l0);
        displaylist_texcoord(&dl, uv4[6], uv4[7]);
    }

    // ---- North face (+Z, index 2) ----
    {
        int vx0 = x0,  vy0 = y0, vz0 = z1;
        int vx1 = x0,  vy1 = y1, vz1 = z1;
        int vx2 = x1,  vy2 = y1, vz2 = z1;
        int vx3 = x1,  vy3 = y0, vz3 = z1;
        uint8_t l0 = DIM_LIGHT(raw_bottom, level_table_1, true, 0);
        uint8_t l1 = DIM_LIGHT(raw_top,    level_table_1, true, 0);

        render_entity_fill_rotated_UVs(un0, vn0, un1, vn1, faceRotations[2], uv4);
        displaylist_pos(&dl, vx0, vy0, vz0);
        displaylist_color(&dl, l0);
        displaylist_texcoord(&dl, uv4[0], uv4[1]);
        displaylist_pos(&dl, vx1, vy1, vz1);
        displaylist_color(&dl, l1);
        displaylist_texcoord(&dl, uv4[2], uv4[3]);
        displaylist_pos(&dl, vx2, vy2, vz2);
        displaylist_color(&dl, l1);
        displaylist_texcoord(&dl, uv4[4], uv4[5]);
        displaylist_pos(&dl, vx3, vy3, vz3);
        displaylist_color(&dl, l0);
        displaylist_texcoord(&dl, uv4[6], uv4[7]);
    }

    // ---- South face (−Z, index 3) ----
    {
        int vx0 = x1, vy0 = y0, vz0 = z0;
        int vx1 = x1, vy1 = y1, vz1 = z0;
        int vx2 = x0, vy2 = y1, vz2 = z0;
        int vx3 = x0, vy3 = y0, vz3 = z0;
        uint8_t l0 = DIM_LIGHT(raw_bottom, level_table_1, true, 0);
        uint8_t l1 = DIM_LIGHT(raw_top,    level_table_1, true, 0);

        render_entity_fill_rotated_UVs(us0, vs0, us1, vs1, faceRotations[3], uv4);
        displaylist_pos(&dl, vx0, vy0, vz0);
        displaylist_color(&dl, l0);
        displaylist_texcoord(&dl, uv4[0], uv4[1]);
        displaylist_pos(&dl, vx1, vy1, vz1);
        displaylist_color(&dl, l1);
        displaylist_texcoord(&dl, uv4[2], uv4[3]);
        displaylist_pos(&dl, vx2, vy2, vz2);
        displaylist_color(&dl, l1);
        displaylist_texcoord(&dl, uv4[4], uv4[5]);
        displaylist_pos(&dl, vx3, vy3, vz3);
        displaylist_color(&dl, l0);
        displaylist_texcoord(&dl, uv4[6], uv4[7]);
    }

    // ---- West face (−X, index 4) ----
    {
        int vx0 = x0, vy0 = y0, vz0 = z0;
        int vx1 = x0, vy1 = y1, vz1 = z0;
        int vx2 = x0, vy2 = y1, vz2 = z1;
        int vx3 = x0, vy3 = y0, vz3 = z1;
        uint8_t l0 = DIM_LIGHT(raw_bottom, level_table_2, true, 0);
        uint8_t l1 = DIM_LIGHT(raw_top,    level_table_2, true, 0);

        render_entity_fill_rotated_UVs(uw0, vw0, uw1, vw1, faceRotations[4], uv4);
        displaylist_pos(&dl, vx0, vy0, vz0);
        displaylist_color(&dl, l0);
        displaylist_texcoord(&dl, uv4[0], uv4[1]);
        displaylist_pos(&dl, vx1, vy1, vz1);
        displaylist_color(&dl, l1);
        displaylist_texcoord(&dl, uv4[2], uv4[3]);
        displaylist_pos(&dl, vx2, vy2, vz2);
        displaylist_color(&dl, l1);
        displaylist_texcoord(&dl, uv4[4], uv4[5]);
        displaylist_pos(&dl, vx3, vy3, vz3);
        displaylist_color(&dl, l0);
        displaylist_texcoord(&dl, uv4[6], uv4[7]);
    }

    // ---- East face (+X, index 5) ----
    {
        int vx0 = x1, vy0 = y0, vz0 = z1;
        int vx1 = x1, vy1 = y1, vz1 = z1;
        int vx2 = x1, vy2 = y1, vz2 = z0;
        int vx3 = x1, vy3 = y0, vz3 = z0;
        uint8_t l0 = DIM_LIGHT(raw_bottom, level_table_2, true, 0);
        uint8_t l1 = DIM_LIGHT(raw_top,    level_table_2, true, 0);

        render_entity_fill_rotated_UVs(ue0, ve0, ue1, ve1, faceRotations[5], uv4);
        displaylist_pos(&dl, vx0, vy0, vz0);
        displaylist_color(&dl, l0);
        displaylist_texcoord(&dl, uv4[0], uv4[1]);
        displaylist_pos(&dl, vx1, vy1, vz1);
        displaylist_color(&dl, l1);
        displaylist_texcoord(&dl, uv4[2], uv4[3]);
        displaylist_pos(&dl, vx2, vy2, vz2);
        displaylist_color(&dl, l1);
        displaylist_texcoord(&dl, uv4[4], uv4[5]);
        displaylist_pos(&dl, vx3, vy3, vz3);
        displaylist_color(&dl, l0);
        displaylist_texcoord(&dl, uv4[6], uv4[7]);
    }
}

void render_entity_update_light(uint8_t light) {
    memset(vertex_light,     light,  sizeof(vertex_light));
    memset(vertex_light_inv, ~light, sizeof(vertex_light_inv));
}


static inline uint8_t MAX_U8(uint8_t a, uint8_t b) {
    return a > b ? a : b;
}



// minecart specific:
void render_entity_minecart_init(void) {
    displaylist_init(&dl, 320, 64);
    memset(vertex_light,     0x0F, sizeof(vertex_light));
    memset(vertex_light_inv, 0xFF, sizeof(vertex_light_inv));
}

void render_entity_minecart(mat4 view, bool fullbright) {
    assert(view);

    mat4 model, mv;
    glm_mat4_identity(model);
    glm_translate_make(model, (vec3){-0.5F, 0.0F,-0.5F });
    glm_mat4_mul(view, model, mv);
   // glm_rotate_y(mv, GLM_PI_2, mv);
    gfx_matrix_modelview(mv);

    gfx_bind_texture(&texture_minecart); // 64×32 PNG direct gebonden
    displaylist_reset(&dl);


    //setup the texture coordinates for each face

    UVRect bottomSideUV[6] = {
        {  2,  12, 20, 16 }, // bottom face
		{  25, 13, 18, 14 }, // top face
        {  0,  12, 2, 16 }, // north face (+Z)
        {  22, 12, 2, 16 }, // south face (−Z)
        {  2,  10, 20, 2 }, // west face  (−X)
        {  22, 10, 20, 2 }  // east face  (+X)
    };
    int bottomSideDirection[6] = {-90,0,0,0,-90,-90};

    UVRect rightSideUV[6] = {	// is actually back
        {  2, 0, 16, 2 },
        {  2, 0, 16, 2 },
        {  2, 2, 16, 8 },
        { 20, 2, 16, 8 },
        { 18, 2, 2,  8 },
        {  0, 2, 2,  8 }
    };
    int rightSideDirection[6] = {-90,180,-90,-90,90,90};

    UVRect leftSideUV[6] = {		// is actually front
            {  2, 0, 16, 2 },
            {  2, 0, 16, 2 },
            { 20, 2, 16, 8 },
            {  2, 2, 16, 8 },
            {  0, 2, 2,  8 },
			{ 18, 2, 2,  8 }
    };
    int leftSideDirection[6] = {-90,0,-90,-90,90,90};

    UVRect backSideUV[6] = {		// is actually right // kant van de fakkels
            {  2, 0, 16, 2 },
            {  2, 0, 16, 2 },
            { 18, 2, 2,  8 },
            {  0, 2, 2,  8 },
            { 20, 2, 16, 8 },
            {  2, 2, 16, 8 }

    };
    int backSideDirection[6] = {90,-90,0,0,-90,-90};


    UVRect frontSideUV[6] = {		// is actually left
            {  2, 0, 16, 2 },
            {  2, 0, 16, 2 },
            { 18, 2, 2,  8 },
            {  0, 2, 2,  8 },
            {  2, 2, 16, 8 },
            { 20, 2, 16, 8 },

    };
    int frontSideDirection[6] =  {90,90,0,0,-90,-90};





// generate the cubes
    render_entity_create_cube(
        0, 0, 0,
        16, 2, 20,
		bottomSideUV,
		bottomSideDirection
    );

    render_entity_create_cube(
		0, 2, 0,
       16, 8, 2,
	    rightSideUV,
		rightSideDirection
    );

    render_entity_create_cube(
        0, 2, 18,
		16, 8, 2,
        leftSideUV,
		leftSideDirection
    );

    render_entity_create_cube(
        0, 2, 2,
        2, 8, 16,
		backSideUV,
		backSideDirection
    );

    render_entity_create_cube(
        14, 2, 2,
        2, 8, 16,
		frontSideUV,
		frontSideDirection
    );

    gfx_lighting(true);
    displaylist_render_immediate(&dl, 5 * 24);
    gfx_lighting(false);
    gfx_matrix_modelview(GLM_MAT4_IDENTITY);
}


