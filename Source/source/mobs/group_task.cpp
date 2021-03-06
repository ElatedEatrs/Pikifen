/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Group task class and group task-related functions.
 */

#include "group_task.h"
#include "../vars.h"

/* ----------------------------------------------------------------------------
 * Creates a new group task mob.
 */
group_task::group_task(
    const point &pos, group_task_type* type, const float angle
):
    mob(pos, type, angle),
    worker_nr(0),
    ran_task_finished_code(false),
    tas_type(type) {
    
    //Initialize spots.
    float row_angle = get_angle(tas_type->first_row_p1, tas_type->first_row_p2);
    size_t needed_rows = ceil(tas_type->max_pikmin / tas_type->pikmin_per_row);
    float point_dist =
        dist(tas_type->first_row_p1, tas_type->first_row_p2).to_float();
    float space_between_neighbors =
        point_dist / (float) (tas_type->pikmin_per_row - 1);
        
    //Rotate the anchor point, first_row_p1, based on the mob's angle.
    point anchor = rotate_point(tas_type->first_row_p1, angle);
    
    //Create a transformation based on the anchor.
    ALLEGRO_TRANSFORM trans;
    al_identity_transform(&trans);
    al_rotate_transform(&trans, row_angle + angle);
    al_translate_transform(
        &trans, anchor.x, anchor.y
    );
    
    for(size_t r = 0; r < needed_rows; ++r) {
    
        for(size_t s = 0; s < tas_type->pikmin_per_row; ++s) {
        
            float x;
            if(tas_type->pikmin_per_row % 2 == 0) {
                x =
                    space_between_neighbors / 2.0 +
                    space_between_neighbors * ceil((s - 1.0f) / 2.0);
                x *= (s % 2 == 0) ? 1 : -1;
            } else {
                if(s == 0) {
                    x = 0;
                } else {
                    x = space_between_neighbors * ceil(s / 2.0);
                    x *= (s % 2 == 0) ? 1 : -1;
                }
            }
            x += point_dist / 2.0f;
            
            point pos(x, r * tas_type->interval_between_rows);
            al_transform_coordinates(&trans, &pos.x, &pos.y);
            
            spots.push_back(group_task_spot(pos));
        }
    }
}


/* ----------------------------------------------------------------------------
 * Adds a Pikmin to the task as an actual worker.
 * who: Pikmin to add.
 */
void group_task::add_worker(pikmin* who) {
    for(size_t s = 0; s < spots.size(); ++s) {
        if(spots[s].pikmin_here == who) {
            spots[s].state = 2;
            worker_nr++;
            break;
        }
    }
    
    if(worker_nr == tas_type->pikmin_goal) {
        string msg = "goal_reached";
        who->send_message(this, msg);
    }
}


/* ----------------------------------------------------------------------------
 * Code to run when the task is finished.
 */
void group_task::finish_task() {
    for(size_t p = 0; p < pikmin_list.size(); ++p) {
        if(pikmin_list[p]->focused_mob && pikmin_list[p]->focused_mob == this) {
            pikmin_list[p]->fsm.run_event(MOB_EVENT_FOCUSED_MOB_UNAVAILABLE);
        }
    }
}


/* ----------------------------------------------------------------------------
 * Frees up a previously-reserved spot.
 * whose: Who had the reservation?
 */
void group_task::free_up_spot(pikmin* whose) {
    for(size_t s = 0; s < spots.size(); ++s) {
        if(spots[s].pikmin_here == whose) {
            if(spots[s].state == 2) {
                worker_nr--;
            }
            spots[s].state = 0;
            spots[s].pikmin_here = NULL;
            break;
        }
    }
    
    if(worker_nr == tas_type->pikmin_goal - 1) {
        string msg = "goal_lost";
        whose->send_message(this, msg);
    }
}


/* ----------------------------------------------------------------------------
 * Returns a free spot, closest to the center and to the frontmost row as
 * possible.
 * Returns NULL if there is none.
 */
group_task::group_task_spot* group_task::get_free_spot() {
    size_t spots_taken = 0;
    
    for(size_t s = 0; s < spots.size(); ++s) {
        if(spots[s].state != 0) {
            spots_taken++;
            if(spots_taken == tas_type->max_pikmin) {
                //Max Pikmin reached! The Pikmin can't join,
                //regardless of there being free spots.
                return NULL;
            }
        }
        if(spots[s].state == 0) return &(spots[s]);
    }
    
    return NULL;
}


/* ----------------------------------------------------------------------------
 * Returns the current number of workers.
 */
size_t group_task::get_worker_nr() {
    return worker_nr;
}


/* ----------------------------------------------------------------------------
 * Returns the current world coordinates of a spot, occupied by a Pikmin.
 * Returns a (0,0) point if that Pikmin doesn't have a spot.
 * whose: Pikmin whose spot to check.
 */
point group_task::get_spot_pos(pikmin* whose) {
    for(size_t s = 0; s < spots.size(); ++s) {
        if(spots[s].pikmin_here == whose) {
            return pos + spots[s].pos;
        }
    }
    return point();
}


/* ----------------------------------------------------------------------------
 * Reserves a spot for a Pikmin.
 * spot: Pointer to the spot to reserve.
 * who:  Who will be reserving this spot?
 */
void group_task::reserve_spot(group_task::group_task_spot* spot, pikmin* who) {
    spot->state = 1;
    spot->pikmin_here = who;
}


/* ----------------------------------------------------------------------------
 * Runs code specific to this class, once per frame.
 */
void group_task::tick_class_specifics() {
    if(health <= 0 && !ran_task_finished_code) {
        ran_task_finished_code = true;
        finish_task();
    }
    
    if(health > 0) {
        ran_task_finished_code = false;
    }
    
    if(
        chasing &&
        worker_nr >= tas_type->pikmin_goal &&
        tas_type->speed_bonus != 0.0f
    ) {
        //Being moved and movements can go through speed bonuses?
        //Let's update the speed.
        chase_speed =
            type->move_speed +
            (worker_nr - tas_type->pikmin_goal) * tas_type->speed_bonus;
    }
}


/* ----------------------------------------------------------------------------
 * Creates a new group task spot struct.
 */
group_task::group_task_spot::group_task_spot(const point &pos) :
    pos(pos), state(0), pikmin_here(nullptr) {
    
}
