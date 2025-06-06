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

#include <assert.h>

#include "../platform/gfx.h"
#include "../platform/input.h"
#include "camera.h"
#include "game_state.h"

#include "../cglm/cglm.h"

// all vectors normalized
static float plane_distance(vec3 n, vec3 p0, vec3 l0, vec3 l) {
	float d = glm_vec3_dot(n, l);

	if(d < GLM_FLT_EPSILON)
		return FLT_MAX;

	vec3 tmp;
	glm_vec3_sub(p0, l0, tmp);
	return glm_vec3_dot(tmp, n) / d;
}

void camera_ray_pick(struct world* w, float gx0, float gy0, float gz0,
					 float gx1, float gy1, float gz1,
					 struct camera_ray_result* res) {
	assert(w && res);
	int sx = gx1 > gx0 ? 1 : -1;
	int sy = gy1 > gy0 ? 1 : -1;
	int sz = gz1 > gz0 ? 1 : -1;

	int gx = floor(gx0);
	int gy = floor(gy0);
	int gz = floor(gz0);

	int x1 = floor(gx1);
	int y1 = floor(gy1);
	int z1 = floor(gz1);

	vec3 dir = {gx1 - gx0, gy1 - gy0, gz1 - gz0};
	glm_vec3_normalize(dir);

	while(1) {
		enum side s;
		// TODO:
		if(world_block_intersection(w,
									&(struct ray) {.x = gx0,
												   .y = gy0,
												   .z = gz0,
												   .dx = dir[0],
												   .dy = dir[1],
												   .dz = dir[2]},
									gx, gy, gz, &s)) {
			res->x = gx;
			res->y = gy;
			res->z = gz;
			res->hit = true;
			res->side = s;
			return;
		}

		if(gx == x1 && gy == y1 && gz == z1) {
			res->hit = false;
			return;
		}

		float t1 = plane_distance((vec3) {sx, 0, 0},
								  (vec3) {gx + (sx + 1) / 2, 0, 0},
								  (vec3) {gx0, gy0, gz0}, dir);
		float t2 = plane_distance((vec3) {0, sy, 0},
								  (vec3) {0, gy + (sy + 1) / 2, 0},
								  (vec3) {gx0, gy0, gz0}, dir);
		float t3 = plane_distance((vec3) {0, 0, sz},
								  (vec3) {0, 0, gz + (sz + 1) / 2},
								  (vec3) {gx0, gy0, gz0}, dir);

		if(t1 <= t2 && t1 <= t3) {
			gx += sx;
		} else if(t2 <= t1 && t2 <= t3) {
			gy += sy;
		} else if(t3 <= t1 && t3 <= t2) {
			gz += sz;
		}
	}
}

