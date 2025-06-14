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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef PLATFORM_WII
#include <fat.h>
#include <gccore.h> 
#endif

#include "chunk_mesher.h"
#include "daytime.h"
#include "game/game_state.h"
#include "game/gui/screen.h"
#include "graphics/gfx_util.h"
#include "graphics/gui_util.h"
#include "graphics/gfx_settings.h"
#include "graphics/render_entity.h"
#include "item/recipe.h"
#include "network/client_interface.h"
#include "network/server_interface.h"
#include "network/server_local.h"
#include "particle.h"
#include "platform/gfx.h"
#include "platform/input.h"
#include "world.h"

#include "cNBT/nbt.h"
#include "cglm/cglm.h"
#include "lodepng/lodepng.h"

int main(void) {
	float daytime, tick_delta;
	bool render_world;

	gstate.quit = false;
	gstate.camera = (struct camera) {
		.x = 0, .y = 0, .z = 0, .rx = 0, .ry = 0, .controller = {0, 0, 0}};
	gstate.config.fov = 70.0F;
	gstate.config.render_distance = 192.0F;
	gstate.config.fog_distance = 5 * 16.0F;
	gstate.world_loaded = false;
	gstate.held_item_animation.punch.start = time_get();
	gstate.held_item_animation.switch_item.start = time_get();
	gstate.digging.cooldown = time_get();
	gstate.digging.active = false;
	gstate.paused = false;

	rand_gen_seed(&gstate.rand_src);

#ifdef PLATFORM_WII
	fatInitDefault();
	#ifndef NDEBUG
		SYS_STDIO_Report(true);
		SYS_Report("[INIT] STDIO redirection is now active\n");
	#endif
#endif

	config_create(&gstate.config_user, "config.json");

	input_init();
	blocks_init();
	items_init();
	render_entity_init();

	recipe_init();
	gfx_setup();

	screen_set(&screen_select_world);

	world_create(&gstate.world);

	for(size_t k = 0; k < 256; k++)
		gstate.windows[k] = NULL;

	clin_init();
	svin_init();
	chunk_mesher_init();
	particle_init();

	dict_entity_init(gstate.entities);
	gstate.local_player = NULL;
	gstate.in_water = false;
	gstate.oxygen = MAX_OXYGEN;

	struct server_local server;
	server_local_create(&server);

	ptime_t last_frame = time_get();
	ptime_t last_tick = last_frame;

	while(!gstate.quit) {
		ptime_t this_frame = time_get();
		gstate.stats.dt = time_diff_s(last_frame, this_frame);
		gstate.stats.fps = 1.0F / gstate.stats.dt;
		last_frame = this_frame;

		if(!gstate.paused) daytime
			= (float)((gstate.world_time
					 + time_diff_ms(gstate.world_time_start, this_frame)
						 / DAY_TICK_MS)
						% DAY_LENGTH_TICKS)
			/ (float)DAY_LENGTH_TICKS;

		clin_update();

		tick_delta = time_diff_s(last_tick, time_get()) / 0.05F;

		while(tick_delta >= 1.0F) {
			last_tick = time_add_ms(last_tick, 50);
			tick_delta -= 1.0F;
			if(!gstate.paused) {
				particle_update();
				entities_client_tick(gstate.entities);
			}
		}

		if(gstate.local_player)
			camera_attach(&gstate.camera, gstate.local_player, tick_delta,
							gstate.stats.dt);
		// update particle‐system with current camera pos for spawn‐culling
		particle_set_camera((vec3){
		    gstate.camera.x,
		    gstate.camera.y,
		    gstate.camera.z
		});

		render_world
			= gstate.current_screen->render_world && gstate.world_loaded;
		bool in_water_new = false;

		if(render_world) {
			struct block_data blk = world_get_block(
				&gstate.world, floorf(gstate.camera.x),
				floorf(gstate.camera.y + 0.1F), floorf(gstate.camera.z));
			in_water_new		
				= blk.type == BLOCK_WATER_FLOW || blk.type == BLOCK_WATER_STILL;
			#ifndef GFX_FANCY_LIQUIDS
			if(gstate.in_water != in_water_new) {
			#endif
				gstate.in_water = in_water_new;	
			#ifndef GFX_FANCY_LIQUIDS
				world_redraw_chunks(&gstate.world);
			}
			#endif
		}

		camera_update(&gstate.camera, gstate.in_water);

		if(render_world) {
			world_pre_render(&gstate.world, &gstate.camera, gstate.camera.view);

			{
				// 1) Bereken eerst de ray‐origin en direction uit de camera
				vec3 origin, dir;
				camera_get_ray(&gstate.camera, origin, dir);

				// 2) Probeer eerst een entiteit te raken binnen 4.5 eenheid
				float tHit;
				struct entity *hitE = raycast_entity(&gstate.entities,
													 origin, dir,
													 4.5f,
													 &tHit);
				if (hitE == gstate.local_player) {
				    hitE = NULL;
				}

				if (hitE) {
					gstate.camera_hit.entity_hit = true;
					gstate.camera_hit.entity_id  = hitE->id;
					gstate.camera_hit.hit = false;
				}
				else {
					gstate.camera_hit.entity_hit = false;
					gstate.camera_hit.entity_id  = 0;

					camera_ray_pick(&gstate.world,
									gstate.camera.x, gstate.camera.y, gstate.camera.z,
									gstate.camera.x + sinf(gstate.camera.rx) * sinf(gstate.camera.ry) * 4.5F,
									gstate.camera.y +            cosf(gstate.camera.ry) * 4.5F,
									gstate.camera.z + cosf(gstate.camera.rx) * sinf(gstate.camera.ry) * 4.5F,
									&gstate.camera_hit);
				}
			}
		} else {
		    world_pre_render_clear(&gstate.world);
		    gstate.camera_hit.hit        = false;
		    gstate.camera_hit.entity_hit = false;
		    gstate.camera_hit.entity_id  = 0;
		}

		world_update_lighting(&gstate.world);
		world_build_chunks(&gstate.world, CHUNK_MESHER_QLENGTH);

		if(gstate.current_screen->update)
			gstate.current_screen->update(gstate.current_screen,
											gstate.stats.dt);

		gfx_flip_buffers(&gstate.stats.dt_gpu, &gstate.stats.dt_vsync);

		if(!gstate.paused) {
			// must not modify displaylists while still rendering!
			chunk_mesher_receive();
			world_render_completed(&gstate.world, render_world);

			vec3 top_plane_color, bottom_plane_color, atmosphere_color;
			daytime_sky_colors(daytime, top_plane_color, bottom_plane_color,
								 atmosphere_color);

			if(render_world) {
				gfx_clear_buffers(atmosphere_color[0], atmosphere_color[1],
									atmosphere_color[2]);
			} else {
				gfx_clear_buffers(128, 128, 128);
			}

			gfx_fog_color(atmosphere_color[0], atmosphere_color[1],
							atmosphere_color[2]);

			gfx_mode_world();
			gfx_matrix_projection(gstate.camera.projection, true);

			if(render_world) {
				gfx_update_light(daytime_brightness(daytime),
								 world_dimension_light(&gstate.world));

				if(gstate.world.dimension == WORLD_DIM_OVERWORLD)
					gutil_sky_box(gstate.camera.view,
									daytime_celestial_angle(daytime), top_plane_color,
									bottom_plane_color);

				gstate.stats.chunks_rendered
					= world_render(&gstate.world, &gstate.camera, false);
			} else {
				gstate.stats.chunks_rendered = 0;
			}

			if(gstate.current_screen->render3D) {
				gfx_fog(false);
				gstate.current_screen->render3D(gstate.current_screen,
												gstate.camera.view);
			}

			if(render_world) {
				gfx_fog(false);
				particle_render(
					gstate.camera.view,
					(vec3) {gstate.camera.x, gstate.camera.y, gstate.camera.z},
					tick_delta);
				entities_client_render(gstate.entities, &gstate.camera, tick_delta);
				gfx_fog(true);

				#ifdef GFX_FANCY_LIQUIDS
				world_render(&gstate.world, &gstate.camera, true);
				#endif

				#ifdef GFX_CLOUDS
				if(gstate.world.dimension == WORLD_DIM_OVERWORLD)
					gutil_clouds(gstate.camera.view, daytime_brightness(daytime));
				#endif
			}

			gfx_mode_gui();

			if(gstate.in_water) {
				gfx_bind_texture(&texture_water);
				gutil_texquad_col(0, 0, -gstate.camera.rx / GLM_PI * 256,
									gstate.camera.ry / GLM_PI * 256, 512,
									512 * (float)gfx_height() / (float)gfx_width(),
									gfx_width(), gfx_height(), 0xFF, 0xFF, 0xFF,
									0x80);
			}

		}

		if(gstate.current_screen->render2D)
			gstate.current_screen->render2D(gstate.current_screen, gfx_width(),
											gfx_height());

		if(input_pressed(IB_SCREENSHOT)) {
			size_t width, height;
			gfx_copy_framebuffer(NULL, &width, &height);

			void* image = malloc(width * height * 4);

			if(image) {
				gfx_copy_framebuffer(image, &width, &height);

				char name[64];
				snprintf(name, sizeof(name), "%ld.png", (long)time(NULL));

				lodepng_encode32_file(name, image, width, height);
				free(image);
			}
		}

		input_poll();
		gfx_finish(true);
	}

	return 0;
}
