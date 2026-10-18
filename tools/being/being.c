/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 * SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)
 */

/* The tool behind tools/being/search.py: worlds evolved on the saved
 * parameters (~/.config/syn-biogenesis/params.cfg), codes harvested from
 * them, and a code's fate when one copy is imported into a world.
 *
 *   being base SEED FRAMES OUT.sbw          an evolved world to import into
 *   being evolve SEED FRAMES EVERY OUTDIR   a long evolution; every EVERY
 *                                           frames the three commonest
 *                                           living codes go to OUTDIR
 *   being harvest IN.sbw OUTDIR K           the K commonest living codes
 *   being eval CODE.bgg WORLD RSEED FRAMES [EVERY [SAVE.sbw]]
 *       imports one copy as the window's Import does, at a random free spot,
 *       and runs FRAMES frames. Prints "T frame lineage population" every
 *       EVERY frames (default FRAMES/10), then "END lineage population
 *       extinct_frame peak": its lineage is every living organism whose code
 *       descends from it, through children and through the children of
 *       organisms it infected. With SAVE, the world is saved at the end
 *       (contact flashes worn off as on screen, see fade_flashes) and
 *       "SPOT x y n all" names the lineage organism with the most of its
 *       lineage (n) within 250 pixels, where all organisms alive number
 *       all (for a screenshot).
 *   being layout CODE.bgg                   the genetic laboratory's drawing
 *                                           of the code: "x0 y0 x1 y1 #rgb"
 *   being battle WORLD BSEED FRAMES EVERY A.bgg B.bgg [...]
 *       2 to 8 contenders imported into the same world, see battle() below.
 */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io/bgg.h"
#include "io/bgw.h"
#include "io/worldfile.h"
#include "sim/jrandom.h"
#include "sim/params.h"
#include "sim/world.h"

static bool load_world(World *w, const char *path, char *err, size_t errn)
{
    return bgw_sniff(path) ? bgw_load(w, path, err, errn) : worldfile_load(w, path, err, errn);
}

static int lineage(const World *w, int *pop)
{
    int n = 0;
    *pop = 0;
    for (int i = 0; i < w->norgs; i++) {
        const Organism *o = w->orgs[i];
        if (!o->alive)
            continue;
        (*pop)++;
        n += o->gc->lineage == 1;
    }
    return n;
}

/* Imports one organism with code gc at a random spot, as the window's
 * Import does at a clicked one: spots are drawn from place (x then y,
 * uniform over the world) until org_paste accepts one, up to 5000 tries. */
/* A contact flash (blue for a blocked bite, yellow for a bitten organism, and
 * so on) wears off only as the window draws the organism (Organism.draw), so
 * a world run without a window keeps every flash it ever got. For a world
 * saved to be looked at, wear them off as a window drawing every frame
 * would. Display only: the simulation never reads frames_color. */
static void fade_flashes(World *w)
{
    for (int i = 0; i < w->norgs; i++)
        if (w->orgs[i]->frames_color > 0)
            w->orgs[i]->frames_color--;
}

static bool import_random(World *w, GeneticCode *gc, JRandom *place)
{
    for (int t = 0; t < 5000; t++) {
        Organism *o = org_new_with_code(w, gc);
        int x = jr_next_int(place, w->width), y = jr_next_int(place, w->height);
        if (org_paste(o, x, y)) {
            world_add_organism(w, o, NULL);
            return true;
        }
        org_free(o);
    }
    return false;
}

/* The battle protocol: deterministic, so a battle is named by its world,
 * BSEED and the contenders in order, and replays exactly.
 *   1. Load WORLD. Seed the world's generator with BSEED.
 *   2. Seed a separate placement generator with BSEED * 7919 + 17.
 *   3. Import the contenders in argument order, each at its own random
 *      spot (import_random); contender k's code is marked lineage k.
 *   4. Run FRAMES frames. Every EVERY frames print "T frame n1 .. nk
 *      population": each contender's living lineage and every living
 *      organism. At the end print "END n1 .. nk population".
 * The winner has the largest lineage at the end. Import order matters
 * (earlier organisms move first each frame), so a fair pairing plays
 * both orders. */
static int battle(int argc, char **argv)
{
    char err[256];
    int k = argc - 6;
    World w;
    world_init(&w);
    if (!load_world(&w, argv[2], err, sizeof err)) {
        fprintf(stderr, "%s\n", err);
        return 1;
    }
    int64_t bseed = atoll(argv[3]);
    long frames = atol(argv[4]), every = atol(argv[5]);
    jr_set_seed(&g_rng, bseed);
    JRandom place;
    jr_set_seed(&place, bseed * 7919 + 17);
    for (int i = 0; i < k; i++) {
        GeneticCode *gc = bgg_read(argv[6 + i], err, sizeof err);
        if (!gc) {
            fprintf(stderr, "%s: %s\n", argv[6 + i], err);
            return 1;
        }
        gc->lineage = (uint32_t)(i + 1);
        bool placed = import_random(&w, gc, &place);
        gc_unref(gc);
        if (!placed) {
            fprintf(stderr, "%s: no free spot\n", argv[6 + i]);
            return 1;
        }
    }
    for (long f = 1; f <= frames; f++) {
        world_time(&w);
        if (f % every && f != frames)
            continue;
        int n[8] = {0}, pop = 0;
        for (int i = 0; i < w.norgs; i++) {
            const Organism *o = w.orgs[i];
            if (!o->alive)
                continue;
            pop++;
            if (o->gc->lineage >= 1 && o->gc->lineage <= (uint32_t)k)
                n[o->gc->lineage - 1]++;
        }
        printf(f == frames ? "END" : "T %ld", f);
        for (int i = 0; i < k; i++)
            printf(" %d", n[i]);
        printf(" %d\n", pop);
    }
    world_free(&w);
    return 0;
}

