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

static bool pointer_has_item;
static bool pointer_available;
static float pointer_x, pointer_y, pointer_angle;

static void screen_pause_reset(struct screen* s, int width, int height) {
	if(gstate.local_player)
		gstate.local_player->data.local_player.capture_input = false;

	s->render3D = screen_ingame.render3D;
}

static void screen_pause_update(struct screen* s, float dt) {
	if(input_pressed(IB_HOME)) {
		svin_rpc_send(&(struct server_rpc) {
			.type = SRPC_TOGGLE_PAUSE,
		});
		gstate.paused = false;

		screen_set(&screen_ingame);
	}

	if(input_pressed(IB_INVENTORY)) {
		svin_rpc_send(&(struct server_rpc) {
			.type = SRPC_TOGGLE_PAUSE,
		});
		gstate.paused = false;

		screen_set(&screen_select_world);
		svin_rpc_send(&(struct server_rpc) {
			.type = SRPC_UNLOAD_WORLD,
		});
	}
}

static void screen_pause_render2D(struct screen* s, int width, int height) {
	// darken background
	gfx_texture(false);
	gutil_texquad_col(0, 0, 0, 0, 0, 0, width, height, 0, 0, 0, 180);
	gfx_texture(true);

	gutil_text(4, 4 + (GFX_GUI_SCALE * 8 + 1) * 0, "PAUSED", GFX_GUI_SCALE * 8, true);

	int icon_offset = GFX_GUI_SCALE * 16;
	icon_offset += gutil_control_icon(icon_offset, IB_HOME, "Back to game");
	icon_offset += gutil_control_icon(icon_offset, IB_INVENTORY, "Save & quit");
}

struct screen screen_pause = {
	.reset = screen_pause_reset,
	.update = screen_pause_update,
	.render2D = screen_pause_render2D,
	.render3D = NULL,
	.render_world = false,
};
