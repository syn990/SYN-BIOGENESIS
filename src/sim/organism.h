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

/* Organism.java: a body of coloured segments that moves, eats, breathes,
 * grows, collides and reproduces.
 *
 * Field names follow the Java ones so the two can be read side by side.
 * The JRect `r` is the java.awt.Rectangle the Java class extended: the
 * organism's bounding box in world coordinates.
 */
#ifndef SIM_ORGANISM_H
#define SIM_ORGANISM_H

#include <stdbool.h>
#include <stdint.h>

#include "sim/genetic_code.h"
#include "sim/jgeom.h"

struct World;

typedef struct Organism {
    JRect r;
    GeneticCode *gc;
    GeneticCode *infected_gc; /* code this organism will reproduce if infected */
    int n_children;
    struct World *world;
    int parent_id;
    int id;
    int generation;
    int infected_generation;
    int total_children;
    int total_kills;
    int total_infected;

    int segments;
    int *start_x, *start_y, *end_x, *end_y; /* body in its own frame */
    double *m1, *m2, *m;                    /* |start|, |end|, segment length */
    double *a1, *a2;                        /* atan2 of start and end (cached) */
    int *x1, *y1, *x2, *y2;                 /* rotated segments, relative to centre */
    uint8_t *seg_color;

    int center_x, center_y;
    double dcenter_x, dcenter_y;
    int growth_ratio; /* 16 = newborn ... 1 = adult */
    double mass;
    double inertia;
    double energy;
    JRect size_rect;
    double theta;
    double last_theta;
    double dx, dy, dtheta;
    int age;
    uint8_t flash_color; /* drawn instead of segment colours while frames_color > 0 */
    int frames_color;
    int time_to_reproduce;
    int has_grown;
    bool has_moved;
    bool alive;

    /* world bookkeeping, not part of the Java organism */
    bool removed;    /* out of energy this frame; freed when the frame ends */
    int tree_idx;    /* position in the collision index built at frame start */
    unsigned stamp;  /* de-duplicates collision index queries */
    double env[4];   /* frame-start envelope: minx, maxx, miny, maxy */
} Organism;

Organism *org_new(struct World *w);
Organism *org_new_with_code(struct World *w, GeneticCode *gc);
void org_free(Organism *o);
/* Organism.create: allocate the per-segment arrays for o->gc. */
void org_create(Organism *o);

bool org_random_create(Organism *o);
bool org_inherit(Organism *o, Organism *parent, bool first);
bool org_paste(Organism *o, int x, int y);
void org_symmetric(Organism *o);
/* Recompute the cached point angles (after loading a saved body). */
void org_prepare_angles(Organism *o);
void org_calculate_bounds(Organism *o, bool force);
void org_reproduce(Organism *o);
bool org_move(Organism *o);
bool org_use_energy(Organism *o, double q);
void org_breath(Organism *o);
void org_die(Organism *o, Organism *killer);
void org_infected_by(Organism *o, Organism *infector);
bool org_contact(Organism *o, Organism *other);
/* VisibleWorld's Revive action. */
void org_revive(Organism *o);

#endif
