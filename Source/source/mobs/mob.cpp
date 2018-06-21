/*
 * Copyright (c) Andre 'Espyo' Silva 2013-2018.
 * The following source file belongs to the open-source project
 * Pikifen. Please read the included
 * README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Mob class and mob-related functions.
 */

#include <algorithm>

#include "../const.h"
#include "../drawing.h"
#include "../functions.h"
#include "../geometry_utils.h"
#include "mob.h"
#include "pikmin.h"
#include "ship.h"
#include "../status.h"
#include "../vars.h"


size_t next_mob_id = 0;


/* ----------------------------------------------------------------------------
 * Creates a mob of no particular type.
 */
mob::mob(
    const point &pos, mob_type* type,
    const float angle, const string &vars
) :
    type(type),
    to_delete(false),
    anim(&type->anims),
    fsm(this),
    script_timer(0),
    focused_mob(nullptr),
    itch_damage(0),
    itch_time(0),
    far_reach(INVALID),
    near_reach(INVALID),
    pos(pos),
    z(0),
    speed_z(0),
    angle(angle),
    intended_turn_angle(angle),
    intended_turn_pos(nullptr),
    z_cap(FLT_MAX),
    home(pos),
    ground_floor(nullptr),
    center_sector(nullptr),
    gravity_mult(1.0f),
    push_amount(0),
    push_angle(0),
    unpushable(false),
    tangible(true),
    was_thrown(false),
    chasing(false),
    chase_offset(pos),
    chase_orig_coords(nullptr),
    chase_teleport_z(nullptr),
    chase_teleport(false),
    chase_free_move(false),
    chase_target_dist(0),
    chase_speed(-1),
    reached_destination(false),
    following_group(nullptr),
    subgroup_type_ptr(nullptr),
    group(nullptr),
    group_spot_index(INVALID),
    carry_info(nullptr),
    carrying_target(nullptr),
    cur_path_stop_nr(INVALID),
    id(next_mob_id),
    health(type->max_health),
    invuln_period(0),
    team(MOB_TEAM_DECORATION),
    hide(false),
    on_hazard(nullptr),
    dead(false),
    chomp_max(0),
    disabled_state_flags(0) {
    
    next_mob_id++;
    
    sector* sec = get_sector(pos, nullptr, true);
    if(sec) {
        ground_floor = &(sec->floors[0]);
        z = sec->floors[0].z;
    } else {
        to_delete = true;
    }
    center_sector = sec;
    
    if(type->is_obstacle) team = MOB_TEAM_OBSTACLE;
    
    for(size_t a = 0; a < type->init_actions.size(); ++a) {
        type->init_actions[a]->run(this, NULL, NULL, MOB_EVENT_UNKNOWN);
    }
    if(!vars.empty()) {
        vector<string> vars_strings = split(vars, ";");
        for(size_t v = 0; v < vars_strings.size(); ++v) {
            vector<string> parts = split(vars_strings[v], "=");
            if(parts.size() < 2) continue;
            this->vars[parts[0]] = parts[1];
        }
    }
    fsm.set_state(type->first_state_nr);
}


/* ----------------------------------------------------------------------------
 * Adds a bitmap effect to the manager, responsible for shading the
 * mob when it is in a shaded sector.
 */
void mob::add_sector_brightness_bitmap_effect(bitmap_effect_manager* manager) {
    if(center_sector->brightness == 255) return;
    
    bitmap_effect se;
    bitmap_effect_props props;
    
    props.tint_color = map_gray(center_sector->brightness);
    
    se.add_keyframe(0, props);
    manager->add_effect(se);
}


/* ----------------------------------------------------------------------------
 * Adds a bitmap effect to the manager, responsible for color
 * and scaling the mob when it is being delivered to an Onion.
 */
void mob::add_delivery_bitmap_effect(
    bitmap_effect_manager* manager, const float delivery_time_ratio_left,
    const ALLEGRO_COLOR &onion_color
) {

    bitmap_effect se;
    bitmap_effect_props props_half;
    bitmap_effect_props props_end;
    
    se.add_keyframe(0, bitmap_effect_props());
    
    props_half.glow_color = onion_color;
    se.add_keyframe(0.5, props_half);
    
    props_end.glow_color = onion_color;
    props_end.scale = point(0, 0);
    se.add_keyframe(1.0, props_end);
    
    se.set_cur_time(1.0f - delivery_time_ratio_left);
    manager->add_effect(se);
}


/* ----------------------------------------------------------------------------
 * Adds the bitmap effects caused by the status effects to the manager.
 */
void mob::add_status_bitmap_effects(bitmap_effect_manager* manager) {
    for(size_t s = 0; s < statuses.size(); ++s) {
        status_type* t = this->statuses[s].type;
        if(
            t->tint.r == 1.0f &&
            t->tint.g == 1.0f &&
            t->tint.b == 1.0f &&
            t->tint.a == 1.0f
        ) {
            continue;
        }
        
        bitmap_effect se;
        bitmap_effect_props props;
        props.tint_color = t->tint;
        
        se.add_keyframe(0, props);
        manager->add_effect(se);
    }
}


/* ----------------------------------------------------------------------------
 * Adds a mob to this mob's group.
 */
void mob::add_to_group(mob* new_member) {
    //If it's already following, never mind.
    if(new_member->following_group == this) return;
    
    new_member->following_group = this;
    group->members.push_back(new_member);
    
    //Find a spot.
    group->init_spots(new_member);
    new_member->group_spot_index = group->spots.size() - 1;
    
    if(!group->cur_standby_type) {
        if(
            new_member->type->category->id != MOB_CATEGORY_LEADERS ||
            can_throw_leaders
        ) {
            group->cur_standby_type =
                new_member->subgroup_type_ptr;
        }
    }
    
    if(group->members.size() == 1) {
        //If this is the first member, update the anchor position.
        group->anchor = pos;
    }
}


/* ----------------------------------------------------------------------------
 * Applies the knockback values to a mob.
 */
void mob::apply_knockback(const float knockback, const float knockback_angle) {
    if(knockback != 0) {
        stop_chasing();
        speed.x = cos(knockback_angle) * knockback * MOB_KNOCKBACK_H_POWER;
        speed.y = sin(knockback_angle) * knockback * MOB_KNOCKBACK_H_POWER;
        speed_z = MOB_KNOCKBACK_V_POWER;
        face(get_angle(point(), point(speed)) + M_PI, NULL);
    }
}


/* ----------------------------------------------------------------------------
 * Applies a status effect's effects.
 */
void mob::apply_status_effect(status_type* s, const bool refill) {
    if(!can_receive_status(s)) return;
    
    //Check if the mob is already under this status.
    for(size_t ms = 0; ms < this->statuses.size(); ++ms) {
        if(this->statuses[ms].type == s) {
            //Already exists. Can we refill its duration?
            
            if(refill && s->auto_remove_time > 0.0f) {
                this->statuses[ms].time_left = s->auto_remove_time;
            }
            
            return;
        }
    }
    
    //This status is not already inflicted. Let's do so.
    this->statuses.push_back(status(s));
    if(s->causes_disable) {
        receive_disable_from_status(
            (s->disabled_state_inedible ? DISABLED_STATE_FLAG_INEDIBLE : 0)
        );
    } else if(s->causes_panic) {
        receive_panic_from_status();
    } else if(s->causes_flailing) {
        receive_flailing_from_status();
    }
    change_maturity_amount_from_status(s->maturity_change_amount);
    if(s->generates_particles) {
        particle_generator pg = *s->particle_gen;
        pg.follow_pos = &this->pos;
        pg.follow_angle = &this->angle;
        pg.reset();
        particle_generators.push_back(pg);
    }
}


/* ----------------------------------------------------------------------------
 * Makes the mob attack another mob.
 * Returns true if the attack was successful.
 * victim:   The mob to be attacked.
 * attack_h: Hitbox used for the attack.
 * victim_h: Victim's hitbox that got hit.
 * damage:   If not NULL, total damage caused is returned here.
 */
bool mob::attack(
    mob* victim, hitbox* attack_h, hitbox* victim_h, float* damage
) {
    float total_damage = 0;
    float attacker_offense = 0;
    float defense_multiplier = 1;
    
    //First, check if this mob cannot be damaged.
    if(victim_h->type != HITBOX_TYPE_NORMAL) {
        //This hitbox can't be damaged! Abort!
        return false;
    }
    
    if(!is_resistant_to_hazards(victim_h->hazards)) {
        //If the hitbox says it has a fire effect, and this
        //mob is not immune to fire, don't let it be a wise-guy;
        //it cannot be able to attack the hitbox.
        return false;
    }
    
    for(size_t h = 0; h < hit_opponents.size(); ++h) {
        if(hit_opponents[h].second == victim) {
            //This opponent has already been hit by this mob recently.
            //Don't let it attack again. This stops the same attack from
            //hitting every single frame.
            return false;
        }
    }
    
    //Calculate the damage.
    if(attack_h) {
        attacker_offense = attack_h->value;
    } else {
        attacker_offense = 1;
    }
    
    if(victim_h->value == 0.0f) {
        //Hah, this hitbox is invulnerable!
        return false;
    }
    defense_multiplier = victim_h->value;
    
    for(size_t s = 0; s < statuses.size(); ++s) {
        attacker_offense *= statuses[s].type->attack_multiplier;
    }
    for(size_t s = 0; s < victim->statuses.size(); ++s) {
        defense_multiplier *= victim->statuses[s].type->defense_multiplier;
    }
    
    total_damage = attacker_offense * (1.0 / defense_multiplier);
    
    //Actually perform the damage and script-related events.
    victim->set_health(true, false, -total_damage);
    
    hitbox_interaction ev_info(this, victim_h, attack_h);
    victim->fsm.run_event(MOB_EVENT_DAMAGE, (void*) &ev_info);
    
    victim->cause_spike_damage(victim, false);
    
    //Final setup.
    victim->itch_damage += total_damage;
    hit_opponents.push_back(
        make_pair(OPPONENT_HIT_REGISTER_TIMEOUT, victim)
    );
    
    //Smack particle effect.
    point smack_p_pos =
        pos +
        (victim->pos - pos) *
        (type->radius / (type->radius + victim->type->radius));
    sfx_attack.play(0.06, false, 0.6f);
    particle smack_p(
        PARTICLE_TYPE_SMACK, smack_p_pos,
        64, SMACK_PARTICLE_DUR, PARTICLE_PRIORITY_MEDIUM
    );
    smack_p.bitmap = bmp_smack;
    smack_p.color = al_map_rgb(255, 160, 128);
    smack_p.before_mobs = false;
    particles.add(smack_p);
    
    if(damage) *damage = total_damage;
    return true;
}


