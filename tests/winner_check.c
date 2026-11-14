/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 * SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)
 */

/* What Create a winner (src/syn/winner.c) stands on, checked apart from
 * it, with the simulation alone (built for 0.9 and for Color Mod):
 *
 *   winner_check same WORLD FRAMES MORE SNAPSHOT
 *       WORLD run FRAMES frames and saved to SNAPSHOT, a random code
 *       pasted, and the world run MORE frames; then SNAPSHOT opened, the
 *       generator put back, the same code pasted at the same place and run
 *       as long. Prints both worlds hashed and exits 0 if they are one.
 *   winner_check replay WORLD STATE CODE X Y T
 *       WORLD opened, the generator at STATE (hexadecimal), CODE pasted at
 *       (X, Y) as the window pastes it, and the world run to time unit T.
 *       Prints "T MINE ALIVE HASH": how many of the living are the pasted
 *       organism's bloodline, how many live, and the world hashed as
 *       syn_winner_hash hashes it. */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io/bgg.h"
#include "io/bgw.h"
#include "sim/jrandom.h"
#include "sim/organism.h"
#include "sim/params.h"
#include "sim/world.h"

static uint64_t fnv(uint64_t h, uint64_t v)
{
    for (int i = 0; i < 8; i++) {
        h ^= (v >> (8 * i)) & 0xff;
        h *= 0x100000001b3ULL;
    }
    return h;
}

static uint64_t dbits(double d)
{
    uint64_t u;
    memcpy(&u, &d, sizeof u);
    return u;
}

/* syn_winner_hash's recipe */
static uint64_t hash(const World *w)
{
    uint64_t h = 0xcbf29ce484222325ULL;
    h = fnv(h, (uint64_t)(int64_t)w->population);
    h = fnv(h, dbits(w->o2));
    h = fnv(h, dbits(w->co2));
#ifdef BIO_COLOUR
    h = fnv(h, dbits(w->ch4));
#endif
    h = fnv(h, (uint64_t)(int64_t)w->next_id);
    h = fnv(h, (uint64_t)w->stats.time);
    h = fnv(h, (uint64_t)(int64_t)w->nframes);
    for (int i = 0; i < w->norgs; i++) {
        const Organism *o = w->orgs[i];
        if (o->removed)
            continue;
        h = fnv(h, (uint64_t)(int64_t)o->id);
        h = fnv(h, o->alive ? 1 : 0);
        h = fnv(h, dbits(o->energy));
        h = fnv(h, dbits(o->dcenter_x));
        h = fnv(h, dbits(o->dcenter_y));
    }
    return fnv(h, g_rng.seed);
}

static Organism *paste(World *w, GeneticCode *gc, int x, int y)
{
    Organism *o = org_new_with_code(w, gc);
#ifdef BIO_COLOUR
    bool ok = org_paste_organism(o, x, y);
#else
    bool ok = org_paste(o, x, y);
#endif
    if (!ok) {
        org_free(o);
        return NULL;
    }
    world_add_organism(w, o, NULL);
    return o;
}

static bool open_world(World *w, const char *path)
{
    char err[256];
    world_init(w);
    if (bgw_load(w, path, err, sizeof err))
        return true;
    fprintf(stderr, "%s: %s\n", path, err);
    return false;
}

/* a place a code fits: the world's middle, else along its diagonal. A try
 * that finds no room still draws from the generator, so it is put back:
 * the paste that fits starts from the same state as one straight there. */
static Organism *paste_somewhere(World *w, GeneticCode *gc, int *x, int *y)
{
    for (int k = 0; k < 64; k++) {
        *x = w->width / 2 + (k % 2 ? 1 : -1) * (k / 2) * w->width / 140;
        *y = w->height / 2 + (k % 2 ? 1 : -1) * (k / 2) * w->height / 140;
        JRandom before = g_rng;
        Organism *o = paste(w, gc, *x, *y);
        if (o)
            return o;
        g_rng = before;
    }
    return NULL;
}

