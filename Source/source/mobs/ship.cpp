/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Ship class and ship-related functions.
 */

#include "ship.h"

#include "../drawing.h"
#include "../utils/geometry_utils.h"
#include "../vars.h"
#include "leader.h"

/* ----------------------------------------------------------------------------
 * Creates a ship mob.
 */
ship::ship(const point &pos, ship_type* type, float angle) :
    mob(pos, type, angle),
    shi_type(type),
	pokos(0),
	beam_final_pos(rotate_point(type->beam_offset, angle)) {
    beam_final_pos += pos;
}


/* ----------------------------------------------------------------------------
 * Draws a ship.
 */
void ship::draw_mob(bitmap_effect_manager* effect_manager) {

    mob::draw_mob();
    
    al_draw_circle(
        beam_final_pos.x,
        beam_final_pos.y,
        shi_type->beam_radius,
        al_map_rgb(
            ship_beam_ring_color[0],
            ship_beam_ring_color[1],
            ship_beam_ring_color[2]
        ),
        2
    );
}


/* ----------------------------------------------------------------------------
 * Heals a leader, causes particle effects, etc.
 */
void ship::heal_leader(leader* l) {
    l->set_health(false, true, 1.0);
    
    particle p(
        PARTICLE_TYPE_BITMAP,
        l->pos, l->z + l->height, 16, 3,
        PARTICLE_PRIORITY_LOW
    );
    p.bitmap = bmp_sparkle;
    p.color = al_map_rgba(192, 255, 192, 255);
    p.speed = point(0, -24);
    
    particle_generator g(0, p, 12);
    g.duration_deviation = 0.5;
    g.pos_deviation = point(l->type->radius, l->type->radius);
    g.emit(particles);
}


/* ----------------------------------------------------------------------------
 * Checks whether the specified leader is currently under the ship's
 * ring of light or not.
 */
bool ship::is_leader_under_ring(leader* l) {
    return dist(l->pos, beam_final_pos) <= shi_type->beam_radius;
}
