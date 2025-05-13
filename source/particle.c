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

#include "game/game_state.h"
#include "graphics/render_block.h"
#include "particle.h"
#include "platform/gfx.h"
#include "graphics/texture_atlas.h"
#include "platform/texture.h"


#define PARTICLES_AREA 8
#define PARTICLES_VOLUME 64

ARRAY_DEF(array_particle, struct particle, M_POD_OPLIST)

array_particle_t particles;

void particle_init() {
	array_particle_init(particles);
}

static float rnd(void) {
    return rand_gen_flt(&gstate.rand_src);
}

void particle_add(vec3 pos,
                  vec3 vel,
                  uint8_t tex,
                  float size,
                  float lifetime,
                  bool gravity,
				  uint8_t brightness,
                  particle_atlas_t atlas)
{
    struct particle *p = array_particle_push_new(particles);
    if (!p) return;

    // copy position & velocity
    glm_vec3_copy(pos,    p->pos);
    glm_vec3_copy(pos,    p->pos_old);
    glm_vec3_copy(vel,    p->vel);

    // store basic properties
    p->tex       = tex;
    p->size      = size;
    p->age       = (int)lifetime;   // remaining life
    p->lifetime  = (int)lifetime;   // for smoke animation
    p->gravity   = gravity;
    p->brightness = brightness;
    p->atlas     = atlas;

    // only terrain‐atlas gets a random UV offset
    if (atlas == TEXTURE_ATLAS_TERRAIN) {
        // pick a random 4×4px corner within the 256×256 terrain atlas
        float fx = (TEX_OFFSET(TEXTURE_X(tex)) + rnd() * 12.0f) / 256.0f;
        float fy = (TEX_OFFSET(TEXTURE_Y(tex)) + rnd() * 12.0f) / 256.0f;
        p->tex_uv[0] = fx;
        p->tex_uv[1] = fy;
    } else {
        // particle‐atlas uses static UVs computed from p->tex
        p->tex_uv[0] = 0.0f;
        p->tex_uv[1] = 0.0f;
    }
}

void particle_generate_block(struct block_info* info) {
    assert(info && info->block && info->neighbours);
    if (!blocks[info->block->type]) return;

    size_t count = blocks[info->block->type]->getBoundingBox(info, false, NULL);
    if (!count) return;

    struct AABB aabb[count];
    blocks[info->block->type]->getBoundingBox(info, false, aabb);

    float volume = (aabb->x2 - aabb->x1)
                 * (aabb->y2 - aabb->y1)
                 * (aabb->z2 - aabb->z1);
    uint8_t tex = blocks[info->block->type]
                  ->getTextureIndex(info, SIDE_FRONT);

    for (int k = 0; k < volume * PARTICLES_VOLUME; k++) {
        float x = rand_gen_flt(&gstate.rand_src) * (aabb->x2 - aabb->x1) + aabb->x1;
        float y = rand_gen_flt(&gstate.rand_src) * (aabb->y2 - aabb->y1) + aabb->y1;
        float z = rand_gen_flt(&gstate.rand_src) * (aabb->z2 - aabb->z1) + aabb->z1;

        vec3 vel = {
            rand_gen_flt(&gstate.rand_src) - 0.5F,
            rand_gen_flt(&gstate.rand_src) - 0.5F,
            rand_gen_flt(&gstate.rand_src) - 0.5F
        };
        glm_vec3_normalize(vel);
        glm_vec3_scale(
            vel,
            (2.0F * rand_gen_flt(&gstate.rand_src) + 0.5F) * 0.05F,
            vel
        );

        vec3 pos = { info->x + x, info->y + y, info->z + z };
        float size = (rand_gen_flt(&gstate.rand_src) + 1.0F) * 0.03125F;
        float age  = 4.0F / (rand_gen_flt(&gstate.rand_src) * 0.9F + 0.1F);

        particle_add(pos, vel, tex, size, age, true, 255, TEXTURE_ATLAS_TERRAIN);
    }
}

