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

/* The seed-cracking engine: from a being alone, its genesis ancestor, every
 * seed that ancestor was a founder in, the being's mutations in the order
 * they were drawn, and how likely each of those seeds is to end with the
 * ancestor's bloodline alone.
 *
 * Three facts of Biogenesis 0.9 (docs/crack) carry it:
 * - A founder is a fixed run of draws: org_new's angle (2), the mirroring,
 *   the arms and the genes' number (1 each), then per gene its length (2),
 *   angle (2) and colour (1), then the dispersal (1). So one of its gene
 *   lengths, cracked to the state that drew it (telescope.h), steps back to
 *   the state the founder was begun at, and drawing again from there gives
 *   the whole founder. Its other genes, wherever they still are in the
 *   being, prove it.
 * - Genesis is founders made one after another from one stream, so seeds a
 *   few draws apart soon make the same founders from the same states (the
 *   Kruskal count). The seeds whose genesis begins a founder at a given
 *   state are all within a few hundred draws before it per founder slot,
 *   and trying each finds them all.
 * - Which bloodline is left (the lone bloodline, by about time unit 100)
 *   follows how fast each founder turns light into children: a plant's
 *   rate from its code alone, fitted against hundreds of replayed worlds.
 *
 * Only Biogenesis 0.9. Nothing here changes the simulation: it draws codes
 * as gc_new_random does, makes worlds as world_genesis does, and reads them.
 */
#ifndef SYN_CRACK_H
#define SYN_CRACK_H

#include <stdbool.h>
#include <stdint.h>

struct GeneticCode;
struct World;

/* gc_new_random gives a founder 1 to 8 genes */
#define CRACK_FOUNDER_GENES 8

/* A founder the being comes from, drawn again. */
typedef struct {
    uint64_t start;          /* the generator's state when org_new began it */
    struct GeneticCode *gc;  /* its code, drawn again from start */
    int gene;                /* its gene the cracked one was (from 0) */
    int proof;               /* the being's other draws in it: lengths and angles, 53 bits each */
    int kept;                /* the being's genes it has whole (length, angle, colour) */
    int changes;             /* the fewest mutations from it to the being */
} CrackAncestor;

/* Its genesis ancestors, the likeliest first: the founders that some gene
 * of the being points back to, with at least one more of its draws to
 * prove each. A founder's genes are blocks of five draws and so is what
 * goes before them, so the same run of genes reads as founders begun a
 * gene apart, each the founder of some seed; the one the being is the
 * fewest mutations from is first. */
int crack_ancestors(const struct GeneticCode *being, CrackAncestor *out, int max);
void crack_ancestors_free(CrackAncestor *a, int n);
/* The fewest mutations that make one code from another: genes added,
 * removed, or with their length, angle or colour drawn again (in order, as
 * gc_new_mutated keeps them), and the arms, mirroring and dispersal. */
int crack_changes(const struct GeneticCode *from, const struct GeneticCode *to);
/* The founder org_new would begin at this state, under P's colours. */
struct GeneticCode *crack_founder_at(uint64_t start);

/* A gene of the being drawn after its ancestor was: its length (a gene
 * added, or its length drawn again) or its angle alone. */
typedef struct {
    int gene;       /* the being's gene (from 0) */
    bool angle;     /* the draw is its angle (its length is the ancestor's) */
    bool whole;     /* its angle drawn straight after its length: a gene added */
    uint64_t state; /* the state that made the draw (0: a number the generator never makes) */
    uint64_t bits;  /* the length or angle drawn, as the double's bits */
    int64_t place;  /* draws after its ancestor was begun, or with none after the being's
                     * earliest draw (-1: not in that stream) */
} CrackDraw;

/* The being's draws its ancestor hasn't, each placed in the ancestor's
 * stream, the soonest first. With no ancestor (NULL), all its genes,
 * placed after the earliest of them. */
int crack_draws(const struct GeneticCode *being, const CrackAncestor *a, CrackDraw *out, int max);

/* How fast a code grows as a plant, children per frame on a log scale: its
 * green segments' light at this CO2 less its breathing, into its
 * reproduction energy, over the time it takes to grow; 0 if it can't feed
 * itself. */
double crack_fitness(const struct GeneticCode *gc, double co2);
/* The chance founder i of n is the bloodline left, from their fitness. */
double crack_win_chance(const double *fit, int n, int i);

/* A seed whose genesis begins the ancestor. */
typedef struct {
    uint64_t state; /* its first state */
    int64_t seed;   /* in [0, 2^48) */
    int slot;       /* the ancestor's place among its founders (from 0) */
    int org;        /* its index in the world's organisms (some founders find no place) */
    int founders;
    double fit;     /* the ancestor's fitness, and the best of the others' */
    double rival;
    double chance;  /* that the ancestor's bloodline is the one left */
} CrackSeed;

/* Every seed whose genesis, under P, begins a founder at `start` that
 * finds a place in its world, the likeliest to end with its bloodline
 * first. back: how many draws before it to look (0: as far as P's
 * founders reach). Returns how many, of which at most max are written
 * (none for NULL: only counted). */
int crack_basin(uint64_t start, int64_t back, CrackSeed *out, int max);

/* syn-biogenesis --telescope crack|forge ... */
int crack_main(int argc, char **argv);

#endif
