/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Translated to C in 2026 from Organism.java and Vector2D.java
 * of Biogenesis 0.9 (https://biogenesis.sourceforge.net/),
 * Copyright (C) 2006-2010 Joan Queralt Molina.
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

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "sim/jmath.h"
#include "sim/params.h"
#include "sim/world.h"

/* Java's (int) cast: truncation, NaN to 0, saturating at the int range. */
static inline int jcast_int(double d)
{
    if (d != d)
        return 0;
    if (d >= 2147483647.0)
        return 2147483647;
    if (d <= -2147483648.0)
        return (-2147483647 - 1);
    return (int)d;
}

Organism *org_new(World *w)
{
    Organism *o = calloc(1, sizeof *o);
    if (!o)
        abort();
    o->world = w;
    o->theta = jr_next_double(&g_rng) * M_PI * 2.0;
    o->last_theta = -1;
    o->has_moved = true;
    o->alive = true;
    o->flash_color = FLASH_NONE;
    return o;
}

Organism *org_new_with_code(World *w, GeneticCode *gc)
{
    Organism *o = org_new(w);
    o->gc = gc_ref(gc);
    return o;
}

void org_free(Organism *o)
{
    if (!o)
        return;
    gc_unref(o->gc);
    gc_unref(o->infected_gc);
    free(o->m1);
    free(o);
}

/* Organism.create: per-segment arrays, zeroed like Java's new int[]. */
void org_create(Organism *o)
{
    int n = o->gc->ngenes * o->gc->symmetry;
    o->segments = n;
    free(o->m1);
    size_t ints = (size_t)n * 8 * sizeof(int);
    size_t dbls = (size_t)n * 5 * sizeof(double);
    char *block = calloc(1, dbls + ints + (size_t)n);
    if (!block)
        abort();
    /* One block per organism; doubles first so they stay aligned. m1 is
     * the block start and the pointer that gets freed. */
    o->m1 = (double *)block;
    o->m2 = o->m1 + n;
    o->m = o->m2 + n;
    o->a1 = o->m + n;
    o->a2 = o->a1 + n;
    int *ib = (int *)(block + dbls);
    o->start_x = ib;
    o->start_y = ib + n;
    o->end_x = ib + 2 * n;
    o->end_y = ib + 3 * n;
    o->x1 = ib + 4 * n;
    o->y1 = ib + 5 * n;
    o->x2 = ib + 6 * n;
    o->y2 = ib + 7 * n;
    o->seg_color = (uint8_t *)(block + dbls + ints);
    for (int i = 0; i < n; i++)
        o->seg_color[i] = o->gc->genes[i % o->gc->ngenes].color;
}

static bool org_is_inside_world(Organism *o);

void org_symmetric(Organism *o)
{
    const GeneticCode *gc = o->gc;
    int symmetry = gc->symmetry;
    int mirror = gc->mirror;
    int sequence = o->segments / symmetry;
    int left = 0, right = 0, top = 0, bottom = 0;
    int segment = 0;
    double vmod = 0, vth = 0;

    for (int i = 0; i < symmetry; i++) {
        for (int j = 0; j < sequence; j++, segment++) {
            const Gene *g = &gc->genes[j];
            vmod = g->length / k_scale[o->growth_ratio - 1];
            if (j == 0) {
                o->start_x[segment] = 0;
                o->start_y[segment] = 0;
                if (mirror == 0 || i % 2 == 0) {
                    vth = g->theta + i * 2 * M_PI / symmetry;
                } else {
                    vth = g->theta + (i - 1) * 2 * M_PI / symmetry;
                    vth = M_PI - vth;
                }
            } else {
                o->start_x[segment] = o->end_x[segment - 1];
                o->start_y[segment] = o->end_y[segment - 1];
                if (mirror == 0 || i % 2 == 0)
                    vth += g->theta;
                else
                    vth += -g->theta;
            }
            o->end_x[segment] = jround(vmod * bio_cos(vth) + o->start_x[segment]);
            o->end_y[segment] = jround(vmod * bio_sin(vth) + o->start_y[segment]);
            left = o->end_x[segment] < left ? o->end_x[segment] : left;
            right = o->end_x[segment] > right ? o->end_x[segment] : right;
            top = o->end_y[segment] < top ? o->end_y[segment] : top;
            bottom = o->end_y[segment] > bottom ? o->end_y[segment] : bottom;
        }
    }
    jrect_set(&o->size_rect, left, top, right - left + 1, bottom - top + 1);
    int cx0 = (left + right) >> 1;
    int cy0 = (top + bottom) >> 1;
    o->mass = 0;
    o->inertia = 0;
    for (int i = 0; i < o->segments; i++) {
        o->start_x[i] -= cx0;
        o->start_y[i] -= cy0;
        o->end_x[i] -= cx0;
        o->end_y[i] -= cy0;
        int sx = o->start_x[i], sy = o->start_y[i], ex = o->end_x[i], ey = o->end_y[i];
        o->m1[i] = sqrt((double)(sx * sx + sy * sy));
        o->m2[i] = sqrt((double)(ex * ex + ey * ey));
        o->m[i] = sqrt(bio_pow(ex - sx, 2) + bio_pow(ey - sy, 2));
        if (o->m[i] == 0)
            o->m[i] = 0.01;
        o->mass += o->m[i];
        double cx = (sx + ex) / 2.0;
        double cy = (sy + ey) / 2.0;
        /* The original adds cy*cy without the segment mass. Kept: inertia
         * feeds every collision and cilia kick. */
        o->inertia += bio_pow(o->m[i], 3) / 12.0 + o->m[i] * cx * cx + cy * cy;
    }
    org_prepare_angles(o);
}

void org_prepare_angles(Organism *o)
{
    for (int i = 0; i < o->segments; i++) {
        o->a1[i] = bio_atan2(o->start_y[i], o->start_x[i]);
        o->a2[i] = bio_atan2(o->end_y[i], o->end_x[i]);
    }
}

void org_calculate_bounds(Organism *o, bool force)
{
    /* right/bottom start at Double.MIN_VALUE, the smallest positive double,
     * not the most negative one. Harmless for world coordinates; kept. */
    double left = DBL_MAX, right = DBL_TRUE_MIN, top = DBL_MAX, bottom = DBL_TRUE_MIN;
    if (o->last_theta != o->theta || force) {
        /* Each rotated point is a pure function of (theta, |p|, angle of p),
         * and a segment starts where the previous one ends (or, for the first
         * segment of each symmetry arm, at the shared origin), so each
         * distinct point is rotated once and copied. Same values as rotating
         * every point; half the trigonometry. */
        int n = o->segments;
        for (int i = n - 1; i >= 0; i--) {
            double th = o->theta + o->a2[i];
            o->x2[i] = jcast_int(o->m2[i] * bio_cos(th));
            o->y2[i] = jcast_int(o->m2[i] * bio_sin(th));
        }
        bool have_origin = false;
        int ox = 0, oy = 0, rx = 0, ry = 0;
        for (int i = n - 1; i >= 0; i--) {
            int sx = o->start_x[i], sy = o->start_y[i];
            if (i > 0 && sx == o->end_x[i - 1] && sy == o->end_y[i - 1]) {
                o->x1[i] = o->x2[i - 1];
                o->y1[i] = o->y2[i - 1];
            } else if (have_origin && sx == ox && sy == oy) {
                o->x1[i] = rx;
                o->y1[i] = ry;
            } else {
                double th = o->theta + o->a1[i];
                o->x1[i] = jcast_int(o->m1[i] * bio_cos(th));
                o->y1[i] = jcast_int(o->m1[i] * bio_sin(th));
                have_origin = true;
                ox = sx;
                oy = sy;
                rx = o->x1[i];
                ry = o->y1[i];
            }
        }
    }
    for (int i = o->segments - 1; i >= 0; i--) {
        left = jmin(jmin(left, o->x1[i] + o->dcenter_x), o->x2[i] + o->dcenter_x);
        right = jmax(jmax(right, o->x1[i] + o->dcenter_x), o->x2[i] + o->dcenter_x);
        top = jmin(jmin(top, o->y1[i] + o->dcenter_y), o->y2[i] + o->dcenter_y);
        bottom = jmax(jmax(bottom, o->y1[i] + o->dcenter_y), o->y2[i] + o->dcenter_y);
    }
    jrect_set(&o->r, jcast_int(left), jcast_int(top), jcast_int(right - left + 1) + 1,
              jcast_int(bottom - top + 1) + 1);
    o->last_theta = o->theta;
}

static bool org_place_random(Organism *o)
{
    World *w = o->world;
    for (int i = 12; i > 0; i--) {
        int bx = w->width - o->size_rect.width;
        int by = w->height - o->size_rect.height;
        /* Random.nextInt throws for a bound <= 0; nothing can be placed. */
        if (bx <= 0)
            return false;
        int ox = jr_next_int(&g_rng, bx);
        if (by <= 0)
            return false;
        int oy = jr_next_int(&g_rng, by);
        jrect_set(&o->r, ox, oy, o->size_rect.width, o->size_rect.height);
        o->dcenter_x = o->center_x = ox + (o->size_rect.width >> 1);
        o->dcenter_y = o->center_y = oy + (o->size_rect.height >> 1);
        if (!world_fast_check_hit(w, o)) {
            o->id = world_new_id(w);
            return true;
        }
    }
    return false;
}

static bool org_place_near(Organism *o, Organism *parent)
{
    World *w = o->world;
    int npos = jr_next_int(&g_rng, 8);
    for (int side = 0; side < 8; side++) {
        /* o->r.width is 0 on the first try (a fresh Rectangle), then the
         * width from the previous try's bounds, exactly as in Java. */
        o->dcenter_x =
            parent->dcenter_x + (parent->r.width / 2 + o->r.width / 2 + 1) * k_side[npos][0];
        o->dcenter_y =
            parent->dcenter_y + (parent->r.height / 2 + o->r.height / 2 + 1) * k_side[npos][1];
        o->center_x = jcast_int(o->dcenter_x);
        o->center_y = jcast_int(o->dcenter_y);
        org_calculate_bounds(o, true);
        if (org_is_inside_world(o)) {
            if (!world_fast_check_hit(w, o)) {
                if (parent->gc->disperse_children) {
                    o->dx = k_side[npos][0];
                    o->dy = k_side[npos][1];
                } else {
                    o->dx = parent->dx;
                    o->dy = parent->dy;
                }
                o->id = world_new_id(w);
                parent->energy -= o->energy;
                return true;
            }
        }
        npos = (npos + 1) % 8;
    }
    return false;
}

bool org_random_create(Organism *o)
{
    World *w = o->world;
    gc_assign(&o->gc, NULL);
    o->gc = gc_new_random();
    o->parent_id = -1;
    o->generation = 1;
    o->growth_ratio = 16;
    o->energy = jmin(P.initial_energy, w->co2);
    world_decrease_co2(w, o->energy);
    world_add_o2(w, o->energy);
    org_create(o);
    org_symmetric(o);
    return org_place_random(o);
}

bool org_inherit(Organism *o, Organism *parent, bool first)
{
    GeneticCode *inherit_gc;
    bool ok = true;
    if (parent->infected_gc) {
        inherit_gc = parent->infected_gc;
        o->generation = parent->infected_generation + 1;
    } else {
        inherit_gc = parent->gc;
        o->generation = parent->generation + 1;
    }
    gc_assign(&o->gc, NULL);
    o->gc = gc_new_mutated(inherit_gc);
    o->parent_id = parent->id;
    o->growth_ratio = 16;
    o->energy = jmin(inherit_gc->reproduce_energy / (double)(parent->n_children + 1),
                     parent->energy);
    if (first || parent->energy >= o->energy + P.yellow_energy_consumption) {
        org_create(o);
        org_symmetric(o);
        ok = org_place_near(o, parent);
        if (ok && !first)
            org_use_energy(parent, P.yellow_energy_consumption);
    } else {
        ok = false;
    }
    return ok;
}

bool org_paste(Organism *o, int posx, int posy)
{
    World *w = o->world;
    o->parent_id = -1;
    o->generation = 1;
    o->growth_ratio = 16;
    org_create(o);
    org_symmetric(o);
    o->dcenter_x = o->center_x = posx;
    o->dcenter_y = o->center_y = posy;
    org_calculate_bounds(o, true);
    if (org_is_inside_world(o)) {
        if (!world_fast_check_hit(w, o)) {
            o->id = world_new_id(w);
            o->energy = jmin(P.initial_energy, w->co2);
            world_decrease_co2(w, o->energy);
            world_add_o2(w, o->energy);
            return true;
        }
    }
    return false;
}

static void org_grow(Organism *o)
{
    if (o->growth_ratio > 1 && (o->age & 0x07) == 0x07 && o->alive && o->energy >= o->mass / 10) {
        o->growth_ratio--;
        double m = o->mass, I = o->inertia;
        org_symmetric(o);
        /* Kinetic energy is kept: speed scales with the mass change. */
        m = sqrt(m / o->mass);
        o->dx *= m;
        o->dy *= m;
        o->dtheta *= sqrt(I / o->inertia);
        o->has_grown = 1;
    } else if (o->growth_ratio < 15 && o->energy < o->mass / 12) {
        o->growth_ratio++;
        double m = o->mass, I = o->inertia;
        org_symmetric(o);
        m = sqrt(m / o->mass);
        o->dx *= m;
        o->dy *= m;
        o->dtheta *= sqrt(I / o->inertia);
        o->has_grown = -1;
    } else {
        o->has_grown = 0;
    }
}

void org_reproduce(Organism *o)
{
    for (int i = 0; i < bio_between_i(o->n_children, 1, 8); i++) {
        Organism *child = org_new(o->world);
        if (org_inherit(child, o, i == 0)) {
            o->total_children++;
            world_add_organism(o->world, child, o);
            gc_assign(&o->infected_gc, NULL);
        } else {
            org_free(child);
        }
        o->time_to_reproduce = 20;
    }
}

static bool org_is_inside_world(Organism *o)
{
    World *w = o->world;
    JRect *r = &o->r;
    if (r->x < 0 || r->y < 0 || r->x + r->width >= w->width || r->y + r->height >= w->height) {
        if (r->height <= 1 && r->width <= 1 && o->alive && r->x == 0 && r->y == 0)
            org_die(o, o);
        if (r->x < 0 || r->x + r->width >= w->width)
            o->dx = -o->dx;
        if (r->y < 0 || r->y + r->height >= w->height)
            o->dy = -o->dy;
        o->dtheta = 0;
        return false;
    }
    return true;
}

static void org_offset(Organism *o, double ox, double oy, double otheta)
{
    o->dcenter_x += ox;
    o->dcenter_y += oy;
    o->theta += otheta;
    o->center_x = jcast_int(o->dcenter_x);
    o->center_y = jcast_int(o->dcenter_y);
}

static void org_set_color(Organism *o, int c)
{
    o->flash_color = (uint8_t)c;
    o->frames_color = 10;
}

static void org_segments_frame_effects(Organism *o)
{
    if (!o->alive)
        return;
    double photosynthesis = 0;
    o->n_children = 1;
    for (int i = o->segments - 1; i >= 0; i--) {
        switch (o->seg_color[i]) {
        case COL_CYAN:
            if (jr_next_int(&g_rng, 100) < 8 && org_use_energy(o, P.cyan_energy_consumption)) {
                o->dx = bio_between(o->dx + 12.0 * (o->x2[i] - o->x1[i]) / o->mass, -P.max_vel,
                                    P.max_vel);
                o->dy = bio_between(o->dy + 12.0 * (o->y2[i] - o->y1[i]) / o->mass, -P.max_vel,
                                    P.max_vel);
                double kick = bio_random_sign() * o->m[i] * M_PI / o->inertia;
                o->dtheta = bio_between(o->dtheta + kick, -P.max_rot, P.max_rot);
            }
            break;
        case COL_GREEN:
            if (org_use_energy(o, P.green_energy_consumption))
                photosynthesis += o->m[i];
            break;
        case COL_YELLOW:
            o->n_children++;
            break;
        }
    }
    o->energy += world_photosynthesis(o->world, photosynthesis);
}

static void org_rubbing(Organism *o)
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

bool org_move(Organism *o)
{
    World *w = o->world;
    bool collision = false;
    o->has_moved = false;
    o->last_frame = o->r;
    if (fabs(o->dx) < BIO_TOL)
        o->dx = 0;
    if (fabs(o->dy) < BIO_TOL)
        o->dy = 0;
    if (fabs(o->dtheta) < BIO_TOL)
        o->dtheta = 0;
    org_segments_frame_effects(o);
    org_rubbing(o);
    org_grow(o);
    double dxbak = o->dx, dybak = o->dy, dthetabak = o->dtheta;
    org_offset(o, o->dx, o->dy, o->dtheta);
    org_calculate_bounds(o, o->has_grown != 0);

    if (o->has_grown != 0 || o->dx != 0 || o->dy != 0 || o->dtheta != 0) {
        o->has_moved = true;
        collision = !org_is_inside_world(o);
        /* (biological corridors to networked worlds are not ported) */
        if (world_check_hit(w, o))
            collision = true;
        if (collision) {
            o->has_moved = false;
            org_offset(o, -dxbak, -dybak, -dthetabak);
            if (o->has_grown != 0) {
                o->growth_ratio += o->has_grown;
                org_symmetric(o);
            }
            org_calculate_bounds(o, o->has_grown != 0);
        }
    }
    if (o->time_to_reproduce > 0)
        o->time_to_reproduce--;
    if (o->energy > o->gc->reproduce_energy + P.yellow_energy_consumption * (o->n_children - 1) &&
        o->growth_ratio == 1 && o->time_to_reproduce == 0 && o->alive)
        org_reproduce(o);
    if (o->energy > 2 * o->gc->reproduce_energy)
        org_use_energy(o, o->energy - 2 * o->gc->reproduce_energy);
    org_breath(o);
    return o->energy > BIO_TOL;
}

bool org_use_energy(Organism *o, double q)
{
    if (o->energy < q)
        return false;
    double respiration = world_respiration(o->world, q);
    o->energy -= respiration;
    if (respiration < q)
        return false;
    return true;
}

void org_breath(Organism *o)
{
    if (o->alive) {
        o->age++;
        bool can_breath = org_use_energy(o, jmin(o->mass / P.segment_cost_divisor, o->energy));
        if ((o->age >> 8) > o->gc->max_age || !can_breath) {
            org_die(o, NULL);
        } else if (o->energy <= BIO_TOL) {
            o->alive = false;
            world_decrease_population(o->world);
            world_organism_has_died(o->world, o, NULL);
        }
    } else {
        /* the corpse slowly decays */
        org_use_energy(o, jmin(o->energy, P.decay_energy));
    }
}

void org_die(Organism *o, Organism *killer)
{
    o->alive = false;
    o->has_moved = true;
    for (int i = 0; i < o->segments; i++)
        o->seg_color[i] = COL_BROWN;
    world_decrease_population(o->world);
    if (killer)
        killer->total_kills++;
    world_organism_has_died(o->world, o, killer);
}

void org_infected_by(Organism *o, Organism *infector)
{
    infector->total_infected++;
    gc_assign(&o->infected_gc, infector->gc);
    o->infected_generation = infector->generation;
    world_organism_has_been_infected(o->world, o, infector);
}

void org_revive(Organism *o)
{
    if (o->alive)
        return;
    for (int i = 0; i < o->segments; i++)
        o->seg_color[i] = o->gc->genes[i % o->gc->ngenes].color;
    o->alive = true;
    o->has_moved = true;
    o->age = 0;
    world_increase_population(o->world);
}

/* Organism.touchMove: impulse exchange at the contact point p along the
 * normal of line l (the line hit in its middle rather than at a vertex). */
static void org_touch_move(Organism *a, Organism *b, double px, double py, const JLine *l,
                           bool this_organism)
{
    double rapx = px - a->dcenter_x;
    double rapy = py - a->dcenter_y;
    double rbpx = px - b->dcenter_x;
    double rbpy = py - b->dcenter_y;
    double vap1x = a->dx - a->dtheta * rapy + a->has_grown * rapx / 10.0;
    double vap1y = a->dy + a->dtheta * rapx + a->has_grown * rapy / 10.0;
    double vbp1x = b->dx - b->dtheta * rbpy;
    double vbp1y = b->dy + b->dtheta * rbpx;
    double vab1x = vap1x - vbp1x;
    double vab1y = vap1y - vbp1y;
    double nx = l->y1 - l->y2;
    double ny = l->x2 - l->x1;
    double modn = sqrt(nx * nx + ny * ny);
    if (modn == 0)
        modn = 0.000000000000001;
    nx /= modn;
    ny /= modn;
    if (this_organism) {
        if ((px + nx - b->dcenter_x) * (px + nx - b->dcenter_x) +
                (py + ny - b->dcenter_y) * (py + ny - b->dcenter_y) <
            (px - nx - b->dcenter_x) * (px - nx - b->dcenter_x) +
                (py - ny - b->dcenter_y) * (py - ny - b->dcenter_y)) {
            nx = -nx;
            ny = -ny;
        }
    } else {
        if ((px + nx - a->dcenter_x) * (px + nx - a->dcenter_x) +
                (py + ny - a->dcenter_y) * (py + ny - a->dcenter_y) >
            (px - nx - a->dcenter_x) * (px - nx - a->dcenter_x) +
                (py - ny - a->dcenter_y) * (py - ny - a->dcenter_y)) {
            nx = -nx;
            ny = -ny;
        }
    }
    double j = (-(1 + P.elasticity) * (vab1x * nx + vab1y * ny)) /
               (1 / a->mass + 1 / b->mass + bio_pow(rapx * ny - rapy * nx, 2) / a->inertia +
                bio_pow(rbpx * ny - rbpy * nx, 2) / b->inertia);
    a->dx = bio_between(a->dx + j * nx / a->mass, -P.max_vel, P.max_vel);
    a->dy = bio_between(a->dy + j * ny / a->mass, -P.max_vel, P.max_vel);
    b->dx = bio_between(b->dx - j * nx / b->mass, -P.max_vel, P.max_vel);
    b->dy = bio_between(b->dy - j * ny / b->mass, -P.max_vel, P.max_vel);
    a->dtheta = bio_between(a->dtheta + j * (rapx * ny - rapy * nx) / a->inertia, -P.max_rot,
                            P.max_rot);
    /* "rbpy * ny" (not nx) is the original's typo; kept for fidelity. */
    b->dtheta = bio_between(b->dtheta - j * (rbpx * ny - rbpy * ny) / b->inertia, -P.max_rot,
                            P.max_rot);
}

static void org_touch_effects(Organism *a, Organism *b, int seg, int oseg, bool first_call)
{
    if ((a->parent_id == b->id || a->id == b->parent_id) && b->alive)
        return;
    double taken = 0;
    switch (a->seg_color[seg]) {
    case COL_RED:
        switch (b->seg_color[oseg]) {
        case COL_BLUE:
            if (org_use_energy(b, P.blue_energy_consumption)) {
                org_set_color(b, COL_BLUE);
            } else if (org_use_energy(a, P.red_energy_consumption)) {
                taken = bio_between(a->m[seg] * P.organic_obtained_energy, 0, b->energy);
                org_set_color(b, COL_YELLOW);
            }
            break;
        case COL_RED:
            if (org_use_energy(a, P.red_energy_consumption)) {
                taken = bio_between(a->m[seg] * P.organic_obtained_energy, 0, b->energy);
                org_set_color(b, COL_RED);
            }
            break;
        default:
            if (org_use_energy(a, P.red_energy_consumption)) {
                taken = bio_between(a->m[seg] * P.organic_obtained_energy, 0, b->energy);
                org_set_color(b, COL_YELLOW);
            }
        }
        b->energy -= taken;
        a->energy += taken;
        org_use_energy(a, taken * P.organic_subs_produced);
        org_set_color(a, COL_RED);
        break;
    case COL_WHITE:
        switch (b->seg_color[oseg]) {
        case COL_BLUE:
            if (org_use_energy(b, P.blue_energy_consumption)) {
                org_set_color(a, COL_WHITE);
                org_set_color(b, COL_BLUE);
            } else if (b->infected_gc != a->gc) {
                if (org_use_energy(a, P.white_energy_consumption)) {
                    org_infected_by(b, a);
                    org_set_color(b, COL_YELLOW);
                    org_set_color(a, COL_WHITE);
                }
            }
            break;
        case COL_BROWN:
            break;
        default:
            if (b->infected_gc != a->gc) {
                if (org_use_energy(a, P.white_energy_consumption)) {
                    org_infected_by(b, a);
                    org_set_color(b, COL_YELLOW);
                    org_set_color(a, COL_WHITE);
                }
            }
        }
        break;
    case COL_GRAY:
        switch (b->seg_color[oseg]) {
        case COL_BLUE:
            if (org_use_energy(b, P.blue_energy_consumption)) {
                org_set_color(b, COL_BLUE);
                org_set_color(a, COL_GRAY);
            } else if (org_use_energy(a, P.gray_energy_consumption)) {
                org_die(b, a);
                org_set_color(a, COL_GRAY);
            }
            break;
        case COL_BROWN:
            break;
        default:
            if (org_use_energy(a, P.gray_energy_consumption)) {
                org_die(b, a);
                org_set_color(a, COL_GRAY);
            }
        }
        break;
    }
    if (b->alive && b->energy < BIO_TOL)
        org_die(b, a);
    if (first_call)
        org_touch_effects(b, a, oseg, seg, false);
}

static double dist_sq(double ax, double ay, double bx, double by)
{
    double px = bx - ax, py = by - ay;
    return px * px + py * py;
}

bool org_contact(Organism *o, Organism *org)
{
    JLine line, bline;
    /* "Does org's segment j cross my bounding box" does not depend on i;
     * the original asked it again for every i. Memoised per call:
     * 0 unknown, 1 yes, 2 no. */
    uint8_t stackmemo[512];
    uint8_t *memo = org->segments <= 512 ? stackmemo : calloc((size_t)org->segments, 1);
    if (!memo)
        abort();
    if (memo == stackmemo)
        memset(memo, 0, (size_t)org->segments);
    bool touched = false;
    for (int i = o->segments - 1; i >= 0 && !touched; i--) {
        if (!(o->m[i] >= 1))
            continue;
        line.x1 = o->x1[i] + o->center_x;
        line.y1 = o->y1[i] + o->center_y;
        line.x2 = o->x2[i] + o->center_x;
        line.y2 = o->y2[i] + o->center_y;
        if (!jrect_intersects_line(&org->r, &line))
            continue;
        for (int j = org->segments - 1; j >= 0; j--) {
            if (!(org->m[j] >= 1))
                continue;
            bline.x1 = org->x1[j] + org->center_x;
            bline.y1 = org->y1[j] + org->center_y;
            bline.x2 = org->x2[j] + org->center_x;
            bline.y2 = org->y2[j] + org->center_y;
            if (!memo[j])
                memo[j] = jrect_intersects_line(&o->r, &bline) ? 1 : 2;
            if (memo[j] == 1 && jline_intersects_line(&line, &bline)) {
                org_touch_effects(o, org, i, j, true);
                double ix, iy;
                jline_intersection(&line, &bline, BIO_TOL, &ix, &iy);
                double dl1 = dist_sq(ix, iy, line.x1, line.y1);
                double dl2 = dist_sq(ix, iy, line.x2, line.y2);
                double dbl1 = dist_sq(ix, iy, bline.x1, bline.y1);
                double dbl2 = dist_sq(ix, iy, bline.x2, bline.y2);
                if (jmin(dl1, dl2) < jmin(dbl1, dbl2))
                    org_touch_move(o, org, ix, iy, &bline, false);
                else
                    org_touch_move(o, org, ix, iy, &line, true);
                touched = true;
                break;
            }
        }
    }
    if (memo != stackmemo)
        free(memo);
    return touched;
}
