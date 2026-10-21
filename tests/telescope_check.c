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
 * 4. The count of draws between two states is the jump that joins them,
 *    and a gene drawn in a running world is found at its place in the
 *    world's stream.
 * 5. The engine (src/syn/crack.c): every founder of a dozen worlds drawn
 *    again from its genes, the world's seed among those its basin lists,
 *    and a gene drawn later placed in its ancestor's stream.
 */
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "sim/genetic_code.h"
#include "sim/jrandom.h"
#include "sim/organism.h"
#include "sim/params.h"
#include "sim/world.h"
#include "syn/crack.h"
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

    /* 4: draws between states, there and back over the whole period */
    int counts = 0;
    uint64_t x = 0x2545f4914f6cULL;
    for (int i = 0; i < 200; i++) {
        x = x * 0x9e3779b97f4a7c15ULL + 1;
        uint64_t from = x & TEL_MASK, n = (x >> 13) & TEL_MASK;
        if (i < 48)
            n = 1ULL << i;
        else if (i < 64)
            n = (uint64_t)(i - 48);
        CHECK(tel_draws(from, tel_jump(from, (int64_t)n)) == n,
              "%" PRIu64 " draws on from 0x%012" PRIx64, n, from);
        CHECK(tel_draws(tel_jump(from, -(int64_t)n), from) == n,
              "%" PRIu64 " draws back to 0x%012" PRIx64, n, from);
        counts += 2;
    }
    CHECK(tel_draws(5, 5) == 0, "no draws");
    CHECK(tel_draws(tel_jump(7, 1), 7) == TEL_MASK, "a draw back is 2^48 - 1 on");
    {
        /* a gene drawn after some thousands of draws, cracked from its
         * length alone and counted from the seed */
        jr_set_seed(&g_rng, 20261021);
        uint64_t s0 = g_rng.seed;
        for (int j = 0; j < 12345; j++)
            jr_next_boolean(&g_rng);
        uint64_t before = g_rng.seed;
        Gene g;
        gene_randomize_length(&g);
        uint64_t st[64];
        int n = tel_crack_length(g.length, st, 64);
        bool at = false;
        for (int k = 0; k < n && k < 64; k++)
            at |= tel_draws(s0, st[k]) == 12345;
        CHECK(at && tel_draws(s0, before) == 12345, "a gene's place in the stream");
        CHECK(tel_draws(s0, g_rng.seed) == 12347, "the stream after the gene's two draws");
        counts += 2;
    }

    /* 5: founders drawn again, their seeds, a later draw placed */
    int founders = 0, basins = 0, placed = 0;
    {
        World w;
        world_init(&w);
        for (int64_t seed = 1; seed <= 12; seed++) {
            jr_set_seed(&g_rng, seed);
            uint64_t s0 = g_rng.seed;
            world_genesis(&w);
            bool changed = false;
            for (int i = 0; i < w.norgs; i++) {
                const GeneticCode *gc = w.orgs[i]->gc;
                CrackAncestor a[8];
                int na = crack_ancestors(gc, a, 8);
                CHECK(na > 0 && a[0].changes == 0 && crack_changes(a[0].gc, gc) == 0,
                      "seed %" PRId64 ": founder %d drawn again from its genes", seed, i + 1);
                founders++;
                if (na && i % 4 == 0) {
                    static CrackSeed bs[1024];
                    int nb = crack_basin(a[0].start, 0, bs, 1024);
                    bool in = false;
                    for (int k = 0; k < nb && k < 1024; k++)
                        in |= bs[k].state == s0 && bs[k].org == i;
                    CHECK(in, "seed %" PRId64 " among the %d seeds that make its founder %d", seed, nb, i + 1);
                    basins++;
                }
                if (na && !changed && gc->ngenes >= 2) {
                    /* its first gene's length drawn again, 54321 draws on
                     * (a founder of one gene would have none left to know
                     * it by) */
                    changed = true;
                    Gene g[CRACK_FOUNDER_GENES];
                    for (int j = 0; j < gc->ngenes; j++)
                        g[j] = gc->genes[j];
                    uint64_t at = tel_jump(a[0].start, 54321);
                    g_rng.seed = at;
                    gene_randomize_length(&g[0]);
                    GeneticCode *m = gc_new_from_genes(g, gc->ngenes, gc->symmetry, gc->mirror,
                                                       gc->disperse_children);
                    CrackAncestor b[8];
                    int nb = crack_ancestors(m, b, 8);
                    CrackDraw d[8];
                    int nd = nb ? crack_draws(m, &b[0], d, 8) : 0;
                    CHECK(nb && b[0].start == a[0].start && b[0].changes == 1,
                          "seed %" PRId64 ": a changed founder's ancestor", seed);
                    CHECK(nd >= 1 && d[0].gene == 0 && !d[0].angle && d[0].place == 54321,
                          "seed %" PRId64 ": its changed length placed 54321 draws on", seed);
                    placed++;
                    crack_ancestors_free(b, nb);
                    gc_unref(m);
                }
                crack_ancestors_free(a, na);
            }
        }
        world_free(&w);
    }

    if (!failures)
        printf("telescope: %d jumps land where single draws do, seeds as java.util.Random keeps "
               "them, %d gene draws cracked back to their states (%d candidate state%s at most), "
               "%d counts of draws between states\n",
               jumps, ncrack, most, most == 1 ? "" : "s", counts);
    if (!failures)
        printf("telescope: the engine drew %d founders again from their genes, found %d worlds among their "
               "founders' seeds and placed %d later draws\n",
               founders, basins, placed);
    return failures != 0;
}
