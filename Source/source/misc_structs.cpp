/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Miscellaneous structures, too small
 * to warrant their own files.
 */

#include <algorithm>
#include <climits>

#include "misc_structs.h"

#include "const.h"
#include "drawing.h"
#include "functions.h"
#include "load.h"
#include "utils/string_utils.h"
#include "vars.h"

/* ----------------------------------------------------------------------------
 * Creates a structure with information about a bitmap, for the manager.
 */
bmp_manager::bmp_info::bmp_info(ALLEGRO_BITMAP* b) :
    b(b),
    calls(1) {
    
}


/* ----------------------------------------------------------------------------
 * Creates a bitmap manager.
 */
bmp_manager::bmp_manager(const string &base_dir) :
    base_dir(base_dir),
    total_calls(0) {
    
}


/* ----------------------------------------------------------------------------
 * Returns the specified bitmap, by name.
 */
ALLEGRO_BITMAP* bmp_manager::get(
    const string &name, data_node* node,
    const bool report_errors
) {
    if(name.empty()) return load_bmp("", node, report_errors);
    
    if(list.find(name) == list.end()) {
        ALLEGRO_BITMAP* b =
            load_bmp(base_dir + "/" + name, node, report_errors);
        list[name] = bmp_info(b);
        total_calls++;
        return b;
    } else {
        list[name].calls++;
        total_calls++;
        return list[name].b;
    }
};


/* ----------------------------------------------------------------------------
 * Marks a bitmap to have one less call.
 * If it has 0 calls, it's automatically cleared.
 */
void bmp_manager::detach(map<string, bmp_info>::iterator it) {
    if(it == list.end()) return;
    
    it->second.calls--;
    total_calls--;
    if(it->second.calls == 0) {
        if(it->second.b != bmp_error) {
            al_destroy_bitmap(it->second.b);
        }
        list.erase(it);
    }
}


/* ----------------------------------------------------------------------------
 * Marks a bitmap to have one less call.
 * If it has 0 calls, it's automatically cleared.
 */
void bmp_manager::detach(const string &name) {
    if(name.empty()) return;
    detach(list.find(name));
}


/* ----------------------------------------------------------------------------
 * Deletes all bitmaps loaded and clears the list.
 */
void bmp_manager::clear() {
    for(auto b = list.begin(); b != list.end(); ++b) {
        if(b->second.b != bmp_error) {
            al_destroy_bitmap(b->second.b);
        }
    }
    list.clear();
    total_calls = 0;
}


/* ----------------------------------------------------------------------------
 * Marks a bitmap to have one less call.
 * If it has 0 calls, it's automatically cleared.
 */
void bmp_manager::detach(ALLEGRO_BITMAP* bmp) {
    if(!bmp || bmp == bmp_error) return;
    
    auto it = list.begin();
    for(; it != list.end(); ++it) {
        if(it->second.b == bmp) break;
    }
    
    detach(it);
}


/* ----------------------------------------------------------------------------
 * Returns the total number of calls. Used for debugging.
 */
long bmp_manager::get_total_calls() {
    return total_calls;
}


/* ----------------------------------------------------------------------------
 * Returns the size of the list. Used for debugging.
 */
size_t bmp_manager::get_list_size() {
    return list.size();
}



/* ----------------------------------------------------------------------------
 * Adds a new button to the list.
 */
void button_manager::add(
    const size_t id, const string &name, const string &option_name,
    const string &default_control_str
) {
    button_manager::button b;
    b.id = id;
    b.name = name;
    b.option_name = option_name;
    b.default_control_str = default_control_str;
    
    list.push_back(b);
}


/* ----------------------------------------------------------------------------
 * Creates a new HUD item.
 * center: Center coordinates, in screen dimension ratio.
 * size:   Dimensions, in screen dimension ratio.
 */
hud_item::hud_item(const point center, const point size) :
    center(center),
    size(size) {
    
}


/* ----------------------------------------------------------------------------
 * Initializes a HUD item manager.
 */
hud_item_manager::hud_item_manager(const size_t item_total) :
    move_in(false),
    move_timer(0),
    offscreen(false) {
    
    items.assign(item_total, hud_item());
}


/* ----------------------------------------------------------------------------
 * Sets a HUD item's data.
 * id:   ID of the HUD item.
 * x, y: Center coordinates, in screen dimension ratio (0 to 1, normally).
 * w, h: Total width and height, in screen dimension ratio (0 to 1, normally).
 */