/* ----------------------------------------------------------------------------
 * Sets up data for a mob to become carriable.
 */
void mob::become_carriable(const bool to_ship) {
    carry_info = new carry_info_struct(this, to_ship);
}


/* ----------------------------------------------------------------------------
 * Sets up data for a mob to stop being carriable.
 */
void mob::become_uncarriable() {
    if(!carry_info) return;
    
    for(size_t p = 0; p < carry_info->spot_info.size(); ++p) {
        if(carry_info->spot_info[p].state != CARRY_SPOT_FREE) {
            carry_info->spot_info[p].pik_ptr->fsm.run_event(
                MOB_EVENT_FOCUSED_MOB_UNCARRIABLE
            );
        }
    }
    
    stop_chasing();
    
    delete carry_info;
    carry_info = NULL;
}


/* ----------------------------------------------------------------------------
 * Updates carrying data, begins moving if needed, etc.
 * added:   The Pikmin that got added, if any.
 * removed: The Pikmin that got removed, if any.
 */
void mob::calculate_carrying_destination(mob* added, mob* removed) {
    if(!carry_info) return;
    
    carry_info->stuck_state = 0;
    
    //For starters, check if this is to be carried to the ship.
    //Get that out of the way if so.
    if(carry_info->carry_to_ship) {
    
        ship* closest_ship = NULL;
        dist closest_ship_dist;
        
        for(size_t s = 0; s < ships.size(); ++s) {
            ship* s_ptr = ships[s];
            dist d(pos, s_ptr->beam_final_pos);
            
            if(!closest_ship || d < closest_ship_dist) {
                closest_ship = s_ptr;
                closest_ship_dist = d;
            }
        }
        
        if(closest_ship) {
            carry_info->final_destination = closest_ship->beam_final_pos;
            carrying_target = closest_ship;
            
        } else {
            carrying_target = NULL;
            carry_info->stuck_state = 1;
            return;
        }
        
        return;
    }
    
    //If it's meant for an Onion, we need to decide which Onion, based on
    //the Pikmin. Buckle up, because it's not as easy as it might seem.
    
    //How many of each Pikmin type are carrying.
    map<pikmin_type*, unsigned> type_quantity;
    //The Pikmin type with the most carriers.
    vector<pikmin_type*> majority_types;
    unordered_set<pikmin_type*> available_onions;
    
    //First, check what Onions even are available.
    for(size_t o = 0; o < onions.size(); o++) {
        onion* o_ptr = onions[o];
        if(o_ptr->activated) {
            available_onions.insert(o_ptr->oni_type->pik_type);
        }
    }
    
    if(available_onions.empty()) {
        //No Onions?! Well...make the Pikmin stuck.
        carrying_target = NULL;
        carry_info->stuck_state = 1;
        return;
    }
    
    //Count how many of each type there are carrying.
    for(size_t p = 0; p < type->max_carriers; ++p) {
        pikmin* pik_ptr = NULL;
        
        if(carry_info->spot_info[p].state != CARRY_SPOT_USED) continue;
        
        pik_ptr = (pikmin*) carry_info->spot_info[p].pik_ptr;
        
        //If it doesn't have an Onion, it won't even count.
        if(available_onions.find(pik_ptr->pik_type) == available_onions.end()) {
            continue;
        }
        
        type_quantity[pik_ptr->pik_type]++;
    }
    
    //Then figure out what are the majority types.
    unsigned most = 0;
    for(auto t = type_quantity.begin(); t != type_quantity.end(); ++t) {
        if(t->second > most) {
            most = t->second;
            majority_types.clear();
        }
        if(t->second == most) majority_types.push_back(t->first);
    }
    
    //If we ended up with no candidates, pick a type at random,
    //out of all possible types.
    if(majority_types.empty()) {
        for(
            auto t = available_onions.begin();
            t != available_onions.end(); ++t
        ) {
            majority_types.push_back(*t);
        }
    }
    
    pikmin_type* decided_type = NULL;
    
    //Now let's pick an Onion from the candidates.
    if(majority_types.size() == 1) {
        //If there's only one possible type to pick, pick it.
        decided_type = *majority_types.begin();
        
    } else {
        //If there's a tie, let's take a careful look.
        bool new_tie = false;
        
        //Is the Pikmin that just joined part of the majority types?
        //If so, that means this Pikmin just created a NEW tie!
        //So let's pick a random Onion again.
        if(added) {
            for(size_t mt = 0; mt < majority_types.size(); ++mt) {
                if(added->type == majority_types[mt]) {
                    new_tie = true;
                    break;
                }
            }
        }
        
        //If a Pikmin left, check if it is related to the majority types.
        //If not, then a new tie wasn't made, no worries.
        //If it was related, a new tie was created.
        if(removed) {
            new_tie = false;
            for(size_t mt = 0; mt < majority_types.size(); ++mt) {
                if(removed->type == majority_types[mt]) {
                    new_tie = true;
                    break;
                }
            }
        }
        
        //Check if the previously decided type belongs to one of the majorities.
        //If so, it can be chosen again, but if not, it cannot.
        bool can_continue = false;
        for(size_t mt = 0; mt < majority_types.size(); ++mt) {
            if(majority_types[mt] == decided_type) {
                can_continue = true;
                break;
            }
        }
        if(!can_continue) decided_type = NULL;
        
        //If the Pikmin that just joined is not a part of the majorities,
        //then it had no impact on the existing ties.
        //Go with the Onion that had been decided before.
        if(new_tie || !decided_type) {
            decided_type =
                majority_types[randomi(0, majority_types.size() - 1)];
        }
    }
    
    
    //Figure out where that type's Onion is.
    size_t onion_nr = 0;
    for(; onion_nr < onions.size(); ++onion_nr) {
        if(onions[onion_nr]->oni_type->pik_type == decided_type) {
            break;
        }
    }
    
    //Finally, set the destination data.
    carry_info->final_destination = onions[onion_nr]->pos;
    carrying_target = onions[onion_nr];
}


/* ----------------------------------------------------------------------------
 * Makes the mob cause spike damage to another mob.
 * victim:       The mob that will be damaged.
 * is_ingestion: If true, the attacker just got eaten.
 *   If false, it merely got hurt.
 */
void mob::cause_spike_damage(mob* victim, const bool is_ingestion) {
    if(!type->spike_damage) return;
    
    if(type->spike_damage->ingestion_only != is_ingestion) return;
    
    float damage;
    if(type->spike_damage->is_damage_ratio) {
        damage = victim->type->max_health * type->spike_damage->damage;
    } else {
        damage = type->spike_damage->damage;
    }
    
    auto v =
        victim->type->spike_damage_vulnerabilities.find(type->spike_damage);
    if(v != victim->type->spike_damage_vulnerabilities.end()) {
        damage *= v->second;
    }
    
    victim->set_health(true, false, -damage);
    
    if(type->spike_damage->particle_gen) {
        particle_generator pg = *(type->spike_damage->particle_gen);
        pg.base_particle.pos = victim->pos;
        pg.emit(particles);
    }
}


/* ----------------------------------------------------------------------------
 * Sets a target for the mob to follow.
 * offs_*:          Coordinates of the target, relative to either the
 *   world origin, or another point, specified in the next parameters.
 * orig_*:          Pointers to changing coordinates. If NULL, it is
 *   the world origin. Use this to make the mob follow another mob
 *   wherever they go, for instance.
 * teleport:        If true, the mob teleports to that spot,
 *   instead of walking to it.
 * teleport_z:      Teleports to this Z coordinate, too.
 * free_move:       If true, the mob can go to a direction they're not facing.
 * target_distance: Distance from the target in which the mob is
 *   considered as being there.
 * speed:           Speed at which to go to the target. -1 uses the mob's speed.
 */
void mob::chase(
    const point &offset, point* orig_coords,
    const bool teleport, float* teleport_z,
    const bool free_move, const float target_distance, const float speed
) {

    this->chase_offset = offset;
    this->chase_orig_coords = orig_coords;
    this->chase_teleport = teleport;
    this->chase_teleport_z = teleport_z;
    this->chase_free_move = free_move;
    this->chase_target_dist = target_distance;
    this->chase_speed = (speed == -1 ? get_base_speed() : speed);
    
    chasing = true;
    reached_destination = false;
}


/* ----------------------------------------------------------------------------
 * Deletes all status effects asking to be deleted.
 */
void mob::delete_old_status_effects() {
    for(size_t s = 0; s < this->statuses.size(); ) {
        if(statuses[s].to_delete) {
            if(statuses[s].type->causes_panic) {
                lose_panic_from_status();
            }
            if(statuses[s].type->generates_particles) {
                remove_particle_generator(statuses[s].type->particle_gen->id);
            }
            this->statuses.erase(this->statuses.begin() + s);
        } else {
            ++s;
        }
    }
}


