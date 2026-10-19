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

/* A new world made an organism at a time, so the window can show it being
 * made and stop it part way (syn/genesis_screen.h). It makes the calls
 * world_genesis makes, in its order, for either simulation, so a world
 * made to the end is the one world_genesis makes: tests/history_check
 * runs one against the other. */
#ifndef SYN_GENESIS_H
#define SYN_GENESIS_H

#include <stdbool.h>

#include "sim/world.h"

typedef struct {
    World *w;
    int made;  /* organisms tried, placed or not */
    int total; /* the parameters' initial organisms */
} SynGenesis;

/* Empties the world and sets it up as world_genesis does. */
void syn_genesis_begin(SynGenesis *g, World *w);
/* Tries the next organism; false once they have all been tried. */
bool syn_genesis_next(SynGenesis *g);

/* About how many seconds making n organisms takes, as measured here. It
 * grows faster than the square of n: each organism is placed where it
 * touches none of those placed before it, checked one by one. */
double syn_genesis_seconds(double n);

#endif
