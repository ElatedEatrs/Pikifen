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

#include "gameplay.h"
#include "Save.h"
#include "drawing.h"
#include "functions.h"
#include "load.h"
#include "misc_structs.h"
#include "utils/string_utils.h"
#include "vars.h"

//How long the HUD moves for when the area is entered.
const float gameplay::AREA_INTRO_HUD_MOVE_TIME = 3.0f;


/* ----------------------------------------------------------------------------
 * Creates the "gameplay" state.
 */
gameplay::gameplay() :
    game_state(),
    bmp_bubble(nullptr),
    bmp_counter_bubble_group(nullptr),
    bmp_counter_bubble_field(nullptr),
    bmp_counter_bubble_standby(nullptr),
    bmp_counter_bubble_total(nullptr),
    bmp_day_bubble(nullptr),
    bmp_distant_pikmin_marker(nullptr),
    bmp_fog(nullptr),
    bmp_hard_bubble(nullptr),
    bmp_message_box(nullptr),
    bmp_no_pikmin_bubble(nullptr),
    bmp_sun(nullptr),
	end_of_day(false){
	switch(max_players){
		case 1: {
			break;
		}
		case 2: {
			
			bmp_player1 = al_create_bitmap(scr_w * 1, scr_h * 1);
			bmp_player2 = al_create_bitmap(scr_w * 1, scr_h * 1);
			bmp_player3 = al_create_bitmap(scr_w * 1, scr_h * 1);
			bmp_player4 = al_create_bitmap(scr_w * 1, scr_h * 1);
			
			break;
		}
		case 3: {
			bmp_player1 = al_create_bitmap(scr_w * 1, scr_h * 1);
			bmp_player2 = al_create_bitmap(scr_w * 1, scr_h * 1);
			bmp_player3 = al_create_bitmap(scr_w * 1, scr_h * 1);
			bmp_player4 = al_create_bitmap(scr_w * 1, scr_h * 1);
			break;

		}
		case 4: {
			bmp_player1 = al_create_bitmap(scr_w * 1, scr_h * 1);
			bmp_player2 = al_create_bitmap(scr_w * 1, scr_h * 1);
			bmp_player3 = al_create_bitmap(scr_w * 1, scr_h * 1);
			bmp_player4 = al_create_bitmap(scr_w * 1, scr_h * 1);
			break;
		}
}

}

const int FOG_BITMAP_SIZE = 128;

/* ----------------------------------------------------------------------------
 * Generates the bitmap that'll draw the fog fade effect.
 */