void particle_generate_side(struct block_info* info, enum side s) {
    assert(info && info->block && info->neighbours);
    if (!blocks[info->block->type]) return;

    size_t count = blocks[info->block->type]->getBoundingBox(info, false, NULL);
    if (!count) return;

    struct AABB aabb[count];
    blocks[info->block->type]->getBoundingBox(info, false, aabb);

    float area;
    switch (s) {
        case SIDE_RIGHT:
        case SIDE_LEFT:
            area = (aabb->y2 - aabb->y1) * (aabb->z2 - aabb->z1);
            break;
        case SIDE_BOTTOM:
        case SIDE_TOP:
            area = (aabb->x2 - aabb->x1) * (aabb->z2 - aabb->z1);
            break;
        case SIDE_FRONT:
        case SIDE_BACK:
            area = (aabb->x2 - aabb->x1) * (aabb->y2 - aabb->y1);
            break;
        default:
            return;
    }

    uint8_t tex = blocks[info->block->type]
                  ->getTextureIndex(info, s);
    float offset = 0.0625F;

    for (int k = 0; k < area * PARTICLES_AREA; k++) {
        float x = rand_gen_flt(&gstate.rand_src) * (aabb->x2 - aabb->x1) + aabb->x1;
        float y = rand_gen_flt(&gstate.rand_src) * (aabb->y2 - aabb->y1) + aabb->y1;
        float z = rand_gen_flt(&gstate.rand_src) * (aabb->z2 - aabb->z1) + aabb->z1;

        switch (s) {
            case SIDE_LEFT:   x = aabb->x1 - offset; break;
            case SIDE_RIGHT:  x = aabb->x2 + offset; break;
            case SIDE_BOTTOM: y = aabb->y1 - offset; break;
            case SIDE_TOP:    y = aabb->y2 + offset; break;
            case SIDE_FRONT:  z = aabb->z1 - offset; break;
            case SIDE_BACK:   z = aabb->z2 + offset; break;
            default: return;
        }

        vec3 vel = {
            rand_gen_flt(&gstate.rand_src) - 0.5F,
            rand_gen_flt(&gstate.rand_src) - 0.5F,
            rand_gen_flt(&gstate.rand_src) - 0.5F
        };
        glm_vec3_normalize(vel);
        glm_vec3_scale(
            vel,
            (2.0F * rand_gen_flt(&gstate.rand_src) + 0.5F) * 0.05F,
            vel
        );

        vec3 pos = { info->x + x, info->y + y, info->z + z };
        float size = (rand_gen_flt(&gstate.rand_src) + 1.0F) * 0.03125F;
        float age  = 4.0F / (rand_gen_flt(&gstate.rand_src) * 0.9F + 0.1F);

        particle_add(pos, vel, tex, size, age, true, 255, TEXTURE_ATLAS_TERRAIN);
        }
}

void particle_generate_explosion_flash(vec3 center, float intensity) {
    // atlas-index for the largest smoke frame (frame 7)
    uint8_t tex_smoke_base = tex_atlas_lookup_particle(TEXAT_PARTICLE_SMOKE_7);
    // roughly 12 smoke puffs per unit intensity
    int count = (int)ceilf(intensity * 12.0f);

    for (int i = 0; i < count; i++) {
        // 1) random direction in a sphere
        vec3 vel = {
            rnd()*2.0f - 1.0f,
            rnd()*2.0f - 1.0f,
            rnd()*2.0f - 1.0f
        };
        glm_vec3_normalize(vel);
        // give them a bit of speed outward
        glm_vec3_scale(vel, (0.3f + rnd()*0.2f) * intensity, vel);

        // 2) spawn very close to the center, small jitter
        vec3 pos = {
            center[0] + (rnd()-0.5f)*0.2f,
            center[1] + (rnd()-0.5f)*0.2f,
            center[2] + (rnd()-0.5f)*0.2f
        };

        // 3) size scales with intensity
        float size = 0.2f * intensity;

        // 4) shorter, randomized lifetime: 8–16 ticks × intensity
        float life = (8.0f + rnd()*8.0f) * intensity;

        // 5) random gray shade for depth (50 = dark, 200 = light)
        uint8_t brightness = (uint8_t)(rnd() * 150.0f + 50.0f);

        // 6) add as smoke atlas particle (will animate smoke_7→smoke_0),
        //    no gravity => no damping, so they drift until age runs out
        particle_add(
            pos,
            vel,
            tex_smoke_base,            // start frame for smoke animation
            size,                      // particle size
            life,                      // lifetime in ticks
            false,                     // no gravity (no damping)
            brightness,                // random gray brightness
            TEXTURE_ATLAS_PARTICLES    // use particle atlas
        );
    }
}

