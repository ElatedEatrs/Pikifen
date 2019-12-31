/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Header for the leader class and leader-related functions.
 */

#ifndef LEADER_INCLUDED
#define LEADER_INCLUDED

#include <vector>

#include "../mob_types/leader_type.h"
#include "mob.h"

class pikmin;

using namespace std;

enum LEADER_STATES {
    LEADER_STATE_IDLING,
    LEADER_STATE_ACTIVE,
    LEADER_STATE_WHISTLING,
    LEADER_STATE_PUNCHING,
    LEADER_STATE_HOLDING,
    LEADER_STATE_DISMISSING,
    LEADER_STATE_SPRAYING,
    LEADER_STATE_PAIN,
    LEADER_STATE_INACTIVE_PAIN,
    LEADER_STATE_KNOCKED_BACK,
    LEADER_STATE_INACTIVE_KNOCKED_BACK,
    LEADER_STATE_DYING,
    LEADER_STATE_IN_GROUP_CHASING,
    LEADER_STATE_IN_GROUP_STOPPED,
    LEADER_STATE_GOING_TO_PLUCK,
    LEADER_STATE_PLUCKING,
    LEADER_STATE_INACTIVE_GOING_TO_PLUCK,
    LEADER_STATE_INACTIVE_PLUCKING,
    LEADER_STATE_SLEEPING_WAITING,
    LEADER_STATE_SLEEPING_MOVING,
    LEADER_STATE_SLEEPING_STUCK,
    LEADER_STATE_INACTIVE_SLEEPING_WAITING,
    LEADER_STATE_INACTIVE_SLEEPING_MOVING,
    LEADER_STATE_INACTIVE_SLEEPING_STUCK,
    //Time during which the leader is getting up.
    LEADER_STATE_WAKING_UP,
    //Time during which the leader is getting up.
    LEADER_STATE_INACTIVE_WAKING_UP,
    LEADER_STATE_HELD,
    LEADER_STATE_THROWN,
    LEADER_STATE_DRINKING,
    LEADER_STATE_RIDING_TRACK,
    LEADER_STATE_INACTIVE_RIDING_TRACK,
    
    N_LEADER_STATES,
    
};

enum LEADER_ANIMATIONS {
    LEADER_ANIM_IDLING,
    LEADER_ANIM_WALKING,
    LEADER_ANIM_PLUCKING,
    LEADER_ANIM_GETTING_UP,
    LEADER_ANIM_DISMISSING,
    LEADER_ANIM_THROWING,
    LEADER_ANIM_WHISTLING,
    LEADER_ANIM_PUNCHING,
    LEADER_ANIM_LYING,
    LEADER_ANIM_PAIN,
    LEADER_ANIM_KNOCKED_DOWN,
    LEADER_ANIM_SPRAYING,
    LEADER_ANIM_DRINKING,
};

const float LEADER_HELD_MOB_ANGLE = TAU / 2;
const float LEADER_HELD_MOB_DIST = 1.2f;
const float LEADER_INVULN_PERIOD = 1.5f;


/* ----------------------------------------------------------------------------
 * A leader controls Pikmin, and
 * is controlled by the player.
 */
class leader : public mob {
private:
    size_t get_dismiss_rows(const size_t n_members);
    
public:
    leader_type* lea_type;
	bool whistling;
	timer whistle_fade_timer;
	float whistle_fade_radius;
    bool auto_plucking;
    pikmin* pluck_target;
    bool queued_pluck_cancel;
	size_t playernum;
    bool is_in_walking_anim;

    leader(const point &pos, leader_type* type, const float angle);
    
    virtual void draw_mob(bitmap_effect_manager* effect_manager = NULL);

	void update_closest_group_member();
	bool grab_closest_group_member();
	void change_to_next_leader(const bool forward, const bool force_success);
	void signal_group_move_start();
    void signal_group_move_end();
    void dismiss();
    void start_whistling();
    void stop_whistling();
    void swap_held_pikmin(mob* new_pik);
    
    virtual bool can_receive_status(status_type* s);
    virtual void tick_class_specifics();
    
};





#endif //ifndef LEADER_INCLUDED