ALLEGRO_BITMAP* gameplay::generate_fog_bitmap(
    const float near_radius, const float far_radius
) {
    if(far_radius == 0) return NULL;
    
    ALLEGRO_BITMAP* bmp = al_create_bitmap(FOG_BITMAP_SIZE, FOG_BITMAP_SIZE);
    
    ALLEGRO_LOCKED_REGION* region =
        al_lock_bitmap(
            bmp, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_WRITEONLY
        );
    unsigned char* row = (unsigned char*) region->data;
    
    //We need to draw a radial gradient to represent the fog.
    //Between the center and the "near" radius, the opacity is 0%.
    //From there to the edge, the opacity fades to 100%.
    //Because the every quadrant of the image is the same, just mirrored,
    //we only need to process the pixels on the top-left quadrant and then
    //apply them to the respective pixels on the other quadrants as well.
    
    //How close to the edge is the current pixel? 0 = center, 1 = edge.
    float cur_ratio = 0;
    //Alpha to use for this pixel.
    unsigned char cur_a = 0;
    //This is where the "near" section of the fog is.
    float near_ratio = near_radius / far_radius;
    //Memory location of the opposite row's pixels.
    unsigned char* opposite_row;
    
#define fill_pixel(x, row) \
    row[(x) * 4 + 0] = 255; \
    row[(x) * 4 + 1] = 255; \
    row[(x) * 4 + 2] = 255; \
    row[(x) * 4 + 3] = cur_a; \
    
    for(int y = 0; y < ceil(FOG_BITMAP_SIZE / 2.0); ++y) {
        for(int x = 0; x < ceil(FOG_BITMAP_SIZE / 2.0); ++x) {
            //First, get how far this pixel is from the center.
            //Center = 0, radius or beyond = 1.
            cur_ratio =
                dist(
                    point(x, y),
                    point(FOG_BITMAP_SIZE / 2.0, FOG_BITMAP_SIZE / 2.0)
                ).to_float() / (FOG_BITMAP_SIZE / 2.0);
            cur_ratio = min(cur_ratio, 1.0f);
            //Then, map that ratio to a different ratio that considers
            //the start of the "near" section as 0.
            cur_ratio =
                interpolate_number(cur_ratio, near_ratio, 1.0f, 0.0f, 1.0f);
            //Finally, clamp the value and get the alpha.
            cur_ratio = clamp(cur_ratio, 0.0f, 1.0f);
            cur_a = 255 * cur_ratio;
            
            opposite_row = row + region->pitch * (FOG_BITMAP_SIZE - y - y - 1);
            fill_pixel(x, row);
            fill_pixel(FOG_BITMAP_SIZE - x - 1, row);
            fill_pixel(x, opposite_row);
            fill_pixel(FOG_BITMAP_SIZE - x - 1, opposite_row);
        }
        row += region->pitch;
    }
    
#undef fill_pixel
    
    al_unlock_bitmap(bmp);
    bmp = recreate_bitmap(bmp); //Refresh mipmaps.
    return bmp;
}


/* ----------------------------------------------------------------------------
 * Leaves the gameplay state, returning to the main menu, or wherever else.
 */
void gameplay::leave() {
	if (end_of_day == true) {
		change_game_state(GAME_STATE_ENDOFDAY);
	}else if(area_editor_quick_play_area.empty()) {
        change_game_state(GAME_STATE_MAIN_MENU);
    } else {
        change_game_state(GAME_STATE_AREA_EDITOR);
    }
}


/* ----------------------------------------------------------------------------
 * Loads the "gameplay" state into memory.
 */
