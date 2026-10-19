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

#include "syn/genesis.h"

#include <math.h>
#include <stddef.h>

#include "sim/organism.h"
#include "sim/params.h"
#include "sim/stats.h"

/* world_genesis (sim/world.c, src/colour/sim/world.c) up to its loop */
void syn_genesis_begin(SynGenesis *g, World *w)
{
    world_clear(w);
    w->nframes = 0;
    w->o2 = P.initial_o2;
    w->co2 = P.initial_co2;
#ifdef BIO_COLOUR
    w->ch4 = P.initial_ch4;
#endif
    w->next_id = 0;
    w->population = 0;
    w->width = P.world_width;
    w->height = P.world_height;
    stats_release(&w->stats);
    stats_init(&w->stats);
    g->w = w;
    g->made = 0;
    g->total = P.initial_organisms;
}

/* world_genesis timed without a window (--headless --frames 0), on one
 * core of the desktop this was written on: organisms, seconds */
static const double k_times[][2] = {
#ifdef BIO_COLOUR
    {2000, 0.046}, {12500, 1.07}, {25000, 5.45}, {50000, 35.4},
#else
    {2000, 0.035}, {5000, 0.16}, {10000, 0.63}, {20000, 2.8}, {50000, 35},
#endif
};
#define N_TIMES ((int)(sizeof k_times / sizeof k_times[0]))

double syn_genesis_seconds(double n)
{
    if (n <= 0)
        return 0;
    if (n <= k_times[0][0])
        return k_times[0][1] * (n / k_times[0][0]) * (n / k_times[0][0]);
    /* between two timings on a log-log line, past the last on the last */
    int i = 1;
    while (i < N_TIMES - 1 && n > k_times[i][0])
        i++;
    double slope = log(k_times[i][1] / k_times[i - 1][1]) / log(k_times[i][0] / k_times[i - 1][0]);
    return k_times[i - 1][1] * pow(n / k_times[i - 1][0], slope);
}

/* one turn of its loop */
bool syn_genesis_next(SynGenesis *g)
{
    if (g->made >= g->total)
        return false;
    Organism *b = org_new(g->w);
    if (org_random_create(b))
        world_add_organism(g->w, b, NULL);
    else
        org_free(b);
    g->made++;
    return g->made < g->total;
}
