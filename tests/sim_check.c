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

/* File format checks.
 *
 * 1. A world saved and loaded mid-run continues exactly like the world
 *    that was never saved (same random stream, compared field by field),
 *    as .sbw and as .bgw; a .bgw saved again after loading is unchanged.
 * 2. Genetic codes survive a .bgg write/read with every double intact.
 * 3. Malformed .bgg input is rejected, not half-loaded.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "io/bgg.h"
#include "io/bgw.h"
#include "io/worldfile.h"
#include "sim/params.h"
#include "sim/world.h"

static int failures;

#define CHECK(cond, ...)                                                                           \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);                                            \
            printf(__VA_ARGS__);                                                                   \
            printf("\n");                                                                          \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

static bool same_org(const Organism *a, const Organism *b)
{
    if (a->id != b->id || a->alive != b->alive || a->segments != b->segments ||
        a->energy != b->energy || a->dcenter_x != b->dcenter_x || a->dcenter_y != b->dcenter_y ||
        a->theta != b->theta || a->dx != b->dx || a->dy != b->dy || a->dtheta != b->dtheta ||
        a->growth_ratio != b->growth_ratio || a->age != b->age || a->r.x != b->r.x ||
        a->r.y != b->r.y || a->r.width != b->r.width || a->r.height != b->r.height ||
        a->total_children != b->total_children || a->total_kills != b->total_kills ||
        a->total_infected != b->total_infected || a->gc->ngenes != b->gc->ngenes ||
        (a->infected_gc == NULL) != (b->infected_gc == NULL))
        return false;
    for (int i = 0; i < a->segments; i++)
        if (a->x1[i] != b->x1[i] || a->y2[i] != b->y2[i] || a->seg_color[i] != b->seg_color[i])
            return false;
    return true;
}

static bool same_world(const World *a, const World *b)
{
    if (a->norgs != b->norgs || a->population != b->population || a->o2 != b->o2 ||
        a->co2 != b->co2 || a->next_id != b->next_id || a->nframes != b->nframes ||
        a->stats.time != b->stats.time || a->stats.death_sum != b->stats.death_sum ||
        a->stats.mass_extinctions != b->stats.mass_extinctions)
        return false;
    for (int i = 0; i < a->norgs; i++)
        if (!same_org(a->orgs[i], b->orgs[i]))
            return false;
    return true;
}

typedef bool (*SaveFn)(const World *, const char *, char *, size_t);
typedef bool (*LoadFn)(World *, const char *, char *, size_t);

static bool same_file(const char *p, const char *q)
{
    FILE *a = fopen(p, "rb"), *b = fopen(q, "rb");
    bool same = a && b;
    int x, y;
    while (same && ((x = fgetc(a)) != EOF) | ((y = fgetc(b)) != EOF))
        same = x == y;
    if (a)
        fclose(a);
    if (b)
        fclose(b);
    return same;
}

static void check_world_roundtrip(const char *path, const char *ext, SaveFn save, LoadFn load)
{
    jr_set_seed(&g_rng, 4242);
    World a;
    world_init(&a);
    world_genesis(&a);
    for (int i = 0; i < 3000; i++)
        world_time(&a);

    char err[256];
    CHECK(save(&a, path, err, sizeof err), "save: %s", err);
    World b;
    world_init(&b);
    CHECK(load(&b, path, err, sizeof err), "load: %s", err);
    CHECK(same_world(&a, &b), "loaded world differs from saved world");
    char again[1100];
    snprintf(again, sizeof again, "%s.again", path);
    CHECK(save(&b, again, err, sizeof err), "save again: %s", err);
    if (save == bgw_save)
        CHECK(same_file(path, again), "a loaded .bgw saves differently");
    remove(again);

    /* identity of shared codes must survive (white segments compare them) */
    int shared_a = 0, shared_b = 0;
    for (int i = 0; i < a.norgs; i++)
        for (int j = i + 1; j < a.norgs && j < i + 50; j++) {
            shared_a += a.orgs[i]->gc == a.orgs[j]->gc;
            shared_b += b.orgs[i]->gc == b.orgs[j]->gc;
        }
    CHECK(shared_a == shared_b, "code sharing changed: %d vs %d", shared_a, shared_b);

    JRandom saved = g_rng;
    for (int i = 0; i < 3000; i++)
        world_time(&a);
    g_rng = saved;
    for (int i = 0; i < 3000; i++)
        world_time(&b);
    CHECK(same_world(&a, &b), "loaded world diverged after 3000 more frames");
    printf("world save/load (.%s): %d organisms, identical continuation for 3000 frames\n", ext,
           a.norgs);
    world_free(&a);
    world_free(&b);

    /* damaged files are refused without touching the world */
    FILE *f = fopen(path, "r+b");
    if (f) {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fclose(f);
        if (truncate(path, size / 2) != 0)
            perror("truncate");
    }
    World c;
    world_init(&c);
    world_genesis(&c);
    int before = c.norgs;
    CHECK(!load(&c, path, err, sizeof err), "truncated world was accepted");
    CHECK(c.norgs == before, "failed load modified the world");
    world_free(&c);
}