void gameplay::load() {
    size_t errors_reported_at_start = errors_reported_today;
	pnum = 0;
    ready_for_input = false;
	end_of_day = false;
	msx_mission = load_sample("mission.ogg", mixer);
	msx_alert = load_sample("Alert.ogg", mixer);
	msx_mission.play(10, true, 0.7, 0.5, 1);

    draw_loading_screen("", "", 1.0f);
    al_flip_display();
	scorer = "";
    //Game content.
    load_game_content();
    
    //Initializing game things.
    size_t n_spray_types = spray_types.size();
    for(size_t s = 0; s < n_spray_types; ++s) {
        spray_stats.push_back(spray_stats_struct());
    }
    
    load_area(area_to_load, false, false);
    load_area_textures();
    
    if(!cur_area_data.weather_condition.blackout_strength.empty()) {
        lightmap_bmp = al_create_bitmap(scr_w, scr_h);
    }
    if(!cur_area_data.weather_condition.fog_color.empty()) {
        bmp_fog =
            generate_fog_bitmap(
                cur_area_data.weather_condition.fog_near,
                cur_area_data.weather_condition.fog_far
            );
    }
    
    //Generate mobs.
    vector<mob*> mobs_per_gen;
    
    for(size_t m = 0; m < cur_area_data.mob_generators.size(); ++m) {
        mob_gen* m_ptr = cur_area_data.mob_generators[m];
        
        mobs_per_gen.push_back(
            create_mob(
                m_ptr->category, m_ptr->pos, m_ptr->type,
                m_ptr->angle, m_ptr->vars
            )
        );
    }
	if(day != 1)loadmobs();
    //Panic check -- If there are no leaders, abort.
    if(leaders.empty()) {
        show_message_box(
            display, "No leaders!", "No leaders!",
            "This area has no leaders! You need at least one "
            "in order to play.",
            NULL, ALLEGRO_MESSAGEBOX_WARN
        );
        leave();
        return;
    }
    
    //Mob links.
    //Because mobs can create other mobs when loaded, mob gen number X
    //does not necessarily correspond to mob number X. Hence, we need
    //to keep the pointers to the created mobs in a vector, and use this
    //to link the mobs by (generator) number.
    for(size_t m = 0; m < cur_area_data.mob_generators.size(); ++m) {
        mob_gen* m_ptr = cur_area_data.mob_generators[m];
        
        for(size_t l = 0; l < m_ptr->link_nrs.size(); ++l) {
            mobs_per_gen[m]->links.push_back(mobs_per_gen[m_ptr->link_nrs[l]]);
        }
    }


    //Sort leaders.
    sort(
        leaders.begin(), leaders.end(),
    [] (leader * l1, leader * l2) -> bool {
        size_t priority_l1 =
        find(leader_order.begin(), leader_order.end(), l1->lea_type) -
        leader_order.begin();
        size_t priority_l2 =
        find(leader_order.begin(), leader_order.end(), l2->lea_type) -
        leader_order.begin();
        return priority_l1 < priority_l2;
    }
    );
	for (size_t o = 0; o < max_players; ++o) {
		pnum = o;
		cur_leader_nrs[o] = o;
		cur_leader_ptrs[o] = leaders[o];
		leaders[o]->playernum = o;
		cur_leader_ptrs[o]->fsm.set_state(LEADER_STATE_ACTIVE);
	}

	pnum = 0;
	for (size_t o = 0; o < max_players; ++o) {
		pnum = o;
		cam_pos[o] = cam_final_pos[o] = cur_leader_ptrs[o]->pos;
		cam_zoom = cam_final_zoom = zoom_mid_level;
		update_transformations();
	}

	pnum = 0;
    ALLEGRO_MOUSE_STATE mouse_state;
    al_get_mouse_state(&mouse_state);
    mouse_cursor_s[pnum].x = al_get_mouse_state_axis(&mouse_state, 0);
    mouse_cursor_s[pnum].y = al_get_mouse_state_axis(&mouse_state, 1);
    mouse_cursor_w[pnum] = mouse_cursor_s[pnum];
	
	al_transform_coordinates(
		&screen_to_world_transform[pnum],
		&mouse_cursor_w[pnum].x, &mouse_cursor_w[pnum].y);

	for (size_t o = 0; o < max_players; ++o) {
		leader_cursor_ws[o]=(point(o,o));
		leader_cursor_ss[o]=(point(o,o));
		cur_leader_ptrs[o]->stop_whistling();
	}
    day_minutes = day_minutes_start;
    area_time_passed = 0;
	if (VERSUS_ON) {
		for (size_t i = 0; i < mobs.size(); ++i) {
			bool place_inside = true;
			mob* mob_ptr = mobs[i];
			if (mob_ptr->team == MOB_TEAM_JURY || mob_ptr->team == MOB_TEAM_JERRY) {
				for (size_t m = 0; m < marbles.size(); ++m) {
					if (marbles[m]->type == mob_ptr->type) {
						place_inside = false;
						break;
					}
				}
				if (place_inside == true) {
					point pose = mob_ptr->pos;
					mob_type* typee = mob_ptr->type;
					float anglee = 0;
					mob* m_ptr = mob_ptr->type->category->create_mob(pose, typee, anglee);
					for (size_t a = 0; a < typee->init_actions.size(); ++a) {
						typee->init_actions[a]->run(m_ptr, NULL, NULL);
					}
					marbles.push_back(m_ptr);
				}
			}
		}
		for (size_t i = 0; i < marbles.size(); ++i) {
			marbles[i]->tick();
			marbles[i]->pos = point(marbles[i]->pos.x + 10000.0, marbles[i]->pos.y + 10000.0);

		}
		if (marbles.size() != 1) {
			size_t barbles = (marbles.size() / 4) ;

			for (size_t i = 0; i < barbles; ++i) {
				size_t warbles = randomi(0, marbles.size() - 1);
				mob* m_ptr = marbles[warbles];
				m_ptr->type->category->erase_mob(m_ptr);
				marbles.erase(find(marbles.begin(), marbles.end(), m_ptr));

			}
		}
	}
    vector<string> spray_amount_name_strs;
    vector<string> spray_amount_value_strs;
    get_var_vectors(
        cur_area_data.spray_amounts,
        spray_amount_name_strs, spray_amount_value_strs
    );
    
    for(size_t s = 0; s < spray_amount_name_strs.size(); ++s) {
        size_t spray_id = 0;
        for(; spray_id < spray_types.size(); ++spray_id) {
            if(spray_types[spray_id].name == spray_amount_name_strs[s]) {
                break;
            }
        }
        if(spray_id == spray_types.size()) {
            log_error(
                "Unknown spray type \"" + spray_amount_name_strs[s] + "\", "
                "while trying to set the starting number of sprays for "
                "area \"" + cur_area_data.name + "\"!", NULL
            );
            continue;
        }
        
        spray_stats[spray_id].nr_sprays = s2i(spray_amount_value_strs[s]);
    }
	for (size_t o = 0; o < max_players; ++o) {
		for (size_t c = 0; c < controls[o].size(); ++c) {
			if (controls[o][c].action == BUTTON_THROW) {
				click_control_id = c;
				break;
			}
		}
	}
	for (size_t o = 0; o < max_players; ++o) {
		for (size_t c = 0; c < controls[o].size(); ++c) {
			if (controls[o][c].action == BUTTON_WHISTLE) {
				whistle_control_id[pnum] = c;
				break;
			}
		}
	}
    //TODO Uncomment this when replays are implemented.
    /*
    replay_timer = timer(
        REPLAY_SAVE_FREQUENCY,
    [this] () {
        this->replay_timer.start();
        vector<mob*> obstacles; //TODO
        session_replay.add_state(
            leaders, pikmin_list, enemies, treasures, onions, obstacles,
            cur_leader_nrs[pnum] 
        );
    }
    );
    replay_timer.start();
    session_replay.clear();*/
    
    al_hide_mouse_cursor(display);
    
    area_title_fade_timer.start();
    hud_items.start_move(true, AREA_INTRO_HUD_MOVE_TIME);
    
    //Aesthetic stuff.
    cur_message_char_timer =
        timer(
            message_char_interval,
    [] () {
        cur_message_char_timer.start();
        cur_message_char++;
    }
        );
        
    if(errors_reported_today > errors_reported_at_start) {
        print_info(
            "\n\n\nERRORS FOUND!\n"
            "See \"" + ERROR_LOG_FILE_PATH + "\".\n\n\n",
            20.0f, 3.0f
        );
    }
	
    framerate_last_avg_point = 0;
    framerate_history.clear();
    
}


