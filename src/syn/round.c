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

#include "syn/round.h"

#include <math.h>
#include <string.h>

#include "sim/jrandom.h"
#include "sim/organism.h"
#include "syn/arena.h"

/* sides of the maps; the first organisms as many to the area as the
 * simulation's default world has */
#ifdef BIO_COLOUR
/* Color Mod's worlds fill up: 400 x 400 holds about a thousand */
static const int k_map_side[MAP_COUNT] = {0, 400, 700, 1000};
#else
static const int k_map_side[MAP_COUNT] = {0, 1000, 2000, 3000};
#endif

void round_apply_map(Params *p, int map)
{
    if (map <= MAP_PARAMS || map >= MAP_COUNT)
        return;
    Params d;
    memset(&d, 0, sizeof d);
    params_set_defaults(&d);
    int s = k_map_side[map];
    p->world_width = p->world_height = s;
    p->initial_organisms =
        (int)lround((double)d.initial_organisms * s * s / ((double)d.world_width * d.world_height));
    if (p->initial_organisms < 1)
        p->initial_organisms = 1;
}

void round_params(Params *q, int map)
{
    memcpy(q, &P, sizeof *q);
    round_apply_map(q, map);
}

static uint64_t fnv(uint64_t h, const void *p, size_t n)
{
    const unsigned char *b = p;
    for (size_t i = 0; i < n; i++) {
        h ^= b[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

uint64_t round_fingerprint(int map)
{
    Params q;
    round_params(&q, map);
    /* what only the window reads */
    q.window_width = q.window_height = 0;
    q.delay = 0;
#ifdef BIO_COLOUR
    q.window_x = q.window_y = q.window_state = 0;
    q.auto_backup = false;
    q.backup_delay = q.local_port = q.max_connections = 0;
    q.accept_connections = q.connect_to_server = false;
    q.hardware_acceleration = q.server_port = 0;
#endif
    uint64_t h = fnv(0xcbf29ce484222325ULL, &q, sizeof q);
    return h ? h : 1;
}

/* A code put in the world as the original pastes one, at a random place:
 * twelve tries, as genesis gives each random organism. */
static Organism *paste(World *w, GeneticCode *gc)
{
    for (int tries = 12; tries > 0; tries--) {
        Organism *o = org_new_with_code(w, gc);
        int x = jr_next_int(&g_rng, w->width), y = jr_next_int(&g_rng, w->height);
#ifdef BIO_COLOUR
        bool ok = org_paste_organism(o, x, y);
#else
        bool ok = org_paste(o, x, y);
#endif
        if (ok) {
            world_add_organism(w, o, NULL);
            return o;
        }
        org_free(o);
    }
    return NULL;
}

int round_begin(World *w, int64_t seed, GeneticCode *const *code, int ns, int random_pct,
                void (*after_genesis)(World *), int *first, int *placed)
{
    jr_set_seed(&g_rng, seed);
    /* the random organisms as world_genesis makes them, fewer by the
     * fighters' share */
    int total = P.initial_organisms, per = 0;
    if (ns > 0) {
        int nc = (int)lround(total * (100.0 - random_pct) / 100.0);
        per = nc / ns;
        if (per < 1)
            per = 1;
    }
    /* none at all for winners only, not the few left over */
    int nrandom = ns > 0 && random_pct == 0 ? 0 : total - per * ns;
    if (nrandom < 0)
        nrandom = 0;
    P.initial_organisms = nrandom;
    world_genesis(w);
    P.initial_organisms = total;
    if (after_genesis)
        after_genesis(w);

    /* then the fighters, each its own bloodline from its first copy */
    for (int k = 0; k < ns; k++) {
        first[k] = -1;
        placed[k] = 0;
        for (int i = 0; i < per; i++) {
            Organism *o = paste(w, code[k]);
            if (o) {
                if (first[k] < 0)
                    first[k] = o->id;
                placed[k]++;
            }
        }
    }
    return per;
}
