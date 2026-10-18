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

/* The colours of segments as the SYN edition's looks, minimap and
 * statistics see them, for either simulation: a slot per colour, with its
 * RGB, its name and its job.
 *
 * Biogenesis 0.9: the eight BioColors, slot = colour. Color Mod 2.01: its
 * 46 gene colours, then the colours its segments turn for a while
 * (frozen, broken, withered, recharging...), then one for any other
 * colour, then corpses. Slots are in order of job, so stacked charts read
 * as families. */
#ifndef SYN_PALETTE_H
#define SYN_PALETTE_H

#include <stdbool.h>
#include <stdint.h>

#include "sim/world.h"

typedef enum {
    ROLE_PHOTO,    /* makes energy from light (or methane) */
    ROLE_EAT,      /* eats, decomposes, parasitises */
    ROLE_KILL,     /* kills, poisons, weakens */
    ROLE_DEFEND,   /* shields, freezes, pushes */
    ROLE_INFECT,   /* infects with its own code */
    ROLE_HEAL,     /* heals, immunises, shares (Color Mod) */
    ROLE_MOVE,     /* propels the organism */
    ROLE_REPRO,    /* more children, sooner or later */
    ROLE_SENSE,    /* eyes (Color Mod) */
    ROLE_OTHER,    /* everything else */
    ROLE_INACTIVE, /* out of action for now: frozen, broken... (Color Mod) */
    ROLE_DEAD,
    ROLE_COUNT
} Role;

#ifdef BIO_COLOUR
#define SYN_NCOL 66 /* 46 gene colours, 18 states, other, corpse */
#else
#define SYN_NCOL COL_COUNT
#endif
/* The slot corpses are drawn in; the others are of the living. */
#define SYN_DEAD (SYN_NCOL - 1)
#define SYN_LIVING (SYN_NCOL - 1)

/* The slot of a segment colour as the simulation keeps it. */
int syn_col(uint32_t colour);
void syn_col_rgb(int slot, uint8_t *r, uint8_t *g, uint8_t *b);
const char *syn_col_name(int slot);
Role syn_col_role(int slot);

/* The slot segment i is drawn in: corpses' are SYN_DEAD (Color Mod's eyes
 * excepted, as its Organism.draw has them). */
int syn_seg(const Organism *o, int i);
/* The slot of the colour the organism flashes in, or -1 when it isn't. */
int syn_flash(const Organism *o);
/* 1 at the start of a flash, down to 0 as it ends */
float syn_flash_left(const Organism *o);

/* Read the same way from either simulation's code */
int syn_genes(const GeneticCode *gc);
/* The palette slot of gene i's colour. */
int syn_gene_col(const GeneticCode *gc, int i);
int syn_generation(const Organism *o);
double syn_reproduce_energy(const Organism *o);
/* What gene lengths are multiplied by at the organism's growth: 0.9
 * divides by its scale table, Color Mod multiplies by its own. */
double syn_growth_scale(const Organism *o);
/* The line an organism descends from: 0.9's the root of its species'
 * tree (syn/family.h); Color Mod's clade ID starts with its founder's
 * number, its own. */
uint32_t syn_line_key(const Organism *o);

#endif