/* ----------------------------------------------------------------------------
 * Loads all of the game's content.
 */
void gameplay::load_game_content() {
    load_custom_particle_generators(true);
    load_liquids(true);
    load_status_types(true);
    load_spray_types(true);
    load_hazards();
    load_hud_info();
    load_weather();
    load_spike_damage_types();
    
    //Mob types.
    load_mob_types(true);
    
    //Register leader sub-group types.
    for(size_t p = 0; p < pikmin_order.size(); ++p) {
        subgroup_types.register_type(
            SUBGROUP_TYPE_CATEGORY_PIKMIN, pikmin_order[p],
            pikmin_order[p]->bmp_icon
        );
    }
    
    vector<string> tool_types_vector;
    for(auto t = tool_types.begin(); t != tool_types.end(); ++t) {
        tool_types_vector.push_back(t->first);
    }
    sort(tool_types_vector.begin(), tool_types_vector.end());
    for(size_t t = 0; t < tool_types_vector.size(); ++t) {
        tool_type* tt_ptr = tool_types[tool_types_vector[t]];
        subgroup_types.register_type(
            SUBGROUP_TYPE_CATEGORY_TOOL, tt_ptr, tt_ptr->bmp_icon
        );
    }
    
    subgroup_types.register_type(SUBGROUP_TYPE_CATEGORY_LEADER);
    
}