void hud_item_manager::set_item(
    const size_t id,
    const float x, const float y, const float w, const float h
) {
    items[id] =
        hud_item(point(x / 100.0f, y / 100.0f), point(w / 100.0f, h / 100.0f));
}


/* ----------------------------------------------------------------------------
 * Retrieves the data necessary for the drawing routine.
 * Returns false if this element shouldn't be drawn.
 * id:     ID of the HUD item.
 * center: Pointer to place the final center coordinates in, if any.
 * size:   Pointer to place the final dimensions in, if any.
 */
bool hud_item_manager::get_draw_data(
    const size_t id, point* center, point* size
) {
    hud_item* h = &items[id];
    if(offscreen) return false;
    if(h->size.x <= 0 || h->size.y <= 0) return false;
    if(h->center.x + h->size.x / 2.0f < 0)    return false;
    if(h->center.x - h->size.x / 2.0f > 1.0f) return false;
    if(h->center.y + h->size.y / 2.0f < 0)    return false;
    if(h->center.y - h->size.y / 2.0f > 1.0f) return false;
    
    point normal_coords, final_coords;
    normal_coords.x = h->center.x * scr_w;
    normal_coords.y = h->center.y * scr_h;
    
    if(move_timer.time_left == 0.0f) {
        final_coords = normal_coords;
        
    } else {
        point start_coords, end_coords;
        unsigned char ease_method;
        point offscreen_coords;
        
        float angle = get_angle(point(0.5, 0.5), h->center);
        offscreen_coords.x = h->center.x + cos(angle);
        offscreen_coords.y = h->center.y + sin(angle);
        offscreen_coords.x *= scr_w;
        offscreen_coords.y *= scr_h;
        
        if(move_in) {
            start_coords = offscreen_coords;
            end_coords = normal_coords;
            ease_method = EASE_OUT;
        } else {
            start_coords = normal_coords;
            end_coords = offscreen_coords;
            ease_method = EASE_IN;
        }
        
        final_coords.x =
            interpolate_number(
                ease(ease_method, 1 - move_timer.get_ratio_left()),
                0, 1, start_coords.x, end_coords.x
            );
        final_coords.y =
            interpolate_number(
                ease(ease_method, 1 - move_timer.get_ratio_left()),
                0, 1, start_coords.y, end_coords.y
            );
    }
    
    if(center) {
        *center = final_coords;
    }
    if(size) {
        size->x = h->size.x * scr_w;
        size->y = h->size.y * scr_h;
    }
    
    return true;
}


/* ----------------------------------------------------------------------------
 * Starts a movement animation.
 * in:       Are the items moving into view, or out of view?
 * duration: How long this animation lasts for.
 */
void hud_item_manager::start_move(const bool in, const float duration) {
    move_in = in;
    move_timer.start(duration);
}


/* ----------------------------------------------------------------------------
 * Ticks the manager one frame in time.
 */
void hud_item_manager::tick(const float time) {
    move_timer.tick(time);
    if(!move_in && move_timer.time_left == 0.0f) {
        offscreen = true;
    } else {
        offscreen = false;
    }
}


/* ----------------------------------------------------------------------------
 * Initializes a movement struct with all movements set to 0.
 */
movement_struct::movement_struct() :
    right(0),
    up(0),
    left(0),
    down(0) {
    
}


/* ----------------------------------------------------------------------------
 * Returns the values of the coordinates, magnitude, and angle,
 * exactly as they are right now, without being "cleaned".
 * Don't use this one for normal gameplay, please.
 * All parameters are mandatory.
 */
void movement_struct::get_raw_info(
    point* coords, float* angle, float* magnitude
) {
    *coords = point(right - left, down - up);
    coordinates_to_angle(*coords, angle, magnitude);
}


/* ----------------------------------------------------------------------------
 * Returns the values of the coordinates, magnitude, and angle,
 * but "cleaned" up.
 * All parameters are mandatory.
 */
void movement_struct::get_clean_info(
    point* coords, float* angle, float* magnitude
) {
    get_raw_info(coords, angle, magnitude);
    *magnitude =
        clamp(*magnitude, joystick_min_deadzone, joystick_max_deadzone);
    *magnitude =
        interpolate_number(
            *magnitude,
            joystick_min_deadzone, joystick_max_deadzone,
            0.0f, 1.0f
        );
    *coords = angle_to_coordinates(*angle, *magnitude);
}



