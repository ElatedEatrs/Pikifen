/*
 * Copyright (c) Andre 'Espyo' Silva 2013-2018.
 * The following source file belongs to the open-source project
 * Pikifen. Please read the included
 * README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Ship type class and ship type-related functions.
 */

#include "../functions.h"
#include "ship.h"
#include "ship_fsm.h"
#include "ship_type.h"

/* ----------------------------------------------------------------------------
 * Creates a type of ship.
 */
ship_type::ship_type() :
    mob_type(MOB_CATEGORY_SHIPS),
    can_heal(false),
    beam_radius(0.0f) {
    
    ship_fsm::create_fsm(this);
    always_active = true;
}


/* ----------------------------------------------------------------------------
 * Loads parametes from a data file.
 */
void ship_type::load_parameters(data_node* file) {
    can_heal = file->get_child_by_name("can_heal");
    beam_offset.x = s2f(file->get_child_by_name("beam_offset_x")->value);
    beam_offset.y = s2f(file->get_child_by_name("beam_offset_y")->value);
    beam_radius = s2f(file->get_child_by_name("beam_radius")->value);
}


/* ----------------------------------------------------------------------------
 * Returns the vector of animation conversions.
 */
anim_conversion_vector ship_type::get_anim_conversions() {
    anim_conversion_vector v;
    v.push_back(make_pair(SHIP_ANIM_IDLING, "idling"));
    return v;
}
