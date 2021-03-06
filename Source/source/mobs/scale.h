/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Header for the scale class and scale-related functions.
 */

#ifndef SCALE_INCLUDED
#define SCALE_INCLUDED

#include "mob.h"

#include "../mob_types/scale_type.h"

/* ----------------------------------------------------------------------------
 * A scale is something that measures the weight being applied on top of it,
 * and does something depending on the value.
 */
class scale : public mob {
public:

    scale_type* sca_type;
    
    float calculate_cur_weight();
    
    scale(const point &pos, scale_type* type, float angle);
};

#endif //ifndef SCALE_INCLUDED
