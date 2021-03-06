/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Header for the bridge type class and bridge type-related functions.
 */

#ifndef BRIDGE_TYPE_INCLUDED
#define BRIDGE_TYPE_INCLUDED

#include <allegro5/allegro.h>

#include "../data_file.h"
#include "mob_type.h"

enum BRIDGE_ANIMATIONS {
    BRIDGE_ANIM_IDLING,
    BRIDGE_ANIM_DESTROYED,
};


/* ----------------------------------------------------------------------------
 * A type of bridge.
 */
class bridge_type : public mob_type {
public:
    ALLEGRO_BITMAP* bmp_main_texture;
    ALLEGRO_BITMAP* bmp_rail_texture;
    string main_texture_file_name;
    string rail_texture_file_name;
    
    bridge_type();
    ~bridge_type();
    void load_resources(data_node* file);
    anim_conversion_vector get_anim_conversions();
    void unload_resources();
};

#endif //ifndef BRIDGE_TYPE_INCLUDED