/* ----------------------------------------------------------------------------
 * Creates a "reader setter".
 * dn: Pointer to the base data node.
 */
reader_setter::reader_setter(data_node* dn) :
    node(dn) {
    
}


/* ----------------------------------------------------------------------------
 * Reads a child node's value, and uses it to set a variable.
 * Will not do anything if the child's value is empty.
 * child: Name of the child node.
 * var:   The var to set. This is an Allegro color.
 */
void reader_setter::set(const string &child, ALLEGRO_COLOR &var) {
    string s = node->get_child_by_name(child)->value;
    if(s.empty()) return;
    var = s2c(s);
}


/* ----------------------------------------------------------------------------
 * Reads a child node's value, and uses it to set a variable.
 * Will not do anything if the child's value is empty.
 * child: Name of the child node.
 * var:   The var to set. This is a string.
 */
void reader_setter::set(const string &child, string &var) {
    string s = node->get_child_by_name(child)->value;
    if(s.empty()) return;
    var = s;
}


/* ----------------------------------------------------------------------------
 * Reads a child node's value, and uses it to set a variable.
 * Will not do anything if the child's value is empty.
 * child: Name of the child node.
 * var:   The var to set. This is an integer.
 */
void reader_setter::set(const string &child, size_t &var) {
    string s = node->get_child_by_name(child)->value;
    if(s.empty()) return;
    var = s2i(s);
}


/* ----------------------------------------------------------------------------
 * Reads a child node's value, and uses it to set a variable.
 * Will not do anything if the child's value is empty.
 * child: Name of the child node.
 * var:   The var to set. This is an integer.
 */
void reader_setter::set(const string &child, int &var) {
    string s = node->get_child_by_name(child)->value;
    if(s.empty()) return;
    var = s2i(s);
}


/* ----------------------------------------------------------------------------
 * Reads a child node's value, and uses it to set a variable.
 * Will not do anything if the child's value is empty.
 * child: Name of the child node.
 * var:   The var to set. This is an unsigned char.
 */
void reader_setter::set(const string &child, unsigned char &var) {
    string s = node->get_child_by_name(child)->value;
    if(s.empty()) return;
    var = s2i(s);
}


/* ----------------------------------------------------------------------------
 * Reads a child node's value, and uses it to set a variable.
 * Will not do anything if the child's value is empty.
 * child: Name of the child node.
 * var:   The var to set. This is a boolean.
 */
void reader_setter::set(const string &child, bool &var) {
    string s = node->get_child_by_name(child)->value;
    if(s.empty()) return;
    var = s2b(s);
}


/* ----------------------------------------------------------------------------
 * Reads a child node's value, and uses it to set a variable.
 * Will not do anything if the child's value is empty.
 * child: Name of the child node.
 * var:   The var to set. This is a float.
 */
void reader_setter::set(const string &child, float &var) {
    string s = node->get_child_by_name(child)->value;
    if(s.empty()) return;
    var = s2f(s);
}


/* ----------------------------------------------------------------------------
 * Reads a child node's value, and uses it to set a variable.
 * Will not do anything if the child's value is empty.
 * child: Name of the child node.
 * var:   The var to set. This is a point.
 */
void reader_setter::set(const string &child, point &var) {
    string s = node->get_child_by_name(child)->value;
    if(s.empty()) return;
    var = s2p(s);
}



/* ----------------------------------------------------------------------------
 * Creates a structure with sample info.
 */
sample_struct::sample_struct(ALLEGRO_SAMPLE* s, ALLEGRO_MIXER* mixer) :
    sample(s),
    instance(NULL) {
    
    if(!s) return;
    instance = al_create_sample_instance(s);
    al_attach_sample_instance_to_mixer(instance, mixer);
}


/* ----------------------------------------------------------------------------
 * Play the sample.
 * max_override_pos: Override the currently playing sound
 *   only if it's already in this position, or beyond.
 *   This is in seconds. 0 means always override. -1 means never override.
 * loop: Loop the sound?
 * gain: Volume, 0 - 1.
 * pan: Panning, 0 - 1 (0.5 is centered).
 * speed: Playing speed.
 */
