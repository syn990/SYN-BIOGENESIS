/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Part of a C port of Biogenesis (https://biogenesis.sourceforge.net/).
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

/* The dice telescope (src/syn/telescope.c) against the generator itself.
 *
 * 1. A jump of n draws, either way, lands where n single draws do.
 * 2. A seed's first state is the one java.util.Random.setSeed leaves, and
 *    seeds equal mod 2^48 share it.
 * 3. Every gene length and angle the simulation draws cracks back to a set
 *    of states that includes the one it was drawn from.
 */
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "sim/genetic_code.h"
#include "sim/jrandom.h"
#include "sim/params.h"
#include "syn/telescope.h"

static int failures;

#define CHECK(cond, ...)                                                                           \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            printf("FAIL telescope: " __VA_ARGS__);                                                \
            printf("\n");                                                                          \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

static bool contains(const uint64_t *v, int n, uint64_t x)
{
    for (int i = 0; i < n; i++)
        if (v[i] == x)
            return true;
    return false;
}

int main(void)
{
    params_set_defaults(&P);
    JRandom r;

    /* 1: jumps, against one draw at a time (a boolean is one draw) */
    static const int64_t lengths[] = {0, 1, 2, 3, 5, 64, 1000, 99991};
    int jumps = 0;
    for (int64_t seed = -3; seed < 40; seed++) {
        for (size_t k = 0; k < sizeof lengths / sizeof *lengths; k++) {
            jr_set_seed(&r, seed * 7919 + 13);
            uint64_t start = r.seed;
            for (int64_t i = 0; i < lengths[k]; i++)
                jr_next_boolean(&r);
            CHECK(tel_jump(start, lengths[k]) == r.seed, "seed %" PRId64 ": %" PRId64 " draws on",
                  seed, lengths[k]);
            CHECK(tel_jump(r.seed, -lengths[k]) == start, "seed %" PRId64 ": %" PRId64 " draws back",
                  seed, lengths[k]);
            jumps += 2;
        }
    }
    CHECK(tel_jump(tel_jump(0x123456789abcULL, 1LL << 40), -(1LL << 40)) == 0x123456789abcULL,
          "2^40 draws there and back");

    /* 2: seeds */
    static const int64_t seeds[] = {0, 1, 42, -1, 123456789, 1760990000123LL, INT64_MIN, INT64_MAX};
    for (size_t k = 0; k < sizeof seeds / sizeof *seeds; k++) {
        jr_set_seed(&r, seeds[k]);
        CHECK(r.seed == tel_seed_state(seeds[k]), "seed %" PRId64 "'s first state", seeds[k]);
        int64_t low = (int64_t)((uint64_t)seeds[k] & TEL_MASK);
        CHECK(tel_state_seed(r.seed) == low, "seed %" PRId64 " back from its state", seeds[k]);
        jr_set_seed(&r, (int64_t)((uint64_t)seeds[k] + (1ULL << 48)));
        CHECK(r.seed == tel_seed_state(seeds[k]), "seed %" PRId64 " + 2^48", seeds[k]);
    }

    /* 3: genes drawn by the simulation, from states spread over the
     * generator, and from 16 up, where a length has lost its last bit */
    int ncrack = 0, most = 0;
    for (int i = 0; i < 48; i++) {
        jr_set_seed(&g_rng, 1000003LL * i);
        for (int j = 0; j < i % 7; j++)
            jr_next_boolean(&g_rng);
        uint64_t s = g_rng.seed;
        Gene g;
        gene_randomize_length(&g);
        uint64_t st[64];
        int n = tel_crack_length(g.length, st, 64);
        CHECK(contains(st, n < 64 ? n : 64, s), "length %.17g from 0x%012" PRIx64, g.length, s);
        most = n > most ? n : most;
        g_rng.seed = s;
        gene_randomize_theta(&g);
        n = tel_crack_theta(g.theta, st, 64);
        CHECK(contains(st, n < 64 ? n : 64, s), "angle %.17g from 0x%012" PRIx64, g.theta, s);
        ncrack += 2;
    }
    int high = 0;
    for (uint64_t s = 1; high < 8 && s < 100000; s += 7) {
        g_rng.seed = s * 0x9e3779b97f4aULL & TEL_MASK;
        uint64_t st0 = g_rng.seed;
        Gene g;
        gene_randomize_length(&g);
        if (g.length < 16.0)
            continue;
        uint64_t st[64];
        int n = tel_crack_length(g.length, st, 64);
        CHECK(contains(st, n < 64 ? n : 64, st0), "length %.17g (16 and up)", g.length);
        high++;
        ncrack++;
    }
    CHECK(tel_crack_length(1.5, (uint64_t[1]){0}, 1) == 0, "a length no gene can have");

    if (!failures)
        printf("telescope: %d jumps land where single draws do, seeds as java.util.Random keeps "
               "them, %d gene draws cracked back to their states (%d candidate state%s at most)\n",
               jumps, ncrack, most, most == 1 ? "" : "s");
    return failures != 0;
}
