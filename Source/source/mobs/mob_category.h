/*
 * Copyright (c) Andre 'Espyo' Silva 2013-2017.
 * The following source file belongs to the open-source project
 * Pikmin fangame engine. Please read the included
 * README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Header for the mob category classes and mob category-related functions.
 */

#ifndef MOB_CATEGORY_INCLUDED
#define MOB_CATEGORY_INCLUDED

#include <string>
#include <vector>

#include <allegro5/allegro.h>


using namespace std;

enum MOB_CATEGORIES {
    //Sorted by what types of mobs to load first.
    MOB_CATEGORY_NONE,
    MOB_CATEGORY_PIKMIN,
    MOB_CATEGORY_ONIONS,
    MOB_CATEGORY_LEADERS,
    MOB_CATEGORY_ENEMIES,
    MOB_CATEGORY_TREASURES,
    MOB_CATEGORY_PELLETS,
    MOB_CATEGORY_SPECIAL,
    MOB_CATEGORY_SHIPS,
    MOB_CATEGORY_GATES,
    MOB_CATEGORY_BRIDGES,
    MOB_CATEGORY_CUSTOM,
    
    N_MOB_CATEGORIES,
};

class mob;
class mob_type;

/* ----------------------------------------------------------------------------
 * A mob category. Pikmin, leader, enemy, etc.
 * Each category helps organize the types of mob and the mobs themselves.
 */
class mob_category {
public:
    string name;
    size_t id;
    
    string plural_name;
    string folder;
    ALLEGRO_COLOR editor_color;
    
    virtual void get_type_names(vector<string> &list) = 0;
    virtual mob_type* get_type(const string &name) = 0;
    virtual mob_type* create_type() = 0;
    virtual void register_type(mob_type* type) = 0;
    virtual mob* create_mob(
        const point pos, mob_type* type,
        const float angle, const string vars
    ) = 0;
    virtual void erase_mob(mob* m) = 0;
    
    mob_category(
        const size_t id, const string &name, const string &plural_name,
        const string &folder, const ALLEGRO_COLOR editor_color
    );
    
};



/* ----------------------------------------------------------------------------
 * A list of the different mob categories.
 * The MOB_CATEGORY_* constants are meant to be used here.
 * Read the sector type manager's comments for more info.
 */
struct mob_category_manager {
private:
    vector<mob_category*> categories;
    
public:
    void register_category(size_t nr, mob_category* category);
    mob_category* get(const size_t id);
    mob_category* get_from_name(const string &name);
    mob_category* get_from_pname(const string &pname);
};



/* ----------------------------------------------------------------------------
 * "None" mob category. Used as a placeholder.
 */
class none_category : public mob_category {
public:
    virtual void get_type_names(vector<string> &list);
    virtual mob_type* get_type(const string &name);
    virtual mob_type* create_type();
    virtual void register_type(mob_type* type);
    virtual mob* create_mob(
        const point pos, mob_type* type,
        const float angle, const string vars
    );
    virtual void erase_mob(mob* m);
    
    none_category();
};


/* ----------------------------------------------------------------------------
 * Mob category for the Pikmin.
 */
class pikmin_category : public mob_category {
public:
    virtual void get_type_names(vector<string> &list);
    virtual mob_type* get_type(const string &name);
    virtual mob_type* create_type();
    virtual void register_type(mob_type* type);
    virtual mob* create_mob(
        const point pos, mob_type* type,
        const float angle, const string vars
    );
    virtual void erase_mob(mob* m);
    
    pikmin_category();
};


/* ----------------------------------------------------------------------------
 * Mob category for the enemies.
 */
class enemy_category : public mob_category {
public:
    virtual void get_type_names(vector<string> &list);
    virtual mob_type* get_type(const string &name);
    virtual mob_type* create_type();
    virtual void register_type(mob_type* type);
    virtual mob* create_mob(
        const point pos, mob_type* type,
        const float angle, const string vars
    );
    virtual void erase_mob(mob* m);
    
    enemy_category();
};


/* ----------------------------------------------------------------------------
 * Mob category for the leaders.
 */