/* ----------------------------------------------------------------------------
 * Draws the mob. This can be overwritten by child classes.
 * effect_manager: Use this effect manager.
 *   If NULL, an effect manager is created inside exclusively for the function.
 */
void mob::draw(bitmap_effect_manager* effect_manager) {

    sprite* s_ptr = anim.get_cur_sprite();
    
    if(!s_ptr) return;
    
    bool internal_effect_manager = false;
    
    if(!effect_manager) {
        effect_manager = new bitmap_effect_manager();
        internal_effect_manager = true;
    }
    
    point draw_pos = get_sprite_center(s_ptr);
    point draw_size = get_sprite_dimensions(s_ptr);
    
    add_status_bitmap_effects(effect_manager);
    add_sector_brightness_bitmap_effect(effect_manager);
    
    draw_bitmap_with_effects(
        s_ptr->bitmap,
        draw_pos, draw_size,
        angle + s_ptr->angle, effect_manager
    );
    
    if(internal_effect_manager) {
        delete effect_manager;
    }
}


/* ----------------------------------------------------------------------------
 * Makes the mob swallow some of the opponents it has chomped on.
 * nr: Number of captured opponents to swallow.
 */
void mob::swallow_chomped_pikmin(const size_t nr) {

    size_t total = min(nr, chomping_pikmin.size());
    
    for(size_t p = 0; p < total; ++p) {
        chomping_pikmin[p]->set_health(false, false, 0.0f);
        chomping_pikmin[p]->dead = true;
        chomping_pikmin[p]->cause_spike_damage(this, true);
    }
    chomping_pikmin.clear();
}


/* ----------------------------------------------------------------------------
 * Makes a mob intend to face a new angle.
 * new_angle: Face this angle.
 * new_pos:   If this is not NULL, turn towards this point every frame, instead.
 */
void mob::face(const float new_angle, point* new_pos) {
    if(carry_info) return; //If it's being carried, it shouldn't rotate.
    intended_turn_angle = new_angle;
    intended_turn_pos = new_pos;
}


//Normally, the spirit's diameter is the enemy's. Multiply the spirit by this.
const float ENEMY_SPIRIT_SIZE_MULT = 0.7;
//Maximum diameter an enemy's spirit can be.
const float ENEMY_SPIRIT_MAX_SIZE = 128;
//Minimum diameter an enemy's spirit can be.
const float ENEMY_SPIRIT_MIN_SIZE = 16;

/* ----------------------------------------------------------------------------
 * Sets up stuff for the end of the mob's dying process.
 */
void mob::finish_dying() {
    if(dead) return;
    dead = true;
    
    if(type->category->id == MOB_CATEGORY_ENEMIES) {
        //TODO move this to the enemy class.
        enemy* e_ptr = (enemy*) this;
        if(e_ptr->ene_type->drops_corpse) {
            become_carriable(false);
            e_ptr->fsm.set_state(ENEMY_EXTRA_STATE_CARRIABLE_WAITING);
        }
        particle par(
            PARTICLE_TYPE_ENEMY_SPIRIT, pos,
            clamp(
                type->radius * 2 * ENEMY_SPIRIT_SIZE_MULT,
                ENEMY_SPIRIT_MIN_SIZE, ENEMY_SPIRIT_MAX_SIZE
            ),
            2, PARTICLE_PRIORITY_MEDIUM
        );
        par.bitmap = bmp_enemy_spirit;
        par.speed.x = 0;
        par.speed.y = -50;
        par.friction = 0.5;
        par.gravity = 0;
        par.color = al_map_rgb(255, 192, 255);
        particles.add(par);
    }
    
    release_chomped_pikmin();
}


/* ----------------------------------------------------------------------------
 * Checks if the mob is colliding against a floor. This only checks the
 * vertical aspect, i.e. Zs and heights.
 * z:      Z to use.
 * equals: If true, also checks if the Zs are equal, instead of just
 *   checking less than or greater than.
 */
bool mob::check_floor_collision(
    sector_floor* floor_ptr, const float z, const bool equals
) {
    if(equals) {
        if(z > floor_ptr->z) return false;
        if(z + type->height < floor_ptr->bottom_z) return false;
    } else {
        if(z >= floor_ptr->z) return false;
        if(z + type->height <= floor_ptr->bottom_z) return false;
    }
    return true;
}


/* ----------------------------------------------------------------------------
 * Makes the mob focus on m2.
 */
void mob::focus_on_mob(mob* m) {
    unfocus_from_mob();
    focused_mob = m;
}


/* ----------------------------------------------------------------------------
 * Returns the base speed for this mob.
 * This is overwritten by some child classes.
 */
float mob::get_base_speed() {
    return this->type->move_speed;
}


/* ----------------------------------------------------------------------------
 * Returns the actual location of the movement target.
 */
point mob::get_chase_target() {
    point p = chase_offset;
    if(chase_orig_coords) p += (*chase_orig_coords);
    return p;
}


/* ----------------------------------------------------------------------------
 * Returns the closest hitbox to a point, belonging to a mob's current frame
 * of animation and position.
 * p:      The point.
 * h_type: Type of hitbox. INVALID means any.
 * d:      Return the distance here, optionally.
 */
hitbox* mob::get_closest_hitbox(const point &p, const size_t h_type, dist* d) {
    sprite* s = anim.get_cur_sprite();
    if(!s) return NULL;
    hitbox* closest_hitbox = NULL;
    float closest_hitbox_dist = 0;
    
    for(size_t h = 0; h < s->hitboxes.size(); ++h) {
        hitbox* h_ptr = &s->hitboxes[h];
        if(h_type != INVALID && h_ptr->type != h_type) continue;
        
        point h_pos = rotate_point(h_ptr->pos, angle);
        float d =
            dist(
                point(p.x - pos.x, p.y - pos.y),
                h_pos
            ).to_float() - h_ptr->radius;
        if(closest_hitbox == NULL || d < closest_hitbox_dist) {
            closest_hitbox_dist = d;
            closest_hitbox = h_ptr;
        }
    }
    
    if(d) *d = closest_hitbox_dist;
    
    return closest_hitbox;
}


/* ----------------------------------------------------------------------------
 * Returns the hitbox in the current animation with
 * the specified number.
 */
hitbox* mob::get_hitbox(const size_t nr) {
    sprite* s = anim.get_cur_sprite();
    if(!s) return NULL;
    if(s->hitboxes.empty()) return NULL;
    return &s->hitboxes[nr];
}


/* ----------------------------------------------------------------------------
 * Returns how many Pikmin are currently latched on to this mob.
 */
size_t mob::get_latched_pikmin_amount() {
    size_t total = 0;
    for(size_t p = 0; p < pikmin_list.size(); ++p) {
        pikmin* p_ptr = pikmin_list[p];
        if(
            p_ptr->focused_mob == this &&
            p_ptr->connected_hitbox_nr != INVALID
        ) {
            total++;
        }
    }
    return total;
}


/* ----------------------------------------------------------------------------
 * Returns the total weight of the Pikmin that are currently
 * latched on to this mob.
 */
float mob::get_latched_pikmin_weight() {
    float total = 0;
    for(size_t p = 0; p < pikmin_list.size(); ++p) {
        pikmin* p_ptr = pikmin_list[p];
        if(
            p_ptr->focused_mob == this &&
            p_ptr->connected_hitbox_nr != INVALID
        ) {
            total += p_ptr->type->weight;
        }
    }
    return total;
}


/* ----------------------------------------------------------------------------
 * Returns where a sprite's center should be, for normal mob drawing routines.
 */
point mob::get_sprite_center(sprite* s) {
    point p;
    float co = cos(angle), si = sin(angle);
    p.x = pos.x + co * s->offset.x - si * s->offset.y;
    p.y = pos.y + si * s->offset.x + co * s->offset.y;
    return p;
}


/* ----------------------------------------------------------------------------
 * Returns what a sprite's dimensions should be,
 * for normal mob drawing routines.
 * s:     the sprite.
 * scale: variable to return the scale used to. Optional.
 */
point mob::get_sprite_dimensions(sprite* s, float* scale) {
    point dim;
    dim.x = s->file_size.x;
    dim.y = s->file_size.y;
    dim.x *= s->scale.x;
    dim.y *= s->scale.y;
    
    float sucking_mult = 1.0;
    float height_mult = 1 + z * MOB_SPRITE_Z_GROWTH_FACTOR;
    
    float final_scale = sucking_mult * height_mult;
    if(scale) *scale = final_scale;
    
    dim *= final_scale;
    return dim;
}


/* ----------------------------------------------------------------------------
 * Returns the current sprite of one of the status effects
 * that the mob is under.
 * bmp_scale: Returns the mob size's scale to apply to the image.
 */
ALLEGRO_BITMAP* mob::get_status_bitmap(float* bmp_scale) {
    *bmp_scale = 0.0f;
    for(size_t st = 0; st < this->statuses.size(); ++st) {
        status_type* t = this->statuses[st].type;
        if(t->animation_name.empty()) continue;
        sprite* sp = t->anim_instance.get_cur_sprite();
        if(!sp) return NULL;
        *bmp_scale = t->animation_mob_scale;
        return sp->bitmap;
    }
    return NULL;
}


/* ----------------------------------------------------------------------------
 * Checks if a mob is resistant to a list of hazards, given the list
 * of hazards.
 */
