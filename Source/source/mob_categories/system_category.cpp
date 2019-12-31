/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * system mob category class.
 */

#include <algorithm>

#include "system_category.h"

#include "../vars.h"


/* ----------------------------------------------------------------------------
 * Creates an instance of the category for the system mob types.
 */
psystem_category::psystem_category() :
    mob_category(
        MOB_CATEGORY_PSYSTEM, "System", "System",
        PSYSTEM_MOB_FOLDER_PATH, al_map_rgb(224, 128, 224)
    ) {
    
}


/* ----------------------------------------------------------------------------
 * Returns all system types by name.
 */
void psystem_category::get_type_names(vector<string> &list) {
    for(auto t = psystem_mob_types.begin(); t != psystem_mob_types.end(); ++t) {
        list.push_back(t->first);
    }
}


/* ----------------------------------------------------------------------------
 * Returns a system type given its name, or NULL on error.
 */
mob_type* psystem_category::get_type(const string &name) {
    auto it = psystem_mob_types.find(name);
    if(it == psystem_mob_types.end()) return NULL;
    return it->second;
}


/* ----------------------------------------------------------------------------
 * Creates a new, empty system type.
 */
mob_type* psystem_category::create_type() {
    return new mob_type(MOB_CATEGORY_PSYSTEM);
}


/* ----------------------------------------------------------------------------
 * Registers a created system type.
 */
void psystem_category::register_type(mob_type* type) {
    psystem_mob_types[type->name] = type;
}


/* ----------------------------------------------------------------------------
 * Creates a system mob and adds it to the list of system mobs.
 */
mob* psystem_category::create_mob(
    const point &pos, mob_type* type, const float angle
) {
    mob* m = new mob(pos, type, angle);
    return m;
}


/* ----------------------------------------------------------------------------
 * Clears a system mob from the list of system mobs.
 */
void psystem_category::erase_mob(mob* m) { }


/* ----------------------------------------------------------------------------
 * Clears the list of registered types of system mob.
 */
void psystem_category::clear_types() {
    for(auto t = psystem_mob_types.begin(); t != psystem_mob_types.end(); ++t) {
        delete t->second;
    }
    psystem_mob_types.clear();
}


psystem_category::~psystem_category() { }
