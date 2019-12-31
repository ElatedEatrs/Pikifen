/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Gameplay state class and
 * gameplay state-related functions.
 */

#include <algorithm>

#include <allegro5/allegro_native_dialog.h>
#include <algorithm>
#include "endofday.h"
#include "drawing.h"
#include "functions.h"
#include "load.h"
#include "misc_structs.h"
#include "utils/string_utils.h"
#include "vars.h"



/* ----------------------------------------------------------------------------
 * Creates the "gameplay" state.
 */
endo::endo() :
	game_state(){

}
void endo::load() {
	ready_for_input = false;
	msx_today = load_sample("today.ogg", mixer);
	msx_today.play(10, true, 0.7, 0.5, 0.9);
	draw_loading_screen("Today's results", "Situation:Report", 1.0);
    area_title_fade_timer.start();
	framerate_last_avg_point = 0;
	framerate_history.clear();
	for (size_t o = 0; o < max_players; ++o) {
		scorer += "team" + i2s(o) + ":" + i2s(playerpts[o]);
	}
}
void endo::leave() {
	if (bim == true) {
		change_game_state(GAME_STATE_MAIN_MENU);
	}
	else {
		change_game_state(GAME_STATE_AREA_EDITOR);
	}
}
void endo::do_drawing() {

	do_nightime_drawing();

}
void endo::unload() {
	msx_today.stop();
	msx_today.destroy();
}
void endo::update_transformations() {
	//World coordinates to screen coordinates.
	world_to_screen_transform[pnum] = identity_transform;
	al_translate_transform(
		&world_to_screen_transform[pnum],
		-cam_pos[pnum].x + scr_w / 2.0 / cam_zoom,
		-cam_pos[pnum].y + scr_h / 2.0 / cam_zoom
	);
	al_scale_transform(&world_to_screen_transform[pnum], cam_zoom, cam_zoom);

	//Screen coordinates to world coordinates.
	screen_to_world_transform[pnum] = world_to_screen_transform[pnum];
	al_invert_transform(&screen_to_world_transform[pnum]);
}
void endo::handle_controls(const ALLEGRO_EVENT &ev) {

		vector<action_from_event> actions = get_actions_from_event(ev);
		for (size_t a = 0; a < actions.size(); ++a) {
			handle_button(actions[a].button, actions[a].pos, actions[a].playee);
		}
}

void endo::handle_button(
	const size_t button, const float pos, const size_t player
) {
	if (!ready_for_input || !is_input_allowed) return;

	bool is_down = (pos >= 0.5);
	if (button == BUTTON_PAUSE) {

		/********************
		*           +-+ +-+ *
		*   Pause   | | | | *
		*           +-+ +-+ *
		********************/

		if (!is_down) return;

		is_input_allowed = false;
		bim = true;
		fade_mgr.start_fade(
			false,
			[this]() {
			this->leave();
		}
		);

		//paused = true;

	}
}
void endo::do_logic() { 
	area_title_fade_timer.tick(delta_t);

	//Fade.
	fade_mgr.tick(delta_t);

	if (!ready_for_input) {
		ready_for_input = true;
		is_input_allowed = true;
	}
}
endo::~endo() {}