static int same(const char *world, long frames, long more, const char *snap)
{
    char err[256];
    World a;
    if (!open_world(&a, world))
        return 2;
    for (long f = 0; f < frames; f++)
        world_time(&a);
    if (!bgw_save(&a, snap, err, sizeof err)) {
        fprintf(stderr, "%s: %s\n", snap, err);
        return 2;
    }
    /* the code as a .bgg, read afresh for each world, as the search's
     * workers and the window read the winner (Color Mod writes a pasted
     * code's clade into it) */
    char code[1100];
    snprintf(code, sizeof code, "%s.bgg", snap);
    GeneticCode *gc = gc_new_random();
    if (!bgg_write(code, gc)) {
        fprintf(stderr, "%s: not written\n", code);
        return 2;
    }
    if (!(gc = bgg_read(code, err, sizeof err))) {
        fprintf(stderr, "%s: %s\n", code, err);
        return 2;
    }
    JRandom at = g_rng;
    uint64_t before = hash(&a);
    int x, y;
    if (!paste_somewhere(&a, gc, &x, &y)) {
        fprintf(stderr, "no room for the code\n");
        return 2;
    }
    for (long f = 0; f < more; f++)
        world_time(&a);
    uint64_t ha = hash(&a);
    int pa = a.population;
    world_free(&a);

    World b;
    if (!open_world(&b, snap))
        return 2;
    g_rng = at;
    uint64_t opened = hash(&b);
    if (!(gc = bgg_read(code, err, sizeof err))) {
        fprintf(stderr, "%s: %s\n", code, err);
        return 2;
    }
    if (!paste(&b, gc, x, y)) {
        fprintf(stderr, "no room for the code in the opened world\n");
        return 2;
    }
    for (long f = 0; f < more; f++)
        world_time(&b);
    uint64_t hb = hash(&b);
    printf("%ld frames, saved, a code pasted at (%d, %d), %ld more: left running %016" PRIx64
           " (%d living), saved and opened %016" PRIx64 " (%d living); at the save %016" PRIx64 " and %016" PRIx64
           "\n",
           frames, x, y, more, ha, pa, hb, b.population, before, opened);
    world_free(&b);
    return ha == hb && before == opened ? 0 : 1;
}

static int replay(const char *world, const char *state, const char *code, int x, int y, long long t)
{
    char err[256];
    World w;
    if (!open_world(&w, world))
        return 2;
    g_rng.seed = strtoull(state, NULL, 16);
    GeneticCode *gc = bgg_read(code, err, sizeof err);
    if (!gc) {
        fprintf(stderr, "%s: %s\n", code, err);
        return 2;
    }
#ifndef BIO_COLOUR
    for (int i = 0; i < w.norgs; i++) {
        w.orgs[i]->gc->lineage = 0;
        if (w.orgs[i]->infected_gc)
            w.orgs[i]->infected_gc->lineage = 0;
    }
    gc->lineage = 1;
#endif
    Organism *o = paste(&w, gc, x, y);
    if (!o) {
        fprintf(stderr, "no room at (%d, %d)\n", x, y);
        return 2;
    }
#ifdef BIO_COLOUR
    long root = strtol(o->gc->clade_id, NULL, 10);
#endif
    while (w.stats.time < t)
        world_time(&w);
    int mine = 0, alive = 0;
    for (int i = 0; i < w.norgs; i++) {
        const Organism *q = w.orgs[i];
        if (q->removed || !q->alive)
            continue;
        alive++;
#ifdef BIO_COLOUR
        mine += q->gc && q->gc->clade_id && strtol(q->gc->clade_id, NULL, 10) == root;
#else
        mine += q->gc->lineage == 1;
#endif
    }
    printf("%lld %d %d %016" PRIx64 "\n", (long long)w.stats.time, mine, alive, hash(&w));
    world_free(&w);
    return 0;
}

int main(int argc, char **argv)
{
    params_set_defaults(&P);
    params_load();
    if (argc == 6 && !strcmp(argv[1], "same"))
        return same(argv[2], atol(argv[3]), atol(argv[4]), argv[5]);
    if (argc == 8 && !strcmp(argv[1], "replay"))
        return replay(argv[2], argv[3], argv[4], atoi(argv[5]), atoi(argv[6]), atoll(argv[7]));
    fprintf(stderr, "usage: winner_check same WORLD FRAMES MORE SNAPSHOT\n"
                    "       winner_check replay WORLD STATE CODE X Y T\n");
    return 2;
}
