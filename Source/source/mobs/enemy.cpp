/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Enemy class and enemy-related functions.
 */

#include <algorithm>
#include <unordered_set>

#include "enemy.h"

#include "../drawing.h"
#include "../functions.h"
#include "../mob_types/mob_type.h"
#include "../utils/math_utils.h"
#include "../utils/string_utils.h"

/* ----------------------------------------------------------------------------
 * Creates an enemy mob.
 */
enemy::enemy(const point &pos, enemy_type* type, const float angle) :
    mob(pos, type, angle),
    ene_type(type),
    spawn_delay(0),
    respawn_days_left(0),
    respawns_after_x_days(0),
    appears_after_day(0),
    appears_before_day(0),
    appears_every_x_days(0) {
    
    team = MOB_TEAM_ENEMY_1; //TODO removeish.
}


/* ----------------------------------------------------------------------------
 * Draws an enemy, tinting it if necessary (for Onion delivery).
 */
void enemy::draw_mob(bitmap_effect_manager* effect_manager) {
    sprite* s_ptr = anim.get_cur_sprite();
    if(!s_ptr) return;
    
    point draw_pos = get_sprite_center(s_ptr);
    point draw_size = get_sprite_dimensions(s_ptr);
    
    bitmap_effect_manager effects;
    
    if(fsm.cur_state->id == ENEMY_EXTRA_STATE_BEING_DELIVERED) {
        onion* o_ptr = ((onion*) focused_mob);
        add_delivery_bitmap_effect(
            &effects, script_timer.get_ratio_left(),
            o_ptr->oni_type->pik_type->main_color
        );
    }
    
    add_status_bitmap_effects(&effects);
    add_sector_brightness_bitmap_effect(&effects);
    
    draw_bitmap_with_effects(
        s_ptr->bitmap,
        draw_pos, draw_size,
        angle + s_ptr->angle, &effects
    );
    
    draw_status_effect_bmp(this, &effects);
    
}


/* ----------------------------------------------------------------------------
 * Returns whether or not an enemy can receive a given status effect.
 */
bool enemy::can_receive_status(status_type* s) {
    return s->affects & STATUS_AFFECTS_ENEMIES;
}


/* ----------------------------------------------------------------------------
 * Reads the provided script variables, if any, and does stuff with them.
 */
void enemy::read_script_vars(const string &vars) {
    mob::read_script_vars(vars);
    
    spawn_delay = s2f(get_var_value(vars, "spawn_delay", "0"));
    vector<string> spoils_strs =
        semicolon_list_to_vector(get_var_value(vars, "spoils", ""), ",");
    vector<string> random_pellet_spoils_strs =
        semicolon_list_to_vector(
            get_var_value(vars, "random_pellet_spoils", ""), ","
        );
        
    for(size_t s = 0; s < spoils_strs.size(); ++s) {
        mob_type* type_ptr =
            mob_categories.find_mob_type(spoils_strs[s]);
        if(!type_ptr) {
            log_error(
                "A mob (" + get_error_message_mob_info(this) + ") is set to "
                "have a spoil of type \"" + spoils_strs[s] + "\", but no "
                "such mob type exists!"
            );
            continue;
        }
        specific_spoils.push_back(type_ptr);
    }
    
    for(size_t s = 0; s < random_pellet_spoils_strs.size(); ++s) {
        random_pellet_spoils.push_back(s2i(random_pellet_spoils_strs[s]));
    }
}


/* ----------------------------------------------------------------------------
 * Sets up stuff for the beginning of the enemy's death process.
 */
void enemy::start_dying_class_specific() {
    vector<mob_type*> spoils_to_spawn = specific_spoils;
    
    //If there are random pellets to spawn, then prepare some data.
    if(!random_pellet_spoils.empty()) {
        vector<pikmin_type*> available_pik_types;
        
        //Start by obtaining a list of available Pikmin types, given the
        //Onions currently in the area.
        for(size_t o = 0; o < onions.size(); ++o) {
            available_pik_types.push_back(onions[o]->oni_type->pik_type);
        }
        
        //Remove duplicates from the list.
        sort(available_pik_types.begin(), available_pik_types.end());
        available_pik_types.erase(
            unique(available_pik_types.begin(), available_pik_types.end()),
            available_pik_types.end()
        );
        
        for(size_t s = 0; s < random_pellet_spoils.size(); ++s) {
            //For every pellet that we want to spawn...
            vector<pellet_type*> possible_pellets;
            
            //Check the pellet types that match that number and
            //also match the available Pikmin types.
            for(auto p = pellet_types.begin(); p != pellet_types.end(); ++p) {
                bool pik_type_ok = false;
                for(size_t pt = 0; pt < available_pik_types.size(); ++pt) {
                    if(p->second->pik_type == available_pik_types[pt]) {
                        pik_type_ok = true;
                        break;
                    }
                }
                
                if(!pik_type_ok) {
                    //There is no Onion for this pellet type. Pass.
                    continue;
                }
                
                if(p->second->number == random_pellet_spoils[s]) {
                    possible_pellets.push_back(p->second);
                }
            }
            
            //And now, pick a random one out of the possible pellets.
            if(!possible_pellets.empty()) {
                spoils_to_spawn.push_back(
                    possible_pellets[randomi(0, possible_pellets.size() - 1)]
                );
            }
        }
    }
    
    for(size_t s = 0; s < spoils_to_spawn.size(); ++s) {
        mob_type::spawn_struct str;
        str.angle = 0;
        str.coords_z = 0;
        str.relative = true;
        str.momentum = 100;
        spawn(&str, spoils_to_spawn[s]);
    }
}
