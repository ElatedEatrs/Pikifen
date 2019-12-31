/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Onion finite state machine logic.
 */

#include "onion_fsm.h"

#include "../functions.h"
#include "../mobs/onion.h"
#include "../particle.h"
#include "../utils/string_utils.h"
#include "../vars.h"

/* ----------------------------------------------------------------------------
 * Creates the finite state machine for the Onion's logic.
 */
void onion_fsm::create_fsm(mob_type* typ) {
    easy_fsm_creator efc;
    
    efc.new_state("idling", ONION_STATE_IDLING); {
        efc.new_event(MOB_EVENT_RECEIVE_DELIVERY); {
            efc.run(onion_fsm::receive_mob);
        }
    }
    
    typ->states = efc.finish();
    typ->first_state_nr = fix_states(typ->states, "idling");
    
    //Check if the number in the enum and the total match up.
    engine_assert(
        typ->states.size() == N_ONION_STATES,
        i2s(typ->states.size()) + " registered, " +
        i2s(N_ONION_STATES) + " in enum."
    );
}


/* ----------------------------------------------------------------------------
 * When an Onion finishes receiving a mob carried by Pikmin.
 * info1: Pointer to the mob.
 */
void onion_fsm::receive_mob(mob* m, void* info1, void* info2) {
	engine_assert(info1 != NULL, m->print_state_history());

	mob* delivery = (mob*)info1;
	onion* o_ptr = (onion*)m;
	size_t seeds = 0;
	if (VERSUS_ON == true) {

		size_t teamnumber = 0;
		bool is_marble = false;
		for (size_t o = 0; o < marbles.size(); ++o) {
			if (delivery->type == marbles[o]->type)is_marble = true;
		}

		if (o_ptr->team == MOB_TEAM_PLAYER_1) {
			teamnumber = 0;
		}
		else	if (o_ptr->team == MOB_TEAM_PLAYER_2) {
			teamnumber = 1;
		}
		else if (o_ptr->team == MOB_TEAM_PLAYER_3) {
			teamnumber = 2;
		}
		else if (o_ptr->team == MOB_TEAM_PLAYER_4) {
			teamnumber = 3;
		}

		if (delivery->team == MOB_TEAM_JERRY&&is_marble == true) {

			playerpts[teamnumber] -= 1;
		}
		else if (delivery->team == MOB_TEAM_JURY && is_marble == true) {
			playerpts[teamnumber] += 1;
		}


		if ((teamnumber == 0 && delivery->team == MOB_TEAM_ALLIES1) ||
			(teamnumber == 1 && delivery->team == MOB_TEAM_ALLIES2) ||
			(teamnumber == 2 && delivery->team == MOB_TEAM_ALLIES3) ||
			(teamnumber == 3 && delivery->team == MOB_TEAM_ALLIES4)) {
			return;
		}
	}
    if(delivery->type->category->id == MOB_CATEGORY_ENEMIES) {
        seeds = ((enemy*) delivery)->ene_type->pikmin_seeds;
    } else if(delivery->type->category->id == MOB_CATEGORY_PELLETS) {
        pellet* p_ptr = (pellet*) delivery;
        if(p_ptr->pel_type->pik_type == o_ptr->oni_type->pik_type) {
            seeds = p_ptr->pel_type->match_seeds;
        } else {
            seeds = p_ptr->pel_type->non_match_seeds;
        }
    }
    
    o_ptr->full_spew_timer.start();
    o_ptr->next_spew_timer.stop();
    o_ptr->spew_queue += seeds;
    
    particle p(
        PARTICLE_TYPE_BITMAP, m->pos, m->z + m->height - 0.01,
        24, 1.5, PARTICLE_PRIORITY_MEDIUM
    );
    p.bitmap = bmp_smoke;
    particle_generator pg(0, p, 15);
    pg.number_deviation = 5;
    pg.angle = 0;
    pg.angle_deviation = TAU / 2;
    pg.total_speed = 70;
    pg.total_speed_deviation = 10;
    pg.duration_deviation = 0.5;
    pg.emit(particles);
    
}