void particle_generate_explosion_smoke(vec3 center, float intensity) {
    // number of smoke puffs
    int count = (int)ceilf(intensity * 12.0f);

    for (int i = 0; i < count; i++) {
        // 1) Choose a random smoke frame (0…7) for variety
        uint8_t frame = (uint8_t)(rnd() * 8.0f);
        uint8_t tex_smoke = tex_atlas_lookup_particle(TEXAT_PARTICLE_SMOKE_0 + frame);

        // 2) Random brightness between 100 (dark gray) and 255 (light gray)
        uint8_t brightness = (uint8_t)(rnd() * 155.0f + 100.0f);

        // 3) Random direction in a sphere
        vec3 vel = {
            rnd()*2.0f - 1.0f,
            rnd()*2.0f - 1.0f,
            rnd()*2.0f - 1.0f
        };
        glm_vec3_normalize(vel);
        // scale velocity so puffs stay reasonably close
        glm_vec3_scale(vel, (0.2f + rnd()*0.1f) * intensity, vel);

        // 4) Spawn very close to center, with small jitter
        vec3 pos = {
            center[0] + (rnd()-0.5f)*0.15f,
            center[1] + (rnd()-0.5f)*0.15f,
            center[2] + (rnd()-0.5f)*0.15f
        };

        // 5) Randomize lifetime between 10–20 ticks × intensity
        float life = (10.0f + rnd()*10.0f) * intensity;

        // 6) Add particle with no gravity (so no damping), random gray shade
        particle_add(
            pos,
            vel,
            tex_smoke,             // random smoke frame
            0.15f * intensity,     // size
            life,                  // lifetime
            false,                 // no gravity => no damping
            brightness,            // random gray brightness
            TEXTURE_ATLAS_PARTICLES
        );
    }
}


static void render_single(struct particle* p, vec3 camera, float delta) {
    // 1) Distance-cull at 32 units
    if (glm_vec3_distance2(p->pos, camera) > 32.0f * 32.0f)
        return;

    // 2) Interpolate position
    vec3 pos_lerp;
    glm_vec3_lerp(p->pos_old, p->pos, delta, pos_lerp);

    // 3) Build billboarding axes
    vec3 view_dir, axis_s, axis_t;
    glm_vec3_sub(pos_lerp, camera, view_dir);
    glm_vec3_crossn(view_dir, (vec3){0,1,0}, axis_s);
    glm_vec3_crossn(view_dir, axis_s, axis_t);
    glm_vec3_scale(axis_s, p->size, axis_s);
    glm_vec3_scale(axis_t, p->size, axis_t);

    //

    uint8_t brightness = p->brightness;

    // 5) Determine which atlas-index to use
    uint8_t tile = p->tex;

    // 6) Animate smoke only (frames smoke_0…smoke_7) 7→0
    if (p->atlas == TEXTURE_ATLAS_PARTICLES
     && tile >= tex_atlas_lookup_particle(TEXAT_PARTICLE_SMOKE_0)
     && tile <= tex_atlas_lookup_particle(TEXAT_PARTICLE_SMOKE_7))
    {
        float t = (float)p->age / (float)p->lifetime;  // 1.0→0.0
        int frame = (int)(t * 7.0f + 0.5f);
        tile = tex_atlas_lookup_particle(TEXAT_PARTICLE_SMOKE_0 + frame);
    }

    // 7) Compute UVs
    float u0, v0, u1, v1;
    if (p->atlas == TEXTURE_ATLAS_TERRAIN) {
        // Terrain‐atlas uses 16×16px tiles and our stored random offset
        u0 = p->tex_uv[0];
        v0 = p->tex_uv[1];
        u1 = u0 + (4.0f  / 256.0f);
        v1 = v0 + (4.0f  / 256.0f);

    } else {
        // Particle‐atlas uses 4×4px tiles at fixed grid positions
        u0 = (TEX_OFFSET(TEXTURE_X(tile)))       / 256.0f;
        v0 = (TEX_OFFSET(TEXTURE_Y(tile)))       / 256.0f;
        u1 = u0 + (16.0f / 256.0f);
        v1 = v0 + (16.0f / 256.0f);
    }

    // 8) Draw quad
    gfx_draw_quads_flt(
        4,
        (float[]){
            -axis_s[0] - axis_t[0] + pos_lerp[0],
            -axis_s[1] - axis_t[1] + pos_lerp[1],
            -axis_s[2] - axis_t[2] + pos_lerp[2],

             axis_s[0] - axis_t[0] + pos_lerp[0],
             axis_s[1] - axis_t[1] + pos_lerp[1],
             axis_s[2] - axis_t[2] + pos_lerp[2],

             axis_s[0] + axis_t[0] + pos_lerp[0],
             axis_s[1] + axis_t[1] + pos_lerp[1],
             axis_s[2] + axis_t[2] + pos_lerp[2],

            -axis_s[0] + axis_t[0] + pos_lerp[0],
            -axis_s[1] + axis_t[1] + pos_lerp[1],
            -axis_s[2] + axis_t[2] + pos_lerp[2]
        },
        (uint8_t[]){
            brightness, brightness, brightness, 255,
            brightness, brightness, brightness, 255,
            brightness, brightness, brightness, 255,
            brightness, brightness, brightness, 255
        },
        (float[]){
            u0, v0,
            u1, v0,
            u1, v1,
            u0, v1
        }
    );

}