void sample_struct::play(
    const float max_override_pos, const bool loop, const float gain,
    const float pan, const float speed
) {
    if(!sample || !instance) return;
    
    if(max_override_pos != 0 && al_get_sample_instance_playing(instance)) {
        float secs = al_get_sample_instance_position(instance) / (float) 44100;
        if(
            (secs < max_override_pos && max_override_pos > 0) ||
            max_override_pos == -1
        ) {
            return;
        }
    }
    
    al_set_sample_instance_playmode(
        instance, (loop ? ALLEGRO_PLAYMODE_LOOP : ALLEGRO_PLAYMODE_ONCE)
    );
    al_set_sample_instance_gain(instance, gain);
    al_set_sample_instance_pan(instance, pan);
    al_set_sample_instance_speed(instance, speed);
    
    al_set_sample_instance_position(instance, 0);
    al_set_sample_instance_playing( instance, true);
}


/* ----------------------------------------------------------------------------
 * Stops a playing sample instance.
 */
void sample_struct::stop() {
    al_set_sample_instance_playing(instance, false);
}


/* ----------------------------------------------------------------------------
 * Destroys a structure with sample info.
 */
void sample_struct::destroy() {
    //TODO uncommenting this is causing a crash.
    //al_detach_sample_instance(instance);
    al_destroy_sample_instance(instance);
    al_destroy_sample(sample);
}


/* ----------------------------------------------------------------------------
 * Registers a new type of sector.
 */
void sector_types_manager::register_type(
    const unsigned char nr, const string &name
) {
    if(nr >= names.size()) {
        names.insert(names.end(), (nr + 1) - names.size(), "");
    }
    names[nr] = name;
}


/* ----------------------------------------------------------------------------
 * Returns the number of a sector type, given its name.
 * Returns 255 on error.
 */
unsigned char sector_types_manager::get_nr(const string &name) {
    for(unsigned char n = 0; n < names.size(); ++n) {
        if(names[n] == name) return n;
    }
    return 255;
}


/* ----------------------------------------------------------------------------
 * Returns the name of a sector type, given its number.
 * Returns an empty string on error.
 */
string sector_types_manager::get_name(const unsigned char nr) {
    if(nr < names.size()) return names[nr];
    return "";
}


/* ----------------------------------------------------------------------------
 * Returns the number of sector types registered.
 */
unsigned char sector_types_manager::get_nr_of_types() {
    return names.size();
}


/* ----------------------------------------------------------------------------
 * Creates a timer.
 */
timer::timer(float duration, const function<void()> &on_end) :
    time_left(0),
    duration(duration),
    on_end(on_end) {
    
    
}


/* ----------------------------------------------------------------------------
 * Destroys a timer.
 */
timer::~timer() {
    //TODO Valgrind detects a leak with on_end...
}


/* ----------------------------------------------------------------------------
 * Starts a timer.
 * can_restart: If false, calling this while the timer is still ticking down
 *   will not do anything.
 */
void timer::start(const bool can_restart) {
    if(!can_restart && time_left > 0) return;
    time_left = duration;
}


/* ----------------------------------------------------------------------------
 * Starts a timer, but sets a new duration.
 */
void timer::start(const float new_duration) {
    duration = new_duration;
    start();
}


/* ----------------------------------------------------------------------------
 * Stops a timer, without executing the on_end callback.
 */
void timer::stop() {
    time_left = 0.0f;
}


/* ----------------------------------------------------------------------------
 * Ticks a timer.
 * amount: Time to tick.
 */
void timer::tick(const float amount) {
    if(time_left == 0.0f) return;
    time_left = max(time_left - amount, 0.0f);
    if(time_left == 0.0f && on_end) {
        on_end();
    }
}


/* ----------------------------------------------------------------------------
 * Returns the ratio of time left (i.e. 0 if done, 1 if all time is left).
 */
float timer::get_ratio_left() {
    return time_left / duration;
}


const float fade_manager::FADE_DURATION = 0.15f;

/* ----------------------------------------------------------------------------
 * Creates a fade manager.
 */
fade_manager::fade_manager() :
    time_left(0),
    fade_in(false),
    on_end(nullptr) {
    
}


/* ----------------------------------------------------------------------------
 * Sets up the start of a fade.
 */
void fade_manager::start_fade(
    const bool fade_in, const function<void()> &on_end
) {
    time_left = FADE_DURATION;
    this->fade_in = fade_in;
    this->on_end = on_end;
}


/* ----------------------------------------------------------------------------
 * Returns whether the current fade is a fade in or fade out.
 */