bool mob::is_resistant_to_hazards(vector<hazard*> &hazards) {
    size_t n_matches = 0;
    for(size_t h = 0; h < hazards.size(); ++h) {
        for(size_t r = 0; r < type->resistances.size(); ++r) {
            if(hazards[h] == type->resistances[r]) {
                n_matches++;
                break;
            }
        }
    }
    if(n_matches == hazards.size()) {
        //The mob can resist all
        //of these hazards!
        return true;
    }
    return false;
}


/* ----------------------------------------------------------------------------
 * Removes all particle generators with the given ID.
 */
void mob::remove_particle_generator(const size_t id) {
    for(size_t g = 0; g < particle_generators.size();) {
        if(particle_generators[g].id == id) {
            particle_generators.erase(particle_generators.begin() + g);
        } else {
            ++g;
        }
    }
}


/* ----------------------------------------------------------------------------
 * Removes a mob from its leader's group.
 */
void mob::leave_group() {
    if(!following_group) return;
    
    mob* group_leader = following_group;
    
    group_leader->group->members.erase(
        find(
            group_leader->group->members.begin(),
            group_leader->group->members.end(),
            this
        )
    );
    
    group_leader->group->init_spots(this);
    
    //Check if there are no more members of the same type.
    //If not, choose a new type as the standby!
    bool last_of_its_type = true;
    for(size_t m = 0; m < group_leader->group->members.size(); ++m) {
        if(
            group_leader->group->members[m]->subgroup_type_ptr ==
            subgroup_type_ptr
        ) {
            last_of_its_type = false;
            break;
        }
    }
    if(last_of_its_type) {
        group_leader->group->set_next_cur_standby_type(false);
    }
    
    following_group = NULL;
}


/* ----------------------------------------------------------------------------
 * Safely releases all chomped Pikmin.
 */
void mob::release_chomped_pikmin() {
    for(size_t p = 0; p < chomping_pikmin.size(); ++p) {
        chomping_pikmin[p]->fsm.run_event(MOB_EVENT_RELEASED);
    }
    chomping_pikmin.clear();
}


/* ----------------------------------------------------------------------------
 * Respawns an object back to its home.
 */
void mob::respawn() {
    pos = home;
    center_sector = get_sector(pos, NULL, true);
    ground_floor = &(center_sector->floors[center_sector->n_floors - 1]);
    z = ground_floor->z + 100;
}


/* ----------------------------------------------------------------------------
 * Sets the mob's animation.
 * nr:        Animation number.
 *   It's the animation instance number from the database.
 * pre_named: If true, the animation has already been named in-engine.
 */
void mob::set_animation(const size_t nr, const bool pre_named) {
    if(nr >= type->anims.animations.size()) return;
    
    size_t final_nr;
    if(pre_named) {
        if(anim.anim_db->pre_named_conversions.size() <= nr) return;
        final_nr = anim.anim_db->pre_named_conversions[nr];
    } else {
        final_nr = nr;
    }
    
    if(final_nr == INVALID) {
        log_error(
            "Mob " + this->type->name + " tried to switch from " +
            (
                anim.cur_anim ? "animation \"" + anim.cur_anim->name + "\"" :
                "no animation"
            ) +
            " to a non-existent one (with the internal"
            " number of " + i2s(nr) + ")!"
        );
        return;
    }
    
    animation* new_anim = anim.anim_db->animations[final_nr];
    anim.cur_anim = new_anim;
    anim.start();
}


/* ----------------------------------------------------------------------------
 * Changes a mob's health, relatively or absolutely.
 * add:    If true, change is relative to the current value
 *   (i.e. add or subtract from current health).
 *   If false, simply set to that number.
 * ratio:  If true, the specified value represents the max health ratio.
 *   If false, it's the number in HP.
 * amount: Health amount.
 */
void mob::set_health(const bool add, const bool ratio, const float amount) {
    float change = amount;
    if(ratio) change = type->max_health * amount;
    float base_nr = 0;
    if(add) base_nr = health;
    
    health = clamp(base_nr + change, 0.0f, type->max_health);
}


/* ----------------------------------------------------------------------------
 * Changes the timer's time and interval.
 * time: New time.
 */
void mob::set_timer(const float time) {
    script_timer.duration = time;
    script_timer.start();
}


/* ----------------------------------------------------------------------------
 * Sets a script variable's value.
 * name:  The variable's name
 * value: The variable's new value.
 */
void mob::set_var(const string &name, const string &value) {
    vars[name] = value;
}


/* ----------------------------------------------------------------------------
 * Should this mob attack v? Teams and other factors are used to decide this.
 */
bool mob::should_attack(mob* v) {
    if(team == v->team && team != MOB_TEAM_NONE) {
        //Teammates can't hurt each other.
        return false;
    }
    if(v->team == MOB_TEAM_DECORATION) {
        //Decoration isn't meant to be hurt.
        return false;
    }
    if(type->is_projectile && !v->type->projectiles_can_damage) {
        //Projectiles can't hurt those which are invulnerable to them.
        return false;
    }
    if(
        team == MOB_TEAM_OBSTACLE &&
        (v->team < MOB_TEAM_PLAYER_1 || v->team > MOB_TEAM_PLAYER_4)
    ) {
        //Obstacles can only hurt Pikmin and leaders.
        return false;
    }
    if(
        v->team == MOB_TEAM_OBSTACLE &&
        type->category->id != MOB_CATEGORY_PIKMIN &&
        !type->is_projectile
    ) {
        //Only Pikmin and projectiles can hurt obstacles.
        return false;
    }
    return true;
}


/* ----------------------------------------------------------------------------
 * Sets up stuff for the beginning of the mob's death process.
 */
void mob::start_dying() {
    set_health(false, false, 0.0f);
    
    stop_chasing();
    intended_turn_angle = angle;
    intended_turn_pos = NULL;
    gravity_mult = 1.0;
    
    particle p(PARTICLE_TYPE_BITMAP, pos, 64, 1.5, PARTICLE_PRIORITY_LOW);
    p.bitmap = bmp_sparkle;
    p.color = al_map_rgb(255, 192, 192);
    particle_generator pg(0, p, 25);
    pg.number_deviation = 5;
    pg.angle = 0;
    pg.angle_deviation = M_PI;
    pg.total_speed = 100;
    pg.total_speed_deviation = 40;
    pg.duration_deviation = 0.5;
    pg.emit(particles);
}


/* ----------------------------------------------------------------------------
 * Makes a mob not follow any target any more.
 */
void mob::stop_chasing() {
    chasing = false;
    reached_destination = false;
    chase_teleport_z = NULL;
    
    speed.x = speed.y = 0;
}


/* ----------------------------------------------------------------------------
 * Makes the mob follow a game tick.
 * This basically calls sub-tickers.
 * Think of it this way: when you want to go somewhere,
 * you first think about rotating your body to face that
 * point, and then think about moving your legs.
 * Then, the actual physics go into place, your nerves
 * send signals to the muscles, and gravity, intertia, etc.
 * take over the rest, to make you move.
 */
void mob::tick() {
    tick_brain();
    tick_physics();
    tick_misc_logic();
    tick_animation();
    tick_script();
    tick_class_specifics();
}


/* ----------------------------------------------------------------------------
 * Ticks one game frame into the mob's animations.
 */
void mob::tick_animation() {
    float mult = 1.0f;
    for(size_t s = 0; s < this->statuses.size(); ++s) {
        mult *= this->statuses[s].type->anim_speed_multiplier;
    }
    
    vector<size_t> frame_signals;
    bool finished_anim = anim.tick(delta_t* mult, &frame_signals);
    
    if(finished_anim) {
        fsm.run_event(MOB_EVENT_ANIMATION_END);
    }
    for(size_t s = 0; s < frame_signals.size(); ++s) {
        fsm.run_event(MOB_EVENT_FRAME_SIGNAL, &frame_signals[s]);
    }
    
    for(size_t h = 0; h < hit_opponents.size();) {
        hit_opponents[h].first -= delta_t;
        if(hit_opponents[h].first <= 0.0f) {
            hit_opponents.erase(hit_opponents.begin() + h);
        } else {
            ++h;
        }
    }
}


/* ----------------------------------------------------------------------------
 * Ticks the mob's brain for the next frame.
 * This has nothing to do with the mob's individual script.
 * This is related to mob-global things, like
 * thinking about where to move next and such.
 */
void mob::tick_brain() {
    //Chasing a target.
    if(chasing && !chase_teleport && speed_z == 0) {
    
        //Calculate where the target is.
        point final_target_pos = get_chase_target();
        
        if(!chase_teleport) {
        
            if(
                dist(pos, final_target_pos) > chase_target_dist
            ) {
                //If it still hasn't reached its target
                //(or close enough to the target),
                //time to make it think about how to get there.
                
                //Let the mob think about facing the actual target.
                face(get_angle(pos, final_target_pos), NULL);
                
            } else {
                //Reached the location. The mob should now think
                //about stopping.
                
                chase_speed = 0;
                reached_destination = true;
                fsm.run_event(MOB_EVENT_REACHED_DESTINATION);
            }
            
        }
    }
}


/* ----------------------------------------------------------------------------
 * Code specific for each class. Meant to be overwritten by the child classes.
 */
void mob::tick_class_specifics() {
}


/* ----------------------------------------------------------------------------
 * Performs some logic code for this game frame.
 */
void mob::tick_misc_logic() {
    invuln_period.tick(delta_t);
    
    for(size_t s = 0; s < this->statuses.size(); ++s) {
        statuses[s].tick(delta_t);
        set_health(
            true, true,
            statuses[s].type->health_change_ratio * delta_t
        );
    }
    delete_old_status_effects();
    
    for(size_t g = 0; g < particle_generators.size();) {
        particle_generators[g].tick(delta_t, particles);
        if(particle_generators[g].emission_interval == 0) {
            particle_generators.erase(particle_generators.begin() + g);
        } else {
            ++g;
        }
    }
}