static void check_bgg_roundtrip(const char *path)
{
    jr_set_seed(&g_rng, 77);
    int checked = 0;
    for (int n = 0; n < 200; n++) {
        GeneticCode *gc = gc_new_random();
        CHECK(bgg_write(path, gc), "bgg_write");
        char err[256];
        GeneticCode *back = bgg_read(path, err, sizeof err);
        CHECK(back != NULL, "bgg_read: %s", err);
        if (back) {
            bool same = back->ngenes == gc->ngenes && back->symmetry == gc->symmetry &&
                        back->mirror == gc->mirror &&
                        back->disperse_children == gc->disperse_children &&
                        back->reproduce_energy == gc->reproduce_energy;
            for (int i = 0; same && i < gc->ngenes; i++)
                same = back->genes[i].length == gc->genes[i].length &&
                       back->genes[i].theta == gc->genes[i].theta &&
                       back->genes[i].color == gc->genes[i].color;
            CHECK(same, "genetic code changed through .bgg");
            gc_unref(back);
            checked++;
        }
        gc_unref(gc);
    }
    printf("bgg round trip: %d random codes exact\n", checked);

    const char *bad[] = {
        "<genetic_code symmetry=\"9\" mirror=\"no\" disperse=\"no\"><gene length=\"2\" "
        "theta=\"0\" color=\"red\"/></genetic_code>",
        "<genetic_code symmetry=\"2\" mirror=\"maybe\" disperse=\"no\"><gene length=\"2\" "
        "theta=\"0\" color=\"red\"/></genetic_code>",
        "<genetic_code symmetry=\"2\" mirror=\"no\" disperse=\"no\"><gene length=\"x\" "
        "theta=\"0\" color=\"red\"/></genetic_code>",
        "<genetic_code symmetry=\"2\" mirror=\"no\" disperse=\"no\"><gene length=\"2\" "
        "theta=\"0\" color=\"purple\"/></genetic_code>",
        "<genetic_code symmetry=\"2\" mirror=\"no\" disperse=\"no\"></genetic_code>",
        "<other/>",
        "<genetic_code symmetry=\"2\" mirror=\"no\" disperse=\"no\"><gene length=\"2",
    };
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        char err[256] = "";
        GeneticCode *gc = bgg_parse(bad[i], strlen(bad[i]), err, sizeof err);
        CHECK(gc == NULL, "malformed input %zu accepted", i);
        gc_unref(gc);
    }
    printf("bgg rejects %zu malformed inputs\n", sizeof bad / sizeof bad[0]);
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : "/tmp";
    char wpath[1024], bpath[1024], gpath[1024];
    snprintf(wpath, sizeof wpath, "%s/sim_check_world.sbw", dir);
    snprintf(bpath, sizeof bpath, "%s/sim_check_world.bgw", dir);
    snprintf(gpath, sizeof gpath, "%s/sim_check_code.bgg", dir);
    params_set_defaults(&P);
    check_world_roundtrip(wpath, WORLD_FILE_EXT, worldfile_save, worldfile_load);
    check_world_roundtrip(bpath, BGW_FILE_EXT, bgw_save, bgw_load);
    check_bgg_roundtrip(gpath);
    remove(wpath);
    remove(bpath);
    remove(gpath);
    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    printf("all file checks passed\n");
    return 0;
}