bool fade_manager::is_fade_in() {
    return fade_in;
}


/* ----------------------------------------------------------------------------
 * Returns whether or not a fade is currently in progress.
 */
bool fade_manager::is_fading() {
    return time_left > 0;
}


/* ----------------------------------------------------------------------------
 * Returns the percentage of progress left in the current fade.
 */
float fade_manager::get_perc_left() {
    return time_left / FADE_DURATION;
}


/* ----------------------------------------------------------------------------
 * Ticks the fade manager by one frame.
 */
void fade_manager::tick(const float time) {
    if(time_left == 0) return;
    time_left -= time;
    if(time_left <= 0) {
        time_left = 0;
        if(on_end) on_end();
    }
}


/* ----------------------------------------------------------------------------
 * Draws the fade overlay, if there is a fade in progress.
 */
void fade_manager::draw() {
    if(is_fading()) {
        unsigned char alpha = (fade_mgr.get_perc_left()) * 255;
        al_draw_filled_rectangle(
            0, 0, scr_w, scr_h,
            al_map_rgba(
                0, 0, 0, (fade_mgr.is_fade_in() ? alpha : 255 - alpha)
            )
        );
    }
}


/* ----------------------------------------------------------------------------
 * Initializes a bitmap effect properties struct.
 */
bitmap_effect_props::bitmap_effect_props() :
    translation(0, 0),
    rotation(0),
    scale(1, 1),
    tint_color(al_map_rgb(255, 255, 255)),
    glow_color(al_map_rgb(0, 0, 0)) {
    
}


/* ----------------------------------------------------------------------------
 * Initializes a bitmap effect struct.
 */
bitmap_effect::bitmap_effect() :
    cur_time(0) {
    
}


/* ----------------------------------------------------------------------------
 * Adds a keyframe to the effect.
 * i.e. What the properties should be at a given point in time.
 */
void bitmap_effect::add_keyframe(
    const float time, const bitmap_effect_props &props
) {
    keyframes[time] = props;
}


/* ----------------------------------------------------------------------------
 * Sets the current life time on the effect.
 */
void bitmap_effect::set_cur_time(const float cur_time) {
    this->cur_time = cur_time;
}


/* ----------------------------------------------------------------------------
 * Returns what the properties should be at the specified time.
 * These values are interpolated using the keyframes.
 */
bitmap_effect_props bitmap_effect::get_final_properties() {
    assert(!keyframes.empty());
    
    if(keyframes.size() == 1) {
        return keyframes[0];
        
    } else {
        //Find the previous and next keyframes.
        float prev_time = 0;
        float next_time = 0;
        bitmap_effect_props* prev_keyframe = &keyframes[0];
        bitmap_effect_props* next_keyframe = NULL;
        for(auto k = keyframes.begin(); k != keyframes.end(); ++k) {
            if(k->first > cur_time) {
                next_keyframe = &k->second;
                next_time = k->first;
                break;
            } else {
                prev_keyframe = &k->second;
                prev_time = k->first;
            }
        }
        if(!next_keyframe) next_keyframe = prev_keyframe;
        
        bitmap_effect_props final_props;
        final_props.translation.x =
            interpolate_number(
                cur_time, prev_time, next_time,
                prev_keyframe->translation.x, next_keyframe->translation.x
            );
        final_props.translation.y =
            interpolate_number(
                cur_time, prev_time, next_time,
                prev_keyframe->translation.y, next_keyframe->translation.y
            );
        final_props.rotation =
            interpolate_number(
                cur_time, prev_time, next_time,
                prev_keyframe->rotation, next_keyframe->rotation
            );
        final_props.scale.x =
            interpolate_number(
                cur_time, prev_time, next_time,
                prev_keyframe->scale.x, next_keyframe->scale.x
            );
        final_props.scale.y =
            interpolate_number(
                cur_time, prev_time, next_time,
                prev_keyframe->scale.y, next_keyframe->scale.y
            );
        final_props.tint_color =
            interpolate_color(
                cur_time, prev_time, next_time,
                prev_keyframe->tint_color, next_keyframe->tint_color
            );
        final_props.glow_color =
            interpolate_color(
                cur_time, prev_time, next_time,
                prev_keyframe->glow_color, next_keyframe->glow_color
            );
            
        return final_props;
    }
}


/* ----------------------------------------------------------------------------
 * Adds an effect to the manager.
 */