/* ----------------------------------------------------------------------------
 * Ticks the mob's actual physics procedures:
 * falling because of gravity, moving forward, etc.
 */
void mob::tick_physics() {
    if(!ground_floor) {
        //Object is placed out of bounds.
        return;
    }
    
    //Movement.
    bool finished_moving = false;
    bool doing_slide = false;
    
    point new_pos = pos;
    float new_z = z;
    sector_floor* new_ground_floor = ground_floor;
    float pre_move_ground_z = ground_floor->z;
    
    point move_speed = speed;
    
    float radius_to_use = type->radius;
    
    //Change the facing angle to the angle the mob wants to face.
    if(angle > M_PI)  angle -= M_PI * 2;
    if(angle < -M_PI) angle += M_PI * 2;
    if(intended_turn_pos) {
        intended_turn_angle = get_angle(pos, *intended_turn_pos);
    }
    if(intended_turn_angle > M_PI)  intended_turn_angle -= M_PI * 2;
    if(intended_turn_angle < -M_PI) intended_turn_angle += M_PI * 2;
    
    float angle_dif = intended_turn_angle - angle;
    if(angle_dif > M_PI)  angle_dif -= M_PI * 2;
    if(angle_dif < -M_PI) angle_dif += M_PI * 2;
    
    float movement_speed_mult = 1.0f;
    for(size_t s = 0; s < this->statuses.size(); ++s) {
        movement_speed_mult *= this->statuses[s].type->speed_multiplier;
    }
    
    angle +=
        sign(angle_dif) * min(
            (double) (type->rotation_speed * movement_speed_mult * delta_t),
            (double) fabs(angle_dif)
        );
        
    if(chasing) {
        point final_target_pos = get_chase_target();
        
        if(chase_teleport) {
            sector* sec =
                get_sector(final_target_pos, NULL, true);
            if(!sec) {
                //No sector, invalid teleport. No move.
                return;
                
            } else {
                if(chase_teleport_z) {
                    z = *chase_teleport_z;
                }
                ground_floor = &(sec->floors[0]);
                center_sector = sec;
                if(sec->n_floors > 1 && sec->floors[1].z < z) {
                    ground_floor = &(sec->floors[1]);
                }
                speed.x = speed.y = speed_z = 0;
                pos = final_target_pos;
                finished_moving = true;
            }
            
        } else {
        
            //Make it go to the direction it wants.
            float d = dist(pos, final_target_pos).to_float();
            
            float move_amount =
                min(
                    (double) (d / delta_t),
                    (double) chase_speed * movement_speed_mult
                );
                
            bool can_free_move = chase_free_move || d <= 10.0;
            
            float movement_angle =
                can_free_move ?
                get_angle(pos, final_target_pos) :
                angle;
                
            move_speed.x = cos(movement_angle) * move_amount;
            move_speed.y = sin(movement_angle) * move_amount;
        }
    }
    
    
    //If another mob is pushing it.
    if(push_amount != 0.0f) {
        //Overly-aggressive pushing results in going through walls.
        //Let's place a cap.
        push_amount =
            min(push_amount, (float) ((type->radius / delta_t) - chase_speed));
        move_speed.x +=
            cos(push_angle) * (push_amount + MOB_PUSH_EXTRA_AMOUNT);
        move_speed.y +=
            sin(push_angle) * (push_amount + MOB_PUSH_EXTRA_AMOUNT);
    }
    
    push_amount = 0;
    bool touched_wall = false;
    
    
    //Try placing it in the place it should be at, judging
    //from the movement speed.
    while(!finished_moving) {
    
        if(move_speed.x == 0 && move_speed.y == 0) break;
        
        //Start by checking sector collisions. Or rather, floor collisions.
        //For this, we will only check if the mob is intersecting
        //with any edge. With this, we trust that mobs can't go so fast
        //that they're fully on one side of an edge in one frame,
        //and the other side on the next frame.
        //It's pretty naive...but it works!
        bool successful_move = true;
        
        new_pos.x = pos.x + delta_t* move_speed.x;
        new_pos.y = pos.y + delta_t* move_speed.y;
        new_z = z;
        
        new_ground_floor = ground_floor;
        set<edge*> intersecting_edges;
        
        //Get the sector the mob's center will be on.
        sector* new_center_sector = get_sector(new_pos, NULL, true);
        sector_floor* step_floor = &(new_center_sector->floors[0]);
        if(new_center_sector->n_floors > 1 && new_center_sector->floors[1].z <= z) {
            step_floor = &(new_center_sector->floors[1]);
        }
        
        if(!new_center_sector) {
            //Out of bounds. No movement.
            break;
        }
        
        new_ground_floor = step_floor;
        
        //If it'd end up inside a floor, refuse the move.
        bool goes_inside = false;
        for(unsigned char f = 0; f < new_center_sector->n_floors; ++f) {
            if(check_floor_collision(&new_center_sector->floors[f], z, false)) {
                goes_inside = true;
                break;
            }
        }
        if(goes_inside) break;
        
        //Before checking the edges, let's consult the blockmap and look at
        //the edges in the same block the mob is on.
        //This way, we won't check for edges that are really far away.
        //Use the bounding box to know which blockmap blocks the mob will be on.
        size_t bx1 = cur_area_data.bmap.get_col(new_pos.x - radius_to_use);
        size_t bx2 = cur_area_data.bmap.get_col(new_pos.x + radius_to_use);
        size_t by1 = cur_area_data.bmap.get_row(new_pos.y - radius_to_use);
        size_t by2 = cur_area_data.bmap.get_row(new_pos.y + radius_to_use);
        
        if(
            bx1 == INVALID || bx2 == INVALID ||
            by1 == INVALID || by2 == INVALID
        ) {
            //Somehow out of bounds. No movement.
            break;
        }
        
        float move_angle;
        float total_move_speed;
        coordinates_to_angle(
            move_speed, &move_angle, &total_move_speed
        );
        
        //Angle to slide towards.
        float slide_angle = move_angle;
        //Difference between the movement angle and the slide.
        float slide_angle_dif = 0;
        
        edge* e_ptr = NULL;
        
        //Go through the blocks, to find intersections, and set up some things.
        for(size_t bx = bx1; bx <= bx2; ++bx) {
            for(size_t by = by1; by <= by2; ++by) {
            
                vector<edge*>* edges = &cur_area_data.bmap.edges[bx][by];
                
                for(size_t e = 0; e < edges->size(); ++e) {
                
                    e_ptr = (*edges)[e];
                    bool is_edge_blocking = false;
                    
                    if(
                        !circle_intersects_line(
                            new_pos, radius_to_use,
                            point(
                                e_ptr->vertexes[0]->x, e_ptr->vertexes[0]->y
                            ),
                            point(
                                e_ptr->vertexes[1]->x, e_ptr->vertexes[1]->y
                            ),
                            NULL, NULL
                        )
                    ) {
                        continue;
                    }
                    
                    if(e_ptr->sectors[0] && e_ptr->sectors[1]) {
                    
                        if(
                            e_ptr->sectors[0]->type == SECTOR_TYPE_BLOCKING ||
                            e_ptr->sectors[1]->type == SECTOR_TYPE_BLOCKING
                        ) {
                            is_edge_blocking = true;
                        }
                        
                        if(!is_edge_blocking) {
                            if(
                                e_ptr->sectors[0]->floors[e_ptr->sectors[0]->n_floors - 1].z < z &&
                                e_ptr->sectors[1]->floors[e_ptr->sectors[1]->n_floors - 1].z < z
                            ) {
                                //An edge whose floors are below the mob?
                                //No collision here.
                                continue;
                            }
                        }
                        
                        //Figure out what is the tallest floor the mob can step on.
                        sector_floor* tallest_floor = NULL; //Tallest floor of the two sectors.
                        if(
                            e_ptr->sectors[0]->type == SECTOR_TYPE_BLOCKING
                        ) {
                            if(
                                e_ptr->sectors[1]->n_floors == 2 &&
                                e_ptr->sectors[1]->floors[1].z < z
                            ) {
                                tallest_floor = &(e_ptr->sectors[1]->floors[1]);
                            } else {
                                tallest_floor = &(e_ptr->sectors[1]->floors[0]);
                            }
                            
                        } else if(
                            e_ptr->sectors[1]->type == SECTOR_TYPE_BLOCKING
                        ) {
                            if(
                                e_ptr->sectors[0]->n_floors == 2 &&
                                e_ptr->sectors[0]->floors[1].z < z
                            ) {
                                tallest_floor = &(e_ptr->sectors[0]->floors[1]);
                            } else {
                                tallest_floor = &(e_ptr->sectors[0]->floors[0]);
                            }
                            
                        } else {
                            for(unsigned char s = 0; s < 2; ++s) {
                                for(unsigned char f = 0; f < e_ptr->sectors[s]->n_floors; ++f) {
                                    if(e_ptr->sectors[s]->floors[f].bottom_z > z + type->height) {
                                        //This floor is too high up. The mob won't walk on it.
                                        continue;
                                    }
                                    if(
                                        !tallest_floor ||
                                        e_ptr->sectors[s]->floors[f].z > tallest_floor->z
                                    ) {
                                        tallest_floor = &(e_ptr->sectors[s]->floors[f]);
                                    }
                                }
                            }
                        }
                        
                        if(
                            tallest_floor->z > new_ground_floor->z &&
                            tallest_floor->z <= z
                        ) {
                            new_ground_floor = tallest_floor;
                        }
                        
                        //Check if it can go up this step.
                        //It can go up this step if the floor is within
                        //stepping distance of the mob's current Z,
                        //and if this step is larger than any step
                        //encountered of all edges crossed.
                        if(
                            !was_thrown &&
                            tallest_floor->z <= z + SECTOR_STEP &&
                            tallest_floor->z > step_floor->z
                        ) {
                            step_floor = tallest_floor;
                        }
                        
                        //Add this edge to the list of intersections, then.
                        intersecting_edges.insert(e_ptr);
                        
                    } else {
                    
                        //If we're on the edge of out-of-bounds geometry,
                        //block entirely.
                        successful_move = false;
                        break;
                        
                    }
                    
                }
                
                if(!successful_move) break;
            }
            
            if(!successful_move) break;
        }
        
        if(!successful_move) break;
        
        if(step_floor->z > new_ground_floor->z) {
            new_ground_floor = step_floor;
        }
        
        if(z < step_floor->z) new_z = step_floor->z;
        
        //Check wall angles and heights to check which of these edges
        //really are wall collisions.
        for(
            auto e = intersecting_edges.begin();
            e != intersecting_edges.end(); e++
        ) {
        
            e_ptr = *e;
            bool is_edge_wall = false;
            unsigned char wall_sector = 0;
            
            for(unsigned char s = 0; s < 2; s++) {
                if(e_ptr->sectors[s]->type == SECTOR_TYPE_BLOCKING) {
                    is_edge_wall = true;
                    wall_sector = s;
                }
            }
            
            if(!is_edge_wall) {
                for(unsigned char s = 0; s < 2; s++) {
                    for(unsigned char f = 0; f < e_ptr->sectors[s]->n_floors; ++f) {
                        if(check_floor_collision(&(e_ptr->sectors[s]->floors[f]), new_z, false)) {
                            is_edge_wall = true;
                            wall_sector = s;
                        }
                    }
                }
            }
            
            //This isn't a wall... Get out of here, faker.
            if(!is_edge_wall) continue;
            
            //If both floors of this edge are above the mob...
            //then what does that mean? That the mob is under the ground?
            //Nonsense! Throw this edge away!
            //It's a false positive, and the only
            //way for it to get caught is if it's behind a more logical
            //edge that we actually did collide against.
            if(e_ptr->sectors[0] && e_ptr->sectors[1]) {
                if(
                    (
                        e_ptr->sectors[0]->floors[0].z > new_z ||
                        e_ptr->sectors[0]->type == SECTOR_TYPE_BLOCKING
                    ) &&
                    (
                        e_ptr->sectors[1]->floors[0].z > new_z ||
                        e_ptr->sectors[1]->type == SECTOR_TYPE_BLOCKING
                    )
                ) {
                    continue;
                }
            }
            
            successful_move = false;
            touched_wall = true;
            //Ok, at this point, there's clearly been a collision,
            //so let's work out what wall the mob will slide on.
            
            //The wall's normal is the direction the wall is facing.
            //i.e. the direction from the top floor to the bottom floor.
            //We know which side of an edge is which sector because of
            //the vertexes. Imagine you're in first person view,
            //following the edge as a line on the ground.
            //You start on vertex 0 and face vertex 1.
            //Sector 0 will always be on your left.
            if(!doing_slide) {
            
                float wall_normal;
                float wall_angle =
                    get_angle(
                        point(e_ptr->vertexes[0]->x, e_ptr->vertexes[0]->y),
                        point(e_ptr->vertexes[1]->x, e_ptr->vertexes[1]->y)
                    );
                    
                if(wall_sector == 0) {
                    wall_normal = normalize_angle(wall_angle + M_PI_2);
                } else {
                    wall_normal = normalize_angle(wall_angle - M_PI_2);
                }
                
                float nd = get_angle_cw_dif(wall_normal, move_angle);
                if(nd < M_PI_2 || nd > M_PI + M_PI_2) {
                    //If the difference between the movement and the wall's
                    //normal is this, that means we came FROM the wall.
                    //No way! There has to be an edge that makes more sense.
                    continue;
                }
                
                //If we were to slide on this edge, this would be
                //the slide angle.
                float tentative_slide_angle;
                if(nd < M_PI) {
                    //Coming in from the "left" of the normal. Slide right.
                    tentative_slide_angle = wall_normal + M_PI_2;
                } else {
                    //Coming in from the "right" of the normal. Slide left.
                    tentative_slide_angle = wall_normal - M_PI_2;
                }
                
                float sd =
                    get_angle_smallest_dif(move_angle, tentative_slide_angle);
                if(sd > slide_angle_dif) {
                    slide_angle_dif = sd;
                    slide_angle = tentative_slide_angle;
                }
                
            }
        }
        
        //If the mob is just slamming against the wall head-on, perpendicularly,
        //then forget any idea about sliding.
        //It'd just be awkwardly walking in place.
        if(!successful_move && slide_angle_dif > M_PI_2 - 0.05) {
            doing_slide = true;
        }
        
        
        //We're done here. If the move was unobstructed, good, go there.
        //If not, we'll use the info we gathered before to calculate sliding,
        //and try again.
        
        if(successful_move) {
            //Good news, the mob can move to this new spot freely.
            pos = new_pos;
            z = new_z;
            ground_floor = new_ground_floor;
            center_sector = new_center_sector;
            finished_moving = true;
            
        } else {
        
            //Try sliding.
            if(doing_slide) {
                //We already tried sliding, and we still hit something...
                //Let's just stop completely. This mob can't go forward.
                speed.x = 0;
                speed.y = 0;
                finished_moving = true;
                
            } else {
            
                doing_slide = true;
                //To limit the speed, we should use a cross-product of the
                //movement and slide vectors.
                //But nuts to that, this is just as nice, and a lot simpler!
                total_move_speed *= 1 - (slide_angle_dif / M_PI);
                move_speed =
                    angle_to_coordinates(
                        slide_angle, total_move_speed
                    );
                    
            }
            
        }
        
    }
    
    if(touched_wall) {
        fsm.run_event(MOB_EVENT_TOUCHED_WALL);
    }
    
    
    //Vertical movement.
    
    //If the current floor is one step (or less) below
    //the previous floor, just instantly go down the step.
    if(
        pre_move_ground_z - ground_floor->z <= SECTOR_STEP &&
        z == pre_move_ground_z
    ) {
        z = ground_floor->z;
    }
    
    //Landing on a floor. Also, check for hazards and the bottomless pit.
    hazard* new_on_hazard = NULL;
    z += delta_t* speed_z;
    if(z <= ground_floor->z) {
        z = ground_floor->z;
        speed_z = 0;
        was_thrown = false;
        fsm.run_event(MOB_EVENT_LANDED);
        
        if(ground_floor->bottom_z == -INFINITY) {
            if(ground_floor->s_ptr->is_bottomless_pit) {
                fsm.run_event(MOB_EVENT_BOTTOMLESS_PIT);
            }
            
            for(size_t h = 0; h < ground_floor->s_ptr->hazards.size(); ++h) {
                fsm.run_event(
                    MOB_EVENT_TOUCHED_HAZARD,
                    (void*) ground_floor->s_ptr->hazards[h]
                );
                new_on_hazard = ground_floor->s_ptr->hazards[h];
            }
        }
    }
    
    //Due to framerate imperfections, thrown Pikmin/leaders can reach higher
    //than intended. z_cap forces a cap. FLT_MAX = no cap.
    if(speed_z <= 0) {
        z_cap = FLT_MAX;
    } else if(z_cap < FLT_MAX) {
        z = min(z, z_cap);
    }
    
    //Gravity.
    if(gravity_mult > 0) {
        if(z > ground_floor->z) {
            speed_z += delta_t* gravity_mult * GRAVITY_ADDER;
        } else {
            speed_z = 0;
        }
    } else {
        speed_z += delta_t* gravity_mult * GRAVITY_ADDER;
    }
    
    //On a sector that has a hazard that is not on the floor.
    if(z > ground_floor->z && !ground_floor->s_ptr->hazard_floor) {
        for(size_t h = 0; h < ground_floor->s_ptr->hazards.size(); ++h) {
            fsm.run_event(
                MOB_EVENT_TOUCHED_HAZARD,
                (void*) ground_floor->s_ptr->hazards[h]
            );
            new_on_hazard = ground_floor->s_ptr->hazards[h];
        }
    }
    
    if(new_on_hazard != on_hazard && on_hazard != NULL) {
        fsm.run_event(
            MOB_EVENT_LEFT_HAZARD,
            (void*) on_hazard
        );
    }
    on_hazard = new_on_hazard;
    
    //Quick panic check: if it's somehow inside the ground, pop it out.
    z = max(z, ground_floor->z);
}