/* ----------------------------------------------------------------------------
 * Loads all gameplay HUD info.
 */
void gameplay::load_hud_info() {
    data_node file(MISC_FOLDER_PATH + "/HUD.txt");
    if(!file.file_was_opened) return;
    
    //Hud coordinates.
    data_node* positions_node = file.get_child_by_name("positions");
    
#define loader(id, name) \
    load_hud_coordinates(id, positions_node->get_child_by_name(name)->value)
    
    loader(HUD_ITEM_TIME,                  "time");
    loader(HUD_ITEM_DAY_BUBBLE,            "day_bubble");
    loader(HUD_ITEM_DAY_NUMBER,            "day_number");
    loader(HUD_ITEM_LEADER_1_ICON,         "leader_1_icon");
    loader(HUD_ITEM_LEADER_2_ICON,         "leader_2_icon");
    loader(HUD_ITEM_LEADER_3_ICON,         "leader_3_icon");
    loader(HUD_ITEM_LEADER_1_HEALTH,       "leader_1_health");
    loader(HUD_ITEM_LEADER_2_HEALTH,       "leader_2_health");
    loader(HUD_ITEM_LEADER_3_HEALTH,       "leader_3_health");
    loader(HUD_ITEM_PIKMIN_STANDBY_ICON,   "pikmin_standby_icon");
    loader(HUD_ITEM_PIKMIN_STANDBY_M_ICON, "pikmin_standby_m_icon");
    loader(HUD_ITEM_PIKMIN_STANDBY_NR,     "pikmin_standby_nr");
    loader(HUD_ITEM_PIKMIN_STANDBY_X,      "pikmin_standby_x");
    loader(HUD_ITEM_PIKMIN_GROUP_NR,       "pikmin_group_nr");
    loader(HUD_ITEM_PIKMIN_FIELD_NR,       "pikmin_field_nr");
    loader(HUD_ITEM_PIKMIN_TOTAL_NR,       "pikmin_total_nr");
    loader(HUD_ITEM_PIKMIN_SLASH_1,        "pikmin_slash_1");
    loader(HUD_ITEM_PIKMIN_SLASH_2,        "pikmin_slash_2");
    loader(HUD_ITEM_PIKMIN_SLASH_3,        "pikmin_slash_3");
    loader(HUD_ITEM_SPRAY_1_ICON,          "spray_1_icon");
    loader(HUD_ITEM_SPRAY_1_AMOUNT,        "spray_1_amount");
    loader(HUD_ITEM_SPRAY_1_BUTTON,        "spray_1_button");
    loader(HUD_ITEM_SPRAY_2_ICON,          "spray_2_icon");
    loader(HUD_ITEM_SPRAY_2_AMOUNT,        "spray_2_amount");
    loader(HUD_ITEM_SPRAY_2_BUTTON,        "spray_2_button");
    loader(HUD_ITEM_SPRAY_PREV_ICON,       "spray_prev_icon");
    loader(HUD_ITEM_SPRAY_PREV_BUTTON,     "spray_prev_button");
    loader(HUD_ITEM_SPRAY_NEXT_ICON,       "spray_next_icon");
    loader(HUD_ITEM_SPRAY_NEXT_BUTTON,     "spray_next_button");
    
#undef loader
    
    //Bitmaps.
    data_node* bitmaps_node = file.get_child_by_name("files");
    
#define loader(var, name) \
    var = \
          bitmaps.get( \
                       bitmaps_node->get_child_by_name(name)->value, \
                       bitmaps_node->get_child_by_name(name) \
                     );
    
    loader(bmp_bubble,                 "bubble");
    loader(bmp_counter_bubble_field,   "counter_bubble_field");
    loader(bmp_counter_bubble_group,   "counter_bubble_group");
    loader(bmp_counter_bubble_standby, "counter_bubble_standby");
    loader(bmp_counter_bubble_total,   "counter_bubble_total");
    loader(bmp_day_bubble,             "day_bubble");
    loader(bmp_distant_pikmin_marker,  "distant_pikmin_marker");
    loader(bmp_hard_bubble,            "hard_bubble");
    loader(bmp_message_box,            "message_box");
    loader(bmp_no_pikmin_bubble,       "no_pikmin_bubble");
    loader(bmp_sun,                    "sun");
    
#undef loader
    
}


