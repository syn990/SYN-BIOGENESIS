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
 * Collision candidates come from a JTS STRtree rebuilt every frame from each
 * organism's frame-start bounding box, visited in the tree's order, as 0.9
 * does (see world.c).
 */
#ifndef SIM_WORLD_H
#define SIM_WORLD_H

#include <stdbool.h>
#include <stdint.h>

#include "sim/organism.h"
#include "sim/stats.h"

/* A node of the collision tree (JTS AbstractNode / STRtreeNode) */
typedef struct StrNode {
    double env[4];   /* minx, maxx, miny, maxy */
    int kid0, nkids; /* children: World.str_kids[kid0 .. kid0+nkids) */
    int level;       /* 0 for the nodes that hold organisms */
    bool has_bounds; /* JTS computes a node's bounds lazily; saved worlds show it */
} StrNode;

typedef struct World {
    int width, height;
    Organism **orgs; /* every organism, dead ones included, in Java list order */
    int norgs, cap;
    int population;
    int next_id;
    int nframes; /* frames into the current time unit (256 frames) */
    double o2, co2;
    WorldStats stats;

    /* collision index (JTS STRtree), rebuilt in world_time */
    Organism **snapshot; /* frame-start list: the tree's items */
    int snapshot_n, snapshot_cap;
    StrNode *str_nodes;
    int str_nnodes, str_nodes_cap;
    int *str_kids; /* every node's children: item index >= 0, node ~index < 0 */
    int str_nkids, str_kids_cap;
    int str_root;  /* ~index of the root node, 0 for an empty tree */
    struct StrKey *str_keys; /* sort scratch, and three ref lists */
    int *str_a, *str_b, *str_x;
    int str_scratch_cap;
    bool tree_valid;
    bool str_built; /* queried this frame: JTS builds the tree on the first query */
    Organism **cand;
    int cand_cap;
    /* Organisms removed in the last frame. The tree still holds them (and a
     * saved world with it), so they are freed when the next frame starts. */
    Organism **grave;
    int ngrave, grave_cap;

    /* Called when an organism leaves the world (out of energy, or the world
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