void bitmap_effect_manager::add_effect(bitmap_effect effect) {
    effects.push_back(effect);
}


/* ----------------------------------------------------------------------------
 * Returns the final bitmap effect properties, at the current time.
 * This is a combination of all current effects.
 */
bitmap_effect_props bitmap_effect_manager::get_final_properties() {
    bitmap_effect_props final_props;
    ALLEGRO_COLOR glow_color_sum;
    glow_color_sum = al_map_rgba(0, 0, 0, 0);
    size_t n_glow_colors = 0;
    
    for(size_t e = 0; e < effects.size(); ++e) {
        bitmap_effect_props props = effects[e].get_final_properties();
        
        final_props.translation = final_props.translation + props.translation;
        final_props.rotation += props.rotation;
        final_props.scale = final_props.scale * props.scale;
        final_props.tint_color.r *= props.tint_color.r;
        final_props.tint_color.g *= props.tint_color.g;
        final_props.tint_color.b *= props.tint_color.b;
        final_props.tint_color.a *= props.tint_color.a;
        
        if(props.glow_color.a > 0) {
            glow_color_sum.r += props.glow_color.r;
            glow_color_sum.g += props.glow_color.g;
            glow_color_sum.b += props.glow_color.b;
            glow_color_sum.a += props.glow_color.a;
            n_glow_colors++;
        }
        
    }
    
    if(n_glow_colors > 0) {
        final_props.glow_color.r = glow_color_sum.r / n_glow_colors;
        final_props.glow_color.g = glow_color_sum.g / n_glow_colors;
        final_props.glow_color.b = glow_color_sum.b / n_glow_colors;
        final_props.glow_color.a = glow_color_sum.a / n_glow_colors;
    }
    
    return final_props;
    
}


/* ----------------------------------------------------------------------------
 * Registers a new type of subgroup.
 * category:      The category of subgroup type. Pikmin, leader, bomb-rock, etc.
 * specific_type: Specific type of mob, if you want to specify further.
 * icon:          If not NULL, use this icon to represent this subgroup.
 */
void subgroup_type_manager::register_type(
    const SUBGROUP_TYPE_CATEGORIES category,
    mob_type* specific_type,
    ALLEGRO_BITMAP* icon
) {
    subgroup_type* new_sg_type = new subgroup_type();
    
    new_sg_type->category = category;
    new_sg_type->specific_type = specific_type;
    new_sg_type->icon = icon;
    
    types.push_back(new_sg_type);
}


/* ----------------------------------------------------------------------------
 * Returns the type of subgroup corresponding to the parameters.
 * Returns NULL if not found.
 * category:      The category of subgroup type. Pikmin, leader, bomb-rock, etc.
 * specific_type: Specific type of mob, if you want to specify further.
 */
subgroup_type* subgroup_type_manager::get_type(
    const SUBGROUP_TYPE_CATEGORIES category,
    mob_type* specific_type
) {
    for(size_t t = 0; t < types.size(); ++t) {
        subgroup_type* t_ptr = types[t];
        if(
            t_ptr->category == category &&
            t_ptr->specific_type == specific_type
        ) {
            return t_ptr;
        }
    }
    return NULL;
}


/* ----------------------------------------------------------------------------
 * Returns the first registered subgroup type.
 */
subgroup_type* subgroup_type_manager::get_first_type() {
    return types.front();
}


/* ----------------------------------------------------------------------------
 * Returns the subgroup type that comes before the given type.
 */
subgroup_type* subgroup_type_manager::get_prev_type(
    subgroup_type* sgt
) {
    for(size_t t = 0; t < types.size(); ++t) {
        if(types[t] == sgt) {
            return get_prev_in_vector(types, t);
        }
    }
    return NULL;
}


/* ----------------------------------------------------------------------------
 * Returns the subgroup type that comes after the given type.
 */
subgroup_type* subgroup_type_manager::get_next_type(
    subgroup_type* sgt
) {
    for(size_t t = 0; t < types.size(); ++t) {
        if(types[t] == sgt) {
            return get_next_in_vector(types, t);
        }
    }
    return NULL;
}


/* ----------------------------------------------------------------------------
 * Clears the list of registered subgroup types.
 */
void subgroup_type_manager::clear() {
    for(size_t t = 0; t < types.size(); ++t) {
        delete types[t];
    }
    types.clear();
}