/* ----------------------------------------------------------------------------
 * Loads HUD coordinates of a specific HUD item.
 */
void gameplay::load_hud_coordinates(const int item, string data) {
    vector<string> words = split(data);
    if(data.size() < 4) return;
    
    hud_items.set_item(
        item, s2f(words[0]), s2f(words[1]), s2f(words[2]), s2f(words[3])
    );
}


/* ----------------------------------------------------------------------------
 * Unloads the "gameplay" state from memory.
 */
void gameplay::unload() {
    al_show_mouse_cursor(display);
	for (size_t o = 0; o < max_players; ++o) {
		pnum = o;
		cur_leader_ptrs[pnum] = NULL;
		cam_pos[pnum] = cam_final_pos[pnum] = point();
	}
	pnum = 0;
	msx_mission.stop();
    cam_zoom = cam_final_zoom = 1.0f;
	savemobs();
	save_sectors();
	save_flags();
	marbles.clear();
    while(!mobs.empty()) {
		mob* m_ptr = *mobs.begin();
        delete_mob(*mobs.begin(), true);
		delete m_ptr;
	}
    
    if(lightmap_bmp) {
        al_destroy_bitmap(lightmap_bmp);
        lightmap_bmp = NULL;
    }
    
    unload_area_textures();
    unload_area();
    
    spray_stats.clear();
    particles.clear();
    
    unload_game_content();
	bitmaps.detach(bmp_player1);
	bitmaps.detach(bmp_player2);
	bitmaps.detach(bmp_player3);
	bitmaps.detach(bmp_player4);
    bitmaps.detach(bmp_bubble);
    bitmaps.detach(bmp_counter_bubble_field);
    bitmaps.detach(bmp_counter_bubble_group);
    bitmaps.detach(bmp_counter_bubble_standby);
    bitmaps.detach(bmp_counter_bubble_total);
    bitmaps.detach(bmp_day_bubble);
    bitmaps.detach(bmp_distant_pikmin_marker);
    bitmaps.detach(bmp_hard_bubble);
    bitmaps.detach(bmp_message_box);
    bitmaps.detach(bmp_no_pikmin_bubble);
    bitmaps.detach(bmp_sun);
    if(bmp_fog) {
        al_destroy_bitmap(bmp_fog);
        bmp_fog = NULL;
    }
    
    cur_message.clear();
    info_print_text.clear();
}


/* ----------------------------------------------------------------------------
 * Unloads loaded game content.
 */
void gameplay::unload_game_content() {
    weather_conditions.clear();
    
    subgroup_types.clear();
    
    unload_mob_types(true);
    
    unload_spike_damage_types();
    unload_hazards();
    unload_spray_types();
    unload_status_types(true);
    unload_liquids();
    unload_custom_particle_generators();
}


/* ----------------------------------------------------------------------------
 * Updates the transformations, with the current camera coordinates, zoom, etc.
 */