static uint64_t code_hash(const GeneticCode *gc)
{
    uint64_t h = 1469598103934665603ULL;
#define MIX(v)                                                                                     \
    do {                                                                                           \
        uint64_t x_ = (uint64_t)(v);                                                               \
        for (int b_ = 0; b_ < 8; b_++) {                                                           \
            h ^= (x_ >> (8 * b_)) & 0xff;                                                          \
            h *= 1099511628211ULL;                                                                 \
        }                                                                                          \
    } while (0)
    MIX(gc->symmetry);
    MIX(gc->mirror);
    MIX(gc->disperse_children);
    for (int i = 0; i < gc->ngenes; i++) {
        uint64_t a, b;
        memcpy(&a, &gc->genes[i].length, 8);
        memcpy(&b, &gc->genes[i].theta, 8);
        MIX(a);
        MIX(b);
        MIX(gc->genes[i].color);
    }
    return h;
}

int main(int argc, char **argv)
{
    params_set_defaults(&P);
    params_load();
    char err[256];
    if (argc >= 8 && argc <= 14 && !strcmp(argv[1], "battle"))
        return battle(argc, argv);
    if (argc == 3 && !strcmp(argv[1], "layout")) {
        GeneticCode *gc = bgg_read(argv[2], err, sizeof err);
        if (!gc) {
            fprintf(stderr, "%s: %s\n", argv[2], err);
            return 1;
        }
        int max = gc->ngenes * gc->symmetry, w, h;
        GcLine *l = calloc((size_t)max, sizeof *l);
        int n = gc_layout(gc, l, max, &w, &h);
        for (int i = 0; i < n; i++) {
            uint8_t r, g, b;
            bio_color_rgb(l[i].color, &r, &g, &b);
            printf("%d %d %d %d #%02x%02x%02x\n", l[i].x0, l[i].y0, l[i].x1, l[i].y1, r, g, b);
        }
        free(l);
        gc_unref(gc);
        return 0;
    }
    if (argc == 5 && !strcmp(argv[1], "base")) {
        jr_set_seed(&g_rng, atoll(argv[2]));
        World w;
        world_init(&w);
        world_genesis(&w);
        long frames = atol(argv[3]);
        for (long f = 0; f < frames; f++)
            world_time(&w);
        int pop;
        lineage(&w, &pop);
        printf("base seed %s: %ld frames, population %d, organisms %d, CO2 %.0f\n", argv[2],
               frames, pop, w.norgs, w.co2);
        if (!worldfile_save(&w, argv[4], err, sizeof err)) {
            fprintf(stderr, "%s\n", err);
            return 1;
        }
        world_free(&w);
        return 0;
    }
    if (argc == 6 && !strcmp(argv[1], "evolve")) {
        /* evolve SEED FRAMES EVERY OUTDIR: every EVERY frames, the three
         * commonest living codes go to OUTDIR/eSEED_FRAME_RANK.bgg */
        jr_set_seed(&g_rng, atoll(argv[2]));
        World w;
        world_init(&w);
        world_genesis(&w);
        long frames = atol(argv[3]), every = atol(argv[4]);
        for (long f = 1; f <= frames; f++) {
            world_time(&w);
            if (f % every)
                continue;
            int pop;
            lineage(&w, &pop);
            if (pop == 0) {
                printf("seed %s extinct by frame %ld\n", argv[2], f);
                break;
            }
            int n = w.norgs;
            uint64_t *h = calloc((size_t)n + 1, sizeof *h);
            int *cnt = calloc((size_t)n + 1, sizeof *cnt);
            GeneticCode **rep = calloc((size_t)n + 1, sizeof *rep);
            int nu = 0;
            for (int i = 0; i < n; i++) {
                if (!w.orgs[i]->alive)
                    continue;
                uint64_t x = code_hash(w.orgs[i]->gc);
                int j = 0;
                while (j < nu && h[j] != x)
                    j++;
                if (j == nu) {
                    h[nu] = x;
                    rep[nu++] = w.orgs[i]->gc;
                }
                cnt[j]++;
            }
            for (int r = 0; r < 3; r++) {
                int best = -1;
                for (int j = 0; j < nu; j++)
                    if (cnt[j] > 1 && (best < 0 || cnt[j] > cnt[best]))
                        best = j;
                if (best < 0)
                    break;
                char path[1200];
                snprintf(path, sizeof path, "%s/e%s_%ld_%d.bgg", argv[5], argv[2], f, r);
                bgg_write(path, rep[best]);
                cnt[best] = 0;
            }
            printf("seed %s frame %ld population %d\n", argv[2], f, pop);
            fflush(stdout);
            free(h);
            free(cnt);
            free(rep);
        }
        world_free(&w);
        return 0;
    }
    if (argc == 5 && !strcmp(argv[1], "harvest")) {
        World w;
        world_init(&w);
        if (!load_world(&w, argv[2], err, sizeof err)) {
            fprintf(stderr, "%s\n", err);
            return 1;
        }
        int k = atoi(argv[4]), n = w.norgs;
        uint64_t *h = calloc((size_t)n, sizeof *h);
        int *cnt = calloc((size_t)n, sizeof *cnt);
        GeneticCode **rep = calloc((size_t)n, sizeof *rep);
        int nu = 0;
        for (int i = 0; i < n; i++) {
            if (!w.orgs[i]->alive)
                continue;
            uint64_t x = code_hash(w.orgs[i]->gc);
            int j = 0;
            while (j < nu && h[j] != x)
                j++;
            if (j == nu) {
                h[nu] = x;
                rep[nu] = w.orgs[i]->gc;
                nu++;
            }
            cnt[j]++;
        }
        for (int r = 0; r < k; r++) {
            int best = -1;
            for (int j = 0; j < nu; j++)
                if (cnt[j] > 0 && (best < 0 || cnt[j] > cnt[best]))
                    best = j;
            if (best < 0)
                break;
            char path[1200];
            snprintf(path, sizeof path, "%s/h%d.bgg", argv[3], r);
            bgg_write(path, rep[best]);
            printf("%s %d\n", path, cnt[best]);
            cnt[best] = 0;
        }
        world_free(&w);
        return 0;
    }
    if (argc >= 6 && argc <= 8 && !strcmp(argv[1], "eval")) {
        World w;
        world_init(&w);
        if (!load_world(&w, argv[3], err, sizeof err)) {
            fprintf(stderr, "%s\n", err);
            return 1;
        }
        int64_t rseed = atoll(argv[4]);
        jr_set_seed(&g_rng, rseed);
        GeneticCode *gc = bgg_read(argv[2], err, sizeof err);
        if (!gc) {
            fprintf(stderr, "%s: %s\n", argv[2], err);
            return 1;
        }
        gc->lineage = 1;
        /* a random free spot, from its own generator so the world's isn't touched */
        JRandom place;
        jr_set_seed(&place, rseed * 7919 + 17);
        bool placed = import_random(&w, gc, &place);
        gc_unref(gc);
        if (!placed) {
            printf("END 0 0 0 0 unplaced\n");
            return 0;
        }
        long frames = atol(argv[5]), extinct = 0;
        long every = argc == 7 ? atol(argv[6]) : frames / 10;
        if (every < 50)
            every = 50;
        int peak = 1, pop;
        for (long f = 1; f <= frames; f++) {
            world_time(&w);
            if (argc == 8)
                fade_flashes(&w);
            if (f % 50 == 0 || f == frames) {
                int n = lineage(&w, &pop);
                if (n > peak)
                    peak = n;
                if (n == 0 && !extinct) {
                    extinct = f;
                    printf("END 0 %d %ld %d\n", pop, extinct, peak);
                    world_free(&w);
                    return 0;
                }
                if (f % every == 0)
                    printf("T %ld %d %d\n", f, n, pop);
            }
        }
        int n = lineage(&w, &pop);
        if (argc == 8) {
            const Organism *spot = NULL;
            int most = -1, all = 0;
            for (int i = 0; i < w.norgs; i++) {
                const Organism *a = w.orgs[i];
                if (!a->alive || a->gc->lineage != 1)
                    continue;
                int near = 0, alive = 0;
                for (int k = 0; k < w.norgs; k++) {
                    const Organism *b = w.orgs[k];
                    double dx = a->dcenter_x - b->dcenter_x, dy = a->dcenter_y - b->dcenter_y;
                    bool in = b->alive && dx * dx + dy * dy < 250.0 * 250.0;
                    near += in && b->gc->lineage == 1;
                    alive += in;
                }
                if (near > most) {
                    most = near;
                    all = alive;
                    spot = a;
                }
            }
            if (spot)
                printf("SPOT %.0f %.0f %d %d\n", spot->dcenter_x, spot->dcenter_y, most, all);
            if (!worldfile_save(&w, argv[7], err, sizeof err))
                fprintf(stderr, "%s\n", err);
        }
        printf("END %d %d 0 %d\n", n, pop, peak);
        world_free(&w);
        return 0;
    }
    fprintf(stderr, "usage: see the comment at the top of being.c\n");
    return 2;
}