void particle_update() {
	array_particle_it_t it;
	array_particle_it(it, particles);

	while(!array_particle_end_p(it)) {
		struct particle* p = array_particle_ref(it);

		glm_vec3_copy(p->pos, p->pos_old);

		vec3 new_pos;
		glm_vec3_add(p->pos, p->vel, new_pos);

		w_coord_t bx = floorf(new_pos[0]);
		w_coord_t by = floorf(new_pos[1]);
		w_coord_t bz = floorf(new_pos[2]);
		struct block_data in_block = world_get_block(&gstate.world, bx, by, bz);

		bool intersect = false;
		if(blocks[in_block.type]) {
			struct block_info blk = (struct block_info) {
				.block = &in_block,
				.neighbours = NULL,
				.x = bx,
				.y = by,
				.z = bz,
			};

			size_t count
				= blocks[in_block.type]->getBoundingBox(&blk, true, NULL);
			if(count > 0) {
				struct AABB aabb[count];
				blocks[in_block.type]->getBoundingBox(&blk, true, aabb);

				for(size_t k = 0; k < count; k++) {
					aabb_translate(aabb + k, bx, by, bz);
					intersect = aabb_intersection_point(aabb + k, new_pos[0],
														new_pos[1], new_pos[2]);
					if(intersect)
						break;
				}
			}
		}

		if(!intersect) {
			glm_vec3_copy(new_pos, p->pos);
		} else {
			glm_vec3_zero(p->vel);
		}

        if (p->gravity) {
            p->vel[1] -= 0.04F;
            glm_vec3_scale(p->vel, 0.98F, p->vel);
        }
		p->age--;

		if(p->age > 0) {
			array_particle_next(it);
		} else {
			array_particle_remove(particles, it);
		}
	}
}