/* ----------------------------------------------------------------------------
 * Checks general events in the mob's script for this frame.
 */
void mob::tick_script() {
    if(!fsm.cur_state) return;
    
    //Timer events.
    mob_event* timer_ev = q_get_event(this, MOB_EVENT_TIMER);
    if(script_timer.duration > 0) {
        if(script_timer.time_left > 0) {
            script_timer.tick(delta_t);
            if(script_timer.time_left == 0.0f && timer_ev) {
                timer_ev->run(this);
            }
        }
    }
    
    //Has it reached its home?
    mob_event* reach_dest_ev = q_get_event(this, MOB_EVENT_REACHED_DESTINATION);
    if(reach_dest_ev && reached_destination) {
        reach_dest_ev->run(this);
    }
    
    //Is it dead?
    if(health <= 0 && type->max_health != 0) {
        fsm.run_event(MOB_EVENT_DEATH, this);
    }
    
    //Itch event.
    if(type->itch_damage > 0 || type->itch_time > 0) {
        itch_time += delta_t;
        mob_event* itch_ev = q_get_event(this, MOB_EVENT_ITCH);
        if(
            itch_ev &&
            itch_damage > type->itch_damage && itch_time > type->itch_time
        ) {
            itch_ev->run(this);
            itch_damage = 0;
            itch_time = 0;
        }
    }
    
    //Health regeneration.
    if(health > 0) {
        set_health(true, false, type->health_regen * delta_t);
    }
    
    //Check if it got whistled.
    mob_event* whistled_ev = q_get_event(this, MOB_EVENT_WHISTLED);
    if(whistling && whistled_ev) {
        if(dist(pos, leader_cursor_w) <= whistle_radius) {
            whistled_ev->run(this);
        }
    }
    
    //Following a leader.
    if(following_group) {
        mob_event* spot_near_ev = q_get_event(this, MOB_EVENT_SPOT_IS_NEAR);
        mob_event* spot_far_ev =  q_get_event(this, MOB_EVENT_SPOT_IS_FAR);
        
        if(spot_near_ev || spot_far_ev) {
            point final_pos =
                following_group->group->anchor +
                following_group->group->get_spot_offset(
                    group_spot_index
                );
            dist d(pos, final_pos);
            if(spot_far_ev && d >= 5) {
                spot_far_ev->run(this, (void*) &final_pos);
            } else if(spot_near_ev && d < 5) {
                spot_near_ev->run(this);
            }
        }
    }
    
    //Focused on a mob.
    if(focused_mob) {
    
        if(focused_mob->health <= 0) {
            fsm.run_event(MOB_EVENT_FOCUS_DIED);
            fsm.run_event(MOB_EVENT_FOCUS_OFF_REACH);
        }
        
        //We have to recheck if the focused mob is not NULL, because
        //sending MOB_EVENT_FOCUS_DIED could've set this to NULL.
        if(focused_mob) {
        
            mob* focus = focused_mob;
            
            mob_event* for_ev =
                q_get_event(this, MOB_EVENT_FOCUS_OFF_REACH);
                
            if(far_reach != INVALID && for_ev) {
                dist d(pos, focus->pos);
                float face_diff =
                    get_angle_smallest_dif(
                        angle,
                        get_angle(pos, focus->pos)
                    );
                    
                mob_type::reach_struct* r_ptr =
                    &type->reaches[far_reach];
                if(
                    (
                        d > r_ptr->radius_1 +
                        (type->radius + focus->type->radius) ||
                        face_diff > r_ptr->angle_1 / 2.0f
                    ) && (
                        d > r_ptr->radius_2 +
                        (type->radius + focus->type->radius) ||
                        face_diff > r_ptr->angle_2 / 2.0f
                    )
                    
                ) {
                    for_ev->run(this);
                }
                
            }
        }
        
        if(focused_mob) {
            if(!focused_mob->carry_info) {
                fsm.run_event(MOB_EVENT_FOCUSED_MOB_UNCARRIABLE);
            }
        }
        
    }
    
    //Far away from home.
    mob_event* far_from_home_ev = q_get_event(this, MOB_EVENT_FAR_FROM_HOME);
    if(far_from_home_ev) {
        dist d(pos, home);
        if(d >= type->territory_radius) {
            far_from_home_ev->run(this);
        }
    }
    
    //Being carried, but has an obstacle.
    if(carry_info) {
        for(
            auto o = carry_info->obstacle_ptrs.begin();
            o != carry_info->obstacle_ptrs.end();
            ++o
        ) {
            if((*o)->health == 0) {
                fsm.run_event(MOB_EVENT_CARRY_BEGIN_MOVE);
                break;
            }
        }
    }
    
    //Tick event.
    fsm.run_event(MOB_EVENT_ON_TICK);
}


