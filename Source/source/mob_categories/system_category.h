/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Header for the system mob category class.
 */

#ifndef PSYSTEM_CATEGORY_INCLUDED
#define PSYSTEM_CATEGORY_INCLUDED

#include <string>
#include <vector>

#include "../const.h"
#include "../mob_categories/mob_category.h"

using namespace std;


const string PSYSTEM_MOB_FOLDER_PATH = TYPES_FOLDER_PATH + "/System";


/* ----------------------------------------------------------------------------
 * Mob category for the system mob types.
 */
class psystem_category : public mob_category {
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
    
    psystem_category();
    ~psystem_category();
};

#endif //ifndef PSYSTEM_CATEGORY_INCLUDED
