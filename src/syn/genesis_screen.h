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

/* "Creating world": a new world made in the window (syn/genesis.h), for
 * either simulation and either interface. A world that takes more than a
 * moment is shown being made, with how far it has got, in the SYN-OS
 * theme or the Heritage colours, and the window keeps answering: Esc
 * starts the world with the organisms made so far, and closing the window
 * stops it to quit. Screenshots and scripted runs make it unseen, to the
 * end. */
#ifndef SYN_GENESIS_SCREEN_H
#define SYN_GENESIS_SCREEN_H

#include <SDL.h>
#include <stddef.h>

#include "syn/genesis.h"

typedef enum {
    SYN_GENESIS_DONE,    /* every organism tried, as world_genesis */
    SYN_GENESIS_STOPPED, /* Esc: the world has the organisms made so far */
    SYN_GENESIS_QUIT     /* the window was closed: stopped, to quit */
} SynGenesisEnd;

SynGenesisEnd syn_genesis_run(SynGenesis *g, World *w, SDL_Renderer *r);
/* The status line for a world stopped part way. */
void syn_genesis_stopped_text(const SynGenesis *g, char *out, size_t n);

/* For the parameters dialog, under its initial number of organisms (the
 * field's text): how long a new world of that many takes to make, when
 * that is more than a few seconds, and how many make one quickly. Draws
 * it at x, y when draw is set; returns the height it takes, 0 for none. */
int syn_genesis_warning(int x, int y, const char *initial_organisms, bool draw);

#endif