/* ----------------------------------------------------------------------------
 * Makes the mob lose focus on its currently focused mob.
 */
void mob::unfocus_from_mob() {
    focused_mob = nullptr;
}


bool mob::can_receive_status(status_type* s) { return false; };
void mob::receive_disable_from_status(const unsigned char flags) {}
void mob::receive_flailing_from_status() {}
void mob::receive_panic_from_status() {}
void mob::lose_panic_from_status() {}
void mob::change_maturity_amount_from_status(const int amount) {}


mob::~mob() {
    if(carry_info) delete carry_info;
    if(group) delete group;
}



/* ----------------------------------------------------------------------------
 * Creates a structure with info about a carrying spot.
 */
carrier_spot_struct::carrier_spot_struct(const point &pos) :
    state(CARRY_SPOT_FREE),
    pos(pos),
    pik_ptr(NULL) {
    
}


/* ----------------------------------------------------------------------------
 * Creates a structure with info about carrying.
 * m:             The mob this info belongs to.
 * max_carriers:  The maximum number of carrier Pikmin.
 * carry_to_ship: If true, this mob is delivered to a ship. Otherwise, an Onion.
 */
carry_info_struct::carry_info_struct(mob* m, const bool carry_to_ship) :
    m(m),
    carry_to_ship(carry_to_ship),
    cur_carrying_strength(0),
    cur_n_carriers(0),
    go_straight(false),
    stuck_state(0),
    is_moving(false) {
    
    for(size_t c = 0; c < m->type->max_carriers; ++c) {
        float angle = (M_PI * 2) / m->type->max_carriers * c;
        point p(
            cos(angle) * (m->type->radius + standard_pikmin_radius),
            sin(angle) * (m->type->radius + standard_pikmin_radius)
        );
        spot_info.push_back(carrier_spot_struct(p));
    }
}


/* ----------------------------------------------------------------------------
 * Returns the speed at which the object should move, given the carrier Pikmin.
 */
float carry_info_struct::get_speed() {
    float max_speed = 0;
    
    //Begin by obtaining the average walking speed of the carriers.
    for(size_t s = 0; s < spot_info.size(); ++s) {
        carrier_spot_struct* s_ptr = &spot_info[s];
        
        if(s_ptr->state != CARRY_SPOT_USED) continue;
        
        pikmin* p_ptr = (pikmin*) s_ptr->pik_ptr;
        max_speed += p_ptr->get_base_speed();
    }
    max_speed /= cur_n_carriers;
    
    //If the object has all carriers, the Pikmin move as fast
    //as possible, which looks bad, since they're not jogging,
    //they're carrying. Let's add a penalty for the weight...
    max_speed *= (1 - carrying_speed_weight_mult * m->type->weight);
    //...and a global carrying speed penalty.
    max_speed *= carrying_speed_max_mult;
    
    //The closer the mob is to having full carriers,
    //the closer to the max speed we get.
    //The speed goes from carrying_speed_base_mult (0 carriers)
    //to max_speed (all carriers).
    return
        max_speed * (
            carrying_speed_base_mult +
            (cur_n_carriers / (float) spot_info.size()) *
            (1 - carrying_speed_base_mult)
        );
}


/* ----------------------------------------------------------------------------
 * Returns true if all spots are reserved. False otherwise.
 */
bool carry_info_struct::is_full() {
    for(size_t s = 0; s < spot_info.size(); ++s) {
        if(spot_info[s].state == CARRY_SPOT_FREE) return false;
    }
    return true;
}


/* ----------------------------------------------------------------------------
 * Deletes a carrier info structure.
 */
carry_info_struct::~carry_info_struct() {
    //TODO
}



/* ----------------------------------------------------------------------------
 * Creates a new group information struct.
 */
group_info::group_info(mob* leader_ptr) :
    radius(0),
    anchor(leader_ptr->pos),
    transform(identity_transform),
    cur_standby_type(nullptr),
    follow_mode(false) {
}


/* ----------------------------------------------------------------------------
 * (Re-)Initializes the group spots. This resizes it to the current number
 * of group members. Any old group members are moved to the appropriate
 * new spot.
 * new_mob_ptr: If this initialization is because a new mob entered
 *   or left the group, this should point to said mob.
 */
