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

/* Create a winner: a being worked out for a world as it is, the moment it
 * is asked for, whose bloodline will be all that lives in it.
 *
 * Nothing in a world is left to chance once its state and the random
 * generator's are known. The simulation goes the same way every time, in
 * the window or without one, and a world saved and opened again with the
 * generator's state goes on exactly as the one left running (both
 * simulations, checked to the hash). So what a code would do in the world
 * is not a probability but a fact, read by running it, and the search is
 * that reading done many times over on every core:
 *
 *   - the world saved as it is, with its parameters and the generator's
 *     state;
 *   - candidates from what is known: the world's commonest codes (the
 *     bloodlines already winning in it), the simulation's own mutations of
 *     them, the being library, and random codes (0.9: the likeliest of
 *     many by the plant fitness of syn/crack.h at the world's CO2);
 *   - places: the one asked for, and the most open places in the world,
 *     far from every organism;
 *   - each candidate pasted at each place as the window pastes a code, and
 *     the world run on from there by a worker (this program again), a few
 *     time units first, then the best third longer, and so on; a run ends
 *     as soon as the candidate's bloodline dies out or is all that lives;
 *   - while time is left and none is all that lives, the best mutated and
 *     run again.
 *
 * The winner is the candidate whose bloodline is soonest all that lives
 * and is still alive when the longest runs end (the last organism of a
 * dying world is not a winner), else the one with the largest share of
 * the living after the longest run. With it comes the world's state hashed at that time unit, so the
 * window can tell, when the time comes, whether the world went as
 * calculated: it does unless something else is done to it meanwhile.
 *
 * Its bloodline: 0.9, every organism whose code descends from the pasted
 * one (gc->lineage, which a mutated child and an infected parent's child
 * keep); Color Mod, every organism whose clade began with it (the pasted
 * organism's number, at the head of its clade ID). */
#ifndef SYN_WINNER_H
#define SYN_WINNER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sim/world.h"

/* The world as the simulation goes on from it, hashed: its gases, the
 * organisms' places, energy and lives, and the random generator's state. */
uint64_t syn_winner_hash(const World *w);

/* ---- in the window ---- */

/* The search begun for w as it is now, the winner asked for at (x, y):
 * the world, the parameters and the generator's state saved in the
 * configuration folder, and this program run again to search them for at
 * most seconds. */
bool syn_winner_start(const World *w, int x, int y, int seconds, char *err, size_t n);
bool syn_winner_running(void);
/* The search stopped, its result dropped. */
void syn_winner_cancel(void);
/* What the search is doing, a line for the hints; NULL when none runs. */
const char *syn_winner_status(void);
/* The world searched, hashed when the search began. */
uint64_t syn_winner_start_hash(void);

typedef struct {
    bool found;      /* a candidate whose bloodline lived */
    bool owns;       /* and is all that lives from time t, still alive at end_t */
    int x, y;        /* where it goes */
    int64_t t;       /* that time unit; or the end of its longest run */
    int mine, alive; /* its bloodline then, and all the living */
    uint64_t hash;   /* the world then, hashed */
    int64_t end_t;   /* the end of its run, and its bloodline then */
    int end_n;
    char bgg[1100];  /* its code */
    char label[200]; /* what it is and where it came from */
    char why[200];   /* found false: why */
} WinnerResult;

/* true once the search has ended, with what it found */
bool syn_winner_poll(WinnerResult *r);
/* Lines the search printed since last asked, one at a time, for the
 * console; NULL when there are none. */
const char *syn_winner_line(void);

/* The winner was pasted: at time r->t the world is compared with what
 * was calculated. */
void syn_winner_expect(const World *w, const WinnerResult *r);
/* Every time unit (syn_history_frame): the comparison, when its time has
 * come. */
void syn_winner_unit(const World *w);
/* What the comparison found, once; NULL meanwhile. */
const char *syn_winner_verdict(void);

/* ---- this program again ---- */

/* syn-biogenesis [--colour] --winner WORLD [options] */
int syn_winner_main(int argc, char **argv);
/* syn-biogenesis --winner-worker SIM WORLD PARAMS STATE (from syn_winner_main) */
int syn_winner_worker(int argc, char **argv);

#endif
