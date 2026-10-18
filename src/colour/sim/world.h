/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Translated to C in 2026 from World.java of Biogenesis Color Mod 2.01
 * (https://sourceforge.net/projects/biogenesis/),
 * Copyright (C) 2006-2010 Joan Queralt Molina,
 * Color Mod Copyright (C) 2012-2022 MarcoDBAA.
 *
 * The collision index is translated from JTS 1.15.0's STRtree
 * (https://github.com/locationtech/jts), Copyright (c) 2016 Vivid
 * Solutions, under the Eclipse Distribution License v1.0: see LICENSE.JTS.
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

/* World.java of Color Mod: the organisms, the atmosphere (O2, CO2 and
 * methane) and the frame loop.
 *
 * As in 0.9, collision candidates come from a JTS STRtree built every frame
 * from each organism's frame-start bounding box (see world.c). Color Mod
 * also places children with it (fastCheckHit), so organisms that ran out
 * of energy earlier in the frame still stand in the way until it ends.
 */
#ifndef COLOUR_WORLD_H
#define COLOUR_WORLD_H

#include <stdbool.h>
#include <stdint.h>

#include "sim/organism.h"
#include "sim/stats.h"

/* A node of the collision tree (JTS AbstractNode / STRtreeNode) */
typedef struct StrNode {
    double env[4];   /* minx, maxx, miny, maxy */
    int kid0, nkids; /* children: World.str_kids[kid0 .. kid0+nkids) */
    int level;       /* 0 for the nodes that hold organisms */
    bool has_bounds;
} StrNode;

typedef struct World {
    int width, height;
    Organism **orgs; /* every organism, dead ones included, in Java list order */
    int norgs, cap;
    int population;
    int next_id;
    int nframes; /* frames into the current time unit (256 frames) */
    double o2, co2, ch4;
    bool corridor_exists; /* network corridors are not ported: always false */
    void *visible_world;  /* Java's VisibleWorld; unused here */
    WorldStats stats;

    /* collision index (JTS STRtree), rebuilt in world_time */
    Organism **snapshot; /* frame-start list: the tree's items */
    int snapshot_n, snapshot_cap;
    StrNode *str_nodes;
    int str_nnodes, str_nodes_cap;
    int *str_kids; /* every node's children: item index >= 0, node ~index < 0 */
    int str_nkids, str_kids_cap;
    int str_root; /* ~index of the root node, 0 for an empty tree */
    struct StrKey *str_keys;
    int *str_a, *str_b, *str_x;
    int str_scratch_cap;
    bool tree_valid;
    bool str_built;
    Organism **cand;
    int cand_cap;
    /* Organisms removed in the last frame. The tree still holds them, so
     * they are freed when the next frame starts. */
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
/* Free the genetic codes and genes nothing refers to any more: the world's
 * organisms and statistics, and the roots given to jgc_add_root. Call
 * between frames, never while C code holds a code in a local. */
void world_collect(World *w);

int world_get_width(const World *w);
int world_get_height(const World *w);
int world_get_new_id(World *w);
int64_t world_get_time(const World *w);
int64_t world_get_frame(const World *w);
int world_get_ncorpses(const World *w);
int world_get_population(const World *w);
double world_get_o2(const World *w);
double world_get_co2(const World *w);
double world_get_ch4(const World *w);

void world_increase_population(World *w);
void world_decrease_population(World *w);
void world_add_o2(World *w, double q);
void world_add_co2(World *w, double q);
void world_add_ch4(World *w, double q);
void world_decrease_o2(World *w, double q);
void world_decrease_co2(World *w, double q);
void world_decrease_ch4(World *w, double q);
double world_respiration(World *w, double q);
double world_decomposition(World *w, double q);
double world_photosynthesis(World *w, double q);
double world_methanotrophy(World *w, double q);

void world_disperse_all(World *w);
void world_kill_all(World *w);
Organism *world_find_organism_from_position(World *w, int x, int y);

/* Bounding-box overlap with any organism in the list (genesis, pasting). */
Organism *world_genesis_check_hit(World *w, Organism *b1);
/* Bounding-box overlap with any organism in the frame's collision index. */
Organism *world_fast_check_hit(World *w, Organism *b1);
/* The same, ignoring b1's parent and children. */
Organism *world_transform_check_hit(World *w, Organism *b1);
/* Touch test against the frame's collision index; applies the effects. */
Organism *world_check_hit(World *w, Organism *org1);
OutCorridor *world_check_hit_corridor(World *w, Organism *org);

void world_add_organism(World *w, Organism *child, Organism *parent);
void world_organism_has_died(World *w, Organism *dying, Organism *killer);
void world_organism_has_been_infected(World *w, Organism *infected, Organism *infector);

/* Append an already-initialised organism (used by the loader). */
void world_append_raw(World *w, Organism *o);

#endif
