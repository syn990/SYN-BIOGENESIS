/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Translated to C in 2026 from World.java
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

/* World.java: the organisms, the atmosphere (O2/CO2) and the frame loop.
 *
 * Collision candidates come from a uniform grid rebuilt every frame from
 * each organism's frame-start bounding box. 0.9 used a JTS STRtree built the
 * same way; the grid returns the same candidate set and visits it in list
 * order (0.8's order) so results do not depend on tree layout.
 */
#ifndef SIM_WORLD_H
#define SIM_WORLD_H

#include <stdbool.h>
#include <stdint.h>

#include "sim/organism.h"
#include "sim/stats.h"

typedef struct World {
    int width, height;
    Organism **orgs; /* every organism, dead ones included, in Java list order */
    int norgs, cap;
    int population;
    int next_id;
    int nframes; /* frames into the current time unit (256 frames) */
    double o2, co2;
    WorldStats stats;

    /* collision grid, rebuilt in world_time */
    int grid_w, grid_h, cell;
    int *cell_start; /* grid_w*grid_h+1 offsets into cell_items */
    int *cell_items;
    int cell_items_cap;
    Organism **snapshot; /* frame-start list, indexed by Organism.tree_idx */
    int snapshot_n, snapshot_cap;
    bool grid_valid;
    unsigned stamp;
    Organism **cand;
    int cand_cap;

    /* Called just before an organism is freed (out of energy, or the world
     * is cleared) so the UI can drop pointers to it. */
    void (*on_remove)(void *ud, Organism *o);
    void *ud;
} World;

void world_init(World *w);
void world_free(World *w);
/* Remove and free every organism; keeps size and atmosphere. */
void world_clear(World *w);
/* World.genesis: a fresh world from the current parameters. */
void world_genesis(World *w);
/* World.time: one frame. */
void world_time(World *w);

int world_new_id(World *w);
int64_t world_time_units(const World *w);
int world_n_corpses(const World *w);

void world_increase_population(World *w);
void world_decrease_population(World *w);
void world_add_o2(World *w, double q);
void world_add_co2(World *w, double q);
void world_decrease_o2(World *w, double q);
void world_decrease_co2(World *w, double q);
double world_respiration(World *w, double q);
double world_photosynthesis(World *w, double q);

void world_disperse_all(World *w);
void world_kill_all(World *w);

/* Bounding-box overlap with any other organism (placement test). */
Organism *world_fast_check_hit(World *w, Organism *o);
/* Touch test against the frame's collision index; applies the effects. */
Organism *world_check_hit(World *w, Organism *o);
Organism *world_find_organism_at(World *w, int x, int y);

void world_add_organism(World *w, Organism *child, Organism *parent);
void world_organism_has_died(World *w, Organism *dying, Organism *killer);
void world_organism_has_been_infected(World *w, Organism *infected, Organism *infector);

/* Append an already-initialised organism (used by the loader). */
void world_append_raw(World *w, Organism *o);

#endif
