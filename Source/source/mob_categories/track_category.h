/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Header for the track mob category class.
 */

#ifndef TRACK_CATEGORY_INCLUDED
#define TRACK_CATEGORY_INCLUDED

#include "../const.h"
#include "../mob_categories/mob_category.h"

using namespace std;


const string TRACKS_FOLDER_PATH = TYPES_FOLDER_PATH + "/Tracks";


/* ----------------------------------------------------------------------------
 * Mob category for anything that transports a mob from point A to point B
 * in a more-or-less linear fashion.
 */
class track_category : public mob_category {
public:
    virtual void get_type_names(vector<string> &list);
    virtual mob_type* get_type(const string &name);
    virtual mob_type* create_type();
    virtual void register_type(mob_type* type);
    virtual mob* create_mob(
        const point &pos, mob_type* type, const float angle
    );
    virtual void erase_mob(mob* m);
    virtual void clear_types();
    
    track_category();
    ~track_category();
};

#endif //ifndef TRACK_CATEGORY_INCLUDED
