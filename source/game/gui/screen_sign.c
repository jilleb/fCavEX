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
#include "../../graphics/texture_atlas.h"
#include "../../network/server_interface.h"
#include "../../platform/gfx.h"
#include "../../platform/input.h"
#include "../../platform/time.h"
#include "../game_state.h"
#include "screen.h"

#define GUI_WIDTH 128
#define GUI_HEIGHT 64

static bool pointer_has_item;
static bool pointer_available;
static float pointer_x, pointer_y, pointer_angle;
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

	selected_slot = 0;
}

static void screen_sign_update(struct screen* s, float dt) {
	if(input_pressed(IB_INVENTORY)) {
		svin_rpc_send(&(struct server_rpc) {
			.type = SRPC_WINDOW_CLOSE,
			.payload.window_close.window = sign_container,
		});

		screen_set(&screen_ingame);
	}

	// TODO: keyboard input on PC, on-screen keyboard on Wii
	if(input_pressed(IB_GUI_LEFT)) {
		selected_slot = (selected_slot - 1) & 63;
	}

	if(input_pressed(IB_GUI_RIGHT)) {
		selected_slot = (selected_slot + 1) & 63;
	}

	if(input_pressed(IB_GUI_UP)) {
		// this action decrements the character
		uint16_t action_id;
		if(windowc_new_action(gstate.windows[sign_container], &action_id,
							  false, selected_slot)) {
			svin_rpc_send(&(struct server_rpc) {
				.type = SRPC_WINDOW_CLICK,
				.payload.window_click.window = sign_container,
				.payload.window_click.action_id = action_id,
				.payload.window_click.right_click = false,
				.payload.window_click.slot = selected_slot,
			});
		}
	}

	if(input_pressed(IB_GUI_DOWN)) {
		// this action increments the character
		uint16_t action_id;
		if(windowc_new_action(gstate.windows[sign_container], &action_id,
							  true, selected_slot)) {
			svin_rpc_send(&(struct server_rpc) {
				.type = SRPC_WINDOW_CLICK,
				.payload.window_click.window = sign_container,
				.payload.window_click.action_id = action_id,
				.payload.window_click.right_click = true,
				.payload.window_click.slot = selected_slot,
			});
		}
	}
}

static void screen_sign_render2D(struct screen* s, int width, int height) {
	struct inventory* inv
		= windowc_get_latest(gstate.windows[sign_container]);

	// darken background
	gfx_texture(false);
	gutil_texquad_col(0, 0, 0, 0, 0, 0, width, height, 0, 0, 0, 180);
	gfx_texture(true);

	int off_x = (width - GUI_WIDTH * GFX_GUI_SCALE) / 2;
	int off_y = (height - GUI_HEIGHT * GFX_GUI_SCALE) / 2;

	// draw sign texture
	gfx_bind_texture(&texture_terrain);
	gutil_texquad(off_x, off_y, TEX_OFFSET(TEXTURE_X(tex_atlas_lookup(TEXAT_PLANKS))), TEX_OFFSET(TEXTURE_Y(tex_atlas_lookup(TEXAT_PLANKS))), 16, 16, GUI_WIDTH * GFX_GUI_SCALE, GUI_HEIGHT * GFX_GUI_SCALE);

	// draw text
	for(size_t k = 0; k < 64; k++) {
		char str[2];
		struct item_data chr;
		str[1] = '\0';
		if(inventory_get_slot(inv, k, &chr)) {
			str[0] = chr.count;
			gutil_text(off_x + (2 + 8 * (k % 16)) * GFX_GUI_SCALE, off_y + (10 + 12 * (k / 16)) * GFX_GUI_SCALE, str, 8 * GFX_GUI_SCALE, false);
		}
	}

	// draw cursor
	gutil_text(off_x + (2 + 8 * (selected_slot % 16)) * GFX_GUI_SCALE, off_y + (12 + 12 * (selected_slot / 16)) * GFX_GUI_SCALE, "_", 8 * GFX_GUI_SCALE, false);

	// draw controls
	int icon_offset = 16 * GFX_GUI_SCALE;
	icon_offset += gutil_control_icon(icon_offset, IB_GUI_UP, "Dec. char");
	icon_offset += gutil_control_icon(icon_offset, IB_GUI_DOWN, "Inc. char");
	icon_offset += gutil_control_icon(icon_offset, IB_GUI_LEFT, "Prev. char");
	icon_offset += gutil_control_icon(icon_offset, IB_GUI_RIGHT, "Next char");
	icon_offset += gutil_control_icon(icon_offset, IB_INVENTORY, "Leave");

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
