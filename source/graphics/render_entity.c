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
/* entity.c
   -----------------
   Draws the entities as a simple 3D cubes instead of a billboard.
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

static inline uint8_t DIM_LIGHT(uint8_t l, uint8_t* table, bool shade_sides, uint8_t luminance) {
    inline uint8_t MAX_U8_LOCAL(uint8_t a, uint8_t b) { return a > b ? a : b; }
    if (shade_sides) {
        return (table[MAX_U8_LOCAL(l >> 4, luminance)] << 4) | table[l & 0x0F];
    } else {
        return (MAX_U8_LOCAL(l >> 4, luminance) << 4) | (l & 0x0F);
    }
}

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
    render_entity_create_cube:
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
    int x0_px, int y0_px, int z0_px,
    int sx_px, int sy_px, int sz_px,
    const UVRect faceUVs[6],
    const int faceRotations[6]
) {
    int x0 = x0_px * 16;
    int y0 = y0_px * 16;
    int z0 = z0_px * 16;
    int x1 = x0 + sx_px * 16;
    int y1 = y0 + sy_px * 16;
    int z1 = z0 + sz_px * 16;
    const int sw = 256 / 64;
    const int sh = 256 / 32;
    const int UV_MAX = 255;

    // bottom face UV
    uint8_t ub0 = faceUVs[0].u * sw;
    uint8_t vb0 = faceUVs[0].v * sh;
    int    ub1_i = faceUVs[0].u * sw + faceUVs[0].w * sw;
    int    vb1_i = faceUVs[0].v * sh + faceUVs[0].h * sh;
    if (ub1_i > UV_MAX) ub1_i = UV_MAX;
    if (vb1_i > UV_MAX) vb1_i = UV_MAX;
    uint8_t ub1 = (uint8_t)ub1_i;
    uint8_t vb1 = (uint8_t)vb1_i;

    // top face UV
    uint8_t ut0 = faceUVs[1].u * sw;
    uint8_t vt0 = faceUVs[1].v * sh;
    int    ut1_i = faceUVs[1].u * sw + faceUVs[1].w * sw;
    int    vt1_i = faceUVs[1].v * sh + faceUVs[1].h * sh;
    if (ut1_i > UV_MAX) ut1_i = UV_MAX;
    if (vt1_i > UV_MAX) vt1_i = UV_MAX;
    uint8_t ut1 = (uint8_t)ut1_i;
    uint8_t vt1 = (uint8_t)vt1_i;

    // north face UV
    uint8_t un0 = faceUVs[2].u * sw;
    uint8_t vn0 = faceUVs[2].v * sh;
    int    un1_i = faceUVs[2].u * sw + faceUVs[2].w * sw;
    int    vn1_i = faceUVs[2].v * sh + faceUVs[2].h * sh;
    if (un1_i > UV_MAX) un1_i = UV_MAX;
    if (vn1_i > UV_MAX) vn1_i = UV_MAX;
    uint8_t un1 = (uint8_t)un1_i;
    uint8_t vn1 = (uint8_t)vn1_i;

    // south face UV
    uint8_t us0 = faceUVs[3].u * sw;
    uint8_t vs0 = faceUVs[3].v * sh;
    int    us1_i = faceUVs[3].u * sw + faceUVs[3].w * sw;
    int    vs1_i = faceUVs[3].v * sh + faceUVs[3].h * sh;
    if (us1_i > UV_MAX) us1_i = UV_MAX;
    if (vs1_i > UV_MAX) vs1_i = UV_MAX;
    uint8_t us1 = (uint8_t)us1_i;
    uint8_t vs1 = (uint8_t)vs1_i;

    // west face UV
    uint8_t uw0 = faceUVs[4].u * sw;
    uint8_t vw0 = faceUVs[4].v * sh;
    int    uw1_i = faceUVs[4].u * sw + faceUVs[4].w * sw;
    int    vw1_i = faceUVs[4].v * sh + faceUVs[4].h * sh;
    if (uw1_i > UV_MAX) uw1_i = UV_MAX;
    if (vw1_i > UV_MAX) vw1_i = UV_MAX;
    uint8_t uw1 = (uint8_t)uw1_i;
    uint8_t vw1 = (uint8_t)vw1_i;

    // east face UV
    uint8_t ue0 = faceUVs[5].u * sw;
    uint8_t ve0 = faceUVs[5].v * sh;
    int    ue1_i = faceUVs[5].u * sw + faceUVs[5].w * sw;
    int    ve1_i = faceUVs[5].v * sh + faceUVs[5].h * sh;
    if (ue1_i > UV_MAX) ue1_i = UV_MAX;
    if (ve1_i > UV_MAX) ve1_i = UV_MAX;
    uint8_t ue1 = (uint8_t)ue1_i;
    uint8_t ve1 = (uint8_t)ve1_i;

    uint8_t uniform_light = vertex_light[0];
    uint8_t l0, l1;
    uint8_t uv4[8];

    {
        int vx0 = x1, vy0 = y0, vz0 = z0;
        int vx1 = x0, vy1 = y0, vz1 = z0;
        int vx2 = x0, vy2 = y0, vz2 = z1;
        int vx3 = x1, vy3 = y0, vz3 = z1;
        l0 = uniform_light;
        render_entity_fill_rotated_UVs(ub0, vb0, ub1, vb1, faceRotations[0], uv4);
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

    {
        int vx0 = x1, vy0 = y1, vz0 = z1;
        int vx1 = x0, vy1 = y1, vz1 = z1;
        int vx2 = x0, vy2 = y1, vz2 = z0;
        int vx3 = x1, vy3 = y1, vz3 = z0;
        l0 = uniform_light;
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

    {
        int vx0 = x0, vy0 = y0, vz0 = z1;
        int vx1 = x0, vy1 = y1, vz1 = z1;
        int vx2 = x1, vy2 = y1, vz2 = z1;
        int vx3 = x1, vy3 = y0, vz3 = z1;
        l0 = uniform_light;
        l1 = uniform_light;
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

    {
        int vx0 = x1, vy0 = y0, vz0 = z0;
        int vx1 = x1, vy1 = y1, vz1 = z0;
        int vx2 = x0, vy2 = y1, vz2 = z0;
        int vx3 = x0, vy3 = y0, vz3 = z0;
        l0 = uniform_light;
        l1 = uniform_light;
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

    {
        int vx0 = x0, vy0 = y0, vz0 = z0;
        int vx1 = x0, vy1 = y1, vz1 = z0;
        int vx2 = x0, vy2 = y1, vz2 = z1;
        int vx3 = x0, vy3 = y0, vz3 = z1;
        l0 = uniform_light;
        l1 = uniform_light;
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

    {
        int vx0 = x1, vy0 = y0, vz0 = z1;
        int vx1 = x1, vy1 = y1, vz1 = z1;
        int vx2 = x1, vy2 = y1, vz2 = z0;
        int vx3 = x1, vy3 = y0, vz3 = z0;
        l0 = uniform_light;
        l1 = uniform_light;
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

// minecart
void render_entity_minecart_init(void) {
    displaylist_init(&dl, 320, 64);
    memset(vertex_light,     0x0F, sizeof(vertex_light));
    memset(vertex_light_inv, 0xFF, sizeof(vertex_light_inv));
}

void render_entity_minecart(mat4 view) {
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

// creeper
// todo: leg movement.
void render_entity_creeper(mat4 view, float headYawDeg) {
    assert(view);

    mat4 model, body_mv;
    glm_mat4_identity(model);
    glm_translate_make(model, (vec3){-0.5f, 0.0f, -0.5f});
    glm_mat4_mul(view, model, body_mv);

    gfx_bind_texture(&texture_creeper);

    // head
    displaylist_reset(&dl);
    {
        mat4 head_model;
        glm_mat4_identity(head_model);
        glm_translate_make(head_model, (vec3){-0.5f, 0.0f, -0.5f});

        vec3 pivot = {
            (4.0f + 8.0f/2.0f) / 16.0f,  // x
            (18.0f + 8.0f/2.0f) / 16.0f, // y
            (4.0f + 8.0f/2.0f) / 16.0f   // z
        };
        glm_translate(head_model, pivot);
        glm_rotate_y(head_model, glm_rad(headYawDeg), head_model);
        vec3 invPivot = {-pivot[0], -pivot[1], -pivot[2]};
        glm_translate(head_model, invPivot);

        mat4 head_mv;
        glm_mat4_mul(view, head_model, head_mv);
        gfx_matrix_modelview(head_mv);
    }

	static const UVRect creeperHeadUVs[6] = {
		{16,  0, 8, 8}, {8,   0, 8, 8},
		{0,   8, 8, 8}, {8,   8, 8, 8},
		{16,  8, 8, 8}, {24,  8, 8, 8}
	};
	int creeperHeadDirection[6] = {-90, -90, -90, -90, -90, -90};

	render_entity_create_cube(
		4, 18, 4,  // x0,y0,z0 in “px”
		8,  8,  8, // sx,sy,sz
		creeperHeadUVs,
		creeperHeadDirection
	);
	displaylist_render_immediate(&dl,24);

	// body
    displaylist_reset(&dl);
    gfx_matrix_modelview(body_mv);

    const int bwidth = 8;
    const int bheight = 12; //12
    const int bdepth = 4;

    static const UVRect creeperBodyUVs[6] = {
    		{   20, 16,  bwidth, bdepth },  // bottom
    		{   28,  16, bwidth, bdepth },   // top
    		{   32, 20,  bwidth, bheight },  // back
    		{   20,  20, bwidth, bheight },  // front
    		{   28,  20, bdepth, bheight },  // left
    		{   16,  20, bdepth, bheight }   // right
    };
    int creeperBodyDirection[6] =  {0,0,-90,-90,-90,-90};
    render_entity_create_cube(
        4, 6, 6,    // x0=4, y0=6,  z0=6
        8, 12, 4,   // sx=8, sy=12, sz=4
		creeperBodyUVs,
		creeperBodyDirection
    );

const int width = 4;
const int height = 6;
const int depth = 4;
// legs
    static const UVRect creeperLegUVs[6] = {
		{   8, 16,  width, depth }, // bottom
		{   4,  16, width, depth },   // top
		{   12, 20, width, height },  // back
		{   4,  20, width, height },  // front
		{   8,  20, width, height },  // left
		{   0,  20, width, height }   // right
    };

    static const int creeperLegDirection[6] = {
    		0,0,-90,-90,-90,-90
    };


    int legPositions[4][3] = {
        { 4, 0,  2 },
        { 8, 0,  2 },
        { 4, 0, 10 },
        { 8, 0, 10 }
    };

    for (int i = 0; i < 4; i++) {
        render_entity_create_cube(
            legPositions[i][0], legPositions[i][1], legPositions[i][2],
			width,          height,          depth,
            creeperLegUVs,     creeperLegDirection
        );
    }
//
//    //back left leg
//    render_entity_create_cube(4, 0, 2, 4, 6, 4, creeperLegUVs, creeperLegDirection);
//
//    // back right leg
//    render_entity_create_cube(8, 0, 2, 4, 6, 4, creeperLegUVs, creeperLegDirection);
//
//    //Front-left leg
//    render_entity_create_cube(4, 0, 10, 4, 6, 4, creeperLegUVs, creeperLegDirection);
//
//    // front-right leg
//    render_entity_create_cube(8, 0, 10, 4, 6, 4, creeperLegUVs, creeperLegDirection);

    displaylist_render_immediate(&dl, 5 * 24);
    gfx_lighting(true);
    gfx_matrix_modelview(GLM_MAT4_IDENTITY);
}


