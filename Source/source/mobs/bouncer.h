/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Header for the bouncer class and bouncer-related functions.
 */

#ifndef BOUNCER_INCLUDED
#define BOUNCER_INCLUDED

#include <vector>

#include "../mob_types/bouncer_type.h"
#include "mob.h"


enum BOUNCER_STATES {
    BOUNCER_STATE_IDLING,
    N_BOUNCER_STATES,
};


/* ----------------------------------------------------------------------------
 * An object that throws another mob, bouncing it away.
 */
class bouncer : public mob {
public:
    bouncer_type* bou_type;
    
    bouncer(const point &pos, bouncer_type* type, const float angle);
};

#endif //ifndef BOUNCER_INCLUDED