void gameplay::update_transformations() {
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


/* ----------------------------------------------------------------------------
 * Tick the gameplay logic by one frame.
 */
void gameplay::do_logic() {
    if(creator_tool_change_speed) {
        delta_t *= creator_tool_change_speed_mult;
    }
	for (size_t i = 0; i < max_players; ++i) {
		pnum = i;
		do_leader_logic();
	}
	pnum = 0;
    do_gameplay_logic();
    do_aesthetic_logic();
	if (VERSUS_ON == false) {
		if (day_minutes >= day_minutes_end) {
			end_of_day = true;
			msx_alert.stop();
			leave();
		}
	}
	for (size_t o = 0; o < max_players; ++o) {
		if (playerpts[o] >= max_score) {
		end_of_day = true;
		msx_mission.stop();
		msx_alert.stop();
		leave();
		}
	}
}


/* ----------------------------------------------------------------------------
 * Draw the gameplay.
 */
void gameplay::do_drawing() {
	
	switch (max_players) {
	case 1: {
		pnum = 0;
		do_game_drawing();
		break;
	}
	case 2: {
		pnum = 0;

		do_game_drawing(bmp_player1);
		pnum = 1;
		do_game_drawing(bmp_player2);
		al_clear_to_color(cur_area_data.bg_color);
		draw_bitmap(bmp_player1, point(scr_w * 0.5, scr_h * 0.25), point(scr_w * 0.48, scr_h * 0.48), 0);
		draw_bitmap(bmp_player2, point(scr_w * 0.5, scr_h * 0.75), point(scr_w * 0.48, scr_h * 0.48), 0);
		pnum = 0;
		break;
	}
	case 3: {
		pnum = 0;
		do_game_drawing(bmp_player1);
		pnum = 1;
		do_game_drawing(bmp_player2);
		pnum = 2;
		do_game_drawing(bmp_player3);
		al_clear_to_color(cur_area_data.bg_color);
		draw_bitmap(bmp_player1, point(scr_w * 0.25, scr_h * 0.25), point(scr_w * 0.48, scr_h * 0.48), 0);
		draw_bitmap(bmp_player2, point(scr_w * 0.75, scr_h * 0.25), point(scr_w * 0.48, scr_h * 0.48), 0);
		draw_bitmap(bmp_player3, point(scr_w * 0.25, scr_h * 0.75), point(scr_w * 0.48, scr_h * 0.48), 0);
		pnum = 0;
		break;
	}
	case 4: {

		pnum = 0;
		do_game_drawing(bmp_player1);
		pnum = 1;
		do_game_drawing(bmp_player2);
		pnum = 2;
		do_game_drawing(bmp_player3);
		pnum = 3;
		do_game_drawing(bmp_player4);
		al_clear_to_color(cur_area_data.bg_color);
		draw_bitmap(bmp_player1, point(scr_w * 0.25, scr_h * 0.25), point(scr_w * 0.48, scr_h * 0.48), 0);
		draw_bitmap(bmp_player2, point(scr_w * 0.75, scr_h * 0.25), point(scr_w * 0.48, scr_h * 0.48), 0);
		draw_bitmap(bmp_player3, point(scr_w * 0.25, scr_h * 0.75), point(scr_w * 0.48, scr_h * 0.48), 0);
		draw_bitmap(bmp_player4, point(scr_w * 0.75, scr_h * 0.75), point(scr_w * 0.48, scr_h * 0.48), 0);
		pnum = 0;
		break;
	}
	}
	for (size_t o = 0; o < max_players; ++o) {
			draw_bitmap(
				bmp_mouse_cursor,
				mouse_cursor_s[pnum],
				point(
					cam_zoom * al_get_bitmap_width(bmp_mouse_cursor) * 0.5,
					cam_zoom * al_get_bitmap_height(bmp_mouse_cursor) * 0.5
				),
				-(area_time_passed * cursor_spin_speed),
				change_color_lighting(
					cur_leader_ptrs[pnum]->lea_type->main_color,
					cursor_height_diff_light
				)
			);
	}
	
}


gameplay::~gameplay() { }
