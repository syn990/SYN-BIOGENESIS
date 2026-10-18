/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Part of the SYN edition of SYN-BIOGENESIS, a C port of Biogenesis
 * (https://biogenesis.sourceforge.net/).
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

/* The SYN edition's family tree of a world: its species and which species
 * each one came from, recorded beside the simulation as the history is
 * (syn/history.h), for the statistics screen's FAMILY tab.
 *
 * Biogenesis 0.9: a species is a body plan, the arms, the mirroring and
 * the colours of the genes in order. Lengths and angles mutate within it;
 * a birth that changes the plan founds a new species, a child of the one
 * its code was copied from. gc->lineage carries the species: the
 * simulation copies it from parent to mutated child, and from the
 * infecting code to an infected parent's child, and reads it nowhere (the
 * same mark tools/being uses). The lines of the history are the roots of
 * these trees.
 *
 * Color Mod 2.01: a species is one of its clades. Its clade ID is already
 * a family path (the founder's number, then +N -N >N <N where the gene
 * count or the symmetry changed, N the organism), so the tree is read
 * from it, even across saved worlds, and nothing is written.
 *
 * Species that never mattered are forgotten once they die out: those that
 * never had FAM_NOTABLE living at once (and as much as one in 50 of the
 * living), with nothing kept descending from them, best of no epoch. */
#ifndef SYN_FAMILY_H
#define SYN_FAMILY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sim/world.h"

#define FAM_NOTABLE 5

typedef struct {
    uint32_t id;     /* 0.9: the mark in gc->lineage */
    uint32_t parent; /* 0: a root (genesis, the world as opened, a pasted or new code) */
    uint32_t root;
    int depth;       /* steps from its root */
    int founder;     /* the organism it began with */
    int64_t born, last_seen, peak_t; /* time units */
    int alive, peak;
    int64_t members; /* organisms born into it (or found in it) */
    int kids;        /* species kept that came from it */
    int pins;        /* epochs it is the best of */
    int carriers;    /* 0.9: living organisms infected with its code */
    bool notable, extinct;
    uint64_t key;    /* its plan (0.9) or its clade (Color Mod), hashed */
    uint8_t colour;  /* its founder's commonest palette slot */
    GeneticCode *gc; /* its founder's code (held); NULL for a Color Mod clade never seen alive */
} Species;

/* With the history's (syn_history_reset). */
void syn_family_reset(World *w);
/* After every simulation frame: the newborn. */
void syn_family_frame(World *w);
/* Once a time unit, before the history reads the world: the living
 * counted, the species that never mattered forgotten. */
void syn_family_unit(World *w, int64_t t);
void syn_family_free(void);

/* The species an organism belongs to (0: none yet). */
uint32_t syn_family_of(const Organism *o);
/* NULL if unknown or forgotten. Pointers last until the next frame. */
const Species *syn_family_get(uint32_t id);
/* All of them, parents before their children. */
int syn_family_count(void);
const Species *syn_family_at(int i);
/* Its place among them, or -1. */
int syn_family_index(uint32_t id);
/* The species with the most living now, and how many (0: none). */
uint32_t syn_family_top(int *alive);
/* Pin (+1) or unpin (-1): a pinned species is never forgotten. */
void syn_family_pin(uint32_t id, int d);
/* Changes with every species founded, forgotten or counted. */
unsigned syn_family_version(void);

/* What changed from its parent's plan ("6 to 7 arms, + blue segment
 * (8th)"); "" for a root. */
void syn_family_change(const Species *s, char *out, size_t n);
/* Its plan in words ("7 arms; 8 segments per arm: 5 green, yellow, 2 blue"). */
void syn_family_plan(const GeneticCode *gc, char *out, size_t n);

#endif
