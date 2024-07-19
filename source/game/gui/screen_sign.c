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

#include "../../graphics/gfx_util.h"
#include "../../graphics/gfx_settings.h"
#include "../../graphics/gui_util.h"
#include "../../graphics/render_model.h"
#include "../../network/server_interface.h"
#include "../../platform/gfx.h"
#include "../../platform/input.h"
#include "../../platform/time.h"
#include "../game_state.h"
#include "screen.h"

#define GUI_WIDTH 176
#define GUI_HEIGHT 167

struct inv_slot {
	int x, y;
	size_t slot;
};

static bool pointer_has_item;
static bool pointer_available;
static float pointer_x, pointer_y, pointer_angle;
static struct inv_slot slots[INVENTORY_SIZE];
static size_t slots_index;
static size_t selected_slot;
static uint8_t sign_container;

void screen_sign_set_windowc(uint8_t container) {
	sign_container = container;
}

static void screen_sign_reset(struct screen* s, int width, int height) {
	input_pointer_enable(true);

	if(gstate.local_player)
		gstate.local_player->data.local_player.capture_input = false;

	s->render3D = screen_ingame.render3D;

	pointer_available = false;
	pointer_has_item = false;
}

static void screen_sign_update(struct screen* s, float dt) {
	if(input_pressed(IB_INVENTORY)) {
		svin_rpc_send(&(struct server_rpc) {
			.type = SRPC_WINDOW_CLOSE,
			.payload.window_close.window = sign_container,
		});

		screen_set(&screen_ingame);
	}

	if(input_pressed(IB_GUI_LEFT)) {
		// TODO
	}

	if(input_pressed(IB_GUI_RIGHT)) {
		// TODO
	}

	if(input_pressed(IB_GUI_UP)) {
		// TODO
	}

	if(input_pressed(IB_GUI_DOWN)) {
		// TODO
	}
}

static void screen_sign_render2D(struct screen* s, int width, int height) {
	// darken background
	gfx_texture(false);
	gutil_texquad_col(0, 0, 0, 0, 0, 0, width, height, 0, 0, 0, 180);
	gfx_texture(true);

	int off_x = (width - GUI_WIDTH * GFX_GUI_SCALE) / 2;
	int off_y = (height - GUI_HEIGHT * GFX_GUI_SCALE) / 2;

	// draw sign texture 
	gutil_text(off_x + 86 * GFX_GUI_SCALE, off_y + 16 * GFX_GUI_SCALE, "SIGN TEST", 8 * GFX_GUI_SCALE, false);

	if(pointer_available) {
		gfx_bind_texture(&texture_pointer);
		gutil_texquad_rt_any(pointer_x, pointer_y, glm_rad(pointer_angle), 0, 0,
							 256, 256, 48 * GFX_GUI_SCALE, 48 * GFX_GUI_SCALE);
	}
}

struct screen screen_sign = {
	.reset = screen_sign_reset,
	.update = screen_sign_update,
	.render2D = screen_sign_render2D,
	.render3D = NULL,
	.render_world = true,
};