void group_info::init_spots(mob* affected_mob_ptr) {
    if(members.empty()) {
        spots.clear();
        radius = 0;
        return;
    }
    
    //First, backup the old mob indexes.
    vector<mob*> old_mobs;
    old_mobs.resize(spots.size());
    for(size_t m = 0; m < spots.size(); ++m) {
        old_mobs[m] = spots[m].mob_ptr;
    }
    
    //Now, rebuild the spots. Let's draw wheels from the center, for now.
    struct alpha_spot {
        point pos;
        dist distance_to_rightmost;
        alpha_spot(const point &p) :
            pos(p) { }
    };
    
    vector<alpha_spot> alpha_spots;
    size_t current_wheel = 1;
    radius = standard_pikmin_radius;
    
    //Center spot first.
    alpha_spots.push_back(alpha_spot(point()));
    
    while(alpha_spots.size() < members.size()) {
    
        //First, calculate how far the center
        //of these spots are from the central spot.
        float dist_from_center =
            standard_pikmin_radius * current_wheel + //Spots.
            GROUP_SPOT_INTERVAL * current_wheel; //Interval between spots.
            
        /* Now we need to figure out what's the angular distance
         * between each spot. For that, we need the actual diameter
         * (distance from one point to the other),
         * and the central distance, which is distance between the center
         * and the middle of two spots.
         */
        
        /* We can get the middle distance because we know the actual diameter,
         * which should be the size of a Pikmin and one interval unit,
         * and we know the distance from one spot to the center.
         */
        float actual_diameter =
            standard_pikmin_radius * 2.0 + GROUP_SPOT_INTERVAL;
            
        //Just calculate the remaining side of the triangle, now that we know
        //the hypotenuse and the actual diameter (one side of the triangle).
        float middle_distance =
            sqrt(
                (dist_from_center * dist_from_center) -
                (actual_diameter * 0.5 * actual_diameter * 0.5)
            );
            
        //Now, get the angular distance.
        float angular_dist =
            atan2(actual_diameter, middle_distance * 2.0f) * 2.0;
            
        //Finally, we can calculate where the other spots are.
        size_t n_spots_on_wheel = floor(M_PI * 2 / angular_dist);
        //Get a better angle. One that can evenly distribute the spots.
        float angle = M_PI * 2.0 / n_spots_on_wheel;
        
        for(unsigned s = 0; s < n_spots_on_wheel; ++s) {
            alpha_spots.push_back(
                alpha_spot(
                    point(
                        dist_from_center * cos(angle * s) +
                        randomf(-GROUP_SPOT_INTERVAL, GROUP_SPOT_INTERVAL),
                        dist_from_center * sin(angle * s) +
                        randomf(-GROUP_SPOT_INTERVAL, GROUP_SPOT_INTERVAL)
                    )
                )
            );
        }
        
        current_wheel++;
        radius = dist_from_center;
    }
    
    //Now, given all of these points, create our final spot vector,
    //with the rightmost points coming first.
    
    //Start by sorting the points.
    for(size_t a = 0; a < alpha_spots.size(); ++a) {
        alpha_spots[a].distance_to_rightmost =
            dist(
                alpha_spots[a].pos,
                point(radius, 0)
            );
    }
    
    std::sort(
        alpha_spots.begin(), alpha_spots.end(),
    [] (alpha_spot a1, alpha_spot a2) -> bool {
        return a1.distance_to_rightmost < a2.distance_to_rightmost;
    }
    );
    
    //Finally, create the group spots.
    spots.clear();
    spots.resize(members.size(), group_spot());
    for(size_t s = 0; s < members.size(); ++s) {
        spots[s] =
            group_spot(
                point(
                    alpha_spots[s].pos.x - radius,
                    alpha_spots[s].pos.y
                ),
                NULL
            );
    }
    
    //Pass the old mobs over.
    if(old_mobs.size() < spots.size()) {
        for(size_t m = 0; m < old_mobs.size(); ++m) {
            spots[m].mob_ptr = old_mobs[m];
        }
        spots[old_mobs.size()].mob_ptr = affected_mob_ptr;
        
    } else if(old_mobs.size() > spots.size()) {
        for(size_t m = 0; m < old_mobs.size(); ++m) {
            if(old_mobs[m] == affected_mob_ptr) continue;
            spots[m].mob_ptr = old_mobs[m];
        }
        
    } else {
        for(size_t m = 0; m < old_mobs.size(); ++m) {
            spots[m].mob_ptr = old_mobs[m];
        }
    }
}


/* ----------------------------------------------------------------------------
 * Returns the average position of the members.
 */
point group_info::get_average_member_pos() {
    point avg;
    for(size_t m = 0; m < members.size(); ++m) {
        avg += members[m]->pos;
    }
    return avg / members.size();
}


/* ----------------------------------------------------------------------------
 * Returns a point's offset from the anchor,
 * given the current group transformation.
 */
point group_info::get_spot_offset(const size_t spot_index) {
    point res = spots[spot_index].pos;
    al_transform_coordinates(&transform, &res.x, &res.y);
    return res;
}


/* ----------------------------------------------------------------------------
 * Assigns each mob a new spot, given how close each one of them is to
 * each spot.
 */
void group_info::reassign_spots() {
    for(size_t m = 0; m < members.size(); ++m) {
        members[m]->group_spot_index = INVALID;
    }
    
    for(size_t s = 0; s < spots.size(); ++s) {
        point spot_pos = anchor + get_spot_offset(s);
        mob* closest_mob = NULL;
        dist closest_dist;
        
        for(size_t m = 0; m < members.size(); ++m) {
            mob* m_ptr = members[m];
            if(m_ptr->group_spot_index != INVALID) continue;
            
            dist d(m_ptr->pos, spot_pos);
            
            if(!closest_mob || d < closest_dist) {
                closest_mob = m_ptr;
                closest_dist = d;
            }
        }
        
        closest_mob->group_spot_index = s;
    }
}


/* ----------------------------------------------------------------------------
 * Sets the standby group member type to the next available one,
 * or NULL if none.
 * Returns true on success, false on failure.
 * move_backwards: If true, go through the list backwards.
 */
bool group_info::set_next_cur_standby_type(const bool move_backwards) {

    if(members.empty()) {
        cur_standby_type = NULL;
        return true;
    }
    
    bool success = false;
    subgroup_type* starting_type = cur_standby_type;
    subgroup_type* final_type = cur_standby_type;
    if(!starting_type) starting_type = subgroup_types.get_first_type();
    subgroup_type* scanning_type = starting_type;
    subgroup_type* leader_subgroup_type =
        subgroup_types.get_type(SUBGROUP_TYPE_CATEGORY_LEADER);
        
    if(move_backwards) {
        scanning_type = subgroup_types.get_prev_type(scanning_type);
    } else {
        scanning_type = subgroup_types.get_next_type(scanning_type);
    }
    while(scanning_type != starting_type && !success) {
        //For each type, let's check if there's any group member that matches.
        if(
            scanning_type == leader_subgroup_type &&
            !can_throw_leaders
        ) {
            //If this is a leader, and leaders cannot be thrown, skip.
        } else {
            for(size_t m = 0; m < members.size(); ++m) {
                if(members[m]->subgroup_type_ptr == scanning_type) {
                    final_type = scanning_type;
                    success = true;
                    break;
                }
            }
        }
        
        if(move_backwards) {
            scanning_type = subgroup_types.get_prev_type(scanning_type);
        } else {
            scanning_type = subgroup_types.get_next_type(scanning_type);
        }
    }
    
    cur_standby_type = final_type;
    return success;
}


/* ----------------------------------------------------------------------------
 * Sorts the group with the specified type at the front, and the other types
 * (in order) behind.
 */
void group_info::sort(subgroup_type* leading_type) {

    for(size_t m = 0; m < members.size(); ++m) {
        members[m]->group_spot_index = INVALID;
    }
    
    subgroup_type* cur_type = leading_type;
    size_t cur_spot = 0;
    
    while(cur_spot != spots.size()) {
        point spot_pos = anchor + get_spot_offset(cur_spot);
        
        //Find the member closest to this spot.
        mob* closest_member = NULL;
        dist closest_dist;
        for(size_t m = 0; m < members.size(); ++m) {
            mob* m_ptr = members[m];
            if(m_ptr->subgroup_type_ptr != cur_type) continue;
            if(m_ptr->group_spot_index != INVALID) continue;
            
            dist d(m_ptr->pos, spot_pos);
            
            if(!closest_member || d < closest_dist) {
                closest_member = m_ptr;
                closest_dist = d;
            }
            
        }
        
        if(!closest_member) {
            //There are no more members of the current type left!
            //Next type.
            cur_type = subgroup_types.get_next_type(cur_type);
        } else {
            spots[cur_spot].mob_ptr = closest_member;
            closest_member->group_spot_index = cur_spot;
            cur_spot++;
        }
        
    }
    
}



/* ----------------------------------------------------------------------------
 * Calculates how much knockback an attack will cause.
 * attacker:   the attacking mob.
 * victim:     the mob that'll take the damage.
 * attacker_h: the hitbox of the attacker mob, if any.
 * victim_h:   the hitbox of the victim mob, if any.
 * knockback:  the variable to return the knockback amount to.
 * angle:      the variable to return the angle of the knockback to.
 */
void calculate_knockback(
    mob* attacker, mob* victim, hitbox* attacker_h,
    hitbox* victim_h, float* knockback, float* angle
) {
    if(attacker_h) {
        *knockback = attacker_h->knockback;
        if(attacker_h->knockback_outward) {
            *angle = get_angle(attacker->pos, victim->pos);
        } else {
            *angle = attacker->angle + attacker_h->knockback_angle;
        }
    }
}


/* ----------------------------------------------------------------------------
 * Creates a mob, adding it to the corresponding vectors.
 * Returns the new mob.
 */
mob* create_mob(
    mob_category* category, const point &pos, mob_type* type,
    const float angle, const string &vars
) {
    mob* m_ptr = NULL;
    if(type->create_mob_func) {
        m_ptr = type->create_mob_func(pos, angle, vars);
    } else {
        m_ptr = category->create_mob(pos, type, angle, vars);
    }
    mobs.push_back(m_ptr);
    return m_ptr;
}


/* ----------------------------------------------------------------------------
 * Deletes a mob from the relevant vectors.
 * It's always removed from the vector of mobs, but it's
 * also removed from the vector of Pikmin if it's a Pikmin,
 * leaders if it's a leader, etc.
 * m_ptr:                The mob to delete.
 * complete_destruction: If true, don't bother removing it from groups and such,
 *   since everything is going to be destroyed.
 */
void delete_mob(mob* m_ptr, const bool complete_destruction) {
    if(creator_tool_info_lock == m_ptr) creator_tool_info_lock = NULL;
    
    if(!complete_destruction) {
        m_ptr->leave_group();
        
        for(size_t m = 0; m < mobs.size(); ++m) {
            if(mobs[m]->focused_mob == m_ptr) {
                mobs[m]->focused_mob = NULL;
            }
        }
        
        m_ptr->fsm.set_state(INVALID);
    }
    
    if(m_ptr->type->erase_mob_func) {
        m_ptr->type->erase_mob_func(m_ptr);
    } else {
        m_ptr->type->category->erase_mob(m_ptr);
    }
    mobs.erase(find(mobs.begin(), mobs.end(), m_ptr));
    
    delete m_ptr;
}