void particle_generate_smoke(vec3 center, float intensity) {
    // spawn count: unchanged
    int count = (int)ceilf(intensity * 4.0f);
    // always start at the largest smoke-frame
    uint8_t tex_smoke_base = tex_atlas_lookup_particle(TEXAT_PARTICLE_SMOKE_7);

    for (int i = 0; i < count; i++) {
        // 1) Position jitter around center
        vec3 pos = {
            center[0] + (rnd() - 0.5f) * 0.3f,
            center[1] + 0.7f + rnd() * 0.2f,
            center[2] + (rnd() - 0.5f) * 0.3f
        };

        // 2) Upward drift + slight horizontal drift
        vec3 vel = {
            (rnd() - 0.5f) * 0.01f,
            0.1f + rnd() * 0.02f,
            (rnd() - 0.5f) * 0.01f
        };

        // 3) Shorter, randomized lifetime (10–20 ticks) × intensity
        float life = (10.0f + rnd() * 10.0f) * intensity;

        // 4) Random gray shade: brightness between 50 (dark) and 200 (light)
        uint8_t brightness = (uint8_t)(rnd() * 150.0f + 50.0f);

        // 5) Add particle without gravity (no damping), varies in gray level
        particle_add(
            pos,
            vel,
            tex_smoke_base,           // enum index for frame 7
            0.15f * intensity,        // size
            life,                     // new, shorter lifetime
            false,                    // no gravity => no damping
            brightness,               // random gray brightness
            TEXTURE_ATLAS_PARTICLES   // atlas
        );
    }
}


void particle_generate_fire(vec3 pos) {
    uint8_t tex_flame      = tex_atlas_lookup_particle(TEXAT_PARTICLE_FLAME);
    uint8_t tex_smoke_base = tex_atlas_lookup_particle(TEXAT_PARTICLE_SMOKE_7);

    // spawn 2 sparks per call
    for (int i = 0; i < 2; i++) {
        // 1) flickering flame spark
        vec3 vel_f = {
            (rnd() - 0.5f) * 0.015f,
            0.015f + rnd() * 0.01f,
            (rnd() - 0.5f) * 0.015f
        };
        float size_f = 0.04f + rnd() * 0.04f;   // size 0.04–0.08
        float life_f = 6.0f  + rnd() * 2.0f;    // lifetime 6–8 ticks

        particle_add(
            pos, vel_f,
            tex_flame,
            size_f, life_f,
            false,    // no gravity (no damping)
            255,      // full brightness for flame
            TEXTURE_ATLAS_PARTICLES
        );

        // 2) occasional smoke: about 1 in 10 sparks
        if (rnd() < 0.1f) {
            // a) smaller smoke puff
            vec3 vel_s = {
                (rnd() - 0.5f) * 0.006f,
                0.008f + rnd() * 0.005f,
                (rnd() - 0.5f) * 0.006f
            };
            vec3 spawn = {
                pos[0] + (rnd() - 0.5f) * 0.03f,
                pos[1] + (rnd() - 0.5f) * 0.03f,
                pos[2] + (rnd() - 0.5f) * 0.03f
            };
            float size_s = 0.06f + rnd() * 0.04f;   // now 0.06–0.10
            float life_s = 25.0f + rnd() * 15.0f;
            float life_s = 10.0f + rnd() * 10.0f;
            uint8_t brightness_s = (uint8_t)(rnd() * 100.0f + 50.0f);

            particle_add(
                spawn, vel_s,
                tex_smoke_base,
                size_s, life_s,
                false,          // no gravity => no damping
                brightness_s,   // varied gray brightness
                TEXTURE_ATLAS_PARTICLES
            );
        }
    }
}


// TODO: make block/side particles follow the current light again, like on original fCavex

void particle_render(mat4 view, vec3 camera, float delta) {
    gfx_matrix_modelview(view);
    gfx_lighting(false);

    // 1) Terrain/block particles
    gfx_bind_texture(&texture_terrain);
    array_particle_it_t it;
    array_particle_it(it, particles);
    while(!array_particle_end_p(it)) {
        struct particle* p = array_particle_ref(it);
        if(p->atlas == TEXTURE_ATLAS_TERRAIN)
            render_single(p, camera, delta);
        array_particle_next(it);
    }

    // 2) Particle-atlas particles (sparks, smoke, etc)
    gfx_bind_texture(&texture_particles);
    array_particle_it(it, particles);
    while(!array_particle_end_p(it)) {
        struct particle* p = array_particle_ref(it);
        if(p->atlas == TEXTURE_ATLAS_PARTICLES)
            render_single(p, camera, delta);
        array_particle_next(it);
    }

    gfx_lighting(true);
}
