/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Translated to C in 2026 by tools/colour/j2c.py from Organism.java of
 * Biogenesis Color Mod 2.01 (https://sourceforge.net/projects/biogenesis/),
 * Copyright (C) 2006-2010 Joan Queralt Molina,
 * Color Mod Copyright (C) 2012-2022 MarcoDBAA.
 * Generated: edit the translator or the Java, not this file.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version. It is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License (LICENSE) for more details.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "sim/organism.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sim/params.h"
#include "sim/world.h"

static Vector2D org_v;
static bool org_place_random(Organism *o);
static bool org_place_near_yellow(Organism *o, Organism *parent);
static bool org_place_near_first(Organism *o, Organism *parent);
static bool org_place_near(Organism *o, Organism *parent);
static bool org_transform_organism(Organism *o, Organism *parent);
static void org_grow(Organism *o);
static void org_touch_hatch(Organism *o, Organism *org, int seg, int oseg);
static void org_touch_move(Organism *o, Organism *org, JPoint p, JLine l, bool thisOrganism);
static bool org_is_inside_world(Organism *o);
static void org_offset(Organism *o, double offsetx, double offsety, double offsettheta);
static void org_stopmoving(Organism *o);
static void org_standstill(Organism *o);
static void org_standochre(Organism *o);
static void org_virusneutralized(Organism *o);
static void org_lavenderinhibition(Organism *o);
static void org_lavendershield(Organism *o);
static void org_weaklavendershield(Organism *o);
static void org_pretouch_effects(Organism *o, Organism *org, int seg, int oseg);
static void org_touchfriend_effects(Organism *o, Organism *org, int seg, int oseg);
static void org_touchfriend_reaction(Organism *o, Organism *org, int seg, int oseg);
static void org_touch_reaction(Organism *o, Organism *org, int seg, int oseg);
static void org_touchfriend_brake(Organism *o, Organism *org, int seg, int oseg);
static void org_touch_brake(Organism *o, Organism *org, int seg, int oseg);
static void org_touch_effects1(Organism *o, Organism *org, int seg, int oseg);
static void org_touch_effects2(Organism *o, Organism *org, int seg, int oseg);
static void org_rubbing_frames_effects(Organism *o);
static void org_segments_frame_effects(Organism *o);
static void org_leaf_extra_effects(Organism *o);
static void org_jade_extra_effects(Organism *o);
static void org_segments_healing_effects(Organism *o);
static void org_segments_extra_effects(Organism *o);
static void org_segments_extra_plants_effects(Organism *o);
static void org_plants_update_effects(Organism *o);
static void org_segments_update_effects(Organism *o);
static void org_darkfire_restore_effects(Organism *o);
static int org_get_type_color(JColor c);
static void org_set_color(Organism *o, JColor c);

static void org_finalize(void *obj)
{
    Organism *o = obj;
    jarr_free(o->start_x);
    jarr_free(o->start_y);
    jarr_free(o->end_x);
    jarr_free(o->end_y);
    jarr_free(o->m1);
    jarr_free(o->m2);
    jarr_free(o->m);
    jarr_free(o->mphoto);
    jarr_free(o->seg_color);
    jarr_free(o->segred_reaction);
    jarr_free(o->seggreen_reaction);
    jarr_free(o->segblue_reaction);
    jarr_free(o->segplague_reaction);
    jarr_free(o->segscourge_reaction);
    jarr_free(o->segwhite_reaction);
    jarr_free(o->seggray_reaction);
    jarr_free(o->segdefault_reaction);
    jarr_free(o->segconsumer_reaction);
    jarr_free(o->segplant_reaction);
    jarr_free(o->segmagenta_reaction);
    jarr_free(o->segpink_reaction);
    jarr_free(o->segcoral_reaction);
    jarr_free(o->segorange_reaction);
    jarr_free(o->segbark_reaction);
    jarr_free(o->segviolet_reaction);
    jarr_free(o->segvirus_reaction);
    jarr_free(o->segmaroon_reaction);
    jarr_free(o->segolive_reaction);
    jarr_free(o->segmint_reaction);
    jarr_free(o->segcream_reaction);
    jarr_free(o->segspike_reaction);
    jarr_free(o->segfallow_reaction);
    jarr_free(o->seglightblue_reaction);
    jarr_free(o->segochre_reaction);
    jarr_free(o->segsky_reaction);
    jarr_free(o->seglilac_reaction);
    jarr_free(o->segsilver_reaction);
    jarr_free(o->segfire_reaction);
    jarr_free(o->seglightbrown_reaction);
    jarr_free(o->seggreenbrown_reaction);
    jarr_free(o->segbrown_reaction);
    jarr_free(o->segice_reaction);
    jarr_free(o->segbroken_reaction);
    jarr_free(o->segsick_reaction);
    jarr_free(o->segfriend_reaction);
    jarr_free(o->x1);
    jarr_free(o->y1);
    jarr_free(o->x2);
    jarr_free(o->y2);
}

void org_mark_refs(void *obj)
{
    Organism *o = obj;
    jgc_mark(o->gc);
    jgc_mark(o->infected_gc);
    jgc_mark(o->saved_gc);
    jstr_mark(o->n_position);
}

void org_free(Organism *o)
{
    if (!o)
        return;
    org_finalize(o);
    free(o);
}


Organism *org_alloc_blank(void)
{
    Organism *x = calloc(1, sizeof *x);
    if (!x)
        abort();
    return x;
}

static Organism *org_alloc(void)
{
    Organism *o = calloc(1, sizeof *o);
    if (!o)
        abort();
    o->summerinactivity = -1;
    o->lengthfriend = -1;
    o->thetafriend = -1;
    o->hasdodged = true;
    o->last_theta = -1;
    o->has_moved = true;
    o->alive = true;
    o->active = true;
    return o;
}

/* Returns true if this organism is alive, false otherwise.
 * 
 * @return  true if this organism is alive, false otherwise.
 */
bool org_is_alive(Organism *o)
{
    return o->alive;
}

/* Returns the amount of chemical energy stored by this organism.
 * 
 * @return  The amount of chemical energy stored by this organism.
 */
double org_get_energy(Organism *o)
{
    return o->energy;
}

/* Returns the identification number of this organism.
 * 
 * @return  The identification number of this organism.
 */
int org_get_id(Organism *o)
{
    return o->id;
}

/* Returns the identification number of this organism's parent.
 * 
 * @return  The identification number of this organism's parent.
 */
int org_get_parent_id(Organism *o)
{
    return o->parent_id;
}

/* Returns the energy needed to replicate this genetic code.
 * This energy is equal to 40 plus 3 for each segment.
 * 
 * @return  the energy needed to replicate this genetic code.
 */
int org_get_reproduce_energy(Organism *o)
{
    return o->reproduce_energy;
}

/* Returns the age of this organism.
 * 
 * @return  The age of this organism, in number of frames.
 */
int org_get_age(Organism *o)
{
    return o->age;
}

/* Returns the maximum age that the organism can be.
 * 
 * @return  The maximum age that the organism can be.
 */
int org_get_max_age(Organism *o)
{
    return o->max_age;
}

/* Returns the number of children that this organism produced.
 * 
 * @return  The number of children that this organism produced.
 */
int org_get_total_children(Organism *o)
{
    return o->total_children;
}

/* Returns the number of organisms killed by this organism.
 * 
 * @return  The number of organisms killed by this organism.
 */
int org_get_total_kills(Organism *o)
{
    return o->total_kills;
}

/* Returns the number of organisms infected by this organism.
 * 
 * @return  The number of organisms infected by this organism.
 */
int org_get_total_infected(Organism *o)
{
    return o->total_infected;
}

/* Returns a reference to this organism's genetic code.
 * 
 * @return  A reference to this organism's genetic code.
 */
GeneticCode *org_get_genetic_code(Organism *o)
{
    return o->gc;
}

/* Returns the total mass of this organism.
 * 
 * @return  The total mass of this organism calculated as the sum
 * of all its segments length.
 */
double org_get_mass(Organism *o)
{
    return o->mass;
}

/* Creates all data structures of this organism. Must be used after the organism
 * has a genetic code assigned.
 */
void org_create(Organism *o)
{
    o->segments = gc_get_n_genes(o->gc) * gc_get_symmetry(o->gc);
    JARR_SET(o->seg_color, JARR_NEW(JColor, o->segments));
    JARR_SET(o->segred_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->seggreen_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segblue_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segplague_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segscourge_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segwhite_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->seggray_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segsilver_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segdefault_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segmagenta_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segpink_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segcoral_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segorange_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segbark_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segviolet_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segvirus_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segmaroon_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segolive_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segmint_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segcream_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segspike_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->seglightblue_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segochre_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->seglightbrown_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segbrown_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segsick_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segsky_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->seglilac_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segice_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segbroken_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segfire_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segfriend_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->seggreenbrown_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segfallow_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segconsumer_reaction, JARR_NEW(int, o->segments));
    JARR_SET(o->segplant_reaction, JARR_NEW(int, o->segments));
    for (int i = 0; i < o->segments; i++) {
        o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        if (o->seg_color[i] == CM_EYE) {
            o->haseyes = true;
        }
        o->segred_reaction[i] = gene_getred_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->seggreen_reaction[i] = gene_getgreen_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segblue_reaction[i] = gene_getblue_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segplague_reaction[i] = gene_getplague_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segscourge_reaction[i] = gene_getscourge_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segwhite_reaction[i] = gene_getwhite_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->seggray_reaction[i] = gene_getgray_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segdefault_reaction[i] = gene_getdefault_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segmagenta_reaction[i] = gene_getmagenta_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segpink_reaction[i] = gene_getpink_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segcoral_reaction[i] = gene_getcoral_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segorange_reaction[i] = gene_getorange_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segbark_reaction[i] = gene_getbark_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segviolet_reaction[i] = gene_getviolet_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segvirus_reaction[i] = gene_getvirus_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segmaroon_reaction[i] = gene_getmaroon_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segolive_reaction[i] = gene_getolive_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segmint_reaction[i] = gene_getmint_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segcream_reaction[i] = gene_getcream_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segspike_reaction[i] = gene_getspike_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->seglightblue_reaction[i] = gene_getlightblue_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segochre_reaction[i] = gene_getochre_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->seglightbrown_reaction[i] = gene_getlightbrown_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segbrown_reaction[i] = gene_getbrown_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segsick_reaction[i] = gene_getsick_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segsky_reaction[i] = gene_getsky_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->seglilac_reaction[i] = gene_getlilac_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segice_reaction[i] = gene_getice_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segbroken_reaction[i] = gene_getbroken_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segsilver_reaction[i] = gene_getsilver_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segfire_reaction[i] = gene_getfire_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segfriend_reaction[i] = gene_getfriend_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->seggreenbrown_reaction[i] = gene_getgreenbrown_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segfallow_reaction[i] = gene_getfallow_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segconsumer_reaction[i] = gene_getconsumer_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
        o->segplant_reaction[i] = gene_getplant_reaction(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
    }
    JARR_SET(o->start_x, JARR_NEW(int, o->segments));
    JARR_SET(o->start_y, JARR_NEW(int, o->segments));
    JARR_SET(o->end_x, JARR_NEW(int, o->segments));
    JARR_SET(o->end_y, JARR_NEW(int, o->segments));
    JARR_SET(o->m1, JARR_NEW(double, o->segments));
    JARR_SET(o->m2, JARR_NEW(double, o->segments));
    JARR_SET(o->m, JARR_NEW(double, o->segments));
    JARR_SET(o->mphoto, JARR_NEW(double, o->segments));
    JARR_SET(o->x1, JARR_NEW(int, o->segments));
    JARR_SET(o->y1, JARR_NEW(int, o->segments));
    JARR_SET(o->x2, JARR_NEW(int, o->segments));
    JARR_SET(o->y2, JARR_NEW(int, o->segments));
}

void org_segments_check_plant(Organism *o)
{
    int i = 0;
    o->spin = 0;
    if (o->jadefactor > 0) {
        o->jadefactor = 1;
    }
    for (i = o->segments - 1; i >= 0; i--) {
        // Manteniment
        switch (org_get_type_color(o->seg_color[i])) {
        case TY_JADE:
            o->jadefactor = 2;
            break;
        case TY_DARKJADE:
            o->jadefactor = 2;
            break;
        case TY_SPRING:
            o->spin += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            break;
        case TY_DARKGREEN:
            if (gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc))) == CM_JADE) {
                o->jadefactor = 2;
            }
            if (gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc))) == CM_SPRING) {
                o->spin += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            }
            break;
        }
    }
    // Calculate jade delay used in restoration of the color
    if (o->jadefactor > 1) {
        if (gc_get_symmetry(o->gc) != 1) {
            o->jadefactor = P.darkjade_delay * gc_get_symmetry(o->gc);
        } else {
            o->jadefactor = P.darkjade_delay * 2;
        }
    }
}

void org_segments_restore_effects(Organism *o)
{
    int i = 0;
    o->spin = 0;
    if (o->jadefactor > 0) {
        o->jadefactor = 1;
    }
    o->healing = 0;
    o->antiviral = 0;
    o->createlavender = 0;
    o->yellow_counter = 0;
    double fertility = 0;
    o->indigo = 0;
    o->early_reproduce_energy = 0;
    if (o->reproducelate > 0) {
        o->reproducelate = 0;
        o->reproduce_energy = (40 + 3 * o->segments);
    }
    if (o->gold > 0) {
        o->gold = 0;
        o->max_age = P.max_age + (o->segments / P.age_divisor);
    }
    o->isreproductive = false;
    if (o->skyversion > 0) {
        o->skyversion = -1;
    }
    if (o->isauburn) {
        if ((o->isaconsumer) || (o->isaplant) || (o->isafungus)) {
            o->isauburn = false;
        }
    }
    for (i = o->segments - 1; i >= 0; i--) {
        // Manteniment
        switch (org_get_type_color(o->seg_color[i])) {
        case TY_JADE:
            o->jadefactor = 2;
            break;
        case TY_DARKJADE:
            o->jadefactor = 2;
            break;
        case TY_SPRING:
            o->spin += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            break;
        case TY_DARKGREEN:
            if (gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc))) == CM_JADE) {
                o->jadefactor = 2;
            }
            if (gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc))) == CM_SPRING) {
                o->spin += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            }
            break;
        case TY_TEAL:
            o->canmove = true;
            o->canreact = true;
            if (gc_get_passive(o->gc) == false) {
                o->useframemovement = true;
                o->mphoto[i] = -4;
            }
            break;
        case TY_CYAN:
            o->canmove = true;
            o->useframemovement = true;
            o->mphoto[i] = -3;
            break;
        case TY_SPORE:
            if (gc_get_modifiesspore(o->gc) <= 6) {
                o->isreproductive = true;
                if (gc_get_modifiesspore(o->gc) <= 2) {
                    o->early_reproduce_energy += 20 * gc_get_n_genes(o->gc);
                }
            }
            break;
        case TY_BLOND:
            o->early_reproduce_energy += 30 + j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            break;
        case TY_FLOWER:
            if (o->reproducelate < 0) {
                o->reproducelate = 0;
            }
            o->reproducelate += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            break;
        case TY_GOLD:
            o->gold += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            break;
        case TY_AUBURN:
            o->isauburn = true;
            o->isreproductive = true;
            break;
        case TY_YELLOW:
            o->isreproductive = true;
            o->yellow_counter++;
            if (gc_get_symmetry(o->gc) != 3) {
                fertility += gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
            } else {
                if (gc_get_mirror(o->gc) == 0) {
                    fertility += (5 / 6) * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                } else {
                    fertility += (2 / 3) * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                }
            }
            break;
        case TY_INDIGO:
            o->isreproductive = true;
            if (gc_get_symmetry(o->gc) != 3) {
                o->indigo += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            } else {
                if (gc_get_mirror(o->gc) == 0) {
                    o->indigo += j_d2i(((50 / 6) * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
                } else {
                    o->indigo += j_d2i(((20 / 3) * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
                }
            }
            break;
        case TY_LAVENDER:
            o->createlavender += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            break;
        case TY_MINT:
            if (gc_get_symmetry(o->gc) != 3) {
                o->antiviral += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            } else {
                if (gc_get_mirror(o->gc) == 0) {
                    o->antiviral += j_d2i(((50 / 6) * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
                } else {
                    o->antiviral += j_d2i(((20 / 3) * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
                }
            }
            break;
        case TY_MAGENTA:
            if (gc_get_symmetry(o->gc) != 3) {
                o->healing += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            } else {
                if (gc_get_mirror(o->gc) == 0) {
                    o->healing += j_d2i(((50 / 6) * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
                } else {
                    o->healing += j_d2i(((20 / 3) * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
                }
            }
            break;
        case TY_SKY:
        case TY_DEEPSKY:
            if (o->skyversion <= 0) {
                if (gc_get_modifiessky(o->gc)) {
                    o->skyversion = 2;
                } else {
                    o->skyversion = 1;
                }
            }
            break;
        case TY_LIGHTBROWN:
            o->isinjured = true;
            break;
        case TY_GREENBROWN:
            o->isinjured = true;
            break;
        case TY_DARKFIRE:
            o->isinjured = true;
            break;
        case TY_LIGHT_BLUE:
            o->isinjured = true;
            break;
        case TY_BROKEN:
            o->isinjured = true;
            break;
        case TY_ICE:
            o->isfrozen = true;
            o->isinjured = true;
            break;
        case TY_DEADBARK:
            o->isfrozen = true;
            break;
        }
    }
    // Can this organism dodge?
    if (o->canreact) {
        if ((!o->isaconsumer) && (!o->isafungus) && (!o->isakiller) && (o->skyversion == 0) && (((!o->isinfectious) && (!o->iscoral) && (o->plagueversion == 0)) || ((!o->isaplant) && (o->methanotrophy == 0) && (gc_get_passive(o->gc))))) {
            o->candodge = true;
        }
    }
    // Calculate jade delay used in restoration of the color
    if (o->jadefactor > 1) {
        if (gc_get_symmetry(o->gc) != 1) {
            o->jadefactor = P.darkjade_delay * gc_get_symmetry(o->gc);
        } else {
            o->jadefactor = P.darkjade_delay * 2;
        }
    }
    // Calculate the total shield production capacity of lavender segments
    if (o->createlavender > 0) {
        o->createlavender = o->createlavender / gc_get_symmetry(o->gc);
    }
    // Calculate extra children for yellow
    if (o->yellow_counter > 0) {
        o->yellow_counter = (o->yellow_counter / gc_get_symmetry(o->gc)) + j_d2i((fertility / 23));
    }
    // Calculate indigo
    if (o->indigo > 0) {
        o->indigo = j_d2i((o->indigo / P.indigo_divisor));
    }
    // Calculate reproduction energy for blond segments
    if (o->early_reproduce_energy > 0) {
        o->early_reproduce_energy = (int)j_lround(o->early_reproduce_energy / 10);
        o->isreproductive = true;
        if (o->isaplant) {
            o->early_reproduce_energy = o->reproduce_energy - o->early_reproduce_energy;
            if (o->early_reproduce_energy < 40) {
                o->early_reproduce_energy = 40;
            }
        } else {
            if ((o->isaconsumer) || (o->isafungus)) {
                o->early_reproduce_energy = o->reproduce_energy - o->early_reproduce_energy;
                if (o->early_reproduce_energy < 21) {
                    o->early_reproduce_energy = 21;
                }
            } else {
                if ((o->transfersenergy) && (o->isinfectious) && (o->isblond)) {
                    o->early_reproduce_energy = o->reproduce_energy - o->early_reproduce_energy;
                    if (o->early_reproduce_energy < 21) {
                        o->early_reproduce_energy = 21;
                    }
                } else {
                    if ((o->sporeversion < 0) || (o->sporeversion == 6)) {
                        o->early_reproduce_energy = o->reproduce_energy - o->early_reproduce_energy;
                        if (o->early_reproduce_energy < 21) {
                            o->early_reproduce_energy = 21;
                        }
                    } else {
                        o->early_reproduce_energy = o->reproduce_energy - o->early_reproduce_energy;
                        if (o->early_reproduce_energy < 14) {
                            o->early_reproduce_energy = 14;
                        }
                    }
                }
            }
        }
    }
    // Calculate reproduction energy for flower segments
    if (o->reproducelate > 0) {
        o->reproducelate = (int)j_lround(o->reproducelate / 10);
        if (o->isinfectious) {
            o->reproduce_energy = (40 + 3 * o->segments) + j_d2i((o->reproducelate * k_scale[o->growth_ratio - 1]));
        } else {
            if ((o->reproducelate - (3 * gc_get_symmetry(o->gc))) > 0) {
                o->reproduce_energy = (40 + 3 * o->segments) + o->reproducelate - (3 * gc_get_symmetry(o->gc));
            }
        }
    }
    // Calculate life expectancy with gold segments
    if (o->gold > 0) {
        o->max_age = P.max_age + (o->segments / P.age_divisor) + (int)j_lround((o->gold / 10) / P.gold_divisor);
        o->gold = 1;
    }
}

void org_segments_create_effects(Organism *o)
{
    int i = 0;
    o->spin = 0;
    o->healing = 0;
    o->antiviral = 0;
    o->createlavender = 0;
    o->yellow_counter = 0;
    double fertility = 0;
    o->indigo = 0;
    o->early_reproduce_energy = 0;
    if (o->reproducelate > 0) {
        o->reproducelate = 0;
    }
    o->gold = 0;
    o->haseyes = false;
    bool enhancedconsumer = false;
    bool ispink = false;
    int isprotective = 0;
    o->update_effects = 2;
    if (gc_get_sibling_battle(o->gc)) {
        o->siblingbattle = true;
    }
    if (gc_get_generation_battle(o->gc)) {
        o->generationbattle = true;
    }
    if (gc_get_altruist(o->gc)) {
        o->altruist = true;
    }
    double photofactor = 0;
    if (gc_get_symmetry(o->gc) == 8) {
        if (gc_get_n_genes(o->gc) != 1) {
            photofactor = 1961 + j_lround(6000 / ((double)gc_get_n_genes(o->gc) + 2)) + j_lround(6315 / (double)gc_get_symmetry(o->gc));
        } else {
            photofactor = 1961 + 2200 + j_lround(6315 / (double)gc_get_symmetry(o->gc));
        }
    } else {
        if (gc_get_n_genes(o->gc) != 1) {
            photofactor = 1961.329 + j_lround(6000 / ((double)gc_get_n_genes(o->gc) + 2)) + j_lround(6315 / (double)gc_get_symmetry(o->gc));
        } else {
            photofactor = 1961.329 + 2200 + j_lround(6315 / (double)gc_get_symmetry(o->gc));
        }
        if (gc_get_symmetry(o->gc) >= 6) {
            o->lowersymmetric = 700 + 170;
        } else {
            if (gc_get_symmetry(o->gc) >= 4) {
                o->lowersymmetric = 700 + 240;
            } else {
                o->lowersymmetric = 700 + (1200 / (gc_get_symmetry(o->gc) + 1));
            }
        }
    }
    double photomultiplier = (photofactor * 0.0006) / P.green_obtained_energy_divisor;
    for (i = o->segments - 1; i >= 0; i--) {
        // Manteniment
        switch (org_get_type_color(o->seg_color[i])) {
        case TY_GREEN:
            o->mphoto[i] = P.green_energy_consumption * photomultiplier * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
            o->isaplant = true;
            break;
        case TY_GRASS:
            o->mphoto[i] = P.grass_energy_consumption * photomultiplier * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
            o->isaplant = true;
            break;
        case TY_BARK:
            o->mphoto[i] = P.bark_energy_consumption * photomultiplier * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
            o->isaplant = true;
            break;
        case TY_SUMMER:
        case TY_WINTER:
            o->mphoto[i] = P.summer_energy_consumption * photomultiplier * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
            o->isaplant = true;
            if (o->summerinactivity == -1) {
                if (gc_get_activity(o->gc) == 0) {
                    o->summerinactivity = 0;
                } else {
                    if (gc_get_activity(o->gc) == 1) {
                        o->summerinactivity = 1;
                    } else {
                        o->summerinactivity = 2;
                    }
                }
            }
            break;
        case TY_LEAF:
            o->mphoto[i] = P.leaf_energy_consumption * photomultiplier * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
            o->isaplant = true;
            break;
        case TY_FOREST:
            o->mphoto[i] = P.forest_energy_consumption * photomultiplier * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
            o->isaplant = true;
            o->forestphoto = 1;
            if (o->segmentpretoucheffects == 0) {
                o->segmentpretoucheffects = 1;
            }
            o->usepretoucheffects = true;
            break;
        case TY_LIME:
            o->mphoto[i] = P.lime_energy_consumption * photomultiplier * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
            o->isaplant = true;
            o->islime = true;
            break;
        case TY_JADE:
        case TY_DARKJADE:
            o->mphoto[i] = P.jade_energy_consumption * photomultiplier * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
            o->isaplant = true;
            o->jadefactor = 2;
            break;
        case TY_C4:
            o->mphoto[i] = P.c4_energy_consumption * photomultiplier * (10 + gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc))));
            o->isonlyc4 = true;
            break;
        case TY_SPRING:
            o->mphoto[i] = P.spring_energy_consumption * photomultiplier * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
            o->spin += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            if (gc_get_clockwise(o->gc)) {
                o->clockwise = true;
            }
            o->isaplant = true;
            break;
        case TY_DARKGREEN:
            if (gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc))) == CM_JADE) {
                o->jadefactor = 2;
            }
            if (gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc))) == CM_SPRING) {
                o->spin += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            }
            break;
        case TY_PURPLE:
            if (o->age == 0) {
                o->mphoto[i] = P.purple_energy_consumption * photomultiplier * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
            }
            o->methanotrophy = 1;
            break;
        case TY_TEAL:
            o->canmove = true;
            o->canreact = true;
            if (gc_get_passive(o->gc) == false) {
                o->useframemovement = true;
                o->mphoto[i] = -4;
            }
            break;
        case TY_CYAN:
            o->canmove = true;
            o->useframemovement = true;
            o->mphoto[i] = -3;
            break;
        case TY_RED:
        case TY_FIRE:
        case TY_ORANGE:
        case TY_MAROON:
        case TY_CREAM:
            o->isaconsumer = true;
            o->mphoto[i] = -1;
            break;
        case TY_PINK:
            ispink = true;
            o->isafungus = true;
            if (gc_get_modifiespink(o->gc)) {
                o->isaconsumer = true;
            }
            o->mphoto[i] = -1;
            break;
        case TY_AUBURN:
            o->isauburn = true;
            o->isreproductive = true;
            break;
        case TY_YELLOW:
            o->isreproductive = true;
            o->yellow_counter++;
            if (gc_get_symmetry(o->gc) != 3) {
                fertility += gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
            } else {
                if (gc_get_mirror(o->gc) == 0) {
                    fertility += (5 / 6) * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                } else {
                    fertility += (2 / 3) * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                }
            }
            break;
        case TY_INDIGO:
            o->isreproductive = true;
            if (gc_get_symmetry(o->gc) != 3) {
                o->indigo += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            } else {
                if (gc_get_mirror(o->gc) == 0) {
                    o->indigo += j_d2i(((50 / 6) * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
                } else {
                    o->indigo += j_d2i(((20 / 3) * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
                }
            }
            break;
        case TY_SILVER:
            enhancedconsumer = true;
            o->issilver = true;
            o->mphoto[i] = -1;
            break;
        case TY_WHITE:
            o->isinfectious = true;
            o->mphoto[i] = -2;
            break;
        case TY_PLAGUE:
            if (o->plagueversion == 0) {
                if (!gc_get_plague(o->gc)) {
                    o->plagueversion = 1;
                } else {
                    o->plagueversion = 2;
                }
            }
            o->mphoto[i] = -2;
            break;
        case TY_CORAL:
            o->iscoral = true;
            o->mphoto[i] = -1;
            break;
        case TY_FALLOW:
            if (isprotective == 0) {
                isprotective = 1;
            }
            if (o->fallowversion == 0) {
                if (gc_get_modifiesfallow(o->gc) == 1) {
                    o->fallowversion = 1;
                } else {
                    if (gc_get_modifiesfallow(o->gc) == 2) {
                        o->fallowversion = 2;
                    } else {
                        if (gc_get_modifiesfallow(o->gc) == 3) {
                            o->fallowversion = 3;
                        } else {
                            o->fallowversion = 4;
                        }
                    }
                }
            }
            o->mphoto[i] = -2;
            break;
        case TY_DARKGRAY:
            o->isenhanced = true;
            o->mphoto[i] = 1;
            break;
        case TY_EYE:
            o->haseyes = true;
            break;
        case TY_GOLD:
            o->gold += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            break;
        case TY_FLOWER:
            if (o->reproducelate < 0) {
                o->reproducelate = 0;
            }
            o->reproducelate += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            break;
        case TY_BLOND:
            o->isblond = true;
            o->early_reproduce_energy += 30 + j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            break;
        case TY_SPORE:
            if (gc_get_modifiesspore(o->gc) <= 6) {
                o->isreproductive = true;
                if (gc_get_modifiesspore(o->gc) <= 2) {
                    o->early_reproduce_energy += 20 * gc_get_n_genes(o->gc);
                }
            }
            if (o->age == 0) {
                o->sporetime += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            }
            break;
        case TY_LAVENDER:
            o->createlavender += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            if (o->altruist) {
                o->mphoto[i] = -2;
            }
            o->usefriendeffects = 2;
            break;
        case TY_MINT:
            if (gc_get_symmetry(o->gc) != 3) {
                o->antiviral += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            } else {
                if (gc_get_mirror(o->gc) == 0) {
                    o->antiviral += j_d2i(((50 / 6) * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
                } else {
                    o->antiviral += j_d2i(((20 / 3) * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
                }
            }
            o->mphoto[i] = -2;
            o->usefriendeffects = 2;
            break;
        case TY_MAGENTA:
            if (gc_get_symmetry(o->gc) != 3) {
                o->healing += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            } else {
                if (gc_get_mirror(o->gc) == 0) {
                    o->healing += j_d2i(((50 / 6) * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
                } else {
                    o->healing += j_d2i(((20 / 3) * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
                }
            }
            o->mphoto[i] = -2;
            o->usefriendeffects = 2;
            break;
        case TY_ROSE:
            if (o->transfersenergy == false) {
                o->transfersenergy = true;
                o->lengthfriend = j_d2i((gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
                o->thetafriend = j_d2i((gene_get_theta(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            }
            if (o->altruist) {
                o->mphoto[i] = -2;
            }
            if (o->usefriendeffects < 1) {
                o->usefriendeffects = 1;
            }
            break;
        case TY_VIOLET:
            o->mphoto[i] = -2;
            break;
        case TY_OCHRE:
            if (isprotective == 0) {
                isprotective = 1;
            }
            o->mphoto[i] = -1;
            break;
        case TY_OLIVE:
        case TY_DARKOLIVE:
            isprotective = 2;
            o->mphoto[i] = -2;
            break;
        case TY_BLUE:
            if (isprotective == 0) {
                isprotective = 1;
            }
            break;
        case TY_SKY:
        case TY_DEEPSKY:
            if (isprotective == 0) {
                isprotective = 1;
            }
            if (o->skyversion <= 0) {
                if (gc_get_modifiessky(o->gc)) {
                    o->skyversion = 2;
                } else {
                    o->skyversion = 1;
                }
            }
            o->mphoto[i] = -2;
            break;
        case TY_GRAY:
            o->isgray = true;
            o->isakiller = true;
            o->mphoto[i] = -2;
            break;
        case TY_LILAC:
        case TY_DARKLILAC:
            o->isakiller = true;
            o->mphoto[i] = -1;
            break;
        case TY_SPIKE:
            enhancedconsumer = true;
            o->isspike = true;
            o->isakiller = true;
            o->usepretoucheffects = true;
            o->mphoto[i] = 0;
            break;
        case TY_SPIKEPOINT:
            enhancedconsumer = true;
            o->isspike = true;
            o->isakiller = true;
            o->usepretoucheffects = true;
            o->mphoto[i] = -1;
            break;
        case TY_DARK:
            isprotective = 2;
            o->segmentpretoucheffects = 2;
            o->usepretoucheffects = true;
            o->mphoto[i] = 0;
            break;
        }
    }
    if (enhancedconsumer) {
        if (o->isenhanced) {
            o->isaconsumer = true;
        }
        if (o->issilver) {
            if (ispink) {
                o->isaconsumer = true;
            }
            if (o->iscoral) {
                o->isinfectious = true;
            }
            if ((o->isakiller) || ((o->plagueversion > 0) && (!o->isinfectious))) {
                o->isafungus = true;
            }
        }
    }
    if (o->isonlyc4) {
        if ((!o->isaplant) && (o->methanotrophy == 0) && (!o->isaconsumer) && (!o->isafungus) && (!o->isakiller) && (o->skyversion == 0) && (!o->isinfectious) && (!o->iscoral) && (o->plagueversion == 0) && (isprotective == 0)) {
            o->candodge = true;
            if ((o->sporetime == 0) || (gc_get_modifiesspore(o->gc) <= 6)) {
                int j = 0;
                for (j = o->segments - 1; j >= 0; j--) {
                    if (o->seg_color[j] == CM_C4) {
                        o->mphoto[j] = P.c4_energy_consumption * photomultiplier * (11 + gene_get_length(gc_get_gene(o->gc, j % gc_get_n_genes(o->gc))));
                    }
                }
            }
            if ((o->indigo > 0) && (o->jadefactor == 0)) {
                o->jadefactor = -1;
            }
        } else {
            o->isonlyc4 = false;
        }
        o->isaplant = true;
    }
    // Can this organism dodge?
    if (o->canreact) {
        if ((!o->isaconsumer) && (!o->isafungus) && (!o->isakiller) && (o->skyversion == 0) && (isprotective <= 1) && (((!o->isinfectious) && (!o->iscoral) && (o->plagueversion == 0)) || ((!o->isaplant) && (o->methanotrophy == 0) && (gc_get_passive(o->gc))))) {
            o->candodge = true;
        } else {
            o->candodge = false;
            o->dodge = false;
        }
    }
    // Calculate jade delay used in restoration of the color
    if (o->jadefactor > 1) {
        if (gc_get_symmetry(o->gc) != 1) {
            o->jadefactor = P.darkjade_delay * gc_get_symmetry(o->gc);
        } else {
            o->jadefactor = P.darkjade_delay * 2;
        }
    }
    // Give non green methanotrophs the ability of jade during virus hatching, then make them count as plants and lower effectivity for mixed organisms
    if (o->methanotrophy > 0) {
        if ((o->isenhanced) && (!o->isakiller)) {
            o->segmentpretoucheffects = 2;
            o->usepretoucheffects = true;
        }
        if (!o->isaplant) {
            if (o->jadefactor == 0) {
                o->jadefactor = -1;
            }
        } else {
            if (o->age == 0) {
                int p = 0;
                for (p = o->segments - 1; p >= 0; p--) {
                    if (o->seg_color[p] == CM_PURPLE) {
                        o->mphoto[p] = 0.55 * P.purple_energy_consumption * photomultiplier * gene_get_length(gc_get_gene(o->gc, p % gc_get_n_genes(o->gc)));
                    }
                }
            }
        }
        o->isaplant = true;
    }
    if ((o->transfersenergy) && (!o->isaplant)) {
        if ((!o->isaconsumer) && (!o->isafungus)) {
            if (o->segmentpretoucheffects == 0) {
                o->segmentpretoucheffects = 1;
            }
            o->usepretoucheffects = true;
            if (o->lengthfriend > 0) {
                o->colony_photosynthesis = P.symbiont_energy_consumption * (o->lengthfriend + 9);
                o->lengthfriend = -o->lengthfriend;
            }
        } else {
            o->colony_photosynthesis = 0;
        }
    }
    // Calculate the total shield production capacity of lavender segments
    if (o->createlavender > 0) {
        o->createlavender = o->createlavender / gc_get_symmetry(o->gc);
        if (o->age == 0) {
            o->lavender = P.lavender_shield;
        }
    }
    if (o->sporetime > 0) {
        if (o->age == 0) {
            if (gc_get_modifiesspore(o->gc) <= 6) {
                o->sporeversion = gc_get_modifiesspore(o->gc);
                o->sporetime = ((o->sporetime / gc_get_symmetry(o->gc)) / 10) + 2;
                if (o->createlavender > 0) {
                    o->lavender = 200;
                }
                if ((!o->isaplant) && (!o->isaconsumer) && (!o->isafungus)) {
                    if (o->sporeversion <= 2) {
                        o->time_to_reproduce = o->time_to_reproduce_max;
                    }
                }
            } else {
                if (gc_get_modifiesspore(o->gc) <= 9) {
                    o->sporeversion = -2;
                } else {
                    o->sporeversion = -3;
                }
                o->time_to_reproduce_max -= 6;
            }
        } else {
            if (o->age == 1) {
                if ((!o->isaplant) && (!o->isaconsumer) && (!o->isafungus)) {
                    if (o->sporeversion >= 5) {
                        o->time_to_reproduce = o->time_to_reproduce_max;
                        if (o->sporeversion == 5) {
                            o->isafungus = true;
                        }
                    }
                }
            }
        }
    }
    // Calculate extra children for yellow
    if (o->yellow_counter > 0) {
        o->yellow_counter = (o->yellow_counter / gc_get_symmetry(o->gc)) + j_d2i((fertility / 23));
        if ((o->isinfectious) && (!o->isaconsumer) && (!o->isaplant) && (!o->isafungus)) {
            // Yellow of viruses uses touch effects
            int j = 0;
            for (j = o->segments - 1; j >= 0; j--) {
                if (o->seg_color[j] == JC_YELLOW) {
                    o->mphoto[j] = -2;
                }
            }
        }
    }
    // Calculate indigo
    if (o->indigo > 0) {
        o->indigo = j_d2i((o->indigo / P.indigo_divisor));
    }
    // Calculate reproduction energy for blond segments
    if (o->early_reproduce_energy > 0) {
        o->early_reproduce_energy = (int)j_lround(o->early_reproduce_energy / 10);
        o->isreproductive = true;
        if (o->isaplant) {
            o->early_reproduce_energy = o->reproduce_energy - o->early_reproduce_energy;
            if (o->early_reproduce_energy < 40) {
                o->early_reproduce_energy = 40;
            }
        } else {
            if ((o->isaconsumer) || (o->isafungus)) {
                o->early_reproduce_energy = o->reproduce_energy - o->early_reproduce_energy;
                if (o->early_reproduce_energy < 21) {
                    o->early_reproduce_energy = 21;
                }
            } else {
                if ((o->transfersenergy) && (o->isinfectious) && (o->isblond)) {
                    o->early_reproduce_energy = o->reproduce_energy - o->early_reproduce_energy;
                    if (o->early_reproduce_energy < 21) {
                        o->early_reproduce_energy = 21;
                    }
                    if (o->age == 0) {
                        o->time_to_reproduce = o->time_to_reproduce_max;
                    }
                } else {
                    if ((o->sporeversion < 0) || (o->sporeversion == 6)) {
                        o->early_reproduce_energy = o->reproduce_energy - o->early_reproduce_energy;
                        if (o->early_reproduce_energy < 21) {
                            o->early_reproduce_energy = 21;
                        }
                    } else {
                        o->early_reproduce_energy = o->reproduce_energy - o->early_reproduce_energy;
                        if (o->early_reproduce_energy < 14) {
                            o->early_reproduce_energy = 14;
                        }
                    }
                }
                if ((o->createlavender > 0) && (o->isinfectious) && (o->isblond) && (o->age == 0)) {
                    o->lavender = 0;
                }
            }
        }
    }
    // Calculate reproduction energy for flower segments
    if (o->reproducelate > 0) {
        o->reproducelate = (int)j_lround(o->reproducelate / 10);
        if (o->isinfectious) {
            o->reproduce_energy = (40 + 3 * o->segments) + j_d2i((o->reproducelate * k_scale[o->growth_ratio - 1]));
        } else {
            if ((o->reproducelate - (3 * gc_get_symmetry(o->gc))) > 0) {
                o->reproduce_energy = (40 + 3 * o->segments) + o->reproducelate - (3 * gc_get_symmetry(o->gc));
            }
            if (o->age == 0) {
                o->time_to_reproduce_max = 14;
            }
        }
    }
    // Calculate life expectancy with gold segments
    if (o->gold > 0) {
        o->max_age = P.max_age + (o->segments / P.age_divisor) + (int)j_lround((o->gold / 10) / P.gold_divisor);
        o->gold = 1;
    }
}

/* Initializes variables for a new random organism and finds a place
 * to put it in the world.
 * 
 * @return  true if it found a place for this organism or false otherwise.
 */
bool org_random_create(Organism *o)
{
    // Generates a random genetic code
    o->gc = gc_new_random();
    // it has no parent
    o->parent_id = -1;
    o->gc->generation = 1;
    o->growth_ratio = 16;
    // initial energy
    o->energy = jmin(P.initial_energy, world_get_co2(o->world));
    world_decrease_co2(o->world, o->energy);
    world_add_o2(o->world, o->energy);
    // initialize
    org_create(o);
    org_symmetric(o);
    // put it in the world
    return org_place_random(o);
}

/* Initializes variables for a new organism born from an existing
 * organism. Generates a mutated genetic code based on the parent's one
 * and finds a place in the world to put it.
 * 
 * @param parent  The organism from which this organism is born.
 * @return  true if it found a place for this organism or false otherwise.
 */
bool org_inherit(Organism *o, Organism *parent, bool first)
{
    GeneticCode *inheritGeneticCode = NULL;
    bool ok = true;
    int sporeremember = 0;
    int inheritReproduceEnergy = 1;
    // Prepare the inherited genetic code
    if (parent->sporeversion > 0) {
        if (parent->infected_gc != NULL) {
            // inheritReproduceEnergy is used for virus delay at first here
            if ((parent->isaconsumer) || (parent->isafungus)) {
                if (parent->isreproductive) {
                    if (parent->n_virus_children == 0) {
                        inheritReproduceEnergy = 14 + (gc_get_symmetry(parent->gc) / 2);
                    } else {
                        if (parent->n_virus_children == 1) {
                            if (gc_get_plague(parent->infected_gc)) {
                                inheritReproduceEnergy = 4 + (gc_get_symmetry(parent->gc) / 2);
                            } else {
                                if (gc_get_symmetry(parent->infected_gc) != 2) {
                                    inheritReproduceEnergy = (120 / gc_get_symmetry(parent->infected_gc)) - 11 + (gc_get_symmetry(parent->gc) / 2);
                                } else {
                                    inheritReproduceEnergy = 40 - 11 + (gc_get_symmetry(parent->gc) / 2);
                                }
                            }
                        }
                    }
                }
            } else {
                if (parent->isreproductive) {
                    if (parent->n_virus_children == 0) {
                        inheritReproduceEnergy = 14 + (gc_get_symmetry(parent->gc) / 2);
                    } else {
                        if (parent->n_virus_children == 1) {
                            if (gc_get_symmetry(parent->infected_gc) != 2) {
                                inheritReproduceEnergy = (120 / gc_get_symmetry(parent->infected_gc)) - 11 + (gc_get_symmetry(parent->gc) / 2);
                            } else {
                                inheritReproduceEnergy = 40 - 11 + (gc_get_symmetry(parent->gc) / 2);
                            }
                        }
                    }
                }
            }
            if ((first && jr_next_int(&g_rng, inheritReproduceEnergy) < 3) || (parent->n_virus_children > 0 && !first) || (parent->sporetime < 0)) {
                if (parent->infected_reproduce_energy > 0) {
                    inheritGeneticCode = parent->infected_gc;
                    inheritReproduceEnergy = parent->infected_reproduce_energy;
                    if (parent->n_virus_children == 0) {
                        sporeremember = 1;
                    } else {
                        sporeremember = 0;
                    }
                } else {
                    inheritReproduceEnergy = 2 + (gc_get_symmetry(parent->gc) / 4);
                    if ((first && jr_next_int(&g_rng, inheritReproduceEnergy) < 1) || (parent->n_virus_children > 0 && !first) || (parent->sporetime < 0)) {
                        inheritGeneticCode = parent->infected_gc;
                        inheritReproduceEnergy = -parent->infected_reproduce_energy;
                        if (parent->n_virus_children == 0) {
                            sporeremember = 1;
                        } else {
                            sporeremember = 0;
                        }
                    } else {
                        parent->n_children = 1;
                        parent->n_virus_children = 0;
                        inheritReproduceEnergy = 1;
                        sporeremember = -1;
                        org_set_color(parent, CM_SPORE);
                        return false;
                    }
                }
            } else {
                if (parent->n_virus_children != 0) {
                    parent->n_children = 1;
                    parent->n_virus_children = 0;
                    inheritReproduceEnergy = 1;
                    sporeremember = -1;
                    org_set_color(parent, CM_SPORE);
                    return false;
                } else {
                    inheritGeneticCode = parent->gc;
                    if (parent->early_reproduce_energy > 0) {
                        inheritReproduceEnergy = parent->early_reproduce_energy;
                    } else {
                        inheritReproduceEnergy = parent->reproduce_energy;
                    }
                    if (parent->sporeversion <= 2) {
                        parent->infected_gc = NULL;
                        parent->saved_gc = NULL;
                        sporeremember = -2;
                    } else {
                        sporeremember = -1;
                    }
                    if (parent->isauburn && first) {
                        parent->n_children -= 1;
                    }
                }
            }
        } else {
            if (parent->sporetime > 0) {
                inheritGeneticCode = parent->gc;
                if (parent->early_reproduce_energy > 0) {
                    inheritReproduceEnergy = parent->early_reproduce_energy;
                } else {
                    inheritReproduceEnergy = parent->reproduce_energy;
                }
                if (parent->sporeversion <= 2) {
                    sporeremember = -2;
                }
            } else {
                if (first) {
                    parent->sporetime = -parent->sporetime;
                }
                parent->n_children = 1;
                org_set_color(parent, CM_FLOWER);
                return false;
            }
        }
    } else {
        if (parent->infected_gc != NULL) {
            inheritGeneticCode = parent->infected_gc;
            inheritReproduceEnergy = parent->infected_reproduce_energy;
        } else {
            inheritGeneticCode = parent->gc;
            if (parent->early_reproduce_energy > 0) {
                inheritReproduceEnergy = parent->early_reproduce_energy;
            } else {
                inheritReproduceEnergy = parent->reproduce_energy;
            }
        }
    }
    // Initial energy: minimum energy required to reproduce is divided
    // between all children and the parent.
    if ((parent->sporeversion > 0) && (inheritGeneticCode == parent->gc)) {
        if ((parent->sporeversion <= 3) || (parent->isblond)) {
            o->energy = jmin(parent->sporetime, parent->energy);
        } else {
            o->energy = jmin(parent->reproduce_energy * 0.025 * parent->sporetime, parent->energy);
        }
    } else if ((gc_get_selfish(parent->gc)) && (inheritGeneticCode == parent->gc)) {
        o->energy = jmin(((inheritReproduceEnergy / 2) / (double)parent->n_children), parent->energy);
    } else if ((parent->indigo > 0) && (inheritGeneticCode == parent->infected_gc)) {
        if ((parent->jadefactor != 0) && (!parent->isaconsumer) && (!parent->isafungus) && (inheritReproduceEnergy > parent->reproduce_energy)) {
            o->energy = jmin(((parent->reproduce_energy / (((parent->indigo * k_scale[parent->growth_ratio - 1]) / 10) + 1)) / (double)(parent->n_children + 1)), parent->energy);
        } else {
            o->energy = jmin(((inheritReproduceEnergy / (((parent->indigo * k_scale[parent->growth_ratio - 1]) / 10) + 1)) / (double)(parent->n_children + 1)), parent->energy);
        }
        if ((gc_get_symmetry(inheritGeneticCode) != 3) || (gc_get_mirror(inheritGeneticCode) == 0)) {
            if (o->energy < 3) {
                parent->infected_gc = NULL;
                parent->saved_gc = NULL;
                parent->n_children = 1;
                parent->n_virus_children = 0;
                if (parent->time_to_reproduce > 20) {
                    parent->time_to_reproduce = 20;
                }
                return false;
            }
        } else {
            if (o->energy < 4) {
                parent->infected_gc = NULL;
                parent->saved_gc = NULL;
                parent->n_children = 1;
                parent->n_virus_children = 0;
                if (parent->time_to_reproduce > 20) {
                    parent->time_to_reproduce = 20;
                }
                return false;
            }
        }
    } else {
        if ((parent->jadefactor != 0) && (!parent->isaconsumer) && (!parent->isafungus) && (inheritReproduceEnergy > parent->reproduce_energy)) {
            o->energy = jmin((parent->reproduce_energy / (double)(parent->n_children + 1)), parent->energy);
        } else {
            o->energy = jmin((inheritReproduceEnergy / (double)(parent->n_children + 1)), parent->energy);
        }
    }
    // Create the inherited genetic code
    if (sporeremember == -2) {
        o->gc = parent->gc;
    } else if ((parent->saved_gc != NULL) && (sporeremember >= 0)) {
        o->gc = parent->saved_gc;
    } else {
        o->gc = gc_new_mutated(inheritGeneticCode);
    }
    // Take a reference to the parent
    o->parent_id = org_get_id(parent);
    o->growth_ratio = 16;
    // Initialize
    org_create(o);
    org_symmetric(o);
    // Put it in the world, near its parent
    if ((parent->n_children <= 1) && (parent->n_virus_children <= 1)) {
        ok = org_place_near(o, parent);
    } else {
        if (first) {
            parent->n_position = jstr_lit("01234567");
            ok = org_place_near_first(o, parent);
        } else {
            ok = org_place_near_yellow(o, parent);
        }
    }
    if (ok) {
        if (parent->reproducelate != 0) {
            if (parent->reproducelate > 0) {
                if (!parent->isinfectious) {
                    parent->time_to_reproduce = 0;
                }
            } else {
                // Organism affected by modified fallow without white segments will have a dead child
                if (inheritGeneticCode == parent->gc) {
                    if (parent->time_to_reproduce == parent->time_to_reproduce_max) {
                        parent->time_to_reproduce--;
                        if ((!parent->isaconsumer) && (!parent->isafungus)) {
                            if (parent->reproducelate < -1) {
                                parent->reproducelate = -1;
                            } else {
                                parent->reproducelate++;
                            }
                        } else {
                            parent->reproducelate++;
                        }
                    }
                    if (parent->reproducelate < 0) {
                        inheritReproduceEnergy = 0;
                    }
                }
            }
        }
        if ((parent->sporeversion < 0) && (parent->n_virus_children == 0)) {
            if (inheritGeneticCode == parent->infected_gc) {
                if (parent->n_children <= 1) {
                    parent->time_to_reproduce = 0;
                }
            }
        }
        if (sporeremember == 1) {
            parent->infected_gc = NULL;
            parent->saved_gc = NULL;
        }
        // Maximum age that an organism can reach
        o->max_age = P.max_age + (o->segments / P.age_divisor);
        // Calculates the energy required to reproduce this genetic code.
        o->reproduce_energy = (40 + 3 * o->segments);
        // Maximum time that needs to elapse after a reproduction attempt
        o->time_to_reproduce_max = 8;
        int additionalcentersegments = 0;
        for (int j = 0; j < gc_get_n_genes(o->gc); j++) {
            if ((gene_get_branch(gc_get_gene(o->gc, j)) != 0) || (j == 0)) {
                o->time_to_reproduce_max += 6;
            } else {
                additionalcentersegments += gc_get_symmetry(o->gc);
                if ((gc_get_symmetry(o->gc) + additionalcentersegments) > 8) {
                    o->time_to_reproduce_max += 9;
                } else {
                    o->time_to_reproduce_max += 6;
                }
            }
        }
        if (inheritReproduceEnergy <= 4) {
            // Virus created by this noninfectious plague organism, or child of an organism affected by modified fallow will die at birth
            o->age = ((o->max_age + 1) * 512);
            for (int i = 0; i < o->segments; i++) {
                if (o->seg_color[i] == CM_EYE) {
                    o->seg_color[i] = CM_EYE;
                } else {
                    o->seg_color[i] = CM_BROWN;
                }
            }
            if (inheritReproduceEnergy > 0) {
                parent->time_to_reproduce = 0;
            }
        } else {
            // Initialize effects
            org_segments_create_effects(o);
            // Create spores
            if (o->sporetime > 0) {
                if (o->sporeversion == 1) {
                    if (parent->sporeversion == 1) {
                        if ((org_use_energy(o, P.spore_energy_consumption)) || (org_use_energy(o, o->energy))) {
                            for (int i = 0; i < o->segments; i++) {
                                o->seg_color[i] = CM_FRUIT;
                                o->mphoto[i] = -2;
                            }
                        }
                        o->usepretoucheffects = false;
                        o->usefriendeffects = 0;
                        o->canreact = false;
                        o->active = false;
                        o->has_moved = true;
                        o->isenhanced = false;
                        if ((!o->isaplant) && (!o->isaconsumer) && (!o->isafungus)) {
                            o->isenhanced = true;
                        }
                        o->isaplant = false;
                        o->isaconsumer = false;
                        o->isafungus = false;
                        if (o->haseyes) {
                            o->haseyes = false;
                            org_symmetric(o);
                        }
                    }
                } else if (o->sporeversion == 2) {
                    if (parent->sporeversion == 2) {
                        if ((org_use_energy(o, P.spore_energy_consumption)) || (org_use_energy(o, o->energy))) {
                            for (int i = 0; i < o->segments; i++) {
                                o->seg_color[i] = CM_FRUIT;
                                o->mphoto[i] = 0;
                            }
                        }
                        o->usepretoucheffects = false;
                        o->usefriendeffects = 0;
                        o->canreact = false;
                        o->active = false;
                        o->has_moved = true;
                        o->isenhanced = false;
                        if ((o->isaplant) && (!o->isafungus) && (!o->isaconsumer)) {
                            o->isenhanced = true;
                            o->max_age = o->sporetime;
                            if (o->max_age > 20) {
                                o->max_age = 20;
                            }
                        } else {
                            o->max_age = o->sporetime - 1;
                            if (o->max_age > 19) {
                                o->max_age = 19;
                            }
                        }
                        o->isaplant = false;
                        o->isaconsumer = false;
                        o->isafungus = false;
                        if (o->haseyes) {
                            o->haseyes = false;
                            org_symmetric(o);
                        }
                    }
                } else if (o->sporeversion == 3) {
                    if (parent->sporeversion == 3) {
                        if (o->isaplant) {
                            if ((o->isaconsumer) || (o->isafungus)) {
                                if (gc_get_disperse_children(parent->gc)) {
                                    o->dx = 2 * o->dx;
                                    o->dy = 2 * o->dy;
                                }
                                o->isenhanced = false;
                                for (int i = 0; i < o->segments; i++) {
                                    o->seg_color[i] = CM_BARK;
                                    o->mphoto[i] = 0;
                                }
                            } else {
                                if (gc_get_disperse_children(parent->gc)) {
                                    if (!o->canmove) {
                                        o->dx = 3 * o->dx;
                                        o->dy = 3 * o->dy;
                                    } else {
                                        o->dx = 2 * o->dx;
                                        o->dy = 2 * o->dy;
                                    }
                                }
                                o->isenhanced = true;
                                for (int i = 0; i < o->segments; i++) {
                                    o->seg_color[i] = CM_BARK;
                                    o->mphoto[i] = 0;
                                }
                            }
                        } else {
                            if (gc_get_disperse_children(parent->gc)) {
                                if (!o->canmove) {
                                    o->dx = 3 * o->dx;
                                    o->dy = 3 * o->dy;
                                } else {
                                    o->dx = 2 * o->dx;
                                    o->dy = 2 * o->dy;
                                }
                            }
                            if ((o->isaconsumer) || (o->isafungus)) {
                                o->isenhanced = false;
                                for (int i = 0; i < o->segments; i++) {
                                    o->seg_color[i] = CM_VISION;
                                    o->mphoto[i] = 0;
                                }
                            } else {
                                o->isenhanced = true;
                                for (int i = 0; i < o->segments; i++) {
                                    o->seg_color[i] = CM_BARK;
                                    o->mphoto[i] = 0;
                                }
                            }
                        }
                        o->time_to_reproduce = 104 - o->sporetime;
                        o->usepretoucheffects = false;
                        o->usefriendeffects = 0;
                        o->canreact = false;
                        o->active = false;
                        o->has_moved = true;
                        o->isaplant = false;
                        o->isaconsumer = false;
                        o->isafungus = false;
                        if (o->haseyes) {
                            o->haseyes = false;
                            org_symmetric(o);
                        }
                    }
                } else if (o->sporeversion == 4) {
                    if (parent->sporeversion == 4) {
                        for (int i = 0; i < o->segments; i++) {
                            o->seg_color[i] = CM_BROWN;
                            o->mphoto[i] = 0;
                        }
                        if ((o->isaconsumer) || (o->isafungus)) {
                            o->time_to_reproduce = 115 - o->sporetime + jr_next_int(&g_rng, 2);
                            o->isenhanced = false;
                        } else {
                            o->time_to_reproduce = 104 - o->sporetime;
                            o->isenhanced = true;
                        }
                        o->usepretoucheffects = false;
                        o->usefriendeffects = 0;
                        o->canreact = false;
                        o->active = false;
                        o->has_moved = true;
                        o->isaplant = false;
                        o->isaconsumer = false;
                        o->isafungus = false;
                        if (o->haseyes) {
                            o->haseyes = false;
                            org_symmetric(o);
                        }
                    }
                } else if (o->sporeversion == 5) {
                    if (parent->sporeversion == 5) {
                        for (int i = 0; i < o->segments; i++) {
                            o->seg_color[i] = CM_CREAM;
                            o->mphoto[i] = -1;
                        }
                        o->usepretoucheffects = false;
                        o->usefriendeffects = 0;
                        o->canreact = false;
                        o->active = false;
                        o->has_moved = true;
                        o->isaplant = false;
                        o->isaconsumer = true;
                        o->isafungus = false;
                        o->isenhanced = false;
                        if (o->haseyes) {
                            o->haseyes = false;
                            org_symmetric(o);
                        }
                    }
                } else if (o->sporeversion == 6) {
                    if (parent->sporeversion == 6) {
                        bool largeenough = false;
                        o->usepretoucheffects = false;
                        if (o->isaplant) {
                            if ((o->isaconsumer) || (o->isafungus)) {
                                if (o->methanotrophy > 0) {
                                    for (int i = 0; i < o->segments; i++) {
                                        o->seg_color[i] = CM_PURPLE;
                                        o->mphoto[i] = 0;
                                        if (o->m[i] >= 1) {
                                            largeenough = true;
                                        }
                                    }
                                    if (largeenough) {
                                        o->methanotrophy = (60 * o->mass) / P.green_obtained_energy_divisor;
                                    }
                                } else {
                                    if (o->canmove) {
                                        if (o->jadefactor > 0) {
                                            o->jadefactor = 1;
                                        }
                                        o->islime = false;
                                        for (int i = 0; i < o->segments; i++) {
                                            o->seg_color[i] = CM_C4;
                                            o->mphoto[i] = 0;
                                            if (o->m[i] >= 1) {
                                                largeenough = true;
                                            }
                                        }
                                        if (largeenough) {
                                            o->photosynthesis = (7.5 * o->mass) / P.green_obtained_energy_divisor;
                                        }
                                    } else {
                                        if (o->jadefactor > 0) {
                                            o->jadefactor = 1;
                                        }
                                        o->islime = false;
                                        for (int i = 0; i < o->segments; i++) {
                                            o->seg_color[i] = CM_LEAF;
                                            o->mphoto[i] = 0;
                                            if (o->m[i] >= 1) {
                                                largeenough = true;
                                            }
                                        }
                                        if (largeenough) {
                                            o->photosynthesis = (8 * o->mass) / P.green_obtained_energy_divisor;
                                        }
                                    }
                                }
                            } else {
                                if (o->methanotrophy > 0) {
                                    for (int i = 0; i < o->segments; i++) {
                                        o->seg_color[i] = CM_PURPLE;
                                        o->mphoto[i] = 0;
                                        if (o->m[i] >= 1) {
                                            largeenough = true;
                                        }
                                    }
                                    if (largeenough) {
                                        o->methanotrophy = (120 * o->mass) / P.green_obtained_energy_divisor;
                                    }
                                } else {
                                    if (o->segmentpretoucheffects == 2) {
                                        o->islime = false;
                                        for (int i = 0; i < o->segments; i++) {
                                            o->seg_color[i] = CM_WINTER;
                                            o->mphoto[i] = 0;
                                            if (o->m[i] >= 1) {
                                                largeenough = true;
                                            }
                                        }
                                        if (largeenough) {
                                            o->photosynthesis = (12 * o->mass) / P.green_obtained_energy_divisor;
                                        }
                                    } else {
                                        if ((o->isakiller) || (o->fallowversion > 0)) {
                                            o->islime = false;
                                            for (int i = 0; i < o->segments; i++) {
                                                o->seg_color[i] = CM_SUMMER;
                                                o->mphoto[i] = 0;
                                                if (o->m[i] >= 1) {
                                                    largeenough = true;
                                                }
                                            }
                                            if (largeenough) {
                                                o->photosynthesis = (15 * o->mass) / P.green_obtained_energy_divisor;
                                            }
                                        } else {
                                            if (o->islime) {
                                                for (int i = 0; i < o->segments; i++) {
                                                    o->seg_color[i] = CM_LIME;
                                                    o->mphoto[i] = 0;
                                                    if (o->m[i] >= 1) {
                                                        largeenough = true;
                                                    }
                                                }
                                                if (largeenough) {
                                                    o->photosynthesis = (15 * o->mass) / P.green_obtained_energy_divisor;
                                                    o->leafphoto = (20 * o->mass) / P.green_obtained_energy_divisor;
                                                }
                                            } else {
                                                if (o->forestphoto > 0) {
                                                    for (int i = 0; i < o->segments; i++) {
                                                        o->seg_color[i] = CM_FOREST;
                                                        o->mphoto[i] = 1;
                                                        if (o->m[i] >= 1) {
                                                            largeenough = true;
                                                        }
                                                    }
                                                    if (largeenough) {
                                                        o->photosynthesis = (18 * o->mass) / P.green_obtained_energy_divisor;
                                                        o->forestphoto = o->photosynthesis;
                                                        o->usepretoucheffects = true;
                                                    }
                                                } else {
                                                    if (o->isenhanced) {
                                                        for (int i = 0; i < o->segments; i++) {
                                                            o->seg_color[i] = CM_GRASS;
                                                            o->mphoto[i] = 0;
                                                            if (o->m[i] >= 1) {
                                                                largeenough = true;
                                                            }
                                                        }
                                                        if (largeenough) {
                                                            o->photosynthesis = (19 * o->mass) / P.green_obtained_energy_divisor;
                                                        }
                                                    } else {
                                                        if ((o->jadefactor > 1) && (!o->isblond)) {
                                                            for (int i = 0; i < o->segments; i++) {
                                                                o->seg_color[i] = CM_JADE;
                                                                o->mphoto[i] = 0;
                                                                if (o->m[i] >= 1) {
                                                                    largeenough = true;
                                                                }
                                                            }
                                                            if (largeenough) {
                                                                o->photosynthesis = (19 * o->mass) / P.green_obtained_energy_divisor;
                                                            }
                                                        } else {
                                                            for (int i = 0; i < o->segments; i++) {
                                                                o->seg_color[i] = JC_GREEN;
                                                                o->mphoto[i] = 0;
                                                                if (o->m[i] >= 1) {
                                                                    largeenough = true;
                                                                }
                                                            }
                                                            if (largeenough) {
                                                                o->photosynthesis = (19 * o->mass) / P.green_obtained_energy_divisor;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ((o->isaconsumer) || (o->isafungus)) {
                                o->isaplant = true;
                                for (int i = 0; i < o->segments; i++) {
                                    o->seg_color[i] = CM_SPRING;
                                    o->mphoto[i] = 0;
                                    if (o->m[i] >= 1) {
                                        largeenough = true;
                                    }
                                }
                                if (largeenough) {
                                    o->photosynthesis = (7 * o->mass) / P.green_obtained_energy_divisor;
                                }
                            } else {
                                o->isaplant = true;
                                o->gold = -1;
                                for (int i = 0; i < o->segments; i++) {
                                    o->seg_color[i] = CM_PURPLE;
                                    o->mphoto[i] = 0;
                                    if (o->m[i] >= 1) {
                                        largeenough = true;
                                    }
                                }
                                if (largeenough) {
                                    o->methanotrophy = (22 * o->mass) / P.green_obtained_energy_divisor;
                                }
                            }
                        }
                        if (gc_get_symmetry(o->gc) == 3) {
                            if (gc_get_mirror(o->gc) == 0) {
                                o->photosynthesis = (5.0 / 6.0) * o->photosynthesis;
                                o->methanotrophy = (5.0 / 6.0) * o->methanotrophy;
                            } else {
                                o->photosynthesis = (2.0 / 3.0) * o->photosynthesis;
                                o->methanotrophy = (2.0 / 3.0) * o->methanotrophy;
                            }
                        }
                        if (o->gold != 0) {
                            o->max_age = P.max_age + (o->segments / P.age_divisor);
                        }
                        if ((o->isblond) && (o->jadefactor > 0)) {
                            o->jadefactor = 1;
                        }
                        o->usefriendeffects = 0;
                        o->canreact = false;
                        o->candodge = false;
                        o->active = false;
                        o->has_moved = true;
                        o->isreproductive = false;
                        if (o->haseyes) {
                            o->haseyes = false;
                            org_symmetric(o);
                        }
                    }
                } else if (gc_get_modifiesspore(o->gc) <= 9) {
                    if (gc_get_adaptspore(o->gc) <= 20) {
                        org_blue_spore(o);
                    } else if (gc_get_adaptspore(o->gc) <= 30) {
                        if (jr_next_boolean(&g_rng)) {
                            org_blue_spore(o);
                        }
                    } else if (gc_get_adaptspore(o->gc) <= 40) {
                        if (jr_next_int(&g_rng, 4) == 0) {
                            org_blue_spore(o);
                        }
                    } else if (gc_get_adaptspore(o->gc) <= 50) {
                        if (jr_next_int(&g_rng, 8) == 0) {
                            org_blue_spore(o);
                        }
                    } else if (gc_get_adaptspore(o->gc) <= 60) {
                        if (jr_next_int(&g_rng, 16) == 0) {
                            org_blue_spore(o);
                        }
                    }
                } else if (gc_get_modifiesspore(o->gc) <= 12) {
                    if (gc_get_adaptspore(o->gc) <= 20) {
                        org_cyan_spore(o);
                    } else if (gc_get_adaptspore(o->gc) <= 30) {
                        if (jr_next_boolean(&g_rng)) {
                            org_cyan_spore(o);
                        }
                    } else if (gc_get_adaptspore(o->gc) <= 40) {
                        if (jr_next_int(&g_rng, 4) == 0) {
                            org_cyan_spore(o);
                        }
                    } else if (gc_get_adaptspore(o->gc) <= 50) {
                        if (jr_next_int(&g_rng, 8) == 0) {
                            org_cyan_spore(o);
                        }
                    } else if (gc_get_adaptspore(o->gc) <= 60) {
                        if (jr_next_int(&g_rng, 16) == 0) {
                            org_cyan_spore(o);
                        }
                    }
                }
            }
        }
        // Create subclades
        if (gc_get_update_clade(o->gc)) {
            o->gc->update_clade = false;
            int subcladecounter = 0;
            {
                char chars_1[JSTR_LOOP_MAX];
                jstr_copy_to(chars_1, sizeof chars_1, o->gc->clade_id);
                for (const char *p_2 = chars_1; *p_2; p_2++) {
                    char c = *p_2;
                    if (c == '+' || c == '-' || c == '<' || c == '>') {
                        ++subcladecounter;
                    }
                }
            }
            if (subcladecounter < P.clade_complexity) {
                if (gc_get_n_genes(o->gc) > gc_get_n_genes(inheritGeneticCode)) {
                    o->gc->clade_id = jstr_newf("%s+%d", JSTR_JAVA(o->gc->clade_id), o->id);
                } else if (gc_get_n_genes(o->gc) < gc_get_n_genes(inheritGeneticCode)) {
                    o->gc->clade_id = jstr_newf("%s-%d", JSTR_JAVA(o->gc->clade_id), o->id);
                }
                if (gc_get_symmetry(o->gc) > gc_get_symmetry(inheritGeneticCode)) {
                    o->gc->clade_id = jstr_newf("%s>%d", JSTR_JAVA(o->gc->clade_id), o->id);
                } else if (gc_get_symmetry(o->gc) < gc_get_symmetry(inheritGeneticCode)) {
                    o->gc->clade_id = jstr_newf("%s<%d", JSTR_JAVA(o->gc->clade_id), o->id);
                }
            }
        }
    } else {
        if ((parent->saved_gc == NULL) && (sporeremember >= 0)) {
            parent->saved_gc = o->gc;
        }
    }
    return ok;
}

/* Initializes variables for a new organism born from an existing
 * transformed organism. Generates a mutated genetic code based on the transformers's one
 * and finds a place in the world to put it.
 * 
 * @param victim  The organism from which this organism is transformed.
 * @return  true if it found a place for this organism or false otherwise.
 */
bool org_inherit_transformation(Organism *o, Organism *victim, bool first)
{
    GeneticCode *inheritGeneticCode = NULL;
    bool ok = true;
    // Create the transformed genetic code
    inheritGeneticCode = victim->infected_gc;
    o->gc = gc_new_mutated(inheritGeneticCode);
    // Take a reference to the victim
    o->parent_id = org_get_id(victim);
    o->growth_ratio = 16;
    // Initial energy is total energy of the victim
    o->energy = victim->energy;
    if (first) {
        // Initialize
        org_create(o);
        org_symmetric(o);
        // Put it in the world, near its parent
        ok = org_transform_organism(o, victim);
        if (ok) {
            // Maximum age that an organism can reach
            o->max_age = P.max_age + (o->segments / P.age_divisor);
            // Calculates the energy required to reproduce this genetic code.
            o->reproduce_energy = (40 + 3 * o->segments);
            // Maximum time that needs to elapse after a reproduction attempt
            o->time_to_reproduce_max = 8;
            int additionalcentersegments = 0;
            for (int j = 0; j < gc_get_n_genes(o->gc); j++) {
                if ((gene_get_branch(gc_get_gene(o->gc, j)) != 0) || (j == 0)) {
                    o->time_to_reproduce_max += 6;
                } else {
                    additionalcentersegments += gc_get_symmetry(o->gc);
                    if ((gc_get_symmetry(o->gc) + additionalcentersegments) > 8) {
                        o->time_to_reproduce_max += 9;
                    } else {
                        o->time_to_reproduce_max += 6;
                    }
                }
            }
            // Initialize effects
            org_segments_create_effects(o);
            // Create spores
            if (o->sporetime > 0) {
                if (o->sporeversion == 1) {
                    if ((org_use_energy(o, P.spore_energy_consumption)) || (org_use_energy(o, o->energy))) {
                        for (int i = 0; i < o->segments; i++) {
                            o->seg_color[i] = CM_FRUIT;
                            o->mphoto[i] = -2;
                        }
                    }
                    o->usepretoucheffects = false;
                    o->usefriendeffects = 0;
                    o->canreact = false;
                    o->active = false;
                    o->has_moved = true;
                    o->isenhanced = false;
                    if ((!o->isaplant) && (!o->isaconsumer) && (!o->isafungus)) {
                        o->isenhanced = true;
                    }
                    o->isaplant = false;
                    o->isaconsumer = false;
                    o->isafungus = false;
                    if (o->haseyes) {
                        o->haseyes = false;
                        org_symmetric(o);
                    }
                } else if (o->sporeversion == 2) {
                    if ((org_use_energy(o, P.spore_energy_consumption)) || (org_use_energy(o, o->energy))) {
                        for (int i = 0; i < o->segments; i++) {
                            o->seg_color[i] = CM_FRUIT;
                            o->mphoto[i] = 0;
                        }
                    }
                    o->usepretoucheffects = false;
                    o->usefriendeffects = 0;
                    o->canreact = false;
                    o->active = false;
                    o->has_moved = true;
                    o->isenhanced = false;
                    if ((o->isaplant) && (!o->isafungus) && (!o->isaconsumer)) {
                        o->isenhanced = true;
                        o->max_age = o->sporetime;
                        if (o->max_age > 20) {
                            o->max_age = 20;
                        }
                    } else {
                        o->max_age = o->sporetime - 1;
                        if (o->max_age > 19) {
                            o->max_age = 19;
                        }
                    }
                    o->isaplant = false;
                    o->isaconsumer = false;
                    o->isafungus = false;
                    if (o->haseyes) {
                        o->haseyes = false;
                        org_symmetric(o);
                    }
                } else if (o->sporeversion == 3) {
                    if (o->isaplant) {
                        if ((o->isaconsumer) || (o->isafungus)) {
                            o->isenhanced = false;
                            for (int i = 0; i < o->segments; i++) {
                                o->seg_color[i] = CM_BARK;
                                o->mphoto[i] = 0;
                            }
                        } else {
                            o->isenhanced = true;
                            for (int i = 0; i < o->segments; i++) {
                                o->seg_color[i] = CM_BARK;
                                o->mphoto[i] = 0;
                            }
                        }
                    } else {
                        if ((o->isaconsumer) || (o->isafungus)) {
                            o->isenhanced = false;
                            for (int i = 0; i < o->segments; i++) {
                                o->seg_color[i] = CM_VISION;
                                o->mphoto[i] = 0;
                            }
                        } else {
                            o->isenhanced = true;
                            for (int i = 0; i < o->segments; i++) {
                                o->seg_color[i] = CM_BARK;
                                o->mphoto[i] = 0;
                            }
                        }
                    }
                    o->time_to_reproduce = 104 - o->sporetime;
                    o->usepretoucheffects = false;
                    o->usefriendeffects = 0;
                    o->canreact = false;
                    o->active = false;
                    o->has_moved = true;
                    o->isaplant = false;
                    o->isaconsumer = false;
                    o->isafungus = false;
                    if (o->haseyes) {
                        o->haseyes = false;
                        org_symmetric(o);
                    }
                } else if (o->sporeversion == 4) {
                    for (int i = 0; i < o->segments; i++) {
                        o->seg_color[i] = CM_BROWN;
                        o->mphoto[i] = 0;
                    }
                    if ((o->isaconsumer) || (o->isafungus)) {
                        o->time_to_reproduce = 115 - o->sporetime + jr_next_int(&g_rng, 2);
                        o->isenhanced = false;
                    } else {
                        o->time_to_reproduce = 104 - o->sporetime;
                        o->isenhanced = true;
                    }
                    o->usepretoucheffects = false;
                    o->usefriendeffects = 0;
                    o->canreact = false;
                    o->active = false;
                    o->has_moved = true;
                    o->isaplant = false;
                    o->isaconsumer = false;
                    o->isafungus = false;
                    if (o->haseyes) {
                        o->haseyes = false;
                        org_symmetric(o);
                    }
                } else if (o->sporeversion == 5) {
                    for (int i = 0; i < o->segments; i++) {
                        o->seg_color[i] = CM_CREAM;
                        o->mphoto[i] = -1;
                    }
                    o->usepretoucheffects = false;
                    o->usefriendeffects = 0;
                    o->canreact = false;
                    o->active = false;
                    o->has_moved = true;
                    o->isaplant = false;
                    o->isaconsumer = true;
                    o->isafungus = false;
                    o->isenhanced = false;
                    if (o->haseyes) {
                        o->haseyes = false;
                        org_symmetric(o);
                    }
                } else if (o->sporeversion == 6) {
                    bool largeenough = false;
                    o->usepretoucheffects = false;
                    if (o->isaplant) {
                        if ((o->isaconsumer) || (o->isafungus)) {
                            if (o->methanotrophy > 0) {
                                for (int i = 0; i < o->segments; i++) {
                                    o->seg_color[i] = CM_PURPLE;
                                    o->mphoto[i] = 0;
                                    if (o->m[i] >= 1) {
                                        largeenough = true;
                                    }
                                }
                                if (largeenough) {
                                    o->methanotrophy = (60 * o->mass) / P.green_obtained_energy_divisor;
                                }
                            } else {
                                if (o->canmove) {
                                    if (o->jadefactor > 0) {
                                        o->jadefactor = 1;
                                    }
                                    o->islime = false;
                                    for (int i = 0; i < o->segments; i++) {
                                        o->seg_color[i] = CM_C4;
                                        o->mphoto[i] = 0;
                                        if (o->m[i] >= 1) {
                                            largeenough = true;
                                        }
                                    }
                                    if (largeenough) {
                                        o->photosynthesis = (7.5 * o->mass) / P.green_obtained_energy_divisor;
                                    }
                                } else {
                                    if (o->jadefactor > 0) {
                                        o->jadefactor = 1;
                                    }
                                    o->islime = false;
                                    for (int i = 0; i < o->segments; i++) {
                                        o->seg_color[i] = CM_LEAF;
                                        o->mphoto[i] = 0;
                                        if (o->m[i] >= 1) {
                                            largeenough = true;
                                        }
                                    }
                                    if (largeenough) {
                                        o->photosynthesis = (8 * o->mass) / P.green_obtained_energy_divisor;
                                    }
                                }
                            }
                        } else {
                            if (o->methanotrophy > 0) {
                                for (int i = 0; i < o->segments; i++) {
                                    o->seg_color[i] = CM_PURPLE;
                                    o->mphoto[i] = 0;
                                    if (o->m[i] >= 1) {
                                        largeenough = true;
                                    }
                                }
                                if (largeenough) {
                                    o->methanotrophy = (120 * o->mass) / P.green_obtained_energy_divisor;
                                }
                            } else {
                                if (o->segmentpretoucheffects == 2) {
                                    o->islime = false;
                                    for (int i = 0; i < o->segments; i++) {
                                        o->seg_color[i] = CM_WINTER;
                                        o->mphoto[i] = 0;
                                        if (o->m[i] >= 1) {
                                            largeenough = true;
                                        }
                                    }
                                    if (largeenough) {
                                        o->photosynthesis = (12 * o->mass) / P.green_obtained_energy_divisor;
                                    }
                                } else {
                                    if ((o->isakiller) || (o->fallowversion > 0)) {
                                        o->islime = false;
                                        for (int i = 0; i < o->segments; i++) {
                                            o->seg_color[i] = CM_SUMMER;
                                            o->mphoto[i] = 0;
                                            if (o->m[i] >= 1) {
                                                largeenough = true;
                                            }
                                        }
                                        if (largeenough) {
                                            o->photosynthesis = (15 * o->mass) / P.green_obtained_energy_divisor;
                                        }
                                    } else {
                                        if (o->islime) {
                                            for (int i = 0; i < o->segments; i++) {
                                                o->seg_color[i] = CM_LIME;
                                                o->mphoto[i] = 0;
                                                if (o->m[i] >= 1) {
                                                    largeenough = true;
                                                }
                                            }
                                            if (largeenough) {
                                                o->photosynthesis = (15 * o->mass) / P.green_obtained_energy_divisor;
                                                o->leafphoto = (20 * o->mass) / P.green_obtained_energy_divisor;
                                            }
                                        } else {
                                            if (o->forestphoto > 0) {
                                                for (int i = 0; i < o->segments; i++) {
                                                    o->seg_color[i] = CM_FOREST;
                                                    o->mphoto[i] = 1;
                                                    if (o->m[i] >= 1) {
                                                        largeenough = true;
                                                    }
                                                }
                                                if (largeenough) {
                                                    o->photosynthesis = (18 * o->mass) / P.green_obtained_energy_divisor;
                                                    o->forestphoto = o->photosynthesis;
                                                    o->usepretoucheffects = true;
                                                }
                                            } else {
                                                if (o->isenhanced) {
                                                    for (int i = 0; i < o->segments; i++) {
                                                        o->seg_color[i] = CM_GRASS;
                                                        o->mphoto[i] = 0;
                                                        if (o->m[i] >= 1) {
                                                            largeenough = true;
                                                        }
                                                    }
                                                    if (largeenough) {
                                                        o->photosynthesis = (19 * o->mass) / P.green_obtained_energy_divisor;
                                                    }
                                                } else {
                                                    if ((o->jadefactor > 1) && (!o->isblond)) {
                                                        for (int i = 0; i < o->segments; i++) {
                                                            o->seg_color[i] = CM_JADE;
                                                            o->mphoto[i] = 0;
                                                            if (o->m[i] >= 1) {
                                                                largeenough = true;
                                                            }
                                                        }
                                                        if (largeenough) {
                                                            o->photosynthesis = (19 * o->mass) / P.green_obtained_energy_divisor;
                                                        }
                                                    } else {
                                                        for (int i = 0; i < o->segments; i++) {
                                                            o->seg_color[i] = JC_GREEN;
                                                            o->mphoto[i] = 0;
                                                            if (o->m[i] >= 1) {
                                                                largeenough = true;
                                                            }
                                                        }
                                                        if (largeenough) {
                                                            o->photosynthesis = (19 * o->mass) / P.green_obtained_energy_divisor;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if ((o->isaconsumer) || (o->isafungus)) {
                            o->isaplant = true;
                            for (int i = 0; i < o->segments; i++) {
                                o->seg_color[i] = CM_SPRING;
                                o->mphoto[i] = 0;
                                if (o->m[i] >= 1) {
                                    largeenough = true;
                                }
                            }
                            if (largeenough) {
                                o->photosynthesis = (7 * o->mass) / P.green_obtained_energy_divisor;
                            }
                        } else {
                            o->isaplant = true;
                            o->gold = -1;
                            for (int i = 0; i < o->segments; i++) {
                                o->seg_color[i] = CM_PURPLE;
                                o->mphoto[i] = 0;
                                if (o->m[i] >= 1) {
                                    largeenough = true;
                                }
                            }
                            if (largeenough) {
                                o->methanotrophy = (22 * o->mass) / P.green_obtained_energy_divisor;
                            }
                        }
                    }
                    if (gc_get_symmetry(o->gc) == 3) {
                        if (gc_get_mirror(o->gc) == 0) {
                            o->photosynthesis = (5.0 / 6.0) * o->photosynthesis;
                            o->methanotrophy = (5.0 / 6.0) * o->methanotrophy;
                        } else {
                            o->photosynthesis = (2.0 / 3.0) * o->photosynthesis;
                            o->methanotrophy = (2.0 / 3.0) * o->methanotrophy;
                        }
                    }
                    if (o->gold != 0) {
                        o->max_age = P.max_age + (o->segments / P.age_divisor);
                    }
                    if ((o->isblond) && (o->jadefactor > 0)) {
                        o->jadefactor = 1;
                    }
                    o->usefriendeffects = 0;
                    o->canreact = false;
                    o->candodge = false;
                    o->active = false;
                    o->has_moved = true;
                    o->isreproductive = false;
                    if (o->haseyes) {
                        o->haseyes = false;
                        org_symmetric(o);
                    }
                } else if (gc_get_modifiesspore(o->gc) <= 9) {
                    if (gc_get_adaptspore(o->gc) <= 20) {
                        org_blue_spore(o);
                    } else if (gc_get_adaptspore(o->gc) <= 30) {
                        if (jr_next_boolean(&g_rng)) {
                            org_blue_spore(o);
                        }
                    } else if (gc_get_adaptspore(o->gc) <= 40) {
                        if (jr_next_int(&g_rng, 4) == 0) {
                            org_blue_spore(o);
                        }
                    } else if (gc_get_adaptspore(o->gc) <= 50) {
                        if (jr_next_int(&g_rng, 8) == 0) {
                            org_blue_spore(o);
                        }
                    } else if (gc_get_adaptspore(o->gc) <= 60) {
                        if (jr_next_int(&g_rng, 16) == 0) {
                            org_blue_spore(o);
                        }
                    }
                } else if (gc_get_modifiesspore(o->gc) <= 12) {
                    if (gc_get_adaptspore(o->gc) <= 20) {
                        org_cyan_spore(o);
                    } else if (gc_get_adaptspore(o->gc) <= 30) {
                        if (jr_next_boolean(&g_rng)) {
                            org_cyan_spore(o);
                        }
                    } else if (gc_get_adaptspore(o->gc) <= 40) {
                        if (jr_next_int(&g_rng, 4) == 0) {
                            org_cyan_spore(o);
                        }
                    } else if (gc_get_adaptspore(o->gc) <= 50) {
                        if (jr_next_int(&g_rng, 8) == 0) {
                            org_cyan_spore(o);
                        }
                    } else if (gc_get_adaptspore(o->gc) <= 60) {
                        if (jr_next_int(&g_rng, 16) == 0) {
                            org_cyan_spore(o);
                        }
                    }
                }
            }
            // Create subclades
            if (gc_get_update_clade(o->gc)) {
                o->gc->update_clade = false;
                int subcladecounter = 0;
                {
                    char chars_3[JSTR_LOOP_MAX];
                    jstr_copy_to(chars_3, sizeof chars_3, o->gc->clade_id);
                    for (const char *p_4 = chars_3; *p_4; p_4++) {
                        char c = *p_4;
                        if (c == '+' || c == '-' || c == '<' || c == '>') {
                            ++subcladecounter;
                        }
                    }
                }
                if (subcladecounter < P.clade_complexity) {
                    if (gc_get_n_genes(o->gc) > gc_get_n_genes(inheritGeneticCode)) {
                        o->gc->clade_id = jstr_newf("%s+%d", JSTR_JAVA(o->gc->clade_id), o->id);
                    } else if (gc_get_n_genes(o->gc) < gc_get_n_genes(inheritGeneticCode)) {
                        o->gc->clade_id = jstr_newf("%s-%d", JSTR_JAVA(o->gc->clade_id), o->id);
                    }
                    if (gc_get_symmetry(o->gc) > gc_get_symmetry(inheritGeneticCode)) {
                        o->gc->clade_id = jstr_newf("%s>%d", JSTR_JAVA(o->gc->clade_id), o->id);
                    } else if (gc_get_symmetry(o->gc) < gc_get_symmetry(inheritGeneticCode)) {
                        o->gc->clade_id = jstr_newf("%s<%d", JSTR_JAVA(o->gc->clade_id), o->id);
                    }
                }
            }
        }
    } else
        ok = false;
    return ok;
}

/* Places the organism at the specified position in the world and initializes its
 * variables. The organism must has an assigned genetic code.
 * 
 * @param posx  The x coordinate of the position in the world we want to put this organism.
 * @param posy  The y coordinate of the position in the world we want to put this organism.
 * @return  true if there were enough space to put the organism, false otherwise.
 */
bool org_paste_organism(Organism *o, int posx, int posy)
{
    bool isacheater = false;
    o->parent_id = -1;
    if (gc_get_generation(o->gc) == 0) {
        o->gc->generation = 1;
    }
    o->growth_ratio = 16;
    org_create(o);
    org_symmetric(o);
    for (int i = 0; i < gc_get_n_genes(o->gc); i++) {
        if (gene_get_branch(gc_get_gene(o->gc, i)) > i) {
            isacheater = true;
        }
    }
    if (isacheater == false) {
        o->dcenter_x = o->center_x = posx;
        o->dcenter_y = o->center_y = posy;
        org_calculate_bounds(o, true);
        // Check that the position is inside the world
        if (org_is_inside_world(o)) {
            // Check that the organism will not overlap other organisms
            if (world_genesis_check_hit(o->world, o) == NULL) {
                // Generem identificador
                o->id = world_get_new_id(o->world);
                if (gc_getclade_id(o->gc) == NULL) {
                    o->gc->clade_id = jstr_newf("%d", o->id);
                }
                o->energy = jmin(P.initial_energy, world_get_co2(o->world));
                world_decrease_co2(o->world, o->energy);
                world_add_o2(o->world, o->energy);
                // Maximum age that an organism can reach
                o->max_age = P.max_age + (o->segments / P.age_divisor);
                // Calculates the energy required to reproduce this genetic code.
                o->reproduce_energy = (40 + 3 * o->segments);
                // Maximum time that needs to elapse after a reproduction attempt
                o->time_to_reproduce_max = 8;
                int additionalcentersegments = 0;
                for (int j = 0; j < gc_get_n_genes(o->gc); j++) {
                    if ((gene_get_branch(gc_get_gene(o->gc, j)) != 0) || (j == 0)) {
                        o->time_to_reproduce_max += 6;
                    } else {
                        additionalcentersegments += gc_get_symmetry(o->gc);
                        if ((gc_get_symmetry(o->gc) + additionalcentersegments) > 8) {
                            o->time_to_reproduce_max += 9;
                        } else {
                            o->time_to_reproduce_max += 6;
                        }
                    }
                }
                // Initialize effects
                org_segments_create_effects(o);
                return true;
            }
        }
    }
    // It can't be placed		
    return false;
}

/* Translates the genetic code of this organism to its segments representation in the world.
 * Also, calculates some useful information like segments length, inertia, etc.
 * This method must be called when an organism is firstly displayed on the world and every
 * time it changes its size.
 * inherit, randomCreate and pasteOrganism are the standard ways to add an organism to a world
 * and they already call this method.
 */
void org_symmetric(Organism *o)
{
    int i = 0;
    int j = 0;
    int segment = 0;
    int symmetry = gc_get_symmetry(o->gc);
    int mirror = gc_get_mirror(o->gc);
    int sequence = o->segments / symmetry;
    int left = 0;
    int right = 0;
    int top = 0;
    int bottom = 0;
    int centerX = 0;
    int centerY = 0;
    double cx = 0;
    double cy = 0;
    if (o->update_effects == 0) {
        o->update_effects = 1;
    }
    o->mass = 0;
    if (o->haseyes) {
        for (i = 0; i < symmetry; i++) {
            for (j = 0; j < sequence; j++, segment++) {
                // Here, we take the vector that forms the segment, scale it depending on
                // the relative size of the organism and rotate it depending on the
                // symmetry and mirroring.
                if (o->seg_color[j] == CM_EYE) {
                    if (o->hasgoodvision) {
                        org_v.modulus = 2 * (gene_get_length(gc_get_gene(o->gc, j)) * k_scale[o->growth_ratio - 1]);
                    } else {
                        org_v.modulus = 1.5 * (gene_get_length(gc_get_gene(o->gc, j)) * k_scale[o->growth_ratio - 1]);
                    }
                    if ((o->isaconsumer) || (o->isafungus) || (o->isinfectious)) {
                        o->m[segment] = 0.026 * org_v.modulus;
                    } else {
                        o->m[segment] = 0.025 * org_v.modulus;
                    }
                } else {
                    org_v.modulus = gene_get_length(gc_get_gene(o->gc, j)) * k_scale[o->growth_ratio - 1];
                    o->m[segment] = org_v.modulus;
                }
                o->mass += o->m[segment];
                if (j == 0) {
                    o->start_x[segment] = 0;
                    o->start_y[segment] = 0;
                    if (mirror == 0 || i % 2 == 0)
                        org_v.theta = gene_get_theta(gc_get_gene(o->gc, j)) + i * 2 * M_PI / symmetry;
                    else {
                        org_v.theta = gene_get_theta(gc_get_gene(o->gc, j)) + (i - 1) * 2 * M_PI / symmetry;
                        vec_invert_x(&org_v);
                    }
                } else {
                    if (gene_get_branch(gc_get_gene(o->gc, j)) == -1) {
                        o->start_x[segment] = o->end_x[segment - 1];
                        o->start_y[segment] = o->end_y[segment - 1];
                        if (o->seg_color[segment - 1] == CM_EYE) {
                            o->seg_color[segment - 1] = CM_VISION;
                            o->hasgoodvision = true;
                        }
                        if (mirror == 0 || i % 2 == 0)
                            org_v.theta += gene_get_theta(gc_get_gene(o->gc, j));
                        else
                            org_v.theta += -gene_get_theta(gc_get_gene(o->gc, j));
                    } else {
                        if (gene_get_branch(gc_get_gene(o->gc, j)) == 0) {
                            o->start_x[segment] = 0;
                            o->start_y[segment] = 0;
                            if (mirror == 0 || i % 2 == 0)
                                org_v.theta = gene_get_theta(gc_get_gene(o->gc, j)) + i * 2 * M_PI / symmetry;
                            else {
                                org_v.theta = gene_get_theta(gc_get_gene(o->gc, j)) + (i - 1) * 2 * M_PI / symmetry;
                                vec_invert_x(&org_v);
                            }
                        } else {
                            o->start_x[segment] = o->end_x[(i * sequence) + gene_get_branch(gc_get_gene(o->gc, j)) - 1];
                            o->start_y[segment] = o->end_y[(i * sequence) + gene_get_branch(gc_get_gene(o->gc, j)) - 1];
                            if (o->seg_color[(i * sequence) + gene_get_branch(gc_get_gene(o->gc, j)) - 1] == CM_EYE) {
                                o->seg_color[(i * sequence) + gene_get_branch(gc_get_gene(o->gc, j)) - 1] = CM_VISION;
                                o->hasgoodvision = true;
                            }
                            if (mirror == 0 || i % 2 == 0)
                                org_v.theta += gene_get_theta(gc_get_gene(o->gc, j));
                            else
                                org_v.theta += -gene_get_theta(gc_get_gene(o->gc, j));
                        }
                    }
                }
                // Apply the vector to the starting point to get the ending point.
                o->end_x[segment] = (int)j_lround(vec_get_x(&org_v) + o->start_x[segment]);
                o->end_y[segment] = (int)j_lround(vec_get_y(&org_v) + o->start_y[segment]);
                // Calculate the bounding rectangle of this organism
                left = j_imin(left, o->end_x[segment]);
                right = j_imax(right, o->end_x[segment]);
                top = j_imin(top, o->end_y[segment]);
                bottom = j_imax(bottom, o->end_y[segment]);
            }
        }
    } else {
        for (i = 0; i < symmetry; i++) {
            for (j = 0; j < sequence; j++, segment++) {
                // Here, we take the vector that forms the segment, scale it depending on
                // the relative size of the organism and rotate it depending on the
                // symmetry and mirroring.
                org_v.modulus = gene_get_length(gc_get_gene(o->gc, j)) * k_scale[o->growth_ratio - 1];
                o->m[segment] = org_v.modulus;
                o->mass += o->m[segment];
                if (j == 0) {
                    o->start_x[segment] = 0;
                    o->start_y[segment] = 0;
                    if (mirror == 0 || i % 2 == 0)
                        org_v.theta = gene_get_theta(gc_get_gene(o->gc, j)) + i * 2 * M_PI / symmetry;
                    else {
                        org_v.theta = gene_get_theta(gc_get_gene(o->gc, j)) + (i - 1) * 2 * M_PI / symmetry;
                        vec_invert_x(&org_v);
                    }
                } else {
                    if (gene_get_branch(gc_get_gene(o->gc, j)) == -1) {
                        o->start_x[segment] = o->end_x[segment - 1];
                        o->start_y[segment] = o->end_y[segment - 1];
                        if (mirror == 0 || i % 2 == 0)
                            org_v.theta += gene_get_theta(gc_get_gene(o->gc, j));
                        else
                            org_v.theta += -gene_get_theta(gc_get_gene(o->gc, j));
                    } else {
                        if (gene_get_branch(gc_get_gene(o->gc, j)) == 0) {
                            o->start_x[segment] = 0;
                            o->start_y[segment] = 0;
                            if (mirror == 0 || i % 2 == 0)
                                org_v.theta = gene_get_theta(gc_get_gene(o->gc, j)) + i * 2 * M_PI / symmetry;
                            else {
                                org_v.theta = gene_get_theta(gc_get_gene(o->gc, j)) + (i - 1) * 2 * M_PI / symmetry;
                                vec_invert_x(&org_v);
                            }
                        } else {
                            o->start_x[segment] = o->end_x[(i * sequence) + gene_get_branch(gc_get_gene(o->gc, j)) - 1];
                            o->start_y[segment] = o->end_y[(i * sequence) + gene_get_branch(gc_get_gene(o->gc, j)) - 1];
                            if (mirror == 0 || i % 2 == 0)
                                org_v.theta += gene_get_theta(gc_get_gene(o->gc, j));
                            else
                                org_v.theta += -gene_get_theta(gc_get_gene(o->gc, j));
                        }
                    }
                }
                // Apply the vector to the starting point to get the ending point.
                o->end_x[segment] = (int)j_lround(vec_get_x(&org_v) + o->start_x[segment]);
                o->end_y[segment] = (int)j_lround(vec_get_y(&org_v) + o->start_y[segment]);
                // Calculate the bounding rectangle of this organism
                left = j_imin(left, o->end_x[segment]);
                right = j_imax(right, o->end_x[segment]);
                top = j_imin(top, o->end_y[segment]);
                bottom = j_imax(bottom, o->end_y[segment]);
            }
        }
    }
    jrect_set(&o->size_rect, left, top, right - left + 1, bottom - top + 1);
    // image center
    centerX = (left + right) >> 1;
    centerY = (top + bottom) >> 1;
    o->mass = 1000 * o->mass;
    o->mass = 0.001 * j_lround(o->mass);
    o->inertia = 0;
    for (i = 0; i < o->segments; i++) {
        // express points relative to the image center
        o->start_x[i] -= centerX;
        o->start_y[i] -= centerY;
        o->end_x[i] -= centerX;
        o->end_y[i] -= centerY;
        // calculate points distance of the origin and modulus
        o->m1[i] = sqrt(o->start_x[i] * o->start_x[i] + o->start_y[i] * o->start_y[i]);
        o->m2[i] = sqrt(o->end_x[i] * o->end_x[i] + o->end_y[i] * o->end_y[i]);
        // calculate inertia moment
        // the mass center of a segment is its middle point
        cx = (o->start_x[i] + o->end_x[i]) / 2.0;
        cy = (o->start_y[i] + o->end_y[i]) / 2.0;
        // add the effect of this segment, following the parallel axis theorem
        o->inertia += bio_pow(o->m[i], 3) / 12.0 + o->m[i] * cx * cx + cy * cy;
        // mass * length^2 (center is at 0,0)
    }
}

/* Tries to find a spare place in the world for this organism and place it.
 * It also generates an identification number for the organism if it can be placed
 * somewhere.
 * 
 * @return  true if a suitable place has been found, false if not.
 */
static bool org_place_random(Organism *o)
{
    /* We try to place the organism in 12 different positions. If all of them
     * are occupied, we return false.
     */
    for (int i = 12; i > 0; i--) {
        /* Get a random point for the top left corner of the organism
         * making sure it is inside the world.
         */
        int arg_5 = jr_next_int(&g_rng, world_get_width(o->world) - o->size_rect.width);
        int arg_6 = jr_next_int(&g_rng, world_get_height(o->world) - o->size_rect.height);
        JIPoint origin = (JIPoint){arg_5, arg_6};
        jrect_set(&o->r, origin.x, origin.y, o->size_rect.width, o->size_rect.height);
        o->dcenter_x = o->center_x = origin.x + (o->size_rect.width >> 1);
        o->dcenter_y = o->center_y = origin.y + (o->size_rect.height >> 1);
        // Check that the position is not occupied.
        if (world_genesis_check_hit(o->world, o) == NULL) {
            // Generate an identification
            o->id = world_get_new_id(o->world);
            o->gc->clade_id = jstr_newf("%d", o->id);
            // Maximum age that an organism can reach
            o->max_age = P.max_age + (o->segments / P.age_divisor);
            // Calculates the energy required to reproduce this genetic code.
            o->reproduce_energy = (40 + 3 * o->segments);
            // Maximum time that needs to elapse after a reproduction attempt
            o->time_to_reproduce_max = 8;
            int additionalcentersegments = 0;
            for (int j = 0; j < gc_get_n_genes(o->gc); j++) {
                if ((gene_get_branch(gc_get_gene(o->gc, j)) != 0) || (j == 0)) {
                    o->time_to_reproduce_max += 6;
                } else {
                    additionalcentersegments += gc_get_symmetry(o->gc);
                    if ((gc_get_symmetry(o->gc) + additionalcentersegments) > 8) {
                        o->time_to_reproduce_max += 9;
                    } else {
                        o->time_to_reproduce_max += 6;
                    }
                }
            }
            // Initialize effects
            org_segments_create_effects(o);
            return true;
        }
    }
    // If we get here, we haven't find a place for this organism.
    return false;
}

/* Version for the second to last child of an organism.
 * Tries to find a spare place near its parent for this organism and place it.
 * It also generates an identification number for the organism if it can be placed
 * somewhere and substracts its energy from its parent's energy.
 * 
 * @return  true if a suitable place has been found, false if not.
 */
static bool org_place_near_yellow(Organism *o, Organism *parent)
{
    int nPos = jr_next_int(&g_rng, 8);
    char cPos = (char)(nPos + '0');
    // Try to put it in any possible position, starting from a randomly chosen one.
    for (int nSide = 0; nSide < 8; nSide++) {
        // Calculate candidate position
        o->dcenter_x = parent->dcenter_x + (parent->r.width / 2 + o->r.width / 2 + 1) * k_side[nPos][0];
        o->dcenter_y = parent->dcenter_y + (parent->r.height / 2 + o->r.height / 2 + 1) * k_side[nPos][1];
        o->center_x = j_d2i(o->dcenter_x);
        o->center_y = j_d2i(o->dcenter_y);
        org_calculate_bounds(o, true);
        // Check this position is inside the world.
        if (org_is_inside_world(o)) {
            // Check that it doesn't overlap with other organisms.
            if (world_fast_check_hit(o->world, o) == NULL) {
                {
                    char chars_7[JSTR_LOOP_MAX];
                    jstr_copy_to(chars_7, sizeof chars_7, parent->n_position);
                    for (const char *p_8 = chars_7; *p_8; p_8++) {
                        char c = *p_8;
                        if (c == cPos) {
                            if (gc_get_disperse_children(parent->gc)) {
                                o->dx = k_side[nPos][0];
                                o->dy = k_side[nPos][1];
                            } else {
                                o->dx = parent->dx;
                                o->dy = parent->dy;
                            }
                            // Generate an identification
                            o->id = world_get_new_id(o->world);
                            // Substract the energy from the parent
                            parent->energy -= o->energy;
                            parent->n_position = jstr_replace(parent->n_position, cPos, 'X');
                            return true;
                        }
                    }
                }
            }
        }
        nPos = (nPos + 1) % 8;
        cPos = (char)(nPos + '0');
    }
    // It can't be placed.
    return false;
}

/* Version for the first of more children of an organism.
 * Tries to find a spare place near its parent for this organism and place it.
 * It also generates an identification number for the organism if it can be placed
 * somewhere and substracts its energy from its parent's energy.
 * 
 * @return  true if a suitable place has been found, false if not.
 */
static bool org_place_near_first(Organism *o, Organism *parent)
{
    int nPos = jr_next_int(&g_rng, 8);
    // Try to put it in any possible position, starting from a randomly chosen one.
    for (int nSide = 0; nSide < 8; nSide++) {
        // Calculate candidate position
        o->dcenter_x = parent->dcenter_x + (parent->r.width / 2 + o->r.width / 2 + 1) * k_side[nPos][0];
        o->dcenter_y = parent->dcenter_y + (parent->r.height / 2 + o->r.height / 2 + 1) * k_side[nPos][1];
        o->center_x = j_d2i(o->dcenter_x);
        o->center_y = j_d2i(o->dcenter_y);
        org_calculate_bounds(o, true);
        // Check this position is inside the world.
        if (org_is_inside_world(o)) {
            // Check that it doesn't overlap with other organisms.
            if (world_fast_check_hit(o->world, o) == NULL) {
                if (gc_get_disperse_children(parent->gc)) {
                    o->dx = k_side[nPos][0];
                    o->dy = k_side[nPos][1];
                } else {
                    o->dx = parent->dx;
                    o->dy = parent->dy;
                }
                // Generate an identification
                o->id = world_get_new_id(o->world);
                // Substract the energy from the parent
                parent->energy -= o->energy;
                char cPos = (char)(nPos + '0');
                parent->n_position = jstr_replace(parent->n_position, cPos, 'X');
                return true;
            }
        }
        nPos = (nPos + 1) % 8;
    }
    // It can't be placed.
    return false;
}

/* Version for the only child of an organism.
 * Tries to find a spare place near its parent for this organism and place it.
 * It also generates an identification number for the organism if it can be placed
 * somewhere and substracts its energy from its parent's energy.
 * 
 * @return  true if a suitable place has been found, false if not.
 */
static bool org_place_near(Organism *o, Organism *parent)
{
    int nPos = jr_next_int(&g_rng, 8);
    // Try to put it in any possible position, starting from a randomly chosen one.
    for (int nSide = 0; nSide < 8; nSide++) {
        // Calculate candidate position
        o->dcenter_x = parent->dcenter_x + (parent->r.width / 2 + o->r.width / 2 + 1) * k_side[nPos][0];
        o->dcenter_y = parent->dcenter_y + (parent->r.height / 2 + o->r.height / 2 + 1) * k_side[nPos][1];
        o->center_x = j_d2i(o->dcenter_x);
        o->center_y = j_d2i(o->dcenter_y);
        org_calculate_bounds(o, true);
        // Check this position is inside the world.
        if (org_is_inside_world(o)) {
            // Check that it doesn't overlap with other organisms.
            if (world_fast_check_hit(o->world, o) == NULL) {
                if (gc_get_disperse_children(parent->gc)) {
                    o->dx = k_side[nPos][0];
                    o->dy = k_side[nPos][1];
                } else {
                    o->dx = parent->dx;
                    o->dy = parent->dy;
                }
                // Generate an identification
                o->id = world_get_new_id(o->world);
                // Substract the energy from the parent
                parent->energy -= o->energy;
                return true;
            }
        }
        nPos = (nPos + 1) % 8;
    }
    // It can't be placed.
    return false;
}

/* Transforms an organism after infected by coral.
 * It also generates an identification number for the organism
 * and substracts its energy from its parent's energy.
 * 
 * @return  true if a suitable place has been found, false if not.
 */
static bool org_transform_organism(Organism *o, Organism *parent)
{
    o->dcenter_x = parent->dcenter_x;
    o->dcenter_y = parent->dcenter_y;
    o->center_x = j_d2i(o->dcenter_x);
    o->center_y = j_d2i(o->dcenter_y);
    org_calculate_bounds(o, true);
    // Check this position is inside the world and check that it doesn't overlap with other organisms.
    if ((org_is_inside_world(o)) && (world_transform_check_hit(o->world, o) == NULL)) {
        o->dx = parent->dx;
        o->dy = parent->dy;
        // Generate an identification
        o->id = world_get_new_id(o->world);
        // Substract the energy from the parent
        parent->energy -= o->energy;
        return true;
    } else {
        int nPos = jr_next_int(&g_rng, 8);
        // Try to put it in any possible position, starting from a randomly chosen one.
        for (int nSide = 0; nSide < 8; nSide++) {
            // Calculate candidate position
            o->dcenter_x = parent->dcenter_x + (parent->r.width / 2 + o->r.width / 2 + 1) * k_side[nPos][0];
            o->dcenter_y = parent->dcenter_y + (parent->r.height / 2 + o->r.height / 2 + 1) * k_side[nPos][1];
            o->center_x = j_d2i(o->dcenter_x);
            o->center_y = j_d2i(o->dcenter_y);
            org_calculate_bounds(o, true);
            // Check this position is inside the world.
            if (org_is_inside_world(o)) {
                // Check that it doesn't overlap with other organisms.
                if (world_fast_check_hit(o->world, o) == NULL) {
                    o->dx = parent->dx;
                    o->dy = parent->dy;
                    // Generate an identification
                    o->id = world_get_new_id(o->world);
                    // Substract the energy from the parent
                    parent->energy -= o->energy;
                    return true;
                }
            }
            nPos = (nPos + 1) % 8;
        }
    }
    // It can't be placed.
    return false;
}

/* Makes this organism transform into its killer */
void org_transform(Organism *o)
{
    Organism *newOrg = NULL;
    newOrg = org_new(o->world);
    if (org_inherit_transformation(newOrg, o, true)) {
        // It can be created
        world_add_organism(o->world, newOrg, o);
        o->infected_gc = NULL;
        o->saved_gc = NULL;
    }
}

/* Calculates the position of all organism points in the world, depending on
 * its rotation. It also calculates the bounding rectangle of the organism.
 * This method must be called from outside this class only when doing
 * manual drawing.
 * 
 * @param force  To avoid calculations, segments position are only calculated
 * if the organism's rotation has changed in the last frame. If it is necessary
 * to calculate them even when the rotation hasn't changed, assign true to this
 * parameter.
 */
void org_calculate_bounds(Organism *o, bool force)
{
    double left = DBL_MAX;
    double right = JDOUBLE_MIN_VALUE;
    double top = DBL_MAX;
    double bottom = JDOUBLE_MIN_VALUE;
    double theta = 0;
    for (int i = o->segments - 1; i >= 0; i--) {
        /* Save calculation: if rotation hasn't changed and it is not forced,
         * don't calculate points again.
         */
        if (o->last_theta != o->theta || force) {
            theta = o->theta + bio_atan2(o->start_y[i], o->start_x[i]);
            o->x1[i] = j_d2i((o->m1[i] * bio_cos(theta)));
            o->y1[i] = j_d2i((o->m1[i] * bio_sin(theta)));
            theta = o->theta + bio_atan2(o->end_y[i], o->end_x[i]);
            o->x2[i] = j_d2i((o->m2[i] * bio_cos(theta)));
            o->y2[i] = j_d2i((o->m2[i] * bio_sin(theta)));
        }
        // Finds the rectangle that comprises the organism
        left = jmin(jmin(left, o->x1[i] + o->dcenter_x), o->x2[i] + o->dcenter_x);
        right = jmax(jmax(right, o->x1[i] + o->dcenter_x), o->x2[i] + o->dcenter_x);
        top = jmin(jmin(top, o->y1[i] + o->dcenter_y), o->y2[i] + o->dcenter_y);
        bottom = jmax(jmax(bottom, o->y1[i] + o->dcenter_y), o->y2[i] + o->dcenter_y);
    }
    jrect_set(&o->r, j_d2i(left), j_d2i(top), j_d2i((right - left + 1)) + 1, j_d2i((bottom - top + 1)) + 1);
    o->last_theta = o->theta;
}

/* If its the time for this organism to grow, calculates its new segments and speed.
 * An active organism can grow once every 8 frames until it gets its maximum size.
 */
static void org_grow(Organism *o)
{
    if ((o->growth_ratio > 1) && (o->energy >= o->mass / 10)) {
        if (((o->active) || (o->growth_ratio == 16)) && ((o->age & 0x07) == 0x07)) {
            o->growth_ratio--;
            double m = o->mass;
            double I = o->inertia;
            org_symmetric(o);
            // Cynetic energy is constant. If mass changes, speed must also change.
            m = sqrt(m / o->mass);
            o->dx *= m;
            o->dy *= m;
            o->dtheta *= sqrt(I / o->inertia);
            o->has_grown = 1;
            if (o->sporeversion < -1) {
                o->sporeversion = -1;
            }
        } else {
            o->has_grown = 0;
        }
    } else if (o->energy < o->mass / 12) {
        if (o->growth_ratio < 15) {
            o->growth_ratio++;
            double m = o->mass;
            double I = o->inertia;
            org_symmetric(o);
            // Cynetic energy is constant. If mass changes, speed must also change.
            m = sqrt(m / o->mass);
            o->dx *= m;
            o->dy *= m;
            o->dtheta *= sqrt(I / o->inertia);
            o->has_grown = -1;
        } else {
            o->has_grown = 0;
            if ((o->sporeversion == -1) && (o->active) && (((!o->isinfectious) && (!o->isblond) && (o->frames_color <= 9)) || (o->frames_color == 0))) {
                o->isfrozen = false;
                o->isinjured = false;
                o->allfrozen = false;
                if (gc_get_modifiesspore(o->gc) >= 10) {
                    org_cyan_spore(o);
                    org_set_color(o, JC_CYAN);
                } else {
                    org_blue_spore(o);
                    org_set_color(o, JC_BLUE);
                }
                if (o->infected_gc != NULL) {
                    o->infected_gc = NULL;
                    o->saved_gc = NULL;
                }
            }
        }
    } else {
        o->has_grown = 0;
    }
}

/* Makes this organism reproduce. It tries to create at least one
 * child and at maximum 8 (depending on the number of yellow segments
 * of the organism) and put them in the world.
 */
void org_reproduce(Organism *o)
{
    o->time_to_reproduce = o->time_to_reproduce_max;
    // Calculate number of children
    if (o->issilver) {
        if ((o->isaconsumer) || (o->isafungus) || (o->isinfectious)) {
            if (o->total_children < 1) {
                o->n_children = 1;
            } else if (o->total_children < 9) {
                o->n_children = 2;
            } else if (o->total_children < 36) {
                o->n_children = 3;
            } else if (o->total_children < 100) {
                o->n_children = 4;
            } else if (o->total_children < 225) {
                o->n_children = 5;
            } else if (o->total_children < 441) {
                o->n_children = 6;
            } else if (o->total_children < 784) {
                o->n_children = 7;
            } else if (o->total_children >= 784) {
                o->n_children = 8;
            }
        } else {
            if (o->total_children < 2) {
                o->n_children = 2;
            } else if (o->total_children < 5) {
                o->n_children = 3;
            } else if (o->total_children < 9) {
                o->n_children = 4;
            } else if (o->total_children < 14) {
                o->n_children = 5;
            } else if (o->total_children < 20) {
                o->n_children = 6;
            } else if (o->total_children < 27) {
                o->n_children = 7;
            } else if (o->total_children >= 27) {
                o->n_children = 8;
            }
        }
    } else {
        o->n_children = 1;
    }
    if ((o->yellow_counter > 0) && (org_use_energy(o, P.yellow_energy_consumption))) {
        o->n_children += o->yellow_counter;
    }
    if (o->isauburn && o->infected_gc != NULL && (org_use_energy(o, P.auburn_energy_consumption))) {
        o->n_children += 1;
    }
    Organism *newOrg = NULL;
    if (o->sporeversion > 0) {
        for (int i = 0; i < bio_between_i(o->n_children, 1, 8); i++) {
            newOrg = org_new(o->world);
            if (org_inherit(newOrg, o, i == 0)) {
                // It can be created
                o->total_children++;
                world_add_organism(o->world, newOrg, o);
                o->saved_gc = NULL;
            } else {
                break;
            }
        }
    } else {
        for (int i = 0; i < bio_between_i(o->n_children, 1, 8); i++) {
            newOrg = org_new(o->world);
            if (org_inherit(newOrg, o, i == 0)) {
                // It can be created
                o->total_children++;
                world_add_organism(o->world, newOrg, o);
                o->infected_gc = NULL;
                o->saved_gc = NULL;
            } else {
                break;
            }
        }
    }
}

/* Makes this organism with blond segments reproduce early. It tries
 * to create at least one child and at maximum 8 (depending on the
 * number of yellow segments of the organism) and put them in the world.
 */
void org_reproduce_early(Organism *o)
{
    o->time_to_reproduce = o->time_to_reproduce_max;
    // Calculate number of children
    if (o->issilver) {
        if ((o->isaconsumer) || (o->isafungus) || (o->isinfectious)) {
            if (o->total_children < 1) {
                o->n_children = 1;
            } else if (o->total_children < 9) {
                o->n_children = 2;
            } else if (o->total_children < 36) {
                o->n_children = 3;
            } else if (o->total_children < 100) {
                o->n_children = 4;
            } else if (o->total_children < 225) {
                o->n_children = 5;
            } else if (o->total_children < 441) {
                o->n_children = 6;
            } else if (o->total_children < 784) {
                o->n_children = 7;
            } else if (o->total_children >= 784) {
                o->n_children = 8;
            }
        } else {
            if (o->total_children < 2) {
                o->n_children = 2;
            } else if (o->total_children < 5) {
                o->n_children = 3;
            } else if (o->total_children < 9) {
                o->n_children = 4;
            } else if (o->total_children < 14) {
                o->n_children = 5;
            } else if (o->total_children < 20) {
                o->n_children = 6;
            } else if (o->total_children < 27) {
                o->n_children = 7;
            } else if (o->total_children >= 27) {
                o->n_children = 8;
            }
        }
    } else {
        o->n_children = 1;
    }
    if ((o->yellow_counter > 0) && (org_use_energy(o, P.yellow_energy_consumption))) {
        o->n_children += o->yellow_counter;
    }
    Organism *newOrg = NULL;
    bool useblondcosts = true;
    for (int i = 0; i < bio_between_i(o->n_children, 1, 8); i++) {
        newOrg = org_new(o->world);
        if (org_inherit(newOrg, o, i == 0)) {
            // It can be created
            o->total_children++;
            world_add_organism(o->world, newOrg, o);
            if (o->isblond) {
                if (useblondcosts) {
                    useblondcosts = false;
                    org_use_energy(o, jmin(o->energy, P.blond_energy_consumption));
                }
            }
            o->saved_gc = NULL;
        } else {
            break;
        }
    }
}

/* Makes this plant organism with blond segments reproduce early. It tries
 * to create at least one child and at maximum 8 (depending on the
 * number of yellow segments of the organism) and put them in the world.
 */
void org_reproduce_early_plant(Organism *o)
{
    o->time_to_reproduce = o->time_to_reproduce_max;
    // Calculate number of children
    if (o->issilver) {
        if ((o->isaconsumer) || (o->isafungus) || (o->isinfectious)) {
            if (o->total_children < 1) {
                o->n_children = 1;
            } else if (o->total_children < 9) {
                o->n_children = 2;
            } else if (o->total_children < 36) {
                o->n_children = 3;
            } else if (o->total_children < 100) {
                o->n_children = 4;
            } else if (o->total_children < 225) {
                o->n_children = 5;
            } else if (o->total_children < 441) {
                o->n_children = 6;
            } else if (o->total_children < 784) {
                o->n_children = 7;
            } else if (o->total_children >= 784) {
                o->n_children = 8;
            }
        } else {
            if (o->total_children < 2) {
                o->n_children = 2;
            } else if (o->total_children < 5) {
                o->n_children = 3;
            } else if (o->total_children < 9) {
                o->n_children = 4;
            } else if (o->total_children < 14) {
                o->n_children = 5;
            } else if (o->total_children < 20) {
                o->n_children = 6;
            } else if (o->total_children < 27) {
                o->n_children = 7;
            } else if (o->total_children >= 27) {
                o->n_children = 8;
            }
        }
    } else {
        o->n_children = 1;
    }
    if ((o->yellow_counter > 0) && (org_use_energy(o, P.yellow_energy_consumption))) {
        o->n_children += o->yellow_counter;
    }
    Organism *newOrg = NULL;
    bool useblondcosts = true;
    for (int i = 0; i < bio_between_i(o->n_children, 1, 8); i++) {
        newOrg = org_new(o->world);
        if (org_inherit(newOrg, o, i == 0)) {
            // It can be created
            o->total_children++;
            world_add_organism(o->world, newOrg, o);
            if (o->isblond) {
                if (useblondcosts) {
                    useblondcosts = false;
                    if ((o->isonlyc4) || (o->reproducelate > 0)) {
                        org_use_energy(o, jmin(o->energy, P.blond_energy_consumption));
                    } else {
                        org_use_energy(o, jmin(o->energy, P.blond_energy_consumption * (o->reproduce_energy - o->early_reproduce_energy)));
                    }
                }
            }
            o->saved_gc = NULL;
        } else {
            break;
        }
    }
}

/* Makes this virus with blond segments reproduce. It tries
 * to create one child and put it in the world.
 */
void org_reproduce_early_virus(Organism *o)
{
    o->time_to_reproduce = o->time_to_reproduce_max;
    o->n_children = 1;
    Organism *newOrg = NULL;
    newOrg = org_new(o->world);
    if (org_inherit(newOrg, o, true)) {
        // It can be created
        o->total_children++;
        world_add_organism(o->world, newOrg, o);
        if (o->isblond) {
            org_use_energy(o, jmin(o->energy, P.blond_energy_consumption));
        }
        if (o->sporeversion <= 4) {
            o->time_to_reproduce = 0;
            o->saved_gc = newOrg->gc;
        } else {
            o->saved_gc = NULL;
        }
    }
}

/* Makes this organism with blue or cyan spores reproduce at death.
 * It tries to create one child as a pore and put it in the world.
 */
void org_reproduce_spore(Organism *o)
{
    o->time_to_reproduce = o->time_to_reproduce_max;
    o->n_children = 1;
    Organism *newOrg = NULL;
    newOrg = org_new(o->world);
    if (o->infected_gc != NULL) {
        o->infected_gc = NULL;
        o->saved_gc = NULL;
    }
    if (org_inherit(newOrg, o, true)) {
        // It can be created
        o->total_children++;
        world_add_organism(o->world, newOrg, o);
        o->saved_gc = NULL;
        if ((newOrg->sporeversion < 0) && (newOrg->active)) {
            if (gc_get_modifiesspore(newOrg->gc) >= 10) {
                org_cyan_spore(newOrg);
            } else {
                org_blue_spore(newOrg);
            }
        }
    }
}

/* Makes this organism create many virus and put them in the world.
 * Used for viruses with yellow segments.
 */
void org_reproduce_yellow(Organism *o)
{
    if (o->time_to_reproduce < o->time_to_reproduce_max) {
        o->time_to_reproduce = o->time_to_reproduce_max;
    }
    bool victimized = false;
    // Calculate number of children
    if (o->issilver) {
        if ((o->isaconsumer) || (o->isafungus) || (o->isinfectious)) {
            if (o->total_children < 1) {
                o->n_children = 1;
            } else if (o->total_children < 9) {
                o->n_children = 2;
            } else if (o->total_children < 36) {
                o->n_children = 3;
            } else if (o->total_children < 100) {
                o->n_children = 4;
            } else if (o->total_children < 225) {
                o->n_children = 5;
            } else if (o->total_children < 441) {
                o->n_children = 6;
            } else if (o->total_children < 784) {
                o->n_children = 7;
            } else if (o->total_children >= 784) {
                o->n_children = 8;
            }
        } else {
            if (o->total_children < 2) {
                o->n_children = 2;
            } else if (o->total_children < 5) {
                o->n_children = 3;
            } else if (o->total_children < 9) {
                o->n_children = 4;
            } else if (o->total_children < 14) {
                o->n_children = 5;
            } else if (o->total_children < 20) {
                o->n_children = 6;
            } else if (o->total_children < 27) {
                o->n_children = 7;
            } else if (o->total_children >= 27) {
                o->n_children = 8;
            }
        }
    } else {
        o->n_children = 1;
    }
    if ((o->yellow_counter > 0) && (org_use_energy(o, P.yellow_energy_consumption))) {
        o->n_children += o->yellow_counter;
    }
    if (o->isauburn && (org_use_energy(o, P.auburn_energy_consumption))) {
        o->n_children += 1;
    }
    Organism *newOrg = NULL;
    for (int i = 0; i < bio_between_i(o->n_virus_children, 1, 8); i++) {
        if (o->infected_gc != NULL) {
            newOrg = org_new(o->world);
            if (org_inherit(newOrg, o, i == 0)) {
                // It can be created
                o->total_children++;
                world_add_organism(o->world, newOrg, o);
                o->saved_gc = NULL;
                victimized = true;
            } else {
                break;
            }
        }
    }
    if (victimized == false) {
        o->n_virus_children = 0;
    }
}

/* Makes this organism create a virus and put it in the world. */
void org_reproduce_virus(Organism *o)
{
    o->n_virus_children = 1;
    bool victimized = false;
    // Calculate number of children
    if (o->issilver) {
        if ((o->isaconsumer) || (o->isafungus) || (o->isinfectious)) {
            if (o->total_children < 1) {
                o->n_children = 1;
            } else if (o->total_children < 9) {
                o->n_children = 2;
            } else if (o->total_children < 36) {
                o->n_children = 3;
            } else if (o->total_children < 100) {
                o->n_children = 4;
            } else if (o->total_children < 225) {
                o->n_children = 5;
            } else if (o->total_children < 441) {
                o->n_children = 6;
            } else if (o->total_children < 784) {
                o->n_children = 7;
            } else if (o->total_children >= 784) {
                o->n_children = 8;
            }
        } else {
            if (o->total_children < 2) {
                o->n_children = 2;
            } else if (o->total_children < 5) {
                o->n_children = 3;
            } else if (o->total_children < 9) {
                o->n_children = 4;
            } else if (o->total_children < 14) {
                o->n_children = 5;
            } else if (o->total_children < 20) {
                o->n_children = 6;
            } else if (o->total_children < 27) {
                o->n_children = 7;
            } else if (o->total_children >= 27) {
                o->n_children = 8;
            }
        }
    } else {
        o->n_children = 1;
    }
    if ((o->yellow_counter > 0) && (org_use_energy(o, P.yellow_energy_consumption))) {
        o->n_children = j_d2i(o->n_children + o->yellow_counter * k_scale[o->growth_ratio - 1]);
    }
    if (o->isauburn && (org_use_energy(o, P.auburn_energy_consumption))) {
        o->n_children += 1;
    }
    Organism *newOrg = NULL;
    newOrg = org_new(o->world);
    if (org_inherit(newOrg, o, true)) {
        // It can be created
        o->total_children++;
        world_add_organism(o->world, newOrg, o);
        o->infected_gc = NULL;
        o->saved_gc = NULL;
        if (o->time_to_reproduce < o->time_to_reproduce_max) {
            o->time_to_reproduce = o->time_to_reproduce_max;
        }
        victimized = true;
    }
    if (victimized == false) {
        o->n_virus_children = 0;
    }
}

/* Makes this organism create a foreign virus and put it in the world. */
void org_reproduceforeign_virus(Organism *o)
{
    bool victimized = false;
    // Calculate number of children
    if (o->issilver) {
        if ((o->isaconsumer) || (o->isafungus) || (o->isinfectious)) {
            if (o->total_children < 1) {
                o->n_children = 1;
            } else if (o->total_children < 9) {
                o->n_children = 2;
            } else if (o->total_children < 36) {
                o->n_children = 3;
            } else if (o->total_children < 100) {
                o->n_children = 4;
            } else if (o->total_children < 225) {
                o->n_children = 5;
            } else if (o->total_children < 441) {
                o->n_children = 6;
            } else if (o->total_children < 784) {
                o->n_children = 7;
            } else if (o->total_children >= 784) {
                o->n_children = 8;
            }
        } else {
            if (o->total_children < 2) {
                o->n_children = 2;
            } else if (o->total_children < 5) {
                o->n_children = 3;
            } else if (o->total_children < 9) {
                o->n_children = 4;
            } else if (o->total_children < 14) {
                o->n_children = 5;
            } else if (o->total_children < 20) {
                o->n_children = 6;
            } else if (o->total_children < 27) {
                o->n_children = 7;
            } else if (o->total_children >= 27) {
                o->n_children = 8;
            }
        }
    } else {
        o->n_children = 1;
    }
    if (o->n_virus_children != -1) {
        o->n_virus_children = -1;
        if (o->isauburn && (org_use_energy(o, P.auburn_energy_consumption))) {
            o->n_children += 1;
        }
    }
    Organism *newOrg = NULL;
    newOrg = org_new(o->world);
    if (org_inherit(newOrg, o, true)) {
        // It can be created
        o->total_children++;
        world_add_organism(o->world, newOrg, o);
        o->infected_gc = NULL;
        o->saved_gc = NULL;
        if (o->time_to_reproduce < o->time_to_reproduce_max) {
            o->time_to_reproduce = o->time_to_reproduce_max;
        }
        victimized = true;
    }
    if (victimized == false) {
        o->n_virus_children = 0;
    }
}

/* Executes the organism's movement for this frame.
 * This includes segments upkeep and activation,
 * movement, growth, collision detection, reproduction,
 * respiration and death.
 */
bool org_move(Organism *o)
{
    *&o->last_frame = *&o->r;
    // Apply rubbing effects
    org_rubbing_frames_effects(o);
    // Check if it can grow or shrink
    org_grow(o);
    // Ticker for the time the organism is shown in color
    if (o->frames_color > 0) {
        o->frames_color--;
    }
    // Movement and collisions
    if (o->dx != 0 || o->dy != 0 || o->dtheta != 0 || o->has_grown != 0 || (o->haseyes && o->canmove)) {
        bool collision = false;
        // Movement
        double dxbak = o->dx;
        double dybak = o->dy;
        double dthetabak = o->dtheta;
        org_offset(o, o->dx, o->dy, o->dtheta);
        org_calculate_bounds(o, o->has_grown != 0);
        // Check it is inside the world
        collision = !org_is_inside_world(o);
        // Collision detection with biological corridors
        if (o->world->corridor_exists == true) {
            if (o->active) {
                OutCorridor *c = world_check_hit_corridor(o->world, o);
                if (c != NULL && out_corridor_can_send(c)) {
                    if (out_corridor_send(c, o))
                        return false;
                }
            }
        }
        // Collision detection with other organisms.
        if (world_check_hit(o->world, o) != NULL)
            collision = true;
        // If there is a collision, undo movement.
        if (collision) {
            if (o->has_moved == true) {
                o->has_moved = false;
            }
            org_offset(o, -dxbak, -dybak, -dthetabak);
            if (o->has_grown != 0) {
                o->growth_ratio += o->has_grown;
                org_symmetric(o);
                if (o->update_effects == 1) {
                    o->update_effects = 0;
                }
            }
            org_calculate_bounds(o, o->has_grown != 0);
        } else {
            if (o->has_moved == false) {
                o->has_moved = true;
            }
        }
    } else {
        if (o->has_moved == true) {
            o->has_moved = false;
        }
    }
    if (o->active) {
        // Apply segment effects for this frame.
        if (o->usefriendeffects == 2) {
            // Check if the organism can and has to heal itself later
            if ((o->healing > 0) && (o->isinjured)) {
                o->useextraeffects = true;
            }
            // Try to remove a virus, or restore organism to have live births
            if (o->antiviral > 0) {
                if (o->infected_gc != NULL) {
                    if (jr_next_int(&g_rng, P.immune_system) <= o->antiviral) {
                        if (org_use_energy(o, P.mint_energy_consumption)) {
                            o->infected_gc = NULL;
                            o->saved_gc = NULL;
                            org_set_color(o, CM_MINT);
                        }
                    }
                }
                if (o->reproducelate < 0) {
                    if (jr_next_int(&g_rng, P.immune_system) <= o->antiviral) {
                        if (org_use_energy(o, P.mint_energy_consumption)) {
                            o->reproducelate = 0;
                            org_set_color(o, CM_MINT);
                        }
                    }
                }
            }
            if (o->createlavender > 0) {
                // Lavender shield production
                if (o->lavender < P.lavender_shield) {
                    o->lavender += o->createlavender;
                    if (o->lavender >= P.lavender_shield) {
                        o->lavender = P.lavender_shield;
                    }
                }
                // Fallow induced reproduction delay is reduced faster
                if (o->time_to_reproduce > o->time_to_reproduce_max) {
                    o->time_to_reproduce -= (o->createlavender / 20);
                    if (o->time_to_reproduce < o->time_to_reproduce_max) {
                        o->time_to_reproduce = o->time_to_reproduce_max;
                    }
                }
            }
        }
        if (o->isaplant) {
            if (o->islime) {
                if (world_fast_check_hit(o->world, o) != NULL) {
                    if (o->crowded == false) {
                        o->crowded = true;
                        o->update_effects = 2;
                    }
                } else {
                    if (o->crowded == true) {
                        o->crowded = false;
                        o->update_effects = 2;
                    }
                }
            }
            if (o->summerinactivity > -1) {
                if (world_get_frame(o->world) == 0) {
                    o->update_effects = 2;
                }
            }
            if (o->update_effects > 0) {
                if (o->useextraeffects) {
                    o->useextraeffects = false;
                }
                org_plants_update_effects(o);
                o->update_effects = 0;
                if ((o->reproducelate > 0) && (o->isinfectious)) {
                    // Calculate reproduction energy for flower segments of viruses
                    o->reproduce_energy = (40 + 3 * o->segments) + j_d2i((o->reproducelate * k_scale[o->growth_ratio - 1]));
                }
            } else {
                if (o->useextraeffects) {
                    o->useextraeffects = false;
                    if (o->jadefactor > 0) {
                        org_jade_extra_effects(o);
                    } else {
                        if ((o->healing > 0) && (o->isinjured)) {
                            org_segments_healing_effects(o);
                        } else {
                            if (o->useframemovement) {
                                org_segments_extra_effects(o);
                            } else {
                                org_segments_extra_plants_effects(o);
                            }
                        }
                        if ((o->leafphoto > 0) && (o->frames_color > 0)) {
                            org_leaf_extra_effects(o);
                        } else {
                            // Get chemotrophic energy from methane
                            if (o->methanotrophy > 0) {
                                if ((o->age & 0x07) == 0x00) {
                                    o->energy += world_methanotrophy(o->world, o->methanotrophy);
                                }
                                if (o->photosynthesis > 0) {
                                    //Get sun's energy
                                    if (o->colony_photosynthesis > 0) {
                                        if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                                            if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                                                o->energy += world_photosynthesis(o->world, o->photosynthesis + o->colony_photosynthesis);
                                            }
                                        } else {
                                            o->energy += world_photosynthesis(o->world, o->photosynthesis + o->colony_photosynthesis);
                                        }
                                        o->colony_photosynthesis = 0;
                                    } else {
                                        if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                                            if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                                                o->energy += world_photosynthesis(o->world, o->photosynthesis);
                                            }
                                        } else {
                                            o->energy += world_photosynthesis(o->world, o->photosynthesis);
                                        }
                                    }
                                }
                            } else {
                                //Get sun's energy
                                if (o->colony_photosynthesis > 0) {
                                    if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                                        if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                                            o->energy += world_photosynthesis(o->world, o->photosynthesis + o->colony_photosynthesis);
                                        }
                                    } else {
                                        o->energy += world_photosynthesis(o->world, o->photosynthesis + o->colony_photosynthesis);
                                    }
                                    o->colony_photosynthesis = 0;
                                } else {
                                    if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                                        if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                                            o->energy += world_photosynthesis(o->world, o->photosynthesis);
                                        }
                                    } else {
                                        o->energy += world_photosynthesis(o->world, o->photosynthesis);
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if (o->useframemovement) {
                        org_segments_frame_effects(o);
                    }
                    if ((o->leafphoto > 0) && (o->frames_color > 0)) {
                        org_leaf_extra_effects(o);
                    } else {
                        // Get chemotrophic energy from methane
                        if (o->methanotrophy > 0) {
                            if ((o->age & 0x07) == 0x00) {
                                o->energy += world_methanotrophy(o->world, o->methanotrophy);
                            }
                            if (o->photosynthesis > 0) {
                                //Get sun's energy
                                if (o->colony_photosynthesis > 0) {
                                    if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                                        if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                                            o->energy += world_photosynthesis(o->world, o->photosynthesis + o->colony_photosynthesis);
                                        }
                                    } else {
                                        o->energy += world_photosynthesis(o->world, o->photosynthesis + o->colony_photosynthesis);
                                    }
                                    o->colony_photosynthesis = 0;
                                } else {
                                    if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                                        if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                                            o->energy += world_photosynthesis(o->world, o->photosynthesis);
                                        }
                                    } else {
                                        o->energy += world_photosynthesis(o->world, o->photosynthesis);
                                    }
                                }
                            }
                        } else {
                            //Get sun's energy
                            if (o->colony_photosynthesis > 0) {
                                if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                                    if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                                        o->energy += world_photosynthesis(o->world, o->photosynthesis + o->colony_photosynthesis);
                                    }
                                } else {
                                    o->energy += world_photosynthesis(o->world, o->photosynthesis + o->colony_photosynthesis);
                                }
                                o->colony_photosynthesis = 0;
                            } else {
                                if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                                    if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                                        o->energy += world_photosynthesis(o->world, o->photosynthesis);
                                    }
                                } else {
                                    o->energy += world_photosynthesis(o->world, o->photosynthesis);
                                }
                            }
                        }
                    }
                }
            }
            if (o->spin > 0) {
                if (o->clockwise) {
                    o->dtheta = bio_between(o->dtheta + (o->spin * k_springscale[o->growth_ratio - 1]) * M_PI / o->inertia, -P.max_rot, P.max_rot);
                } else {
                    o->dtheta = bio_between(o->dtheta - (o->spin * k_springscale[o->growth_ratio - 1]) * M_PI / o->inertia, -P.max_rot, P.max_rot);
                }
            }
        } else {
            if (o->update_effects > 0) {
                if (o->useextraeffects) {
                    o->useextraeffects = false;
                }
                org_segments_update_effects(o);
                o->update_effects = 0;
                if ((o->reproducelate > 0) && (o->isinfectious)) {
                    // Calculate reproduction energy for flower segments of viruses
                    o->reproduce_energy = (40 + 3 * o->segments) + j_d2i((o->reproducelate * k_scale[o->growth_ratio - 1]));
                }
            } else {
                if (o->useextraeffects) {
                    o->useextraeffects = false;
                    if ((o->healing > 0) && (o->isinjured)) {
                        org_segments_healing_effects(o);
                    } else {
                        org_segments_extra_effects(o);
                    }
                } else {
                    if (o->useframemovement) {
                        org_segments_frame_effects(o);
                    }
                }
            }
        }
        // Reset dodging
        if (o->candodge == true) {
            if (o->hasdodged == true) {
                o->dodge = false;
                o->hasdodged = false;
            } else if (o->dodge == false) {
                o->dodge = true;
            }
        }
        // Substract one to the time needed to reproduce
        if (o->time_to_reproduce > 0) {
            o->time_to_reproduce--;
        } else {
            // Calculate reproduction energy for blond segments and check if it can reproduce
            if (o->early_reproduce_energy > 0) {
                if (o->energy > o->early_reproduce_energy) {
                    if (o->infected_gc == NULL) {
                        if (o->isaplant) {
                            if (o->growth_ratio == 1) {
                                org_reproduce_early_plant(o);
                            }
                        } else {
                            if (o->isaconsumer) {
                                if (o->growth_ratio == 1) {
                                    org_reproduce_early(o);
                                }
                            } else {
                                if (o->isafungus) {
                                    if (o->sporeversion < 0) {
                                        if (o->growth_ratio == 1) {
                                            org_reproduce_early(o);
                                        }
                                    } else {
                                        if (o->growth_ratio < 16) {
                                            org_reproduce_early(o);
                                        }
                                    }
                                } else {
                                    if (o->transfersenergy) {
                                        if (o->growth_ratio < 16) {
                                            org_reproduce_early(o);
                                        }
                                    } else {
                                        if (o->growth_ratio < 16) {
                                            org_reproduce_early_virus(o);
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        // Check if it can reproduce: it needs enough energy and to be adult
                        if (o->energy > org_get_reproduce_energy(o) && o->growth_ratio == 1) {
                            if ((o->antiviral == 0) || (o->lavender == 0)) {
                                org_reproduce(o);
                            } else {
                                if (org_use_energy(o, P.mint_energy_consumption / 10)) {
                                    if (o->lavender > 0) {
                                        org_lavenderinhibition(o);
                                    }
                                    if (o->lavender <= 0) {
                                        org_reproduce(o);
                                    }
                                } else {
                                    org_reproduce(o);
                                }
                            }
                        }
                    }
                }
            } else {
                // Check if it can reproduce: it needs enough energy and to be adult
                if (o->energy > org_get_reproduce_energy(o) && o->growth_ratio == 1) {
                    if ((o->antiviral == 0) || (o->lavender == 0) || (o->infected_gc == NULL)) {
                        org_reproduce(o);
                    } else {
                        if (org_use_energy(o, P.mint_energy_consumption / 10)) {
                            if (o->lavender > 0) {
                                org_lavenderinhibition(o);
                            }
                            if (o->lavender <= 0) {
                                org_reproduce(o);
                            }
                        } else {
                            org_reproduce(o);
                        }
                    }
                }
            }
        }
        // Maintenance of active organisms
        org_breath(o);
    } else {
        // Maintenance of dead organisms and spores
        org_decay(o);
    }
    // Check that the organism has energy after this frame
    return o->energy > BIO_TOL;
}

/* Makes the organism decay an amount of energy using the
 * decomposition process (also used for pink feeding).
 * 
 * @param q  The quantity of energy to decay.
 * @return  true if the organism has enough energy and there is
 * enough oxygen in the atmosphere, false otherwise.
 */
bool org_use_decay_energy(Organism *o, double q)
{
    if (o->energy < q) {
        return false;
    }
    double decomposition = world_decomposition(o->world, q);
    o->energy -= decomposition;
    if (decomposition < q)
        return false;
    return true;
}

/* Makes the organism spend an amount of energy using the
 * respiration process.
 * 
 * @param q  The quantity of energy to spend.
 * @return  true if the organism has enough energy and there is
 * enough oxygen in the atmosphere, false otherwise.
 */
bool org_use_energy(Organism *o, double q)
{
    if (o->energy < q) {
        return false;
    }
    double respiration = world_respiration(o->world, q);
    o->energy -= respiration;
    if (respiration < q)
        return false;
    return true;
}

/* Realize the respiration process to maintain its structure.
 * Aging is applied here too.
 */
void org_breath(Organism *o)
{
    o->age++;
    // Respiration process
    bool canBreath = org_use_energy(o, jmin(o->maintenance, o->energy));
    if ((o->age >> 8) > org_get_max_age(o) || !canBreath) {
        // Dormant and moving spore organisms may reproduce before dying
        if ((o->sporeversion == -1) && (o->energy > (org_get_reproduce_energy(o) / 2)) && (o->growth_ratio == 1)) {
            if ((o->age >> 8) > org_get_max_age(o)) {
                o->reproduce_energy = j_d2i(o->energy) + (16 + 3 * gc_get_symmetry(o->gc));
                org_reproduce_spore(o);
                o->reproduce_energy = (40 + 3 * o->segments);
            }
        }
        // It's dead, but still may have energy
        org_die(o, NULL);
    } else {
        // Check that it don't exceed the maximum chemical energy
        if (o->energy > org_get_reproduce_energy(o)) {
            if (o->energy > 2 * org_get_reproduce_energy(o)) {
                org_use_energy(o, j_d2i(o->energy) - 2 * o->reproduce_energy);
                if ((o->reproducelate > 0) && (!o->isinfectious) && (o->time_to_reproduce > 0)) {
                    o->time_to_reproduce = 0;
                }
            } else {
                if ((o->reproducelate <= 0) || (o->isinfectious)) {
                    org_use_energy(o, (j_d2i(o->energy) - o->reproduce_energy) * 0.005);
                } else {
                    if ((o->time_to_reproduce > 0) && (o->energy > 2 * org_get_reproduce_energy(o) - o->reproducelate)) {
                        o->time_to_reproduce = 0;
                    }
                }
            }
        } else {
            if (o->energy <= BIO_TOL) {
                o->active = false;
                o->alive = false;
                world_decrease_population(o->world);
                world_organism_has_died(o->world, o, NULL);
            }
        }
    }
}

/* Realize the decaying process of dead organisms
 * Spore respiration, movement and hatching is applied here too.
 */
void org_decay(Organism *o)
{
    // Spore respiration, movement and hatching
    if (o->alive) {
        if (o->sporeversion == 2) {
            o->age++;
            if ((o->age >> 8) > org_get_max_age(o)) {
                // It's dead, but still may have energy
                org_die(o, NULL);
            } else {
                if (o->energy <= BIO_TOL) {
                    o->alive = false;
                    world_decrease_population(o->world);
                    world_organism_has_died(o->world, o, NULL);
                }
            }
        } else if (o->sporeversion == 1) {
            o->age++;
            // Respiration process
            bool canBreath = org_use_energy(o, jmin(0.003, o->energy));
            if ((o->age >> 8) >= org_get_max_age(o) || !canBreath) {
                // It's dead, but still may have energy
                org_die(o, NULL);
            } else {
                if (o->energy <= BIO_TOL) {
                    o->alive = false;
                    world_decrease_population(o->world);
                    world_organism_has_died(o->world, o, NULL);
                } else {
                    // mobile spore movement
                    if (!o->isfrozen) {
                        int i = 0;
                        for (i = o->segments - 1; i >= 0; i--) {
                            // Movement
                            if (jr_next_int(&g_rng, 100) < 8) {
                                o->dx = bio_between(o->dx + 12.0 * (o->x2[i] - o->x1[i]) / o->mass, -P.max_vel, P.max_vel);
                                o->dy = bio_between(o->dy + 12.0 * (o->y2[i] - o->y1[i]) / o->mass, -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        }
                    }
                }
            }
        } else if (o->sporeversion == 5) {
            o->age++;
            // Respiration process
            bool canBreath = org_use_energy(o, jmin(0.0027, o->energy));
            if ((o->age >> 8) >= org_get_max_age(o) || !canBreath) {
                // It's dead, but still may have energy
                org_die(o, NULL);
            } else {
                if (o->energy <= BIO_TOL) {
                    o->alive = false;
                    world_decrease_population(o->world);
                    world_organism_has_died(o->world, o, NULL);
                } else {
                    // mobile spore movement
                    if (!o->isfrozen) {
                        int i = 0;
                        for (i = o->segments - 1; i >= 0; i--) {
                            // Movement
                            if (jr_next_int(&g_rng, 100) < 8) {
                                o->dx = bio_between(o->dx + 12.0 * (o->x2[i] - o->x1[i]) / (9 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(o->dy + 12.0 * (o->y2[i] - o->y1[i]) / (9 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        }
                    }
                    // Spore hatching
                    if (o->energy > 0.5 * org_get_reproduce_energy(o)) {
                        if (org_use_energy(o, P.spore_energy_consumption)) {
                            for (int j = 0; j < o->segments; j++) {
                                o->seg_color[j] = gene_get_color(gc_get_gene(org_get_genetic_code(o), j % gc_get_n_genes(org_get_genetic_code(o))));
                                o->mphoto[j] = 0;
                            }
                            o->frames_color = 0;
                            o->isaconsumer = false;
                            o->isfrozen = false;
                            o->isinjured = false;
                            o->allfrozen = false;
                            o->active = true;
                            o->has_moved = true;
                            o->age = 1;
                            org_segments_create_effects(o);
                            if (o->haseyes) {
                                org_symmetric(o);
                            }
                        }
                    }
                }
            }
        } else if (o->sporeversion == 6) {
            if ((!o->isinjured) || org_use_decay_energy(o, jmin(0.001, o->energy))) {
                o->age++;
                if ((o->age >> 8) > org_get_max_age(o)) {
                    // It's dead, but still may have energy
                    org_die(o, NULL);
                } else {
                    if (o->energy <= BIO_TOL) {
                        o->alive = false;
                        world_decrease_population(o->world);
                        world_organism_has_died(o->world, o, NULL);
                    } else {
                        // Spore hatching
                        if (((o->isblond) && (o->gold >= 0) && (o->energy > 0.5 * o->early_reproduce_energy)) || (o->energy > 0.5 * org_get_reproduce_energy(o))) {
                            if (((o->isblond) || (o->gold > 0) || (o->sporetime < 0)) && (o->infected_gc != NULL)) {
                                org_reproduce_virus(o);
                                o->n_virus_children = 0;
                                if (o->energy <= BIO_TOL) {
                                    o->alive = false;
                                    world_decrease_population(o->world);
                                    world_organism_has_died(o->world, o, NULL);
                                }
                            } else {
                                if (o->time_to_reproduce == 0) {
                                    if (org_use_energy(o, P.spore_energy_consumption)) {
                                        for (int j = 0; j < o->segments; j++) {
                                            o->seg_color[j] = gene_get_color(gc_get_gene(org_get_genetic_code(o), j % gc_get_n_genes(org_get_genetic_code(o))));
                                            o->mphoto[j] = 0;
                                        }
                                        o->photosynthesis = 0;
                                        o->methanotrophy = 0;
                                        o->leafphoto = 0;
                                        o->forestphoto = 0;
                                        o->frames_color = 0;
                                        o->isaplant = false;
                                        if (o->jadefactor > 0) {
                                            o->jadefactor = 1;
                                        }
                                        o->isfrozen = false;
                                        o->isinjured = false;
                                        o->allfrozen = false;
                                        o->active = true;
                                        o->has_moved = true;
                                        o->gold = 0;
                                        o->age = 1;
                                        org_segments_create_effects(o);
                                        if (o->haseyes) {
                                            org_symmetric(o);
                                        }
                                    }
                                } else {
                                    o->time_to_reproduce--;
                                    org_set_color(o, CM_FLOWER);
                                }
                            }
                        } else {
                            // Photosynthesis
                            if (!o->isinjured) {
                                if (o->methanotrophy > 0) {
                                    if ((o->age & 0x07) == 0x00) {
                                        o->energy += world_methanotrophy(o->world, o->methanotrophy);
                                    }
                                } else {
                                    if (o->islime) {
                                        if (world_fast_check_hit(o->world, o) != NULL) {
                                            o->energy += world_photosynthesis(o->world, o->photosynthesis);
                                        } else {
                                            // leafphoto is used for lime photosynthesis, if alone, here
                                            o->energy += world_photosynthesis(o->world, o->leafphoto);
                                        }
                                    } else {
                                        if (o->colony_photosynthesis > 0) {
                                            o->energy += world_photosynthesis(o->world, o->photosynthesis + o->colony_photosynthesis);
                                            o->colony_photosynthesis = 0;
                                        } else {
                                            o->energy += world_photosynthesis(o->world, o->photosynthesis);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else if (o->sporeversion >= 3) {
            if ((!o->isinjured) || org_use_decay_energy(o, jmin(0.001, o->energy))) {
                if (o->energy <= BIO_TOL) {
                    o->alive = false;
                    world_decrease_population(o->world);
                    world_organism_has_died(o->world, o, NULL);
                } else {
                    // Spore hatching
                    if (o->time_to_reproduce == 0) {
                        if (org_use_energy(o, P.spore_energy_consumption)) {
                            for (int j = 0; j < o->segments; j++)
                                o->seg_color[j] = gene_get_color(gc_get_gene(org_get_genetic_code(o), j % gc_get_n_genes(org_get_genetic_code(o))));
                            o->frames_color = 0;
                            o->isenhanced = false;
                            o->isfrozen = false;
                            o->isinjured = false;
                            o->allfrozen = false;
                            o->active = true;
                            o->has_moved = true;
                            o->age = 1;
                            org_segments_create_effects(o);
                            if (o->haseyes) {
                                org_symmetric(o);
                            }
                        }
                    } else {
                        if (!o->isinjured) {
                            // Color for egg spore
                            if ((o->sporeversion == 4) && (o->frames_color <= 1)) {
                                org_set_color(o, CM_SPORE);
                            }
                            o->time_to_reproduce--;
                        }
                    }
                }
            }
        } else if (o->sporeversion == -3) {
            if ((!o->isinjured) || org_use_decay_energy(o, jmin(0.001, o->energy))) {
                // Respiration process
                bool canBreath = org_use_energy(o, jmin(0.001, o->energy));
                if (!canBreath) {
                    // It's dead, but still may have energy
                    org_die(o, NULL);
                } else {
                    if (o->energy <= BIO_TOL) {
                        o->alive = false;
                        world_decrease_population(o->world);
                        world_organism_has_died(o->world, o, NULL);
                    } else {
                        // mobile spore movement
                        if (!o->isfrozen) {
                            // Color for moving spore
                            if (o->frames_color <= 1) {
                                org_set_color(o, JC_CYAN);
                            }
                            int i = 0;
                            for (i = o->segments - 1; i >= 0; i--) {
                                // Movement
                                if (jr_next_int(&g_rng, 100) < 8) {
                                    o->dx = bio_between(o->dx + 12.0 * (o->x2[i] - o->x1[i]) / o->mass, -P.max_vel, P.max_vel);
                                    o->dy = bio_between(o->dy + 12.0 * (o->y2[i] - o->y1[i]) / o->mass, -P.max_vel, P.max_vel);
                                    o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                                }
                            }
                        }
                        // Spore hatching
                        if (o->time_to_reproduce == 0) {
                            if ((!o->isinjured) && ((world_fast_check_hit(o->world, o) == NULL) || (o->gold == 0))) {
                                org_hatch(o);
                            } else {
                                if (o->gold > 0) {
                                    o->gold--;
                                }
                            }
                        } else {
                            o->time_to_reproduce--;
                        }
                    }
                }
            }
        } else {
            if ((!o->isinjured) || org_use_decay_energy(o, jmin(0.001, o->energy))) {
                if (o->energy <= BIO_TOL) {
                    o->alive = false;
                    world_decrease_population(o->world);
                    world_organism_has_died(o->world, o, NULL);
                } else {
                    // Spore hatching
                    if (o->time_to_reproduce == 0) {
                        if ((!o->isinjured) && ((world_fast_check_hit(o->world, o) == NULL) || (o->gold == 0))) {
                            org_hatch(o);
                        } else {
                            if (o->gold > 0) {
                                o->gold--;
                            }
                        }
                    } else {
                        o->time_to_reproduce--;
                    }
                }
            }
        }
    } else {
        // The corpse slowly decays
        org_use_decay_energy(o, jmin(o->energy, P.decay_energy));
    }
}

/* Create blue stationary spore */
void org_blue_spore(Organism *o)
{
    if (gc_get_modifiesspore(o->gc) == 7) {
        for (int i = 0; i < o->segments; i++) {
            o->seg_color[i] = JC_BLUE;
            o->mphoto[i] = 0;
        }
    } else {
        for (int i = 0; i < o->segments; i++) {
            o->seg_color[i] = JC_BLUE;
            o->mphoto[i] = -2;
        }
    }
    o->sporeversion = -2;
    o->time_to_reproduce = o->sporetime / gc_get_symmetry(o->gc);
    o->time_to_reproduce = (o->time_to_reproduce * o->time_to_reproduce * o->time_to_reproduce) / 400;
    // _gold is used for the possible hatch delay here 
    o->gold = 20;
    o->usepretoucheffects = false;
    o->usefriendeffects = 0;
    o->canreact = false;
    o->active = false;
    o->has_moved = true;
    o->isaplant = false;
    o->isaconsumer = false;
    if (o->isafungus) {
        o->isafungus = false;
        o->spin = -1;
    }
    o->isenhanced = false;
    if (o->haseyes) {
        o->haseyes = false;
        org_symmetric(o);
    }
}

/* Create moving spore (shown in cyan color later) */
void org_cyan_spore(Organism *o)
{
    if (gc_get_modifiesspore(o->gc) == 10) {
        for (int i = 0; i < o->segments; i++) {
            o->seg_color[i] = JC_BLUE;
            o->mphoto[i] = 0;
        }
    } else {
        for (int i = 0; i < o->segments; i++) {
            o->seg_color[i] = JC_BLUE;
            o->mphoto[i] = -2;
        }
    }
    o->sporeversion = -3;
    o->time_to_reproduce = o->sporetime / gc_get_symmetry(o->gc);
    o->time_to_reproduce = (o->time_to_reproduce * o->time_to_reproduce) / 40;
    // _gold is used for the possible hatch delay here 
    o->gold = 20;
    o->usepretoucheffects = false;
    o->usefriendeffects = 0;
    o->canreact = false;
    o->active = false;
    o->has_moved = true;
    o->isaplant = false;
    o->isaconsumer = false;
    if (o->isafungus) {
        o->isafungus = false;
        o->spin = -1;
    }
    o->isenhanced = false;
    if (o->haseyes) {
        o->haseyes = false;
        org_symmetric(o);
    }
}

/* Hatching for blue and cyan spores */
void org_hatch(Organism *o)
{
    if (org_use_energy(o, P.spore_energy_consumption)) {
        for (int j = 0; j < o->segments; j++) {
            o->seg_color[j] = gene_get_color(gc_get_gene(org_get_genetic_code(o), j % gc_get_n_genes(org_get_genetic_code(o))));
            o->mphoto[j] = 0;
        }
        o->frames_color = 0;
        if (gc_get_modifiesspore(o->gc) <= 9) {
            o->sporeversion = -2;
        } else {
            o->sporeversion = -3;
        }
        o->time_to_reproduce = 0;
        o->gold = 0;
        o->isaplant = false;
        o->isfrozen = false;
        o->isinjured = false;
        o->allfrozen = false;
        o->active = true;
        o->has_moved = true;
        o->age++;
        org_segments_create_effects(o);
        if (o->haseyes) {
            org_symmetric(o);
        }
    } else {
        org_use_energy(o, jmin(0.01, o->energy));
        if (o->energy <= BIO_TOL) {
            o->alive = false;
            world_decrease_population(o->world);
            world_organism_has_died(o->world, o, NULL);
        }
    }
}

/* Applies hatching produced by a spore touching a segment
 * 
 * @param org  The organism which is touching.
 * @param seg  Index of this organism's segment.
 * @param oseg  Index of the other organism's segment.
 */
static void org_touch_hatch(Organism *o, Organism *org, int seg, int oseg)
{
    (void)seg;
    switch (org_get_type_color(org->seg_color[oseg])) {
    case TY_RED:
        if (gc_get_adaptspore(o->gc) % 10 == 1) {
            org_hatch(o);
        }
        break;
    case TY_FIRE:
        if (gc_get_adaptspore(o->gc) % 10 == 2) {
            org_hatch(o);
        }
        break;
    case TY_ORANGE:
    case TY_MAROON:
        if (gc_get_adaptspore(o->gc) % 10 == 3) {
            org_hatch(o);
        }
        break;
    case TY_PINK:
        if (gc_get_adaptspore(o->gc) % 10 == 5) {
            org_hatch(o);
        }
        break;
    case TY_BLUE:
    case TY_LIGHT_BLUE:
    case TY_OCHRE:
    case TY_FALLOW:
    case TY_BARK:
    case TY_OLDBARK:
    case TY_DEADBARK:
    case TY_ICE:
        if ((org->isaplant) && (!org->isaconsumer) && (!org->isafungus)) {
            if (gc_get_adaptspore(o->gc) % 10 == 9) {
                org_hatch(o);
            }
        }
        break;
    case TY_OLIVE:
    case TY_DARKOLIVE:
    case TY_SKY:
    case TY_DEEPSKY:
    case TY_VIOLET:
        if ((o->healing > 0) && (org->isaplant) && (!org->isaconsumer) && (!org->isafungus)) {
            if (gc_get_adaptspore(o->gc) % 10 == 9) {
                org_hatch(o);
            }
        }
        break;
    case TY_SPIKE:
    case TY_SPIKEPOINT:
    case TY_GRAY:
        if ((org->isaplant) && (!org->isaconsumer) && (!org->isafungus)) {
            if (gc_get_adaptspore(o->gc) % 10 == 8) {
                org_hatch(o);
            }
        }
        break;
    case TY_LILAC:
    case TY_DARKLILAC:
        if ((!gc_get_modifieslilac(org->gc)) && (org->isaplant) && (!org->isaconsumer) && (!org->isafungus)) {
            if (gc_get_adaptspore(o->gc) % 10 == 8) {
                org_hatch(o);
            }
        }
        break;
    case TY_SILVER:
        if (org->isaconsumer) {
            if (gc_get_adaptspore(o->gc) % 10 == 2) {
                org_hatch(o);
            }
        } else if (org->isafungus) {
            if (gc_get_adaptspore(o->gc) % 10 == 5) {
                org_hatch(o);
            }
        } else if (org->isaplant) {
            if (org->isinfectious) {
                if (gc_get_adaptspore(o->gc) % 10 == 6) {
                    org_hatch(o);
                }
            } else {
                if (gc_get_adaptspore(o->gc) % 10 == 7) {
                    org_hatch(o);
                }
            }
        } else if (org->active) {
            if (gc_get_adaptspore(o->gc) % 10 == 0) {
                org_hatch(o);
            }
        }
        break;
    case TY_CORAL:
        if (org->isaconsumer) {
            if (gc_get_adaptspore(o->gc) % 10 == 4) {
                org_hatch(o);
            }
        } else if (org->isafungus) {
            if (gc_get_adaptspore(o->gc) % 10 == 5) {
                org_hatch(o);
            }
        } else if (org->isaplant) {
            if (org->isinfectious) {
                if (gc_get_adaptspore(o->gc) % 10 == 6) {
                    org_hatch(o);
                }
            } else {
                if (gc_get_adaptspore(o->gc) % 10 == 7) {
                    org_hatch(o);
                }
            }
        } else if (org->active) {
            if ((!o->isinfectious) && (!o->iscoral) && (gc_get_adaptspore(o->gc) % 10 == 0)) {
                org_hatch(o);
            }
        }
        break;
    case TY_FRUIT:
        if (gc_get_modifiesspore(org->gc) == 1) {
            if (gc_get_adaptspore(o->gc) % 10 == 0) {
                org_hatch(o);
            }
        } else {
            if (gc_get_adaptspore(o->gc) % 10 == 6) {
                org_hatch(o);
            }
        }
        break;
    case TY_CREAM:
    case TY_BROWN:
        // "spin -1" is used for "isafungus" here
        if (o->spin == -1) {
            org_hatch(o);
        }
        break;
    default:
        if (org->isaconsumer) {
            if (gc_get_adaptspore(o->gc) % 10 == 4) {
                org_hatch(o);
            }
        } else if (org->isafungus) {
            if (gc_get_adaptspore(o->gc) % 10 == 5) {
                org_hatch(o);
            }
        } else if (org->isaplant) {
            if (org->isinfectious) {
                if (gc_get_adaptspore(o->gc) % 10 == 6) {
                    org_hatch(o);
                }
            } else {
                if (gc_get_adaptspore(o->gc) % 10 == 7) {
                    org_hatch(o);
                }
            }
        } else if (org->active) {
            if (gc_get_adaptspore(o->gc) % 10 == 0) {
                org_hatch(o);
            }
        }
    }
}

/* Kills the organism. Sets its segments to brown and tells the world
 * about the event.
 * 
 * @param killingOrganism  The organism that has killed this organism,
 * or null if it has died of natural causes.
 */
void org_die(Organism *o, Organism *killingOrganism)
{
    o->active = false;
    o->alive = false;
    o->usepretoucheffects = false;
    o->usefriendeffects = 0;
    o->photosynthesis = 0;
    o->useframemovement = false;
    o->canmove = false;
    o->canreact = false;
    o->has_moved = true;
    o->infected_gc = NULL;
    o->saved_gc = NULL;
    for (int i = 0; i < o->segments; i++) {
        if (o->seg_color[i] == CM_EYE) {
            o->seg_color[i] = CM_EYE;
        } else {
            o->seg_color[i] = CM_BROWN;
        }
    }
    world_decrease_population(o->world);
    if (killingOrganism != NULL)
        killingOrganism->total_kills++;
    world_organism_has_died(o->world, o, killingOrganism);
}

/* Magenta segments are getting weakened by a gray segment
 * 
 * @param grayOrganism  The organism that weakened
 * all magenta segments of this organism
 */
void org_survive(Organism *o, Organism *grayOrganism)
{
    o->healing = 0;
    for (int i = 0; i < o->segments; i++) {
        if (o->seg_color[i] == JC_MAGENTA) {
            o->seg_color[i] = CM_DARKFIRE;
            o->useextraeffects = true;
            o->isinjured = true;
        }
    }
    org_set_color(grayOrganism, JC_GRAY);
}

/* Reproductive segments are getting destroyed by a fallow segment
 * 
 * @param fallowOrganism  The organism that destroyed
 * the reproductive segments of this organism
 */
void org_inhibited(Organism *o, Organism *fallowOrganism)
{
    for (int i = 0; i < o->segments; i++) {
        switch (org_get_type_color(o->seg_color[i])) {
        case TY_SPORE:
            if ((o->sporeversion > 0) && (org_use_energy(fallowOrganism, P.fallow_energy_consumption))) {
                if (o->isaplant) {
                    o->seg_color[i] = CM_BROKEN;
                } else {
                    o->seg_color[i] = CM_LIGHTBROWN;
                }
                org_set_color(fallowOrganism, CM_FALLOW);
                o->update_effects = 2;
            }
            break;
        case TY_AUBURN:
        case TY_INDIGO:
            if (org_use_energy(fallowOrganism, P.fallow_energy_consumption)) {
                if (o->isaplant) {
                    o->seg_color[i] = CM_BROKEN;
                } else {
                    o->seg_color[i] = CM_LIGHTBROWN;
                }
                org_set_color(fallowOrganism, CM_FALLOW);
                o->update_effects = 2;
            }
            break;
        case TY_YELLOW:
        case TY_BLOND:
            if (org_use_energy(fallowOrganism, P.fallow_energy_consumption)) {
                if (o->isaplant) {
                    o->seg_color[i] = CM_BROKEN;
                } else {
                    o->seg_color[i] = CM_LIGHTBROWN;
                }
                org_set_color(fallowOrganism, CM_FALLOW);
            }
            break;
        }
    }
    org_segments_restore_effects(o);
}

/* Green and moving segments are getting destroyed by a modified sky segment
 * 
 * @param freezingOrganism  The organism that destroyed
 * the green and moving segments of this organism
 */
void org_frozen(Organism *o, Organism *freezingOrganism)
{
    bool freeze = false;
    o->allfrozen = true;
    o->canmove = false;
    o->canreact = false;
    o->useframemovement = false;
    if (o->skyversion > 0) {
        o->skyversion = -1;
    }
    if (o->jadefactor > 0) {
        o->jadefactor = 1;
    }
    o->spin = 0;
    for (int i = 0; i < o->segments; i++) {
        switch (org_get_type_color(o->seg_color[i])) {
        case TY_SKY:
        case TY_DEEPSKY:
            if (org_use_energy(freezingOrganism, P.sky_energy_consumption)) {
                o->seg_color[i] = CM_LIGHT_BLUE;
                freeze = true;
                o->isinjured = true;
                if ((!o->isaplant) && (!gc_get_modifiessky(o->gc))) {
                    o->update_effects = 2;
                }
            } else {
                if (o->skyversion <= 0) {
                    if (gc_get_modifiessky(o->gc)) {
                        o->skyversion = 2;
                    } else {
                        o->skyversion = 1;
                    }
                }
                o->allfrozen = false;
            }
            break;
        case TY_TEAL:
            if (org_use_energy(freezingOrganism, P.sky_energy_consumption)) {
                o->seg_color[i] = CM_ICE;
                if (gc_get_passive(o->gc)) {
                    o->update_effects = 2;
                }
                freeze = true;
                o->isinjured = true;
                o->isfrozen = true;
                o->mphoto[i] = 0;
            } else {
                o->allfrozen = false;
                o->canmove = true;
                o->canreact = true;
                if (!gc_get_passive(o->gc)) {
                    o->useframemovement = true;
                }
            }
            break;
        case TY_CYAN:
            if (org_use_energy(freezingOrganism, P.sky_energy_consumption)) {
                o->seg_color[i] = CM_ICE;
                freeze = true;
                o->isinjured = true;
                o->isfrozen = true;
                o->mphoto[i] = 0;
            } else {
                o->allfrozen = false;
                o->canmove = true;
                o->useframemovement = true;
            }
            break;
        case TY_WINTER:
        case TY_GREENBROWN:
            if (org_use_energy(freezingOrganism, P.sky_energy_consumption)) {
                o->seg_color[i] = CM_ICE;
                freeze = true;
                o->isinjured = true;
                o->isfrozen = true;
            } else {
                o->allfrozen = false;
            }
            break;
        case TY_JADE:
        case TY_DARKJADE:
            if (org_use_energy(freezingOrganism, P.sky_energy_consumption)) {
                o->seg_color[i] = CM_ICE;
                freeze = true;
                o->isinjured = true;
                o->isfrozen = true;
                o->update_effects = 2;
            } else {
                o->allfrozen = false;
                o->jadefactor = 2;
            }
            break;
        case TY_SPRING:
            if (org_use_energy(freezingOrganism, P.sky_energy_consumption)) {
                o->seg_color[i] = CM_ICE;
                freeze = true;
                o->isinjured = true;
                o->isfrozen = true;
                o->update_effects = 2;
            } else {
                o->allfrozen = false;
                o->spin += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            }
            break;
        case TY_DARKGREEN:
            if (org_use_energy(freezingOrganism, P.sky_energy_consumption)) {
                o->seg_color[i] = CM_ICE;
                freeze = true;
                o->isinjured = true;
                o->isfrozen = true;
                o->update_effects = 2;
            } else {
                o->allfrozen = false;
                if (gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc))) == CM_JADE) {
                    o->jadefactor = 2;
                }
                if (gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc))) == CM_SPRING) {
                    o->spin += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
                }
            }
            break;
        case TY_SUMMER:
        case TY_LEAF:
        case TY_LIME:
        case TY_GREEN:
        case TY_FOREST:
        case TY_GRASS:
        case TY_C4:
        case TY_PURPLE:
            if (org_use_energy(freezingOrganism, P.sky_energy_consumption)) {
                o->seg_color[i] = CM_ICE;
                freeze = true;
                o->isinjured = true;
                o->isfrozen = true;
                o->update_effects = 2;
            } else {
                o->allfrozen = false;
            }
            break;
        case TY_OLDBARK:
        case TY_BARK:
            if (org_use_energy(freezingOrganism, P.sky_energy_consumption)) {
                o->seg_color[i] = CM_DEADBARK;
                freeze = true;
                o->isfrozen = true;
                o->update_effects = 2;
            } else {
                o->allfrozen = false;
            }
            break;
        }
    }
    if (freeze) {
        org_set_color(freezingOrganism, CM_SKY);
    }
    if ((!o->canreact) && (!o->isonlyc4)) {
        o->candodge = false;
        o->dodge = false;
    }
    // Calculate jade delay used in restoration of the color
    if (o->jadefactor > 1) {
        if (gc_get_symmetry(o->gc) != 1) {
            o->jadefactor = P.darkjade_delay * gc_get_symmetry(o->gc);
        } else {
            o->jadefactor = P.darkjade_delay * 2;
        }
    }
}

/* Moving segments are getting destroyed by an unmodified sky segment
 * 
 * @param freezingOrganism  The organism that destroyed
 * the green and moving segments of this organism
 */
void org_paralyzed(Organism *o, Organism *freezingOrganism)
{
    bool freeze = false;
    o->canmove = false;
    o->canreact = false;
    o->useframemovement = false;
    for (int i = 0; i < o->segments; i++) {
        switch (org_get_type_color(o->seg_color[i])) {
        case TY_TEAL:
            if (org_use_energy(freezingOrganism, P.sky_energy_consumption)) {
                o->seg_color[i] = CM_ICE;
                if (gc_get_passive(o->gc)) {
                    o->update_effects = 2;
                }
                freeze = true;
                o->isinjured = true;
                o->isfrozen = true;
                o->mphoto[i] = 0;
            } else {
                o->canmove = true;
                o->canreact = true;
                if (!gc_get_passive(o->gc)) {
                    o->useframemovement = true;
                }
            }
            break;
        case TY_CYAN:
            if (org_use_energy(freezingOrganism, P.sky_energy_consumption)) {
                o->seg_color[i] = CM_ICE;
                freeze = true;
                o->isinjured = true;
                o->isfrozen = true;
                o->mphoto[i] = 0;
            } else {
                o->canmove = true;
                o->useframemovement = true;
            }
            break;
        }
    }
    if (freeze) {
        org_set_color(freezingOrganism, CM_SKY);
    }
    if ((!o->canreact) && (!o->isonlyc4)) {
        o->candodge = false;
        o->dodge = false;
    }
}

/* Infectious, coral and fallow segments are getting destroyed by a mint segment
 * 
 * @param mintOrganism  The organism that destroyed
 * the infectious, coral and fallow segments of this organism
 */
void org_neutralized(Organism *o, Organism *mintOrganism)
{
    bool neutralize = false;
    for (int i = 0; i < o->segments; i++) {
        switch (org_get_type_color(o->seg_color[i])) {
        case TY_WHITE:
        case TY_PLAGUE:
            if (o->isaplant) {
                if (org_use_energy(mintOrganism, P.mint_energy_consumption / 10)) {
                    o->seg_color[i] = CM_BROKEN;
                    neutralize = true;
                }
            } else {
                if ((o->isaconsumer) || (o->iscoral)) {
                    if (org_use_energy(mintOrganism, P.mint_energy_consumption / 10)) {
                        o->seg_color[i] = CM_LIGHTBROWN;
                        neutralize = true;
                    }
                } else {
                    if ((o->antiviral == 0) || (mintOrganism->isaplant) || (mintOrganism->isaconsumer) || (mintOrganism->isafungus) || (mintOrganism->iscoral)) {
                        if ((o->healing > 0) && (o->plagueversion == 0)) {
                            if (org_use_energy(mintOrganism, P.mint_energy_consumption / 10)) {
                                o->seg_color[i] = CM_DARKFIRE;
                                o->useextraeffects = true;
                                neutralize = true;
                            }
                        } else {
                            if (org_use_energy(mintOrganism, P.mint_energy_consumption / 10)) {
                                o->seg_color[i] = CM_LIGHTBROWN;
                                neutralize = true;
                            }
                        }
                    }
                }
            }
            break;
        case TY_CORAL:
            if (o->isaplant) {
                if (org_use_energy(mintOrganism, P.mint_energy_consumption / 10)) {
                    o->seg_color[i] = CM_BROKEN;
                    neutralize = true;
                }
            } else {
                if (org_use_energy(mintOrganism, P.mint_energy_consumption / 10)) {
                    o->seg_color[i] = CM_LIGHTBROWN;
                    neutralize = true;
                    if ((o->gold > 0) && (!o->isaconsumer) && (!o->isafungus)) {
                        o->update_effects = 2;
                    }
                }
            }
            break;
        case TY_FALLOW:
            if (o->isaplant) {
                if (org_use_energy(mintOrganism, P.mint_energy_consumption / 10)) {
                    o->seg_color[i] = CM_BROKEN;
                    neutralize = true;
                    o->update_effects = 2;
                }
            } else {
                if ((o->isaconsumer) || (o->iscoral)) {
                    if (org_use_energy(mintOrganism, P.mint_energy_consumption / 10)) {
                        o->seg_color[i] = CM_LIGHTBROWN;
                        neutralize = true;
                        o->update_effects = 2;
                    }
                } else {
                    if ((o->antiviral == 0) || (mintOrganism->isaplant) || (mintOrganism->isaconsumer) || (mintOrganism->isafungus) || (mintOrganism->iscoral)) {
                        if (org_use_energy(mintOrganism, P.mint_energy_consumption / 10)) {
                            o->seg_color[i] = CM_LIGHTBROWN;
                            neutralize = true;
                            o->update_effects = 2;
                        }
                    }
                }
            }
            break;
        }
    }
    if (neutralize) {
        org_set_color(mintOrganism, CM_MINT);
        o->isinjured = true;
    }
}

/* Infects this organism with a genetic code.
 * Tells the world about this event.
 * Not currently used.
 * 
 * @param infectingCode  The genetic code that infects this organism.
 */
void org_infected_by_genetic_code(Organism *o, GeneticCode *infectingCode)
{
    o->infected_gc = infectingCode;
    o->saved_gc = NULL;
    world_organism_has_been_infected(o->world, o, NULL);
}

/* Infects this organism with the genetic code of another organism.
 * Tells the world about this event.
 * 
 * @param infectingOrganism  The organism that is infecting this one.
 */
void org_infected_by_organism(Organism *o, Organism *infectingOrganism)
{
    infectingOrganism->total_infected++;
    o->infected_gc = org_get_genetic_code(infectingOrganism);
    o->saved_gc = NULL;
    world_organism_has_been_infected(o->world, o, infectingOrganism);
    o->infected_reproduce_energy = org_get_reproduce_energy(infectingOrganism);
}

/* Infects this organism with the genetic code of a fruit.
 * Tells the world about this event.
 * 
 * @param infectingOrganism  The organism that is infecting this one.
 */
void org_infected_by_fruit(Organism *o, Organism *infectingOrganism)
{
    infectingOrganism->total_infected++;
    o->infected_gc = org_get_genetic_code(infectingOrganism);
    o->saved_gc = NULL;
    world_organism_has_been_infected(o->world, o, infectingOrganism);
    if (o->reproducelate == 0) {
        o->infected_reproduce_energy = j_d2i((o->reproduce_energy + (o->reproduce_energy * 0.048 * infectingOrganism->sporetime)));
    } else {
        o->infected_reproduce_energy = j_d2i(((40 + 3 * o->segments) + ((40 + 3 * o->segments) * 0.048 * infectingOrganism->sporetime)));
    }
}

/* Infects this organism with the genetic code of a fruit (weak).
 * Tells the world about this event.
 * 
 * @param infectingOrganism  The organism that is infecting this one.
 */
void org_infected_by_fruit_weak(Organism *o, Organism *infectingOrganism)
{
    infectingOrganism->total_infected++;
    o->infected_gc = org_get_genetic_code(infectingOrganism);
    o->saved_gc = NULL;
    world_organism_has_been_infected(o->world, o, infectingOrganism);
    if (o->reproducelate == 0) {
        o->infected_reproduce_energy = j_d2i((o->reproduce_energy * 0.048 * infectingOrganism->sporetime));
    } else {
        o->infected_reproduce_energy = j_d2i(((40 + 3 * o->segments) * 0.048 * infectingOrganism->sporetime));
    }
}

/* Calculates the resulting speeds after a collision between two organisms, following
 * physical rules.
 * 
 * @param org  The other organism in the collision.
 * @param p  Intersection point between the organisms.
 * @param l  Line that has collided. Of the two lines, this is the one that collided
 * on the center, not on the vertex.
 * @param thisOrganism  true if l is a line of this organism, false if l is a line of org.
 */
static void org_touch_move(Organism *o, Organism *org, JPoint p, JLine l, bool thisOrganism)
{
    // Distance vector between centers of mass and p
    double rapx = p.x - o->dcenter_x;
    double rapy = p.y - o->dcenter_y;
    double rbpx = p.x - org->dcenter_x;
    double rbpy = p.y - org->dcenter_y;
    // Speeds of point p in the body A and B, before collision.
    double vap1x = o->dx - o->dtheta * rapy + o->has_grown * rapx / 10.0;
    double vap1y = o->dy + o->dtheta * rapx + o->has_grown * rapy / 10.0;
    double vbp1x = org->dx - org->dtheta * rbpy;
    double vbp1y = org->dy + org->dtheta * rbpx;
    // Relative speeds between the two collision points.
    double vab1x = vap1x - vbp1x;
    double vab1y = vap1y - vbp1y;
    // Normal vector to the impact line
    //First: perpendicular vector to the line
    double nx = l.y1 - l.y2;
    double ny = l.x2 - l.x1;
    //Second: normalize, modulus 1
    double modn = sqrt(nx * nx + ny * ny);
    if (modn == 0) {
        modn = 0.000000000000001;
    }
    nx /= modn;
    ny /= modn;
    /* Third: of the two possible normal vectors we need the one that points to the
     * outside; we choose the one that its final point is the nearest to the center
     * of the other line.
     */
    if (thisOrganism) {
        if ((p.x + nx - org->dcenter_x) * (p.x + nx - org->dcenter_x) + (p.y + ny - org->dcenter_y) * (p.y + ny - org->dcenter_y) < (p.x - nx - org->dcenter_x) * (p.x - nx - org->dcenter_x) + (p.y - ny - org->dcenter_y) * (p.y - ny - org->dcenter_y)) {
            nx = -nx;
            ny = -ny;
        }
    } else {
        if ((p.x + nx - o->dcenter_x) * (p.x + nx - o->dcenter_x) + (p.y + ny - o->dcenter_y) * (p.y + ny - o->dcenter_y) > (p.x - nx - o->dcenter_x) * (p.x - nx - o->dcenter_x) + (p.y - ny - o->dcenter_y) * (p.y - ny - o->dcenter_y)) {
            nx = -nx;
            ny = -ny;
        }
    }
    // This is the j in the parallel axis theorem
    double j = (-(1 + P.elasticity) * (vab1x * nx + vab1y * ny)) / (1 / o->mass + 1 / org->mass + bio_pow(rapx * ny - rapy * nx, 2) / o->inertia + bio_pow(rbpx * ny - rbpy * nx, 2) / org->inertia);
    // Final speed
    o->dx = bio_between(o->dx + j * nx / o->mass, -P.max_vel, P.max_vel);
    o->dy = bio_between(o->dy + j * ny / o->mass, -P.max_vel, P.max_vel);
    org->dx = bio_between(org->dx - j * nx / org->mass, -P.max_vel, P.max_vel);
    org->dy = bio_between(org->dy - j * ny / org->mass, -P.max_vel, P.max_vel);
    o->dtheta = bio_between(o->dtheta + j * (rapx * ny - rapy * nx) / o->inertia, -P.max_rot, P.max_rot);
    org->dtheta = bio_between(org->dtheta - j * (rbpx * ny - rbpy * ny) / org->inertia, -P.max_rot, P.max_rot);
}

/* Checks if the organism is inside the world. If it is not, calculates its
 * speed after the collision with the world border.
 * This calculation should be updated to follow the parallel axis theorem, just
 * like the collision between two organisms.
 * 
 * @return  true if the organism is inside the world, false otherwise.
 */
static bool org_is_inside_world(Organism *o)
{
    // Check it is inside the world
    if (o->r.x <= 0 || o->r.y <= 0 || o->r.x + o->r.width >= world_get_width(o->world) || o->r.y + o->r.height >= world_get_height(o->world)) {
        if (o->r.height <= 1 && o->r.width <= 1 && o->alive && o->r.x == 0 && o->r.y == 0)
            org_die(o, NULL);
        // Adjust direction
        if (o->r.x <= 0 || o->r.x + o->r.width >= world_get_width(o->world))
            o->dx = -o->dx;
        if (o->r.y <= 0 || o->r.y + o->r.height >= world_get_height(o->world))
            o->dy = -o->dy;
        o->dtheta = 0;
        return false;
    }
    return true;
}

/* Moves the organism and rotates it.
 * 
 * @param offsetx  displacement on the x axis.
 * @param offsety  displacement on the y axis.
 * @param offsettheta  rotation degree.
 */
static void org_offset(Organism *o, double offsetx, double offsety, double offsettheta)
{
    o->dcenter_x += offsetx;
    o->dcenter_y += offsety;
    o->theta += offsettheta;
    o->center_x = j_d2i(o->dcenter_x);
    o->center_y = j_d2i(o->dcenter_y);
}

/* Stop reaction */
static void org_stopmoving(Organism *o)
{
    o->dx *= P.rubbing / 5;
    if (fabs(o->dx) < BIO_TOL)
        o->dx = 0;
    o->dy *= P.rubbing / 5;
    if (fabs(o->dy) < BIO_TOL)
        o->dy = 0;
    o->dtheta *= P.rubbing / 5;
    if (fabs(o->dtheta) < BIO_TOL)
        o->dtheta = 0;
}

/* Stop reaction for non moving organisms with eyes */
static void org_standstill(Organism *o)
{
    o->dx *= P.rubbing / 2.5;
    if (fabs(o->dx) < BIO_TOL)
        o->dx = 0;
    o->dy *= P.rubbing / 2.5;
    if (fabs(o->dy) < BIO_TOL)
        o->dy = 0;
    o->dtheta *= P.rubbing / 2.5;
    if (fabs(o->dtheta) < BIO_TOL)
        o->dtheta = 0;
}

/* Stand still for Ochre */
static void org_standochre(Organism *o)
{
    o->dx *= P.rubbing / 1.5;
    if (fabs(o->dx) < BIO_TOL)
        o->dx = 0;
    o->dy *= P.rubbing / 1.5;
    if (fabs(o->dy) < BIO_TOL)
        o->dy = 0;
    o->dtheta *= P.rubbing / 1.5;
    if (fabs(o->dtheta) < BIO_TOL)
        o->dtheta = 0;
}

/* Infectious segments are destroyed */
static void org_virusneutralized(Organism *o)
{
    for (int x = 0; x < o->segments; x++) {
        switch (org_get_type_color(o->seg_color[x])) {
        case TY_WHITE:
        case TY_PLAGUE:
            if (o->isaplant) {
                o->seg_color[x] = CM_BROKEN;
            } else {
                o->seg_color[x] = CM_LIGHTBROWN;
            }
            o->isinjured = true;
            __attribute__((fallthrough)); /* as in Color Mod */
        case TY_CORAL:
            if (o->isaplant) {
                o->seg_color[x] = CM_BROKEN;
            } else {
                o->seg_color[x] = CM_LIGHTBROWN;
                if ((o->gold > 0) && (!o->isaconsumer) && (!o->isafungus)) {
                    o->update_effects = 2;
                }
            }
            o->isinjured = true;
        }
    }
}

/* Lavender shield is used */
static void org_lavenderinhibition(Organism *o)
{
    o->lavender -= 185;
    if (o->lavender <= 0) {
        o->lavender = 0;
    } else {
        if (o->frames_color < 10) {
            org_set_color(o, CM_LIGHT_BLUE);
        }
    }
}

/* Lavender shield is used */
static void org_lavendershield(Organism *o)
{
    o->lavender -= 185;
    if (o->lavender <= 0) {
        o->lavender = 0;
    } else {
        org_set_color(o, CM_LIGHT_BLUE);
    }
}

/* Weak lavender shield is used */
static void org_weaklavendershield(Organism *o)
{
    o->lavender -= 2220;
    if (o->lavender <= 0) {
        o->lavender = 0;
    } else {
        org_set_color(o, CM_LIGHT_BLUE);
    }
}

/* Finds if two organism are touching and if so applies the effects of the
 * collision.
 * 
 * @param org  The organism to check for collisions.
 * @return  true if the two organisms are touching, false otherwise.
 */
bool org_contact(Organism *o, Organism *org)
{
    int i = 0;
    int j = 0;
    JLine line = (JLine){0};
    JLine bline = (JLine){0};
    // Check collisions for all segments
    if (!o->haseyes) {
        for (i = o->segments - 1; i >= 0; i--) {
            // Consider only segments with modulus greater than 1
            if (o->m[i] >= 1) {
                jline_set(&line, o->x1[i] + o->center_x, o->y1[i] + o->center_y, o->x2[i] + o->center_x, o->y2[i] + o->center_y);
                // First check if the line intersects the bounding box of the other organism
                if (jrect_intersects_line(&org->r, &line)) {
                    // Do the same for the other organism's segments.
                    for (j = org->segments - 1; j >= 0; j--) {
                        if (org->m[j] >= 1) {
                            jline_set(&bline, org->x1[j] + org->center_x, org->y1[j] + org->center_y, org->x2[j] + org->center_x, org->y2[j] + org->center_y);
                            if (jrect_intersects_line(&o->r, &bline) && jline_intersects_line(&line, &bline)) {
                                // Intersection point
                                JPoint intersec = jline_get_intersection(&line, &bline);
                                /* touchMove needs to know which is the line that collides from the middle (not
                                 * from a vertex). Try to guess it by finding the vertex nearest to the
                                 * intersection point.
                                 */
                                double dl1 = 0;
                                double dl2 = 0;
                                double dbl1 = 0;
                                double dbl2 = 0;
                                dl1 = jpoint_distance_sq(intersec, (JPoint){line.x1, line.y1});
                                dl2 = jpoint_distance_sq(intersec, (JPoint){line.x2, line.y2});
                                dbl1 = jpoint_distance_sq(intersec, (JPoint){bline.x1, bline.y1});
                                dbl2 = jpoint_distance_sq(intersec, (JPoint){bline.x2, bline.y2});
                                // Use this to send the best choice to touchMove
                                if (jmin(dl1, dl2) < jmin(dbl1, dbl2))
                                    org_touch_move(o, org, intersec, bline, false);
                                else
                                    org_touch_move(o, org, intersec, line, true);
                                // Find only one collision to speed up.
                                // If we found two intersecting segments, apply effects
                                if (o->usepretoucheffects) {
                                    if ((o->isspike) && ((o->seg_color[i] == CM_SPIKEPOINT) || (o->seg_color[i] == CM_SPIKE))) {
                                        if ((org->isspike) && ((org->seg_color[j] == CM_SPIKEPOINT) || (org->seg_color[j] == CM_SPIKE))) {
                                            if (jmin(dl1, dl2) < jmin(dbl1, dbl2)) {
                                                if (o->mphoto[i] != -1) {
                                                    o->seg_color[i] = CM_SPIKEPOINT;
                                                    o->mphoto[i] = -1;
                                                }
                                            } else {
                                                if (o->mphoto[i] != 0) {
                                                    o->seg_color[i] = CM_SPIKE;
                                                    o->mphoto[i] = 0;
                                                }
                                            }
                                        } else {
                                            if (jmin(dl1, dl2) <= jmin(dbl1, dbl2)) {
                                                if (o->mphoto[i] != -1) {
                                                    o->seg_color[i] = CM_SPIKEPOINT;
                                                    o->mphoto[i] = -1;
                                                }
                                            } else {
                                                if (o->mphoto[i] != 0) {
                                                    o->seg_color[i] = CM_SPIKE;
                                                    o->mphoto[i] = 0;
                                                }
                                            }
                                        }
                                    }
                                    if (o->segmentpretoucheffects > 0) {
                                        org_pretouch_effects(o, org, i, j);
                                    }
                                }
                                if (org->usepretoucheffects) {
                                    if ((org->isspike) && ((org->seg_color[j] == CM_SPIKEPOINT) || (org->seg_color[j] == CM_SPIKE))) {
                                        if ((o->isspike) && ((o->seg_color[i] == CM_SPIKEPOINT) || (o->seg_color[i] == CM_SPIKE))) {
                                            if (jmin(dbl1, dbl2) < jmin(dl1, dl2)) {
                                                if (org->mphoto[j] != -1) {
                                                    org->seg_color[j] = CM_SPIKEPOINT;
                                                    org->mphoto[j] = -1;
                                                }
                                            } else {
                                                if (org->mphoto[j] != 0) {
                                                    org->seg_color[j] = CM_SPIKE;
                                                    org->mphoto[j] = 0;
                                                }
                                            }
                                        } else {
                                            if (jmin(dbl1, dbl2) <= jmin(dl1, dl2)) {
                                                if (org->mphoto[j] != -1) {
                                                    org->seg_color[j] = CM_SPIKEPOINT;
                                                    org->mphoto[j] = -1;
                                                }
                                            } else {
                                                if (org->mphoto[j] != 0) {
                                                    org->seg_color[j] = CM_SPIKE;
                                                    org->mphoto[j] = 0;
                                                }
                                            }
                                        }
                                    }
                                    if (org->segmentpretoucheffects > 0) {
                                        org_pretouch_effects(org, o, j, i);
                                    }
                                }
                                if ((((o->parent_id == org->id || o->id == org->parent_id) && !o->generationbattle && !org->generationbattle) || (o->parent_id == org->parent_id && !o->siblingbattle && !org->siblingbattle && o->parent_id != -1) || (o->transfersenergy && org->transfersenergy && ((gc_get_peaceful(o->gc) && gc_get_peaceful(org->gc)) || (o->lengthfriend == org->lengthfriend && o->thetafriend == org->thetafriend && gc_get_social(o->gc) && gc_get_social(org->gc))))) && o->alive && org->alive) {
                                    if (o->canreact) {
                                        org_touchfriend_reaction(o, org, i, j);
                                    }
                                    if (o->usefriendeffects > 0) {
                                        org_touchfriend_effects(o, org, i, j);
                                    }
                                    if (org->canreact) {
                                        org_touchfriend_reaction(org, o, j, i);
                                    }
                                    if (org->usefriendeffects > 0) {
                                        org_touchfriend_effects(org, o, j, i);
                                    }
                                } else {
                                    if (o->canreact) {
                                        org_touch_reaction(o, org, i, j);
                                    }
                                    if (o->mphoto[i] == -1) {
                                        org_touch_effects1(o, org, i, j);
                                    } else if (o->mphoto[i] == -2) {
                                        org_touch_effects2(o, org, i, j);
                                    }
                                    if (org->canreact) {
                                        org_touch_reaction(org, o, j, i);
                                    }
                                    if (org->mphoto[j] == -1) {
                                        org_touch_effects1(org, o, j, i);
                                    } else if (org->mphoto[j] == -2) {
                                        org_touch_effects2(org, o, j, i);
                                    }
                                }
                                return true;
                            }
                        }
                    }
                }
            }
        }
    } else
        for (i = o->segments - 1; i >= 0; i--) {
            // Consider only segments with modulus greater than 1
            if (o->m[i] >= 1) {
                jline_set(&line, o->x1[i] + o->center_x, o->y1[i] + o->center_y, o->x2[i] + o->center_x, o->y2[i] + o->center_y);
                // First check if the line intersects the bounding box of the other organism
                if (jrect_intersects_line(&org->r, &line)) {
                    // Do the same for the other organism's segments.
                    for (j = org->segments - 1; j >= 0; j--) {
                        if (org->m[j] >= 1) {
                            jline_set(&bline, org->x1[j] + org->center_x, org->y1[j] + org->center_y, org->x2[j] + org->center_x, org->y2[j] + org->center_y);
                            if (jrect_intersects_line(&o->r, &bline) && jline_intersects_line(&line, &bline)) {
                                // Intersection point
                                JPoint intersec = jline_get_intersection(&line, &bline);
                                /* touchMove needs to know which is the line that collides from the middle (not
                                 * from a vertex). Try to guess it by finding the vertex nearest to the
                                 * intersection point.
                                 */
                                double dl1 = 0;
                                double dl2 = 0;
                                double dbl1 = 0;
                                double dbl2 = 0;
                                dl1 = jpoint_distance_sq(intersec, (JPoint){line.x1, line.y1});
                                dl2 = jpoint_distance_sq(intersec, (JPoint){line.x2, line.y2});
                                dbl1 = jpoint_distance_sq(intersec, (JPoint){bline.x1, bline.y1});
                                dbl2 = jpoint_distance_sq(intersec, (JPoint){bline.x2, bline.y2});
                                // Use this to send the best choice to touchMove
                                if (jmin(dl1, dl2) < jmin(dbl1, dbl2))
                                    org_touch_move(o, org, intersec, bline, false);
                                else
                                    org_touch_move(o, org, intersec, line, true);
                                // Find only one collision to speed up.
                                // If we found two intersecting segments, apply effects
                                if (o->usepretoucheffects) {
                                    if ((o->isspike) && ((o->seg_color[i] == CM_SPIKEPOINT) || (o->seg_color[i] == CM_SPIKE))) {
                                        if ((org->isspike) && ((org->seg_color[j] == CM_SPIKEPOINT) || (org->seg_color[j] == CM_SPIKE))) {
                                            if (jmin(dl1, dl2) < jmin(dbl1, dbl2)) {
                                                if (o->mphoto[i] != -1) {
                                                    o->seg_color[i] = CM_SPIKEPOINT;
                                                    o->mphoto[i] = -1;
                                                }
                                            } else {
                                                if (o->mphoto[i] != 0) {
                                                    o->seg_color[i] = CM_SPIKE;
                                                    o->mphoto[i] = 0;
                                                }
                                            }
                                        } else {
                                            if (jmin(dl1, dl2) <= jmin(dbl1, dbl2)) {
                                                if (o->mphoto[i] != -1) {
                                                    o->seg_color[i] = CM_SPIKEPOINT;
                                                    o->mphoto[i] = -1;
                                                }
                                            } else {
                                                if (o->mphoto[i] != 0) {
                                                    o->seg_color[i] = CM_SPIKE;
                                                    o->mphoto[i] = 0;
                                                }
                                            }
                                        }
                                    }
                                    if (o->segmentpretoucheffects > 0) {
                                        org_pretouch_effects(o, org, i, j);
                                    }
                                }
                                if (org->usepretoucheffects) {
                                    if ((org->isspike) && ((org->seg_color[j] == CM_SPIKEPOINT) || (org->seg_color[j] == CM_SPIKE))) {
                                        if ((o->isspike) && ((o->seg_color[i] == CM_SPIKEPOINT) || (o->seg_color[i] == CM_SPIKE))) {
                                            if (jmin(dbl1, dbl2) < jmin(dl1, dl2)) {
                                                if (org->mphoto[j] != -1) {
                                                    org->seg_color[j] = CM_SPIKEPOINT;
                                                    org->mphoto[j] = -1;
                                                }
                                            } else {
                                                if (org->mphoto[j] != 0) {
                                                    org->seg_color[j] = CM_SPIKE;
                                                    org->mphoto[j] = 0;
                                                }
                                            }
                                        } else {
                                            if (jmin(dbl1, dbl2) <= jmin(dl1, dl2)) {
                                                if (org->mphoto[j] != -1) {
                                                    org->seg_color[j] = CM_SPIKEPOINT;
                                                    org->mphoto[j] = -1;
                                                }
                                            } else {
                                                if (org->mphoto[j] != 0) {
                                                    org->seg_color[j] = CM_SPIKE;
                                                    org->mphoto[j] = 0;
                                                }
                                            }
                                        }
                                    }
                                    if (org->segmentpretoucheffects > 0) {
                                        org_pretouch_effects(org, o, j, i);
                                    }
                                }
                                if ((((o->parent_id == org->id || o->id == org->parent_id) && !o->generationbattle && !org->generationbattle) || (o->parent_id == org->parent_id && !o->siblingbattle && !org->siblingbattle && o->parent_id != -1) || (o->transfersenergy && org->transfersenergy && ((gc_get_peaceful(o->gc) && gc_get_peaceful(org->gc)) || (o->lengthfriend == org->lengthfriend && o->thetafriend == org->thetafriend && gc_get_social(o->gc) && gc_get_social(org->gc))))) && o->alive && org->alive) {
                                    if (o->canreact) {
                                        org_touchfriend_reaction(o, org, i, j);
                                    }
                                    if (o->usefriendeffects > 0) {
                                        org_touchfriend_effects(o, org, i, j);
                                    }
                                    if (org->canreact) {
                                        org_touchfriend_reaction(org, o, j, i);
                                    }
                                    if (org->usefriendeffects > 0) {
                                        org_touchfriend_effects(org, o, j, i);
                                    }
                                } else {
                                    if (o->canreact) {
                                        org_touch_reaction(o, org, i, j);
                                    }
                                    if (o->mphoto[i] == -1) {
                                        org_touch_effects1(o, org, i, j);
                                    } else if (o->mphoto[i] == -2) {
                                        org_touch_effects2(o, org, i, j);
                                    }
                                    if (org->canreact) {
                                        org_touch_reaction(org, o, j, i);
                                    }
                                    if (org->mphoto[j] == -1) {
                                        org_touch_effects1(org, o, j, i);
                                    } else if (org->mphoto[j] == -2) {
                                        org_touch_effects2(org, o, j, i);
                                    }
                                }
                                return true;
                            }
                        }
                    }
                }
            } else if (o->seg_color[i] == CM_EYE) {
                if (o->canmove) {
                    jline_set(&line, o->x1[i] + o->center_x, o->y1[i] + o->center_y, o->x2[i] + o->center_x, o->y2[i] + o->center_y);
                    // First check if the line intersects the bounding box of the other organism
                    if (jrect_intersects_line(&org->r, &line)) {
                        // Do the same for the other organism's segments.
                        for (j = 0; j < org->segments; j++) {
                            if (org->m[j] >= 1) {
                                jline_set(&bline, org->x1[j] + org->center_x, org->y1[j] + org->center_y, org->x2[j] + org->center_x, org->y2[j] + org->center_y);
                                if (jrect_intersects_line(&o->r, &bline) && jline_intersects_line(&line, &bline)) {
                                    // If we found two intersecting segments, apply effects
                                    if ((((o->parent_id == org->id || o->id == org->parent_id) && !o->generationbattle && !org->generationbattle) || (o->parent_id == org->parent_id && !o->siblingbattle && !org->siblingbattle && o->parent_id != -1) || (o->transfersenergy && org->transfersenergy && ((gc_get_peaceful(o->gc) && gc_get_peaceful(org->gc)) || (o->lengthfriend == org->lengthfriend && o->thetafriend == org->thetafriend && gc_get_social(o->gc) && gc_get_social(org->gc))))) && o->alive && org->alive) {
                                        org_touchfriend_reaction(o, org, i, j);
                                    } else {
                                        org_touch_reaction(o, org, i, j);
                                    }
                                }
                            }
                        }
                    }
                } else if (o->alive) {
                    jline_set(&line, o->x1[i] + o->center_x, o->y1[i] + o->center_y, o->x2[i] + o->center_x, o->y2[i] + o->center_y);
                    // First check if the line intersects the bounding box of the other organism
                    if (jrect_intersects_line(&org->r, &line)) {
                        // Do the same for the other organism's segments.
                        for (j = 0; j < org->segments; j++) {
                            if (org->m[j] >= 1) {
                                jline_set(&bline, org->x1[j] + org->center_x, org->y1[j] + org->center_y, org->x2[j] + org->center_x, org->y2[j] + org->center_y);
                                if (jrect_intersects_line(&o->r, &bline) && jline_intersects_line(&line, &bline)) {
                                    // If we found two intersecting segments, apply effects
                                    if ((((o->parent_id == org->id || o->id == org->parent_id) && !o->generationbattle && !org->generationbattle) || (o->parent_id == org->parent_id && !o->siblingbattle && !org->siblingbattle && o->parent_id != -1) || (o->transfersenergy && org->transfersenergy && ((gc_get_peaceful(o->gc) && gc_get_peaceful(org->gc)) || (o->lengthfriend == org->lengthfriend && o->thetafriend == org->thetafriend && gc_get_social(o->gc) && gc_get_social(org->gc))))) && o->alive && org->alive) {
                                        org_touchfriend_brake(o, org, i, j);
                                    } else {
                                        org_touch_brake(o, org, i, j);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    return false;
}

/* Applies forest photosynthesis, reactions and some other effects produced by two touching segments.
 * 
 * @param org  The organism which is touching.
 * @param seg  Index of this organism's segment.
 * @param oseg  Index of the other organism's segment.
 */
static void org_pretouch_effects(Organism *o, Organism *org, int seg, int oseg)
{
    if (o->segmentpretoucheffects == 2) {
        switch (org_get_type_color(o->seg_color[seg])) {
        case TY_PURPLE:
            // Purple segment: Get energy from CH4 in the atmosphere, boosted for enhanced organisms if touching brown
            switch (org_get_type_color(org->seg_color[oseg])) {
            case TY_BROWN:
                if ((o->isenhanced) && (!o->isakiller) && (!org->alive)) {
                    o->energy += world_methanotrophy(o->world, o->methanotrophy);
                }
                break;
            default:
                break;
            }
            break;
        case TY_DARKGRAY:
            // Darkgray segment: Get energy from CH4 in the atmosphere, boosted for purple organisms if touching brown
            switch (org_get_type_color(org->seg_color[oseg])) {
            case TY_BROWN:
                if ((o->methanotrophy > 0) && (!o->isakiller) && (!org->alive)) {
                    o->energy += world_methanotrophy(o->world, o->methanotrophy);
                }
                break;
            default:
                break;
            }
            break;
        case TY_DARK:
            // Dark segment: Mimic other segments
            switch (org_get_type_color(org->seg_color[oseg])) {
            case TY_DARK:
            case TY_GREEN:
            case TY_GRASS:
            case TY_FOREST:
            case TY_SPRING:
            case TY_SUMMER:
            case TY_WINTER:
            case TY_LIME:
            case TY_LEAF:
            case TY_C4:
            case TY_JADE:
            case TY_DARKJADE:
            case TY_DARKGREEN:
            case TY_PURPLE:
            case TY_SPORE:
            case TY_FRUIT:
            case TY_VISION:
            case TY_DARKFIRE:
            case TY_ICE:
            case TY_LIGHT_BLUE:
            case TY_LIGHTBROWN:
            case TY_GREENBROWN:
            case TY_BROKEN:
            case TY_DEADBARK:
            case TY_BROWN:
                break;
            default:
                if (gc_get_mimic_all(o->gc)) {
                    for (int x = 0; x < o->segments; x++) {
                        switch (org_get_type_color(o->seg_color[x])) {
                        case TY_DARK:
                            if (org->seg_color[oseg] == CM_BARK) {
                                if (o->isaplant) {
                                    o->seg_color[x] = CM_BARK;
                                } else {
                                    o->seg_color[x] = CM_OLDBARK;
                                }
                            } else {
                                if (org->seg_color[oseg] == CM_OLDBARK) {
                                    if (o->isaplant) {
                                        o->seg_color[x] = CM_BARK;
                                    } else {
                                        o->seg_color[x] = CM_OLDBARK;
                                    }
                                } else {
                                    o->seg_color[x] = org->seg_color[oseg];
                                }
                            }
                            o->segred_reaction[x] = org->segred_reaction[oseg];
                            o->seggreen_reaction[x] = org->seggreen_reaction[oseg];
                            o->segblue_reaction[x] = org->segblue_reaction[oseg];
                            o->segplague_reaction[x] = org->segplague_reaction[oseg];
                            o->segscourge_reaction[x] = org->segscourge_reaction[oseg];
                            o->segwhite_reaction[x] = org->segwhite_reaction[oseg];
                            o->seggray_reaction[x] = org->seggray_reaction[oseg];
                            o->segsilver_reaction[x] = org->segsilver_reaction[oseg];
                            o->segdefault_reaction[x] = org->segdefault_reaction[oseg];
                            o->segconsumer_reaction[x] = org->segconsumer_reaction[oseg];
                            o->segplant_reaction[x] = org->segplant_reaction[oseg];
                            o->segmagenta_reaction[x] = org->segmagenta_reaction[oseg];
                            o->segpink_reaction[x] = org->segpink_reaction[oseg];
                            o->segcoral_reaction[x] = org->segcoral_reaction[oseg];
                            o->segorange_reaction[x] = org->segorange_reaction[oseg];
                            o->segbark_reaction[x] = org->segbark_reaction[oseg];
                            o->segviolet_reaction[x] = org->segviolet_reaction[oseg];
                            o->segvirus_reaction[x] = org->segvirus_reaction[oseg];
                            o->segmaroon_reaction[x] = org->segmaroon_reaction[oseg];
                            o->segolive_reaction[x] = org->segolive_reaction[oseg];
                            o->segmint_reaction[x] = org->segmint_reaction[oseg];
                            o->segcream_reaction[x] = org->segcream_reaction[oseg];
                            o->segspike_reaction[x] = org->segspike_reaction[oseg];
                            o->segfallow_reaction[x] = org->segfallow_reaction[oseg];
                            o->seglightblue_reaction[x] = org->seglightblue_reaction[oseg];
                            o->segochre_reaction[x] = org->segochre_reaction[oseg];
                            o->segsky_reaction[x] = org->segsky_reaction[oseg];
                            o->seglilac_reaction[x] = org->seglilac_reaction[oseg];
                            o->segfire_reaction[x] = org->segfire_reaction[oseg];
                            o->seglightbrown_reaction[x] = org->seglightbrown_reaction[oseg];
                            o->seggreenbrown_reaction[x] = org->seggreenbrown_reaction[oseg];
                            o->segbrown_reaction[x] = org->segbrown_reaction[oseg];
                            o->segice_reaction[x] = org->segice_reaction[oseg];
                            o->segbroken_reaction[x] = org->segbroken_reaction[oseg];
                            o->segsick_reaction[x] = org->segsick_reaction[oseg];
                            o->segfriend_reaction[x] = org->segfriend_reaction[oseg];
                        }
                    }
                } else {
                    if (org->seg_color[oseg] == CM_BARK) {
                        if (o->isaplant) {
                            o->seg_color[seg] = CM_BARK;
                        } else {
                            o->seg_color[seg] = CM_OLDBARK;
                        }
                    } else {
                        if (org->seg_color[oseg] == CM_OLDBARK) {
                            if (o->isaplant) {
                                o->seg_color[seg] = CM_BARK;
                            } else {
                                o->seg_color[seg] = CM_OLDBARK;
                            }
                        } else {
                            o->seg_color[seg] = org->seg_color[oseg];
                        }
                    }
                    o->segred_reaction[seg] = org->segred_reaction[oseg];
                    o->seggreen_reaction[seg] = org->seggreen_reaction[oseg];
                    o->segblue_reaction[seg] = org->segblue_reaction[oseg];
                    o->segplague_reaction[seg] = org->segplague_reaction[oseg];
                    o->segscourge_reaction[seg] = org->segscourge_reaction[oseg];
                    o->segwhite_reaction[seg] = org->segwhite_reaction[oseg];
                    o->seggray_reaction[seg] = org->seggray_reaction[oseg];
                    o->segsilver_reaction[seg] = org->segsilver_reaction[oseg];
                    o->segdefault_reaction[seg] = org->segdefault_reaction[oseg];
                    o->segconsumer_reaction[seg] = org->segconsumer_reaction[oseg];
                    o->segplant_reaction[seg] = org->segplant_reaction[oseg];
                    o->segmagenta_reaction[seg] = org->segmagenta_reaction[oseg];
                    o->segpink_reaction[seg] = org->segpink_reaction[oseg];
                    o->segcoral_reaction[seg] = org->segcoral_reaction[oseg];
                    o->segorange_reaction[seg] = org->segorange_reaction[oseg];
                    o->segbark_reaction[seg] = org->segbark_reaction[oseg];
                    o->segviolet_reaction[seg] = org->segviolet_reaction[oseg];
                    o->segvirus_reaction[seg] = org->segvirus_reaction[oseg];
                    o->segmaroon_reaction[seg] = org->segmaroon_reaction[oseg];
                    o->segolive_reaction[seg] = org->segolive_reaction[oseg];
                    o->segmint_reaction[seg] = org->segmint_reaction[oseg];
                    o->segcream_reaction[seg] = org->segcream_reaction[oseg];
                    o->segspike_reaction[seg] = org->segspike_reaction[oseg];
                    o->segfallow_reaction[seg] = org->segfallow_reaction[oseg];
                    o->seglightblue_reaction[seg] = org->seglightblue_reaction[oseg];
                    o->segochre_reaction[seg] = org->segochre_reaction[oseg];
                    o->segsky_reaction[seg] = org->segsky_reaction[oseg];
                    o->seglilac_reaction[seg] = org->seglilac_reaction[oseg];
                    o->segfire_reaction[seg] = org->segfire_reaction[oseg];
                    o->seglightbrown_reaction[seg] = org->seglightbrown_reaction[oseg];
                    o->seggreenbrown_reaction[seg] = org->seggreenbrown_reaction[oseg];
                    o->segbrown_reaction[seg] = org->segbrown_reaction[oseg];
                    o->segice_reaction[seg] = org->segice_reaction[oseg];
                    o->segbroken_reaction[seg] = org->segbroken_reaction[oseg];
                    o->segsick_reaction[seg] = org->segsick_reaction[oseg];
                    o->segfriend_reaction[seg] = org->segfriend_reaction[oseg];
                }
                org_segments_create_effects(o);
            }
            break;
        }
    }
    if (o->forestphoto > 0) {
        if (org->photosynthesis > 0) {
            // Enhance photosynthesis in a colony
            if (o->mphoto[seg] <= 0) {
                if (org->mphoto[oseg] <= 0) {
                    if (o->colony_photosynthesis == 0) {
                        o->colony_photosynthesis += 0.31 * o->forestphoto;
                    } else {
                        o->colony_photosynthesis += 0.28 * o->forestphoto;
                    }
                } else {
                    switch (org_get_type_color(org->seg_color[oseg])) {
                    case TY_FOREST:
                        if (o->colony_photosynthesis == 0) {
                            o->colony_photosynthesis += 0.51 * o->forestphoto;
                        } else {
                            o->colony_photosynthesis += 0.48 * o->forestphoto;
                        }
                        break;
                    case TY_BARK:
                    case TY_OLDBARK:
                    case TY_PURPLE:
                    case TY_GREENBROWN:
                    case TY_DEADBARK:
                    case TY_ICE:
                        if (o->colony_photosynthesis == 0) {
                            o->colony_photosynthesis += 0.31 * o->forestphoto;
                        } else {
                            o->colony_photosynthesis += 0.28 * o->forestphoto;
                        }
                        break;
                    case TY_DARKGRAY:
                        break;
                    default:
                        if (o->colony_photosynthesis == 0) {
                            o->colony_photosynthesis += 0.41 * o->forestphoto;
                        } else {
                            o->colony_photosynthesis += 0.38 * o->forestphoto;
                        }
                        break;
                    }
                }
            } else {
                switch (org_get_type_color(o->seg_color[seg])) {
                case TY_FOREST:
                    if (org->mphoto[oseg] <= 0) {
                        if (o->colony_photosynthesis == 0) {
                            o->colony_photosynthesis += 0.51 * o->forestphoto;
                        } else {
                            o->colony_photosynthesis += 0.48 * o->forestphoto;
                        }
                    } else {
                        switch (org_get_type_color(org->seg_color[oseg])) {
                        case TY_FOREST:
                            if (o->colony_photosynthesis == 0) {
                                o->colony_photosynthesis += 1.01 * o->forestphoto;
                            } else {
                                o->colony_photosynthesis += 0.98 * o->forestphoto;
                            }
                            break;
                        case TY_BARK:
                        case TY_OLDBARK:
                            if (o->colony_photosynthesis == 0) {
                                o->colony_photosynthesis += 0.61 * o->forestphoto;
                            } else {
                                o->colony_photosynthesis += 0.58 * o->forestphoto;
                            }
                            break;
                        case TY_PURPLE:
                        case TY_GREENBROWN:
                        case TY_DEADBARK:
                        case TY_ICE:
                            if (o->colony_photosynthesis == 0) {
                                o->colony_photosynthesis += 0.51 * o->forestphoto;
                            } else {
                                o->colony_photosynthesis += 0.48 * o->forestphoto;
                            }
                            break;
                        case TY_DARKGRAY:
                            break;
                        default:
                            if (o->colony_photosynthesis == 0) {
                                o->colony_photosynthesis += 0.91 * o->forestphoto;
                            } else {
                                o->colony_photosynthesis += 0.88 * o->forestphoto;
                            }
                            break;
                        }
                    }
                    break;
                case TY_BARK:
                case TY_OLDBARK:
                    if (org->mphoto[oseg] <= 0) {
                        if (o->colony_photosynthesis == 0) {
                            o->colony_photosynthesis += 0.31 * o->forestphoto;
                        } else {
                            o->colony_photosynthesis += 0.28 * o->forestphoto;
                        }
                    } else {
                        switch (org_get_type_color(org->seg_color[oseg])) {
                        case TY_FOREST:
                            if (o->colony_photosynthesis == 0) {
                                o->colony_photosynthesis += 0.61 * o->forestphoto;
                            } else {
                                o->colony_photosynthesis += 0.58 * o->forestphoto;
                            }
                            break;
                        case TY_BARK:
                        case TY_OLDBARK:
                            if (o->colony_photosynthesis == 0) {
                                o->colony_photosynthesis += 0.41 * o->forestphoto;
                            } else {
                                o->colony_photosynthesis += 0.38 * o->forestphoto;
                            }
                            break;
                        case TY_PURPLE:
                        case TY_GREENBROWN:
                        case TY_DEADBARK:
                        case TY_ICE:
                            if (o->colony_photosynthesis == 0) {
                                o->colony_photosynthesis += 0.31 * o->forestphoto;
                            } else {
                                o->colony_photosynthesis += 0.28 * o->forestphoto;
                            }
                            break;
                        case TY_DARKGRAY:
                            break;
                        default:
                            if (o->colony_photosynthesis == 0) {
                                o->colony_photosynthesis += 0.51 * o->forestphoto;
                            } else {
                                o->colony_photosynthesis += 0.48 * o->forestphoto;
                            }
                            break;
                        }
                    }
                    break;
                case TY_PURPLE:
                case TY_GREENBROWN:
                case TY_DEADBARK:
                case TY_ICE:
                    if (org->mphoto[oseg] <= 0) {
                        if (o->colony_photosynthesis == 0) {
                            o->colony_photosynthesis += 0.31 * o->forestphoto;
                        } else {
                            o->colony_photosynthesis += 0.28 * o->forestphoto;
                        }
                    } else {
                        switch (org_get_type_color(org->seg_color[oseg])) {
                        case TY_FOREST:
                            if (o->colony_photosynthesis == 0) {
                                o->colony_photosynthesis += 0.51 * o->forestphoto;
                            } else {
                                o->colony_photosynthesis += 0.48 * o->forestphoto;
                            }
                            break;
                        case TY_BARK:
                        case TY_OLDBARK:
                        case TY_PURPLE:
                        case TY_GREENBROWN:
                        case TY_DEADBARK:
                        case TY_ICE:
                            if (o->colony_photosynthesis == 0) {
                                o->colony_photosynthesis += 0.31 * o->forestphoto;
                            } else {
                                o->colony_photosynthesis += 0.28 * o->forestphoto;
                            }
                            break;
                        case TY_DARKGRAY:
                            break;
                        default:
                            if (o->colony_photosynthesis == 0) {
                                o->colony_photosynthesis += 0.41 * o->forestphoto;
                            } else {
                                o->colony_photosynthesis += 0.38 * o->forestphoto;
                            }
                            break;
                        }
                    }
                    break;
                case TY_DARKGRAY:
                    if (!org->isenhanced) {
                        if ((!o->isaconsumer) && (!o->isakiller)) {
                            o->colony_photosynthesis += P.darkgray_energy_consumption * o->forestphoto;
                        } else {
                            o->colony_photosynthesis += 0.5 * P.darkgray_energy_consumption * o->forestphoto;
                        }
                    }
                    break;
                default:
                    if (org->mphoto[oseg] <= 0) {
                        if (o->colony_photosynthesis == 0) {
                            o->colony_photosynthesis += 0.41 * o->forestphoto;
                        } else {
                            o->colony_photosynthesis += 0.38 * o->forestphoto;
                        }
                    } else {
                        switch (org_get_type_color(org->seg_color[oseg])) {
                        case TY_FOREST:
                            if (o->colony_photosynthesis == 0) {
                                o->colony_photosynthesis += 0.91 * o->forestphoto;
                            } else {
                                o->colony_photosynthesis += 0.88 * o->forestphoto;
                            }
                            break;
                        case TY_BARK:
                        case TY_OLDBARK:
                            if (o->colony_photosynthesis == 0) {
                                o->colony_photosynthesis += 0.51 * o->forestphoto;
                            } else {
                                o->colony_photosynthesis += 0.48 * o->forestphoto;
                            }
                            break;
                        case TY_PURPLE:
                        case TY_GREENBROWN:
                        case TY_DEADBARK:
                        case TY_ICE:
                            if (o->colony_photosynthesis == 0) {
                                o->colony_photosynthesis += 0.41 * o->forestphoto;
                            } else {
                                o->colony_photosynthesis += 0.38 * o->forestphoto;
                            }
                            break;
                        case TY_DARKGRAY:
                            break;
                        default:
                            if (o->colony_photosynthesis == 0) {
                                o->colony_photosynthesis += 0.81 * o->forestphoto;
                            } else {
                                o->colony_photosynthesis += 0.78 * o->forestphoto;
                            }
                            break;
                        }
                    }
                    break;
                }
            }
        }
    } else if ((o->frames_color == 0) && (o->colony_photosynthesis > 0) && (!o->isaplant) && (org->isaplant) && (org->active)) {
        // photosynthesis is enhanced if an organism touches symbiotic rose organisms, if these are not a plant or consumer
        if (org->photosynthesis > 0) {
            double symbiontphotosynthesis = 0;
            if (org->transfersenergy) {
                if (org->forestphoto > 0) {
                    symbiontphotosynthesis = o->colony_photosynthesis * ((1.5 * org->photosynthesis) + (0.375 * org->forestphoto));
                } else {
                    symbiontphotosynthesis = o->colony_photosynthesis * (1.5 * org->photosynthesis);
                }
            } else {
                if (org->forestphoto > 0) {
                    symbiontphotosynthesis = o->colony_photosynthesis * (org->photosynthesis + (0.25 * org->forestphoto));
                } else {
                    symbiontphotosynthesis = o->colony_photosynthesis * org->photosynthesis;
                }
            }
            org->energy += world_photosynthesis(o->world, symbiontphotosynthesis);
            org_set_color(o, CM_ROSE);
        }
        if (org->methanotrophy > 0) {
            double symbiontmethanotrophy = 0;
            if (org->transfersenergy) {
                symbiontmethanotrophy = o->colony_photosynthesis * (0.1875 * org->methanotrophy);
            } else {
                symbiontmethanotrophy = o->colony_photosynthesis * (0.125 * org->methanotrophy);
            }
            org->energy += world_methanotrophy(o->world, symbiontmethanotrophy);
            org_set_color(o, CM_ROSE);
        }
    }
}

/* Applies the friendly effects produced by two touching segments.
 * 
 * @param org  The organism which is touching.
 * @param seg  Index of this organism's segment.
 * @param oseg  Index of the other organism's segment.
 */
static void org_touchfriend_effects(Organism *o, Organism *org, int seg, int oseg)
{
    if ((o->transfersenergy) && (o->growth_ratio == 1) && (o->energy > (org->energy + 2))) {
        if (o->altruist && org->altruist) {
            // Transfers energy
            double takenEnergy1 = bio_between(2 * P.organic_obtained_energy, 0, (o->energy - (org->energy + 2)));
            // The other organism will be shown in cyan
            org_set_color(org, JC_CYAN);
            // This organism will be shown in rose
            org_set_color(o, CM_ROSE);
            org->energy += takenEnergy1;
            o->energy -= takenEnergy1;
            if ((!org->active) && ((org->sporeversion == 3) || (org->sporeversion == 4)) && (org->energy > 0.5 * org_get_reproduce_energy(org))) {
                org->time_to_reproduce = 0;
            }
        } else {
            if ((gc_get_familial(o->gc)) && ((o->lengthfriend == org->lengthfriend) || (o->thetafriend == org->thetafriend) || (o->parent_id == org->id) || (o->id == org->parent_id) || (o->parent_id == org->parent_id && o->parent_id != -1))) {
                // Transfers energy
                double takenEnergy2 = bio_between(2 * P.organic_obtained_energy, 0, (o->energy - (org->energy + 2)));
                // The other organism will be shown in cyan
                org_set_color(org, JC_CYAN);
                // This organism will be shown in rose
                org_set_color(o, CM_ROSE);
                org->energy += takenEnergy2;
                o->energy -= takenEnergy2;
                if ((!org->active) && ((org->sporeversion == 3) || (org->sporeversion == 4)) && (org->energy > 0.5 * org_get_reproduce_energy(org))) {
                    org->time_to_reproduce = 0;
                }
            } else {
                if ((gc_get_peaceful(o->gc)) && (!org->altruist) && (org->transfersenergy) && (org->active) && (!org->isaplant) && (!org->isaconsumer) && (!org->isafungus)) {
                    if ((o->isaplant) || (o->isaconsumer) || (o->isafungus)) {
                        // Transfers energy
                        double takenEnergy3 = bio_between(2 * P.organic_obtained_energy, 0, (o->energy - (org->energy + 2)));
                        // The other organism will be shown in cyan
                        org_set_color(org, JC_CYAN);
                        // This organism will be shown in rose
                        org_set_color(o, CM_ROSE);
                        org->energy += takenEnergy3;
                        o->energy -= takenEnergy3;
                    }
                }
            }
        }
    }
    if (o->usefriendeffects == 2) {
        // Magenta segment: Heal all sick segments
        if ((org->isinjured) && ((o->seg_color[seg] == JC_MAGENTA) || ((o->transfersenergy) && (o->healing > 0)))) {
            switch (org_get_type_color(org->seg_color[oseg])) {
            case TY_BROWN:
                break;
            case TY_MAGENTA:;
                bool heal1 = false;
                for (int j = 0; j < org->segments; j++) {
                    if ((org->seg_color[j] == CM_LIGHTBROWN) || (org->seg_color[j] == CM_GREENBROWN) || (org->seg_color[j] == CM_BROKEN) || (org->seg_color[j] == CM_LIGHT_BLUE) || (org->seg_color[j] == CM_ICE) || (org->seg_color[j] == CM_DARKFIRE)) {
                        if ((org_use_energy(org, P.magenta_energy_consumption / 2)) && (org_use_energy(o, P.magenta_energy_consumption / 2))) {
                            org->seg_color[j] = gene_get_color(gc_get_gene(org->gc, j % gc_get_n_genes(org->gc)));
                            org_set_color(o, JC_MAGENTA);
                            heal1 = true;
                        }
                    }
                }
                if (heal1) {
                    org->isfrozen = false;
                    org->isinjured = false;
                    org->allfrozen = false;
                    org->update_effects = 2;
                    org_segments_restore_effects(org);
                }
                break;
            default:
                if (o->altruist && org->altruist) {
                    if (org->active) {
                        bool heal2 = false;
                        for (int j = 0; j < org->segments; j++) {
                            if ((org->seg_color[j] == CM_LIGHTBROWN) || (org->seg_color[j] == CM_GREENBROWN) || (org->seg_color[j] == CM_BROKEN) || (org->seg_color[j] == CM_LIGHT_BLUE) || (org->seg_color[j] == CM_ICE) || (org->seg_color[j] == CM_DARKFIRE)) {
                                if ((org_use_energy(org, P.magenta_energy_consumption / 2)) && (org_use_energy(o, P.magenta_energy_consumption / 2))) {
                                    org->seg_color[j] = gene_get_color(gc_get_gene(org->gc, j % gc_get_n_genes(org->gc)));
                                    org_set_color(o, JC_MAGENTA);
                                    heal2 = true;
                                }
                            }
                        }
                        if (heal2) {
                            org->isfrozen = false;
                            org->isinjured = false;
                            org->allfrozen = false;
                            org->update_effects = 2;
                            org_segments_restore_effects(org);
                        }
                    } else if (gc_get_modifiesspore(org->gc) >= 7) {
                        for (int a = 0; a < org->segments; a++) {
                            org->seg_color[a] = JC_BLUE;
                        }
                        org->isinjured = false;
                        org->isfrozen = false;
                        org->allfrozen = false;
                    }
                } else if ((gc_get_familial(o->gc)) && ((o->lengthfriend == org->lengthfriend) || (o->thetafriend == org->thetafriend) || (o->parent_id == org->id) || (o->id == org->parent_id) || (o->parent_id == org->parent_id && o->parent_id != -1))) {
                    if (org->active) {
                        bool heal3 = false;
                        for (int j = 0; j < org->segments; j++) {
                            if ((org->seg_color[j] == CM_LIGHTBROWN) || (org->seg_color[j] == CM_GREENBROWN) || (org->seg_color[j] == CM_BROKEN) || (org->seg_color[j] == CM_LIGHT_BLUE) || (org->seg_color[j] == CM_ICE) || (org->seg_color[j] == CM_DARKFIRE)) {
                                if ((org_use_energy(org, P.magenta_energy_consumption / 2)) && (org_use_energy(o, P.magenta_energy_consumption / 2))) {
                                    org->seg_color[j] = gene_get_color(gc_get_gene(org->gc, j % gc_get_n_genes(org->gc)));
                                    org_set_color(o, JC_MAGENTA);
                                    heal3 = true;
                                }
                            }
                        }
                        if (heal3) {
                            org->isfrozen = false;
                            org->isinjured = false;
                            org->allfrozen = false;
                            org->update_effects = 2;
                            org_segments_restore_effects(org);
                        }
                    } else if (gc_get_modifiesspore(org->gc) >= 7) {
                        for (int a = 0; a < org->segments; a++) {
                            org->seg_color[a] = JC_BLUE;
                        }
                        org->isinjured = false;
                        org->isfrozen = false;
                        org->allfrozen = false;
                    }
                }
            }
        }
        // Mint segment: Remove an infection
        if ((o->seg_color[seg] == CM_MINT) || ((o->transfersenergy) && (o->antiviral > 0))) {
            switch (org_get_type_color(org->seg_color[oseg])) {
            case TY_BROWN:
                break;
            case TY_MINT:
                if (org->infected_gc != NULL) {
                    if ((org_use_energy(org, P.mint_energy_consumption / 2)) && (org_use_energy(o, P.mint_energy_consumption / 2))) {
                        org->infected_gc = NULL;
                        org->saved_gc = NULL;
                        org_set_color(org, JC_CYAN);
                        org_set_color(o, CM_MINT);
                    }
                }
                if (org->reproducelate < 0) {
                    if ((org_use_energy(org, P.mint_energy_consumption / 2)) && (org_use_energy(o, P.mint_energy_consumption / 2))) {
                        org->reproducelate = 0;
                        org_set_color(org, JC_CYAN);
                        org_set_color(o, CM_MINT);
                    }
                }
                break;
            default:
                if (o->altruist && org->altruist) {
                    if ((org->infected_gc != o->gc) && (org->infected_gc != NULL)) {
                        if ((org_use_energy(org, P.mint_energy_consumption / 2)) && (org_use_energy(o, P.mint_energy_consumption / 2))) {
                            org->infected_gc = NULL;
                            org->saved_gc = NULL;
                            org_set_color(org, JC_CYAN);
                            org_set_color(o, CM_MINT);
                        }
                    }
                    if (org->reproducelate < 0) {
                        if ((org_use_energy(org, P.mint_energy_consumption / 2)) && (org_use_energy(o, P.mint_energy_consumption / 2))) {
                            org->reproducelate = 0;
                            org_set_color(org, JC_CYAN);
                            org_set_color(o, CM_MINT);
                        }
                    }
                } else if ((gc_get_familial(o->gc)) && ((o->lengthfriend == org->lengthfriend) || (o->thetafriend == org->thetafriend) || (o->parent_id == org->id) || (o->id == org->parent_id) || (o->parent_id == org->parent_id && o->parent_id != -1))) {
                    if ((org->infected_gc != o->gc) && (org->infected_gc != NULL)) {
                        if ((org_use_energy(org, P.mint_energy_consumption / 2)) && (org_use_energy(o, P.mint_energy_consumption / 2))) {
                            org->infected_gc = NULL;
                            org->saved_gc = NULL;
                            org_set_color(org, JC_CYAN);
                            org_set_color(o, CM_MINT);
                        }
                    }
                    if (org->reproducelate < 0) {
                        if ((org_use_energy(org, P.mint_energy_consumption / 2)) && (org_use_energy(o, P.mint_energy_consumption / 2))) {
                            org->reproducelate = 0;
                            org_set_color(org, JC_CYAN);
                            org_set_color(o, CM_MINT);
                        }
                    }
                }
            }
        }
        // Lavender segment: Immunity vs Plague, Coral and Fallow, it can immunize other organisms temporarily. 
        if ((o->seg_color[seg] == CM_LAVENDER) || ((o->transfersenergy) && (o->createlavender > 0))) {
            switch (org_get_type_color(org->seg_color[oseg])) {
            case TY_BROWN:
                break;
            case TY_LAVENDER:
                if (o->lavender >= P.lavender_shield) {
                    if (org->lavender < P.lavender_shield) {
                        org->lavender += o->createlavender;
                        if (org->lavender >= P.lavender_shield) {
                            org->lavender = P.lavender_shield;
                        }
                        org_set_color(org, JC_CYAN);
                        org_set_color(o, CM_LAVENDER);
                    }
                }
                if (o->time_to_reproduce <= o->time_to_reproduce_max) {
                    if (org->time_to_reproduce > org->time_to_reproduce_max) {
                        org->time_to_reproduce -= (o->createlavender / 20);
                        if (org->time_to_reproduce < org->time_to_reproduce_max) {
                            org->time_to_reproduce = org->time_to_reproduce_max;
                        }
                        org_set_color(org, JC_CYAN);
                        org_set_color(o, CM_LAVENDER);
                    }
                }
                break;
            default:
                if (o->altruist && org->altruist) {
                    if (o->lavender >= P.lavender_shield) {
                        if (org->lavender < P.lavender_shield) {
                            org->lavender += o->createlavender;
                            if (org->lavender >= P.lavender_shield) {
                                org->lavender = P.lavender_shield;
                            }
                            org_set_color(org, JC_CYAN);
                            org_set_color(o, CM_LAVENDER);
                        }
                    }
                    if (o->time_to_reproduce <= o->time_to_reproduce_max) {
                        if (org->time_to_reproduce > org->time_to_reproduce_max) {
                            org->time_to_reproduce -= (o->createlavender / 20);
                            if (org->time_to_reproduce < org->time_to_reproduce_max) {
                                org->time_to_reproduce = org->time_to_reproduce_max;
                            }
                            org_set_color(org, JC_CYAN);
                            org_set_color(o, CM_LAVENDER);
                        }
                    }
                } else if ((gc_get_familial(o->gc)) && ((o->lengthfriend == org->lengthfriend) || (o->thetafriend == org->thetafriend) || (o->parent_id == org->id) || (o->id == org->parent_id) || (o->parent_id == org->parent_id && o->parent_id != -1))) {
                    if (o->lavender >= P.lavender_shield) {
                        if (org->lavender < P.lavender_shield) {
                            org->lavender += o->createlavender;
                            if (org->lavender >= P.lavender_shield) {
                                org->lavender = P.lavender_shield;
                            }
                            org_set_color(org, JC_CYAN);
                            org_set_color(o, CM_LAVENDER);
                        }
                    }
                    if (o->time_to_reproduce <= o->time_to_reproduce_max) {
                        if (org->time_to_reproduce > org->time_to_reproduce_max) {
                            org->time_to_reproduce -= (o->createlavender / 20);
                            if (org->time_to_reproduce < org->time_to_reproduce_max) {
                                org->time_to_reproduce = org->time_to_reproduce_max;
                            }
                            org_set_color(org, JC_CYAN);
                            org_set_color(o, CM_LAVENDER);
                        }
                    }
                }
            }
        }
    }
}

/* Applies the friendly reaction effects produced by two touching segments.
 * 
 * @param org  The organism which is touching.
 * @param seg  Index of this organism's segment.
 * @param oseg  Index of the other organism's segment.
 */
static void org_touchfriend_reaction(Organism *o, Organism *org, int seg, int oseg)
{
    int i = 0;
    for (i = o->segments - 1; i >= 0; i--) {
        if ((o->seg_color[i] == CM_TEAL) || ((o->seg_color[seg] == CM_EYE) && (o->seg_color[i] == JC_CYAN))) {
            // Teal segment: React on other organisms
            switch (org_get_type_color(o->seg_color[seg])) {
            default:
                switch (org_get_type_color(org->seg_color[oseg])) {
                case TY_EYE:
                    break;
                default:
                    if (o->segfriend_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segfriend_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (3 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (3 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segfriend_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (3 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (3 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segfriend_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (3 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (3 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segfriend_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (3 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (3 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                }
                break;
            }
            break;
        }
    }
}

/* Applies the reaction effects produced by two touching segments.
 * 
 * @param org  The organism which is touching.
 * @param seg  Index of this organism's segment.
 * @param oseg  Index of the other organism's segment.
 */
static void org_touch_reaction(Organism *o, Organism *org, int seg, int oseg)
{
    int i = 0;
    for (i = o->segments - 1; i >= 0; i--) {
        if ((o->seg_color[i] == CM_TEAL) || ((o->seg_color[seg] == CM_EYE) && (o->seg_color[i] == JC_CYAN))) {
            // Teal segment: React on other organisms
            switch (org_get_type_color(o->seg_color[seg])) {
            case TY_SPIKE:
            case TY_DARKOLIVE:
            case TY_BROKEN:
            case TY_DARKFIRE:
            case TY_LIGHTBROWN:
            case TY_GREENBROWN:
            case TY_LIGHT_BLUE:
            case TY_DEADBARK:
            case TY_ICE:
                switch (org_get_type_color(org->seg_color[oseg])) {
                case TY_EYE:
                    break;
                default:
                    if (o->segsick_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segsick_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segsick_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segsick_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segsick_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                }
                break;
            default:
                switch (org_get_type_color(org->seg_color[oseg])) {
                case TY_EYE:
                    break;
                case TY_WHITE:
                case TY_PLAGUE:
                    if (org->isaplant) {
                        if (org->plagueversion > 0) {
                            if (o->segplague_reaction[seg] == 1) {
                                if (org_use_energy(o, P.teal_energy_consumption)) {
                                    org_stopmoving(o);
                                }
                            } else if (o->segplague_reaction[seg] == 2) {
                                if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                    o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                    o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                    o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                                }
                            } else if (o->segplague_reaction[seg] == 3) {
                                if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                    o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                    o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                    o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                                }
                            } else if (o->segplague_reaction[seg] == 4) {
                                if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                    o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                    o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                    o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                                }
                            } else if (o->segplague_reaction[seg] == 5) {
                                if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                    o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                    o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                    o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                                }
                            }
                        } else {
                            if (o->segwhite_reaction[seg] == 1) {
                                if (org_use_energy(o, P.teal_energy_consumption)) {
                                    org_stopmoving(o);
                                }
                            } else if (o->segwhite_reaction[seg] == 2) {
                                if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                    o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                    o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                    o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                                }
                            } else if (o->segwhite_reaction[seg] == 3) {
                                if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                    o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                    o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                    o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                                }
                            } else if (o->segwhite_reaction[seg] == 4) {
                                if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                    o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                    o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                    o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                                }
                            } else if (o->segwhite_reaction[seg] == 5) {
                                if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                    o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                    o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                    o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                                }
                            }
                        }
                    } else if ((org->plagueversion > 0) || (org->isaconsumer) || (org->isafungus) || (org->isauburn)) {
                        if (o->segscourge_reaction[seg] == 1) {
                            if (org_use_energy(o, P.teal_energy_consumption)) {
                                org_stopmoving(o);
                            }
                        } else if (o->segscourge_reaction[seg] == 2) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segscourge_reaction[seg] == 3) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segscourge_reaction[seg] == 4) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segscourge_reaction[seg] == 5) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        }
                    } else {
                        if (o->segvirus_reaction[seg] == 1) {
                            if (org_use_energy(o, P.teal_energy_consumption)) {
                                org_stopmoving(o);
                            }
                        } else if (o->segvirus_reaction[seg] == 2) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segvirus_reaction[seg] == 3) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segvirus_reaction[seg] == 4) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segvirus_reaction[seg] == 5) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        }
                    }
                    break;
                case TY_ORANGE:
                    if (o->segorange_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segorange_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segorange_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segorange_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segorange_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_FIRE:
                    if (o->segfire_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segfire_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segfire_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segfire_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segfire_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_RED:
                    if (o->segred_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segred_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segred_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segred_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segred_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_PINK:
                    if (o->segpink_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segpink_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segpink_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segpink_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segpink_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_MAROON:
                    if (o->segmaroon_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segmaroon_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segmaroon_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segmaroon_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segmaroon_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_CREAM:
                    if (o->segcream_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segcream_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segcream_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segcream_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segcream_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_BARK:
                case TY_OLDBARK:
                    if (o->segbark_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segbark_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segbark_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segbark_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segbark_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_GREEN:
                case TY_FOREST:
                case TY_SPRING:
                case TY_SUMMER:
                case TY_WINTER:
                case TY_LIME:
                case TY_LEAF:
                case TY_C4:
                case TY_JADE:
                case TY_DARKJADE:
                case TY_DARKGREEN:
                case TY_GRASS:
                case TY_PURPLE:
                    if (o->seggreen_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->seggreen_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->seggreen_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->seggreen_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->seggreen_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_BLUE:
                    if (o->segblue_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segblue_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segblue_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segblue_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segblue_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_OCHRE:
                    if (o->segochre_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segochre_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segochre_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segochre_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segochre_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_OLIVE:
                    if (o->segolive_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segolive_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segolive_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segolive_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segolive_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_SKY:
                case TY_DEEPSKY:
                    if (o->segsky_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segsky_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segsky_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segsky_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segsky_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_SILVER:
                    if (((org->total_kills > 0) && ((org->isaconsumer) || (org->isafungus))) || (org->isenhanced)) {
                        if (o->segsilver_reaction[seg] == 1) {
                            if (org_use_energy(o, P.teal_energy_consumption)) {
                                org_stopmoving(o);
                            }
                        } else if (o->segsilver_reaction[seg] == 2) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segsilver_reaction[seg] == 3) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segsilver_reaction[seg] == 4) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segsilver_reaction[seg] == 5) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        }
                    } else if (org->isaplant) {
                        if (o->segplant_reaction[seg] == 1) {
                            if (org_use_energy(o, P.teal_energy_consumption)) {
                                org_stopmoving(o);
                            }
                        } else if (o->segplant_reaction[seg] == 2) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segplant_reaction[seg] == 3) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segplant_reaction[seg] == 4) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segplant_reaction[seg] == 5) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        }
                    } else if ((org->isaconsumer) || (org->isafungus)) {
                        if (o->segconsumer_reaction[seg] == 1) {
                            if (org_use_energy(o, P.teal_energy_consumption)) {
                                org_stopmoving(o);
                            }
                        } else if (o->segconsumer_reaction[seg] == 2) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segconsumer_reaction[seg] == 3) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segconsumer_reaction[seg] == 4) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segconsumer_reaction[seg] == 5) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        }
                    } else {
                        if (o->segdefault_reaction[seg] == 1) {
                            if (org_use_energy(o, P.teal_energy_consumption)) {
                                org_stopmoving(o);
                            }
                        } else if (o->segdefault_reaction[seg] == 2) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segdefault_reaction[seg] == 3) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segdefault_reaction[seg] == 4) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segdefault_reaction[seg] == 5) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        }
                    }
                    break;
                case TY_FRUIT:
                    if (org->sporeversion == 2) {
                        if (o->segwhite_reaction[seg] == 1) {
                            if (org_use_energy(o, P.teal_energy_consumption)) {
                                org_stopmoving(o);
                            }
                        } else if (o->segwhite_reaction[seg] == 2) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segwhite_reaction[seg] == 3) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segwhite_reaction[seg] == 4) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segwhite_reaction[seg] == 5) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        }
                    } else {
                        if (o->segvirus_reaction[seg] == 1) {
                            if (org_use_energy(o, P.teal_energy_consumption)) {
                                org_stopmoving(o);
                            }
                        } else if (o->segvirus_reaction[seg] == 2) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segvirus_reaction[seg] == 3) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segvirus_reaction[seg] == 4) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segvirus_reaction[seg] == 5) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        }
                    }
                    break;
                case TY_CORAL:
                    if (o->segcoral_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segcoral_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segcoral_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segcoral_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segcoral_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_FALLOW:
                    if (o->segfallow_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segfallow_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segfallow_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segfallow_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segfallow_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_VIOLET:
                    if (o->segviolet_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segviolet_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segviolet_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segviolet_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segviolet_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_SPIKEPOINT:
                    if (o->segspike_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segspike_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segspike_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segspike_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segspike_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_SPIKE:
                    if (o->seg_color[seg] == CM_EYE) {
                        if (o->segspike_reaction[seg] == 1) {
                            if (org_use_energy(o, P.teal_energy_consumption)) {
                                org_stopmoving(o);
                            }
                        } else if (o->segspike_reaction[seg] == 2) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segspike_reaction[seg] == 3) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segspike_reaction[seg] == 4) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segspike_reaction[seg] == 5) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        }
                    } else if (org->isaplant) {
                        if (o->seggreen_reaction[seg] == 1) {
                            if (org_use_energy(o, P.teal_energy_consumption)) {
                                org_stopmoving(o);
                            }
                        } else if (o->seggreen_reaction[seg] == 2) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->seggreen_reaction[seg] == 3) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->seggreen_reaction[seg] == 4) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->seggreen_reaction[seg] == 5) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        }
                    } else if (org->isenhanced) {
                        if (o->segconsumer_reaction[seg] == 1) {
                            if (org_use_energy(o, P.teal_energy_consumption)) {
                                org_stopmoving(o);
                            }
                        } else if (o->segconsumer_reaction[seg] == 2) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segconsumer_reaction[seg] == 3) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segconsumer_reaction[seg] == 4) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segconsumer_reaction[seg] == 5) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        }
                    } else {
                        if (o->segblue_reaction[seg] == 1) {
                            if (org_use_energy(o, P.teal_energy_consumption)) {
                                org_stopmoving(o);
                            }
                        } else if (o->segblue_reaction[seg] == 2) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segblue_reaction[seg] == 3) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segblue_reaction[seg] == 4) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segblue_reaction[seg] == 5) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        }
                    }
                    break;
                case TY_LILAC:
                case TY_DARKLILAC:
                    if (o->seglilac_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->seglilac_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->seglilac_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->seglilac_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->seglilac_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_GRAY:
                    if (o->seggray_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->seggray_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->seggray_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->seggray_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->seggray_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_MINT:
                case TY_LAVENDER:
                    if (o->segmint_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segmint_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segmint_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segmint_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segmint_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_MAGENTA:
                case TY_ROSE:
                    if (o->segmagenta_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segmagenta_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segmagenta_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segmagenta_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segmagenta_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_LIGHTBROWN:
                case TY_DARKFIRE:
                    if (o->seglightbrown_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->seglightbrown_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->seglightbrown_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->seglightbrown_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->seglightbrown_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_GREENBROWN:
                    if (o->seggreenbrown_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->seggreenbrown_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->seggreenbrown_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->seggreenbrown_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->seggreenbrown_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_DARKOLIVE:
                case TY_LIGHT_BLUE:
                    if (o->seglightblue_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->seglightblue_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->seglightblue_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->seglightblue_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->seglightblue_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_ICE:
                case TY_DEADBARK:
                    if (o->segice_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segice_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segice_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segice_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segice_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_BROKEN:
                    if (o->segbroken_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segbroken_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segbroken_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segbroken_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segbroken_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                case TY_BROWN:
                    if (o->segbrown_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_stopmoving(o);
                        }
                    } else if (o->segbrown_reaction[seg] == 2) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segbrown_reaction[seg] == 3) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segbrown_reaction[seg] == 4) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    } else if (o->segbrown_reaction[seg] == 5) {
                        if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                            o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                            o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                        }
                    }
                    break;
                default:
                    if ((org->isaconsumer) || (org->isafungus)) {
                        if (o->segconsumer_reaction[seg] == 1) {
                            if (org_use_energy(o, P.teal_energy_consumption)) {
                                org_stopmoving(o);
                            }
                        } else if (o->segconsumer_reaction[seg] == 2) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segconsumer_reaction[seg] == 3) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segconsumer_reaction[seg] == 4) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segconsumer_reaction[seg] == 5) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        }
                    } else if (org->isaplant) {
                        if (o->segplant_reaction[seg] == 1) {
                            if (org_use_energy(o, P.teal_energy_consumption)) {
                                org_stopmoving(o);
                            }
                        } else if (o->segplant_reaction[seg] == 2) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segplant_reaction[seg] == 3) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segplant_reaction[seg] == 4) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segplant_reaction[seg] == 5) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        }
                    } else {
                        if (o->segdefault_reaction[seg] == 1) {
                            if (org_use_energy(o, P.teal_energy_consumption)) {
                                org_stopmoving(o);
                            }
                        } else if (o->segdefault_reaction[seg] == 2) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[seg] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[seg] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segdefault_reaction[seg] == 3) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x1[0] - o->x2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y1[0] - o->y2[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segdefault_reaction[seg] == 4) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((o->x2[seg] - o->x1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((o->y2[seg] - o->y1[seg]) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        } else if (o->segdefault_reaction[seg] == 5) {
                            if (((!o->candodge && jr_next_boolean(&g_rng)) || (o->candodge && jr_next_int(&g_rng, 10) > 2)) && (org_use_energy(o, P.teal_energy_consumption))) {
                                o->dx = bio_between(((org->center_x - o->center_x) * (o->m[i] * o->m[i]) + 12.0 * (o->x2[i] - o->x1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dy = bio_between(((org->center_y - o->center_y) * (o->m[i] * o->m[i]) + 12.0 * (o->y2[i] - o->y1[i])) / (1.5 * o->mass), -P.max_vel, P.max_vel);
                                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                            }
                        }
                    }
                    break;
                }
                break;
            }
            break;
        }
    }
}

/* Applies the friendly brake effects produced by an eye segment of a non moving organism touching another segment
 * 
 * @param org  The organism which is touching.
 * @param seg  Index of this organism's segment.
 * @param oseg  Index of the other organism's segment.
 */
static void org_touchfriend_brake(Organism *o, Organism *org, int seg, int oseg)
{
    switch (org_get_type_color(o->seg_color[seg])) {
    default:
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_EYE:
            break;
        default:
            if (o->segfriend_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        }
        break;
    }
}

/* Applies the brake effects produced by an eye segment of a non moving organism touching another segment
 * 
 * @param org  The organism which is touching.
 * @param seg  Index of this organism's segment.
 * @param oseg  Index of the other organism's segment.
 */
static void org_touch_brake(Organism *o, Organism *org, int seg, int oseg)
{
    switch (org_get_type_color(o->seg_color[seg])) {
    default:
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_EYE:
            break;
        case TY_WHITE:
        case TY_PLAGUE:
            if (org->isaplant) {
                if (org->plagueversion > 0) {
                    if (o->segplague_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_standstill(o);
                        }
                    }
                    break;
                } else {
                    if (o->segwhite_reaction[seg] == 1) {
                        if (org_use_energy(o, P.teal_energy_consumption)) {
                            org_standstill(o);
                        }
                    }
                    break;
                }
            } else if ((org->plagueversion > 0) || (org->isaconsumer) || (org->isafungus) || (org->isauburn)) {
                if (o->segscourge_reaction[seg] == 1) {
                    if (org_use_energy(o, P.teal_energy_consumption)) {
                        org_standstill(o);
                    }
                }
                break;
            } else {
                if (o->segvirus_reaction[seg] == 1) {
                    if (org_use_energy(o, P.teal_energy_consumption)) {
                        org_standstill(o);
                    }
                }
            }
            break;
        case TY_ORANGE:
            if (o->segorange_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_FIRE:
            if (o->segfire_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_RED:
            if (o->segred_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_PINK:
            if (o->segpink_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_MAROON:
            if (o->segmaroon_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_CREAM:
            if (o->segcream_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_BARK:
        case TY_OLDBARK:
            if (o->segbark_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_GREEN:
        case TY_FOREST:
        case TY_SPRING:
        case TY_SUMMER:
        case TY_WINTER:
        case TY_LIME:
        case TY_LEAF:
        case TY_C4:
        case TY_JADE:
        case TY_DARKJADE:
        case TY_DARKGREEN:
        case TY_GRASS:
        case TY_PURPLE:
            if (o->seggreen_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_BLUE:
            if (o->segblue_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_OCHRE:
            if (o->segochre_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_OLIVE:
            if (o->segolive_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_SKY:
        case TY_DEEPSKY:
            if (o->segsky_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_FRUIT:
            if (org->sporeversion == 2) {
                if (o->segwhite_reaction[seg] == 1) {
                    if (org_use_energy(o, P.teal_energy_consumption)) {
                        org_standstill(o);
                    }
                }
                break;
            } else {
                if (o->segvirus_reaction[seg] == 1) {
                    if (org_use_energy(o, P.teal_energy_consumption)) {
                        org_standstill(o);
                    }
                }
            }
            break;
        case TY_SILVER:
            if (((org->total_kills > 0) && ((org->isaconsumer) || (org->isafungus))) || (org->isenhanced)) {
                if (o->segsilver_reaction[seg] == 1) {
                    if (org_use_energy(o, P.teal_energy_consumption)) {
                        org_standstill(o);
                    }
                }
                break;
            } else if (org->isaplant) {
                if (o->segplant_reaction[seg] == 1) {
                    if (org_use_energy(o, P.teal_energy_consumption)) {
                        org_standstill(o);
                    }
                }
                break;
            } else if ((org->isaconsumer) || (org->isafungus)) {
                if (o->segconsumer_reaction[seg] == 1) {
                    if (org_use_energy(o, P.teal_energy_consumption)) {
                        org_standstill(o);
                    }
                }
                break;
            } else {
                if (o->segdefault_reaction[seg] == 1) {
                    if (org_use_energy(o, P.teal_energy_consumption)) {
                        org_standstill(o);
                    }
                }
            }
            break;
        case TY_CORAL:
            if (o->segcoral_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_FALLOW:
            if (o->segfallow_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_VIOLET:
            if (o->segviolet_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_SPIKEPOINT:
            if (o->segspike_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_SPIKE:
            if (o->seg_color[seg] == CM_EYE) {
                if (o->segspike_reaction[seg] == 1) {
                    if (org_use_energy(o, P.teal_energy_consumption)) {
                        org_standstill(o);
                    }
                }
                break;
            } else if (org->isaplant) {
                if (o->seggreen_reaction[seg] == 1) {
                    if (org_use_energy(o, P.teal_energy_consumption)) {
                        org_standstill(o);
                    }
                }
                break;
            } else if (org->isenhanced) {
                if (o->segconsumer_reaction[seg] == 1) {
                    if (org_use_energy(o, P.teal_energy_consumption)) {
                        org_standstill(o);
                    }
                }
                break;
            } else {
                if (o->segblue_reaction[seg] == 1) {
                    if (org_use_energy(o, P.teal_energy_consumption)) {
                        org_standstill(o);
                    }
                }
            }
            break;
        case TY_LILAC:
        case TY_DARKLILAC:
            if (o->seglilac_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_GRAY:
            if (o->seggray_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_MINT:
        case TY_LAVENDER:
            if (o->segmint_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_MAGENTA:
        case TY_ROSE:
            if (o->segmagenta_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_LIGHTBROWN:
        case TY_DARKFIRE:
            if (o->seglightbrown_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_GREENBROWN:
            if (o->seggreenbrown_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_DARKOLIVE:
        case TY_LIGHT_BLUE:
            if (o->seglightblue_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_ICE:
        case TY_DEADBARK:
            if (o->segice_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_BROKEN:
            if (o->segbroken_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        case TY_BROWN:
            if (o->segbrown_reaction[seg] == 1) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    org_standstill(o);
                }
            }
            break;
        default:
            if ((org->isaconsumer) || (org->isafungus)) {
                if (o->segconsumer_reaction[seg] == 1) {
                    if (org_use_energy(o, P.teal_energy_consumption)) {
                        org_standstill(o);
                    }
                }
                break;
            } else if (org->isaplant) {
                if (o->segplant_reaction[seg] == 1) {
                    if (org_use_energy(o, P.teal_energy_consumption)) {
                        org_standstill(o);
                    }
                }
                break;
            } else {
                if (o->segdefault_reaction[seg] == 1) {
                    if (org_use_energy(o, P.teal_energy_consumption)) {
                        org_standstill(o);
                    }
                }
            }
            break;
        }
        break;
    }
}

/* Applies the hostile effects produced by two touching segments.
 * Version for segments, that can drain energy
 * 
 * @param org  The organism which is touching.
 * @param seg  Index of this organism's segment.
 * @param oseg  Index of the other organism's segment.
 */
static void org_touch_effects1(Organism *o, Organism *org, int seg, int oseg)
{
    switch (org_get_type_color(o->seg_color[seg])) {
    case TY_ORANGE:;
        // Orange segment: try to get energy from the other organism
        double takenEnergyOrange = 0;
        if (org->hasdodged == false) {
            org->hasdodged = true;
        }
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_WHITE:
            if ((org->isaplant) || (org->isaconsumer) || (org->isafungus) || (org->plagueversion > 0) || (org->isauburn)) {
                if (org_use_energy(o, P.orange_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyOrange = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in orange
                    org_set_color(o, JC_ORANGE);
                }
            }
            break;
        case TY_VIOLET:
            if ((org->isaplant) || (org->isaconsumer) || (org->isafungus) || (org->plagueversion > 0) || (org->isauburn)) {
                if (org_use_energy(o, P.orange_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyOrange = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in orange
                    org_set_color(o, JC_ORANGE);
                }
            } else {
                if (org_use_energy(o, P.orange_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyOrange = bio_between((0.5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in green brown
                    org_set_color(org, CM_GREENBROWN);
                    // This organism will be shown in orange
                    org_set_color(o, JC_ORANGE);
                }
            }
            break;
        case TY_GRASS:
            if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                org_set_color(org, CM_TEAL);
                org_set_color(o, JC_ORANGE);
            } else {
                if (org_use_energy(o, P.orange_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyOrange = bio_between((0.5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in green brown
                    org_set_color(org, CM_GREENBROWN);
                    // This organism will be shown in orange
                    org_set_color(o, JC_ORANGE);
                }
            }
            break;
        case TY_GREEN:
        case TY_FOREST:
        case TY_SPRING:
        case TY_SUMMER:
        case TY_WINTER:
        case TY_LIME:
        case TY_LEAF:
        case TY_C4:
        case TY_JADE:
        case TY_DARKJADE:
        case TY_DARKGREEN:
        case TY_PURPLE:
        case TY_TEAL:
        case TY_CYAN:
        case TY_AUBURN:
        case TY_INDIGO:
        case TY_BLOND:
        case TY_FLOWER:
        case TY_DARKGRAY:
        case TY_GOLD:
        case TY_SPORE:
            if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                org_set_color(org, CM_TEAL);
                org_set_color(o, JC_ORANGE);
            } else {
                if (org_use_energy(o, P.orange_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyOrange = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in orange
                    org_set_color(o, JC_ORANGE);
                }
            }
            break;
        case TY_YELLOW:
            if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                org_set_color(org, CM_TEAL);
                org_set_color(o, JC_ORANGE);
            } else {
                if ((org->isaconsumer) || (org->isafungus) || (org->isaplant) || (org->transfersenergy)) {
                    if (org_use_energy(o, P.orange_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyOrange = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in orange
                        org_set_color(o, JC_ORANGE);
                    }
                } else {
                    if (org_use_energy(o, P.orange_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyOrange = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in orange
                        org_set_color(o, JC_ORANGE);
                    }
                }
            }
            break;
        case TY_ORANGE:
            if (org_use_energy(o, P.orange_energy_consumption)) {
                // Get energy depending on segment length
                takenEnergyOrange = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                // The other organism will be shown in orange
                org_set_color(org, JC_ORANGE);
                // This organism will be shown in orange
                org_set_color(o, JC_ORANGE);
            }
            break;
        case TY_FIRE:
            if (org_use_energy(o, P.orange_energy_consumption)) {
                // Get energy depending on segment length
                takenEnergyOrange = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                // The other organism will be shown in fire
                org_set_color(org, CM_FIRE);
                // This organism will be shown in orange
                org_set_color(o, JC_ORANGE);
            }
            break;
        case TY_SILVER:
            if (((org->total_kills > 0) && ((org->isaconsumer) || (org->isafungus))) || (org->isenhanced)) {
                if (org_use_energy(o, P.orange_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyOrange = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in gold
                    org_set_color(org, CM_GOLD);
                    // This organism will be shown in orange
                    org_set_color(o, JC_ORANGE);
                }
            } else {
                if (org_use_energy(o, P.orange_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyOrange = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in orange
                    org_set_color(o, JC_ORANGE);
                }
            }
            break;
        case TY_CORAL:
            if (org->isaconsumer) {
                if (org_use_energy(o, P.orange_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyOrange = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in orange
                    org_set_color(o, JC_ORANGE);
                }
            } else {
                if (org_use_energy(o, P.orange_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyOrange = bio_between((0.5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in green brown
                    org_set_color(org, CM_GREENBROWN);
                    // This organism will be shown in orange
                    org_set_color(o, JC_ORANGE);
                }
            }
            break;
        case TY_PLAGUE:
            if (org->isinfectious) {
                if (org_use_energy(o, P.orange_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyOrange = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in orange
                    org_set_color(o, JC_ORANGE);
                }
            } else {
                if (org_use_energy(o, P.orange_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyOrange = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in green brown
                    org_set_color(org, CM_GREENBROWN);
                    // This organism will be shown in orange
                    org_set_color(o, JC_ORANGE);
                }
            }
            break;
        case TY_SPIKE:
            if ((org->isaplant) || (org->isenhanced)) {
                if (org_use_energy(o, P.orange_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyOrange = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in orange
                    org_set_color(o, JC_ORANGE);
                }
            }
            break;
        case TY_FALLOW:
            if ((!org->isaplant) && (!org->isinfectious)) {
                if ((o->isenhanced) && (org->fallowversion >= 3)) {
                    if (org_use_energy(o, P.orange_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyOrange = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in orange
                        org_set_color(o, JC_ORANGE);
                    }
                } else {
                    if (org_use_energy(o, P.orange_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyOrange = bio_between((0.01 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in broken
                        org_set_color(org, CM_BROKEN);
                        // This organism will be shown in orange
                        org_set_color(o, JC_ORANGE);
                    }
                }
            } else {
                if (((o->isenhanced) && (org->fallowversion >= 3)) || ((org->isinfectious) && (org->fallowversion == 4))) {
                    if (org_use_energy(o, P.orange_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyOrange = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in orange
                        org_set_color(o, JC_ORANGE);
                    }
                } else {
                    if (org_use_energy(o, P.orange_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyOrange = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in orange
                        org_set_color(o, JC_ORANGE);
                    }
                }
            }
            break;
        case TY_BLUE:
            // If the other segment is blue, it acts as a shield
            if (org_use_energy(org, P.blue_energy_consumption)) {
                if (org->isenhanced) {
                    org_use_energy(o, bio_between((0.5 * sqrt(org->m[oseg])) * P.organic_obtained_energy, 0, o->energy));
                    org_set_color(o, CM_DARKLILAC);
                    if (o->energy < BIO_TOL) {
                        org_die(o, org);
                    }
                } else {
                    org_set_color(o, JC_ORANGE);
                }
                org_set_color(org, JC_BLUE);
            } else {
                // Doesn't have energy to use the shield
                if (org_use_energy(o, P.orange_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyOrange = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in orange
                    org_set_color(o, JC_ORANGE);
                }
            }
            break;
        case TY_LIGHT_BLUE:
            if (org_use_energy(org, P.blue_energy_consumption)) {
                org_set_color(o, JC_ORANGE);
                org_set_color(org, CM_LIGHT_BLUE);
            } else {
                // Doesn't have energy to use the shield
                if (org_use_energy(o, P.orange_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyOrange = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in orange
                    org_set_color(o, JC_ORANGE);
                }
            }
            break;
        case TY_SKY:
            if (org_use_energy(org, P.sky_energy_consumption)) {
                org->seg_color[oseg] = CM_DEEPSKY;
                org->useextraeffects = true;
                org_set_color(o, JC_ORANGE);
                org_set_color(org, CM_DEEPSKY);
            } else {
                // Doesn't have energy to use the shield
                if (org_use_energy(o, P.orange_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyOrange = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in orange
                    org_set_color(o, JC_ORANGE);
                }
            }
            break;
        case TY_DEEPSKY:
            org_set_color(o, JC_ORANGE);
            org_set_color(org, CM_DEEPSKY);
            break;
        case TY_MINT:
        case TY_LAVENDER:
        case TY_MAGENTA:
        case TY_ROSE:
            if (o->altruist) {
                break;
            } else {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, JC_ORANGE);
                } else {
                    if (org_use_energy(o, P.orange_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyOrange = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in orange
                        org_set_color(o, JC_ORANGE);
                    }
                }
            }
            break;
        case TY_FRUIT:
            if (org->sporeversion == 2) {
                if (org_use_energy(o, P.orange_energy_consumption)) {
                    if (org->isenhanced) {
                        // Get energy depending on segment length
                        takenEnergyOrange = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                    } else {
                        // Get energy depending on segment length
                        takenEnergyOrange = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                    }
                    if (o->infected_gc != org->gc) {
                        // Infectious fruit
                        org_infected_by_fruit(o, org);
                        org_set_color(o, CM_BLOND);
                    } else {
                        // This organism will be shown in orange
                        org_set_color(o, JC_ORANGE);
                    }
                }
            }
            break;
        case TY_ICE:
        case TY_DEADBARK:
            if ((o->skyversion > 0) || (o->isenhanced)) {
                if (org_use_energy(o, P.orange_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyOrange = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in orange
                    org_set_color(o, JC_ORANGE);
                }
            }
            break;
        case TY_BARK:
            org->seg_color[oseg] = CM_OLDBARK;
            if (org->active) {
                org->photosynthesis -= org->mphoto[oseg] * k_scale[org->growth_ratio - 1];
            }
            break;
        case TY_OLDBARK:
        case TY_RED:
        case TY_MAROON:
        case TY_CREAM:
        case TY_OCHRE:
        case TY_OLIVE:
        case TY_DARKOLIVE:
        case TY_SPIKEPOINT:
        case TY_BROWN:
            break;
        default:
            if (org_use_energy(o, P.orange_energy_consumption)) {
                // Get energy depending on segment length
                takenEnergyOrange = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                // The other organism will be shown in yellow
                org_set_color(org, JC_YELLOW);
                // This organism will be shown in orange
                org_set_color(o, JC_ORANGE);
            }
        }
        // energy interchange
        if (takenEnergyOrange > 0) {
            org->energy -= takenEnergyOrange;
            o->energy += takenEnergyOrange;
            takenEnergyOrange = takenEnergyOrange * P.organic_subs_produced;
            org_use_energy(o, takenEnergyOrange);
        }
        break;
    case TY_FIRE:;
        // Fire segment: omnivore between red and orange
        double takenEnergyFire = 0;
        if (org->hasdodged == false) {
            org->hasdodged = true;
        }
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_WHITE:
            if ((org->isaconsumer) || ((o->skyversion > 0) && (org->isfrozen) && (org->isaplant))) {
                if (org_use_energy(o, P.fire_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyFire = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in fire
                    org_set_color(o, CM_FIRE);
                }
            } else {
                if (org_use_energy(o, P.fire_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyFire = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in dark fire
                    org_set_color(o, CM_DARKFIRE);
                }
            }
            break;
        case TY_GRASS:
            if ((org->isaconsumer) || ((o->skyversion > 0) && (org->isfrozen) && (org->isaplant))) {
                if (org_use_energy(o, P.fire_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyFire = bio_between((0.5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in green brown
                    org_set_color(org, CM_GREENBROWN);
                    // This organism will be shown in fire
                    org_set_color(o, CM_FIRE);
                }
            } else {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_DARKFIRE);
                } else {
                    if (org_use_energy(o, P.fire_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyFire = bio_between((0.125 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in dark fire
                        org_set_color(o, CM_DARKFIRE);
                    }
                }
            }
            break;
        case TY_GREEN:
        case TY_FOREST:
        case TY_SPRING:
        case TY_SUMMER:
        case TY_WINTER:
        case TY_LIME:
        case TY_LEAF:
        case TY_C4:
        case TY_JADE:
        case TY_DARKJADE:
        case TY_DARKGREEN:
        case TY_PURPLE:
        case TY_TEAL:
        case TY_CYAN:
        case TY_AUBURN:
        case TY_INDIGO:
        case TY_BLOND:
        case TY_FLOWER:
        case TY_DARKGRAY:
        case TY_GOLD:
        case TY_SPORE:
            if ((org->isaconsumer) || ((o->skyversion > 0) && (org->isfrozen) && (org->isaplant))) {
                if (org_use_energy(o, P.fire_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyFire = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in fire
                    org_set_color(o, CM_FIRE);
                }
            } else {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_DARKFIRE);
                } else {
                    if (org_use_energy(o, P.fire_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyFire = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in dark fire
                        org_set_color(o, CM_DARKFIRE);
                    }
                }
            }
            break;
        case TY_YELLOW:
            if ((org->isaconsumer) || ((o->skyversion > 0) && (org->isfrozen) && (org->isaplant))) {
                if (org_use_energy(o, P.fire_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyFire = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in fire
                    org_set_color(o, CM_FIRE);
                }
            } else {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_DARKFIRE);
                } else {
                    if ((org->isafungus) || (org->isaplant) || (org->transfersenergy)) {
                        if (org_use_energy(o, P.fire_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergyFire = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in yellow
                            org_set_color(org, JC_YELLOW);
                            // This organism will be shown in dark fire
                            org_set_color(o, CM_DARKFIRE);
                        }
                    } else {
                        if (org_use_energy(o, P.fire_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergyFire = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in green brown
                            org_set_color(org, CM_GREENBROWN);
                            // This organism will be shown in dark fire
                            org_set_color(o, CM_DARKFIRE);
                        }
                    }
                }
            }
            break;
        case TY_RED:
            if (org_use_energy(o, P.fire_energy_consumption)) {
                // Get energy depending on segment length
                takenEnergyFire = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                // The other organism will be shown in red
                org_set_color(org, JC_RED);
                // This organism will be shown in fire
                org_set_color(o, CM_FIRE);
            }
            break;
        case TY_FIRE:
            if (org_use_energy(o, P.fire_energy_consumption)) {
                // Get energy depending on segment length
                takenEnergyFire = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                // The other organism will be shown in fire
                org_set_color(org, CM_FIRE);
                // This organism will be shown in fire
                org_set_color(o, CM_FIRE);
            }
            break;
        case TY_ORANGE:
            if (org_use_energy(o, P.fire_energy_consumption)) {
                // Get energy depending on segment length
                takenEnergyFire = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                // The other organism will be shown in orange
                org_set_color(org, JC_ORANGE);
                // This organism will be shown in fire
                org_set_color(o, CM_FIRE);
            }
            break;
        case TY_SPIKE:
            if ((org->isaplant) || (org->isenhanced)) {
                if ((org->isaconsumer) || ((o->skyversion > 0) && (org->isfrozen) && (org->isaplant))) {
                    if (org_use_energy(o, P.fire_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyFire = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in fire
                        org_set_color(o, CM_FIRE);
                    }
                } else {
                    if (org_use_energy(o, P.fire_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyFire = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in dark fire
                        org_set_color(o, CM_DARKFIRE);
                    }
                }
            }
            break;
        case TY_SILVER:
            if (((org->total_kills > 0) && ((org->isaconsumer) || (org->isafungus))) || (org->isenhanced)) {
                if (org_use_energy(o, P.fire_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyFire = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in gold
                    org_set_color(org, CM_GOLD);
                    // This organism will be shown in fire
                    org_set_color(o, CM_FIRE);
                }
            } else if ((org->isaconsumer) || ((o->skyversion > 0) && (org->isfrozen) && (org->isaplant))) {
                if (org_use_energy(o, P.fire_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyFire = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in fire
                    org_set_color(o, CM_FIRE);
                }
            } else if (org_use_energy(o, P.fire_energy_consumption)) {
                // Get energy depending on segment length
                takenEnergyFire = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                // The other organism will be shown in yellow
                org_set_color(org, JC_YELLOW);
                // This organism will be shown in dark fire
                org_set_color(o, CM_DARKFIRE);
            }
            break;
        case TY_PINK:
            if ((org->isgray) && (gc_get_modifiespink(org->gc))) {
                if (org_use_energy(o, P.fire_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyFire = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in fire
                    org_set_color(o, CM_FIRE);
                }
            } else {
                if (org_use_energy(o, P.fire_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyFire = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in dark fire
                    org_set_color(o, CM_DARKFIRE);
                }
            }
            break;
        case TY_MAROON:
            if (org_use_energy(o, P.fire_energy_consumption)) {
                // Get energy depending on segment length
                takenEnergyFire = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                // The other organism will be shown in yellow
                org_set_color(org, JC_YELLOW);
                // This organism will be shown in dark fire
                org_set_color(o, CM_DARKFIRE);
            }
            break;
        case TY_CREAM:
            if ((org->isenhanced) && (gc_get_modifiescream(org->gc) == 2)) {
                if (org_use_energy(o, P.fire_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyFire = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in green brown
                    org_set_color(org, CM_GREENBROWN);
                    // This organism will be shown in dark fire
                    org_set_color(o, CM_DARKFIRE);
                }
            } else {
                if (org_use_energy(o, P.fire_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyFire = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in dark fire
                    org_set_color(o, CM_DARKFIRE);
                }
            }
            break;
        case TY_FALLOW:
            if ((!org->isaplant) && (!org->isinfectious)) {
                if (org_use_energy(o, P.fire_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyFire = bio_between((0.01 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in broken
                    org_set_color(org, CM_BROKEN);
                    // This organism will be shown in dark fire
                    org_set_color(o, CM_DARKFIRE);
                }
            } else {
                if ((org->isinfectious) && (org->fallowversion == 4)) {
                    if ((org->isaconsumer) || ((o->skyversion > 0) && (org->isfrozen) && (org->isaplant))) {
                        if (org_use_energy(o, P.fire_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergyFire = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in yellow
                            org_set_color(org, JC_YELLOW);
                            // This organism will be shown in fire
                            org_set_color(o, CM_FIRE);
                        }
                    } else {
                        if (org_use_energy(o, P.fire_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergyFire = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in yellow
                            org_set_color(org, JC_YELLOW);
                            // This organism will be shown in dark fire
                            org_set_color(o, CM_DARKFIRE);
                        }
                    }
                } else {
                    if (org_use_energy(o, P.fire_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyFire = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in dark fire
                        org_set_color(o, CM_DARKFIRE);
                    }
                }
            }
            break;
        case TY_PLAGUE:
            if (org->isinfectious) {
                if ((org->isaconsumer) || ((o->skyversion > 0) && (org->isfrozen) && (org->isaplant))) {
                    if (org_use_energy(o, P.fire_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyFire = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in fire
                        org_set_color(o, CM_FIRE);
                    }
                } else {
                    if (org_use_energy(o, P.fire_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyFire = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in dark fire
                        org_set_color(o, CM_DARKFIRE);
                    }
                }
            } else {
                if (org_use_energy(o, P.fire_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyFire = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in green brown
                    org_set_color(org, CM_GREENBROWN);
                    // This organism will be shown in dark fire
                    org_set_color(o, CM_DARKFIRE);
                }
            }
            break;
        case TY_BLUE:
            // If the other segment is blue, it acts as a shield
            if (org_use_energy(org, P.blue_energy_consumption)) {
                if (org->isenhanced) {
                    org_use_energy(o, bio_between((0.5 * sqrt(org->m[oseg])) * P.organic_obtained_energy, 0, o->energy));
                    org_set_color(o, CM_DARKLILAC);
                    if (o->energy < BIO_TOL) {
                        org_die(o, org);
                    }
                } else {
                    org_set_color(o, CM_DARKFIRE);
                }
                org_set_color(org, JC_BLUE);
            } else {
                // Doesn't have energy to use the shield
                if (org_use_energy(o, P.fire_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyFire = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in dark fire
                    org_set_color(o, CM_DARKFIRE);
                }
            }
            break;
        case TY_LIGHT_BLUE:
            if (org_use_energy(org, P.blue_energy_consumption)) {
                org_set_color(o, CM_DARKFIRE);
                org_set_color(org, CM_LIGHT_BLUE);
            } else {
                // Doesn't have energy to use the shield
                if (org_use_energy(o, P.fire_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyFire = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in dark fire
                    org_set_color(o, CM_DARKFIRE);
                }
            }
            break;
        case TY_SKY:
            if ((!o->isenhanced) || (org->skyversion == 2)) {
                if (org_use_energy(org, P.sky_energy_consumption)) {
                    org->seg_color[oseg] = CM_DEEPSKY;
                    org->useextraeffects = true;
                    org_set_color(o, CM_DARKFIRE);
                    org_set_color(org, CM_DEEPSKY);
                } else {
                    // Doesn't have energy to use the shield
                    if (org_use_energy(o, P.fire_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyFire = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in dark fire
                        org_set_color(o, CM_DARKFIRE);
                    }
                }
            }
            break;
        case TY_DEEPSKY:
            if ((!o->isenhanced) || (org->skyversion == 2)) {
                org_set_color(o, CM_DARKFIRE);
                org_set_color(org, CM_DEEPSKY);
            }
            break;
        case TY_VIOLET:
            if ((org->isaconsumer) || ((o->skyversion > 0) && (org->isfrozen) && (org->isaplant))) {
                if (org_use_energy(o, P.fire_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyFire = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in fire
                    org_set_color(o, CM_FIRE);
                }
            } else {
                if (org->isafungus) {
                    if (org_use_energy(o, P.fire_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyFire = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in dark fire
                        org_set_color(o, CM_DARKFIRE);
                    }
                } else {
                    if (!o->isenhanced) {
                        if ((org->isaplant) || (org->plagueversion > 0) || (org->isauburn)) {
                            if ((!org->isenhanced) || (org->isinfectious)) {
                                if (org_use_energy(o, P.fire_energy_consumption)) {
                                    // Get energy depending on segment length
                                    takenEnergyFire = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                                    // The other organism will be shown in yellow
                                    org_set_color(org, JC_YELLOW);
                                    // This organism will be shown in dark fire
                                    org_set_color(o, CM_DARKFIRE);
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_FRUIT:
            if (org->sporeversion == 2) {
                if (org_use_energy(o, P.fire_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyFire = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in green brown
                    org_set_color(org, CM_GREENBROWN);
                    if (o->infected_gc != org->gc) {
                        // Infectious fruit
                        org_infected_by_fruit(o, org);
                        org_set_color(o, CM_BLOND);
                    } else {
                        // This organism will be shown in dark fire
                        org_set_color(o, CM_DARKFIRE);
                    }
                }
            } else {
                if (org_use_energy(o, P.fire_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyFire = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in dark fire
                    org_set_color(o, CM_DARKFIRE);
                }
            }
            break;
        case TY_ICE:
        case TY_DEADBARK:
            if (o->skyversion > 0) {
                if ((org->isaconsumer) || (org->isaplant)) {
                    if (org_use_energy(o, P.fire_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyFire = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in fire
                        org_set_color(o, CM_FIRE);
                    }
                } else {
                    if (org_use_energy(o, P.fire_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyFire = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in dark fire
                        org_set_color(o, CM_DARKFIRE);
                    }
                }
            }
            break;
        case TY_BARK:
            org->seg_color[oseg] = CM_OLDBARK;
            if (org->active) {
                org->photosynthesis -= org->mphoto[oseg] * k_scale[org->growth_ratio - 1];
            }
            break;
        case TY_OLDBARK:
        case TY_OCHRE:
        case TY_OLIVE:
        case TY_DARKOLIVE:
        case TY_SPIKEPOINT:
            break;
        case TY_LIGHTBROWN:
            if (org_use_energy(o, P.fire_energy_consumption)) {
                // Get energy depending on segment length
                takenEnergyFire = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                // The other organism will be shown in yellow
                org_set_color(org, JC_YELLOW);
                // This organism will be shown in fire
                org_set_color(o, CM_FIRE);
            }
            break;
        case TY_BROWN:
            if (o->isenhanced) {
                if (!org->alive) {
                    if (org_use_energy(o, P.fire_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyFire = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in fire
                        org_set_color(o, CM_FIRE);
                    }
                } else {
                    if (org_use_energy(o, P.fire_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyFire = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in dark fire
                        org_set_color(o, CM_DARKFIRE);
                        if (!org->isenhanced) {
                            org->isinjured = true;
                        }
                    }
                }
            }
            break;
        case TY_GREENBROWN:
            if (o->isenhanced) {
                if ((org->isaconsumer) || ((o->skyversion > 0) && (org->isfrozen) && (org->isaplant))) {
                    if (org_use_energy(o, P.fire_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyFire = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in fire
                        org_set_color(o, CM_FIRE);
                    }
                } else {
                    if (org_use_energy(o, P.fire_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyFire = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in dark fire
                        org_set_color(o, CM_DARKFIRE);
                    }
                }
            }
            break;
        case TY_MINT:
        case TY_LAVENDER:
        case TY_MAGENTA:
        case TY_ROSE:
            if (o->altruist) {
                break;
            } else {
                if ((org->isaconsumer) || ((o->skyversion > 0) && (org->isfrozen) && (org->isaplant))) {
                    if (org_use_energy(o, P.fire_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyFire = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in fire
                        org_set_color(o, CM_FIRE);
                    }
                } else {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, CM_DARKFIRE);
                    } else {
                        if (org_use_energy(o, P.fire_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergyFire = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in yellow
                            org_set_color(org, JC_YELLOW);
                            // This organism will be shown in dark fire
                            org_set_color(o, CM_DARKFIRE);
                        }
                    }
                }
            }
            break;
        default:
            if ((org->isaconsumer) || ((o->skyversion > 0) && (org->isfrozen) && (org->isaplant))) {
                if (org_use_energy(o, P.fire_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyFire = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in fire
                    org_set_color(o, CM_FIRE);
                }
            } else {
                if (org_use_energy(o, P.fire_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyFire = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in dark fire
                    org_set_color(o, CM_DARKFIRE);
                }
            }
        }
        // energy interchange
        if (takenEnergyFire > 0) {
            org->energy -= takenEnergyFire;
            o->energy += takenEnergyFire;
            takenEnergyFire = takenEnergyFire * P.organic_subs_produced;
            org_use_energy(o, takenEnergyFire);
        }
        break;
    case TY_RED:;
        // Red segment: try to get energy from other consumers
        double takenEnergyRed = 0;
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_RED:
            if (org_use_energy(o, P.red_energy_consumption)) {
                // Get energy depending on segment length
                takenEnergyRed = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                // The other organism will be shown in red
                org_set_color(org, JC_RED);
                // This organism will be shown in red
                org_set_color(o, JC_RED);
            }
            break;
        case TY_FIRE:
            if (org_use_energy(o, P.red_energy_consumption)) {
                // Get energy depending on segment length
                takenEnergyRed = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                // The other organism will be shown in fire
                org_set_color(org, CM_FIRE);
                // This organism will be shown in red
                org_set_color(o, JC_RED);
            }
            break;
        case TY_ORANGE:
        case TY_MAROON:
        case TY_PINK:
            if (org_use_energy(o, P.red_energy_consumption)) {
                // Get energy depending on segment length
                takenEnergyRed = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                // The other organism will be shown in yellow
                org_set_color(org, JC_YELLOW);
                // This organism will be shown in red
                org_set_color(o, JC_RED);
            }
            break;
        case TY_SILVER:
            if (((org->total_kills > 0) && ((org->isaconsumer) || (org->isafungus))) || (org->isenhanced)) {
                if (org_use_energy(o, P.red_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyRed = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in gold
                    org_set_color(org, CM_GOLD);
                    // This organism will be shown in red
                    org_set_color(o, JC_RED);
                }
            }
            break;
        case TY_SPIKE:
            if ((org->isenhanced) && (org_use_energy(o, P.red_energy_consumption))) {
                // Get energy depending on segment length
                takenEnergyRed = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                // The other organism will be shown in yellow
                org_set_color(org, JC_YELLOW);
                // This organism will be shown in red
                org_set_color(o, JC_RED);
            }
            break;
        case TY_SPIKEPOINT:
            if ((o->isenhanced) && (org->isenhanced) && (org_use_energy(o, P.red_energy_consumption))) {
                // Get energy depending on segment length
                takenEnergyRed = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                // The other organism will be shown in yellow
                org_set_color(org, JC_YELLOW);
                // This organism will be shown in red
                org_set_color(o, JC_RED);
            }
            break;
        case TY_GRAY:
            if ((o->isenhanced) && ((org->isaconsumer) || (org->isafungus))) {
                if (org->isenhanced) {
                    if (org_use_energy(o, P.red_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyRed = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in red
                        org_set_color(o, JC_RED);
                    }
                } else {
                    if (org_use_energy(o, P.red_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyRed = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in dark fire
                        org_set_color(o, CM_DARKFIRE);
                    }
                }
            }
            break;
        case TY_CREAM:
            if (o->isenhanced) {
                if ((org->isenhanced) && (gc_get_modifiescream(org->gc) == 2)) {
                    if (org_use_energy(o, P.red_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyRed = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in dark fire
                        org_set_color(o, CM_DARKFIRE);
                    }
                } else {
                    if (org_use_energy(o, P.red_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyRed = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in dark fire
                        org_set_color(o, CM_DARKFIRE);
                    }
                }
            }
            break;
        case TY_LILAC:
        case TY_DARKLILAC:
            if ((o->isenhanced) && ((org->isaconsumer) || (org->isafungus)) && (org_use_energy(o, P.red_energy_consumption))) {
                // Get energy depending on segment length
                takenEnergyRed = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                // The other organism will be shown in yellow
                org_set_color(org, JC_YELLOW);
                // This organism will be shown in dark fire
                org_set_color(o, CM_DARKFIRE);
            }
            break;
        case TY_BARK:
            org->seg_color[oseg] = CM_OLDBARK;
            if (org->active) {
                org->photosynthesis -= org->mphoto[oseg] * k_scale[org->growth_ratio - 1];
            }
            break;
        default:
            break;
        }
        // energy interchange
        if (takenEnergyRed > 0) {
            org->energy -= takenEnergyRed;
            o->energy += takenEnergyRed;
            takenEnergyRed = takenEnergyRed * P.organic_subs_produced;
            org_use_energy(o, takenEnergyRed);
        }
        break;
    case TY_PINK:;
        // Pink segment: try to get energy from corpses, weak organisms, viruses and parasites
        double takenEnergyPink = 0;
        if (org->hasdodged == false) {
            org->hasdodged = true;
        }
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_PINK:
            if (gc_get_modifiespink(o->gc)) {
                if (gc_get_modifiespink(org->gc)) {
                    if (org_use_energy(o, P.pink_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyPink = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in pink
                        org_set_color(org, JC_PINK);
                        // This organism will be shown in pink
                        org_set_color(o, JC_PINK);
                    }
                } else {
                    if (org_use_energy(o, P.pink_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyPink = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in dark fire
                        org_set_color(org, CM_DARKFIRE);
                        // This organism will be shown in pink
                        org_set_color(o, JC_PINK);
                    }
                }
            } else {
                if (gc_get_modifiespink(org->gc)) {
                    if (org_use_energy(o, P.pink_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyPink = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in pink
                        org_set_color(org, JC_PINK);
                        // This organism will be shown in dark fire
                        org_set_color(o, CM_DARKFIRE);
                    }
                } else {
                    if (org_use_energy(o, P.pink_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyPink = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in dark fire
                        org_set_color(org, CM_DARKFIRE);
                        // This organism will be shown in dark fire
                        org_set_color(o, CM_DARKFIRE);
                    }
                }
            }
            break;
        case TY_WHITE:
        case TY_LIGHTBROWN:
        case TY_ICE:
            if (org_use_energy(o, P.pink_energy_consumption)) {
                // Get energy depending on segment length
                takenEnergyPink = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                // The other organism will be shown in yellow
                org_set_color(org, JC_YELLOW);
                // This organism will be shown in pink
                org_set_color(o, JC_PINK);
            }
            break;
        case TY_FRUIT:
            if (org_use_energy(o, P.pink_energy_consumption)) {
                // Get energy depending on segment length
                takenEnergyPink = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                // The other organism will be shown in yellow
                org_set_color(org, JC_YELLOW);
                if ((org->sporeversion == 2) && (o->infected_gc != org->gc)) {
                    // Infectious fruit
                    if ((gc_get_modifiespink(o->gc)) || (o->isblond)) {
                        org_infected_by_fruit(o, org);
                        org_set_color(o, CM_BLOND);
                    } else if (o->indigo == 0) {
                        org_infected_by_fruit_weak(o, org);
                        org_set_color(o, CM_BLOND);
                    } else {
                        // This organism will be shown in pink
                        org_set_color(o, JC_PINK);
                    }
                } else {
                    // This organism will be shown in pink
                    org_set_color(o, JC_PINK);
                }
            }
            break;
        case TY_CREAM:
            if (((org->isenhanced) || (!org->active)) && (gc_get_modifiescream(org->gc) == 2)) {
                if (org_use_energy(o, P.pink_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyPink = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in green brown
                    org_set_color(org, CM_GREENBROWN);
                    // This organism will be shown in pink
                    org_set_color(o, JC_PINK);
                }
            } else {
                if (org_use_energy(o, P.pink_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyPink = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in pink
                    org_set_color(o, JC_PINK);
                }
            }
            break;
        case TY_GREENBROWN:
        case TY_BROKEN:
        case TY_LIGHT_BLUE:
        case TY_DARKOLIVE:
        case TY_DEEPSKY:
            if ((gc_get_modifiespink(o->gc)) || ((org->isblond) && (org->isaplant)) || (org->isfrozen) || (!org->active)) {
                if (org_use_energy(o, P.pink_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyPink = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in pink
                    org_set_color(o, JC_PINK);
                }
            } else {
                if (org_use_energy(o, P.pink_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyPink = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in dark fire
                    org_set_color(o, CM_DARKFIRE);
                }
            }
            break;
        case TY_DEADBARK:
            if ((gc_get_modifiespink(o->gc)) || (org->isblond) || ((org->isfrozen) && (o->skyversion > 0)) || (!org->active)) {
                if (org_use_energy(o, P.pink_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyPink = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in pink
                    org_set_color(o, JC_PINK);
                }
            } else {
                if (org_use_energy(o, P.pink_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyPink = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in dark fire
                    org_set_color(o, CM_DARKFIRE);
                }
            }
            break;
        case TY_DARKFIRE:
            if ((gc_get_modifiespink(o->gc)) || ((!org->isaplant) && (!org->isaconsumer) && (!org->isafungus)) || (!org->active)) {
                if (org_use_energy(o, P.pink_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyPink = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in pink
                    org_set_color(o, JC_PINK);
                }
            } else {
                if (org_use_energy(o, P.pink_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyPink = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in dark fire
                    org_set_color(o, CM_DARKFIRE);
                }
            }
            break;
        case TY_CORAL:
            if ((!gc_get_modifiespink(o->gc)) || (org->isaconsumer)) {
                if (org_use_energy(o, P.pink_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyPink = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in pink
                    org_set_color(o, JC_PINK);
                }
            } else {
                if (org_use_energy(o, P.pink_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyPink = bio_between((0.5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in green brown
                    org_set_color(org, CM_GREENBROWN);
                    // This organism will be shown in pink
                    org_set_color(o, JC_PINK);
                }
            }
            break;
        case TY_BROWN:
            if ((!o->isgray) || (gc_get_modifiespink(o->gc))) {
                if (org->alive) {
                    if (org_use_energy(o, P.pink_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyPink = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in pink
                        org_set_color(o, JC_PINK);
                        if (!org->isenhanced) {
                            org->isinjured = true;
                        }
                    }
                } else {
                    if (org_use_energy(o, P.pink_energy_consumption)) {
                        if ((o->fallowversion > 0) && ((org->age >> 8) >= org_get_max_age(org) * 2)) {
                            // Get the total energy of the still born child
                            takenEnergyPink = org->energy;
                        } else {
                            // Get energy depending on segment length
                            takenEnergyPink = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        }
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in pink
                        org_set_color(o, JC_PINK);
                    }
                }
            }
            break;
        case TY_PLAGUE:
            if (org->isinfectious) {
                if (org_use_energy(o, P.pink_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyPink = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in pink
                    org_set_color(o, JC_PINK);
                }
            } else {
                if (org_use_energy(o, P.pink_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyPink = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in green brown
                    org_set_color(org, CM_GREENBROWN);
                    // This organism will be shown in pink
                    org_set_color(o, JC_PINK);
                }
            }
            break;
        case TY_OLDBARK:
            if (o->isenhanced) {
                if ((org->isenhanced) && (org->active)) {
                    if ((gc_get_modifiespink(o->gc)) || (org->isblond)) {
                        if (org_use_energy(o, P.pink_energy_consumption)) {
                            if (org->frames_color > 0) {
                                // Get energy depending on segment length
                                takenEnergyPink = bio_between((0.01 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                                // The other organism will be shown in broken
                                org_set_color(org, CM_BROKEN);
                                // This organism will be shown in pink
                                org_set_color(o, JC_PINK);
                            } else {
                                // The other organism will be shown in broken
                                org_set_color(org, CM_BROKEN);
                            }
                        }
                    } else {
                        if (org_use_energy(o, P.pink_energy_consumption)) {
                            if (org->frames_color > 0) {
                                // Get energy depending on segment length
                                takenEnergyPink = bio_between((0.0025 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                                // The other organism will be shown in broken
                                org_set_color(org, CM_BROKEN);
                                // This organism will be shown in dark fire
                                org_set_color(o, CM_DARKFIRE);
                            } else {
                                // The other organism will be shown in broken
                                org_set_color(org, CM_BROKEN);
                            }
                        }
                    }
                } else {
                    if ((gc_get_modifiespink(o->gc)) || (org->isblond)) {
                        if (org_use_energy(o, P.pink_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergyPink = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in yellow
                            org_set_color(org, JC_YELLOW);
                            // This organism will be shown in pink
                            org_set_color(o, JC_PINK);
                        }
                    } else {
                        if (org_use_energy(o, P.pink_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergyPink = bio_between((0.25 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in yellow
                            org_set_color(org, JC_YELLOW);
                            // This organism will be shown in dark fire
                            org_set_color(o, CM_DARKFIRE);
                        }
                    }
                }
            }
            break;
        case TY_BARK:
            org->seg_color[oseg] = CM_OLDBARK;
            if (org->active) {
                org->photosynthesis -= org->mphoto[oseg] * k_scale[org->growth_ratio - 1];
            }
            break;
        default:
            break;
        }
        // energy interchange
        if (takenEnergyPink > 0) {
            org->energy -= takenEnergyPink;
            o->energy += takenEnergyPink;
            takenEnergyPink = takenEnergyPink * P.organic_subs_produced;
            org_use_decay_energy(o, takenEnergyPink);
        }
        break;
    case TY_MAROON:;
        // Maroon segment: try to get energy from plants and feed on ochre, sky, darkolive and cracked light blue segments
        double takenEnergyMaroon = 0;
        if (org->hasdodged == false) {
            org->hasdodged = true;
        }
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_WHITE:
        case TY_FALLOW:
        case TY_OCHRE:
        case TY_DEEPSKY:
        case TY_DARKOLIVE:
        case TY_SPIKE:
        case TY_ICE:
            if (org->isaplant) {
                if (org_use_energy(o, P.maroon_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyMaroon = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in maroon
                    org_set_color(o, CM_MAROON);
                }
            }
            break;
        case TY_GRASS:
            if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                org_set_color(org, CM_TEAL);
                org_set_color(o, CM_MAROON);
            } else {
                if (org_use_energy(o, P.maroon_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyMaroon = bio_between((0.5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in green brown
                    org_set_color(org, CM_GREENBROWN);
                    // This organism will be shown in maroon
                    org_set_color(o, CM_MAROON);
                }
            }
            break;
        case TY_GREEN:
        case TY_FOREST:
        case TY_SPRING:
        case TY_SUMMER:
        case TY_WINTER:
        case TY_LIME:
        case TY_LEAF:
        case TY_C4:
        case TY_JADE:
        case TY_DARKJADE:
        case TY_DARKGREEN:
        case TY_PURPLE:
            if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                org_set_color(org, CM_TEAL);
                org_set_color(o, CM_MAROON);
            } else {
                if (org_use_energy(o, P.maroon_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyMaroon = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in maroon
                    org_set_color(o, CM_MAROON);
                }
            }
            break;
        case TY_BLUE:
            if (org->isaplant) {
                if (org_use_energy(org, P.blue_energy_consumption)) {
                    if ((org->isenhanced) && (!o->isenhanced)) {
                        org_use_energy(o, bio_between((0.25 * sqrt(org->m[oseg])) * P.organic_obtained_energy, 0, o->energy));
                        org_set_color(o, CM_DARKLILAC);
                        if (o->energy < BIO_TOL) {
                            org_die(o, org);
                        }
                    } else {
                        org_set_color(o, CM_MAROON);
                    }
                    org_set_color(org, JC_BLUE);
                } else {
                    // Doesn't have energy to use the shield
                    if (org_use_energy(o, P.maroon_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyMaroon = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in maroon
                        org_set_color(o, CM_MAROON);
                    }
                }
            }
            break;
        case TY_PLAGUE:
            if (org->isaplant) {
                if (org->isinfectious) {
                    if (org_use_energy(o, P.maroon_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyMaroon = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in maroon
                        org_set_color(o, CM_MAROON);
                    }
                } else {
                    if (org_use_energy(o, P.maroon_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyMaroon = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in maroon
                        org_set_color(o, CM_MAROON);
                    }
                }
            }
            break;
        case TY_FRUIT:
            if (org->sporeversion == 2) {
                if (org_use_energy(o, P.maroon_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyMaroon = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    if (o->infected_gc != org->gc) {
                        // Infectious fruit
                        org_infected_by_fruit(o, org);
                        org_set_color(o, CM_BLOND);
                    } else {
                        // This organism will be shown in maroon
                        org_set_color(o, CM_MAROON);
                    }
                }
            }
            break;
        case TY_BROKEN:
        case TY_LIGHT_BLUE:
        case TY_DEADBARK:
            if ((org->isaplant) || (!org->active)) {
                if (org_use_energy(o, P.maroon_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyMaroon = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in maroon
                    org_set_color(o, CM_MAROON);
                }
            }
            break;
        case TY_OLDBARK:
            if (org->active) {
                if (org->isenhanced) {
                    if (org_use_energy(o, P.maroon_energy_consumption)) {
                        if (org->frames_color > 0) {
                            // Get energy depending on segment length
                            takenEnergyMaroon = bio_between((0.01 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in broken
                            org_set_color(org, CM_BROKEN);
                            // This organism will be shown in maroon
                            org_set_color(o, CM_MAROON);
                        } else {
                            // The other organism will be shown in broken
                            org_set_color(org, CM_BROKEN);
                        }
                    }
                } else {
                    if (org_use_energy(o, P.maroon_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyMaroon = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in maroon
                        org_set_color(o, CM_MAROON);
                    }
                }
            } else {
                if (org->isenhanced) {
                    if (org_use_energy(o, P.maroon_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyMaroon = bio_between((0.5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in maroon
                        org_set_color(o, CM_MAROON);
                    }
                } else {
                    if (org_use_energy(o, P.maroon_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyMaroon = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in maroon
                        org_set_color(o, CM_MAROON);
                    }
                }
            }
            break;
        case TY_BARK:
            org->seg_color[oseg] = CM_OLDBARK;
            if (org->active) {
                org->photosynthesis -= org->mphoto[oseg] * k_scale[org->growth_ratio - 1];
            } else {
                if (!org->isenhanced) {
                    if (org_use_energy(o, P.maroon_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyMaroon = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in maroon
                        org_set_color(o, CM_MAROON);
                    }
                }
            }
            break;
        case TY_CYAN:
        case TY_TEAL:
        case TY_YELLOW:
        case TY_AUBURN:
        case TY_INDIGO:
        case TY_BLOND:
        case TY_FLOWER:
        case TY_DARKGRAY:
        case TY_GOLD:
        case TY_SPORE:
            if ((o->isenhanced) && (org->isaplant)) {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_MAROON);
                } else {
                    if (org_use_energy(o, P.maroon_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyMaroon = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in auburn
                        org_set_color(o, CM_AUBURN);
                    }
                }
            }
            break;
        case TY_SILVER:
            if (((org->total_kills > 0) && ((org->isaconsumer) || (org->isafungus))) || (org->isenhanced)) {
                break;
            } else {
                if ((o->isenhanced) && (org->isaplant) && (org_use_energy(o, P.maroon_energy_consumption))) {
                    // Get energy depending on segment length
                    takenEnergyMaroon = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in auburn
                    org_set_color(o, CM_AUBURN);
                }
            }
            break;
        case TY_SPIKEPOINT:
            if ((o->isenhanced) && (!org->isenhanced) && (org->isaplant) && (org_use_energy(o, P.maroon_energy_consumption))) {
                // Get energy depending on segment length
                takenEnergyMaroon = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                // The other organism will be shown in yellow
                org_set_color(org, JC_YELLOW);
                // This organism will be shown in auburn
                org_set_color(o, CM_AUBURN);
            }
            break;
        case TY_MINT:
        case TY_LAVENDER:
        case TY_MAGENTA:
        case TY_ROSE:
            if (o->altruist) {
                break;
            } else {
                if ((o->isenhanced) && (org->isaplant)) {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, CM_MAROON);
                    } else {
                        if (org_use_energy(o, P.maroon_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergyMaroon = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in yellow
                            org_set_color(org, JC_YELLOW);
                            // This organism will be shown in auburn
                            org_set_color(o, CM_AUBURN);
                        }
                    }
                }
            }
            break;
        case TY_SKY:
            if ((o->isenhanced) && (org->isaplant)) {
                if (org_use_energy(org, P.sky_energy_consumption)) {
                    org->seg_color[oseg] = CM_DEEPSKY;
                    org->useextraeffects = true;
                    org_set_color(o, CM_AUBURN);
                    org_set_color(org, CM_DEEPSKY);
                } else {
                    if (org_use_energy(o, P.maroon_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyMaroon = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in auburn
                        org_set_color(o, CM_AUBURN);
                    }
                }
            }
            break;
        case TY_GREENBROWN:
        case TY_VISION:
            if ((o->isenhanced) && (org->isaplant) && (org_use_energy(o, P.maroon_energy_consumption))) {
                // Get energy depending on segment length
                takenEnergyMaroon = bio_between((sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                // The other organism will be shown in yellow
                org_set_color(org, JC_YELLOW);
                // This organism will be shown in auburn
                org_set_color(o, CM_AUBURN);
            }
            break;
        default:
            break;
        }
        // energy interchange
        if (takenEnergyMaroon > 0) {
            org->energy -= takenEnergyMaroon;
            o->energy += takenEnergyMaroon;
            takenEnergyMaroon = takenEnergyMaroon * P.organic_subs_produced;
            org_use_energy(o, takenEnergyMaroon);
        }
        break;
    case TY_CREAM:;
        // Cream segment: Parasitize on other organisms
        double takenEnergyCream = 0;
        if (org->hasdodged == false) {
            org->hasdodged = true;
        }
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_WHITE:
            if ((org->isaplant) || (org->isaconsumer) || (org->isafungus) || (org->plagueversion > 0) || (org->isauburn)) {
                if (org->lavender > 0) {
                    org_lavendershield(org);
                    org_set_color(o, CM_CREAM);
                }
                if (org->lavender <= 0) {
                    if (org_use_energy(o, P.cream_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyCream = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in cream
                        org_set_color(o, CM_CREAM);
                    }
                }
            } else {
                if ((o->isenhanced) && (gc_get_modifiescream(o->gc) == 2)) {
                    if (org->lavender > 0) {
                        org_lavendershield(org);
                        org_set_color(o, JC_DARK_GRAY);
                    }
                    if (org->lavender <= 0) {
                        if (org_use_energy(o, P.cream_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergyCream = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in green brown
                            org_set_color(org, CM_GREENBROWN);
                            // This organism will be shown in dark gray
                            org_set_color(o, JC_DARK_GRAY);
                        }
                    }
                }
            }
            break;
        case TY_GREEN:
        case TY_GRASS:
        case TY_FOREST:
        case TY_SPRING:
        case TY_SUMMER:
        case TY_WINTER:
        case TY_LIME:
        case TY_LEAF:
        case TY_C4:
        case TY_JADE:
        case TY_DARKJADE:
        case TY_DARKGREEN:
        case TY_PURPLE:
        case TY_TEAL:
        case TY_CYAN:
        case TY_YELLOW:
        case TY_AUBURN:
        case TY_INDIGO:
        case TY_BLOND:
        case TY_FLOWER:
        case TY_DARKGRAY:
        case TY_GOLD:
        case TY_SPORE:
            if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                org_set_color(org, CM_TEAL);
                org_set_color(o, CM_CREAM);
            } else {
                if (org->lavender > 0) {
                    org_lavendershield(org);
                    org_set_color(o, CM_CREAM);
                }
                if (org->lavender <= 0) {
                    if (org_use_energy(o, P.cream_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyCream = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in cream
                        org_set_color(o, CM_CREAM);
                    }
                }
            }
            break;
        case TY_SPIKE:
            if (((org->isaplant) && (org->createlavender == 0)) || (org->isenhanced)) {
                if (org->lavender > 0) {
                    org_lavendershield(org);
                    org_set_color(o, CM_CREAM);
                }
                if (org->lavender <= 0) {
                    if (org_use_energy(o, P.cream_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyCream = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in cream
                        org_set_color(o, CM_CREAM);
                    }
                }
            }
            break;
        case TY_BLUE:
            if (((!o->isenhanced) || (gc_get_modifiescream(o->gc) != 3) || (!org->isaplant) || (org->createlavender > 0)) && (org_use_energy(org, P.blue_energy_consumption))) {
                if (org->isenhanced) {
                    org_use_energy(o, bio_between((0.5 * sqrt(org->m[oseg])) * P.organic_obtained_energy, 0, o->energy));
                    org_set_color(o, CM_DARKLILAC);
                    if (o->energy < BIO_TOL) {
                        org_die(o, org);
                    }
                } else {
                    org_set_color(o, CM_CREAM);
                }
                org_set_color(org, JC_BLUE);
            } else {
                if (org->lavender > 0) {
                    org_lavendershield(org);
                    if ((o->isenhanced) && (gc_get_modifiescream(o->gc)) == 3) {
                        // This organism will be shown in dark gray
                        org_set_color(o, JC_DARK_GRAY);
                    } else {
                        // This organism will be shown in cream
                        org_set_color(o, CM_CREAM);
                    }
                }
                if (org->lavender <= 0) {
                    if (org_use_energy(o, P.cream_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyCream = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        if ((o->isenhanced) && (gc_get_modifiescream(o->gc)) == 3) {
                            // This organism will be shown in dark gray
                            org_set_color(o, JC_DARK_GRAY);
                        } else {
                            // This organism will be shown in cream
                            org_set_color(o, CM_CREAM);
                        }
                    }
                }
            }
            break;
        case TY_SKY:
            if ((o->active) || (org->skyversion == 1)) {
                if (org_use_energy(org, P.sky_energy_consumption)) {
                    org->seg_color[oseg] = CM_DEEPSKY;
                    org->useextraeffects = true;
                    org_set_color(o, CM_CREAM);
                    org_set_color(org, CM_DEEPSKY);
                } else {
                    // Doesn't have energy to use the shield
                    if (org->lavender > 0) {
                        org_lavendershield(org);
                        org_set_color(o, CM_CREAM);
                    }
                    if (org->lavender <= 0) {
                        if (org_use_energy(o, P.cream_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergyCream = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in green brown
                            org_set_color(org, CM_GREENBROWN);
                            // This organism will be shown in cream
                            org_set_color(o, CM_CREAM);
                        }
                    }
                }
            }
            break;
        case TY_DEEPSKY:
            if (((o->isenhanced) || (!o->active)) && (gc_get_modifiescream(o->gc) == 3)) {
                if (org->lavender > 0) {
                    org_lavendershield(org);
                    org_set_color(o, JC_DARK_GRAY);
                }
                if (org->lavender <= 0) {
                    if (org_use_energy(o, P.cream_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyCream = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in dark gray
                        org_set_color(o, JC_DARK_GRAY);
                    }
                }
            } else {
                org_set_color(o, CM_CREAM);
                org_set_color(org, CM_DEEPSKY);
            }
            break;
        case TY_FALLOW:
            if ((o->isenhanced) && (gc_get_modifiescream(o->gc) == 3)) {
                if (org->lavender > 0) {
                    org_lavendershield(org);
                    org_set_color(o, JC_DARK_GRAY);
                }
                if (org->lavender <= 0) {
                    if (org_use_energy(o, P.cream_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyCream = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in dark gray
                        org_set_color(o, JC_DARK_GRAY);
                    }
                }
            }
            break;
        case TY_OLDBARK:
            if (((o->isenhanced) || (!o->active)) && (gc_get_modifiescream(o->gc) == 3)) {
                if (org->lavender > 0) {
                    org_lavendershield(org);
                    org_set_color(o, JC_DARK_GRAY);
                }
                if (org->lavender <= 0) {
                    if (org_use_energy(o, P.cream_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyCream = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in dark gray
                        org_set_color(o, JC_DARK_GRAY);
                    }
                }
            }
            break;
        case TY_BARK:
            org->seg_color[oseg] = CM_OLDBARK;
            if (org->active) {
                org->photosynthesis -= org->mphoto[oseg] * k_scale[org->growth_ratio - 1];
            }
            break;
        case TY_SILVER:
            if (((org->total_kills > 0) && ((org->isaconsumer) || (org->isafungus))) || (org->isenhanced)) {
                break;
            } else {
                if (org->lavender > 0) {
                    org_lavendershield(org);
                    org_set_color(o, CM_CREAM);
                }
                if (org->lavender <= 0) {
                    if (org_use_energy(o, P.cream_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyCream = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in cream
                        org_set_color(o, CM_CREAM);
                    }
                }
            }
            break;
        case TY_FIRE:
        case TY_ORANGE:
        case TY_RED:
        case TY_MAROON:
            if ((o->isenhanced) && (gc_get_modifiescream(o->gc) == 1)) {
                if (org->lavender > 0) {
                    org_lavendershield(org);
                    org_set_color(o, JC_DARK_GRAY);
                }
                if (org->lavender <= 0) {
                    if (org_use_energy(o, P.cream_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyCream = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in dark gray
                        org_set_color(o, JC_DARK_GRAY);
                    }
                }
            }
            break;
        case TY_FRUIT:
            if (org->sporeversion == 2) {
                if (org_use_energy(o, P.cream_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyCream = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in green brown
                    org_set_color(org, CM_GREENBROWN);
                    // This organism will be shown in cream
                    org_set_color(o, CM_CREAM);
                }
            } else {
                if ((o->isenhanced) && (gc_get_modifiescream(o->gc) == 2)) {
                    if (org_use_energy(o, P.cream_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyCream = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in dark gray
                        org_set_color(o, JC_DARK_GRAY);
                    }
                }
            }
            break;
        case TY_MAGENTA:
        case TY_ROSE:
            if (o->altruist) {
                break;
            } else {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_CREAM);
                } else {
                    if (org->lavender > 0) {
                        org_lavendershield(org);
                        org_set_color(o, CM_CREAM);
                    }
                    if (org->lavender <= 0) {
                        if (org_use_energy(o, P.cream_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergyCream = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in green brown
                            org_set_color(org, CM_GREENBROWN);
                            // This organism will be shown in cream
                            org_set_color(o, CM_CREAM);
                        }
                    }
                }
            }
            break;
        case TY_CREAM:
            if ((o->isenhanced) && (gc_get_modifiescream(o->gc) == 2)) {
                if ((org->isenhanced) && (gc_get_modifiescream(org->gc) == 2)) {
                    if (org_use_energy(o, P.cream_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyCream = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in dark gray
                        org_set_color(org, JC_DARK_GRAY);
                        // This organism will be shown in dark gray
                        org_set_color(o, JC_DARK_GRAY);
                    }
                } else {
                    if (org->lavender > 0) {
                        org_lavendershield(org);
                        org_set_color(o, JC_DARK_GRAY);
                    }
                    if (org->lavender <= 0) {
                        if (org_use_energy(o, P.cream_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergyCream = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in green brown
                            org_set_color(org, CM_GREENBROWN);
                            // This organism will be shown in dark gray
                            org_set_color(o, JC_DARK_GRAY);
                        }
                    }
                }
            }
            break;
        case TY_PINK:
            if (((o->isenhanced) || (!o->active)) && (gc_get_modifiescream(o->gc) == 2)) {
                if (org->lavender > 0) {
                    org_lavendershield(org);
                    org_set_color(o, JC_DARK_GRAY);
                }
                if (org->lavender <= 0) {
                    if (org_use_energy(o, P.cream_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyCream = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in dark gray
                        org_set_color(o, JC_DARK_GRAY);
                    }
                }
            }
            break;
        case TY_CORAL:
            if ((o->isenhanced) && (gc_get_modifiescream(o->gc) == 2)) {
                if (org->lavender > 0) {
                    org_lavendershield(org);
                    org_set_color(o, JC_DARK_GRAY);
                }
                if (org->lavender <= 0) {
                    if (org_use_energy(o, P.cream_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyCream = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in dark gray
                        org_set_color(o, JC_DARK_GRAY);
                    }
                }
            }
            break;
        case TY_LAVENDER:
            if (org->isenhanced) {
                org_use_energy(o, bio_between((1 / P.lavender_energy_consumption), 0, o->energy));
                if (org->lavender < P.lavender_shield) {
                    org->lavender += 1000;
                    if (org->lavender >= P.lavender_shield) {
                        org->lavender = P.lavender_shield;
                    }
                }
            } else {
                org_use_energy(o, bio_between((0.2 / P.lavender_energy_consumption), 0, o->energy));
                if (org->lavender < P.lavender_shield) {
                    org->lavender += 200;
                    if (org->lavender >= P.lavender_shield) {
                        org->lavender = P.lavender_shield;
                    }
                }
            }
            // The other organism will be shown in lavender
            org_set_color(org, CM_LAVENDER);
            // This organism will be shown in deadbark
            org_set_color(o, CM_DEADBARK);
            if (o->energy < BIO_TOL) {
                org_die(o, org);
            }
            break;
        case TY_MINT:
            if (o->healing > 0) {
                if (org_use_energy(org, P.mint_energy_consumption / 10)) {
                    o->seg_color[seg] = CM_DARKFIRE;
                    org_set_color(org, CM_MINT);
                    o->useextraeffects = true;
                    o->isinjured = true;
                }
            } else {
                if (o->isaplant) {
                    if (org_use_energy(org, P.mint_energy_consumption / 10)) {
                        o->seg_color[seg] = CM_BROKEN;
                        org_set_color(org, CM_MINT);
                        o->isinjured = true;
                    }
                } else {
                    if (org_use_energy(org, P.mint_energy_consumption / 10)) {
                        o->seg_color[seg] = CM_LIGHTBROWN;
                        org_set_color(org, CM_MINT);
                        o->isinjured = true;
                    }
                }
            }
            break;
        case TY_SPIKEPOINT:
            break;
        case TY_BROWN:
            if ((!o->active) && (gc_get_modifiescream(o->gc) == 1)) {
                if (org_use_energy(o, P.cream_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyCream = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in green brown
                    org_set_color(org, CM_GREENBROWN);
                    // This organism will be shown in cream
                    org_set_color(o, CM_CREAM);
                    if ((org->alive) && (!org->isenhanced)) {
                        org->isinjured = true;
                    }
                }
            }
            break;
        default:
            if (org->lavender > 0) {
                org_lavendershield(org);
                org_set_color(o, CM_CREAM);
            }
            if (org->lavender <= 0) {
                if (org_use_energy(o, P.cream_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyCream = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in green brown
                    org_set_color(org, CM_GREENBROWN);
                    // This organism will be shown in cream
                    org_set_color(o, CM_CREAM);
                }
            }
        }
        // energy interchange
        if (takenEnergyCream > 0) {
            org->energy -= takenEnergyCream;
            o->energy += takenEnergyCream;
            takenEnergyCream = takenEnergyCream * P.cream_organic_subs_produced;
            org_use_energy(o, takenEnergyCream);
        }
        break;
    case TY_LILAC:;
        // Lilac segment: Weaken organisms
        double takenEnergyLilac = 0;
        if (org->hasdodged == false) {
            org->hasdodged = true;
        }
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_LILAC:
            if (org_use_energy(o, P.lilac_energy_consumption)) {
                // Get energy depending on segment length
                takenEnergyLilac = bio_between((10 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                // The other organism will be shown in dark lilac
                org_set_color(org, CM_DARKLILAC);
                // This organism will be shown in lilac
                org_set_color(o, CM_LILAC);
                // Organism has to recharge
                if (!o->isenhanced) {
                    o->seg_color[seg] = CM_DARKLILAC;
                    o->useextraeffects = true;
                }
                if (!org->isenhanced) {
                    org->seg_color[oseg] = CM_DARKLILAC;
                    org->useextraeffects = true;
                }
            }
            break;
        case TY_WHITE:
        case TY_OLDBARK:
        case TY_DEADBARK:
        case TY_ICE:
        case TY_CORAL:
        case TY_FRUIT:
            if ((gc_get_modifieslilac(o->gc)) || (org->isaconsumer) || (org->isafungus) || (org->plagueversion > 0) || (org->fallowversion > 0) || ((org->iscoral) && (!o->isaplant) && (!o->isaconsumer))) {
                if (org_use_energy(o, P.lilac_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyLilac = bio_between((10 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in dark lilac
                    org_set_color(org, CM_DARKLILAC);
                    // This organism will be shown in lilac
                    org_set_color(o, CM_LILAC);
                    // Organism has to recharge
                    if (!o->isenhanced) {
                        o->seg_color[seg] = CM_DARKLILAC;
                        o->useextraeffects = true;
                    }
                }
            }
            break;
        case TY_GRASS:
            if ((gc_get_modifieslilac(o->gc)) || (org->isaconsumer) || (org->isafungus) || (org->plagueversion > 0) || (org->fallowversion > 0) || ((org->iscoral) && (!o->isaplant) && (!o->isaconsumer))) {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_LILAC);
                } else {
                    if (org_use_energy(o, P.lilac_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyLilac = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in dark lilac
                        org_set_color(org, CM_DARKLILAC);
                        // This organism will be shown in lilac
                        org_set_color(o, CM_LILAC);
                        // Organism has to recharge
                        if (!o->isenhanced) {
                            o->seg_color[seg] = CM_DARKLILAC;
                            o->useextraeffects = true;
                        }
                    }
                }
            }
            break;
        case TY_GREEN:
        case TY_FOREST:
        case TY_SPRING:
        case TY_SUMMER:
        case TY_WINTER:
        case TY_LIME:
        case TY_LEAF:
        case TY_C4:
        case TY_JADE:
        case TY_DARKJADE:
        case TY_DARKGREEN:
        case TY_PURPLE:
        case TY_TEAL:
        case TY_CYAN:
        case TY_YELLOW:
        case TY_AUBURN:
        case TY_INDIGO:
        case TY_BLOND:
        case TY_FLOWER:
        case TY_DARKGRAY:
        case TY_GOLD:
        case TY_SPORE:
            if ((gc_get_modifieslilac(o->gc)) || (org->isaconsumer) || (org->isafungus) || (org->plagueversion > 0) || (org->fallowversion > 0) || ((org->iscoral) && (!o->isaplant) && (!o->isaconsumer))) {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_LILAC);
                } else {
                    if (org_use_energy(o, P.lilac_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyLilac = bio_between((10 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in dark lilac
                        org_set_color(org, CM_DARKLILAC);
                        // This organism will be shown in lilac
                        org_set_color(o, CM_LILAC);
                        // Organism has to recharge
                        if (!o->isenhanced) {
                            o->seg_color[seg] = CM_DARKLILAC;
                            o->useextraeffects = true;
                        }
                    }
                }
            }
            break;
        case TY_BLUE:
            if (org_use_energy(org, P.blue_energy_consumption)) {
                if (org->isenhanced) {
                    org_use_energy(o, bio_between((0.5 * sqrt(org->m[oseg])) * P.organic_obtained_energy, 0, o->energy));
                    org_set_color(o, CM_DARKLILAC);
                    if (o->energy < BIO_TOL) {
                        org_die(o, org);
                    }
                } else {
                    org_set_color(o, CM_LILAC);
                }
                org_set_color(org, JC_BLUE);
            } else {
                // Doesn't have energy to use the shield
                if (org_use_energy(o, P.lilac_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyLilac = bio_between((10 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in dark lilac
                    org_set_color(org, CM_DARKLILAC);
                    // This organism will be shown in lilac
                    org_set_color(o, CM_LILAC);
                    // Organism has to recharge
                    if (!o->isenhanced) {
                        o->seg_color[seg] = CM_DARKLILAC;
                        o->useextraeffects = true;
                    }
                }
            }
            break;
        case TY_RED:
            if (org->isenhanced) {
                break;
            } else {
                if (org_use_energy(o, P.lilac_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyLilac = bio_between((10 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in dark lilac
                    org_set_color(org, CM_DARKLILAC);
                    // This organism will be shown in lilac
                    org_set_color(o, CM_LILAC);
                    // Organism has to recharge
                    if (!o->isenhanced) {
                        o->seg_color[seg] = CM_DARKLILAC;
                        o->useextraeffects = true;
                    }
                }
            }
            break;
        case TY_MINT:
        case TY_LAVENDER:
        case TY_MAGENTA:
        case TY_ROSE:
            if (o->altruist) {
                break;
            } else {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_LILAC);
                } else {
                    if (org_use_energy(o, P.lilac_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyLilac = bio_between((10 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in dark lilac
                        org_set_color(org, CM_DARKLILAC);
                        // This organism will be shown in lilac
                        org_set_color(o, CM_LILAC);
                        // Organism has to recharge
                        if (!o->isenhanced) {
                            o->seg_color[seg] = CM_DARKLILAC;
                            o->useextraeffects = true;
                        }
                    }
                }
            }
            break;
        case TY_SPIKE:
            if ((org->isaplant) || (org->isenhanced)) {
                if (org_use_energy(o, P.lilac_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyLilac = bio_between((10 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in dark lilac
                    org_set_color(org, CM_DARKLILAC);
                    // This organism will be shown in lilac
                    org_set_color(o, CM_LILAC);
                    // Organism has to recharge
                    if (!o->isenhanced) {
                        o->seg_color[seg] = CM_DARKLILAC;
                        o->useextraeffects = true;
                    }
                }
            }
            break;
        case TY_SPIKEPOINT:
            if (((org->isaplant) && (gc_get_modifieslilac(o->gc))) || ((org->isenhanced) && (!gc_get_modifieslilac(o->gc)))) {
                if (org_use_energy(o, P.lilac_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergyLilac = bio_between((10 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in dark lilac
                    org_set_color(org, CM_DARKLILAC);
                    // This organism will be shown in lilac
                    org_set_color(o, CM_LILAC);
                    // Organism has to recharge
                    if (!o->isenhanced) {
                        o->seg_color[seg] = CM_DARKLILAC;
                        o->useextraeffects = true;
                    }
                }
            }
            break;
        case TY_OLIVE:
            break;
        case TY_BROWN:
            if (org->alive) {
                if ((gc_get_modifieslilac(o->gc)) || (!org->isenhanced)) {
                    if (org_use_energy(o, P.lilac_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergyLilac = bio_between((10 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in dark lilac
                        org_set_color(org, CM_DARKLILAC);
                        // This organism will be shown in lilac
                        org_set_color(o, CM_LILAC);
                        // Organism has to recharge
                        if (!o->isenhanced) {
                            o->seg_color[seg] = CM_DARKLILAC;
                            o->useextraeffects = true;
                        }
                        if (!org->isenhanced) {
                            org->isinjured = true;
                        }
                    }
                }
            }
            break;
        case TY_BARK:
            org->seg_color[oseg] = CM_OLDBARK;
            if (org->active) {
                org->photosynthesis -= org->mphoto[oseg] * k_scale[org->growth_ratio - 1];
            }
            break;
        default:
            if (org_use_energy(o, P.lilac_energy_consumption)) {
                // Get energy depending on segment length
                takenEnergyLilac = bio_between((10 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                // The other organism will be shown in dark lilac
                org_set_color(org, CM_DARKLILAC);
                // This organism will be shown in lilac
                org_set_color(o, CM_LILAC);
                // Organism has to recharge
                if (!o->isenhanced) {
                    o->seg_color[seg] = CM_DARKLILAC;
                    o->useextraeffects = true;
                }
            }
        }
        // energy interchange
        if (takenEnergyLilac > 0) {
            org_use_energy(org, takenEnergyLilac);
        }
        break;
    case TY_SPIKEPOINT:;
        // Spike segment: Hurts organisms, if it hits with its end point, enhanced SPIKE can feed on other organisms
        double takenEnergySpike = 0;
        if (org->hasdodged == false) {
            org->hasdodged = true;
        }
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_SPIKEPOINT:
        case TY_OLDBARK:
        case TY_CREAM:
            break;
        case TY_SPIKE:
            if ((org->isaplant) || (org->isenhanced)) {
                if (o->isenhanced) {
                    if (org_use_energy(o, P.mosquito_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergySpike = bio_between((0.8 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in spike
                        org_set_color(o, CM_SPIKE);
                    }
                } else {
                    if (o->isaplant) {
                        if (org_use_energy(o, P.spike_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in dark lilac
                            org_set_color(org, CM_DARKLILAC);
                            // This organism will be shown in spike
                            org_set_color(o, CM_SPIKE);
                        }
                    } else {
                        if ((org->isaconsumer) || (org->isafungus) || ((!org->isaplant) && ((o->isaconsumer) || (o->isafungus) || (org->iscoral)))) {
                            if (org_use_energy(o, P.spike_energy_consumption)) {
                                // Get energy depending on segment length
                                takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                                // The other organism will be shown in dark lilac
                                org_set_color(org, CM_DARKLILAC);
                                // This organism will be shown in spike
                                org_set_color(o, CM_SPIKE);
                            }
                        }
                    }
                }
            }
            break;
        case TY_BROWN:
            if ((!o->isenhanced) && (org->alive)) {
                if (o->isaplant) {
                    if (org_use_energy(o, P.spike_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in dark lilac
                        org_set_color(org, CM_DARKLILAC);
                        // This organism will be shown in spike
                        org_set_color(o, CM_SPIKE);
                        if (!org->isenhanced) {
                            org->isinjured = true;
                        }
                    }
                }
            }
            break;
        case TY_BLUE:
            if (org_use_energy(org, P.blue_energy_consumption)) {
                if ((org->isenhanced) && ((o->isenhanced) || (o->isaplant))) {
                    org_use_energy(o, bio_between((0.5 * sqrt(org->m[oseg])) * P.organic_obtained_energy, 0, o->energy));
                    org_set_color(o, CM_DARKLILAC);
                    if (o->energy < BIO_TOL) {
                        org_die(o, org);
                    }
                } else {
                    org_set_color(o, CM_SPIKE);
                }
                org_set_color(org, JC_BLUE);
            } else {
                // Doesn't have energy to use the shield
                if (o->isenhanced) {
                    if (org_use_energy(o, P.mosquito_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergySpike = bio_between((0.8 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in spike
                        org_set_color(o, CM_SPIKE);
                    }
                } else {
                    if (o->isaplant) {
                        if (org_use_energy(o, P.spike_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in dark lilac
                            org_set_color(org, CM_DARKLILAC);
                            // This organism will be shown in spike
                            org_set_color(o, CM_SPIKE);
                        }
                    } else {
                        if ((org->isaconsumer) || (org->isafungus) || ((!org->isaplant) && ((o->isaconsumer) || (o->isafungus) || (org->iscoral)))) {
                            if (org_use_energy(o, P.spike_energy_consumption)) {
                                // Get energy depending on segment length
                                takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                                // The other organism will be shown in dark lilac
                                org_set_color(org, CM_DARKLILAC);
                                // This organism will be shown in spike
                                org_set_color(o, CM_SPIKE);
                            }
                        }
                    }
                }
            }
            break;
        case TY_SKY:
            if (org_use_energy(org, P.sky_energy_consumption)) {
                org->seg_color[oseg] = CM_DEEPSKY;
                org->useextraeffects = true;
                org_set_color(o, CM_SPIKE);
                org_set_color(org, CM_DEEPSKY);
            } else {
                // Doesn't have energy to use the shield
                if (o->isenhanced) {
                    if (org_use_energy(o, P.mosquito_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergySpike = bio_between((0.8 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in spike
                        org_set_color(o, CM_SPIKE);
                    }
                } else {
                    if (o->isaplant) {
                        if (org_use_energy(o, P.spike_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in dark lilac
                            org_set_color(org, CM_DARKLILAC);
                            // This organism will be shown in spike
                            org_set_color(o, CM_SPIKE);
                        }
                    } else {
                        if ((org->isaconsumer) || (org->isafungus) || ((!org->isaplant) && ((o->isaconsumer) || (o->isafungus) || (org->iscoral)))) {
                            if (org_use_energy(o, P.spike_energy_consumption)) {
                                // Get energy depending on segment length
                                takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                                // The other organism will be shown in dark lilac
                                org_set_color(org, CM_DARKLILAC);
                                // This organism will be shown in spike
                                org_set_color(o, CM_SPIKE);
                            }
                        }
                    }
                }
            }
            break;
        case TY_DEEPSKY:
            org_set_color(o, CM_SPIKE);
            org_set_color(org, CM_DEEPSKY);
            break;
        case TY_OCHRE:
            if (org->isaplant) {
                if (o->isenhanced) {
                    if (org_use_energy(o, P.mosquito_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergySpike = bio_between((0.8 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in spike
                        org_set_color(o, CM_SPIKE);
                    }
                } else {
                    if (o->isaplant) {
                        if (org_use_energy(o, P.spike_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in dark lilac
                            org_set_color(org, CM_DARKLILAC);
                            // This organism will be shown in spike
                            org_set_color(o, CM_SPIKE);
                        }
                    } else {
                        if ((org->isaconsumer) || (org->isafungus) || ((!org->isaplant) && ((o->isaconsumer) || (o->isafungus) || (org->iscoral)))) {
                            if (org_use_energy(o, P.spike_energy_consumption)) {
                                // Get energy depending on segment length
                                takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                                // The other organism will be shown in dark lilac
                                org_set_color(org, CM_DARKLILAC);
                                // This organism will be shown in spike
                                org_set_color(o, CM_SPIKE);
                            }
                        }
                    }
                }
            }
            break;
        case TY_CORAL:
            if ((!o->isaplant) && (!o->isaconsumer) && (!o->isafungus)) {
                if ((org->isaconsumer) || (org->isafungus) || ((!org->isaplant) && ((o->isaconsumer) || (o->isafungus) || (org->iscoral)))) {
                    if (org_use_energy(o, P.spike_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in dark lilac
                        org_set_color(org, CM_DARKLILAC);
                        // This organism will be shown in spike
                        org_set_color(o, CM_SPIKE);
                    }
                }
            }
            break;
        case TY_RED:
            if (org->isenhanced) {
                break;
            } else {
                if (o->isenhanced) {
                    if (org_use_energy(o, P.mosquito_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergySpike = bio_between((0.8 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in spike
                        org_set_color(o, CM_SPIKE);
                    }
                } else {
                    if (o->isaplant) {
                        if (org_use_energy(o, P.spike_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in dark lilac
                            org_set_color(org, CM_DARKLILAC);
                            // This organism will be shown in spike
                            org_set_color(o, CM_SPIKE);
                        }
                    } else {
                        if ((org->isaconsumer) || (org->isafungus) || ((!org->isaplant) && ((o->isaconsumer) || (o->isafungus) || (org->iscoral)))) {
                            if (org_use_energy(o, P.spike_energy_consumption)) {
                                // Get energy depending on segment length
                                takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                                // The other organism will be shown in dark lilac
                                org_set_color(org, CM_DARKLILAC);
                                // This organism will be shown in spike
                                org_set_color(o, CM_SPIKE);
                            }
                        }
                    }
                }
            }
            break;
        case TY_MINT:
        case TY_LAVENDER:
        case TY_MAGENTA:
        case TY_ROSE:
            if (o->altruist) {
                break;
            } else {
                if (o->isenhanced) {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, CM_SPIKE);
                    } else {
                        if (org_use_energy(o, P.mosquito_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergySpike = bio_between((0.8 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in yellow
                            org_set_color(org, JC_YELLOW);
                            // This organism will be shown in spike
                            org_set_color(o, CM_SPIKE);
                        }
                    }
                } else {
                    if (o->isaplant) {
                        if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                            org_set_color(org, CM_TEAL);
                            org_set_color(o, CM_SPIKE);
                        } else {
                            if (org_use_energy(o, P.spike_energy_consumption)) {
                                // Get energy depending on segment length
                                takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                                // The other organism will be shown in dark lilac
                                org_set_color(org, CM_DARKLILAC);
                                // This organism will be shown in spike
                                org_set_color(o, CM_SPIKE);
                            }
                        }
                    } else {
                        if ((org->isaconsumer) || (org->isafungus) || ((!org->isaplant) && ((o->isaconsumer) || (o->isafungus) || (org->iscoral)))) {
                            if (org_use_energy(o, P.spike_energy_consumption)) {
                                // Get energy depending on segment length
                                takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                                // The other organism will be shown in dark lilac
                                org_set_color(org, CM_DARKLILAC);
                                // This organism will be shown in spike
                                org_set_color(o, CM_SPIKE);
                            }
                        }
                    }
                }
            }
            break;
        case TY_WHITE:
            if (o->isenhanced) {
                if ((org->isaplant) || (org->isaconsumer) || (org->isafungus) || (org->plagueversion > 0) || (org->isauburn)) {
                    if (org_use_energy(o, P.mosquito_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergySpike = bio_between((0.8 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in spike
                        org_set_color(o, CM_SPIKE);
                    }
                }
            } else {
                if (o->isaplant) {
                    if (org_use_energy(o, P.spike_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in dark lilac
                        org_set_color(org, CM_DARKLILAC);
                        // This organism will be shown in spike
                        org_set_color(o, CM_SPIKE);
                    }
                } else {
                    if ((org->isaconsumer) || (org->isafungus) || ((!org->isaplant) && ((o->isaconsumer) || (o->isafungus) || (org->iscoral)))) {
                        if (org_use_energy(o, P.spike_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in dark lilac
                            org_set_color(org, CM_DARKLILAC);
                            // This organism will be shown in spike
                            org_set_color(o, CM_SPIKE);
                        }
                    }
                }
            }
            break;
        case TY_GRASS:
            if (o->isenhanced) {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_SPIKE);
                } else {
                    if (org_use_energy(o, P.mosquito_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergySpike = bio_between((0.4 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in spike
                        org_set_color(o, CM_SPIKE);
                    }
                }
            } else {
                if (o->isaplant) {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, CM_SPIKE);
                    } else {
                        if (org_use_energy(o, P.spike_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergySpike = bio_between((2.5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in dark lilac
                            org_set_color(org, CM_DARKLILAC);
                            // This organism will be shown in spike
                            org_set_color(o, CM_SPIKE);
                        }
                    }
                } else {
                    if ((org->isaconsumer) || (org->isafungus) || ((!org->isaplant) && ((o->isaconsumer) || (o->isafungus) || (org->iscoral)))) {
                        if (org_use_energy(o, P.spike_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergySpike = bio_between((2.5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in dark lilac
                            org_set_color(org, CM_DARKLILAC);
                            // This organism will be shown in spike
                            org_set_color(o, CM_SPIKE);
                        }
                    }
                }
            }
            break;
        case TY_GREEN:
        case TY_FOREST:
        case TY_SPRING:
        case TY_SUMMER:
        case TY_WINTER:
        case TY_LIME:
        case TY_LEAF:
        case TY_C4:
        case TY_JADE:
        case TY_DARKJADE:
        case TY_DARKGREEN:
        case TY_PURPLE:
        case TY_TEAL:
        case TY_CYAN:
        case TY_AUBURN:
        case TY_INDIGO:
        case TY_BLOND:
        case TY_FLOWER:
        case TY_DARKGRAY:
        case TY_GOLD:
        case TY_SPORE:
            if (o->isenhanced) {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_SPIKE);
                } else {
                    if (org_use_energy(o, P.mosquito_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergySpike = bio_between((0.8 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in spike
                        org_set_color(o, CM_SPIKE);
                    }
                }
            } else {
                if (o->isaplant) {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, CM_SPIKE);
                    } else {
                        if (org_use_energy(o, P.spike_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in dark lilac
                            org_set_color(org, CM_DARKLILAC);
                            // This organism will be shown in spike
                            org_set_color(o, CM_SPIKE);
                        }
                    }
                } else {
                    if ((org->isaconsumer) || (org->isafungus) || ((!org->isaplant) && ((o->isaconsumer) || (o->isafungus) || (org->iscoral)))) {
                        if (org_use_energy(o, P.spike_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in dark lilac
                            org_set_color(org, CM_DARKLILAC);
                            // This organism will be shown in spike
                            org_set_color(o, CM_SPIKE);
                        }
                    }
                }
            }
            break;
        case TY_YELLOW:
            if (o->isenhanced) {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_SPIKE);
                } else {
                    if ((org->isaconsumer) || (org->isafungus) || (org->isaplant) || (org->transfersenergy)) {
                        if (org_use_energy(o, P.mosquito_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergySpike = bio_between((0.8 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in yellow
                            org_set_color(org, JC_YELLOW);
                            // This organism will be shown in spike
                            org_set_color(o, CM_SPIKE);
                        }
                    } else {
                        if (org_use_energy(o, P.mosquito_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergySpike = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in green brown
                            org_set_color(org, CM_GREENBROWN);
                            // This organism will be shown in spike
                            org_set_color(o, CM_SPIKE);
                        }
                    }
                }
            } else {
                if (o->isaplant) {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, CM_SPIKE);
                    } else {
                        if (org_use_energy(o, P.spike_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in dark lilac
                            org_set_color(org, CM_DARKLILAC);
                            // This organism will be shown in spike
                            org_set_color(o, CM_SPIKE);
                        }
                    }
                } else {
                    if ((org->isaconsumer) || (org->isafungus) || ((!org->isaplant) && ((o->isaconsumer) || (o->isafungus) || (org->iscoral)))) {
                        if (org_use_energy(o, P.spike_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in dark lilac
                            org_set_color(org, CM_DARKLILAC);
                            // This organism will be shown in spike
                            org_set_color(o, CM_SPIKE);
                        }
                    }
                }
            }
            break;
        case TY_FRUIT:
            if (o->isenhanced) {
                if (org->sporeversion == 2) {
                    if (org_use_energy(o, P.mosquito_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergySpike = bio_between((0.8 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        if ((o->infected_gc != org->gc) && (o->antiviral == 0)) {
                            // Infectious fruit
                            org_infected_by_fruit(o, org);
                            org_set_color(o, CM_BLOND);
                        } else {
                            // This organism will be shown in spike
                            org_set_color(o, CM_SPIKE);
                        }
                    }
                }
            } else {
                if (o->isaplant) {
                    if (org_use_energy(o, P.spike_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in dark lilac
                        org_set_color(org, CM_DARKLILAC);
                        // This organism will be shown in spike
                        org_set_color(o, CM_SPIKE);
                    }
                }
            }
            break;
        case TY_MAROON:
            if (o->isenhanced) {
                if (org_use_energy(o, P.mosquito_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergySpike = bio_between((0.8 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in spike
                    org_set_color(o, CM_SPIKE);
                }
            } else {
                if (o->isaplant) {
                    if (org_use_energy(o, P.spike_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in dark lilac
                        org_set_color(org, CM_DARKLILAC);
                        // This organism will be shown in spike
                        org_set_color(o, CM_SPIKE);
                    }
                } else {
                    if ((org->isaconsumer) || (org->isafungus) || ((!org->isaplant) && ((o->isaconsumer) || (o->isafungus) || (org->iscoral)))) {
                        if (org_use_energy(o, P.spike_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergySpike = bio_between((0.8 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in dark lilac
                            org_set_color(org, CM_DARKLILAC);
                            // This organism will be shown in spike
                            org_set_color(o, CM_SPIKE);
                        }
                    }
                }
            }
            break;
        case TY_SILVER:
            if (o->isenhanced) {
                if (org_use_energy(o, P.mosquito_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergySpike = bio_between((0.8 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    if ((o->total_kills >= org->total_kills) || ((!org->isaconsumer) && (!org->isafungus))) {
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                    } else {
                        // The other organism will be shown in gold
                        org_set_color(org, CM_GOLD);
                    }
                    // This organism will be shown in spike
                    org_set_color(o, CM_SPIKE);
                }
            } else {
                if (o->isaplant) {
                    if (org_use_energy(o, P.spike_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in dark lilac
                        org_set_color(org, CM_DARKLILAC);
                        // This organism will be shown in spike
                        org_set_color(o, CM_SPIKE);
                    }
                } else {
                    if ((org->isaconsumer) || (org->isafungus) || ((!org->isaplant) && ((o->isaconsumer) || (o->isafungus) || (org->iscoral)))) {
                        if (org_use_energy(o, P.spike_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in dark lilac
                            org_set_color(org, CM_DARKLILAC);
                            // This organism will be shown in spike
                            org_set_color(o, CM_SPIKE);
                        }
                    }
                }
            }
            break;
        case TY_PLAGUE:
            if (o->isenhanced) {
                if (org->isinfectious) {
                    if (org_use_energy(o, P.mosquito_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergySpike = bio_between((0.8 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in spike
                        org_set_color(o, CM_SPIKE);
                    }
                } else {
                    if (org_use_energy(o, P.mosquito_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergySpike = bio_between((0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in spike
                        org_set_color(o, CM_SPIKE);
                    }
                }
            } else {
                if (o->isaplant) {
                    if (org_use_energy(o, P.spike_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in dark lilac
                        org_set_color(org, CM_DARKLILAC);
                        // This organism will be shown in spike
                        org_set_color(o, CM_SPIKE);
                    }
                } else {
                    if ((org->isaconsumer) || (org->isafungus) || ((!org->isaplant) && ((o->isaconsumer) || (o->isafungus) || (org->iscoral)))) {
                        if (org_use_energy(o, P.spike_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in dark lilac
                            org_set_color(org, CM_DARKLILAC);
                            // This organism will be shown in spike
                            org_set_color(o, CM_SPIKE);
                        }
                    }
                }
            }
            break;
        case TY_FALLOW:
            if (!o->isenhanced) {
                if (o->isaplant) {
                    if (org_use_energy(o, P.spike_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in dark lilac
                        org_set_color(org, CM_DARKLILAC);
                        // This organism will be shown in spike
                        org_set_color(o, CM_SPIKE);
                    }
                } else {
                    if ((org->isaconsumer) || (org->isafungus) || ((!org->isaplant) && ((o->isaconsumer) || (o->isafungus) || (org->iscoral)))) {
                        if (org_use_energy(o, P.spike_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in dark lilac
                            org_set_color(org, CM_DARKLILAC);
                            // This organism will be shown in spike
                            org_set_color(o, CM_SPIKE);
                        }
                    }
                }
            }
            break;
        case TY_BARK:
            org->seg_color[oseg] = CM_OLDBARK;
            if (org->active) {
                org->photosynthesis -= org->mphoto[oseg] * k_scale[org->growth_ratio - 1];
            }
            break;
        case TY_OLIVE:
            break;
        default:
            if (o->isenhanced) {
                if (org_use_energy(o, P.mosquito_energy_consumption)) {
                    // Get energy depending on segment length
                    takenEnergySpike = bio_between((0.8 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in spike
                    org_set_color(o, CM_SPIKE);
                }
            } else {
                if (o->isaplant) {
                    if (org_use_energy(o, P.spike_energy_consumption)) {
                        // Get energy depending on segment length
                        takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in dark lilac
                        org_set_color(org, CM_DARKLILAC);
                        // This organism will be shown in spike
                        org_set_color(o, CM_SPIKE);
                    }
                } else {
                    if ((org->isaconsumer) || (org->isafungus) || ((!org->isaplant) && ((o->isaconsumer) || (o->isafungus) || (org->iscoral)))) {
                        if (org_use_energy(o, P.spike_energy_consumption)) {
                            // Get energy depending on segment length
                            takenEnergySpike = bio_between((5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in dark lilac
                            org_set_color(org, CM_DARKLILAC);
                            // This organism will be shown in spike
                            org_set_color(o, CM_SPIKE);
                        }
                    }
                }
            }
        }
        // energy interchange
        if (takenEnergySpike > 0) {
            if (o->isenhanced) {
                org->energy -= takenEnergySpike;
                o->energy += takenEnergySpike;
                takenEnergySpike = takenEnergySpike * P.organic_subs_produced;
                org_use_energy(o, takenEnergySpike);
            } else {
                org_use_energy(org, takenEnergySpike);
            }
        }
        break;
    case TY_OCHRE:
        // Ochre segment: Push other organisms away
        if (o->hasdodged == false) {
            o->hasdodged = true;
        }
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_OCHRE:
            if (jr_next_boolean(&g_rng)) {
                if ((o->isaplant && org_use_energy(o, P.ochre_energy_consumption)) || (!o->isaplant && org_use_energy(o, P.ochre_energy_consumption / 100))) {
                    if (!o->useframemovement) {
                        org_standochre(o);
                    }
                    org->dx = bio_between((org->center_x - o->center_x) * 0.2 * o->m[seg] * o->m[seg] / org->mass, -P.max_vel, P.max_vel);
                    org->dy = bio_between((org->center_y - o->center_y) * 0.2 * o->m[seg] * o->m[seg] / org->mass, -P.max_vel, P.max_vel);
                    if (o->isenhanced) {
                        if (org->lavender > 0) {
                            org_lavendershield(org);
                        }
                        if (org->lavender <= 0) {
                            if (org->isenhanced) {
                                org_use_energy(org, bio_between((fabs(org->dx) + fabs(org->dy)) * (P.organic_obtained_energy / 250), 0, org->energy));
                            } else {
                                org_use_energy(org, bio_between(2 * (fabs(org->dx) + fabs(org->dy)) * P.organic_obtained_energy, 0, org->energy));
                            }
                            org_set_color(org, CM_DARKLILAC);
                        }
                    }
                    org_set_color(o, CM_OCHRE);
                }
            }
            break;
        case TY_BLUE:
            if (jr_next_boolean(&g_rng)) {
                if ((o->isaplant && org_use_energy(o, P.ochre_energy_consumption)) || (!o->isaplant && org_use_energy(o, P.ochre_energy_consumption / 100))) {
                    if (!o->useframemovement) {
                        org_standochre(o);
                    }
                    org->dx = bio_between((org->center_x - o->center_x) * 0.2 * o->m[seg] * o->m[seg] / org->mass, -P.max_vel, P.max_vel);
                    org->dy = bio_between((org->center_y - o->center_y) * 0.2 * o->m[seg] * o->m[seg] / org->mass, -P.max_vel, P.max_vel);
                    if (o->isenhanced) {
                        if (org_use_energy(org, P.blue_energy_consumption)) {
                            org_set_color(org, JC_BLUE);
                        } else {
                            if (org->lavender > 0) {
                                org_lavendershield(org);
                            }
                            if (org->lavender <= 0) {
                                org_use_energy(org, bio_between(2 * (fabs(org->dx) + fabs(org->dy)) * P.organic_obtained_energy, 0, org->energy));
                                org_set_color(org, CM_DARKLILAC);
                            }
                        }
                    }
                    org_set_color(o, CM_OCHRE);
                }
            }
            break;
        case TY_RED:
            if (jr_next_boolean(&g_rng)) {
                if ((o->isaplant && org_use_energy(o, P.ochre_energy_consumption)) || (!o->isaplant && org_use_energy(o, P.ochre_energy_consumption / 100))) {
                    if (!o->useframemovement) {
                        org_standochre(o);
                    }
                    org->dx = bio_between((org->center_x - o->center_x) * 0.2 * o->m[seg] * o->m[seg] / org->mass, -P.max_vel, P.max_vel);
                    org->dy = bio_between((org->center_y - o->center_y) * 0.2 * o->m[seg] * o->m[seg] / org->mass, -P.max_vel, P.max_vel);
                    if ((o->isenhanced) && (org->createlavender == 0)) {
                        if (org->lavender > 0) {
                            org_lavendershield(org);
                        }
                        if (org->lavender <= 0) {
                            org_use_energy(org, bio_between(2 * (fabs(org->dx) + fabs(org->dy)) * P.organic_obtained_energy, 0, org->energy));
                            org_set_color(org, CM_DARKLILAC);
                        }
                    }
                    org_set_color(o, CM_OCHRE);
                }
            }
            break;
        case TY_MAROON:
            if (jr_next_boolean(&g_rng)) {
                if ((o->isaplant && org_use_energy(o, P.ochre_energy_consumption)) || (!o->isaplant && org_use_energy(o, P.ochre_energy_consumption / 100))) {
                    if (!o->useframemovement) {
                        org_standochre(o);
                    }
                    org->dx = bio_between((org->center_x - o->center_x) * 0.2 * o->m[seg] * o->m[seg] / org->mass, -P.max_vel, P.max_vel);
                    org->dy = bio_between((org->center_y - o->center_y) * 0.2 * o->m[seg] * o->m[seg] / org->mass, -P.max_vel, P.max_vel);
                    if (o->isenhanced) {
                        if (org->lavender > 0) {
                            org_lavendershield(org);
                        }
                        if (org->lavender <= 0) {
                            if (org->isenhanced) {
                                org_use_energy(org, bio_between(0.5 * (fabs(org->dx) + fabs(org->dy)) * P.organic_obtained_energy, 0, org->energy));
                            } else {
                                org_use_energy(org, bio_between((fabs(org->dx) + fabs(org->dy)) * P.organic_obtained_energy, 0, org->energy));
                            }
                            org_set_color(org, CM_DARKLILAC);
                        }
                    }
                    org_set_color(o, CM_OCHRE);
                }
            }
            break;
        case TY_CREAM:
        case TY_CORAL:
            if (jr_next_boolean(&g_rng)) {
                if ((o->isaplant && org_use_energy(o, P.ochre_energy_consumption)) || (!o->isaplant && org_use_energy(o, P.ochre_energy_consumption / 100))) {
                    if (!o->useframemovement) {
                        org_standochre(o);
                    }
                    org->dx = bio_between((org->center_x - o->center_x) * 0.2 * o->m[seg] * o->m[seg] / org->mass, -P.max_vel, P.max_vel);
                    org->dy = bio_between((org->center_y - o->center_y) * 0.2 * o->m[seg] * o->m[seg] / org->mass, -P.max_vel, P.max_vel);
                    org_set_color(o, CM_OCHRE);
                }
            }
            break;
        case TY_SKY:
        case TY_DEEPSKY:
            if (jr_next_boolean(&g_rng)) {
                if ((o->isaplant && org_use_energy(o, P.ochre_energy_consumption)) || (!o->isaplant && org_use_energy(o, P.ochre_energy_consumption / 100))) {
                    if (!o->useframemovement) {
                        org_standochre(o);
                    }
                    if ((org->skyversion == 1) && (o->isaplant)) {
                        org_set_color(o, CM_OCHRE);
                        org_set_color(org, CM_DEEPSKY);
                    } else {
                        org->dx = bio_between((org->center_x - o->center_x) * 0.2 * o->m[seg] * o->m[seg] / org->mass, -P.max_vel, P.max_vel);
                        org->dy = bio_between((org->center_y - o->center_y) * 0.2 * o->m[seg] * o->m[seg] / org->mass, -P.max_vel, P.max_vel);
                        if (o->isenhanced) {
                            if (org->lavender > 0) {
                                org_lavendershield(org);
                            }
                            if (org->lavender <= 0) {
                                org_use_energy(org, bio_between(2 * (fabs(org->dx) + fabs(org->dy)) * P.organic_obtained_energy, 0, org->energy));
                                org_set_color(org, CM_DARKLILAC);
                            }
                        }
                        org_set_color(o, CM_OCHRE);
                    }
                }
            }
            break;
        case TY_BROWN:
            if ((org->alive) && (jr_next_boolean(&g_rng))) {
                if ((o->isaplant && org_use_energy(o, P.ochre_energy_consumption)) || (!o->isaplant && org_use_energy(o, P.ochre_energy_consumption / 100))) {
                    if (!o->useframemovement) {
                        org_standochre(o);
                    }
                    org->dx = bio_between((org->center_x - o->center_x) * 0.2 * o->m[seg] * o->m[seg] / org->mass, -P.max_vel, P.max_vel);
                    org->dy = bio_between((org->center_y - o->center_y) * 0.2 * o->m[seg] * o->m[seg] / org->mass, -P.max_vel, P.max_vel);
                    if (o->isenhanced) {
                        if (org->lavender > 0) {
                            org_lavendershield(org);
                        }
                        if (org->lavender <= 0) {
                            org_use_energy(org, bio_between(2 * (fabs(org->dx) + fabs(org->dy)) * P.organic_obtained_energy, 0, org->energy));
                            org_set_color(org, CM_DARKLILAC);
                            if (!org->isenhanced) {
                                org->isinjured = true;
                            }
                        }
                    }
                    org_set_color(o, CM_OCHRE);
                }
            }
            break;
        case TY_OLIVE:
            break;
        default:
            if (jr_next_boolean(&g_rng)) {
                if ((o->isaplant && org_use_energy(o, P.ochre_energy_consumption)) || (!o->isaplant && org_use_energy(o, P.ochre_energy_consumption / 100))) {
                    if (!o->useframemovement) {
                        org_standochre(o);
                    }
                    org->dx = bio_between((org->center_x - o->center_x) * 0.2 * o->m[seg] * o->m[seg] / org->mass, -P.max_vel, P.max_vel);
                    org->dy = bio_between((org->center_y - o->center_y) * 0.2 * o->m[seg] * o->m[seg] / org->mass, -P.max_vel, P.max_vel);
                    if (o->isenhanced) {
                        if (org->lavender > 0) {
                            org_lavendershield(org);
                        }
                        if (org->lavender <= 0) {
                            org_use_energy(org, bio_between(2 * (fabs(org->dx) + fabs(org->dy)) * P.organic_obtained_energy, 0, org->energy));
                            org_set_color(org, CM_DARKLILAC);
                        }
                    }
                    org_set_color(o, CM_OCHRE);
                }
            }
        }
        break;
    case TY_SILVER:;
        // Silver segment: infects all other organism, if it has more infections , duels with other absorbing segments
        double takenEnergy = 0;
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_SILVER:
            if ((org->infected_gc != o->gc) && (org->total_infected < o->total_infected)) {
                if ((org->isaplant) || (org->isaconsumer) || ((org->isafungus) && (org->total_kills > 0))) {
                    if (org_use_energy(o, P.silver_energy_consumption)) {
                        org_infected_by_organism(org, o);
                        org_set_color(org, CM_BLOND);
                        org_set_color(o, JC_LIGHT_GRAY);
                    }
                }
            }
            if ((org->indigo > 0) && (o->total_infected > 0) && (!org->isaplant) && (!org->isaconsumer) && (!org->isafungus)) {
                if ((o->indigo == 0) || (o->isaplant) || (o->isaconsumer) || (o->isafungus)) {
                    if (org_use_energy(o, (P.silver_energy_consumption + 0.01) / P.indigo_energy_consumption)) {
                        org_set_color(org, CM_INDIGO);
                        org_set_color(o, JC_LIGHT_GRAY);
                    }
                }
            }
            if (((o->total_kills > 0) && ((o->isaconsumer) || ((o->isafungus) && (o->isaconsumer = true)))) || (o->isenhanced)) {
                if (((org->total_kills > 0) && ((org->isaconsumer) || (org->isafungus))) || (org->isenhanced)) {
                    if (org_use_energy(o, P.experience_energy_consumption)) {
                        // Get energy depending on segment length and relation between kills of both organisms
                        takenEnergy = bio_between(((o->total_kills + 12) / (org->total_kills + 12)) * (sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in gold
                        org_set_color(org, CM_GOLD);
                        // This organism will be shown in gold
                        org_set_color(o, CM_GOLD);
                    }
                } else {
                    if (org_use_energy(o, P.experience_energy_consumption)) {
                        // Get energy depending on segment length and relation between kills of both organisms
                        takenEnergy = bio_between(((o->total_kills + 12) / (org->total_kills + 12)) * (sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in gold
                        org_set_color(o, CM_GOLD);
                    }
                }
            }
            break;
        case TY_WHITE:
        case TY_PLAGUE:
            if ((org->infected_gc != o->gc) && (org->total_infected < o->total_infected)) {
                if ((org->isaplant) || (org->isaconsumer)) {
                    if (org_use_energy(o, P.silver_energy_consumption)) {
                        org_infected_by_organism(org, o);
                        org_set_color(org, CM_BLOND);
                        org_set_color(o, JC_LIGHT_GRAY);
                    }
                }
            }
            if ((org->indigo > 0) && (o->total_infected > 0) && (!org->isaplant) && (!org->isaconsumer) && (!org->isafungus)) {
                if ((o->indigo == 0) || (o->isaplant) || (o->isaconsumer) || (o->isafungus)) {
                    if (org_use_energy(o, (P.silver_energy_consumption + 0.01) / P.indigo_energy_consumption)) {
                        org_set_color(org, CM_INDIGO);
                        org_set_color(o, JC_LIGHT_GRAY);
                    }
                }
            }
            if ((org->isaplant) || (org->isaconsumer) || (org->isafungus) || (org->plagueversion > 0) || (org->isauburn)) {
                if (((o->total_kills > 0) && ((o->isaconsumer) || ((o->isafungus) && (o->isaconsumer = true)))) || (o->isenhanced)) {
                    if (org_use_energy(o, P.experience_energy_consumption)) {
                        // Get energy depending on segment length and relation between kills of both organisms
                        takenEnergy = bio_between(((o->total_kills + 12) / (org->total_kills + 12)) * (sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in gold
                        org_set_color(o, CM_GOLD);
                    }
                }
            }
            break;
        case TY_FRUIT:
            if (org->sporeversion == 2) {
                if (((o->total_kills > 0) && ((o->isaconsumer) || ((o->isafungus) && (o->isaconsumer = true)))) || (o->isenhanced)) {
                    if (org_use_energy(o, P.experience_energy_consumption)) {
                        // Get energy depending on segment length and relation between kills of both organisms
                        takenEnergy = bio_between(((o->total_kills + 12) / (org->total_kills + 12)) * (0.1 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        if (o->infected_gc != org->gc) {
                            // Infectious fruit
                            org_infected_by_fruit(o, org);
                            org_set_color(o, CM_BLOND);
                        } else {
                            // This organism will be shown in gold
                            org_set_color(o, CM_GOLD);
                        }
                    }
                }
            }
            break;
        case TY_INDIGO:
            if (o->total_infected > 0) {
                if ((o->indigo == 0) || (o->isaplant) || (o->isaconsumer) || (o->isafungus)) {
                    if (org_use_energy(o, (P.silver_energy_consumption + 0.01) / P.indigo_energy_consumption)) {
                        org_set_color(org, CM_INDIGO);
                        org_set_color(o, JC_LIGHT_GRAY);
                    }
                }
            }
            if (((o->total_kills > 0) && ((o->isaconsumer) || ((o->isafungus) && (o->isaconsumer = true)))) || (o->isenhanced)) {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_GOLD);
                } else {
                    if (org_use_energy(o, P.experience_energy_consumption)) {
                        // Get energy depending on segment length and relation between kills of both organisms
                        takenEnergy = bio_between(((o->total_kills + 12) / (org->total_kills + 12)) * (sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in gold
                        org_set_color(o, CM_GOLD);
                    }
                }
            }
            break;
        case TY_GRASS:
            if ((org->infected_gc != o->gc) && (org->total_infected < o->total_infected)) {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org->hasdodged = true;
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, JC_LIGHT_GRAY);
                } else {
                    if (org->jadefactor > 1) {
                        org->seg_color[oseg] = CM_DARKGREEN;
                        org_set_color(o, JC_LIGHT_GRAY);
                        org->hasdodged = true;
                        org->useextraeffects = true;
                    } else {
                        if (org_use_energy(o, P.silver_energy_consumption)) {
                            org_infected_by_organism(org, o);
                            org_set_color(org, CM_BLOND);
                            org_set_color(o, JC_LIGHT_GRAY);
                        }
                    }
                }
            }
            if (((o->total_kills > 0) && ((o->isaconsumer) || ((o->isafungus) && (o->isaconsumer = true)))) || (o->isenhanced)) {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_GOLD);
                } else {
                    if (org_use_energy(o, P.experience_energy_consumption)) {
                        // Get energy depending on segment length and relation between kills of both organisms
                        takenEnergy = bio_between(((o->total_kills + 12) / (org->total_kills + 12)) * (0.5 * sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in green brown
                        org_set_color(org, CM_GREENBROWN);
                        // This organism will be shown in gold
                        org_set_color(o, CM_GOLD);
                    }
                }
            }
            break;
        case TY_GREEN:
        case TY_FOREST:
        case TY_SPRING:
        case TY_SUMMER:
        case TY_WINTER:
        case TY_LIME:
        case TY_LEAF:
        case TY_C4:
        case TY_JADE:
        case TY_DARKJADE:
            if ((org->infected_gc != o->gc) && (org->total_infected < o->total_infected)) {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    if ((!org->isonlyc4) || ((!o->isaplant) && ((!org->isauburn) || (org->frames_color > 8))) || ((org->frames_color > 8) && (!org->isauburn))) {
                        org->hasdodged = true;
                    }
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, JC_LIGHT_GRAY);
                } else {
                    if (org->jadefactor > 1) {
                        if (org->seg_color[oseg] == CM_JADE) {
                            org->seg_color[oseg] = CM_DARKJADE;
                        } else {
                            org->seg_color[oseg] = CM_DARKGREEN;
                        }
                        org_set_color(o, JC_LIGHT_GRAY);
                        org->hasdodged = true;
                        org->useextraeffects = true;
                    } else {
                        if (org_use_energy(o, P.silver_energy_consumption)) {
                            org_infected_by_organism(org, o);
                            org_set_color(org, CM_BLOND);
                            org_set_color(o, JC_LIGHT_GRAY);
                        }
                    }
                }
            }
            if (((o->total_kills > 0) && ((o->isaconsumer) || ((o->isafungus) && (o->isaconsumer = true)))) || (o->isenhanced)) {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_GOLD);
                } else {
                    if (org_use_energy(o, P.experience_energy_consumption)) {
                        // Get energy depending on segment length and relation between kills of both organisms
                        takenEnergy = bio_between(((o->total_kills + 12) / (org->total_kills + 12)) * (sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in gold
                        org_set_color(o, CM_GOLD);
                    }
                }
            }
            break;
        case TY_DARKGREEN:
        case TY_PURPLE:
        case TY_TEAL:
        case TY_CYAN:
        case TY_YELLOW:
        case TY_AUBURN:
        case TY_BLOND:
        case TY_FLOWER:
        case TY_DARKGRAY:
        case TY_GOLD:
        case TY_SPORE:
            if ((org->infected_gc != o->gc) && (org->total_infected < o->total_infected)) {
                if ((org->isaplant) || (org->isaconsumer)) {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        if ((!org->isonlyc4) || ((!o->isaplant) && ((!org->isauburn) || (org->frames_color > 8))) || ((org->frames_color > 8) && (!org->isauburn))) {
                            org->hasdodged = true;
                        }
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, JC_LIGHT_GRAY);
                    } else {
                        if (org_use_energy(o, P.silver_energy_consumption)) {
                            org_infected_by_organism(org, o);
                            org_set_color(org, CM_BLOND);
                            org_set_color(o, JC_LIGHT_GRAY);
                        }
                    }
                }
            }
            if (((o->total_kills > 0) && ((o->isaconsumer) || ((o->isafungus) && (o->isaconsumer = true)))) || (o->isenhanced)) {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_GOLD);
                } else {
                    if (org_use_energy(o, P.experience_energy_consumption)) {
                        // Get energy depending on segment length and relation between kills of both organisms
                        takenEnergy = bio_between(((o->total_kills + 12) / (org->total_kills + 12)) * (sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in gold
                        org_set_color(o, CM_GOLD);
                    }
                }
            }
            break;
        case TY_RED:
            if ((org->infected_gc != o->gc) && (org->total_infected < o->total_infected)) {
                if (org_use_energy(o, P.silver_energy_consumption)) {
                    org_infected_by_organism(org, o);
                    org_set_color(org, CM_BLOND);
                    org_set_color(o, JC_LIGHT_GRAY);
                }
            }
            if (((o->total_kills > 0) && ((o->isaconsumer) || ((o->isafungus) && (o->isaconsumer = true)))) || (o->isenhanced)) {
                if (org_use_energy(o, P.experience_energy_consumption)) {
                    // Get energy depending on segment length and relation between kills of both organisms
                    takenEnergy = bio_between(((o->total_kills + 12) / (org->total_kills + 12)) * (sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in red
                    org_set_color(org, JC_RED);
                    // This organism will be shown in gold
                    org_set_color(o, CM_GOLD);
                }
            }
            break;
        case TY_FIRE:
            if ((org->infected_gc != o->gc) && (org->total_infected < o->total_infected)) {
                if (org_use_energy(o, P.silver_energy_consumption)) {
                    org_infected_by_organism(org, o);
                    org_set_color(org, CM_BLOND);
                    org_set_color(o, JC_LIGHT_GRAY);
                }
            }
            if (((o->total_kills > 0) && ((o->isaconsumer) || ((o->isafungus) && (o->isaconsumer = true)))) || (o->isenhanced)) {
                if (org_use_energy(o, P.experience_energy_consumption)) {
                    // Get energy depending on segment length and relation between kills of both organisms
                    takenEnergy = bio_between(((o->total_kills + 12) / (org->total_kills + 12)) * (sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in fire
                    org_set_color(org, CM_FIRE);
                    // This organism will be shown in gold
                    org_set_color(o, CM_GOLD);
                }
            }
            break;
        case TY_ORANGE:
            if ((org->infected_gc != o->gc) && (org->total_infected < o->total_infected)) {
                if (org_use_energy(o, P.silver_energy_consumption)) {
                    org_infected_by_organism(org, o);
                    org_set_color(org, CM_BLOND);
                    org_set_color(o, JC_LIGHT_GRAY);
                }
            }
            if (((o->total_kills > 0) && ((o->isaconsumer) || ((o->isafungus) && (o->isaconsumer = true)))) || (o->isenhanced)) {
                if (org_use_energy(o, P.experience_energy_consumption)) {
                    // Get energy depending on segment length and relation between kills of both organisms
                    takenEnergy = bio_between(((o->total_kills + 12) / (org->total_kills + 12)) * (sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in orange
                    org_set_color(org, JC_ORANGE);
                    // This organism will be shown in gold
                    org_set_color(o, CM_GOLD);
                }
            }
            break;
        case TY_PINK:
            if ((org->infected_gc != o->gc) && (org->total_infected < o->total_infected)) {
                if (gc_get_modifiespink(org->gc)) {
                    if (org_use_energy(o, P.silver_energy_consumption)) {
                        org_infected_by_organism(org, o);
                        org_set_color(org, CM_BLOND);
                        org_set_color(o, JC_LIGHT_GRAY);
                    }
                }
            }
            if (((o->total_kills > 0) && ((o->isaconsumer) || ((o->isafungus) && (o->isaconsumer = true)))) || (o->isenhanced)) {
                if (org_use_energy(o, P.experience_energy_consumption)) {
                    // Get energy depending on segment length and relation between kills of both organisms
                    takenEnergy = bio_between(((o->total_kills + 12) / (org->total_kills + 12)) * (sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in gold
                    org_set_color(o, CM_GOLD);
                }
            }
            break;
        case TY_SPIKE:
            if ((org->infected_gc != o->gc) && (org->total_infected < o->total_infected)) {
                if ((org->isaplant) || (org->isaconsumer)) {
                    if (org_use_energy(o, P.silver_energy_consumption)) {
                        org_infected_by_organism(org, o);
                        org_set_color(org, CM_BLOND);
                        org_set_color(o, JC_LIGHT_GRAY);
                    }
                }
            }
            if ((org->isaplant) || (org->isenhanced) || (org->total_kills < o->total_kills)) {
                if (((o->total_kills > 0) && ((o->isaconsumer) || ((o->isafungus) && (o->isaconsumer = true)))) || (o->isenhanced)) {
                    if (org_use_energy(o, P.experience_energy_consumption)) {
                        // Get energy depending on segment length and relation between kills of both organisms
                        takenEnergy = bio_between(((o->total_kills + 12) / (org->total_kills + 12)) * (sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in gold
                        org_set_color(o, CM_GOLD);
                    }
                }
            }
            break;
        case TY_SPIKEPOINT:
            if (org->total_kills < o->total_kills) {
                if (((o->total_kills > 0) && ((o->isaconsumer) || ((o->isafungus) && (o->isaconsumer = true)))) || (o->isenhanced)) {
                    if (org_use_energy(o, P.experience_energy_consumption)) {
                        // Get energy depending on segment length and relation between kills of both organisms
                        takenEnergy = bio_between(((o->total_kills + 12) / (org->total_kills + 12)) * (sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        if (org->isenhanced) {
                            // The other organism will be shown in spike
                            org_set_color(org, CM_SPIKE);
                        } else {
                            // The other organism will be shown in yellow
                            org_set_color(org, JC_YELLOW);
                        }
                        // This organism will be shown in gold
                        org_set_color(o, CM_GOLD);
                    }
                }
            }
            break;
        case TY_LAVENDER:
        case TY_MINT:
            if (o->altruist) {
                break;
            } else {
                if (((o->total_kills > 0) && ((o->isaconsumer) || ((o->isafungus) && (o->isaconsumer = true)))) || (o->isenhanced)) {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, CM_GOLD);
                    } else {
                        if (org_use_energy(o, P.experience_energy_consumption)) {
                            // Get energy depending on segment length and relation between kills of both organisms
                            takenEnergy = bio_between(((o->total_kills + 12) / (org->total_kills + 12)) * (sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in yellow
                            org_set_color(org, JC_YELLOW);
                            // This organism will be shown in gold
                            org_set_color(o, CM_GOLD);
                        }
                    }
                }
            }
            break;
        case TY_MAGENTA:
        case TY_ROSE:
            if (o->altruist) {
                break;
            } else {
                if ((org->infected_gc != o->gc) && (org->total_infected < o->total_infected)) {
                    if ((org->isaplant) || (org->isaconsumer)) {
                        if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                            if ((!org->isonlyc4) || ((!o->isaplant) && ((!org->isauburn) || (org->frames_color > 8))) || ((org->frames_color > 8) && (!org->isauburn))) {
                                org->hasdodged = true;
                            }
                            org_set_color(org, CM_TEAL);
                            org_set_color(o, JC_LIGHT_GRAY);
                        } else {
                            if (org_use_energy(o, P.silver_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_BLOND);
                                org_set_color(o, JC_LIGHT_GRAY);
                            }
                        }
                    }
                }
                if (((o->total_kills > 0) && ((o->isaconsumer) || ((o->isafungus) && (o->isaconsumer = true)))) || (o->isenhanced)) {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, CM_GOLD);
                    } else {
                        if (org_use_energy(o, P.experience_energy_consumption)) {
                            // Get energy depending on segment length and relation between kills of both organisms
                            takenEnergy = bio_between(((o->total_kills + 12) / (org->total_kills + 12)) * (sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                            // The other organism will be shown in yellow
                            org_set_color(org, JC_YELLOW);
                            // This organism will be shown in gold
                            org_set_color(o, CM_GOLD);
                        }
                    }
                }
            }
            break;
        case TY_BLUE:
            if ((org->infected_gc != o->gc) && (org->total_infected < o->total_infected)) {
                if ((org->isaplant) || (org->isaconsumer)) {
                    if (org_use_energy(o, P.silver_energy_consumption)) {
                        org_infected_by_organism(org, o);
                        org_set_color(org, CM_BLOND);
                        org_set_color(o, JC_LIGHT_GRAY);
                    }
                }
            }
            if (((o->total_kills > 0) && ((o->isaconsumer) || ((o->isafungus) && (o->isaconsumer = true)))) || (o->isenhanced)) {
                if (org_use_energy(org, P.blue_energy_consumption)) {
                    if (org->isenhanced) {
                        org_use_energy(o, bio_between((0.5 * sqrt(org->m[oseg])) * P.organic_obtained_energy, 0, o->energy));
                        org_set_color(o, CM_DARKLILAC);
                        if (o->energy < BIO_TOL) {
                            org_die(o, org);
                        }
                    } else {
                        org_set_color(o, CM_GOLD);
                    }
                    org_set_color(org, JC_BLUE);
                } else {
                    if (org_use_energy(o, P.experience_energy_consumption)) {
                        // Get energy depending on segment length and relation between kills of both organisms
                        takenEnergy = bio_between(((o->total_kills + 12) / (org->total_kills + 12)) * (sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                        // The other organism will be shown in yellow
                        org_set_color(org, JC_YELLOW);
                        // This organism will be shown in gold
                        org_set_color(o, CM_GOLD);
                    }
                }
            }
            break;
        case TY_CREAM:
            if ((org->infected_gc != o->gc) && (org->total_infected < o->total_infected)) {
                if (org->isaplant) {
                    if (org_use_energy(o, P.silver_energy_consumption)) {
                        org_infected_by_organism(org, o);
                        org_set_color(org, CM_BLOND);
                        org_set_color(o, JC_LIGHT_GRAY);
                    }
                }
            }
            break;
        case TY_MAROON:
        case TY_CORAL:
        case TY_OCHRE:
        case TY_SKY:
        case TY_OLIVE:
        case TY_OLDBARK:
        case TY_FALLOW:
            if ((org->infected_gc != o->gc) && (org->total_infected < o->total_infected)) {
                if ((org->isaplant) || (org->isaconsumer)) {
                    if (org_use_energy(o, P.silver_energy_consumption)) {
                        org_infected_by_organism(org, o);
                        org_set_color(org, CM_BLOND);
                        org_set_color(o, JC_LIGHT_GRAY);
                    }
                }
            }
            break;
        case TY_BARK:
            org->seg_color[oseg] = CM_OLDBARK;
            if (org->active) {
                org->photosynthesis -= org->mphoto[oseg] * k_scale[org->growth_ratio - 1];
            }
            if ((org->infected_gc != o->gc) && (org->total_infected < o->total_infected)) {
                if ((!o->isaplant) && (!o->isaconsumer) && (!o->isafungus)) {
                    if (org_use_energy(o, P.silver_energy_consumption)) {
                        org_infected_by_organism(org, o);
                        org_set_color(org, CM_BLOND);
                        org_set_color(o, JC_LIGHT_GRAY);
                    }
                }
            }
            break;
        case TY_BROWN:
            break;
        default:
            if ((org->infected_gc != o->gc) && (org->total_infected < o->total_infected)) {
                if ((org->isaplant) || (org->isaconsumer)) {
                    if (org_use_energy(o, P.silver_energy_consumption)) {
                        org_infected_by_organism(org, o);
                        org_set_color(org, CM_BLOND);
                        org_set_color(o, JC_LIGHT_GRAY);
                    }
                }
            }
            if (((o->total_kills > 0) && ((o->isaconsumer) || ((o->isafungus) && (o->isaconsumer = true)))) || (o->isenhanced)) {
                if (org_use_energy(o, P.experience_energy_consumption)) {
                    // Get energy depending on segment length and relation between kills of both organisms
                    takenEnergy = bio_between(((o->total_kills + 12) / (org->total_kills + 12)) * (sqrt(o->m[seg])) * P.organic_obtained_energy, 0, org->energy);
                    // The other organism will be shown in yellow
                    org_set_color(org, JC_YELLOW);
                    // This organism will be shown in gold
                    org_set_color(o, CM_GOLD);
                }
            }
        }
        if (org->hasdodged == false) {
            if (((o->total_kills > 0) && (o->isaconsumer)) || (o->isenhanced)) {
                org->hasdodged = true;
            }
        }
        // energy interchange
        if (takenEnergy > 0) {
            org->energy -= takenEnergy;
            o->energy += takenEnergy;
            takenEnergy = takenEnergy * P.organic_subs_produced;
            org_use_energy(o, takenEnergy);
        }
        break;
    case TY_CORAL:
        // Transform viruses and particles into children
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_CORAL:
            if ((!org->isaplant) && (!org->isaconsumer) && ((!org->isafungus) || ((o->isenhanced) && (!org->issilver)))) {
                if ((org->indigo > 0) && (!org->isafungus) && ((o->indigo == 0) || (o->isaplant) || (o->isaconsumer) || (o->isafungus))) {
                    if (org_use_energy(o, (P.coral_energy_consumption + 0.1) / P.indigo_energy_consumption)) {
                        org_set_color(org, CM_INDIGO);
                        org_set_color(o, CM_CORAL);
                    }
                }
            }
            break;
        case TY_WHITE:
        case TY_SILVER:
        case TY_PLAGUE:
            if ((!org->isaplant) && (!org->isaconsumer) && ((!org->isafungus) || ((o->isenhanced) && (!org->issilver)))) {
                if ((!org->transfersenergy) || (org->isinfectious) || (org->iscoral) || (org->isafungus)) {
                    if ((org->indigo > 0) && (!org->isafungus)) {
                        if (org_use_energy(o, (P.coral_energy_consumption + 0.01) / P.indigo_energy_consumption)) {
                            org_set_color(org, CM_INDIGO);
                            org_set_color(o, CM_CORAL);
                        }
                        if (org->antiviral > 0) {
                            org_virusneutralized(o);
                        }
                    } else {
                        if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                            org_set_color(org, CM_TEAL);
                            org_set_color(o, CM_CORAL);
                            org->hasdodged = true;
                        } else {
                            if (org->antiviral > 0) {
                                org_virusneutralized(o);
                            }
                            if (org->lavender > 0) {
                                if ((o->isauburn) && (!o->isaplant) && (!o->isaconsumer) && (!o->isafungus) && (!org->isafungus)) {
                                    org_weaklavendershield(org);
                                } else {
                                    org_lavendershield(org);
                                }
                                org_set_color(o, CM_CORAL);
                                org->hasdodged = true;
                            }
                            if (org->lavender <= 0) {
                                if (org_use_energy(o, P.coral_energy_consumption)) {
                                    org_set_color(o, CM_CORAL);
                                    org_infected_by_organism(org, o);
                                    if (org->energy >= 1) {
                                        org_transform(org);
                                    }
                                    org_die(org, o);
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_INDIGO:
            if ((!org->isaplant) && (!org->isaconsumer) && ((!org->isafungus) || ((o->isenhanced) && (!org->issilver)))) {
                if (!org->isinfectious) {
                    if (org_use_energy(o, (P.coral_energy_consumption + 0.1) / P.indigo_energy_consumption)) {
                        org_set_color(org, CM_INDIGO);
                        org_set_color(o, CM_CORAL);
                    }
                    if (org->antiviral > 0) {
                        org_virusneutralized(o);
                    }
                } else {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, CM_CORAL);
                        org->hasdodged = true;
                    } else {
                        if (org->antiviral > 0) {
                            org_virusneutralized(o);
                        }
                        if (org->lavender > 0) {
                            if ((o->isauburn) && (!o->isaplant) && (!o->isaconsumer) && (!o->isafungus) && (!org->isafungus)) {
                                org_weaklavendershield(org);
                            } else {
                                org_lavendershield(org);
                            }
                            org_set_color(o, CM_CORAL);
                            org->hasdodged = true;
                        }
                        if (org->lavender <= 0) {
                            if (org_use_energy(o, P.coral_energy_consumption)) {
                                org_set_color(o, CM_CORAL);
                                org_infected_by_organism(org, o);
                                if (org->energy >= 1) {
                                    org_transform(org);
                                }
                                org_die(org, o);
                            }
                        }
                    }
                }
            }
            break;
        case TY_TEAL:
        case TY_CYAN:
        case TY_AUBURN:
        case TY_BLOND:
        case TY_FLOWER:
            if ((!org->isaplant) && (!org->isaconsumer) && ((!org->isafungus) || ((o->isenhanced) && (!org->issilver)))) {
                if ((!org->transfersenergy) || (org->isinfectious) || (org->iscoral) || (org->isafungus)) {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, CM_CORAL);
                        org->hasdodged = true;
                    } else {
                        if (org->antiviral > 0) {
                            org_virusneutralized(o);
                        }
                        if (org->lavender > 0) {
                            if ((o->isauburn) && (!o->isaplant) && (!o->isaconsumer) && (!o->isafungus) && (!org->isafungus)) {
                                org_weaklavendershield(org);
                            } else {
                                org_lavendershield(org);
                            }
                            org_set_color(o, CM_CORAL);
                            org->hasdodged = true;
                        }
                        if (org->lavender <= 0) {
                            if (org_use_energy(o, P.coral_energy_consumption)) {
                                org_set_color(o, CM_CORAL);
                                org_infected_by_organism(org, o);
                                if (org->energy >= 1) {
                                    org_transform(org);
                                }
                                org_die(org, o);
                            }
                        }
                    }
                }
            }
            break;
        case TY_YELLOW:
        case TY_DARKGRAY:
        case TY_GOLD:
        case TY_VISION:
            if ((!org->isaplant) && (!org->isaconsumer) && ((!org->isafungus) || ((o->isenhanced) && (!org->issilver))) && (org->active)) {
                if ((!org->transfersenergy) || (org->isinfectious) || (org->iscoral) || (org->isafungus)) {
                    if (org->antiviral > 0) {
                        org_virusneutralized(o);
                    }
                    if (org->isafungus) {
                        if (org->lavender > 0) {
                            if ((o->isauburn) && (!o->isaplant) && (!o->isaconsumer) && (!o->isafungus) && (!org->isafungus)) {
                                org_weaklavendershield(org);
                            } else {
                                org_lavendershield(org);
                            }
                            org_set_color(o, CM_CORAL);
                        }
                        if (org->lavender <= 0) {
                            if (org_use_energy(o, P.coral_energy_consumption)) {
                                org_set_color(o, CM_CORAL);
                                org_infected_by_organism(org, o);
                                if (org->energy >= 1) {
                                    org_transform(org);
                                }
                                org_die(org, o);
                            }
                        }
                    }
                }
            }
            break;
        case TY_SPORE:
            if ((!org->isaplant) && (!org->isaconsumer) && ((!org->isafungus) || ((o->isenhanced) && (!org->issilver)))) {
                if ((!org->transfersenergy) || (org->isinfectious) || (org->iscoral) || (org->isafungus)) {
                    if (org->antiviral > 0) {
                        org_virusneutralized(o);
                    }
                    if (org->sporeversion == 6) {
                        if (org->lavender > 0) {
                            if ((o->isauburn) && (!o->isaplant) && (!o->isaconsumer) && (!o->isafungus) && (!org->isafungus)) {
                                org_weaklavendershield(org);
                            } else {
                                org_lavendershield(org);
                            }
                            org_set_color(o, CM_CORAL);
                        }
                        if (org->lavender <= 0) {
                            if (org_use_energy(o, P.coral_energy_consumption)) {
                                org_set_color(o, CM_CORAL);
                                org_infected_by_organism(org, o);
                                if (org->energy >= 1) {
                                    org_transform(org);
                                }
                                org_die(org, o);
                            }
                        }
                    }
                }
            }
            break;
        case TY_LIGHTBROWN:
        case TY_GREENBROWN:
        case TY_DARKFIRE:
        case TY_BROKEN:
        case TY_ICE:
            if ((!org->isaplant) && (!org->isaconsumer) && ((!org->isafungus) || ((o->isenhanced) && (!org->issilver)))) {
                if ((!org->transfersenergy) || (org->isinfectious) || (org->iscoral) || (org->isafungus)) {
                    if (org_use_energy(o, P.coral_energy_consumption)) {
                        org_set_color(o, CM_CORAL);
                        org_infected_by_organism(org, o);
                        if (org->energy >= 1) {
                            org_transform(org);
                        }
                        org_die(org, o);
                    }
                }
            }
            break;
        case TY_MAGENTA:
        case TY_ROSE:
            if (o->altruist) {
                break;
            } else {
                if ((!org->isaplant) && (!org->isaconsumer) && ((!org->isafungus) || ((o->isenhanced) && (!org->issilver)))) {
                    if ((!org->transfersenergy) || (org->isinfectious) || (org->iscoral) || (org->isafungus)) {
                        if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                            org_set_color(org, CM_TEAL);
                            org_set_color(o, CM_CORAL);
                            org->hasdodged = true;
                        } else {
                            if (org->antiviral > 0) {
                                org_virusneutralized(o);
                            }
                            if (org->lavender > 0) {
                                if ((o->isauburn) && (!o->isaplant) && (!o->isaconsumer) && (!o->isafungus) && (!org->isafungus)) {
                                    org_weaklavendershield(org);
                                } else {
                                    org_lavendershield(org);
                                }
                                org_set_color(o, CM_CORAL);
                                org->hasdodged = true;
                            }
                            if (org->lavender <= 0) {
                                if (org_use_energy(o, P.coral_energy_consumption)) {
                                    org_set_color(o, CM_CORAL);
                                    org_infected_by_organism(org, o);
                                    if (org->energy >= 1) {
                                        org_transform(org);
                                    }
                                    org_die(org, o);
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_LAVENDER:
            if ((!org->isaplant) && (!org->isaconsumer) && ((!org->isafungus) || ((o->isenhanced) && (!org->issilver)))) {
                if ((!org->transfersenergy) || (org->isinfectious) || (org->iscoral) || (org->isafungus)) {
                    if (org->antiviral > 0) {
                        org_virusneutralized(o);
                    }
                    if (org->isenhanced) {
                        org_use_energy(o, bio_between((1 / P.lavender_energy_consumption), 0, o->energy));
                        if (org->lavender < P.lavender_shield) {
                            org->lavender += 1000;
                            if (org->lavender >= P.lavender_shield) {
                                org->lavender = P.lavender_shield;
                            }
                        }
                    } else {
                        org_use_energy(o, bio_between((0.2 / P.lavender_energy_consumption), 0, o->energy));
                        if (org->lavender < P.lavender_shield) {
                            org->lavender += 200;
                            if (org->lavender >= P.lavender_shield) {
                                org->lavender = P.lavender_shield;
                            }
                        }
                    }
                    // The other organism will be shown in lavender
                    org_set_color(org, CM_LAVENDER);
                    // This organism will be shown in deadbark
                    org_set_color(o, CM_DEADBARK);
                    if (o->energy < BIO_TOL) {
                        org_die(o, org);
                    }
                }
            }
            break;
        case TY_BLUE:
        case TY_SKY:
        case TY_DEEPSKY:
        case TY_OCHRE:
        case TY_OLIVE:
        case TY_DARKOLIVE:
        case TY_VIOLET:
        case TY_GRAY:
        case TY_LILAC:
        case TY_DARKLILAC:
        case TY_SPIKE:
        case TY_SPIKEPOINT:
        case TY_FALLOW:
        case TY_MINT:
            if ((!org->isaplant) && (!org->isaconsumer) && ((!org->isafungus) || ((o->isenhanced) && (!org->issilver))) && (org->active)) {
                if ((!org->transfersenergy) || (org->isinfectious) || (org->iscoral) || (org->isafungus)) {
                    if (org->antiviral > 0) {
                        org_virusneutralized(o);
                    }
                }
            }
            break;
        case TY_FRUIT:
            if (org->sporeversion == 1) {
                if ((org->frames_color > 0) || (o->isaplant) || ((o->isauburn) && (!o->isaconsumer) && (!o->isafungus))) {
                    if (org_use_energy(o, P.coral_energy_consumption)) {
                        org_set_color(o, CM_CORAL);
                        org_infected_by_organism(org, o);
                        if (org->energy >= 1) {
                            org_transform(org);
                        }
                        org_die(org, o);
                    }
                } else {
                    org_set_color(org, CM_BROKEN);
                }
            }
            break;
        case TY_BARK:
        case TY_OLDBARK:
        case TY_DEADBARK:
        case TY_BROWN:
            break;
        default:
            if ((!org->isaplant) && (!org->isaconsumer) && ((!org->isafungus) || ((o->isenhanced) && (!org->issilver)))) {
                if ((!org->transfersenergy) || (org->isinfectious) || (org->iscoral) || (org->isafungus)) {
                    if (org->antiviral > 0) {
                        org_virusneutralized(o);
                    }
                    if (org->lavender > 0) {
                        if ((o->isauburn) && (!o->isaplant) && (!o->isaconsumer) && (!o->isafungus) && (!org->isafungus)) {
                            org_weaklavendershield(org);
                        } else {
                            org_lavendershield(org);
                        }
                        org_set_color(o, CM_CORAL);
                    }
                    if (org->lavender <= 0) {
                        if (org_use_energy(o, P.coral_energy_consumption)) {
                            org_set_color(o, CM_CORAL);
                            org_infected_by_organism(org, o);
                            if (org->energy >= 1) {
                                org_transform(org);
                            }
                            org_die(org, o);
                        }
                    }
                }
            }
        }
        break;
    }
    // Check if the other organism has died
    if (org_is_alive(org) && org->energy < BIO_TOL) {
        org_die(org, o);
    }
}

/* Applies the hostile effects produced by two touching segments.
 * Version for segments, that cannot drain energy
 * 
 * @param org  The organism which is touching.
 * @param seg  Index of this organism's segment.
 * @param oseg  Index of the other organism's segment.
 */
static void org_touch_effects2(Organism *o, Organism *org, int seg, int oseg)
{
    switch (org_get_type_color(o->seg_color[seg])) {
    case TY_WHITE:
        // White segment: try to infect the other organism
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_WHITE:
        case TY_GREENBROWN:
            if (org->infected_gc != o->gc) {
                if ((org->isaplant) || (org->isaconsumer)) {
                    if (o->isaplant) {
                        if (org_use_energy(o, P.white_energy_consumption)) {
                            org_infected_by_organism(org, o);
                            org_set_color(org, CM_BLOND);
                            org_set_color(o, JC_WHITE);
                        }
                    } else {
                        if (org_use_energy(o, P.virus_energy_consumption)) {
                            org_infected_by_organism(org, o);
                            org_set_color(org, CM_LIGHTBROWN);
                            org_set_color(o, JC_WHITE);
                        }
                    }
                } else {
                    if ((org->indigo > 0) && (!org->isafungus)) {
                        if (o->isaplant) {
                            if (org_use_energy(o, (P.white_energy_consumption + 0.01) / P.indigo_energy_consumption)) {
                                org_set_color(org, CM_INDIGO);
                                org_set_color(o, JC_WHITE);
                            }
                        } else {
                            if ((o->indigo == 0) || (o->isaconsumer) || (o->isafungus)) {
                                if (org_use_energy(o, (P.virus_energy_consumption + 0.1) / P.indigo_energy_consumption)) {
                                    org_set_color(org, CM_INDIGO);
                                    org_set_color(o, JC_WHITE);
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_SILVER:
            if (org->infected_gc != o->gc) {
                if ((org->isaplant) || (org->isaconsumer) || ((org->isafungus) && (org->total_kills > 0))) {
                    if ((org->isenhanced) || (o->total_infected >= org->total_infected)) {
                        if (o->isaplant) {
                            if (org_use_energy(o, P.white_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_BLOND);
                                org_set_color(o, JC_WHITE);
                            }
                        } else {
                            if (org_use_energy(o, P.virus_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_LIGHTBROWN);
                                org_set_color(o, JC_WHITE);
                            }
                        }
                    }
                } else {
                    if ((org->indigo > 0) && (!org->isafungus)) {
                        if (o->isaplant) {
                            if (org_use_energy(o, (P.white_energy_consumption + 0.01) / P.indigo_energy_consumption)) {
                                org_set_color(org, CM_INDIGO);
                                org_set_color(o, JC_WHITE);
                            }
                        } else {
                            if ((o->indigo == 0) || (o->isaconsumer) || (o->isafungus)) {
                                if (org_use_energy(o, (P.virus_energy_consumption + 0.1) / P.indigo_energy_consumption)) {
                                    org_set_color(org, CM_INDIGO);
                                    org_set_color(o, JC_WHITE);
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_PLAGUE:
            if (org->infected_gc != o->gc) {
                if ((org->isaplant) || (org->isaconsumer)) {
                    if (o->isaplant) {
                        if (org_use_energy(o, P.white_energy_consumption)) {
                            org_infected_by_organism(org, o);
                            org_set_color(org, CM_BLOND);
                            org_set_color(o, JC_WHITE);
                            if ((!org->isinfectious) && (!org->isaconsumer) && (!org->isafungus)) {
                                org->infected_reproduce_energy = 4;
                            }
                        }
                    } else {
                        if (org_use_energy(o, P.virus_energy_consumption)) {
                            org_infected_by_organism(org, o);
                            org_set_color(org, CM_LIGHTBROWN);
                            org_set_color(o, JC_WHITE);
                            if ((!org->isinfectious) && (!org->isaconsumer) && (!org->isafungus)) {
                                org->infected_reproduce_energy = 4;
                            }
                        }
                    }
                } else {
                    if ((org->indigo > 0) && (!org->isafungus)) {
                        if (o->isaplant) {
                            if (org_use_energy(o, (P.white_energy_consumption + 0.01) / P.indigo_energy_consumption)) {
                                org_set_color(org, CM_INDIGO);
                                org_set_color(o, JC_WHITE);
                            }
                        } else {
                            if ((o->indigo == 0) || (o->isaconsumer) || (o->isafungus)) {
                                if (org_use_energy(o, (P.virus_energy_consumption + 0.1) / P.indigo_energy_consumption)) {
                                    org_set_color(org, CM_INDIGO);
                                    org_set_color(o, JC_WHITE);
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_INDIGO:
            if (o->isaplant) {
                if (org_use_energy(o, (P.white_energy_consumption + 0.01) / P.indigo_energy_consumption)) {
                    org_set_color(org, CM_INDIGO);
                    org_set_color(o, JC_WHITE);
                }
            } else {
                if ((o->indigo == 0) || (o->isaconsumer) || (o->isafungus)) {
                    if (org_use_energy(o, (P.virus_energy_consumption + 0.1) / P.indigo_energy_consumption)) {
                        org_set_color(org, CM_INDIGO);
                        org_set_color(o, JC_WHITE);
                    }
                }
            }
            if ((org->antiviral > 0) && (o->plagueversion > 0)) {
                org_virusneutralized(o);
            }
            break;
        case TY_GREEN:
        case TY_GRASS:
        case TY_FOREST:
        case TY_SPRING:
        case TY_SUMMER:
        case TY_WINTER:
        case TY_LIME:
        case TY_LEAF:
        case TY_C4:
        case TY_JADE:
        case TY_DARKJADE:
            if (org->infected_gc != o->gc) {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    if ((!org->isonlyc4) || ((!o->isaplant) && ((!org->isauburn) || (org->frames_color > 8))) || ((org->frames_color > 8) && (!org->isauburn))) {
                        org->hasdodged = true;
                    }
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, JC_WHITE);
                } else {
                    if (org->jadefactor > 1) {
                        if (org->seg_color[oseg] == CM_JADE) {
                            org->seg_color[oseg] = CM_DARKJADE;
                        } else {
                            org->seg_color[oseg] = CM_DARKGREEN;
                        }
                        org_set_color(o, JC_WHITE);
                        org->hasdodged = true;
                        org->useextraeffects = true;
                        if ((org->antiviral > 0) && (o->plagueversion > 0)) {
                            org_virusneutralized(o);
                        }
                    } else {
                        if (o->isaplant) {
                            if (org_use_energy(o, P.white_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_BLOND);
                                org_set_color(o, JC_WHITE);
                            }
                        } else {
                            if (org_use_energy(o, P.virus_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_LIGHTBROWN);
                                org_set_color(o, JC_WHITE);
                            }
                        }
                    }
                }
            }
            break;
        case TY_DARKGREEN:
        case TY_PURPLE:
        case TY_TEAL:
        case TY_CYAN:
        case TY_YELLOW:
        case TY_AUBURN:
        case TY_BLOND:
        case TY_FLOWER:
        case TY_DARKGRAY:
        case TY_GOLD:
            if (org->infected_gc != o->gc) {
                if ((org->isaplant) || (org->isaconsumer)) {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        if ((!org->isonlyc4) || ((!o->isaplant) && ((!org->isauburn) || (org->frames_color > 8))) || ((org->frames_color > 8) && (!org->isauburn))) {
                            org->hasdodged = true;
                        }
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, JC_WHITE);
                    } else {
                        if (o->isaplant) {
                            if (org_use_energy(o, P.white_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_BLOND);
                                org_set_color(o, JC_WHITE);
                            }
                        } else {
                            if (org_use_energy(o, P.virus_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_LIGHTBROWN);
                                org_set_color(o, JC_WHITE);
                            }
                        }
                    }
                }
            }
            break;
        case TY_SPORE:
            if (org->infected_gc != o->gc) {
                if ((org->isaplant) || (org->isaconsumer)) {
                    if ((org->sporeversion >= 3) && (org->isaplant)) {
                        if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                            if ((!org->isonlyc4) || ((!o->isaplant) && ((!org->isauburn) || (org->frames_color > 8))) || ((org->frames_color > 8) && (!org->isauburn))) {
                                org->hasdodged = true;
                            }
                            org_set_color(org, CM_TEAL);
                            org_set_color(o, JC_WHITE);
                        } else {
                            if (o->isaplant) {
                                if (org_use_energy(o, P.white_energy_consumption)) {
                                    org_infected_by_organism(org, o);
                                    org_set_color(org, CM_BLOND);
                                    org_set_color(o, JC_WHITE);
                                    if ((!org->isaconsumer) && (!org->isafungus)) {
                                        org->infected_reproduce_energy = -org->infected_reproduce_energy;
                                    }
                                }
                            } else {
                                if (org_use_energy(o, P.virus_energy_consumption)) {
                                    org_infected_by_organism(org, o);
                                    org_set_color(org, CM_LIGHTBROWN);
                                    org_set_color(o, JC_WHITE);
                                    if ((!org->isaconsumer) && (!org->isafungus)) {
                                        org->infected_reproduce_energy = -org->infected_reproduce_energy;
                                    }
                                }
                            }
                        }
                    } else {
                        org_set_color(o, JC_WHITE);
                        org_set_color(org, CM_SPORE);
                    }
                }
            }
            break;
        case TY_BLUE:
            if (org->infected_gc != o->gc) {
                if ((org->isaplant) || (org->isaconsumer)) {
                    if (org_use_energy(org, P.blue_energy_consumption)) {
                        if (org->isenhanced) {
                            org_use_energy(o, bio_between((0.5 * sqrt(org->m[oseg])) * P.organic_obtained_energy, 0, o->energy));
                            org_set_color(o, CM_DARKLILAC);
                            if (o->energy < BIO_TOL) {
                                org_die(o, org);
                            }
                        } else {
                            org_set_color(o, JC_WHITE);
                        }
                        org_set_color(org, JC_BLUE);
                        org->hasdodged = true;
                    } else {
                        if (o->isaplant) {
                            if (org_use_energy(o, P.white_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_BLOND);
                                org_set_color(o, JC_WHITE);
                            }
                        } else {
                            if (org_use_energy(o, P.virus_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_LIGHTBROWN);
                                org_set_color(o, JC_WHITE);
                            }
                        }
                    }
                }
            }
            break;
        case TY_LIGHT_BLUE:
            if (org->infected_gc != o->gc) {
                if ((org->isaplant) || (org->isaconsumer)) {
                    if ((!o->isenhanced) && (o->isaplant) && (org_use_energy(org, P.blue_energy_consumption))) {
                        org_set_color(o, JC_WHITE);
                        org_set_color(org, CM_LIGHT_BLUE);
                        org->hasdodged = true;
                    } else {
                        if (o->isaplant) {
                            if (org_use_energy(o, P.white_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_BLOND);
                                org_set_color(o, JC_WHITE);
                            }
                        } else {
                            if (org_use_energy(o, P.virus_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_LIGHTBROWN);
                                org_set_color(o, JC_WHITE);
                            }
                        }
                    }
                }
            }
            break;
        case TY_SKY:
            if ((((org->skyversion == 2) || (o->isaplant) || (o->isaconsumer) || (o->isafungus)) && ((!o->isfrozen) || (o->healing > 0))) || ((o->isenhanced) && (org->isaplant) && (!o->isakiller) && (org->antiviral == 0))) {
                if ((org->isaplant) || ((org->isaconsumer) && (org->skyversion == 2))) {
                    if (org_use_energy(org, P.sky_energy_consumption)) {
                        org->seg_color[oseg] = CM_DEEPSKY;
                        org->useextraeffects = true;
                        org_set_color(o, JC_WHITE);
                        org_set_color(org, CM_DEEPSKY);
                        org->hasdodged = true;
                    } else {
                        if (org->infected_gc != o->gc) {
                            if (o->isaplant) {
                                if (org_use_energy(o, P.white_energy_consumption)) {
                                    org_infected_by_organism(org, o);
                                    org_set_color(org, CM_BLOND);
                                    org_set_color(o, JC_WHITE);
                                }
                            } else {
                                if (org_use_energy(o, P.virus_energy_consumption)) {
                                    org_infected_by_organism(org, o);
                                    org_set_color(org, CM_LIGHTBROWN);
                                    org_set_color(o, JC_WHITE);
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_DEEPSKY:
            if ((((org->skyversion == 2) || (o->isaplant) || (o->isaconsumer) || (o->isafungus)) && ((!o->isfrozen) || (o->healing > 0))) || ((o->isenhanced) && (org->isaplant) && (!o->isakiller) && (org->antiviral == 0))) {
                if ((org->isaplant) || ((org->isaconsumer) && (org->skyversion == 2))) {
                    if ((o->isenhanced) && (org->isaplant) && (!o->isakiller) && (org->antiviral == 0)) {
                        if (org->infected_gc != o->gc) {
                            if (o->isaplant) {
                                if (org_use_energy(o, P.white_energy_consumption)) {
                                    org_infected_by_organism(org, o);
                                    org_set_color(org, CM_BLOND);
                                    org_set_color(o, JC_WHITE);
                                }
                            } else {
                                if (org_use_energy(o, P.virus_energy_consumption)) {
                                    org_infected_by_organism(org, o);
                                    org_set_color(org, CM_LIGHTBROWN);
                                    org_set_color(o, JC_WHITE);
                                }
                            }
                        }
                    } else {
                        org_set_color(o, JC_WHITE);
                        org_set_color(org, CM_DEEPSKY);
                        org->hasdodged = true;
                    }
                }
            }
            break;
        case TY_OLIVE:
        case TY_DARKOLIVE:
            if (org->infected_gc != o->gc) {
                if ((org->isaplant) || (org->isaconsumer)) {
                    if (o->isaplant) {
                        if ((o->isenhanced) && (org->isaplant) && (!o->isakiller) && (org->antiviral == 0)) {
                            if (org_use_energy(o, P.white_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_BLOND);
                                org_set_color(o, JC_WHITE);
                            }
                        }
                    } else {
                        if (org_use_energy(o, P.virus_energy_consumption)) {
                            org_infected_by_organism(org, o);
                            org_set_color(org, CM_LIGHTBROWN);
                            org_set_color(o, JC_WHITE);
                        }
                    }
                }
            }
            break;
        case TY_VIOLET:
            if (org->infected_gc != o->gc) {
                if ((org->isaplant) || (org->isaconsumer)) {
                    if (o->isaplant) {
                        if ((org->isaconsumer) || ((o->isenhanced) && (!o->isakiller) && (org->antiviral == 0))) {
                            if (org_use_energy(o, P.white_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_BLOND);
                                org_set_color(o, JC_WHITE);
                            }
                        }
                    } else {
                        if (org_use_energy(o, P.virus_energy_consumption)) {
                            org_infected_by_organism(org, o);
                            org_set_color(org, CM_LIGHTBROWN);
                            org_set_color(o, JC_WHITE);
                        }
                    }
                } else {
                    if ((org->isafungus) && (!org->issilver)) {
                        if ((o->isauburn) && (!o->isaplant) && (!o->isaconsumer) && (!o->isafungus)) {
                            if (org_use_energy(o, P.virus_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_LIGHTBROWN);
                                org_set_color(o, JC_WHITE);
                            }
                        }
                    }
                }
            }
            break;
        case TY_LIGHTBROWN:
            if (org->infected_gc != o->gc) {
                if ((org->isaplant) || (org->isaconsumer)) {
                    if (o->isaplant) {
                        if (org_use_energy(o, P.white_energy_consumption)) {
                            org_infected_by_organism(org, o);
                            org_set_color(org, CM_BLOND);
                            org_set_color(o, JC_WHITE);
                        }
                    } else {
                        if (org_use_energy(o, P.virus_energy_consumption)) {
                            org_infected_by_organism(org, o);
                            org_set_color(org, CM_LIGHTBROWN);
                            org_set_color(o, JC_WHITE);
                        }
                    }
                } else {
                    if ((org->isafungus) && (!org->issilver)) {
                        if ((o->isauburn) && (!o->isaplant) && (!o->isaconsumer) && (!o->isafungus)) {
                            if (org_use_energy(o, P.virus_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_LIGHTBROWN);
                                org_set_color(o, JC_WHITE);
                            }
                        }
                    }
                }
            }
            break;
        case TY_ICE:
        case TY_DEADBARK:
            if (org->infected_gc != o->gc) {
                if ((org->isaplant) || (org->isaconsumer)) {
                    if (o->isaplant) {
                        if ((org->isaconsumer) || ((o->isenhanced) && (!o->isakiller))) {
                            if (org_use_energy(o, P.white_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_BLOND);
                                org_set_color(o, JC_WHITE);
                            }
                        }
                    } else {
                        if (org_use_energy(o, P.virus_energy_consumption)) {
                            org_infected_by_organism(org, o);
                            org_set_color(org, CM_LIGHTBROWN);
                            org_set_color(o, JC_WHITE);
                        }
                    }
                }
            }
            break;
        case TY_LAVENDER:
            if ((org->isaplant) || (org->isaconsumer)) {
                if ((o->plagueversion > 0) || (o->fallowversion > 0) || (org->isenhanced)) {
                    if (org->isenhanced) {
                        org_use_energy(o, bio_between((1 / P.lavender_energy_consumption), 0, o->energy));
                        if (org->lavender < P.lavender_shield) {
                            org->lavender += 1000;
                            if (org->lavender >= P.lavender_shield) {
                                org->lavender = P.lavender_shield;
                            }
                        }
                    } else {
                        org_use_energy(o, bio_between((0.2 / P.lavender_energy_consumption), 0, o->energy));
                        if (org->lavender < P.lavender_shield) {
                            org->lavender += 200;
                            if (org->lavender >= P.lavender_shield) {
                                org->lavender = P.lavender_shield;
                            }
                        }
                    }
                } else {
                    org_use_energy(o, bio_between((0.02 / P.lavender_energy_consumption), 0, o->energy));
                    if (org->lavender < P.lavender_shield) {
                        org->lavender += 20;
                        if (org->lavender >= P.lavender_shield) {
                            org->lavender = P.lavender_shield;
                        }
                    }
                }
                if (org->infected_gc != NULL) {
                    org->infected_gc = NULL;
                    org->saved_gc = NULL;
                }
                // The other organism will be shown in lavender
                org_set_color(org, CM_LAVENDER);
                // This organism will be shown in deadbark
                org_set_color(o, CM_DEADBARK);
                org->hasdodged = true;
                if (o->energy < BIO_TOL) {
                    org_die(o, org);
                }
            }
            break;
        case TY_MAGENTA:
        case TY_ROSE:
            if ((!o->altruist) && (org->infected_gc != o->gc)) {
                if ((org->isaplant) || (org->isaconsumer)) {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        if ((!org->isonlyc4) || ((!o->isaplant) && ((!org->isauburn) || (org->frames_color > 8))) || ((org->frames_color > 8) && (!org->isauburn))) {
                            org->hasdodged = true;
                        }
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, JC_WHITE);
                    } else {
                        if (o->isaplant) {
                            if (org_use_energy(o, P.white_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_BLOND);
                                org_set_color(o, JC_WHITE);
                            }
                        } else {
                            if (org_use_energy(o, P.virus_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_LIGHTBROWN);
                                org_set_color(o, JC_WHITE);
                            }
                        }
                    }
                }
            }
            break;
        case TY_PINK:
            if ((gc_get_modifiespink(org->gc)) || ((org->time_to_reproduce > org->time_to_reproduce_max) && (org->reproducelate <= 0))) {
                if (org->infected_gc != o->gc) {
                    if (o->isaplant) {
                        if (org_use_energy(o, P.white_energy_consumption)) {
                            org_infected_by_organism(org, o);
                            org_set_color(org, CM_BLOND);
                            org_set_color(o, JC_WHITE);
                        }
                    } else {
                        if (org_use_energy(o, P.virus_energy_consumption)) {
                            org_infected_by_organism(org, o);
                            org_set_color(org, CM_LIGHTBROWN);
                            org_set_color(o, JC_WHITE);
                        }
                    }
                }
            } else {
                if ((o->isauburn) && (!o->isaplant) && (!o->isaconsumer) && (!o->isafungus)) {
                    if (org->infected_gc != o->gc) {
                        if (org_use_energy(o, P.virus_energy_consumption)) {
                            org_infected_by_organism(org, o);
                            org_set_color(org, CM_LIGHTBROWN);
                            org_set_color(o, JC_WHITE);
                        }
                    }
                }
            }
            break;
        case TY_CREAM:
            if (org->indigo == 0) {
                if (org->isaplant) {
                    if (org->infected_gc != o->gc) {
                        if (o->isaplant) {
                            if ((o->isenhanced) && (!o->isakiller)) {
                                if ((!org->isenhanced) || (gc_get_modifiescream(org->gc) != 2)) {
                                    if (org_use_energy(o, P.white_energy_consumption)) {
                                        org_infected_by_organism(org, o);
                                        org_set_color(org, CM_BLOND);
                                        org_set_color(o, JC_WHITE);
                                    }
                                }
                            }
                        } else {
                            if (org_use_energy(o, P.virus_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_LIGHTBROWN);
                                org_set_color(o, JC_WHITE);
                            }
                        }
                    }
                } else {
                    if ((o->isenhanced) && (!o->isakiller)) {
                        if ((!o->isaplant) && (!o->isaconsumer) && (!o->isafungus)) {
                            if ((!org->isenhanced) || (gc_get_modifiescream(org->gc) != 2)) {
                                if (org->infected_gc != o->gc) {
                                    if (org_use_energy(o, P.virus_energy_consumption)) {
                                        org_infected_by_organism(org, o);
                                        org_set_color(org, CM_LIGHTBROWN);
                                        org_set_color(o, JC_WHITE);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_SPIKE:
            if (org->infected_gc != o->gc) {
                if ((org->isaplant) || (org->isenhanced)) {
                    if (o->isaplant) {
                        if (org_use_energy(o, P.white_energy_consumption)) {
                            org_infected_by_organism(org, o);
                            org_set_color(org, CM_BLOND);
                            org_set_color(o, JC_WHITE);
                        }
                    } else {
                        if (org_use_energy(o, P.virus_energy_consumption)) {
                            org_infected_by_organism(org, o);
                            org_set_color(org, CM_LIGHTBROWN);
                            org_set_color(o, JC_WHITE);
                        }
                    }
                }
            }
            break;
        case TY_SPIKEPOINT:
            if ((o->isenhanced) && (!o->isakiller)) {
                if (((org->isaplant) && (org->antiviral == 0)) || (org->isenhanced)) {
                    if (org->infected_gc != o->gc) {
                        if (o->isaplant) {
                            if (org_use_energy(o, P.white_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_BLOND);
                                org_set_color(o, JC_WHITE);
                            }
                        } else {
                            if (org_use_energy(o, P.virus_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_LIGHTBROWN);
                                org_set_color(o, JC_WHITE);
                            }
                        }
                    }
                }
            }
            break;
        case TY_CORAL:
            if (org->infected_gc != o->gc) {
                if ((org->isaplant) || (org->isaconsumer)) {
                    if (o->isaplant) {
                        if (org_use_energy(o, P.white_energy_consumption)) {
                            org_infected_by_organism(org, o);
                            org_set_color(org, CM_BLOND);
                            org_set_color(o, JC_WHITE);
                        }
                    } else {
                        if ((o->isaconsumer) || (!o->isblond)) {
                            if (org_use_energy(o, P.virus_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_LIGHTBROWN);
                                org_set_color(o, JC_WHITE);
                            }
                        }
                    }
                }
            }
            break;
        case TY_FALLOW:
            if ((o->isenhanced) && (org->isaplant) && (!o->isakiller) && (org->antiviral == 0)) {
                if ((!o->isaplant) && (!o->isaconsumer) && (!o->isafungus)) {
                    if (org->infected_gc != o->gc) {
                        if ((org->isaplant) || (org->isaconsumer)) {
                            if (org_use_energy(o, P.virus_energy_consumption)) {
                                org_infected_by_organism(org, o);
                                org_set_color(org, CM_LIGHTBROWN);
                                org_set_color(o, JC_WHITE);
                            }
                        }
                    }
                }
            }
            break;
        case TY_BARK:
            org->seg_color[oseg] = CM_OLDBARK;
            if (org->active) {
                org->photosynthesis -= org->mphoto[oseg] * k_scale[org->growth_ratio - 1];
            }
            break;
        case TY_OLDBARK:
        case TY_OCHRE:
        case TY_MINT:
        case TY_BROWN:
            break;
        default:
            if (org->infected_gc != o->gc) {
                if ((org->isaplant) || (org->isaconsumer)) {
                    if (o->isaplant) {
                        if (org_use_energy(o, P.white_energy_consumption)) {
                            org_infected_by_organism(org, o);
                            org_set_color(org, CM_BLOND);
                            org_set_color(o, JC_WHITE);
                        }
                    } else {
                        if (org_use_energy(o, P.virus_energy_consumption)) {
                            org_infected_by_organism(org, o);
                            org_set_color(org, CM_LIGHTBROWN);
                            org_set_color(o, JC_WHITE);
                        }
                    }
                }
            }
        }
        break;
    case TY_FRUIT:;
        // Fruit segment: Infect the other organism, if the other organism tries to eat it, or infect and integrate with plants
        double takenEnergyFruit = 0;
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_GREEN:
        case TY_GRASS:
        case TY_FOREST:
        case TY_SPRING:
        case TY_SUMMER:
        case TY_WINTER:
        case TY_LIME:
        case TY_LEAF:
        case TY_C4:
        case TY_JADE:
        case TY_DARKJADE:
            if ((org->infected_gc != o->gc) && (o->sporeversion == 1)) {
                if ((org->infected_gc != NULL) || (org->sporeversion != 1)) {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        if ((!org->isonlyc4) || (org->frames_color > 8)) {
                            org->hasdodged = true;
                        }
                        org_set_color(org, CM_TEAL);
                    } else {
                        if ((org->antiviral > 0) && (org_use_energy(org, P.mint_energy_consumption / 10))) {
                            for (int a = 0; a < o->segments; a++) {
                                switch (org_get_type_color(o->seg_color[a])) {
                                case TY_FRUIT:
                                case TY_DARKFIRE:
                                    o->seg_color[a] = CM_BROKEN;
                                    break;
                                }
                            }
                            org_set_color(org, CM_MINT);
                            o->frames_color = 0;
                            o->isinjured = true;
                            o->isfrozen = true;
                            o->allfrozen = true;
                        } else {
                            if (org->jadefactor > 1) {
                                if (org->seg_color[oseg] == CM_JADE) {
                                    org->seg_color[oseg] = CM_DARKJADE;
                                } else {
                                    org->seg_color[oseg] = CM_DARKGREEN;
                                }
                                org->hasdodged = true;
                                org->useextraeffects = true;
                            } else {
                                org_infected_by_fruit(org, o);
                                takenEnergyFruit = bio_between(o->energy - 0.0000002, 0, o->energy);
                                org->energy += takenEnergyFruit;
                                o->energy -= takenEnergyFruit;
                                org_set_color(org, CM_LIGHTBROWN);
                                org->time_to_reproduce = 0;
                            }
                        }
                    }
                }
            }
            break;
        case TY_DARKGREEN:
        case TY_PURPLE:
        case TY_TEAL:
        case TY_CYAN:
        case TY_YELLOW:
        case TY_AUBURN:
        case TY_BLOND:
        case TY_FLOWER:
        case TY_DARKGRAY:
        case TY_GOLD:
            if ((org->infected_gc != o->gc) && (o->sporeversion == 1)) {
                if (org->isaplant) {
                    if ((org->infected_gc != NULL) || (org->sporeversion != 1)) {
                        if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                            if ((!org->isonlyc4) || (org->frames_color > 8)) {
                                org->hasdodged = true;
                            }
                            org_set_color(org, CM_TEAL);
                        } else {
                            org_infected_by_fruit(org, o);
                            takenEnergyFruit = bio_between(o->energy - 0.0000002, 0, o->energy);
                            org->energy += takenEnergyFruit;
                            o->energy -= takenEnergyFruit;
                            org_set_color(org, CM_LIGHTBROWN);
                            org->time_to_reproduce = 0;
                        }
                    }
                }
            }
            break;
        case TY_SPORE:
            if ((org->infected_gc != o->gc) && (o->sporeversion == 1)) {
                if (org->isaplant) {
                    if (((org->infected_gc != NULL) || (org->sporeversion != 1)) && (org->sporeversion > 0)) {
                        if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                            if ((!org->isonlyc4) || (org->frames_color > 8)) {
                                org->hasdodged = true;
                            }
                            org_set_color(org, CM_TEAL);
                        } else {
                            org_infected_by_fruit(org, o);
                            takenEnergyFruit = bio_between(o->energy - 0.0000002, 0, o->energy);
                            org->energy += takenEnergyFruit;
                            o->energy -= takenEnergyFruit;
                            org_set_color(org, CM_LIGHTBROWN);
                            org->time_to_reproduce = 0;
                            if ((!org->isaconsumer) && (!org->isafungus) && (org->sporeversion >= 3)) {
                                org->infected_reproduce_energy = -org->infected_reproduce_energy;
                            }
                        }
                    } else {
                        if (org->sporeversion != 1) {
                            org_set_color(org, CM_SPORE);
                        }
                    }
                }
            }
            break;
        case TY_INDIGO:
            if ((org->isaplant) && (o->sporeversion == 1)) {
                takenEnergyFruit = bio_between(o->energy - 0.0000002, 0, o->energy);
                org->energy += takenEnergyFruit;
                o->energy -= takenEnergyFruit;
                org_set_color(org, CM_INDIGO);
                org->time_to_reproduce = 0;
            }
            break;
        case TY_MAGENTA:
        case TY_ROSE:
            if ((org->infected_gc != o->gc) && (o->sporeversion == 1) && (!o->altruist)) {
                if (org->isaplant) {
                    if ((org->infected_gc != NULL) || (org->sporeversion != 1)) {
                        if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                            if ((!org->isonlyc4) || (org->frames_color > 8)) {
                                org->hasdodged = true;
                            }
                            org_set_color(org, CM_TEAL);
                        } else {
                            org_infected_by_fruit(org, o);
                            takenEnergyFruit = bio_between(o->energy - 0.0000002, 0, o->energy);
                            org->energy += takenEnergyFruit;
                            o->energy -= takenEnergyFruit;
                            org_set_color(org, CM_LIGHTBROWN);
                            org->time_to_reproduce = 0;
                        }
                    }
                }
            }
            break;
        case TY_SILVER:
            if (org->infected_gc != o->gc) {
                if (((org->total_kills > 0) && ((org->isaconsumer) || (org->isafungus))) || (org->isenhanced)) {
                    break;
                } else {
                    if (o->sporeversion == 1) {
                        if (org->isaplant) {
                            if ((org->infected_gc != NULL) || (org->sporeversion != 1)) {
                                org_infected_by_fruit(org, o);
                                takenEnergyFruit = bio_between(o->energy - 0.0000002, 0, o->energy);
                                org->energy += takenEnergyFruit;
                                o->energy -= takenEnergyFruit;
                                org_set_color(org, CM_LIGHTBROWN);
                                org->time_to_reproduce = 0;
                            }
                        }
                    }
                }
            }
            break;
        case TY_SKY:
            if ((org->skyversion == 2) && (org->infected_gc != o->gc) && (o->sporeversion == 1)) {
                if (org->isaplant) {
                    if ((org->infected_gc != NULL) || (org->sporeversion != 1)) {
                        if (org_use_energy(org, P.sky_energy_consumption)) {
                            org->seg_color[oseg] = CM_DEEPSKY;
                            org->useextraeffects = true;
                            org_set_color(org, CM_DEEPSKY);
                        } else {
                            // isenhanced is used for real viruses here
                            if (((o->isenhanced) && (!o->isblond)) || (org->isblond)) {
                                org_infected_by_fruit(org, o);
                            } else {
                                org_infected_by_fruit_weak(org, o);
                            }
                            takenEnergyFruit = bio_between(o->energy - 0.0000002, 0, o->energy);
                            org->energy += takenEnergyFruit;
                            o->energy -= takenEnergyFruit;
                            org_set_color(org, CM_LIGHTBROWN);
                            org->time_to_reproduce = 0;
                        }
                    }
                }
            }
            break;
        case TY_SPIKE:
        case TY_DEEPSKY:
        case TY_OLIVE:
        case TY_DARKOLIVE:
        case TY_BARK:
        case TY_OLDBARK:
        case TY_OCHRE:
            if ((org->infected_gc != o->gc) && (o->sporeversion == 1)) {
                if (org->isaplant) {
                    if ((org->infected_gc != NULL) || (org->sporeversion != 1)) {
                        // isenhanced is used for real viruses here
                        if (((o->isenhanced) && (!o->isblond)) || (org->isblond)) {
                            org_infected_by_fruit(org, o);
                        } else {
                            org_infected_by_fruit_weak(org, o);
                        }
                        takenEnergyFruit = bio_between(o->energy - 0.0000002, 0, o->energy);
                        org->energy += takenEnergyFruit;
                        o->energy -= takenEnergyFruit;
                        org_set_color(org, CM_LIGHTBROWN);
                        org->time_to_reproduce = 0;
                    }
                }
            }
            break;
        case TY_PLAGUE:
            if ((org->infected_gc != o->gc) && (o->sporeversion == 1)) {
                if (org->isaplant) {
                    if ((org->infected_gc != NULL) || (org->sporeversion != 1)) {
                        if ((!org->isinfectious) && (!org->isaconsumer) && (!org->isafungus)) {
                            org_infected_by_fruit_weak(org, o);
                            org->infected_reproduce_energy = 4;
                        } else {
                            org_infected_by_fruit(org, o);
                        }
                        takenEnergyFruit = bio_between(o->energy - 0.0000002, 0, o->energy);
                        org->energy += takenEnergyFruit;
                        o->energy -= takenEnergyFruit;
                        org_set_color(org, CM_LIGHTBROWN);
                        org->time_to_reproduce = 0;
                    }
                }
            }
            break;
        case TY_LAVENDER:
            if ((org->infected_gc != o->gc) && (o->sporeversion == 1)) {
                if (org->isaplant) {
                    if ((org->infected_gc != NULL) || (org->sporeversion != 1)) {
                        if (org->isenhanced) {
                            org_use_energy(o, bio_between((1 / P.lavender_energy_consumption), 0, o->energy));
                            if (org->lavender < P.lavender_shield) {
                                org->lavender += 1000;
                                if (org->lavender >= P.lavender_shield) {
                                    org->lavender = P.lavender_shield;
                                }
                            }
                        } else {
                            org_use_energy(o, bio_between((0.2 / P.lavender_energy_consumption), 0, o->energy));
                            if (org->lavender < P.lavender_shield) {
                                org->lavender += 200;
                                if (org->lavender >= P.lavender_shield) {
                                    org->lavender = P.lavender_shield;
                                }
                            }
                        }
                        // The other organism will be shown in lavender
                        org_set_color(org, CM_LAVENDER);
                        // This organism will be shown in deadbark
                        org_set_color(o, CM_DEADBARK);
                        org->hasdodged = true;
                        if (o->energy < BIO_TOL) {
                            org_die(o, org);
                        }
                    }
                }
            }
            break;
        case TY_RED:
        case TY_ORANGE:
        case TY_FIRE:
        case TY_MAROON:
        case TY_PINK:
        case TY_CREAM:
        case TY_SPIKEPOINT:
        case TY_BLUE:
        case TY_VIOLET:
        case TY_CORAL:
        case TY_MINT:
        case TY_BROWN:
            break;
        default:
            if ((org->infected_gc != o->gc) && (o->sporeversion == 1)) {
                if (org->isaplant) {
                    if ((org->infected_gc != NULL) || (org->sporeversion != 1)) {
                        org_infected_by_fruit(org, o);
                        takenEnergyFruit = bio_between(o->energy - 0.0000002, 0, o->energy);
                        org->energy += takenEnergyFruit;
                        o->energy -= takenEnergyFruit;
                        org_set_color(org, CM_LIGHTBROWN);
                        org->time_to_reproduce = 0;
                    }
                }
            }
        }
        break;
    case TY_PLAGUE:
        // Force reproduction of infected victims
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_WHITE:
        case TY_PLAGUE:
            if (((org->infected_gc == o->gc) || ((!o->isinfectious) && (org->infected_gc != NULL))) && (org->isinfectious)) {
                if ((((o->plagueversion == 2) || (!o->isinfectious)) && (org->isaconsumer)) || ((o->plagueversion == 1) && (org->isaplant))) {
                    if (org->lavender > 0) {
                        if ((o->isenhanced) && (!o->isinfectious) && (!o->isaconsumer) && (!o->isafungus)) {
                            org_weaklavendershield(org);
                        } else {
                            org_lavendershield(org);
                        }
                        org_set_color(o, CM_PLAGUE);
                    }
                    if (org->lavender <= 0) {
                        if (o->isinfectious) {
                            if (o->isaplant) {
                                if (o->energy > P.plague_energy_consumption) {
                                    org_reproduce_virus(org);
                                    org_set_color(o, CM_PLAGUE);
                                }
                            } else {
                                if (o->energy > P.scourge_energy_consumption) {
                                    org_reproduce_virus(org);
                                    org_set_color(o, CM_PLAGUE);
                                }
                            }
                        } else {
                            if (o->energy > P.scourge_energy_consumption) {
                                if ((!o->isaconsumer) && (!o->isafungus)) {
                                    org->n_virus_children = -1;
                                }
                                org_reproduceforeign_virus(org);
                                org_set_color(o, CM_PLAGUE);
                            }
                        }
                    }
                }
            }
            if ((org->indigo > 0) && (!org->isaplant) && (!org->isaconsumer) && (!org->isafungus)) {
                if ((o->isaplant) && (o->isinfectious)) {
                    if (org_use_energy(o, (P.plague_energy_consumption + 0.01) / P.indigo_energy_consumption)) {
                        org_set_color(org, CM_INDIGO);
                        org_set_color(o, CM_PLAGUE);
                    }
                } else {
                    if ((o->indigo == 0) || (o->isaplant) || (o->isaconsumer) || (o->isafungus)) {
                        if (org_use_energy(o, (P.scourge_energy_consumption + 0.1) / P.indigo_energy_consumption)) {
                            org_set_color(org, CM_INDIGO);
                            org_set_color(o, CM_PLAGUE);
                        }
                    }
                }
            }
            break;
        case TY_SILVER:
            if ((org->infected_gc == o->gc) || ((!o->isinfectious) && (org->infected_gc != NULL))) {
                if ((org->isenhanced) || (o->total_infected >= org->total_infected)) {
                    if ((((o->plagueversion == 2) || (!o->isinfectious)) && (org->isaconsumer)) || ((o->plagueversion == 1) && (org->isaplant))) {
                        if (org->lavender > 0) {
                            if ((o->isenhanced) && (!o->isinfectious) && (!o->isaconsumer) && (!o->isafungus)) {
                                org_weaklavendershield(org);
                            } else {
                                org_lavendershield(org);
                            }
                            org_set_color(o, CM_PLAGUE);
                        }
                        if (org->lavender <= 0) {
                            if (o->isinfectious) {
                                if (o->isaplant) {
                                    if (o->energy > P.plague_energy_consumption) {
                                        org_reproduce_virus(org);
                                        org_set_color(o, CM_PLAGUE);
                                    }
                                } else {
                                    if (o->energy > P.scourge_energy_consumption) {
                                        org_reproduce_virus(org);
                                        org_set_color(o, CM_PLAGUE);
                                    }
                                }
                            } else {
                                if (o->energy > P.scourge_energy_consumption) {
                                    if ((!o->isaconsumer) && (!o->isafungus)) {
                                        org->n_virus_children = -1;
                                    }
                                    org_reproduceforeign_virus(org);
                                    org_set_color(o, CM_PLAGUE);
                                }
                            }
                        }
                    }
                }
            }
            if ((org->indigo > 0) && (!org->isaplant) && (!org->isaconsumer) && (!org->isafungus)) {
                if ((o->isaplant) && (o->isinfectious)) {
                    if (org_use_energy(o, (P.plague_energy_consumption + 0.01) / P.indigo_energy_consumption)) {
                        org_set_color(org, CM_INDIGO);
                        org_set_color(o, CM_PLAGUE);
                    }
                } else {
                    if ((o->indigo == 0) || (o->isaplant) || (o->isaconsumer) || (o->isafungus)) {
                        if (org_use_energy(o, (P.scourge_energy_consumption + 0.1) / P.indigo_energy_consumption)) {
                            org_set_color(org, CM_INDIGO);
                            org_set_color(o, CM_PLAGUE);
                        }
                    }
                }
            }
            break;
        case TY_INDIGO:
            if ((((o->plagueversion == 2) || (!o->isinfectious)) && (org->isaconsumer)) || ((o->plagueversion == 1) && (org->isaplant)) || ((!org->isaplant) && (!org->isaconsumer) && (!org->isafungus))) {
                if ((o->isaplant) && (o->isinfectious)) {
                    if (org_use_energy(o, (P.plague_energy_consumption + 0.01) / P.indigo_energy_consumption)) {
                        org_set_color(org, CM_INDIGO);
                        org_set_color(o, CM_PLAGUE);
                    }
                } else {
                    if ((o->indigo == 0) || (o->isaplant) || (o->isaconsumer) || (o->isafungus)) {
                        if (org_use_energy(o, (P.scourge_energy_consumption + 0.1) / P.indigo_energy_consumption)) {
                            org_set_color(org, CM_INDIGO);
                            org_set_color(o, CM_PLAGUE);
                        }
                    }
                }
                if ((org->antiviral > 0) && (o->isinfectious)) {
                    org_virusneutralized(o);
                }
            }
            break;
        case TY_GREEN:
        case TY_GRASS:
        case TY_FOREST:
        case TY_SPRING:
        case TY_SUMMER:
        case TY_WINTER:
        case TY_LIME:
        case TY_LEAF:
        case TY_C4:
        case TY_JADE:
        case TY_DARKJADE:
            if ((org->infected_gc == o->gc) || ((!o->isinfectious) && (org->infected_gc != NULL))) {
                if (o->plagueversion == 1) {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, CM_PLAGUE);
                        org->hasdodged = true;
                    } else {
                        if (org->lavender > 0) {
                            if ((o->isenhanced) && (!o->isinfectious) && (!o->isaconsumer) && (!o->isafungus)) {
                                org_weaklavendershield(org);
                            } else {
                                org_lavendershield(org);
                            }
                            org_set_color(o, CM_PLAGUE);
                        }
                        if (org->lavender <= 0) {
                            if (org->jadefactor > 1) {
                                if (org->seg_color[oseg] == CM_JADE) {
                                    org->seg_color[oseg] = CM_DARKJADE;
                                } else {
                                    org->seg_color[oseg] = CM_DARKGREEN;
                                }
                                org_set_color(o, CM_PLAGUE);
                                org->hasdodged = true;
                                org->useextraeffects = true;
                                if ((org->antiviral > 0) && (o->isinfectious)) {
                                    org_virusneutralized(o);
                                }
                            } else {
                                if (o->isinfectious) {
                                    if (o->isaplant) {
                                        if (o->energy > P.plague_energy_consumption) {
                                            org_reproduce_virus(org);
                                            org_set_color(o, CM_PLAGUE);
                                        }
                                    } else {
                                        if (o->energy > P.scourge_energy_consumption) {
                                            org_reproduce_virus(org);
                                            org_set_color(o, CM_PLAGUE);
                                        }
                                    }
                                } else {
                                    if (o->energy > P.scourge_energy_consumption) {
                                        if ((!o->isaconsumer) && (!o->isafungus)) {
                                            org->n_virus_children = -1;
                                        }
                                        org_reproduceforeign_virus(org);
                                        org_set_color(o, CM_PLAGUE);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_DARKGREEN:
        case TY_PURPLE:
            if ((org->infected_gc == o->gc) || ((!o->isinfectious) && (org->infected_gc != NULL))) {
                if (o->plagueversion == 1) {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, CM_PLAGUE);
                        org->hasdodged = true;
                    } else {
                        if (org->lavender > 0) {
                            if ((o->isenhanced) && (!o->isinfectious) && (!o->isaconsumer) && (!o->isafungus)) {
                                org_weaklavendershield(org);
                            } else {
                                org_lavendershield(org);
                            }
                            org_set_color(o, CM_PLAGUE);
                        }
                        if (org->lavender <= 0) {
                            if (o->isinfectious) {
                                if (o->isaplant) {
                                    if (o->energy > P.plague_energy_consumption) {
                                        org_reproduce_virus(org);
                                        org_set_color(o, CM_PLAGUE);
                                    }
                                } else {
                                    if (o->energy > P.scourge_energy_consumption) {
                                        org_reproduce_virus(org);
                                        org_set_color(o, CM_PLAGUE);
                                    }
                                }
                            } else {
                                if (o->energy > P.scourge_energy_consumption) {
                                    if ((!o->isaconsumer) && (!o->isafungus)) {
                                        org->n_virus_children = -1;
                                    }
                                    org_reproduceforeign_virus(org);
                                    org_set_color(o, CM_PLAGUE);
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_BLUE:
            if ((org->infected_gc == o->gc) || ((!o->isinfectious) && (org->infected_gc != NULL))) {
                if ((o->plagueversion == 1) && (org->isaplant)) {
                    if (org_use_energy(org, P.blue_energy_consumption)) {
                        if (org->isenhanced) {
                            org_use_energy(o, bio_between((0.5 * sqrt(org->m[oseg])) * P.organic_obtained_energy, 0, o->energy));
                            org_set_color(o, CM_DARKLILAC);
                            if (o->energy < BIO_TOL) {
                                org_die(o, org);
                            }
                        } else {
                            org_set_color(o, CM_PLAGUE);
                        }
                        org_set_color(org, JC_BLUE);
                    } else {
                        if (org->lavender > 0) {
                            if ((o->isenhanced) && (!o->isinfectious) && (!o->isaconsumer) && (!o->isafungus)) {
                                org_weaklavendershield(org);
                            } else {
                                org_lavendershield(org);
                            }
                            org_set_color(o, CM_PLAGUE);
                        }
                        if (org->lavender <= 0) {
                            if (o->isinfectious) {
                                if (o->isaplant) {
                                    if (o->energy > P.plague_energy_consumption) {
                                        org_reproduce_virus(org);
                                        org_set_color(o, CM_PLAGUE);
                                    }
                                } else {
                                    if (o->energy > P.scourge_energy_consumption) {
                                        org_reproduce_virus(org);
                                        org_set_color(o, CM_PLAGUE);
                                    }
                                }
                            } else {
                                if (o->energy > P.scourge_energy_consumption) {
                                    if ((!o->isaconsumer) && (!o->isafungus)) {
                                        org->n_virus_children = -1;
                                    }
                                    org_reproduceforeign_virus(org);
                                    org_set_color(o, CM_PLAGUE);
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_OCHRE:
        case TY_BARK:
        case TY_OLDBARK:
            if ((org->infected_gc == o->gc) || ((!o->isinfectious) && (org->infected_gc != NULL))) {
                if ((o->plagueversion == 1) && (org->isaplant)) {
                    if (org->lavender > 0) {
                        if ((o->isenhanced) && (!o->isinfectious) && (!o->isaconsumer) && (!o->isafungus)) {
                            org_weaklavendershield(org);
                        } else {
                            org_lavendershield(org);
                        }
                        org_set_color(o, CM_PLAGUE);
                    }
                    if (org->lavender <= 0) {
                        if (o->isinfectious) {
                            if (o->isaplant) {
                                if (o->energy > P.plague_energy_consumption) {
                                    org_reproduce_virus(org);
                                    org_set_color(o, CM_PLAGUE);
                                }
                            } else {
                                if (o->energy > P.scourge_energy_consumption) {
                                    org_reproduce_virus(org);
                                    org_set_color(o, CM_PLAGUE);
                                }
                            }
                        } else {
                            if (o->energy > P.scourge_energy_consumption) {
                                if ((!o->isaconsumer) && (!o->isafungus)) {
                                    org->n_virus_children = -1;
                                }
                                org_reproduceforeign_virus(org);
                                org_set_color(o, CM_PLAGUE);
                            }
                        }
                    }
                }
            }
            break;
        case TY_SKY:
        case TY_DEEPSKY:
        case TY_FALLOW:
            if (((org->infected_gc == o->gc) && (o->isenhanced)) || ((!o->isinfectious) && (org->infected_gc != NULL))) {
                if ((o->plagueversion == 1) && (org->isaplant) && (org->createlavender == 0)) {
                    if (org->lavender > 0) {
                        if ((o->isenhanced) && (!o->isinfectious) && (!o->isaconsumer) && (!o->isafungus)) {
                            org_weaklavendershield(org);
                        } else {
                            org_lavendershield(org);
                        }
                        org_set_color(o, CM_PLAGUE);
                    }
                    if (org->lavender <= 0) {
                        if (o->isinfectious) {
                            if (o->isaplant) {
                                if (o->energy > P.plague_energy_consumption) {
                                    org_reproduce_virus(org);
                                    org_set_color(o, CM_PLAGUE);
                                }
                            } else {
                                if (o->energy > P.scourge_energy_consumption) {
                                    org_reproduce_virus(org);
                                    org_set_color(o, CM_PLAGUE);
                                }
                            }
                        } else {
                            if (o->energy > P.scourge_energy_consumption) {
                                if ((!o->isaconsumer) && (!o->isafungus)) {
                                    org->n_virus_children = -1;
                                }
                                org_reproduceforeign_virus(org);
                                org_set_color(o, CM_PLAGUE);
                            }
                        }
                    }
                }
            }
            break;
        case TY_MAROON:
        case TY_ORANGE:
        case TY_FIRE:
            if ((org->infected_gc == o->gc) || ((!o->isinfectious) && (org->infected_gc != NULL))) {
                if ((o->plagueversion == 2) || (!o->isinfectious)) {
                    if (org->lavender > 0) {
                        if ((o->isenhanced) && (!o->isinfectious) && (!o->isaconsumer) && (!o->isafungus)) {
                            org_weaklavendershield(org);
                        } else {
                            org_lavendershield(org);
                        }
                        org_set_color(o, CM_PLAGUE);
                    }
                    if (org->lavender <= 0) {
                        if (o->isinfectious) {
                            if (o->isaplant) {
                                if (o->energy > P.plague_energy_consumption) {
                                    org_reproduce_virus(org);
                                    org_set_color(o, CM_PLAGUE);
                                }
                            } else {
                                if (o->energy > P.scourge_energy_consumption) {
                                    org_reproduce_virus(org);
                                    org_set_color(o, CM_PLAGUE);
                                }
                            }
                        } else {
                            if (o->energy > P.scourge_energy_consumption) {
                                if ((!o->isaconsumer) && (!o->isafungus)) {
                                    org->n_virus_children = -1;
                                }
                                org_reproduceforeign_virus(org);
                                org_set_color(o, CM_PLAGUE);
                            }
                        }
                    }
                }
            }
            break;
        case TY_RED:
        case TY_CREAM:
            if (((org->infected_gc == o->gc) && (o->isenhanced)) || ((!o->isinfectious) && (org->infected_gc != NULL))) {
                if (((o->plagueversion == 2) || (!o->isinfectious)) && (org->createlavender == 0)) {
                    if (org->lavender > 0) {
                        if ((o->isenhanced) && (!o->isinfectious) && (!o->isaconsumer) && (!o->isafungus)) {
                            org_weaklavendershield(org);
                        } else {
                            org_lavendershield(org);
                        }
                        org_set_color(o, CM_PLAGUE);
                    }
                    if (org->lavender <= 0) {
                        if (o->isinfectious) {
                            if (o->isaplant) {
                                if (o->energy > P.plague_energy_consumption) {
                                    org_reproduce_virus(org);
                                    org_set_color(o, CM_PLAGUE);
                                }
                            } else {
                                if (o->energy > P.scourge_energy_consumption) {
                                    org_reproduce_virus(org);
                                    org_set_color(o, CM_PLAGUE);
                                }
                            }
                        } else {
                            if (o->energy > P.scourge_energy_consumption) {
                                if ((!o->isaconsumer) && (!o->isafungus)) {
                                    org->n_virus_children = -1;
                                }
                                org_reproduceforeign_virus(org);
                                org_set_color(o, CM_PLAGUE);
                            }
                        }
                    }
                }
            }
            break;
        case TY_PINK:
            if (((org->infected_gc == o->gc) && ((gc_get_modifiespink(org->gc)) || ((o->isauburn) && (!o->isaplant) && (!o->isaconsumer) && (!o->isafungus)))) || ((!o->isinfectious) && (org->infected_gc != NULL))) {
                if ((o->plagueversion == 2) || (!o->isinfectious)) {
                    if (org->lavender > 0) {
                        if ((o->isenhanced) && (!o->isinfectious) && (!o->isaconsumer) && (!o->isafungus)) {
                            org_weaklavendershield(org);
                        } else {
                            org_lavendershield(org);
                        }
                        org_set_color(o, CM_PLAGUE);
                    }
                    if (org->lavender <= 0) {
                        if (o->isinfectious) {
                            if (o->isaplant) {
                                if (o->energy > P.plague_energy_consumption) {
                                    org_reproduce_virus(org);
                                    org_set_color(o, CM_PLAGUE);
                                }
                            } else {
                                if (o->energy > P.scourge_energy_consumption) {
                                    org_reproduce_virus(org);
                                    org_set_color(o, CM_PLAGUE);
                                }
                            }
                        } else {
                            if (o->energy > P.scourge_energy_consumption) {
                                if ((!o->isaconsumer) && (!o->isafungus)) {
                                    org->n_virus_children = -1;
                                }
                                org_reproduceforeign_virus(org);
                                org_set_color(o, CM_PLAGUE);
                            }
                        }
                    }
                }
            }
            break;
        case TY_SPIKE:
            if ((org->isaplant) || (org->isenhanced)) {
                if ((org->infected_gc == o->gc) || ((!o->isinfectious) && (org->infected_gc != NULL))) {
                    if ((((o->plagueversion == 2) || (!o->isinfectious)) && (org->isaconsumer)) || ((o->plagueversion == 1) && (org->isaplant))) {
                        if (org->lavender > 0) {
                            if ((o->isenhanced) && (!o->isinfectious) && (!o->isaconsumer) && (!o->isafungus)) {
                                org_weaklavendershield(org);
                            } else {
                                org_lavendershield(org);
                            }
                            org_set_color(o, CM_PLAGUE);
                        }
                        if (org->lavender <= 0) {
                            if (o->isinfectious) {
                                if (o->isaplant) {
                                    if (o->energy > P.plague_energy_consumption) {
                                        org_reproduce_virus(org);
                                        org_set_color(o, CM_PLAGUE);
                                    }
                                } else {
                                    if (o->energy > P.scourge_energy_consumption) {
                                        org_reproduce_virus(org);
                                        org_set_color(o, CM_PLAGUE);
                                    }
                                }
                            } else {
                                if (o->energy > P.scourge_energy_consumption) {
                                    if ((!o->isaconsumer) && (!o->isafungus)) {
                                        org->n_virus_children = -1;
                                    }
                                    org_reproduceforeign_virus(org);
                                    org_set_color(o, CM_PLAGUE);
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_LAVENDER:
            if ((org->isaconsumer) || (org->isaplant)) {
                if (org->isenhanced) {
                    org_use_energy(o, bio_between((1 / P.lavender_energy_consumption), 0, o->energy));
                    if (org->lavender < P.lavender_shield) {
                        org->lavender += 1000;
                        if (org->lavender >= P.lavender_shield) {
                            org->lavender = P.lavender_shield;
                        }
                    }
                } else {
                    org_use_energy(o, bio_between((0.2 / P.lavender_energy_consumption), 0, o->energy));
                    if (org->lavender < P.lavender_shield) {
                        org->lavender += 200;
                        if (org->lavender >= P.lavender_shield) {
                            org->lavender = P.lavender_shield;
                        }
                    }
                }
                if (org->infected_gc == o->gc) {
                    org->infected_gc = NULL;
                    org->saved_gc = NULL;
                }
                // The other organism will be shown in lavender
                org_set_color(org, CM_LAVENDER);
                // This organism will be shown in deadbark
                org_set_color(o, CM_DEADBARK);
                org->hasdodged = true;
                if (o->energy < BIO_TOL) {
                    org_die(o, org);
                }
            }
            break;
        case TY_SPORE:
            if ((org->infected_gc == o->gc) || ((!o->isinfectious) && (org->infected_gc != NULL))) {
                if ((((o->plagueversion == 2) || (!o->isinfectious)) && (org->isaconsumer)) || ((o->plagueversion == 1) && (org->isaplant))) {
                    if ((org->sporeversion >= 1) && (org->isaconsumer)) {
                        if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                            org_set_color(org, CM_TEAL);
                            org_set_color(o, CM_PLAGUE);
                            org->hasdodged = true;
                        } else {
                            if (org->lavender > 0) {
                                if ((o->isenhanced) && (!o->isinfectious) && (!o->isaconsumer) && (!o->isafungus)) {
                                    org_weaklavendershield(org);
                                } else {
                                    org_lavendershield(org);
                                }
                                org_set_color(o, CM_PLAGUE);
                            }
                            if (org->lavender <= 0) {
                                if (o->isinfectious) {
                                    if (o->isaplant) {
                                        if (o->energy > P.plague_energy_consumption) {
                                            org_reproduce_virus(org);
                                            org_set_color(o, CM_PLAGUE);
                                        }
                                    } else {
                                        if (o->energy > P.scourge_energy_consumption) {
                                            org_reproduce_virus(org);
                                            org_set_color(o, CM_PLAGUE);
                                        }
                                    }
                                } else {
                                    if (o->energy > P.scourge_energy_consumption) {
                                        if ((!o->isaconsumer) && (!o->isafungus)) {
                                            org->n_virus_children = -1;
                                        }
                                        org_reproduceforeign_virus(org);
                                        org_set_color(o, CM_PLAGUE);
                                    }
                                }
                            }
                        }
                    } else {
                        org_set_color(o, CM_PLAGUE);
                        org_set_color(org, CM_SPORE);
                    }
                }
            }
            break;
        case TY_CYAN:
        case TY_TEAL:
        case TY_YELLOW:
        case TY_AUBURN:
        case TY_BLOND:
        case TY_FLOWER:
        case TY_DARKGRAY:
        case TY_GOLD:
            if ((org->infected_gc == o->gc) || ((!o->isinfectious) && (org->infected_gc != NULL))) {
                if ((((o->plagueversion == 2) || (!o->isinfectious)) && (org->isaconsumer)) || ((o->plagueversion == 1) && (org->isaplant))) {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, CM_PLAGUE);
                        org->hasdodged = true;
                    } else {
                        if (org->lavender > 0) {
                            if ((o->isenhanced) && (!o->isinfectious) && (!o->isaconsumer) && (!o->isafungus)) {
                                org_weaklavendershield(org);
                            } else {
                                org_lavendershield(org);
                            }
                            org_set_color(o, CM_PLAGUE);
                        }
                        if (org->lavender <= 0) {
                            if (o->isinfectious) {
                                if (o->isaplant) {
                                    if (o->energy > P.plague_energy_consumption) {
                                        org_reproduce_virus(org);
                                        org_set_color(o, CM_PLAGUE);
                                    }
                                } else {
                                    if (o->energy > P.scourge_energy_consumption) {
                                        org_reproduce_virus(org);
                                        org_set_color(o, CM_PLAGUE);
                                    }
                                }
                            } else {
                                if (o->energy > P.scourge_energy_consumption) {
                                    if ((!o->isaconsumer) && (!o->isafungus)) {
                                        org->n_virus_children = -1;
                                    }
                                    org_reproduceforeign_virus(org);
                                    org_set_color(o, CM_PLAGUE);
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_MAGENTA:
        case TY_ROSE:
            if (((org->infected_gc == o->gc) || ((!o->isinfectious) && (org->infected_gc != NULL))) && (!o->altruist)) {
                if ((((o->plagueversion == 2) || (!o->isinfectious)) && (org->isaconsumer)) || ((o->plagueversion == 1) && (org->isaplant))) {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, CM_PLAGUE);
                        org->hasdodged = true;
                    } else {
                        if (org->lavender > 0) {
                            if ((o->isenhanced) && (!o->isinfectious) && (!o->isaconsumer) && (!o->isafungus)) {
                                org_weaklavendershield(org);
                            } else {
                                org_lavendershield(org);
                            }
                            org_set_color(o, CM_PLAGUE);
                        }
                        if (org->lavender <= 0) {
                            if (o->isinfectious) {
                                if (o->isaplant) {
                                    if (o->energy > P.plague_energy_consumption) {
                                        org_reproduce_virus(org);
                                        org_set_color(o, CM_PLAGUE);
                                    }
                                } else {
                                    if (o->energy > P.scourge_energy_consumption) {
                                        org_reproduce_virus(org);
                                        org_set_color(o, CM_PLAGUE);
                                    }
                                }
                            } else {
                                if (o->energy > P.scourge_energy_consumption) {
                                    if ((!o->isaconsumer) && (!o->isafungus)) {
                                        org->n_virus_children = -1;
                                    }
                                    org_reproduceforeign_virus(org);
                                    org_set_color(o, CM_PLAGUE);
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_SPIKEPOINT:
            if (org->isenhanced) {
                if ((org->infected_gc == o->gc) || ((!o->isinfectious) && (org->infected_gc != NULL))) {
                    if ((((o->plagueversion == 2) || (!o->isinfectious)) && (org->isaconsumer)) || ((o->plagueversion == 1) && (org->isaplant))) {
                        if (org->lavender > 0) {
                            if ((o->isenhanced) && (!o->isinfectious) && (!o->isaconsumer) && (!o->isafungus)) {
                                org_weaklavendershield(org);
                            } else {
                                org_lavendershield(org);
                            }
                            org_set_color(o, CM_PLAGUE);
                        }
                        if (org->lavender <= 0) {
                            if (o->isinfectious) {
                                if (o->isaplant) {
                                    if (o->energy > P.plague_energy_consumption) {
                                        org_reproduce_virus(org);
                                        org_set_color(o, CM_PLAGUE);
                                    }
                                } else {
                                    if (o->energy > P.scourge_energy_consumption) {
                                        org_reproduce_virus(org);
                                        org_set_color(o, CM_PLAGUE);
                                    }
                                }
                            } else {
                                if (o->energy > P.scourge_energy_consumption) {
                                    if ((!o->isaconsumer) && (!o->isafungus)) {
                                        org->n_virus_children = -1;
                                    }
                                    org_reproduceforeign_virus(org);
                                    org_set_color(o, CM_PLAGUE);
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_MINT:
        case TY_CORAL:
        case TY_OLIVE:
        case TY_BROWN:
            break;
        default:
            if ((org->infected_gc == o->gc) || ((!o->isinfectious) && (org->infected_gc != NULL))) {
                if ((((o->plagueversion == 2) || (!o->isinfectious)) && (org->isaconsumer)) || ((o->plagueversion == 1) && (org->isaplant))) {
                    if (org->lavender > 0) {
                        if ((o->isenhanced) && (!o->isinfectious) && (!o->isaconsumer) && (!o->isafungus)) {
                            org_weaklavendershield(org);
                        } else {
                            org_lavendershield(org);
                        }
                        org_set_color(o, CM_PLAGUE);
                    }
                    if (org->lavender <= 0) {
                        if (o->isinfectious) {
                            if (o->isaplant) {
                                if (o->energy > P.plague_energy_consumption) {
                                    org_reproduce_virus(org);
                                    org_set_color(o, CM_PLAGUE);
                                }
                            } else {
                                if (o->energy > P.scourge_energy_consumption) {
                                    org_reproduce_virus(org);
                                    org_set_color(o, CM_PLAGUE);
                                }
                            }
                        } else {
                            if (o->energy > P.scourge_energy_consumption) {
                                if ((!o->isaconsumer) && (!o->isafungus)) {
                                    org->n_virus_children = -1;
                                }
                                org_reproduceforeign_virus(org);
                                org_set_color(o, CM_PLAGUE);
                            }
                        }
                    }
                }
            }
        }
        if (org->n_virus_children != 0) {
            if (o->isinfectious) {
                if (o->isaplant) {
                    org_use_energy(o, P.plague_energy_consumption);
                    org_set_color(org, CM_DARKLILAC);
                } else {
                    org_use_energy(o, P.scourge_energy_consumption);
                    org_set_color(org, CM_DARKLILAC);
                }
                if (org->antiviral > 0) {
                    org_virusneutralized(o);
                }
            } else {
                org_use_energy(o, P.scourge_energy_consumption);
                org_set_color(org, CM_DARKLILAC);
            }
            org->n_virus_children = 0;
        }
        break;
    case TY_FALLOW:
        // Fallow segment: Inhibit and destroy other organisms reproduction system but let them reproduce if they are infected by this organism.
        if (org->hasdodged == false) {
            if ((!org->isonlyc4) || (org->reproducelate <= 0)) {
                org->hasdodged = true;
            }
        }
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_FALLOW:
            if ((org->isakiller) || (org->isblond)) {
                if ((org->fallowversion != o->fallowversion) || (org->isinfectious) || ((!o->isaconsumer) && (!o->isafungus)) || (o->isenhanced)) {
                    if ((org->isaplant) || (org->isaconsumer) || ((org->isafungus) && (((o->fallowversion == 2) && (org->antiviral == 0)) || (!o->isaplant)))) {
                        if (org->lavender > 0) {
                            if ((o->isenhanced) && (!o->isinfectious)) {
                                org_weaklavendershield(org);
                            } else {
                                org_lavendershield(org);
                            }
                            org_set_color(o, CM_FALLOW);
                        }
                        if (org->lavender <= 0) {
                            if (org->infected_gc != o->gc) {
                                if (o->fallowversion <= 2) {
                                    if ((o->fallowversion == 2) || (org->sporeversion > 0)) {
                                        if (org->time_to_reproduce < org->time_to_reproduce_max + j_d2i((10 * o->m[seg]))) {
                                            if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                org_set_color(o, CM_FALLOW);
                                                org_set_color(org, CM_FLOWER);
                                                org->time_to_reproduce = org->time_to_reproduce_max + j_d2i((10 * o->m[seg]));
                                            }
                                        }
                                    }
                                } else {
                                    if (o->fallowversion == 4) {
                                        if (org->time_to_reproduce < org->time_to_reproduce_max + j_d2i((5 * o->m[seg]))) {
                                            if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                org_set_color(o, CM_FALLOW);
                                                org_set_color(org, CM_FLOWER);
                                                org->time_to_reproduce = org->time_to_reproduce_max + j_d2i((5 * o->m[seg]));
                                            }
                                        }
                                    } else {
                                        if (o->isinfectious) {
                                            if (org->time_to_reproduce < org->time_to_reproduce_max) {
                                                if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                    org_set_color(o, CM_FALLOW);
                                                    org_set_color(org, CM_FLOWER);
                                                    org->time_to_reproduce = org->time_to_reproduce_max;
                                                }
                                            }
                                        } else {
                                            if (org->time_to_reproduce < org->time_to_reproduce_max + j_d2i((o->m[seg]))) {
                                                if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                    org_set_color(o, CM_FALLOW);
                                                    org_set_color(org, CM_FLOWER);
                                                    org->time_to_reproduce = org->time_to_reproduce_max + j_d2i((o->m[seg]));
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ((org->time_to_reproduce > 4) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                    org_set_color(o, CM_FALLOW);
                                    org_set_color(org, CM_BLOND);
                                    org->time_to_reproduce = 4;
                                }
                            }
                            if (o->fallowversion == 3) {
                                if (org->reproducelate <= 0) {
                                    if (o->isinfectious) {
                                        if ((org->reproducelate > -2) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                            org->reproducelate = -2;
                                            org_set_color(o, CM_FALLOW);
                                            org_set_color(org, CM_FLOWER);
                                        }
                                    } else {
                                        if ((org->reproducelate > -j_d2i(((o->m[seg] / 2) + 1))) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                            org->reproducelate = -j_d2i(((o->m[seg] / 2) + 1));
                                            org_set_color(o, CM_FALLOW);
                                            org_set_color(org, CM_FLOWER);
                                        }
                                    }
                                }
                            } else {
                                if (o->fallowversion == 4) {
                                    if ((org->isreproductive) && ((org->isaconsumer) || (org->isaplant)) && (org->antiviral == 0)) {
                                        org_inhibited(org, o);
                                    }
                                } else {
                                    if (o->fallowversion == 1) {
                                        if ((org->sporetime > 0) && (org->sporeversion > 0) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                            org->sporetime = -org->sporetime;
                                            org_set_color(o, CM_FALLOW);
                                            org_set_color(org, CM_FLOWER);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_WHITE:
        case TY_PURPLE:
        case TY_TEAL:
        case TY_CYAN:
        case TY_YELLOW:
        case TY_AUBURN:
        case TY_INDIGO:
        case TY_BLOND:
        case TY_SPORE:
        case TY_GOLD:
            if ((org->fallowversion != o->fallowversion) || (org->isinfectious) || ((!o->isaconsumer) && (!o->isafungus)) || (o->isenhanced)) {
                if ((org->isaplant) || (org->isaconsumer) || ((org->isafungus) && (((o->fallowversion == 2) && (org->antiviral == 0)) || (!o->isaplant)))) {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, CM_FALLOW);
                    } else {
                        if (org->lavender > 0) {
                            if ((o->isenhanced) && (!o->isinfectious)) {
                                org_weaklavendershield(org);
                            } else {
                                org_lavendershield(org);
                            }
                            org_set_color(o, CM_FALLOW);
                        }
                        if (org->lavender <= 0) {
                            if (org->infected_gc != o->gc) {
                                if (o->fallowversion <= 2) {
                                    if ((o->fallowversion == 2) || (org->sporeversion > 0)) {
                                        if (org->time_to_reproduce < org->time_to_reproduce_max + j_d2i((10 * o->m[seg]))) {
                                            if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                org_set_color(o, CM_FALLOW);
                                                org_set_color(org, CM_FLOWER);
                                                org->time_to_reproduce = org->time_to_reproduce_max + j_d2i((10 * o->m[seg]));
                                            }
                                        }
                                    }
                                } else {
                                    if (o->fallowversion == 4) {
                                        if (org->time_to_reproduce < org->time_to_reproduce_max + j_d2i((5 * o->m[seg]))) {
                                            if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                org_set_color(o, CM_FALLOW);
                                                org_set_color(org, CM_FLOWER);
                                                org->time_to_reproduce = org->time_to_reproduce_max + j_d2i((5 * o->m[seg]));
                                            }
                                        }
                                    } else {
                                        if (o->isinfectious) {
                                            if (org->time_to_reproduce < org->time_to_reproduce_max) {
                                                if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                    org_set_color(o, CM_FALLOW);
                                                    org_set_color(org, CM_FLOWER);
                                                    org->time_to_reproduce = org->time_to_reproduce_max;
                                                }
                                            }
                                        } else {
                                            if (org->time_to_reproduce < org->time_to_reproduce_max + j_d2i((o->m[seg]))) {
                                                if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                    org_set_color(o, CM_FALLOW);
                                                    org_set_color(org, CM_FLOWER);
                                                    org->time_to_reproduce = org->time_to_reproduce_max + j_d2i((o->m[seg]));
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ((org->time_to_reproduce > 4) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                    org_set_color(o, CM_FALLOW);
                                    org_set_color(org, CM_BLOND);
                                    org->time_to_reproduce = 4;
                                }
                            }
                            if (o->fallowversion == 3) {
                                if (org->reproducelate <= 0) {
                                    if (o->isinfectious) {
                                        if ((org->reproducelate > -2) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                            org->reproducelate = -2;
                                            org_set_color(o, CM_FALLOW);
                                            org_set_color(org, CM_FLOWER);
                                        }
                                    } else {
                                        if ((org->reproducelate > -j_d2i(((o->m[seg] / 2) + 1))) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                            org->reproducelate = -j_d2i(((o->m[seg] / 2) + 1));
                                            org_set_color(o, CM_FALLOW);
                                            org_set_color(org, CM_FLOWER);
                                        }
                                    }
                                }
                            } else {
                                if (o->fallowversion == 4) {
                                    if ((org->isreproductive) && ((org->isaconsumer) || (org->isaplant))) {
                                        if (org->antiviral == 0) {
                                            org_inhibited(org, o);
                                        } else {
                                            switch (org_get_type_color(org->seg_color[oseg])) {
                                            case TY_SPORE:
                                                if ((org->sporeversion > 0) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                                    if (org->isaplant) {
                                                        org->seg_color[oseg] = CM_BROKEN;
                                                    } else {
                                                        org->seg_color[oseg] = CM_LIGHTBROWN;
                                                    }
                                                    org_set_color(o, CM_FALLOW);
                                                    org_segments_restore_effects(org);
                                                    org->update_effects = 2;
                                                }
                                                break;
                                            case TY_AUBURN:
                                            case TY_INDIGO:
                                                if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                    if (org->isaplant) {
                                                        org->seg_color[oseg] = CM_BROKEN;
                                                    } else {
                                                        org->seg_color[oseg] = CM_LIGHTBROWN;
                                                    }
                                                    org_set_color(o, CM_FALLOW);
                                                    org_segments_restore_effects(org);
                                                    org->update_effects = 2;
                                                }
                                                break;
                                            case TY_YELLOW:
                                            case TY_BLOND:
                                                if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                    if (org->isaplant) {
                                                        org->seg_color[oseg] = CM_BROKEN;
                                                    } else {
                                                        org->seg_color[oseg] = CM_LIGHTBROWN;
                                                    }
                                                    org_set_color(o, CM_FALLOW);
                                                    org_segments_restore_effects(org);
                                                }
                                                break;
                                            }
                                        }
                                    }
                                } else {
                                    if (o->fallowversion == 1) {
                                        if ((org->sporetime > 0) && (org->sporeversion > 0) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                            org->sporetime = -org->sporetime;
                                            org_set_color(o, CM_FALLOW);
                                            org_set_color(org, CM_FLOWER);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_JADE:
        case TY_GREEN:
        case TY_GRASS:
        case TY_FOREST:
        case TY_SPRING:
        case TY_SUMMER:
        case TY_WINTER:
        case TY_LIME:
        case TY_LEAF:
        case TY_C4:
        case TY_DARKJADE:
        case TY_DARKGREEN:
            if ((org->jadefactor <= 1) || ((o->fallowversion == 1) && (org->sporeversion > 0)) || (!org->active)) {
                if ((org->fallowversion != o->fallowversion) || (org->isinfectious) || ((!o->isaconsumer) && (!o->isafungus)) || (o->isenhanced)) {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, CM_FALLOW);
                    } else {
                        if (org->lavender > 0) {
                            if ((o->isenhanced) && (!o->isinfectious)) {
                                org_weaklavendershield(org);
                            } else {
                                org_lavendershield(org);
                            }
                            org_set_color(o, CM_FALLOW);
                        }
                        if (org->lavender <= 0) {
                            if (org->infected_gc != o->gc) {
                                if (o->fallowversion <= 2) {
                                    if ((o->fallowversion == 2) || (org->sporeversion > 0)) {
                                        if (org->time_to_reproduce < org->time_to_reproduce_max + j_d2i((10 * o->m[seg]))) {
                                            if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                org_set_color(o, CM_FALLOW);
                                                org_set_color(org, CM_FLOWER);
                                                org->time_to_reproduce = org->time_to_reproduce_max + j_d2i((10 * o->m[seg]));
                                            }
                                        }
                                    }
                                } else {
                                    if (o->fallowversion == 4) {
                                        if (org->time_to_reproduce < org->time_to_reproduce_max + j_d2i((5 * o->m[seg]))) {
                                            if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                org_set_color(o, CM_FALLOW);
                                                org_set_color(org, CM_FLOWER);
                                                org->time_to_reproduce = org->time_to_reproduce_max + j_d2i((5 * o->m[seg]));
                                            }
                                        }
                                    } else {
                                        if (o->isinfectious) {
                                            if (org->time_to_reproduce < org->time_to_reproduce_max) {
                                                if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                    org_set_color(o, CM_FALLOW);
                                                    org_set_color(org, CM_FLOWER);
                                                    org->time_to_reproduce = org->time_to_reproduce_max;
                                                }
                                            }
                                        } else {
                                            if (org->time_to_reproduce < org->time_to_reproduce_max + j_d2i((o->m[seg]))) {
                                                if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                    org_set_color(o, CM_FALLOW);
                                                    org_set_color(org, CM_FLOWER);
                                                    org->time_to_reproduce = org->time_to_reproduce_max + j_d2i((o->m[seg]));
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ((org->time_to_reproduce > 4) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                    org_set_color(o, CM_FALLOW);
                                    org_set_color(org, CM_BLOND);
                                    org->time_to_reproduce = 4;
                                }
                            }
                            if (o->fallowversion == 3) {
                                if (org->reproducelate <= 0) {
                                    if (o->isinfectious) {
                                        if ((org->reproducelate > -2) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                            org->reproducelate = -2;
                                            org_set_color(o, CM_FALLOW);
                                            org_set_color(org, CM_FLOWER);
                                        }
                                    } else {
                                        if ((org->reproducelate > -j_d2i(((o->m[seg] / 2) + 1))) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                            org->reproducelate = -j_d2i(((o->m[seg] / 2) + 1));
                                            org_set_color(o, CM_FALLOW);
                                            org_set_color(org, CM_FLOWER);
                                        }
                                    }
                                }
                            } else {
                                if (o->fallowversion == 4) {
                                    if ((org->isreproductive) && (org->antiviral == 0)) {
                                        org_inhibited(org, o);
                                    }
                                } else {
                                    if (o->fallowversion == 1) {
                                        if ((org->sporetime > 0) && (org->sporeversion > 0) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                            org->sporetime = -org->sporetime;
                                            org_set_color(o, CM_FALLOW);
                                            org_set_color(org, CM_FLOWER);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_ROSE:
        case TY_MAGENTA:
            if (o->altruist) {
                break;
            } else {
                if ((org->fallowversion != o->fallowversion) || (org->isinfectious) || ((!o->isaconsumer) && (!o->isafungus)) || (o->isenhanced)) {
                    if ((org->isaplant) || (org->isaconsumer) || ((org->isafungus) && (((o->fallowversion == 2) && (org->antiviral == 0)) || (!o->isaplant)))) {
                        if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                            org_set_color(org, CM_TEAL);
                            org_set_color(o, CM_FALLOW);
                        } else {
                            if (org->lavender > 0) {
                                if ((o->isenhanced) && (!o->isinfectious)) {
                                    org_weaklavendershield(org);
                                } else {
                                    org_lavendershield(org);
                                }
                                org_set_color(o, CM_FALLOW);
                            }
                            if (org->lavender <= 0) {
                                if ((org->seg_color[oseg] == JC_MAGENTA) && (org_use_energy(org, P.magenta_energy_consumption))) {
                                    org_set_color(org, JC_MAGENTA);
                                    org_set_color(o, CM_FALLOW);
                                } else {
                                    if (org->infected_gc != o->gc) {
                                        if (o->fallowversion <= 2) {
                                            if ((o->fallowversion == 2) || (org->sporeversion > 0)) {
                                                if (org->time_to_reproduce < org->time_to_reproduce_max + j_d2i((10 * o->m[seg]))) {
                                                    if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                        org_set_color(o, CM_FALLOW);
                                                        org_set_color(org, CM_FLOWER);
                                                        org->time_to_reproduce = org->time_to_reproduce_max + j_d2i((10 * o->m[seg]));
                                                    }
                                                }
                                            }
                                        } else {
                                            if (o->fallowversion == 4) {
                                                if (org->time_to_reproduce < org->time_to_reproduce_max + j_d2i((5 * o->m[seg]))) {
                                                    if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                        org_set_color(o, CM_FALLOW);
                                                        org_set_color(org, CM_FLOWER);
                                                        org->time_to_reproduce = org->time_to_reproduce_max + j_d2i((5 * o->m[seg]));
                                                    }
                                                }
                                            } else {
                                                if (o->isinfectious) {
                                                    if (org->time_to_reproduce < org->time_to_reproduce_max) {
                                                        if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                            org_set_color(o, CM_FALLOW);
                                                            org_set_color(org, CM_FLOWER);
                                                            org->time_to_reproduce = org->time_to_reproduce_max;
                                                        }
                                                    }
                                                } else {
                                                    if (org->time_to_reproduce < org->time_to_reproduce_max + j_d2i((o->m[seg]))) {
                                                        if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                            org_set_color(o, CM_FALLOW);
                                                            org_set_color(org, CM_FLOWER);
                                                            org->time_to_reproduce = org->time_to_reproduce_max + j_d2i((o->m[seg]));
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        if ((org->time_to_reproduce > 4) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                            org_set_color(o, CM_FALLOW);
                                            org_set_color(org, CM_BLOND);
                                            org->time_to_reproduce = 4;
                                        }
                                    }
                                    if (o->fallowversion == 3) {
                                        if (org->reproducelate <= 0) {
                                            if (o->isinfectious) {
                                                if ((org->reproducelate > -2) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                                    org->reproducelate = -2;
                                                    org_set_color(o, CM_FALLOW);
                                                    org_set_color(org, CM_FLOWER);
                                                }
                                            } else {
                                                if ((org->reproducelate > -j_d2i(((o->m[seg] / 2) + 1))) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                                    org->reproducelate = -j_d2i(((o->m[seg] / 2) + 1));
                                                    org_set_color(o, CM_FALLOW);
                                                    org_set_color(org, CM_FLOWER);
                                                }
                                            }
                                        }
                                    } else {
                                        if (o->fallowversion == 4) {
                                            if ((org->isreproductive) && ((org->isaconsumer) || (org->isaplant)) && (org->antiviral == 0)) {
                                                org_inhibited(org, o);
                                            }
                                        } else {
                                            if (o->fallowversion == 1) {
                                                if ((org->sporetime > 0) && (org->sporeversion > 0) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                                    org->sporetime = -org->sporetime;
                                                    org_set_color(o, CM_FALLOW);
                                                    org_set_color(org, CM_FLOWER);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_BLUE:
            if ((org->fallowversion != o->fallowversion) || (org->isinfectious) || ((!o->isaconsumer) && (!o->isafungus)) || (o->isenhanced)) {
                if ((org->isaplant) || (org->isaconsumer) || ((org->isafungus) && (((o->fallowversion == 2) && (org->antiviral == 0)) || (!o->isaplant)))) {
                    if (org_use_energy(org, P.blue_energy_consumption)) {
                        org_set_color(o, CM_FALLOW);
                        org_set_color(org, JC_BLUE);
                    } else {
                        if (org->lavender > 0) {
                            if ((o->isenhanced) && (!o->isinfectious)) {
                                org_weaklavendershield(org);
                            } else {
                                org_lavendershield(org);
                            }
                            org_set_color(o, CM_FALLOW);
                        }
                        if (org->lavender <= 0) {
                            if (org->infected_gc != o->gc) {
                                if (o->fallowversion <= 2) {
                                    if ((o->fallowversion == 2) || (org->sporeversion > 0)) {
                                        if (org->time_to_reproduce < org->time_to_reproduce_max + j_d2i((10 * o->m[seg]))) {
                                            if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                org_set_color(o, CM_FALLOW);
                                                org_set_color(org, CM_FLOWER);
                                                org->time_to_reproduce = org->time_to_reproduce_max + j_d2i((10 * o->m[seg]));
                                            }
                                        }
                                    }
                                } else {
                                    if (o->fallowversion == 4) {
                                        if (org->time_to_reproduce < org->time_to_reproduce_max + j_d2i((5 * o->m[seg]))) {
                                            if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                org_set_color(o, CM_FALLOW);
                                                org_set_color(org, CM_FLOWER);
                                                org->time_to_reproduce = org->time_to_reproduce_max + j_d2i((5 * o->m[seg]));
                                            }
                                        }
                                    } else {
                                        if (o->isinfectious) {
                                            if (org->time_to_reproduce < org->time_to_reproduce_max) {
                                                if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                    org_set_color(o, CM_FALLOW);
                                                    org_set_color(org, CM_FLOWER);
                                                    org->time_to_reproduce = org->time_to_reproduce_max;
                                                }
                                            }
                                        } else {
                                            if (org->time_to_reproduce < org->time_to_reproduce_max + j_d2i((o->m[seg]))) {
                                                if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                    org_set_color(o, CM_FALLOW);
                                                    org_set_color(org, CM_FLOWER);
                                                    org->time_to_reproduce = org->time_to_reproduce_max + j_d2i((o->m[seg]));
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ((org->time_to_reproduce > 4) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                    org_set_color(o, CM_FALLOW);
                                    org_set_color(org, CM_BLOND);
                                    org->time_to_reproduce = 4;
                                }
                            }
                            if (o->fallowversion == 3) {
                                if (org->reproducelate <= 0) {
                                    if (o->isinfectious) {
                                        if ((org->reproducelate > -2) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                            org->reproducelate = -2;
                                            org_set_color(o, CM_FALLOW);
                                            org_set_color(org, CM_FLOWER);
                                        }
                                    } else {
                                        if ((org->reproducelate > -j_d2i(((o->m[seg] / 2) + 1))) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                            org->reproducelate = -j_d2i(((o->m[seg] / 2) + 1));
                                            org_set_color(o, CM_FALLOW);
                                            org_set_color(org, CM_FLOWER);
                                        }
                                    }
                                }
                            } else {
                                if (o->fallowversion == 4) {
                                    if ((org->isreproductive) && ((org->isaconsumer) || (org->isaplant)) && (org->antiviral == 0)) {
                                        org_inhibited(org, o);
                                    }
                                } else {
                                    if (o->fallowversion == 1) {
                                        if ((org->sporetime > 0) && (org->sporeversion > 0) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                            org->sporetime = -org->sporetime;
                                            org_set_color(o, CM_FALLOW);
                                            org_set_color(org, CM_FLOWER);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_RED:
            if ((org->fallowversion != o->fallowversion) || (org->isinfectious) || ((!o->isaconsumer) && (!o->isafungus)) || (o->isenhanced)) {
                if (((o->isenhanced) && (org->createlavender == 0)) || (!o->isaplant) || (o->fallowversion == 1)) {
                    if (org->lavender > 0) {
                        if ((o->isenhanced) && (!o->isinfectious)) {
                            org_weaklavendershield(org);
                        } else {
                            org_lavendershield(org);
                        }
                        org_set_color(o, CM_FALLOW);
                    }
                    if (org->lavender <= 0) {
                        if (org->infected_gc != o->gc) {
                            if (o->fallowversion <= 2) {
                                if ((o->fallowversion == 2) || (org->sporeversion > 0)) {
                                    if (org->time_to_reproduce < org->time_to_reproduce_max + j_d2i((10 * o->m[seg]))) {
                                        if (org_use_energy(o, P.fallow_energy_consumption)) {
                                            org_set_color(o, CM_FALLOW);
                                            org_set_color(org, CM_FLOWER);
                                            org->time_to_reproduce = org->time_to_reproduce_max + j_d2i((10 * o->m[seg]));
                                        }
                                    }
                                }
                            } else {
                                if (o->fallowversion == 4) {
                                    if (org->time_to_reproduce < org->time_to_reproduce_max + j_d2i((5 * o->m[seg]))) {
                                        if (org_use_energy(o, P.fallow_energy_consumption)) {
                                            org_set_color(o, CM_FALLOW);
                                            org_set_color(org, CM_FLOWER);
                                            org->time_to_reproduce = org->time_to_reproduce_max + j_d2i((5 * o->m[seg]));
                                        }
                                    }
                                } else {
                                    if (o->isinfectious) {
                                        if (org->time_to_reproduce < org->time_to_reproduce_max) {
                                            if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                org_set_color(o, CM_FALLOW);
                                                org_set_color(org, CM_FLOWER);
                                                org->time_to_reproduce = org->time_to_reproduce_max;
                                            }
                                        }
                                    } else {
                                        if (org->time_to_reproduce < org->time_to_reproduce_max + j_d2i((o->m[seg]))) {
                                            if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                org_set_color(o, CM_FALLOW);
                                                org_set_color(org, CM_FLOWER);
                                                org->time_to_reproduce = org->time_to_reproduce_max + j_d2i((o->m[seg]));
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if ((org->time_to_reproduce > 4) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                org_set_color(o, CM_FALLOW);
                                org_set_color(org, CM_BLOND);
                                org->time_to_reproduce = 4;
                            }
                        }
                        if (o->fallowversion == 3) {
                            if (org->reproducelate <= 0) {
                                if (o->isinfectious) {
                                    if ((org->reproducelate > -2) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                        org->reproducelate = -2;
                                        org_set_color(o, CM_FALLOW);
                                        org_set_color(org, CM_FLOWER);
                                    }
                                } else {
                                    if ((org->reproducelate > -j_d2i(((o->m[seg] / 2) + 1))) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                        org->reproducelate = -j_d2i(((o->m[seg] / 2) + 1));
                                        org_set_color(o, CM_FALLOW);
                                        org_set_color(org, CM_FLOWER);
                                    }
                                }
                            }
                        } else {
                            if (o->fallowversion == 4) {
                                if ((org->isreproductive) && (org->antiviral == 0)) {
                                    org_inhibited(org, o);
                                }
                            } else {
                                if (o->fallowversion == 1) {
                                    if ((org->sporetime > 0) && (org->sporeversion > 0) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                        org->sporetime = -org->sporetime;
                                        org_set_color(o, CM_FALLOW);
                                        org_set_color(org, CM_FLOWER);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_LAVENDER:
            if ((org->isaplant) || (org->isaconsumer) || ((org->isafungus) && (((o->fallowversion == 2) && (org->antiviral == 0)) || (!o->isaplant)))) {
                if (org->isenhanced) {
                    org_use_energy(o, bio_between((1 / P.lavender_energy_consumption), 0, o->energy));
                    if (org->lavender < P.lavender_shield) {
                        org->lavender += 1000;
                        if (org->lavender >= P.lavender_shield) {
                            org->lavender = P.lavender_shield;
                        }
                    }
                } else {
                    org_use_energy(o, bio_between((0.2 / P.lavender_energy_consumption), 0, o->energy));
                    if (org->lavender < P.lavender_shield) {
                        org->lavender += 200;
                        if (org->lavender >= P.lavender_shield) {
                            org->lavender = P.lavender_shield;
                        }
                    }
                }
                // The other organism will be shown in lavender
                org_set_color(org, CM_LAVENDER);
                // This organism will be shown in deadbark
                org_set_color(o, CM_DEADBARK);
                if (o->energy < BIO_TOL) {
                    org_die(o, org);
                }
            }
            break;
        case TY_MINT:
            if (o->isaplant) {
                if (org_use_energy(org, P.mint_energy_consumption / 10)) {
                    o->seg_color[seg] = CM_BROKEN;
                    org_set_color(org, CM_MINT);
                    o->isinjured = true;
                    o->update_effects = 2;
                }
            } else {
                if ((o->isaconsumer) || (o->iscoral)) {
                    if (org_use_energy(org, P.mint_energy_consumption / 10)) {
                        o->seg_color[seg] = CM_LIGHTBROWN;
                        org_set_color(org, CM_MINT);
                        o->isinjured = true;
                        o->update_effects = 2;
                    }
                } else {
                    if ((o->antiviral == 0) || (org->isaplant) || (org->isaconsumer) || (org->isafungus) || (org->iscoral)) {
                        if (org_use_energy(org, P.mint_energy_consumption / 10)) {
                            o->seg_color[seg] = CM_LIGHTBROWN;
                            org_set_color(org, CM_MINT);
                            o->isinjured = true;
                            o->update_effects = 2;
                        }
                    }
                }
            }
            break;
        case TY_CREAM:
        case TY_SPIKEPOINT:
        case TY_OLIVE:
        case TY_FLOWER:
        case TY_DARKGRAY:
        case TY_CORAL:
        case TY_FRUIT:
            break;
        case TY_BROWN:
        case TY_VISION:
            if ((org->alive) && (!org->active)) {
                if ((o->fallowversion != 2) || (!o->isinfectious)) {
                    if (org->lavender > 0) {
                        if ((o->isenhanced) && (!o->isinfectious)) {
                            org_weaklavendershield(org);
                        } else {
                            org_lavendershield(org);
                        }
                        org_set_color(o, CM_FALLOW);
                    }
                    if ((org->lavender <= 0) || ((!o->isaplant) && (!o->isinfectious)) || (o->fallowversion == 1)) {
                        if (org_use_energy(o, P.fallow_energy_consumption)) {
                            for (int y = 0; y < org->segments; y++) {
                                switch (org_get_type_color(org->seg_color[y])) {
                                case TY_BROWN:
                                case TY_VISION:
                                    org->seg_color[y] = CM_LIGHTBROWN;
                                    break;
                                }
                            }
                            org_set_color(o, CM_FALLOW);
                            org->frames_color = 0;
                            org->isinjured = true;
                            org->isfrozen = true;
                            org->allfrozen = true;
                        }
                    }
                }
            }
            break;
        case TY_BARK:
            org->seg_color[oseg] = CM_OLDBARK;
            if (org->active) {
                org->photosynthesis -= org->mphoto[oseg] * k_scale[org->growth_ratio - 1];
            } else {
                if ((o->fallowversion != 2) || (!o->isinfectious)) {
                    if (org->lavender > 0) {
                        if ((o->isenhanced) && (!o->isinfectious)) {
                            org_weaklavendershield(org);
                        } else {
                            org_lavendershield(org);
                        }
                        org_set_color(o, CM_FALLOW);
                    }
                    if ((org->lavender <= 0) || ((!o->isaplant) && (!o->isinfectious)) || (o->fallowversion == 1)) {
                        if (org_use_energy(o, P.fallow_energy_consumption)) {
                            for (int y = 0; y < org->segments; y++) {
                                switch (org_get_type_color(org->seg_color[y])) {
                                case TY_BARK:
                                case TY_OLDBARK:
                                    org->seg_color[y] = CM_BROKEN;
                                    break;
                                }
                            }
                            org_set_color(o, CM_FALLOW);
                            org->frames_color = 0;
                            org->isinjured = true;
                            org->isfrozen = true;
                            org->allfrozen = true;
                        }
                    }
                }
            }
            break;
        default:
            if ((!org->active) && (org->seg_color[oseg] == CM_OLDBARK)) {
                if ((o->fallowversion != 2) || (!o->isinfectious)) {
                    if (org->lavender > 0) {
                        if ((o->isenhanced) && (!o->isinfectious)) {
                            org_weaklavendershield(org);
                        } else {
                            org_lavendershield(org);
                        }
                        org_set_color(o, CM_FALLOW);
                    }
                    if ((org->lavender <= 0) || ((!o->isaplant) && (!o->isinfectious)) || (o->fallowversion == 1)) {
                        if (org_use_energy(o, P.fallow_energy_consumption)) {
                            for (int y = 0; y < org->segments; y++) {
                                switch (org_get_type_color(org->seg_color[y])) {
                                case TY_BARK:
                                case TY_OLDBARK:
                                    org->seg_color[y] = CM_BROKEN;
                                    break;
                                }
                            }
                            org_set_color(o, CM_FALLOW);
                            org->frames_color = 0;
                            org->isinjured = true;
                            org->isfrozen = true;
                            org->allfrozen = true;
                        }
                    }
                }
            } else {
                if ((org->fallowversion != o->fallowversion) || (org->isinfectious) || ((!o->isaconsumer) && (!o->isafungus)) || (o->isenhanced)) {
                    if ((org->isaplant) || (org->isaconsumer) || ((org->isafungus) && (((o->fallowversion == 2) && (org->antiviral == 0)) || (!o->isaplant)))) {
                        if (org->lavender > 0) {
                            if ((o->isenhanced) && (!o->isinfectious)) {
                                org_weaklavendershield(org);
                            } else {
                                org_lavendershield(org);
                            }
                            org_set_color(o, CM_FALLOW);
                        }
                        if (org->lavender <= 0) {
                            if (org->infected_gc != o->gc) {
                                if (o->fallowversion <= 2) {
                                    if ((o->fallowversion == 2) || (org->sporeversion > 0)) {
                                        if (org->time_to_reproduce < org->time_to_reproduce_max + j_d2i((10 * o->m[seg]))) {
                                            if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                org_set_color(o, CM_FALLOW);
                                                org_set_color(org, CM_FLOWER);
                                                org->time_to_reproduce = org->time_to_reproduce_max + j_d2i((10 * o->m[seg]));
                                            }
                                        }
                                    }
                                } else {
                                    if (o->fallowversion == 4) {
                                        if (org->time_to_reproduce < org->time_to_reproduce_max + j_d2i((5 * o->m[seg]))) {
                                            if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                org_set_color(o, CM_FALLOW);
                                                org_set_color(org, CM_FLOWER);
                                                org->time_to_reproduce = org->time_to_reproduce_max + j_d2i((5 * o->m[seg]));
                                            }
                                        }
                                    } else {
                                        if (o->isinfectious) {
                                            if (org->time_to_reproduce < org->time_to_reproduce_max) {
                                                if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                    org_set_color(o, CM_FALLOW);
                                                    org_set_color(org, CM_FLOWER);
                                                    org->time_to_reproduce = org->time_to_reproduce_max;
                                                }
                                            }
                                        } else {
                                            if (org->time_to_reproduce < org->time_to_reproduce_max + j_d2i((o->m[seg]))) {
                                                if (org_use_energy(o, P.fallow_energy_consumption)) {
                                                    org_set_color(o, CM_FALLOW);
                                                    org_set_color(org, CM_FLOWER);
                                                    org->time_to_reproduce = org->time_to_reproduce_max + j_d2i((o->m[seg]));
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                if ((org->time_to_reproduce > 4) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                    org_set_color(o, CM_FALLOW);
                                    org_set_color(org, CM_BLOND);
                                    org->time_to_reproduce = 4;
                                }
                            }
                            if (o->fallowversion == 3) {
                                if (org->reproducelate <= 0) {
                                    if (o->isinfectious) {
                                        if ((org->reproducelate > -2) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                            org->reproducelate = -2;
                                            org_set_color(o, CM_FALLOW);
                                            org_set_color(org, CM_FLOWER);
                                        }
                                    } else {
                                        if ((org->reproducelate > -j_d2i(((o->m[seg] / 2) + 1))) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                            org->reproducelate = -j_d2i(((o->m[seg] / 2) + 1));
                                            org_set_color(o, CM_FALLOW);
                                            org_set_color(org, CM_FLOWER);
                                        }
                                    }
                                }
                            } else {
                                if (o->fallowversion == 4) {
                                    if ((org->isreproductive) && ((org->isaconsumer) || (org->isaplant)) && (org->antiviral == 0)) {
                                        org_inhibited(org, o);
                                    }
                                } else {
                                    if (o->fallowversion == 1) {
                                        if ((org->sporetime > 0) && (org->sporeversion > 0) && (org_use_energy(o, P.fallow_energy_consumption))) {
                                            org->sporetime = -org->sporetime;
                                            org_set_color(o, CM_FALLOW);
                                            org_set_color(org, CM_FLOWER);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        break;
    case TY_VIOLET:
        // Violet segment: Poison another segment and make it useless
        if (org->hasdodged == false) {
            if ((!org->isonlyc4) || (org->frames_color > 8)) {
                org->hasdodged = true;
            }
        }
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_LIGHTBROWN:
        case TY_GREENBROWN:
        case TY_OLDBARK:
            break;
        case TY_DARKFIRE:
            if ((org->mphoto[oseg] != -1) && (org->active) && (org_use_energy(o, P.violet_energy_consumption))) {
                if (org->isaplant) {
                    org->seg_color[oseg] = CM_GREENBROWN;
                } else {
                    org->seg_color[oseg] = CM_LIGHTBROWN;
                }
                org_set_color(o, CM_VIOLET);
                org->isinjured = true;
            }
            break;
        case TY_VIOLET:
        case TY_WHITE:
            if ((!o->isaconsumer) && (!o->isafungus) && ((o->isaplant) || (o->iscoral) || (org->iscoral))) {
                if (org_use_energy(o, P.violet_energy_consumption)) {
                    if (org->isaplant) {
                        org->seg_color[oseg] = CM_GREENBROWN;
                    } else {
                        org->seg_color[oseg] = CM_LIGHTBROWN;
                    }
                    org_set_color(o, CM_VIOLET);
                    org->isinjured = true;
                }
            } else {
                if ((org->isaplant) || (org->isaconsumer) || (org->isafungus) || (org->plagueversion > 0) || (org->isauburn)) {
                    if (org_use_energy(o, P.violet_energy_consumption)) {
                        if (org->isaplant) {
                            org->seg_color[oseg] = CM_GREENBROWN;
                        } else {
                            org->seg_color[oseg] = CM_LIGHTBROWN;
                        }
                        org_set_color(o, CM_VIOLET);
                        org->isinjured = true;
                    }
                }
            }
            break;
        case TY_GREEN:
        case TY_GRASS:
        case TY_FOREST:
        case TY_SUMMER:
        case TY_LIME:
        case TY_LEAF:
        case TY_C4:
        case TY_PURPLE:
            if ((o->isaconsumer) || (o->isaplant) || (o->isafungus)) {
                if (org->active) {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, CM_VIOLET);
                    } else {
                        if (org_use_energy(o, P.violet_energy_consumption)) {
                            org->seg_color[oseg] = CM_GREENBROWN;
                            org->update_effects = 2;
                            org_set_color(o, CM_VIOLET);
                            org->isinjured = true;
                        }
                    }
                } else {
                    if (org_use_energy(o, P.violet_energy_consumption)) {
                        for (int a = 0; a < org->segments; a++) {
                            switch (org_get_type_color(org->seg_color[a])) {
                            case TY_GREEN:
                            case TY_FOREST:
                            case TY_SPRING:
                            case TY_SUMMER:
                            case TY_WINTER:
                            case TY_LIME:
                            case TY_LEAF:
                            case TY_C4:
                            case TY_GRASS:
                            case TY_JADE:
                            case TY_DARKJADE:
                            case TY_DARKGREEN:
                            case TY_PURPLE:
                                org->seg_color[a] = CM_GREENBROWN;
                                break;
                            }
                        }
                        org_set_color(o, CM_VIOLET);
                        org->frames_color = 0;
                        org->isinjured = true;
                        org->isfrozen = true;
                        org->allfrozen = true;
                    }
                }
            }
            break;
        case TY_SPRING:
        case TY_DARKGREEN:
            if ((o->isaconsumer) || (o->isaplant) || (o->isafungus)) {
                if (org->active) {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, CM_VIOLET);
                    } else {
                        if (org_use_energy(o, P.violet_energy_consumption)) {
                            org->seg_color[oseg] = CM_GREENBROWN;
                            org->update_effects = 2;
                            org_set_color(o, CM_VIOLET);
                            org->isinjured = true;
                            org_segments_check_plant(org);
                        }
                    }
                } else {
                    if (org_use_energy(o, P.violet_energy_consumption)) {
                        for (int a = 0; a < org->segments; a++) {
                            switch (org_get_type_color(org->seg_color[a])) {
                            case TY_GREEN:
                            case TY_FOREST:
                            case TY_SPRING:
                            case TY_SUMMER:
                            case TY_WINTER:
                            case TY_LIME:
                            case TY_LEAF:
                            case TY_C4:
                            case TY_GRASS:
                            case TY_JADE:
                            case TY_DARKJADE:
                            case TY_DARKGREEN:
                            case TY_PURPLE:
                                org->seg_color[a] = CM_GREENBROWN;
                                break;
                            }
                        }
                        org_set_color(o, CM_VIOLET);
                        org->frames_color = 0;
                        org->isinjured = true;
                        org->isfrozen = true;
                        org->allfrozen = true;
                    }
                }
            }
            break;
        case TY_WINTER:
            if ((o->isaconsumer) || (o->isaplant) || (o->isafungus)) {
                if (org->active) {
                    if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                        org_set_color(org, CM_TEAL);
                        org_set_color(o, CM_VIOLET);
                    } else {
                        if (org_use_energy(o, P.violet_energy_consumption)) {
                            org->seg_color[oseg] = CM_GREENBROWN;
                            org_set_color(o, CM_VIOLET);
                            org->isinjured = true;
                        }
                    }
                } else {
                    if (org_use_energy(o, P.violet_energy_consumption)) {
                        for (int a = 0; a < org->segments; a++) {
                            switch (org_get_type_color(org->seg_color[a])) {
                            case TY_GREEN:
                            case TY_FOREST:
                            case TY_SPRING:
                            case TY_SUMMER:
                            case TY_WINTER:
                            case TY_LIME:
                            case TY_LEAF:
                            case TY_C4:
                            case TY_GRASS:
                            case TY_JADE:
                            case TY_DARKJADE:
                            case TY_DARKGREEN:
                            case TY_PURPLE:
                                org->seg_color[a] = CM_GREENBROWN;
                                break;
                            }
                        }
                        org_set_color(o, CM_VIOLET);
                        org->frames_color = 0;
                        org->isinjured = true;
                        org->isfrozen = true;
                        org->allfrozen = true;
                    }
                }
            }
            break;
        case TY_JADE:
            if ((o->isaconsumer) || (o->isaplant) || (o->isafungus)) {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_VIOLET);
                } else {
                    org->seg_color[oseg] = CM_DARKJADE;
                    org_set_color(o, CM_VIOLET);
                    org->useextraeffects = true;
                }
            }
            break;
        case TY_DARKJADE:
            if ((o->isaconsumer) || (o->isaplant) || (o->isafungus)) {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_VIOLET);
                } else {
                    org->seg_color[oseg] = CM_DARKGREEN;
                    org_set_color(o, CM_VIOLET);
                    org->useextraeffects = true;
                }
            }
            break;
        case TY_CYAN:
            if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                org_set_color(org, CM_TEAL);
                org_set_color(o, CM_VIOLET);
            } else {
                if (org_use_energy(o, P.violet_energy_consumption)) {
                    if (org->isaplant) {
                        org->seg_color[oseg] = CM_GREENBROWN;
                    } else {
                        org->seg_color[oseg] = CM_LIGHTBROWN;
                    }
                    org->mphoto[oseg] = 0;
                    org_set_color(o, CM_VIOLET);
                    org->isinjured = true;
                }
            }
            break;
        case TY_TEAL:
            if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                org_set_color(org, CM_TEAL);
                org_set_color(o, CM_VIOLET);
            } else {
                if (org_use_energy(o, P.violet_energy_consumption)) {
                    if (org->isaplant) {
                        org->seg_color[oseg] = CM_GREENBROWN;
                    } else {
                        org->seg_color[oseg] = CM_LIGHTBROWN;
                    }
                    org->mphoto[oseg] = 0;
                    org_set_color(o, CM_VIOLET);
                    if (gc_get_passive(org->gc)) {
                        org->update_effects = 2;
                    }
                    org->isinjured = true;
                }
            }
            break;
        case TY_AUBURN:
        case TY_INDIGO:
        case TY_FLOWER:
        case TY_GOLD:
        case TY_SPORE:
            if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                org_set_color(org, CM_TEAL);
                org_set_color(o, CM_VIOLET);
            } else {
                if (org_use_energy(o, P.violet_energy_consumption)) {
                    if (org->isaplant) {
                        org->seg_color[oseg] = CM_GREENBROWN;
                    } else {
                        org->seg_color[oseg] = CM_LIGHTBROWN;
                    }
                    org_set_color(o, CM_VIOLET);
                    org_segments_restore_effects(org);
                    org->update_effects = 2;
                }
            }
            break;
        case TY_YELLOW:
        case TY_BLOND:
            if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                org_set_color(org, CM_TEAL);
                org_set_color(o, CM_VIOLET);
            } else {
                if (org_use_energy(o, P.violet_energy_consumption)) {
                    if (org->isaplant) {
                        org->seg_color[oseg] = CM_GREENBROWN;
                    } else {
                        org->seg_color[oseg] = CM_LIGHTBROWN;
                    }
                    org_set_color(o, CM_VIOLET);
                    org_segments_restore_effects(org);
                }
            }
            break;
        case TY_DARKGRAY:
            if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                org_set_color(org, CM_TEAL);
                org_set_color(o, CM_VIOLET);
            } else {
                if (org_use_energy(o, P.violet_energy_consumption)) {
                    if (org->isaplant) {
                        org->seg_color[oseg] = CM_GREENBROWN;
                    } else {
                        org->seg_color[oseg] = CM_LIGHTBROWN;
                    }
                    org_set_color(o, CM_VIOLET);
                    org->isinjured = true;
                }
            }
            break;
        case TY_BLUE:
            if (org_use_energy(org, P.blue_energy_consumption)) {
                if (org->isenhanced) {
                    org_use_energy(o, bio_between((0.5 * sqrt(org->m[oseg])) * P.organic_obtained_energy, 0, o->energy));
                    org_set_color(o, CM_DARKLILAC);
                    if (o->energy < BIO_TOL) {
                        org_die(o, org);
                    }
                } else {
                    org_set_color(o, CM_VIOLET);
                }
                org_set_color(org, JC_BLUE);
            } else {
                if (org_use_energy(o, P.violet_energy_consumption)) {
                    if (org->isaplant) {
                        org->seg_color[oseg] = CM_GREENBROWN;
                    } else {
                        org->seg_color[oseg] = CM_LIGHTBROWN;
                    }
                    org_set_color(o, CM_VIOLET);
                    if (!org->isaplant) {
                        org->update_effects = 2;
                    }
                    org->isinjured = true;
                }
            }
            break;
        case TY_SKY:
            if ((org->skyversion == 2) || (org->isaplant)) {
                if (org_use_energy(org, P.sky_energy_consumption)) {
                    org->seg_color[oseg] = CM_DEEPSKY;
                    org->useextraeffects = true;
                    org_set_color(org, CM_DEEPSKY);
                    org_set_color(o, CM_VIOLET);
                } else {
                    if (org_use_energy(o, P.violet_energy_consumption)) {
                        if (org->isaplant) {
                            org->seg_color[oseg] = CM_GREENBROWN;
                        } else {
                            org->seg_color[oseg] = CM_LIGHTBROWN;
                        }
                        org_set_color(o, CM_VIOLET);
                        org_segments_restore_effects(org);
                        if ((!org->isaplant) && (!gc_get_modifiessky(org->gc))) {
                            org->update_effects = 2;
                        }
                    }
                }
            }
            break;
        case TY_DEEPSKY:
            if ((org->skyversion == 2) || (org->isaplant)) {
                org_set_color(org, CM_DEEPSKY);
                org_set_color(o, CM_VIOLET);
            }
            break;
        case TY_ROSE:
            if (o->altruist) {
                break;
            } else {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_VIOLET);
                } else {
                    if (org_use_energy(o, P.violet_energy_consumption)) {
                        if (org->isaplant) {
                            org->seg_color[oseg] = CM_GREENBROWN;
                        } else {
                            org->seg_color[oseg] = CM_LIGHTBROWN;
                        }
                        org_set_color(o, CM_VIOLET);
                        org->update_effects = 2;
                        org->isinjured = true;
                    }
                }
            }
            break;
        case TY_MAGENTA:
            if (o->altruist) {
                break;
            } else {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_VIOLET);
                } else {
                    if ((org->isenhanced) && (!org->isaconsumer) && (!org->isafungus) && (org_use_energy(org, P.magenta_energy_consumption))) {
                        org_set_color(org, JC_MAGENTA);
                        org_set_color(o, CM_VIOLET);
                    } else {
                        if (org_use_energy(o, P.violet_energy_consumption)) {
                            if (org->isaplant) {
                                org->seg_color[oseg] = CM_GREENBROWN;
                            } else {
                                org->seg_color[oseg] = CM_LIGHTBROWN;
                            }
                            org_set_color(o, CM_VIOLET);
                            org_segments_restore_effects(org);
                        }
                    }
                }
            }
            break;
        case TY_MINT:
            if (o->altruist) {
                break;
            } else {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_VIOLET);
                } else {
                    if (org_use_energy(o, P.violet_energy_consumption)) {
                        if (org->isaplant) {
                            org->seg_color[oseg] = CM_GREENBROWN;
                        } else {
                            org->seg_color[oseg] = CM_LIGHTBROWN;
                        }
                        org_set_color(o, CM_VIOLET);
                        org_segments_restore_effects(org);
                    }
                }
            }
            break;
        case TY_LAVENDER:
            if (o->altruist) {
                break;
            } else {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_VIOLET);
                } else {
                    if (org_use_energy(o, P.violet_energy_consumption)) {
                        if (org->isaplant) {
                            org->seg_color[oseg] = CM_GREENBROWN;
                        } else {
                            org->seg_color[oseg] = CM_LIGHTBROWN;
                        }
                        org_set_color(o, CM_VIOLET);
                        org_segments_restore_effects(org);
                        org->update_effects = 2;
                    }
                }
            }
            break;
        case TY_OLIVE:
            if (org_use_energy(org, P.olive_energy_consumption)) {
                if (o->isaplant) {
                    o->seg_color[seg] = CM_BROKEN;
                } else {
                    o->seg_color[seg] = CM_LIGHTBROWN;
                }
                o->isinjured = true;
                if ((org->healing == 0) && (org_use_energy(o, P.violet_energy_consumption))) {
                    org->seg_color[oseg] = CM_DARKOLIVE;
                    org->useextraeffects = true;
                    org_set_color(org, CM_DARKOLIVE);
                    org_set_color(o, CM_VIOLET);
                } else {
                    org_set_color(org, CM_OLIVE);
                }
            } else {
                if (org_use_energy(o, P.violet_energy_consumption)) {
                    if (org->isaplant) {
                        org->seg_color[oseg] = CM_GREENBROWN;
                    } else {
                        org->seg_color[oseg] = CM_LIGHTBROWN;
                    }
                    org_set_color(o, CM_VIOLET);
                    if (!org->isaplant) {
                        org->update_effects = 2;
                    }
                    org->isinjured = true;
                }
            }
            break;
        case TY_DARKOLIVE:
            if (org_use_energy(org, P.olive_energy_consumption)) {
                if (o->isaplant) {
                    o->seg_color[seg] = CM_BROKEN;
                } else {
                    o->seg_color[seg] = CM_LIGHTBROWN;
                }
                o->isinjured = true;
                if ((org->healing == 0) && (org_use_energy(o, P.violet_energy_consumption))) {
                    org->seg_color[oseg] = CM_LIGHT_BLUE;
                    org_set_color(org, CM_DARKOLIVE);
                    org_set_color(o, CM_VIOLET);
                    if (!org->isaplant) {
                        org->update_effects = 2;
                    }
                    org->isinjured = true;
                } else {
                    org_set_color(org, CM_OLIVE);
                }
            } else {
                if (org_use_energy(o, P.violet_energy_consumption)) {
                    if (org->isaplant) {
                        org->seg_color[oseg] = CM_GREENBROWN;
                    } else {
                        org->seg_color[oseg] = CM_LIGHTBROWN;
                    }
                    org_set_color(o, CM_VIOLET);
                    if (!org->isaplant) {
                        org->update_effects = 2;
                    }
                    org->isinjured = true;
                }
            }
            break;
        case TY_SILVER:
            if (((!o->isaconsumer) && (!o->isafungus)) || (o->total_kills >= org->total_kills) || ((!org->isaconsumer) && (!org->isafungus))) {
                if (org_use_energy(o, P.violet_energy_consumption)) {
                    if (org->isaplant) {
                        org->seg_color[oseg] = CM_GREENBROWN;
                    } else {
                        org->seg_color[oseg] = CM_LIGHTBROWN;
                    }
                    org_set_color(o, CM_VIOLET);
                    org->isinjured = true;
                }
            }
            break;
        case TY_FIRE:
            if (((!o->isaconsumer) && (!o->isafungus)) || (org->isenhanced)) {
                if (org_use_energy(o, P.violet_energy_consumption)) {
                    if (org->isaplant) {
                        org->seg_color[oseg] = CM_GREENBROWN;
                    } else {
                        org->seg_color[oseg] = CM_LIGHTBROWN;
                    }
                    org_set_color(o, CM_VIOLET);
                    org->isinjured = true;
                }
            } else {
                if (org_use_energy(o, P.violet_energy_consumption)) {
                    org->seg_color[oseg] = CM_DARKFIRE;
                    org->useextraeffects = true;
                    org_set_color(o, CM_VIOLET);
                    org->isinjured = true;
                }
            }
            break;
        case TY_CREAM:
            if ((!o->isaconsumer) && (!o->isafungus) && (!o->isinfectious)) {
                if (org_use_energy(o, P.violet_energy_consumption)) {
                    if (org->isaplant) {
                        org->seg_color[oseg] = CM_GREENBROWN;
                    } else {
                        org->seg_color[oseg] = CM_LIGHTBROWN;
                    }
                    org_set_color(o, CM_VIOLET);
                    org->isinjured = true;
                }
            } else {
                if (org_use_energy(o, P.violet_energy_consumption)) {
                    org->seg_color[oseg] = CM_DARKFIRE;
                    org->useextraeffects = true;
                    org_set_color(o, CM_VIOLET);
                    org->isinjured = true;
                }
            }
            break;
        case TY_PLAGUE:
            if ((org->isinfectious) || ((!o->isaconsumer) && (!o->isafungus))) {
                if (org_use_energy(o, P.violet_energy_consumption)) {
                    if (org->isaplant) {
                        org->seg_color[oseg] = CM_GREENBROWN;
                    } else {
                        org->seg_color[oseg] = CM_LIGHTBROWN;
                    }
                    org_set_color(o, CM_VIOLET);
                    org->isinjured = true;
                }
            }
            break;
        case TY_FRUIT:
            if ((!o->isaconsumer) && (!o->isafungus)) {
                if (org_use_energy(o, P.violet_energy_consumption)) {
                    org->seg_color[oseg] = CM_DARKFIRE;
                    org_set_color(o, CM_VIOLET);
                    org->isinjured = true;
                }
            }
            break;
        case TY_SPIKE:
            if ((org->isaplant) || (org->isenhanced)) {
                if (org_use_energy(o, P.violet_energy_consumption)) {
                    if (org->isaplant) {
                        org->seg_color[oseg] = CM_GREENBROWN;
                    } else {
                        org->seg_color[oseg] = CM_LIGHTBROWN;
                    }
                    org_set_color(o, CM_VIOLET);
                    org->isinjured = true;
                }
            }
            break;
        case TY_LILAC:
        case TY_DARKLILAC:
            if (((!o->isaconsumer) && (!o->isafungus)) || (o->isenhanced)) {
                if (org_use_energy(o, P.violet_energy_consumption)) {
                    if (org->isaplant) {
                        org->seg_color[oseg] = CM_GREENBROWN;
                    } else {
                        org->seg_color[oseg] = CM_LIGHTBROWN;
                    }
                    org_set_color(o, CM_VIOLET);
                    org->isinjured = true;
                }
            }
            break;
        case TY_FALLOW:
            if ((!o->isaconsumer) && (!o->isafungus) && (!o->isinfectious)) {
                if (org_use_energy(o, P.violet_energy_consumption)) {
                    if (org->isaplant) {
                        org->seg_color[oseg] = CM_GREENBROWN;
                    } else {
                        org->seg_color[oseg] = CM_LIGHTBROWN;
                    }
                    org_set_color(o, CM_VIOLET);
                    org->update_effects = 2;
                    org->isinjured = true;
                }
            }
            break;
        case TY_CORAL:
            if ((!o->isaplant) && (!o->isaconsumer) && (!o->isafungus)) {
                if (org_use_energy(o, P.violet_energy_consumption)) {
                    if (org->isaplant) {
                        org->seg_color[oseg] = CM_GREENBROWN;
                    } else {
                        org->seg_color[oseg] = CM_LIGHTBROWN;
                    }
                    org_set_color(o, CM_VIOLET);
                    if ((org->gold > 0) && (!org->isaplant) && (!org->isaconsumer) && (!org->isafungus)) {
                        org->update_effects = 2;
                    }
                    org->isinjured = true;
                }
            } else {
                if ((org->healing > 0) && (org->frames_color == 0) && ((o->isaconsumer) || (o->isafungus)) && (org_use_energy(org, P.magenta_energy_consumption))) {
                    org_set_color(org, JC_MAGENTA);
                    org_set_color(o, CM_VIOLET);
                } else {
                    if (org_use_energy(o, P.violet_energy_consumption)) {
                        if (org->isaplant) {
                            org->seg_color[oseg] = CM_GREENBROWN;
                        } else {
                            org->seg_color[oseg] = CM_LIGHTBROWN;
                        }
                        org_set_color(o, CM_VIOLET);
                        if ((org->gold > 0) && (!org->isaplant) && (!org->isaconsumer) && (!org->isafungus)) {
                            org->update_effects = 2;
                        }
                        org->isinjured = true;
                    }
                }
            }
            break;
        case TY_MAROON:
            if ((org->healing > 0) && (org->frames_color == 0) && ((o->isaconsumer) || (o->isafungus)) && (org_use_energy(org, P.magenta_energy_consumption))) {
                org_set_color(org, JC_MAGENTA);
                org_set_color(o, CM_VIOLET);
            } else {
                if (org_use_energy(o, P.violet_energy_consumption)) {
                    if (org->isaplant) {
                        org->seg_color[oseg] = CM_GREENBROWN;
                    } else {
                        org->seg_color[oseg] = CM_LIGHTBROWN;
                    }
                    org_set_color(o, CM_VIOLET);
                    org->update_effects = 2;
                    org->isinjured = true;
                }
            }
            break;
        case TY_PINK:
            if (org_use_energy(o, P.violet_energy_consumption)) {
                if (org->isaplant) {
                    org->seg_color[oseg] = CM_GREENBROWN;
                } else {
                    org->seg_color[oseg] = CM_LIGHTBROWN;
                }
                org_set_color(o, CM_VIOLET);
                org->update_effects = 2;
                org->isinjured = true;
            }
            break;
        case TY_BARK:
            org->seg_color[oseg] = CM_OLDBARK;
            if (org->active) {
                org->photosynthesis -= org->mphoto[oseg] * k_scale[org->growth_ratio - 1];
            }
            break;
        case TY_OCHRE:
        case TY_SPIKEPOINT:
        case TY_VISION:
        case TY_ICE:
        case TY_DEADBARK:
        case TY_BROWN:
            break;
        default:
            if (org_use_energy(o, P.violet_energy_consumption)) {
                if (org->isaplant) {
                    org->seg_color[oseg] = CM_GREENBROWN;
                } else {
                    org->seg_color[oseg] = CM_LIGHTBROWN;
                }
                org_set_color(o, CM_VIOLET);
                org->isinjured = true;
            }
        }
        break;
    case TY_SKY:
        // Sky segment: Freeze another organism by disabling its photosynthetic and movement segments, protects itself with a frost shield
        if (org->hasdodged == false) {
            if ((o->skyversion == 1) || ((!org->canreact) && (org->frames_color > 0))) {
                if ((!org->isonlyc4) || (!org->isenhanced)) {
                    org->dodge = false;
                }
            }
            if ((!org->isonlyc4) || (!org->isenhanced) || (org->frames_color > 8)) {
                org->hasdodged = true;
            }
        }
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_SKY:
            if (o->skyversion == 1) {
                if (org->canmove) {
                    if (org->skyversion == 2) {
                        org_paralyzed(org, o);
                    }
                }
            } else {
                if (org->skyversion == 2) {
                    if (org_use_energy(o, P.sky_energy_consumption)) {
                        if (org_use_energy(org, P.sky_energy_consumption)) {
                            o->seg_color[seg] = CM_DEEPSKY;
                            o->useextraeffects = true;
                            org->seg_color[oseg] = CM_DEEPSKY;
                            org->useextraeffects = true;
                            org_set_color(o, CM_DEEPSKY);
                            org_set_color(org, CM_DEEPSKY);
                        } else {
                            if (!org->allfrozen) {
                                org_frozen(org, o);
                            }
                        }
                    }
                } else {
                    if ((o->isaplant) && (!o->isenhanced) && (org_use_energy(org, 12 * P.sky_energy_consumption))) {
                        org->seg_color[oseg] = CM_DEEPSKY;
                        org->useextraeffects = true;
                        org_set_color(o, CM_SKY);
                        org_set_color(org, CM_DEEPSKY);
                    } else {
                        if (!org->allfrozen) {
                            org_frozen(org, o);
                        }
                    }
                }
            }
            break;
        case TY_DEEPSKY:
            if (o->skyversion == 1) {
                if (org->canmove) {
                    if (org->skyversion == 2) {
                        org_paralyzed(org, o);
                    }
                }
            } else {
                if (org->skyversion == 2) {
                    if (!org->allfrozen) {
                        org_frozen(org, o);
                    }
                } else {
                    if ((o->isaplant) && (!o->isenhanced) && (org_use_energy(org, 12 * P.sky_energy_consumption))) {
                        org_set_color(o, CM_SKY);
                        org_set_color(org, CM_DEEPSKY);
                    } else {
                        if (!org->allfrozen) {
                            org_frozen(org, o);
                        }
                    }
                }
            }
            break;
        case TY_GREEN:
        case TY_GRASS:
        case TY_FOREST:
        case TY_SPRING:
        case TY_SUMMER:
        case TY_WINTER:
        case TY_LIME:
        case TY_LEAF:
        case TY_C4:
        case TY_DARKGREEN:
        case TY_PURPLE:
        case TY_TEAL:
        case TY_CYAN:
        case TY_YELLOW:
        case TY_AUBURN:
        case TY_INDIGO:
        case TY_BLOND:
        case TY_FLOWER:
        case TY_DARKGRAY:
        case TY_GOLD:
        case TY_SPORE:
            if ((org->dodge) && (org->frames_color == 0) && (org_use_energy(org, P.dodge_energy_consumption))) {
                org_set_color(org, CM_TEAL);
                org_set_color(o, CM_SKY);
            } else {
                if (o->skyversion == 1) {
                    if (org->canmove) {
                        if ((org->skyversion != 1) || (o->isaplant) || (org->isaplant) || (o->isaconsumer) || (org->isaconsumer) || (o->isafungus) || (org->isafungus)) {
                            org_paralyzed(org, o);
                        }
                    }
                } else {
                    if (!org->allfrozen) {
                        org_frozen(org, o);
                    }
                }
            }
            break;
        case TY_BLUE:
            if (org->active) {
                if (org_use_energy(org, P.blue_energy_consumption)) {
                    if (org->isenhanced) {
                        org_use_energy(o, bio_between((0.5 * sqrt(org->m[oseg])) * P.organic_obtained_energy, 0, o->energy));
                        org_set_color(o, CM_DARKLILAC);
                        if (o->energy < BIO_TOL) {
                            org_die(o, org);
                        }
                    } else {
                        org_set_color(o, CM_SKY);
                    }
                    org_set_color(org, JC_BLUE);
                } else {
                    if (o->skyversion == 1) {
                        if (org->canmove) {
                            if ((org->skyversion != 1) || (o->isaplant) || (org->isaplant) || (o->isaconsumer) || (org->isaconsumer) || (o->isafungus) || (org->isafungus)) {
                                if ((org->healing == 0) || (!o->isaplant) || (o->isenhanced)) {
                                    org_paralyzed(org, o);
                                }
                            }
                        }
                    } else {
                        if (!org->allfrozen) {
                            org_frozen(org, o);
                        }
                    }
                }
            } else {
                if (gc_get_modifiesspore(org->gc) >= 10) {
                    if ((o->skyversion == 2) || (!o->isaplant)) {
                        if (org_use_energy(o, P.sky_energy_consumption)) {
                            for (int a = 0; a < org->segments; a++) {
                                switch (org_get_type_color(org->seg_color[a])) {
                                case TY_BLUE:
                                    org->seg_color[a] = CM_ICE;
                                    break;
                                }
                            }
                            org_set_color(o, CM_SKY);
                            org->frames_color = 0;
                            org->isinjured = true;
                            org->isfrozen = true;
                            org->allfrozen = true;
                        }
                    }
                }
            }
            break;
        case TY_JADE:
            if ((org->dodge) && (org->frames_color == 0) && (org_use_energy(org, P.dodge_energy_consumption))) {
                org_set_color(org, CM_TEAL);
                org_set_color(o, CM_SKY);
            } else {
                if (o->skyversion == 1) {
                    if (org->canmove) {
                        org_paralyzed(org, o);
                    }
                } else {
                    if (!o->isenhanced) {
                        org->seg_color[oseg] = CM_DARKJADE;
                        org_set_color(o, CM_SKY);
                        org->useextraeffects = true;
                    } else {
                        org_frozen(org, o);
                    }
                }
            }
            break;
        case TY_DARKJADE:
            if ((org->dodge) && (org->frames_color == 0) && (org_use_energy(org, P.dodge_energy_consumption))) {
                org_set_color(org, CM_TEAL);
                org_set_color(o, CM_SKY);
            } else {
                if (o->skyversion == 1) {
                    if (org->canmove) {
                        org_paralyzed(org, o);
                    }
                } else {
                    if (!o->isenhanced) {
                        if ((org->healing == 0) || (!org->active)) {
                            org->seg_color[oseg] = CM_DARKGREEN;
                            org_set_color(o, CM_SKY);
                            org->useextraeffects = true;
                        }
                    } else {
                        org_frozen(org, o);
                    }
                }
            }
            break;
        case TY_MINT:
        case TY_LAVENDER:
        case TY_ROSE:
        case TY_MAGENTA:
            if (o->altruist) {
                break;
            } else {
                if ((org->dodge) && (org->frames_color == 0) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, CM_SKY);
                } else {
                    if (o->skyversion == 1) {
                        if (org->canmove) {
                            if ((org->skyversion != 1) || (o->isaplant) || (org->isaplant) || (o->isaconsumer) || (org->isaconsumer) || (o->isafungus) || (org->isafungus)) {
                                if (org->healing == 0) {
                                    org_paralyzed(org, o);
                                } else {
                                    if ((!o->isaplant) || (o->isenhanced)) {
                                        if ((org->seg_color[oseg] == JC_MAGENTA) && (org_use_energy(org, P.magenta_energy_consumption))) {
                                            org_set_color(org, JC_MAGENTA);
                                            org_set_color(o, CM_SKY);
                                        } else {
                                            org_paralyzed(org, o);
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if (!org->allfrozen) {
                            if ((org->seg_color[oseg] == JC_MAGENTA) && (org_use_energy(org, P.magenta_energy_consumption))) {
                                org_set_color(org, JC_MAGENTA);
                                org_set_color(o, CM_SKY);
                            } else {
                                org_frozen(org, o);
                            }
                        }
                    }
                }
            }
            break;
        case TY_MAROON:
            if (o->skyversion == 1) {
                if (org->canmove) {
                    if (org->healing == 0) {
                        org_paralyzed(org, o);
                    } else {
                        if ((!o->isaplant) || (o->isenhanced)) {
                            if ((!o->isenhanced) && (org->frames_color == 0) && ((o->isaconsumer) || (o->isafungus)) && (org_use_energy(org, P.magenta_energy_consumption))) {
                                org_set_color(org, JC_MAGENTA);
                                org_set_color(o, CM_SKY);
                            } else {
                                org_paralyzed(org, o);
                            }
                        }
                    }
                }
            } else {
                if (!org->allfrozen) {
                    org_frozen(org, o);
                }
            }
            break;
        case TY_OLDBARK:
        case TY_VIOLET:
        case TY_GRAY:
        case TY_LIGHTBROWN:
        case TY_GREENBROWN:
        case TY_BROKEN:
        case TY_ICE:
        case TY_DEADBARK:
            if (o->skyversion == 1) {
                if (org->canmove) {
                    if ((org->skyversion != 1) || (o->isaplant) || (org->isaplant) || (o->isaconsumer) || (org->isaconsumer) || (o->isafungus) || (org->isafungus)) {
                        org_paralyzed(org, o);
                    }
                }
            } else {
                if (!org->allfrozen) {
                    org_frozen(org, o);
                }
            }
            break;
        case TY_CORAL:
            if (o->skyversion == 1) {
                if (org->canmove) {
                    if ((org->skyversion != 1) || (o->isaplant) || (org->isaplant) || (o->isaconsumer) || (org->isaconsumer) || (o->isafungus) || (org->isafungus)) {
                        if ((org->healing == 0) || (!o->isaplant) || (o->isenhanced)) {
                            org_paralyzed(org, o);
                        }
                    }
                }
            }
            break;
        case TY_CREAM:
        case TY_FRUIT:
        case TY_DARKFIRE:
            if (org->active) {
                if (o->skyversion == 1) {
                    if (org->canmove) {
                        if ((org->skyversion != 1) || (o->isaplant) || (org->isaplant) || (o->isaconsumer) || (org->isaconsumer) || (o->isafungus) || (org->isafungus)) {
                            if ((org->healing == 0) || (!o->isaplant) || (o->isenhanced) || (org->seg_color[oseg] == CM_DARKFIRE)) {
                                org_paralyzed(org, o);
                            }
                        }
                    }
                } else {
                    if (!org->allfrozen) {
                        org_frozen(org, o);
                    }
                }
            } else {
                if (!org->isfrozen) {
                    if (org->sporeversion == 1) {
                        if (org_use_energy(o, P.sky_energy_consumption)) {
                            for (int a = 0; a < org->segments; a++) {
                                switch (org_get_type_color(org->seg_color[a])) {
                                case TY_FRUIT:
                                case TY_DARKFIRE:
                                    org->seg_color[a] = CM_ICE;
                                    break;
                                }
                            }
                            org_set_color(o, CM_SKY);
                            org->frames_color = 0;
                            org->isinjured = true;
                            org->isfrozen = true;
                            org->allfrozen = true;
                        }
                    } else {
                        if ((o->skyversion == 1) && (org->sporeversion == 5)) {
                            if (org_use_energy(o, P.sky_energy_consumption)) {
                                org_set_color(o, CM_SKY);
                                org->isfrozen = true;
                            }
                        }
                    }
                }
            }
            break;
        case TY_BARK:
            if (o->skyversion == 1) {
                if (org->canmove) {
                    org_paralyzed(org, o);
                }
                org->seg_color[oseg] = CM_OLDBARK;
                if (org->active) {
                    org->photosynthesis -= org->mphoto[oseg] * k_scale[org->growth_ratio - 1];
                }
            } else {
                if (!org->allfrozen) {
                    org_frozen(org, o);
                }
            }
            break;
        case TY_FALLOW:
            if (o->skyversion == 1) {
                if ((org->canmove) && ((org->isaplant) || (org->isinfectious))) {
                    org_paralyzed(org, o);
                }
            }
            break;
        case TY_SPIKE:
            if ((org->isaplant) || (org->isenhanced)) {
                if (o->skyversion == 1) {
                    if (org->canmove) {
                        if ((org->skyversion != 1) || (o->isaplant) || (org->isaplant) || (o->isaconsumer) || (org->isaconsumer) || (o->isafungus) || (org->isafungus)) {
                            org_paralyzed(org, o);
                        }
                    }
                } else {
                    if (!org->allfrozen) {
                        org_frozen(org, o);
                    }
                }
            }
            break;
        case TY_OLIVE:
        case TY_SPIKEPOINT:
        case TY_BROWN:
            break;
        default:
            if (o->skyversion == 1) {
                if (org->canmove) {
                    if ((org->skyversion != 1) || (o->isaplant) || (org->isaplant) || (o->isaconsumer) || (org->isaconsumer) || (o->isafungus) || (org->isafungus)) {
                        if ((org->healing == 0) || (!o->isaplant) || (o->isenhanced)) {
                            org_paralyzed(org, o);
                        }
                    }
                }
            } else {
                if (!org->allfrozen) {
                    org_frozen(org, o);
                }
            }
        }
        break;
    case TY_OLIVE:
        // Olive segment: Crack defense
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_OLIVE:
            if (org->isaplant) {
                if ((!o->isaplant) || ((o->isenhanced) && (org->isinfectious) && (!o->isinfectious))) {
                    if (org_use_energy(o, P.olive_energy_consumption)) {
                        org->seg_color[oseg] = CM_LIGHT_BLUE;
                        org_set_color(o, CM_OLIVE);
                        org->isinjured = true;
                    }
                } else {
                    if ((org->isinfectious) || (!o->isinfectious)) {
                        if (org_use_energy(o, P.olive_energy_consumption)) {
                            if (org_use_energy(org, P.olive_energy_consumption)) {
                                org->seg_color[oseg] = CM_DARKOLIVE;
                                org_set_color(o, CM_OLIVE);
                                org->useextraeffects = true;
                            } else {
                                org->seg_color[oseg] = CM_LIGHT_BLUE;
                                org_set_color(o, CM_OLIVE);
                                org->isinjured = true;
                            }
                        }
                    }
                }
            }
            break;
        case TY_DARKOLIVE:
            if (org->isaplant) {
                if ((!o->isaplant) || ((o->isenhanced) && (org->isinfectious) && (!o->isinfectious))) {
                    if (org_use_energy(o, P.olive_energy_consumption)) {
                        org->seg_color[oseg] = CM_LIGHT_BLUE;
                        org_set_color(o, CM_OLIVE);
                        org->isinjured = true;
                    }
                }
            }
            break;
        case TY_BLUE:
            if (org->active) {
                if ((!o->isenhanced) && ((!org->isaplant) || ((o->isinfectious) && (o->isaplant))) && (org_use_energy(org, P.blue_energy_consumption))) {
                    org_set_color(o, CM_OLIVE);
                    org_set_color(org, JC_BLUE);
                } else {
                    if (org_use_energy(o, P.olive_energy_consumption)) {
                        org->seg_color[oseg] = CM_LIGHT_BLUE;
                        org_set_color(o, CM_OLIVE);
                        if (!org->isaplant) {
                            org->update_effects = 2;
                        }
                        org->isinjured = true;
                    }
                }
            } else {
                if (gc_get_modifiesspore(org->gc) <= 9) {
                    if (((!o->isaplant) && (!o->isinfectious)) || (o->iscoral) || (o->isenhanced)) {
                        if (org_use_energy(o, P.olive_energy_consumption)) {
                            for (int a = 0; a < org->segments; a++) {
                                switch (org_get_type_color(org->seg_color[a])) {
                                case TY_BLUE:
                                    org->seg_color[a] = CM_LIGHT_BLUE;
                                    break;
                                }
                            }
                            org_set_color(o, CM_OLIVE);
                            org->frames_color = 0;
                            org->isinjured = true;
                            org->isfrozen = true;
                            org->allfrozen = true;
                        }
                    }
                }
            }
            break;
        case TY_OCHRE:
            if ((o->isenhanced) || (org->isaplant)) {
                if (org_use_energy(o, P.olive_energy_consumption)) {
                    org->seg_color[oseg] = CM_LIGHT_BLUE;
                    org_set_color(o, CM_OLIVE);
                    org->isinjured = true;
                }
            }
            break;
        case TY_SKY:
        case TY_DEEPSKY:
            if (org_use_energy(o, P.olive_energy_consumption)) {
                org->seg_color[oseg] = CM_LIGHT_BLUE;
                org_set_color(o, CM_OLIVE);
                org_segments_restore_effects(org);
                if ((!org->isaplant) && (!gc_get_modifiessky(org->gc))) {
                    org->update_effects = 2;
                }
            }
            break;
        case TY_LILAC:
        case TY_DARKLILAC:
        case TY_SPIKE:
        case TY_SPIKEPOINT:
            if (org_use_energy(o, P.olive_energy_consumption)) {
                if (org->isaplant) {
                    org->seg_color[oseg] = CM_BROKEN;
                } else {
                    org->seg_color[oseg] = CM_LIGHTBROWN;
                }
                org_set_color(o, CM_OLIVE);
                org->isinjured = true;
            }
            break;
        case TY_FALLOW:
            if ((((!o->isaconsumer) && (!o->isafungus) && ((!o->isinfectious) || (!o->isaplant))) || (o->isenhanced)) && (org->isaplant)) {
                if (org_use_energy(o, P.olive_energy_consumption)) {
                    if (org->isaplant) {
                        org->seg_color[oseg] = CM_BROKEN;
                    } else {
                        org->seg_color[oseg] = CM_LIGHTBROWN;
                    }
                    org_set_color(o, CM_OLIVE);
                    org->update_effects = 2;
                    org->isinjured = true;
                }
            }
            break;
        case TY_BARK:
            if ((((!o->isaplant) && (!o->isaconsumer)) || (org->isaconsumer) || (o->isenhanced) || (org->isenhanced)) && (org->active)) {
                if (org_use_energy(o, P.olive_energy_consumption)) {
                    org->seg_color[oseg] = CM_DEADBARK;
                    org->photosynthesis -= org->mphoto[oseg] * k_scale[org->growth_ratio - 1];
                    org_set_color(o, CM_OLIVE);
                    org->isfrozen = true;
                }
            } else {
                org->seg_color[oseg] = CM_OLDBARK;
                if (org->active) {
                    org->photosynthesis -= org->mphoto[oseg] * k_scale[org->growth_ratio - 1];
                }
            }
            break;
        case TY_OLDBARK:
            if ((((!o->isaplant) && (!o->isaconsumer)) || (org->isaconsumer) || (o->isenhanced) || (org->isenhanced)) && (org->active)) {
                if (org_use_energy(o, P.olive_energy_consumption)) {
                    org->seg_color[oseg] = CM_DEADBARK;
                    org_set_color(o, CM_OLIVE);
                    org->isfrozen = true;
                }
            }
            break;
        default:
            break;
        }
        break;
    case TY_GRAY:
        // Gray segment: Kill an organism
        if (org->hasdodged == false) {
            if ((!org->isonlyc4) || (!org->isenhanced) || (org->frames_color > 8)) {
                org->hasdodged = true;
            }
        }
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_BROWN:
        case TY_SPIKEPOINT:
            break;
        case TY_FALLOW:
            if ((o->isenhanced) && ((org->isaplant) || (org->isinfectious))) {
                if ((org->healing > 0) && ((!org->isgray) || ((!org->isaconsumer) && (!org->isafungus)))) {
                    if (org_use_energy(org, P.magenta_energy_consumption)) {
                        org_set_color(org, JC_MAGENTA);
                        org_set_color(o, JC_GRAY);
                    } else {
                        if (org_use_energy(o, P.gray_energy_consumption)) {
                            org_survive(org, o);
                        }
                    }
                } else {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_die(org, o);
                        org_set_color(o, JC_GRAY);
                    }
                }
            }
            break;
        case TY_PINK:
        case TY_OLDBARK:
            if ((o->isenhanced) && (!o->isaconsumer) && (!o->isafungus)) {
                if ((org->healing > 0) && ((!org->isgray) || ((!org->isaconsumer) && (!org->isafungus))) && (org->active)) {
                    if (org_use_energy(org, P.magenta_energy_consumption)) {
                        org_set_color(org, JC_MAGENTA);
                        org_set_color(o, JC_GRAY);
                    } else {
                        if (org_use_energy(o, P.gray_energy_consumption)) {
                            org_survive(org, o);
                        }
                    }
                } else {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_die(org, o);
                        org_set_color(o, JC_GRAY);
                    }
                }
            }
            break;
        case TY_CORAL:
            if (((!o->isaplant) || (o->isenhanced)) && (!o->isaconsumer) && (!o->isafungus)) {
                if ((org->healing > 0) && ((!org->isgray) || ((!org->isaconsumer) && (!org->isafungus)))) {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_survive(org, o);
                    }
                } else {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_die(org, o);
                        org_set_color(o, JC_GRAY);
                    }
                }
            }
            break;
        case TY_MAROON:
            if ((org->healing > 0) && (org->frames_color == 0) && ((o->isaconsumer) || (o->isafungus)) && (org_use_energy(org, P.magenta_energy_consumption))) {
                org_set_color(org, JC_MAGENTA);
                org_set_color(o, JC_GRAY);
            } else {
                if ((org->healing > 0) && (!org->isgray)) {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_survive(org, o);
                    }
                } else {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_die(org, o);
                        org_set_color(o, JC_GRAY);
                    }
                }
            }
            break;
        case TY_GREEN:
        case TY_GRASS:
        case TY_FOREST:
        case TY_SPRING:
        case TY_SUMMER:
        case TY_WINTER:
        case TY_LIME:
        case TY_LEAF:
        case TY_C4:
        case TY_DARKGREEN:
        case TY_PURPLE:
        case TY_TEAL:
        case TY_CYAN:
        case TY_YELLOW:
        case TY_AUBURN:
        case TY_INDIGO:
        case TY_BLOND:
        case TY_FLOWER:
        case TY_DARKGRAY:
        case TY_GOLD:
        case TY_SPORE:
            if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                org_set_color(org, CM_TEAL);
                org_set_color(o, JC_GRAY);
            } else {
                if ((org->healing > 0) && ((!org->isgray) || ((!org->isaconsumer) && (!org->isafungus))) && (org->active)) {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_survive(org, o);
                    }
                } else {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_die(org, o);
                        org_set_color(o, JC_GRAY);
                    }
                }
            }
            break;
        case TY_JADE:
            if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                org_set_color(org, CM_TEAL);
                org_set_color(o, JC_GRAY);
            } else {
                if ((org->healing > 0) && (!o->isenhanced) && (org->active)) {
                    org->seg_color[oseg] = CM_DARKJADE;
                    org_set_color(o, JC_GRAY);
                    org->useextraeffects = true;
                } else {
                    if ((org->healing > 0) && ((!org->isgray) || ((!org->isaconsumer) && (!org->isafungus))) && (org->active)) {
                        if (org_use_energy(o, P.gray_energy_consumption)) {
                            org_survive(org, o);
                        }
                    } else {
                        if (org_use_energy(o, P.gray_energy_consumption)) {
                            org_die(org, o);
                            org_set_color(o, JC_GRAY);
                        }
                    }
                }
            }
            break;
        case TY_DARKJADE:
            if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                org_set_color(org, CM_TEAL);
                org_set_color(o, JC_GRAY);
            } else {
                if ((org->healing > 0) && (!o->isenhanced) && (org->active)) {
                    org->seg_color[oseg] = CM_DARKGREEN;
                    org_set_color(o, JC_GRAY);
                    org->useextraeffects = true;
                } else {
                    if ((org->healing > 0) && ((!org->isgray) || ((!org->isaconsumer) && (!org->isafungus))) && (org->active)) {
                        if (org_use_energy(o, P.gray_energy_consumption)) {
                            org_survive(org, o);
                        }
                    } else {
                        if (org_use_energy(o, P.gray_energy_consumption)) {
                            org_die(org, o);
                            org_set_color(o, JC_GRAY);
                        }
                    }
                }
            }
            break;
        case TY_WHITE:
            if ((org->isaplant) || (org->isaconsumer) || (org->isafungus) || (org->plagueversion > 0) || (o->isenhanced)) {
                if ((org->healing > 0) && ((!org->isgray) || ((!org->isaconsumer) && (!org->isafungus)))) {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_survive(org, o);
                    }
                } else {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_die(org, o);
                        org_set_color(o, JC_GRAY);
                    }
                }
            } else {
                if ((org->healing > 0) && ((!org->isgray) || ((!org->isaconsumer) && (!org->isafungus)))) {
                    if (org_use_energy(org, P.magenta_energy_consumption)) {
                        org_set_color(org, JC_MAGENTA);
                        org_set_color(o, JC_GRAY);
                    } else {
                        if (org_use_energy(o, P.gray_energy_consumption)) {
                            org_survive(org, o);
                        }
                    }
                } else {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_die(org, o);
                        org_set_color(o, JC_GRAY);
                    }
                }
            }
            break;
        case TY_BLUE:
            if (org_use_energy(org, P.blue_energy_consumption)) {
                if (org->isenhanced) {
                    org_use_energy(o, bio_between((0.5 * sqrt(org->m[oseg])) * P.organic_obtained_energy, 0, o->energy));
                    org_set_color(o, CM_DARKLILAC);
                    if (o->energy < BIO_TOL) {
                        org_die(o, org);
                    }
                } else {
                    org_set_color(o, JC_GRAY);
                }
                org_set_color(org, JC_BLUE);
            } else {
                if ((org->healing > 0) && ((!org->isgray) || ((!org->isaconsumer) && (!org->isafungus))) && (org->active)) {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_survive(org, o);
                    }
                } else {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_die(org, o);
                        org_set_color(o, JC_GRAY);
                    }
                }
            }
            break;
        case TY_SKY:
            if ((org->skyversion == 2) || (org->isaplant)) {
                if (org_use_energy(org, P.sky_energy_consumption)) {
                    org->seg_color[oseg] = CM_DEEPSKY;
                    org->useextraeffects = true;
                    org_set_color(org, CM_DEEPSKY);
                    org_set_color(o, JC_GRAY);
                } else {
                    if ((org->healing > 0) && ((!org->isgray) || ((!org->isaconsumer) && (!org->isafungus)))) {
                        if (org_use_energy(o, P.gray_energy_consumption)) {
                            org_survive(org, o);
                        }
                    } else {
                        if (org_use_energy(o, P.gray_energy_consumption)) {
                            org_die(org, o);
                            org_set_color(o, JC_GRAY);
                        }
                    }
                }
            }
            break;
        case TY_DEEPSKY:
            if ((org->skyversion == 2) || (org->isaplant)) {
                if ((o->isaconsumer) || (o->isafungus)) {
                    org_set_color(org, CM_DEEPSKY);
                    org_set_color(o, JC_GRAY);
                } else {
                    if ((org->healing > 0) && ((!org->isgray) || ((!org->isaconsumer) && (!org->isafungus)))) {
                        if (org_use_energy(o, P.gray_energy_consumption)) {
                            org_survive(org, o);
                        }
                    } else {
                        if (org_use_energy(o, P.gray_energy_consumption)) {
                            org_die(org, o);
                            org_set_color(o, JC_GRAY);
                        }
                    }
                }
            }
            break;
        case TY_OCHRE:
            if (org->isaplant) {
                if ((org->healing > 0) && ((!org->isgray) || ((!org->isaconsumer) && (!org->isafungus))) && (org->active)) {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_survive(org, o);
                    }
                } else {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_die(org, o);
                        org_set_color(o, JC_GRAY);
                    }
                }
            }
            break;
        case TY_MINT:
        case TY_LAVENDER:
        case TY_ROSE:
        case TY_MAGENTA:
            if (o->altruist) {
                break;
            } else {
                if ((org->dodge) && (org_use_energy(org, P.dodge_energy_consumption))) {
                    org_set_color(org, CM_TEAL);
                    org_set_color(o, JC_GRAY);
                } else {
                    if ((org->seg_color[oseg] == JC_MAGENTA) && (!org->isgray) && (org_use_energy(org, P.magenta_energy_consumption))) {
                        org_set_color(org, JC_MAGENTA);
                        org_set_color(o, JC_GRAY);
                    } else {
                        if ((org->healing > 0) && ((!org->isgray) || ((!org->isaconsumer) && (!org->isafungus)))) {
                            if (org_use_energy(o, P.gray_energy_consumption)) {
                                org_survive(org, o);
                            }
                        } else {
                            if (org_use_energy(o, P.gray_energy_consumption)) {
                                org_die(org, o);
                                org_set_color(o, JC_GRAY);
                            }
                        }
                    }
                }
            }
            break;
        case TY_RED:
            if (org->isenhanced) {
                break;
            } else {
                if ((org->healing > 0) && (!org->isgray)) {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_survive(org, o);
                    }
                } else {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_die(org, o);
                        org_set_color(o, JC_GRAY);
                    }
                }
            }
            break;
        case TY_SILVER:
            if ((o->total_kills >= org->total_kills) || ((!org->isaconsumer) && (!org->isafungus))) {
                if ((org->healing > 0) && ((!org->isgray) || ((!org->isaconsumer) && (!org->isafungus)))) {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_survive(org, o);
                    }
                } else {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_die(org, o);
                        org_set_color(o, JC_GRAY);
                    }
                }
            }
            break;
        case TY_SPIKE:
            if ((org->isaplant) || (org->isenhanced)) {
                if ((org->healing > 0) && ((!org->isgray) || ((!org->isaconsumer) && (!org->isafungus)))) {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_survive(org, o);
                    }
                } else {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_die(org, o);
                        org_set_color(o, JC_GRAY);
                    }
                }
            }
            break;
        case TY_OLIVE:
            if (org_use_energy(org, P.olive_energy_consumption)) {
                if (o->isaplant) {
                    o->seg_color[seg] = CM_BROKEN;
                } else {
                    o->seg_color[seg] = CM_LIGHTBROWN;
                }
                o->isinjured = true;
                if ((org->healing == 0) && (org_use_energy(o, P.gray_energy_consumption))) {
                    org->seg_color[oseg] = CM_DARKOLIVE;
                    org->useextraeffects = true;
                    org_set_color(org, CM_DARKOLIVE);
                    org_set_color(o, JC_GRAY);
                } else {
                    org_set_color(org, CM_OLIVE);
                }
            } else {
                if ((org->healing > 0) && ((!org->isgray) || ((!org->isaconsumer) && (!org->isafungus)))) {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_survive(org, o);
                    }
                } else {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_die(org, o);
                        org_set_color(o, JC_GRAY);
                    }
                }
            }
            break;
        case TY_DARKOLIVE:
            if (org_use_energy(org, P.olive_energy_consumption)) {
                if (o->isaplant) {
                    o->seg_color[seg] = CM_BROKEN;
                } else {
                    o->seg_color[seg] = CM_LIGHTBROWN;
                }
                o->isinjured = true;
                if ((org->healing == 0) && (org_use_energy(o, P.gray_energy_consumption))) {
                    org->seg_color[oseg] = CM_LIGHT_BLUE;
                    org_set_color(org, CM_DARKOLIVE);
                    org_set_color(o, JC_GRAY);
                    if (!org->isaplant) {
                        org->update_effects = 2;
                    }
                    org->isinjured = true;
                } else {
                    org_set_color(org, CM_OLIVE);
                }
            } else {
                if ((org->healing > 0) && ((!org->isgray) || ((!org->isaconsumer) && (!org->isafungus)))) {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_survive(org, o);
                    }
                } else {
                    if (org_use_energy(o, P.gray_energy_consumption)) {
                        org_die(org, o);
                        org_set_color(o, JC_GRAY);
                    }
                }
            }
            break;
        case TY_VIOLET:
            if ((org->healing > 0) && ((!org->isgray) || ((!org->isaconsumer) && (!org->isafungus)))) {
                if (org_use_energy(o, P.gray_energy_consumption)) {
                    org_survive(org, o);
                    org->seg_color[oseg] = CM_DARKFIRE;
                    org->useextraeffects = true;
                    org->isinjured = true;
                }
            } else {
                if (org_use_energy(o, P.gray_energy_consumption)) {
                    org_die(org, o);
                    org_set_color(o, JC_GRAY);
                }
            }
            break;
        case TY_BARK:
            org->seg_color[oseg] = CM_OLDBARK;
            if (org->active) {
                org->photosynthesis -= org->mphoto[oseg] * k_scale[org->growth_ratio - 1];
            }
            break;
        default:
            if ((org->healing > 0) && ((!org->isgray) || ((!org->isaconsumer) && (!org->isafungus))) && (org->active)) {
                if (org_use_energy(o, P.gray_energy_consumption)) {
                    org_survive(org, o);
                }
            } else {
                if (org_use_energy(o, P.gray_energy_consumption)) {
                    org_die(org, o);
                    org_set_color(o, JC_GRAY);
                }
            }
        }
        break;
    // Spore hatching
    case TY_BLUE:
        if (!o->active) {
            org_touch_hatch(o, org, seg, oseg);
        }
        break;
    // Yellow segment: Normally used to have more children, viruses try to let other organisms reproduce many viruses at once.
    case TY_YELLOW:
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_BROWN:
            break;
        case TY_BLUE:
            if ((o->isinfectious) && (!o->isaconsumer) && (!o->isaplant) && (!o->isafungus)) {
                if ((org->isaplant) || (org->isaconsumer) || (org->isafungus)) {
                    if (org_use_energy(org, P.blue_energy_consumption)) {
                        if (org->isenhanced) {
                            org_use_energy(o, bio_between((0.5 * sqrt(org->m[oseg])) * P.organic_obtained_energy, 0, o->energy));
                            org_set_color(o, CM_DARKLILAC);
                            if (o->energy < BIO_TOL) {
                                org_die(o, org);
                            }
                        } else {
                            org_set_color(o, CM_FALLOW);
                        }
                        org_set_color(org, JC_BLUE);
                    } else {
                        if (org->time_to_reproduce < org->time_to_reproduce_max) {
                            org->time_to_reproduce = org->time_to_reproduce_max;
                            if (o->time_to_reproduce == 0) {
                                org_set_color(o, CM_FALLOW);
                                org_set_color(org, CM_FLOWER);
                            }
                        }
                        if (org->infected_gc == o->gc) {
                            if ((org->energy > org_get_reproduce_energy(org)) || ((!org->active) && (org->energy > 0.5 * org_get_reproduce_energy(org)))) {
                                if (o->energy > P.virus_energy_consumption) {
                                    org_set_color(o, CM_PLAGUE);
                                    org->n_virus_children = 1 + o->yellow_counter;
                                    org_reproduce_yellow(org);
                                    if (org->n_virus_children > 0) {
                                        if (org->antiviral > 0) {
                                            org->infected_gc = NULL;
                                            org->saved_gc = NULL;
                                        }
                                        org_set_color(org, CM_DARKLILAC);
                                        org_use_energy(o, P.virus_energy_consumption);
                                        org->n_virus_children = 0;
                                        if (o->time_to_reproduce < 10) {
                                            o->time_to_reproduce = 10;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            break;
        case TY_MINT:
            if ((o->isinfectious) && (!o->isaconsumer) && (!o->isaplant) && (!o->isafungus)) {
                if (org_use_energy(org, P.mint_energy_consumption / 10)) {
                    o->seg_color[seg] = CM_LIGHTBROWN;
                    org_set_color(org, CM_MINT);
                    org_segments_restore_effects(o);
                }
            }
            break;
        default:
            if ((o->isinfectious) && (!o->isaconsumer) && (!o->isaplant) && (!o->isafungus)) {
                if ((org->isaplant) || (org->isaconsumer) || (org->isafungus)) {
                    if (org->time_to_reproduce < org->time_to_reproduce_max) {
                        org->time_to_reproduce = org->time_to_reproduce_max;
                        if (o->time_to_reproduce == 0) {
                            org_set_color(o, CM_FALLOW);
                            org_set_color(org, CM_FLOWER);
                        }
                    }
                    if (org->infected_gc == o->gc) {
                        if ((org->energy > org_get_reproduce_energy(org)) || ((!org->active) && (org->energy > 0.5 * org_get_reproduce_energy(org)))) {
                            if (o->energy > P.virus_energy_consumption) {
                                org_set_color(o, CM_PLAGUE);
                                org->n_virus_children = 1 + o->yellow_counter;
                                org_reproduce_yellow(org);
                                if (org->n_virus_children > 0) {
                                    if (org->antiviral > 0) {
                                        org->infected_gc = NULL;
                                        org->saved_gc = NULL;
                                    }
                                    org_set_color(org, CM_DARKLILAC);
                                    org_use_energy(o, P.virus_energy_consumption);
                                    org->n_virus_children = 0;
                                    if (o->time_to_reproduce < 10) {
                                        o->time_to_reproduce = 10;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        break;
    case TY_ROSE:
        // Rose segment: Transfers energy
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_BROWN:
            break;
        default:
            if (o->altruist && org->altruist) {
                if ((o->growth_ratio == 1) && (org->active)) {
                    if ((o->energy > (org->energy + 2)) && (o->energy > (org_get_reproduce_energy(o) / 2)) && (org->energy < (org_get_reproduce_energy(org) / 2))) {
                        // Transfers energy
                        double takenEnergyRose = bio_between(0.5 * P.organic_obtained_energy, 0, (o->energy - (org->energy + 2)));
                        // The other organism will be shown in cyan
                        org_set_color(org, JC_CYAN);
                        // This organism will be shown in rose
                        org_set_color(o, CM_ROSE);
                        org->energy += takenEnergyRose;
                        o->energy -= takenEnergyRose;
                    }
                }
            }
        }
        break;
    case TY_MINT:
        // Mint segment: Remove an infection, corrupt all white and cream segments.
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_BROWN:
            if (org->isinfectious) {
                if (o->infected_gc == org->gc) {
                    if (org_use_energy(o, P.mint_energy_consumption)) {
                        org_set_color(o, CM_MINT);
                        o->infected_gc = NULL;
                        o->saved_gc = NULL;
                    }
                }
            }
            break;
        case TY_BLUE:
            if (((org->isinfectious) || (org->iscoral) || (org->plagueversion > 0) || ((org->fallowversion > 0) && ((org->isaplant) || (org->fallowversion == 4) || (o->isenhanced)))) && (org->active)) {
                if (((!org->isaplant) || (((o->isaconsumer) || (o->isafungus)) && (!o->isenhanced))) && (org_use_energy(org, P.blue_energy_consumption))) {
                    org_set_color(o, CM_MINT);
                    org_set_color(org, JC_BLUE);
                } else {
                    org_neutralized(org, o);
                    if (o->infected_gc == org->gc) {
                        if (org_use_energy(o, P.mint_energy_consumption)) {
                            org_set_color(o, CM_MINT);
                            o->infected_gc = NULL;
                            o->saved_gc = NULL;
                        }
                    }
                }
            }
            if (o->altruist && org->altruist) {
                if ((org->infected_gc != o->gc) && (org->infected_gc != NULL)) {
                    if ((org_use_energy(org, P.mint_energy_consumption / 2)) && (org_use_energy(o, P.mint_energy_consumption / 2))) {
                        org->infected_gc = NULL;
                        org->saved_gc = NULL;
                        org_set_color(org, JC_CYAN);
                        org_set_color(o, CM_MINT);
                    }
                }
                if (org->reproducelate < 0) {
                    if ((org_use_energy(org, P.mint_energy_consumption / 2)) && (org_use_energy(o, P.mint_energy_consumption / 2))) {
                        org->reproducelate = 0;
                        org_set_color(org, JC_CYAN);
                        org_set_color(o, CM_MINT);
                    }
                }
            }
            break;
        case TY_MINT:
            if (org->isinfectious) {
                if (o->infected_gc == org->gc) {
                    if (org_use_energy(o, P.mint_energy_consumption)) {
                        org_set_color(o, CM_MINT);
                        o->infected_gc = NULL;
                        o->saved_gc = NULL;
                    }
                }
            }
            if (o->altruist && org->altruist) {
                if ((org->infected_gc != o->gc) && (org->infected_gc != NULL)) {
                    if ((org_use_energy(org, P.mint_energy_consumption / 2)) && (org_use_energy(o, P.mint_energy_consumption / 2))) {
                        org->infected_gc = NULL;
                        org->saved_gc = NULL;
                        org_set_color(org, JC_CYAN);
                        org_set_color(o, CM_MINT);
                    }
                }
                if (org->reproducelate < 0) {
                    if ((org_use_energy(org, P.mint_energy_consumption / 2)) && (org_use_energy(o, P.mint_energy_consumption / 2))) {
                        org->reproducelate = 0;
                        org_set_color(org, JC_CYAN);
                        org_set_color(o, CM_MINT);
                    }
                }
            }
            break;
        default:
            if (org->active) {
                if ((org->isinfectious) || (org->iscoral) || (org->plagueversion > 0) || ((org->fallowversion > 0) && ((org->isaplant) || (org->fallowversion == 4) || (o->isenhanced)))) {
                    org_neutralized(org, o);
                    if (o->infected_gc == org->gc) {
                        if (org_use_energy(o, P.mint_energy_consumption)) {
                            org_set_color(o, CM_MINT);
                            o->infected_gc = NULL;
                            o->saved_gc = NULL;
                        }
                    }
                }
            } else {
                if ((org->sporeversion == 1) || (org->sporeversion == 2)) {
                    if (!org->allfrozen) {
                        if (org_use_energy(o, P.mint_energy_consumption / 10)) {
                            for (int a = 0; a < org->segments; a++) {
                                switch (org_get_type_color(org->seg_color[a])) {
                                case TY_FRUIT:
                                case TY_DARKFIRE:
                                    org->seg_color[a] = CM_BROKEN;
                                    break;
                                }
                            }
                            org_set_color(o, CM_MINT);
                            org->frames_color = 0;
                            org->isinjured = true;
                            org->isfrozen = true;
                            org->allfrozen = true;
                        }
                    }
                    if (o->infected_gc == org->gc) {
                        if (org_use_energy(o, P.mint_energy_consumption)) {
                            org_set_color(o, CM_MINT);
                            o->infected_gc = NULL;
                            o->saved_gc = NULL;
                        }
                    }
                } else {
                    if (org->sporeversion == 5) {
                        if (!org->allfrozen) {
                            if (org_use_energy(o, P.mint_energy_consumption / 10)) {
                                for (int a = 0; a < org->segments; a++) {
                                    switch (org_get_type_color(org->seg_color[a])) {
                                    case TY_CREAM:
                                    case TY_DARKFIRE:
                                        org->seg_color[a] = CM_LIGHTBROWN;
                                        break;
                                    }
                                }
                                org_set_color(o, CM_MINT);
                                org->frames_color = 0;
                                org->isinjured = true;
                                org->isfrozen = true;
                                org->allfrozen = true;
                                org->isaconsumer = false;
                            }
                        }
                    }
                }
            }
            if (o->altruist && org->altruist) {
                if ((org->infected_gc != o->gc) && (org->infected_gc != NULL)) {
                    if ((org_use_energy(org, P.mint_energy_consumption / 2)) && (org_use_energy(o, P.mint_energy_consumption / 2))) {
                        org->infected_gc = NULL;
                        org->saved_gc = NULL;
                        org_set_color(org, JC_CYAN);
                        org_set_color(o, CM_MINT);
                    }
                }
                if (org->reproducelate < 0) {
                    if ((org_use_energy(org, P.mint_energy_consumption / 2)) && (org_use_energy(o, P.mint_energy_consumption / 2))) {
                        org->reproducelate = 0;
                        org_set_color(org, JC_CYAN);
                        org_set_color(o, CM_MINT);
                    }
                }
            }
        }
        break;
    case TY_LAVENDER:
        // Lavender segment: Immunity vs cream, plague, coral and fallow, it can immunize other organisms temporarily. 
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_WHITE:
        case TY_PLAGUE:
        case TY_CORAL:
        case TY_CREAM:
        case TY_FALLOW:
        case TY_FRUIT:
        case TY_BROWN:
            break;
        default:
            if (o->altruist && org->altruist) {
                if (o->lavender >= P.lavender_shield) {
                    if (org->lavender < P.lavender_shield) {
                        org->lavender += o->createlavender;
                        if (org->lavender >= P.lavender_shield) {
                            org->lavender = P.lavender_shield;
                        }
                        org_set_color(org, JC_CYAN);
                        org_set_color(o, CM_LAVENDER);
                    }
                }
                if (o->time_to_reproduce <= o->time_to_reproduce_max) {
                    if (org->time_to_reproduce > org->time_to_reproduce_max) {
                        org->time_to_reproduce -= (o->createlavender / 20);
                        if (org->time_to_reproduce < org->time_to_reproduce_max) {
                            org->time_to_reproduce = org->time_to_reproduce_max;
                        }
                        org_set_color(org, JC_CYAN);
                        org_set_color(o, CM_LAVENDER);
                    }
                }
            }
        }
        break;
    case TY_MAGENTA:
        // Magenta segment: Heal all sick segments
        switch (org_get_type_color(org->seg_color[oseg])) {
        case TY_BROWN:
            break;
        default:
            if (org->isinjured && org->active && o->altruist && org->altruist) {
                bool heal = false;
                for (int j = 0; j < org->segments; j++) {
                    if ((org->seg_color[j] == CM_LIGHTBROWN) || (org->seg_color[j] == CM_GREENBROWN) || (org->seg_color[j] == CM_BROKEN) || (org->seg_color[j] == CM_LIGHT_BLUE) || (org->seg_color[j] == CM_ICE) || (org->seg_color[j] == CM_DARKFIRE)) {
                        if ((org_use_energy(org, P.magenta_energy_consumption / 2)) && (org_use_energy(o, P.magenta_energy_consumption / 2))) {
                            org->seg_color[j] = gene_get_color(gc_get_gene(org->gc, j % gc_get_n_genes(org->gc)));
                            org_set_color(o, JC_MAGENTA);
                            heal = true;
                        }
                    }
                }
                if (heal) {
                    org->isfrozen = false;
                    org->isinjured = false;
                    org->allfrozen = false;
                    org->update_effects = 2;
                    org_segments_restore_effects(org);
                }
            }
        }
        break;
    }
    // Check if the other organism has died
    if (org_is_alive(org) && org->energy < BIO_TOL) {
        org_die(org, o);
    }
}

static void org_rubbing_frames_effects(Organism *o)
{
    o->dx *= P.rubbing;
    if (fabs(o->dx) < BIO_TOL)
        o->dx = 0;
    o->dy *= P.rubbing;
    if (fabs(o->dy) < BIO_TOL)
        o->dy = 0;
    o->dtheta *= P.rubbing;
    if (fabs(o->dtheta) < BIO_TOL)
        o->dtheta = 0;
}

static void org_segments_frame_effects(Organism *o)
{
    int i = 0;
    for (i = o->segments - 1; i >= 0; i--) {
        // Movement
        if (o->mphoto[i] <= -3 && jr_next_int(&g_rng, 100) < 8) {
            if (o->mphoto[i] == -4) {
                if (org_use_energy(o, P.teal_energy_consumption)) {
                    o->dx = bio_between(o->dx + 11.88 * (o->x2[i] - o->x1[i]) / o->mass, -P.max_vel, P.max_vel);
                    o->dy = bio_between(o->dy + 11.88 * (o->y2[i] - o->y1[i]) / o->mass, -P.max_vel, P.max_vel);
                    o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                }
            } else {
                if (org_use_energy(o, P.cyan_energy_consumption)) {
                    o->dx = bio_between(o->dx + 12.0 * (o->x2[i] - o->x1[i]) / o->mass, -P.max_vel, P.max_vel);
                    o->dy = bio_between(o->dy + 12.0 * (o->y2[i] - o->y1[i]) / o->mass, -P.max_vel, P.max_vel);
                    o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                }
            }
        }
    }
}

static void org_leaf_extra_effects(Organism *o)
{
    // Get chemotrophic energy from methane
    if (o->methanotrophy > 0) {
        if ((o->age & 0x07) == 0x00) {
            o->energy += world_methanotrophy(o->world, o->methanotrophy);
        }
    }
    //Get sun's energy
    if ((o->frames_color == 1) && ((o->flash_color == CM_BLOND) || (o->flash_color == CM_LIGHTBROWN) || (o->flash_color == JC_YELLOW) || (o->flash_color == CM_GREENBROWN) || (o->flash_color == CM_DARKLILAC))) {
        org_set_color(o, CM_DARKGREEN);
    }
    if (o->colony_photosynthesis > 0) {
        if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
            if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                o->energy += world_photosynthesis(o->world, o->photosynthesis - o->leafphoto + o->colony_photosynthesis);
            }
        } else {
            o->energy += world_photosynthesis(o->world, o->photosynthesis - o->leafphoto + o->colony_photosynthesis);
        }
        o->colony_photosynthesis = 0;
    } else {
        if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
            if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                o->energy += world_photosynthesis(o->world, o->photosynthesis - o->leafphoto);
            }
        } else {
            o->energy += world_photosynthesis(o->world, o->photosynthesis - o->leafphoto);
        }
    }
}

static void org_jade_extra_effects(Organism *o)
{
    double specialphoto = 0;
    int i = 0;
    for (i = o->segments - 1; i >= 0; i--) {
        // Manteniment
        switch (org_get_type_color(o->seg_color[i])) {
        // Movement
        case TY_TEAL:
            if (o->mphoto[i] == -4) {
                if (jr_next_int(&g_rng, 100) < 8 && org_use_energy(o, P.teal_energy_consumption)) {
                    o->dx = bio_between(o->dx + 11.88 * (o->x2[i] - o->x1[i]) / o->mass, -P.max_vel, P.max_vel);
                    o->dy = bio_between(o->dy + 11.88 * (o->y2[i] - o->y1[i]) / o->mass, -P.max_vel, P.max_vel);
                    o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                }
            }
            break;
        case TY_CYAN:
            if (jr_next_int(&g_rng, 100) < 8 && org_use_energy(o, P.cyan_energy_consumption)) {
                o->dx = bio_between(o->dx + 12.0 * (o->x2[i] - o->x1[i]) / o->mass, -P.max_vel, P.max_vel);
                o->dy = bio_between(o->dy + 12.0 * (o->y2[i] - o->y1[i]) / o->mass, -P.max_vel, P.max_vel);
                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
            }
            break;
        // Restore Jade
        case TY_DARKJADE:
            if (jr_next_int(&g_rng, o->jadefactor) < 8) {
                o->seg_color[i] = CM_JADE;
            } else {
                o->useextraeffects = true;
            }
            break;
        case TY_DARKGREEN:
            if (gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc))) == CM_SUMMER) {
                if ((world_get_time(o->world) % 3) != o->summerinactivity) {
                    specialphoto += o->mphoto[i];
                    if (jr_next_int(&g_rng, o->jadefactor) < 8) {
                        o->seg_color[i] = CM_SUMMER;
                    } else {
                        o->useextraeffects = true;
                    }
                } else {
                    if (jr_next_int(&g_rng, o->jadefactor) < 8) {
                        o->seg_color[i] = CM_WINTER;
                    } else {
                        o->useextraeffects = true;
                    }
                }
            } else {
                if (gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc))) == CM_LEAF) {
                    if (o->frames_color == 0) {
                        specialphoto += o->mphoto[i];
                    }
                } else {
                    if (gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc))) == CM_LIME) {
                        if (o->crowded == true) {
                            specialphoto += P.crowdedlime_energy_consumption * o->mphoto[i];
                        } else {
                            specialphoto += o->mphoto[i];
                        }
                    } else {
                        specialphoto += o->mphoto[i];
                    }
                }
                if (jr_next_int(&g_rng, o->jadefactor) < 8) {
                    o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                } else {
                    o->useextraeffects = true;
                }
            }
            break;
        // Healing
        case TY_GREENBROWN:
        case TY_LIGHTBROWN:
        case TY_BROKEN:
        case TY_LIGHT_BLUE:
        case TY_ICE:
            if (o->healing > 0) {
                if (jr_next_int(&g_rng, P.healing) <= o->healing) {
                    if (org_use_energy(o, P.magenta_energy_consumption)) {
                        o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                        o->isfrozen = false;
                        o->isinjured = false;
                        o->allfrozen = false;
                        o->update_effects = 2;
                        org_segments_restore_effects(o);
                    }
                }
            }
            break;
        case TY_DARKFIRE:
            if (o->healing > 0) {
                if (jr_next_int(&g_rng, P.healing) <= o->healing) {
                    if (org_use_energy(o, P.magenta_energy_consumption)) {
                        o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                        org_darkfire_restore_effects(o);
                    }
                }
            }
            if ((gc_get_symmetry(o->gc) != 3) || (gc_get_mirror(o->gc) == 0)) {
                if (jr_next_int(&g_rng, 100) < gc_get_symmetry(o->gc)) {
                    o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                    org_darkfire_restore_effects(o);
                } else {
                    o->useextraeffects = true;
                }
            } else {
                if (jr_next_int(&g_rng, 100) < 2) {
                    o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                    org_darkfire_restore_effects(o);
                } else {
                    o->useextraeffects = true;
                }
            }
            break;
        // Restore abilities
        case TY_DARKOLIVE:
            if (jr_next_int(&g_rng, 100) < 8) {
                o->seg_color[i] = CM_OLIVE;
            } else {
                o->useextraeffects = true;
            }
            break;
        case TY_DARKLILAC:
            if (jr_next_int(&g_rng, 100) < 8) {
                o->seg_color[i] = CM_LILAC;
            } else {
                o->useextraeffects = true;
            }
            break;
        case TY_DEEPSKY:
            if (jr_next_int(&g_rng, 100) < 8) {
                o->seg_color[i] = CM_SKY;
            } else {
                o->useextraeffects = true;
            }
            break;
        }
    }
    // Get chemotrophic energy from methane
    if (o->methanotrophy > 0) {
        if ((o->age & 0x07) == 0x00) {
            o->energy += world_methanotrophy(o->world, o->methanotrophy);
        }
    }
    //Get sun's energy
    if ((o->leafphoto > 0) && (o->frames_color > 0)) {
        if ((o->frames_color == 1) && ((o->flash_color == CM_BLOND) || (o->flash_color == CM_LIGHTBROWN) || (o->flash_color == JC_YELLOW) || (o->flash_color == CM_GREENBROWN) || (o->flash_color == CM_DARKLILAC))) {
            org_set_color(o, CM_DARKGREEN);
        }
        if (specialphoto > 0) {
            if (o->colony_photosynthesis > 0) {
                if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                    if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                        o->energy += world_photosynthesis(o->world, o->photosynthesis - o->leafphoto - (specialphoto * k_scale[o->growth_ratio - 1]) + o->colony_photosynthesis);
                    }
                } else {
                    o->energy += world_photosynthesis(o->world, o->photosynthesis - o->leafphoto - (specialphoto * k_scale[o->growth_ratio - 1]) + o->colony_photosynthesis);
                }
                o->colony_photosynthesis = 0;
            } else {
                if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                    if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                        o->energy += world_photosynthesis(o->world, o->photosynthesis - o->leafphoto - (specialphoto * k_scale[o->growth_ratio - 1]));
                    }
                } else {
                    o->energy += world_photosynthesis(o->world, o->photosynthesis - o->leafphoto - (specialphoto * k_scale[o->growth_ratio - 1]));
                }
            }
        } else {
            if (o->colony_photosynthesis > 0) {
                if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                    if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                        o->energy += world_photosynthesis(o->world, o->photosynthesis - o->leafphoto + o->colony_photosynthesis);
                    }
                } else {
                    o->energy += world_photosynthesis(o->world, o->photosynthesis - o->leafphoto + o->colony_photosynthesis);
                }
                o->colony_photosynthesis = 0;
            } else {
                if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                    if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                        o->energy += world_photosynthesis(o->world, o->photosynthesis - o->leafphoto);
                    }
                } else {
                    o->energy += world_photosynthesis(o->world, o->photosynthesis - o->leafphoto);
                }
            }
        }
    } else {
        if (specialphoto > 0) {
            if (o->colony_photosynthesis > 0) {
                if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                    if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                        o->energy += world_photosynthesis(o->world, o->photosynthesis - (specialphoto * k_scale[o->growth_ratio - 1]) + o->colony_photosynthesis);
                    }
                } else {
                    o->energy += world_photosynthesis(o->world, o->photosynthesis - (specialphoto * k_scale[o->growth_ratio - 1]) + o->colony_photosynthesis);
                }
                o->colony_photosynthesis = 0;
            } else {
                if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                    if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                        o->energy += world_photosynthesis(o->world, o->photosynthesis - (specialphoto * k_scale[o->growth_ratio - 1]));
                    }
                } else {
                    o->energy += world_photosynthesis(o->world, o->photosynthesis - (specialphoto * k_scale[o->growth_ratio - 1]));
                }
            }
        } else {
            if (o->colony_photosynthesis > 0) {
                if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                    if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                        o->energy += world_photosynthesis(o->world, o->photosynthesis + o->colony_photosynthesis);
                    }
                } else {
                    o->energy += world_photosynthesis(o->world, o->photosynthesis + o->colony_photosynthesis);
                }
                o->colony_photosynthesis = 0;
            } else {
                if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                    if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                        o->energy += world_photosynthesis(o->world, o->photosynthesis);
                    }
                } else {
                    o->energy += world_photosynthesis(o->world, o->photosynthesis);
                }
            }
        }
    }
}

static void org_segments_healing_effects(Organism *o)
{
    int i = 0;
    for (i = o->segments - 1; i >= 0; i--) {
        // Manteniment
        switch (org_get_type_color(o->seg_color[i])) {
        // Movement
        case TY_TEAL:
            if (o->mphoto[i] == -4) {
                if (jr_next_int(&g_rng, 100) < 8 && org_use_energy(o, P.teal_energy_consumption)) {
                    o->dx = bio_between(o->dx + 11.88 * (o->x2[i] - o->x1[i]) / o->mass, -P.max_vel, P.max_vel);
                    o->dy = bio_between(o->dy + 11.88 * (o->y2[i] - o->y1[i]) / o->mass, -P.max_vel, P.max_vel);
                    o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                }
            }
            break;
        case TY_CYAN:
            if (jr_next_int(&g_rng, 100) < 8 && org_use_energy(o, P.cyan_energy_consumption)) {
                o->dx = bio_between(o->dx + 12.0 * (o->x2[i] - o->x1[i]) / o->mass, -P.max_vel, P.max_vel);
                o->dy = bio_between(o->dy + 12.0 * (o->y2[i] - o->y1[i]) / o->mass, -P.max_vel, P.max_vel);
                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
            }
            break;
        // Healing
        case TY_GREENBROWN:
        case TY_LIGHTBROWN:
        case TY_BROKEN:
        case TY_LIGHT_BLUE:
        case TY_ICE:
            if (jr_next_int(&g_rng, P.healing) <= o->healing) {
                if (org_use_energy(o, P.magenta_energy_consumption)) {
                    o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                    o->isfrozen = false;
                    o->isinjured = false;
                    o->allfrozen = false;
                    o->update_effects = 2;
                    org_segments_restore_effects(o);
                }
            }
            break;
        case TY_DARKFIRE:
            if (jr_next_int(&g_rng, P.healing) <= o->healing) {
                if (org_use_energy(o, P.magenta_energy_consumption)) {
                    o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                    org_darkfire_restore_effects(o);
                }
            }
            if ((gc_get_symmetry(o->gc) != 3) || (gc_get_mirror(o->gc) == 0)) {
                if (jr_next_int(&g_rng, 100) < gc_get_symmetry(o->gc)) {
                    o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                    org_darkfire_restore_effects(o);
                } else {
                    o->useextraeffects = true;
                }
            } else {
                if (jr_next_int(&g_rng, 100) < 2) {
                    o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                    org_darkfire_restore_effects(o);
                } else {
                    o->useextraeffects = true;
                }
            }
            break;
        // Restore abilities
        case TY_DARKOLIVE:
            if (jr_next_int(&g_rng, 100) < 8) {
                o->seg_color[i] = CM_OLIVE;
            } else {
                o->useextraeffects = true;
            }
            break;
        case TY_DARKLILAC:
            if (jr_next_int(&g_rng, 100) < 8) {
                o->seg_color[i] = CM_LILAC;
            } else {
                o->useextraeffects = true;
            }
            break;
        case TY_DEEPSKY:
            if (jr_next_int(&g_rng, 100) < 8) {
                o->seg_color[i] = CM_SKY;
            } else {
                o->useextraeffects = true;
            }
            break;
        }
    }
}

static void org_segments_extra_effects(Organism *o)
{
    int i = 0;
    for (i = o->segments - 1; i >= 0; i--) {
        // Manteniment
        switch (org_get_type_color(o->seg_color[i])) {
        // Movement
        case TY_TEAL:
            if (o->mphoto[i] == -4) {
                if (jr_next_int(&g_rng, 100) < 8 && org_use_energy(o, P.teal_energy_consumption)) {
                    o->dx = bio_between(o->dx + 11.88 * (o->x2[i] - o->x1[i]) / o->mass, -P.max_vel, P.max_vel);
                    o->dy = bio_between(o->dy + 11.88 * (o->y2[i] - o->y1[i]) / o->mass, -P.max_vel, P.max_vel);
                    o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                }
            }
            break;
        case TY_CYAN:
            if (jr_next_int(&g_rng, 100) < 8 && org_use_energy(o, P.cyan_energy_consumption)) {
                o->dx = bio_between(o->dx + 12.0 * (o->x2[i] - o->x1[i]) / o->mass, -P.max_vel, P.max_vel);
                o->dy = bio_between(o->dy + 12.0 * (o->y2[i] - o->y1[i]) / o->mass, -P.max_vel, P.max_vel);
                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
            }
            break;
        // Healing
        case TY_DARKFIRE:
            if ((gc_get_symmetry(o->gc) != 3) || (gc_get_mirror(o->gc) == 0)) {
                if (jr_next_int(&g_rng, 100) < gc_get_symmetry(o->gc)) {
                    o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                    org_darkfire_restore_effects(o);
                } else {
                    o->useextraeffects = true;
                }
            } else {
                if (jr_next_int(&g_rng, 100) < 2) {
                    o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                    org_darkfire_restore_effects(o);
                } else {
                    o->useextraeffects = true;
                }
            }
            break;
        // Restore abilities
        case TY_DARKOLIVE:
            if (jr_next_int(&g_rng, 100) < 8) {
                o->seg_color[i] = CM_OLIVE;
            } else {
                o->useextraeffects = true;
            }
            break;
        case TY_DARKLILAC:
            if (jr_next_int(&g_rng, 100) < 8) {
                o->seg_color[i] = CM_LILAC;
            } else {
                o->useextraeffects = true;
            }
            break;
        case TY_DEEPSKY:
            if (jr_next_int(&g_rng, 100) < 8) {
                o->seg_color[i] = CM_SKY;
            } else {
                o->useextraeffects = true;
            }
            break;
        }
    }
}

static void org_segments_extra_plants_effects(Organism *o)
{
    int i = 0;
    for (i = o->segments - 1; i >= 0; i--) {
        if (o->mphoto[i] < 0) {
            // Manteniment
            switch (org_get_type_color(o->seg_color[i])) {
            // Healing
            case TY_DARKFIRE:
                if ((gc_get_symmetry(o->gc) != 3) || (gc_get_mirror(o->gc) == 0)) {
                    if (jr_next_int(&g_rng, 100) < gc_get_symmetry(o->gc)) {
                        o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                        org_darkfire_restore_effects(o);
                    } else {
                        o->useextraeffects = true;
                    }
                } else {
                    if (jr_next_int(&g_rng, 100) < 2) {
                        o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                        org_darkfire_restore_effects(o);
                    } else {
                        o->useextraeffects = true;
                    }
                }
                break;
            // Restore abilities
            case TY_DARKOLIVE:
                if (jr_next_int(&g_rng, 100) < 8) {
                    o->seg_color[i] = CM_OLIVE;
                } else {
                    o->useextraeffects = true;
                }
                break;
            case TY_DARKLILAC:
                if (jr_next_int(&g_rng, 100) < 8) {
                    o->seg_color[i] = CM_LILAC;
                } else {
                    o->useextraeffects = true;
                }
                break;
            case TY_DEEPSKY:
                if (jr_next_int(&g_rng, 100) < 8) {
                    o->seg_color[i] = CM_SKY;
                } else {
                    o->useextraeffects = true;
                }
                break;
            }
        }
    }
}

static void org_plants_update_effects(Organism *o)
{
    int i = 0;
    double addmaintenance = o->mass;
    double addphoto = 0;
    double specialphoto = 0;
    o->forestphoto = 0;
    o->leafphoto = 0;
    o->methanotrophy = 0;
    for (i = o->segments - 1; i >= 0; i--) {
        // Manteniment
        switch (org_get_type_color(o->seg_color[i])) {
        // Photosynthesis
        case TY_LIME:
            if (o->crowded == true) {
                addphoto += P.crowdedlime_energy_consumption * o->mphoto[i];
            } else {
                addphoto += o->mphoto[i];
            }
            break;
        case TY_FOREST:
            addphoto += o->mphoto[i];
            o->forestphoto += o->mphoto[i];
            break;
        case TY_SPRING:
            addphoto += o->mphoto[i];
            break;
        case TY_GRASS:
            addphoto += o->mphoto[i];
            break;
        case TY_JADE:
            addphoto += o->mphoto[i];
            break;
        case TY_LEAF:
            addphoto += o->mphoto[i];
            o->leafphoto += o->mphoto[i];
            break;
        case TY_SUMMER:
            if ((world_get_time(o->world) % 3) != o->summerinactivity) {
                addphoto += o->mphoto[i];
            } else {
                o->seg_color[i] = CM_WINTER;
            }
            break;
        case TY_GREEN:
            addphoto += o->mphoto[i];
            break;
        case TY_C4:
            addphoto += o->mphoto[i];
            break;
        case TY_BARK:
            addphoto += o->mphoto[i];
            break;
        case TY_PURPLE:
            o->methanotrophy += o->mphoto[i];
            addmaintenance -= 0.99 * o->m[i];
            break;
        case TY_WINTER:
            if ((world_get_time(o->world) % 3) != o->summerinactivity) {
                o->seg_color[i] = CM_SUMMER;
                addphoto += o->mphoto[i];
            }
            break;
        // Stop at white, oldbark and blond
        case TY_WHITE:
            break;
        case TY_OLDBARK:
            break;
        case TY_BLOND:
            break;
        // Movement
        case TY_TEAL:
            if (o->mphoto[i] == -4) {
                if (jr_next_int(&g_rng, 100) < 8 && org_use_energy(o, P.teal_energy_consumption)) {
                    o->dx = bio_between(o->dx + 11.88 * (o->x2[i] - o->x1[i]) / o->mass, -P.max_vel, P.max_vel);
                    o->dy = bio_between(o->dy + 11.88 * (o->y2[i] - o->y1[i]) / o->mass, -P.max_vel, P.max_vel);
                    o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                }
            } else {
                addmaintenance -= 0.95 * o->m[i];
            }
            break;
        case TY_CYAN:
            if (jr_next_int(&g_rng, 100) < 8 && org_use_energy(o, P.cyan_energy_consumption)) {
                o->dx = bio_between(o->dx + 12.0 * (o->x2[i] - o->x1[i]) / o->mass, -P.max_vel, P.max_vel);
                o->dy = bio_between(o->dy + 12.0 * (o->y2[i] - o->y1[i]) / o->mass, -P.max_vel, P.max_vel);
                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
            }
            break;
        // is a consumer
        case TY_MAROON:
            addmaintenance -= 0.8 * o->m[i];
            break;
        // Organisms with indigo segments reduce the energy the new born virus receives
        case TY_INDIGO:
            addmaintenance -= 0.8 * o->m[i];
            break;
        // Organism will create spores
        case TY_SPORE:
            addmaintenance -= 0.99 * o->m[i];
            break;
        // is a fungus
        case TY_PINK:
            if (!o->isgray) {
                addmaintenance -= 0.8 * o->m[i];
            }
            break;
        // Organisms with flower segments reproduce later
        case TY_FLOWER:
            addmaintenance -= o->m[i] - (P.flower_energy_consumption * o->m[i]);
            break;
        // Lavender immunity against cream, plague, coral and fallow
        case TY_LAVENDER:
            addmaintenance -= 0.75 * o->m[i];
            break;
        // Auburn always has one real child if infected
        case TY_AUBURN:
            addmaintenance -= 0.99 * o->m[i];
            break;
        // Organisms with eye segments can react to other organisms without colliding with them
        case TY_EYE:
            addmaintenance += P.eye_energy_consumption * o->m[i];
            break;
        // Organisms with fallow segments inhibit the reproduction of other organisms
        case TY_FALLOW:
            if (o->fallowversion <= 3) {
                addmaintenance -= 0.9 * o->m[i];
            } else {
                if (!o->isinfectious) {
                    addmaintenance -= 0.8 * o->m[i];
                }
            }
            break;
        // Organisms with rose segments transfer energy to friendly organisms
        case TY_ROSE:
            if (!o->isakiller) {
                addmaintenance -= o->m[i] - (P.rose_energy_consumption * o->m[i]);
            } else {
                addmaintenance -= (0.75 * o->m[i]) - (P.rose_energy_consumption * o->m[i]);
            }
            break;
        case TY_VISION:
            if (o->haseyes) {
                addmaintenance -= o->m[i];
            } else {
                addmaintenance -= 0.95 * o->m[i];
            }
            break;
        // Organisms with gold segments live longer
        case TY_GOLD:
            if (o->isonlyc4) {
                addmaintenance -= 0.9 * o->m[i];
            } else {
                addmaintenance -= 0.874 * o->m[i];
            }
            break;
        // Organisms with dark segments mimic other segments
        case TY_DARK:
            addmaintenance -= o->m[i] - (P.dark_energy_consumption * o->m[i]);
            break;
        // Restore Jade
        case TY_DARKJADE:
            addphoto += o->mphoto[i];
            if (jr_next_int(&g_rng, o->jadefactor) < 8) {
                o->seg_color[i] = CM_JADE;
            } else {
                o->useextraeffects = true;
            }
            break;
        case TY_DARKGREEN:
            if (gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc))) == CM_SUMMER) {
                if ((world_get_time(o->world) % 3) != o->summerinactivity) {
                    addphoto += o->mphoto[i];
                    specialphoto += o->mphoto[i];
                    if (jr_next_int(&g_rng, o->jadefactor) < 8) {
                        o->seg_color[i] = CM_SUMMER;
                    } else {
                        o->useextraeffects = true;
                    }
                } else {
                    if (jr_next_int(&g_rng, o->jadefactor) < 8) {
                        o->seg_color[i] = CM_WINTER;
                    } else {
                        o->useextraeffects = true;
                    }
                }
            } else {
                if (gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc))) == CM_FOREST) {
                    addphoto += o->mphoto[i];
                    o->forestphoto += o->mphoto[i];
                    specialphoto += o->mphoto[i];
                } else {
                    if (gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc))) == CM_LEAF) {
                        addphoto += o->mphoto[i];
                        o->leafphoto += o->mphoto[i];
                        if (o->frames_color == 0) {
                            specialphoto += o->mphoto[i];
                        }
                    } else {
                        if (gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc))) == CM_LIME) {
                            if (o->crowded == true) {
                                addphoto += P.crowdedlime_energy_consumption * o->mphoto[i];
                                specialphoto += P.crowdedlime_energy_consumption * o->mphoto[i];
                            } else {
                                addphoto += o->mphoto[i];
                                specialphoto += o->mphoto[i];
                            }
                        } else {
                            addphoto += o->mphoto[i];
                            specialphoto += o->mphoto[i];
                        }
                    }
                }
                if (jr_next_int(&g_rng, o->jadefactor) < 8) {
                    o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                } else {
                    o->useextraeffects = true;
                }
            }
            break;
        // Healing
        case TY_GREENBROWN:
        case TY_LIGHTBROWN:
        case TY_BROKEN:
        case TY_LIGHT_BLUE:
        case TY_ICE:
            if (o->healing > 0) {
                if (jr_next_int(&g_rng, P.healing) <= o->healing) {
                    if (org_use_energy(o, P.magenta_energy_consumption)) {
                        o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                        o->isfrozen = false;
                        o->isinjured = false;
                        o->allfrozen = false;
                        o->update_effects = 2;
                        org_segments_restore_effects(o);
                    }
                }
            }
            break;
        case TY_DARKFIRE:
            if (o->healing > 0) {
                if (jr_next_int(&g_rng, P.healing) <= o->healing) {
                    if (org_use_energy(o, P.magenta_energy_consumption)) {
                        o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                        org_darkfire_restore_effects(o);
                    }
                }
            }
            if ((gc_get_symmetry(o->gc) != 3) || (gc_get_mirror(o->gc) == 0)) {
                if (jr_next_int(&g_rng, 100) < gc_get_symmetry(o->gc)) {
                    o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                    org_darkfire_restore_effects(o);
                } else {
                    o->useextraeffects = true;
                }
            } else {
                if (jr_next_int(&g_rng, 100) < 2) {
                    o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                    org_darkfire_restore_effects(o);
                } else {
                    o->useextraeffects = true;
                }
            }
            break;
        // Restore abilities
        case TY_DARKOLIVE:
            if (jr_next_int(&g_rng, 100) < 8) {
                o->seg_color[i] = CM_OLIVE;
            } else {
                o->useextraeffects = true;
            }
            break;
        case TY_DARKLILAC:
            if (jr_next_int(&g_rng, 100) < 8) {
                o->seg_color[i] = CM_LILAC;
            } else {
                o->useextraeffects = true;
            }
            break;
        case TY_DEEPSKY:
            if (jr_next_int(&g_rng, 100) < 8) {
                o->seg_color[i] = CM_SKY;
            } else {
                o->useextraeffects = true;
            }
            break;
        }
    }
    // Effective maintenance used for breathing
    o->maintenance = (addmaintenance / P.segment_cost_divisor);
    // Get chemotrophic energy from methane
    if (o->methanotrophy > 0) {
        o->methanotrophy = (o->methanotrophy * k_scale[o->growth_ratio - 1]);
        if ((o->age & 0x07) == 0x00) {
            o->energy += world_methanotrophy(o->world, o->methanotrophy);
        }
    }
    //Get sun's energy
    o->photosynthesis = (addphoto * k_scale[o->growth_ratio - 1]);
    if (o->photosynthesis > 0) {
        // Effective colony photosynthesis
        if (o->forestphoto > 0) {
            o->forestphoto = P.crowdedforest_energy_consumption * (o->forestphoto * k_scale[o->growth_ratio - 1]);
        }
        if (o->leafphoto > 0) {
            o->leafphoto = (o->leafphoto * k_scale[o->growth_ratio - 1]);
            if (o->frames_color > 0) {
                if ((o->frames_color == 1) && ((o->flash_color == CM_BLOND) || (o->flash_color == CM_LIGHTBROWN) || (o->flash_color == JC_YELLOW) || (o->flash_color == CM_GREENBROWN) || (o->flash_color == CM_DARKLILAC))) {
                    org_set_color(o, CM_DARKGREEN);
                }
                if (specialphoto > 0) {
                    if (o->colony_photosynthesis > 0) {
                        if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                            if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                                o->energy += world_photosynthesis(o->world, o->photosynthesis - o->leafphoto - (specialphoto * k_scale[o->growth_ratio - 1]) + o->colony_photosynthesis);
                            }
                        } else {
                            o->energy += world_photosynthesis(o->world, o->photosynthesis - o->leafphoto - (specialphoto * k_scale[o->growth_ratio - 1]) + o->colony_photosynthesis);
                        }
                        o->colony_photosynthesis = 0;
                    } else {
                        if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                            if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                                o->energy += world_photosynthesis(o->world, o->photosynthesis - o->leafphoto - (specialphoto * k_scale[o->growth_ratio - 1]));
                            }
                        } else {
                            o->energy += world_photosynthesis(o->world, o->photosynthesis - o->leafphoto - (specialphoto * k_scale[o->growth_ratio - 1]));
                        }
                    }
                } else {
                    if (o->colony_photosynthesis > 0) {
                        if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                            if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                                o->energy += world_photosynthesis(o->world, o->photosynthesis - o->leafphoto + o->colony_photosynthesis);
                            }
                        } else {
                            o->energy += world_photosynthesis(o->world, o->photosynthesis - o->leafphoto + o->colony_photosynthesis);
                        }
                        o->colony_photosynthesis = 0;
                    } else {
                        if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                            if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                                o->energy += world_photosynthesis(o->world, o->photosynthesis - o->leafphoto);
                            }
                        } else {
                            o->energy += world_photosynthesis(o->world, o->photosynthesis - o->leafphoto);
                        }
                    }
                }
            } else {
                if (specialphoto > 0) {
                    if (o->colony_photosynthesis > 0) {
                        if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                            if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                                o->energy += world_photosynthesis(o->world, o->photosynthesis - (specialphoto * k_scale[o->growth_ratio - 1]) + o->colony_photosynthesis);
                            }
                        } else {
                            o->energy += world_photosynthesis(o->world, o->photosynthesis - (specialphoto * k_scale[o->growth_ratio - 1]) + o->colony_photosynthesis);
                        }
                        o->colony_photosynthesis = 0;
                    } else {
                        if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                            if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                                o->energy += world_photosynthesis(o->world, o->photosynthesis - (specialphoto * k_scale[o->growth_ratio - 1]));
                            }
                        } else {
                            o->energy += world_photosynthesis(o->world, o->photosynthesis - (specialphoto * k_scale[o->growth_ratio - 1]));
                        }
                    }
                } else {
                    if (o->colony_photosynthesis > 0) {
                        if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                            if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                                o->energy += world_photosynthesis(o->world, o->photosynthesis + o->colony_photosynthesis);
                            }
                        } else {
                            o->energy += world_photosynthesis(o->world, o->photosynthesis + o->colony_photosynthesis);
                        }
                        o->colony_photosynthesis = 0;
                    } else {
                        if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                            if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                                o->energy += world_photosynthesis(o->world, o->photosynthesis);
                            }
                        } else {
                            o->energy += world_photosynthesis(o->world, o->photosynthesis);
                        }
                    }
                }
            }
        } else {
            if (specialphoto > 0) {
                if (o->colony_photosynthesis > 0) {
                    if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                        if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                            o->energy += world_photosynthesis(o->world, o->photosynthesis - (specialphoto * k_scale[o->growth_ratio - 1]) + o->colony_photosynthesis);
                        }
                    } else {
                        o->energy += world_photosynthesis(o->world, o->photosynthesis - (specialphoto * k_scale[o->growth_ratio - 1]) + o->colony_photosynthesis);
                    }
                    o->colony_photosynthesis = 0;
                } else {
                    if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                        if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                            o->energy += world_photosynthesis(o->world, o->photosynthesis - (specialphoto * k_scale[o->growth_ratio - 1]));
                        }
                    } else {
                        o->energy += world_photosynthesis(o->world, o->photosynthesis - (specialphoto * k_scale[o->growth_ratio - 1]));
                    }
                }
            } else {
                if (o->colony_photosynthesis > 0) {
                    if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                        if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                            o->energy += world_photosynthesis(o->world, o->photosynthesis + o->colony_photosynthesis);
                        }
                    } else {
                        o->energy += world_photosynthesis(o->world, o->photosynthesis + o->colony_photosynthesis);
                    }
                    o->colony_photosynthesis = 0;
                } else {
                    if ((o->lowersymmetric > 0) && (world_get_co2(o->world) < o->lowersymmetric)) {
                        if (jr_next_int(&g_rng, o->lowersymmetric) < world_get_co2(o->world)) {
                            o->energy += world_photosynthesis(o->world, o->photosynthesis);
                        }
                    } else {
                        o->energy += world_photosynthesis(o->world, o->photosynthesis);
                    }
                }
            }
        }
    }
}

static void org_segments_update_effects(Organism *o)
{
    int i = 0;
    double addmaintenance = o->mass;
    for (i = o->segments - 1; i >= 0; i--) {
        // Manteniment
        switch (org_get_type_color(o->seg_color[i])) {
        // Movement
        case TY_TEAL:
            if (o->mphoto[i] == -4) {
                if (jr_next_int(&g_rng, 100) < 8 && org_use_energy(o, P.teal_energy_consumption)) {
                    o->dx = bio_between(o->dx + 11.88 * (o->x2[i] - o->x1[i]) / o->mass, -P.max_vel, P.max_vel);
                    o->dy = bio_between(o->dy + 11.88 * (o->y2[i] - o->y1[i]) / o->mass, -P.max_vel, P.max_vel);
                    o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
                }
            } else {
                addmaintenance -= 0.95 * o->m[i];
            }
            break;
        case TY_CYAN:
            if (jr_next_int(&g_rng, 100) < 8 && org_use_energy(o, P.cyan_energy_consumption)) {
                o->dx = bio_between(o->dx + 12.0 * (o->x2[i] - o->x1[i]) / o->mass, -P.max_vel, P.max_vel);
                o->dy = bio_between(o->dy + 12.0 * (o->y2[i] - o->y1[i]) / o->mass, -P.max_vel, P.max_vel);
                o->dtheta = bio_between(o->dtheta + bio_random_sign() * o->m[i] * M_PI / o->inertia, -P.max_rot, P.max_rot);
            }
            break;
        // is a consumer
        case TY_MAROON:
            addmaintenance -= 0.8 * o->m[i];
            break;
        // Organisms with indigo segments reduce the energy the new born virus receives
        case TY_INDIGO:
            addmaintenance -= 0.8 * o->m[i];
            break;
        // Organism will create spores
        case TY_SPORE:
            addmaintenance -= 0.99 * o->m[i];
            break;
        // Organisms with violet poison other segments and make them useless
        case TY_VIOLET:
            if ((!o->isaconsumer) && (!o->isafungus)) {
                addmaintenance -= 0.95 * o->m[i];
            }
            break;
        // is a fungus
        case TY_PINK:
            if (!o->isgray) {
                addmaintenance -= 0.8 * o->m[i];
            }
            break;
        // Organisms with flower segments reproduce later
        case TY_FLOWER:
            addmaintenance -= o->m[i] - (P.flower_energy_consumption * o->m[i]);
            break;
        // Lavender immunity against cream, plague, coral and fallow
        case TY_LAVENDER:
            if (o->isaconsumer) {
                addmaintenance -= 0.75 * o->m[i];
            } else {
                addmaintenance -= 0.95 * o->m[i];
            }
            break;
        // Auburn always has one real child if infected
        case TY_AUBURN:
            addmaintenance -= 0.99 * o->m[i];
            break;
        // Organisms with eye segments can react to other organisms without colliding with them
        case TY_EYE:
            addmaintenance += P.eye_energy_consumption * o->m[i];
            break;
        // Organisms with olive destroy defense and killer segments
        case TY_OLIVE:
            addmaintenance -= 0.55 * o->m[i];
            break;
        // Organisms with sky segments freeze movement and photosynthesis, if modified
        case TY_SKY:
            if (o->skyversion == 1) {
                addmaintenance -= 0.9 * o->m[i];
            }
            break;
        // Organisms with blue segments protect with a shield
        case TY_BLUE:
            addmaintenance -= 0.9 * o->m[i];
            break;
        // Organisms with fallow segments inhibit the reproduction of other organisms
        case TY_FALLOW:
            addmaintenance -= 0.99 * o->m[i];
            break;
        // Organisms with coral segments transform viruses into themselves
        case TY_CORAL:
            if ((o->gold > 0) && (!o->isaconsumer) && (!o->isafungus)) {
                addmaintenance -= 0.9 * o->m[i];
            }
            break;
        // Organisms with rose segments transfer energy to friendly organisms
        case TY_ROSE:
            if (!o->isakiller) {
                addmaintenance -= o->m[i] - (P.rose_energy_consumption * o->m[i]);
            } else {
                addmaintenance -= (0.75 * o->m[i]) - (P.rose_energy_consumption * o->m[i]);
            }
            break;
        case TY_VISION:
            if (o->haseyes) {
                addmaintenance -= o->m[i];
            } else {
                addmaintenance -= 0.95 * o->m[i];
            }
            break;
        // Organisms with gold segments live longer
        case TY_GOLD:
            if ((o->isaconsumer) || (o->isafungus)) {
                addmaintenance -= 0.874 * o->m[i];
            } else {
                addmaintenance -= 0.95 * o->m[i];
            }
            break;
        // Organisms with dark segments mimic other segments
        case TY_DARK:
            addmaintenance -= o->m[i] - (P.dark_energy_consumption * o->m[i]);
            break;
        // Healing
        case TY_GREENBROWN:
        case TY_LIGHTBROWN:
        case TY_BROKEN:
        case TY_LIGHT_BLUE:
        case TY_ICE:
            if (o->healing > 0) {
                if (jr_next_int(&g_rng, P.healing) <= o->healing) {
                    if (org_use_energy(o, P.magenta_energy_consumption)) {
                        o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                        o->isfrozen = false;
                        o->isinjured = false;
                        o->allfrozen = false;
                        o->update_effects = 2;
                        org_segments_restore_effects(o);
                    }
                }
            }
            break;
        case TY_DARKFIRE:
            if (o->healing > 0) {
                if (jr_next_int(&g_rng, P.healing) <= o->healing) {
                    if (org_use_energy(o, P.magenta_energy_consumption)) {
                        o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                        org_darkfire_restore_effects(o);
                    }
                }
            }
            if ((gc_get_symmetry(o->gc) != 3) || (gc_get_mirror(o->gc) == 0)) {
                if (jr_next_int(&g_rng, 100) < gc_get_symmetry(o->gc)) {
                    o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                    org_darkfire_restore_effects(o);
                } else {
                    o->useextraeffects = true;
                }
            } else {
                if (jr_next_int(&g_rng, 100) < 2) {
                    o->seg_color[i] = gene_get_color(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)));
                    org_darkfire_restore_effects(o);
                } else {
                    o->useextraeffects = true;
                }
            }
            break;
        // Restore abilities
        case TY_DARKOLIVE:
            addmaintenance -= 0.5 * o->m[i];
            if (jr_next_int(&g_rng, 100) < 8) {
                o->seg_color[i] = CM_OLIVE;
            } else {
                o->useextraeffects = true;
            }
            break;
        case TY_DARKLILAC:
            if (jr_next_int(&g_rng, 100) < 8) {
                o->seg_color[i] = CM_LILAC;
            } else {
                o->useextraeffects = true;
            }
            break;
        case TY_DEEPSKY:
            if (o->skyversion == 1) {
                addmaintenance -= 0.9 * o->m[i];
            }
            if (jr_next_int(&g_rng, 100) < 8) {
                o->seg_color[i] = CM_SKY;
            } else {
                o->useextraeffects = true;
            }
            break;
        }
    }
    // Effective maintenance used for breathing
    o->maintenance = (addmaintenance / P.segment_cost_divisor);
}

static void org_darkfire_restore_effects(Organism *o)
{
    int i = 0;
    o->healing = 0;
    o->isinjured = false;
    for (i = o->segments - 1; i >= 0; i--) {
        // Manteniment
        switch (org_get_type_color(o->seg_color[i])) {
        case TY_MAGENTA:
            if (gc_get_symmetry(o->gc) != 3) {
                o->healing += j_d2i((10 * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
            } else {
                if (gc_get_mirror(o->gc) == 0) {
                    o->healing += j_d2i(((50 / 6) * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
                } else {
                    o->healing += j_d2i(((20 / 3) * gene_get_length(gc_get_gene(o->gc, i % gc_get_n_genes(o->gc)))));
                }
            }
            break;
        case TY_LIGHTBROWN:
            o->isinjured = true;
            break;
        case TY_GREENBROWN:
            o->isinjured = true;
            break;
        case TY_DARKFIRE:
            o->isinjured = true;
            break;
        case TY_LIGHT_BLUE:
            o->isinjured = true;
            break;
        case TY_BROKEN:
            o->isinjured = true;
            break;
        case TY_ICE:
            o->isinjured = true;
            break;
        }
    }
}

static int org_get_type_color(JColor c)
{
    if (c == JC_GREEN)
        return TY_GREEN;
    if (c == CM_FOREST)
        return TY_FOREST;
    if (c == CM_SPRING)
        return TY_SPRING;
    if (c == CM_SUMMER)
        return TY_SUMMER;
    if (c == CM_LIME)
        return TY_LIME;
    if (c == CM_LEAF)
        return TY_LEAF;
    if (c == CM_C4)
        return TY_C4;
    if (c == CM_JADE)
        return TY_JADE;
    if (c == CM_GRASS)
        return TY_GRASS;
    if (c == CM_BARK)
        return TY_BARK;
    if (c == CM_PURPLE)
        return TY_PURPLE;
    if (c == JC_RED)
        return TY_RED;
    if (c == CM_FIRE)
        return TY_FIRE;
    if (c == JC_ORANGE)
        return TY_ORANGE;
    if (c == CM_MAROON)
        return TY_MAROON;
    if (c == JC_PINK)
        return TY_PINK;
    if (c == CM_CREAM)
        return TY_CREAM;
    if (c == JC_LIGHT_GRAY)
        return TY_SILVER;
    if (c == CM_SPIKE)
        return TY_SPIKE;
    if (c == CM_LILAC)
        return TY_LILAC;
    if (c == JC_GRAY)
        return TY_GRAY;
    if (c == CM_VIOLET)
        return TY_VIOLET;
    if (c == CM_OLIVE)
        return TY_OLIVE;
    if (c == CM_SKY)
        return TY_SKY;
    if (c == JC_BLUE)
        return TY_BLUE;
    if (c == CM_OCHRE)
        return TY_OCHRE;
    if (c == CM_FALLOW)
        return TY_FALLOW;
    if (c == CM_SPORE)
        return TY_SPORE;
    if (c == JC_WHITE)
        return TY_WHITE;
    if (c == CM_PLAGUE)
        return TY_PLAGUE;
    if (c == CM_CORAL)
        return TY_CORAL;
    if (c == CM_MINT)
        return TY_MINT;
    if (c == CM_LAVENDER)
        return TY_LAVENDER;
    if (c == JC_MAGENTA)
        return TY_MAGENTA;
    if (c == CM_ROSE)
        return TY_ROSE;
    if (c == JC_CYAN)
        return TY_CYAN;
    if (c == CM_TEAL)
        return TY_TEAL;
    if (c == JC_YELLOW)
        return TY_YELLOW;
    if (c == CM_AUBURN)
        return TY_AUBURN;
    if (c == CM_INDIGO)
        return TY_INDIGO;
    if (c == CM_BLOND)
        return TY_BLOND;
    if (c == CM_FLOWER)
        return TY_FLOWER;
    if (c == JC_DARK_GRAY)
        return TY_DARKGRAY;
    if (c == CM_GOLD)
        return TY_GOLD;
    if (c == CM_DARK)
        return TY_DARK;
    if (c == CM_EYE)
        return TY_EYE;
    if (c == CM_WINTER)
        return TY_WINTER;
    if (c == CM_OLDBARK)
        return TY_OLDBARK;
    if (c == CM_DARKJADE)
        return TY_DARKJADE;
    if (c == CM_DARKGREEN)
        return TY_DARKGREEN;
    if (c == CM_DARKFIRE)
        return TY_DARKFIRE;
    if (c == CM_DARKLILAC)
        return TY_DARKLILAC;
    if (c == CM_DEEPSKY)
        return TY_DEEPSKY;
    if (c == CM_DARKOLIVE)
        return TY_DARKOLIVE;
    if (c == CM_SPIKEPOINT)
        return TY_SPIKEPOINT;
    if (c == CM_FRUIT)
        return TY_FRUIT;
    if (c == CM_VISION)
        return TY_VISION;
    if (c == CM_ICE)
        return TY_ICE;
    if (c == CM_LIGHT_BLUE)
        return TY_LIGHT_BLUE;
    if (c == CM_LIGHTBROWN)
        return TY_LIGHTBROWN;
    if (c == CM_GREENBROWN)
        return TY_GREENBROWN;
    if (c == CM_BROKEN)
        return TY_BROKEN;
    if (c == CM_DEADBARK)
        return TY_DEADBARK;
    if (c == CM_BROWN)
        return TY_BROWN;
    return TY_NOCOLOR;
}

static void org_set_color(Organism *o, JColor c)
{
    o->flash_color = c;
    o->frames_color = 10;
}

/* Basic constructor. Doesn't initialize it: use {@link randomCreate}
 * or {@link inherit} to do this.
 * 
 * @param world  A reference to the world where this organism is in.
 */
Organism *org_new(World *world)
{
    Organism *o = org_alloc();
    o->world = world;
    o->visible_world = world->visible_world;
    o->theta = jr_next_double(&g_rng) * M_PI * 2.0;
    return o;
}

/* Construct an organism with a given genetic code. Doesn't initialize it:
 * use {@link pasteOrganism} to do it. Use {@link World.addOrganism} to add
 * it to the world.
 * 
 * @param world  A reference to the world where this organism is in.
 * @param geneticCode  A reference to the genetic code of this organism.
 */
Organism *org_new_with_code(World *world, GeneticCode *geneticCode)
{
    Organism *o = org_alloc();
    o->world = world;
    o->visible_world = world->visible_world;
    o->theta = jr_next_double(&g_rng) * M_PI * 2.0;
    o->gc = geneticCode;
    return o;
}