void camera_physics(struct camera* c, float dt) {
	assert(c);

	float jdx, jdy;
	if(input_joystick(dt, &jdx, &jdy)) {
		c->rx -= jdx * 2.0F;
		c->ry -= jdy * 2.0F;
	}

	float acc_x = 0, acc_y = 0, acc_z = 0;
	float speed_c = 40.0F;
	float air_friction = 0.05F;

	if(input_held(IB_LEFT)) {
		acc_x += cos(c->rx) * speed_c;
		acc_z -= sin(c->rx) * speed_c;
	}

	if(input_held(IB_RIGHT)) {
		acc_x -= cos(c->rx) * speed_c;
		acc_z += sin(c->rx) * speed_c;
	}

	if(input_held(IB_FORWARD)) {
		acc_x += sin(c->rx) * sin(c->ry) * speed_c;
		acc_y += cos(c->ry) * speed_c;
		acc_z += cos(c->rx) * sin(c->ry) * speed_c;
	}

	if(input_held(IB_BACKWARD)) {
		acc_x -= sin(c->rx) * sin(c->ry) * speed_c;
		acc_y -= cos(c->ry) * speed_c;
		acc_z -= cos(c->rx) * sin(c->ry) * speed_c;
	}

	if(input_held(IB_JUMP))
		acc_y += speed_c;

	if(input_held(IB_SNEAK))
		acc_y -= speed_c;

	c->controller.vx += acc_x * dt;
	c->controller.vy += acc_y * dt;
	c->controller.vz += acc_z * dt;

	c->controller.vx *= powf(air_friction, dt);
	c->controller.vy *= powf(air_friction, dt);
	c->controller.vz *= powf(air_friction, dt);

	struct AABB bbox;

	aabb_setsize_centered(&bbox, 0.6F, 0.6F, 0.6F);
	aabb_translate(&bbox, c->x + c->controller.vx * dt, c->y, c->z);
	if(!world_aabb_intersection(&gstate.world, &bbox)) {
		c->x += c->controller.vx * dt;
	} else {
		c->controller.vx = 0;
	}

	aabb_setsize_centered(&bbox, 0.6F, 0.6F, 0.6F);
	aabb_translate(&bbox, c->x, c->y + c->controller.vy * dt, c->z);
	if(!world_aabb_intersection(&gstate.world, &bbox)) {
		c->y += c->controller.vy * dt;
	} else {
		c->controller.vy = 0;
	}

	aabb_setsize_centered(&bbox, 0.6F, 0.6F, 0.6F);
	aabb_translate(&bbox, c->x, c->y, c->z + c->controller.vz * dt);
	if(!world_aabb_intersection(&gstate.world, &bbox)) {
		c->z += c->controller.vz * dt;
	} else {
		c->controller.vz = 0;
	}

	c->ry = glm_clamp(c->ry, glm_rad(0.5F), GLM_PI - glm_rad(0.5F));
}

void camera_update(struct camera* c, bool in_water) {
	assert(c);

	glm_perspective(glm_rad(gstate.config.fov)
						* (in_water ? 6.0F / 7.0F : 1.0F),
					(float)gfx_width() / (float)gfx_height(), 0.075F,
					gstate.config.render_distance, c->projection);

	glm_lookat((vec3) {c->x, c->y, c->z},
			   (vec3) {c->x + sinf(c->rx) * sinf(c->ry), c->y + cosf(c->ry),
					   c->z + cosf(c->rx) * sinf(c->ry)},
			   (vec3) {0, 1, 0}, c->view);

	mat4 view_proj;
	glm_mat4_mul(c->projection, c->view, view_proj);
	glm_frustum_planes(view_proj, c->frustum_planes);
}

void camera_attach(struct camera* c, struct entity* e, float tick_delta,
				   float dt) {
	vec3 pos_lerp;
	glm_vec3_lerp(e->pos_old, e->pos, tick_delta, pos_lerp);
	c->x = pos_lerp[0];
	c->y = pos_lerp[1];
	c->z = pos_lerp[2];

	//printf("c%.03f %.03f %.03f\n", c->x, c->y, c->z);

	float jdx, jdy;
	if(e->data.local_player.capture_input && input_joystick(dt, &jdx, &jdy)) {
		c->rx -= jdx * 2.0F;
		c->ry -= jdy * 2.0F;
	}

	c->ry = glm_clamp(c->ry, glm_rad(0.5F), GLM_PI - glm_rad(0.5F));

	e->orient[0] = c->rx;
	e->orient[1] = c->ry;
}


void camera_get_ray(const struct camera *c, vec3 origin, vec3 dir) {
    // Set origin to the camera’s current position
    origin[0] = c->x;
    origin[1] = c->y;
    origin[2] = c->z;

    // Compute an endpoint 4.5 units in front of the camera using rx/ry angles
    vec3 end;
    end[0] = c->x + sinf(c->rx) * sinf(c->ry) * 4.5f;
    end[1] = c->y +            cosf(c->ry) * 4.5f;
    end[2] = c->z + cosf(c->rx) * sinf(c->ry) * 4.5f;

    // Subtract origin from end to get the unnormalized direction
    glm_vec3_sub(end, origin, dir);

    // Normalize the direction vector to unit length
    glm_vec3_normalize(dir);
}