class leader_category : public mob_category {
public:
    virtual void get_type_names(vector<string> &list);
    virtual mob_type* get_type(const string &name);
    virtual mob_type* create_type();
    virtual void register_type(mob_type* type);
    virtual mob* create_mob(
        const point pos, mob_type* type,
        const float angle, const string vars
    );
    virtual void erase_mob(mob* m);
    
    leader_category();
};


/* ----------------------------------------------------------------------------
 * Mob category for the Onions.
 */
class onion_category : public mob_category {
public:
    virtual void get_type_names(vector<string> &list);
    virtual mob_type* get_type(const string &name);
    virtual mob_type* create_type();
    virtual void register_type(mob_type* type);
    virtual mob* create_mob(
        const point pos, mob_type* type,
        const float angle, const string vars
    );
    virtual void erase_mob(mob* m);
    
    onion_category();
};


/* ----------------------------------------------------------------------------
 * Mob category for the pellets.
 */
class pellet_category : public mob_category {
public:
    virtual void get_type_names(vector<string> &list);
    virtual mob_type* get_type(const string &name);
    virtual mob_type* create_type();
    virtual void register_type(mob_type* type);
    virtual mob* create_mob(
        const point pos, mob_type* type,
        const float angle, const string vars
    );
    virtual void erase_mob(mob* m);
    
    pellet_category();
};


/* ----------------------------------------------------------------------------
 * Mob category for the ships.
 */
class ship_category : public mob_category {
public:
    virtual void get_type_names(vector<string> &list);
    virtual mob_type* get_type(const string &name);
    virtual mob_type* create_type();
    virtual void register_type(mob_type* type);
    virtual mob* create_mob(
        const point pos, mob_type* type,
        const float angle, const string vars
    );
    virtual void erase_mob(mob* m);
    
    ship_category();
};


/* ----------------------------------------------------------------------------
 * Mob category for the treasures.
 */
class treasure_category : public mob_category {
public:
    virtual void get_type_names(vector<string> &list);
    virtual mob_type* get_type(const string &name);
    virtual mob_type* create_type();
    virtual void register_type(mob_type* type);
    virtual mob* create_mob(
        const point pos, mob_type* type,
        const float angle, const string vars
    );
    virtual void erase_mob(mob* m);
    
    treasure_category();
};


/* ----------------------------------------------------------------------------
 * Mob category for the gates.
 */
class gate_category : public mob_category {
public:
    virtual void get_type_names(vector<string> &list);
    virtual mob_type* get_type(const string &name);
    virtual mob_type* create_type();
    virtual void register_type(mob_type* type);
    virtual mob* create_mob(
        const point pos, mob_type* type,
        const float angle, const string vars
    );
    virtual void erase_mob(mob* m);
    
    gate_category();
};


/* ----------------------------------------------------------------------------
 * Mob category for the bridges.
 */
class bridge_category : public mob_category {
public:
    virtual void get_type_names(vector<string> &list);
    virtual mob_type* get_type(const string &name);
    virtual mob_type* create_type();
    virtual void register_type(mob_type* type);
    virtual mob* create_mob(
        const point pos, mob_type* type,
        const float angle, const string vars
    );
    virtual void erase_mob(mob* m);
    
    bridge_category();
};


/* ----------------------------------------------------------------------------
 * Mob category for the special mob types.
 */
class special_category : public mob_category {
public:
    virtual void get_type_names(vector<string> &list);
    virtual mob_type* get_type(const string &name);
    virtual mob_type* create_type();
    virtual void register_type(mob_type* type);
    virtual mob* create_mob(
        const point pos, mob_type* type,
        const float angle, const string vars
    );
    virtual void erase_mob(mob* m);
    
    special_category();
};


/* ----------------------------------------------------------------------------
 * Mob category for the custom mob types.
 */
class custom_category : public mob_category {
public:
    virtual void get_type_names(vector<string> &list);
    virtual mob_type* get_type(const string &name);
    virtual mob_type* create_type();
    virtual void register_type(mob_type* type);
    virtual mob* create_mob(
        const point pos, mob_type* type,
        const float angle, const string vars
    );
    virtual void erase_mob(mob* m);
    
    custom_category();
};


#endif //ifndef MOB_CATEGORY_INCLUDED