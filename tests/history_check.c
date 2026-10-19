/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 * SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)
 *
 * The SYN edition's history (src/syn/history.c) against the simulation:
 *
 *   history_check FRAMES SEED plain|history OUT.bgw > hashes
 *
 * runs a world, with or without the history recording it, printing a hash
 * of the whole world every frame and the random generator at the end, and
 * saves it. The two runs must print and save the same, byte for byte: the
 * history only reads the world (and marks lineage, which nothing reads).
 * The recorded run's world is made as the window makes it, an organism at
 * a time (syn/genesis.h), the plain one's by world_genesis.
 * With "history" it also checks the history's own books at every time
 * unit: samples in time order, epochs end to end, every birth, death and
 * infection counted in exactly one epoch, events in time order; and the
 * family tree's: every parent founded before its children, every living
 * organism in a species, their counts the population's, each epoch's best
 * pinned, nothing kept that should have been forgotten. Built with
 * small HIST_CAP and EPOCHS_CAP, a short run merges both many times. Built
 * with -Isrc/colour -DBIO_COLOUR, the same for Color Mod 2.01. */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io/bgw.h"
#include "sim/params.h"
#include "sim/world.h"
#include "syn/family.h"
#include "syn/genesis.h"
#include "syn/history.h"

#ifndef HIST_CAP
#define HIST_CAP 4096 /* as src/syn/history.c */
#endif

static int failures;

#define CHECK(cond, ...)                                                                           \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            if (failures++ < 10) {                                                                 \
                fprintf(stderr, "history_check: ");                                                \
                fprintf(stderr, __VA_ARGS__);                                                      \
                fputc('\n', stderr);                                                               \
            }                                                                                      \
        }                                                                                          \
    } while (0)

static uint64_t h = 0xcbf29ce484222325ULL;

static void mix(const void *p, size_t n)
{
    const unsigned char *b = p;
    for (size_t i = 0; i < n; i++) {
        h ^= b[i];
        h *= 0x100000001b3ULL;
    }
}

/* Everything an organism's future depends on. */
static uint64_t world_hash(const World *w)
{
    h = 0xcbf29ce484222325ULL;
    mix(&w->population, sizeof w->population);
    mix(&w->next_id, sizeof w->next_id);
    mix(&w->o2, sizeof w->o2);
    mix(&w->co2, sizeof w->co2);
    mix(&w->norgs, sizeof w->norgs);
    mix(&g_rng.seed, sizeof g_rng.seed);
    for (int i = 0; i < w->norgs; i++) {
        const Organism *o = w->orgs[i];
        mix(&o->id, sizeof o->id);
        mix(&o->alive, sizeof o->alive);
        mix(&o->energy, sizeof o->energy);
        mix(&o->dcenter_x, sizeof o->dcenter_x);
        mix(&o->dcenter_y, sizeof o->dcenter_y);
        mix(&o->theta, sizeof o->theta);
        mix(&o->dx, sizeof o->dx);
        mix(&o->dy, sizeof o->dy);
        mix(&o->dtheta, sizeof o->dtheta);
        mix(&o->age, sizeof o->age);
        mix(&o->total_children, sizeof o->total_children);
        mix(&o->total_kills, sizeof o->total_kills);
        mix(&o->total_infected, sizeof o->total_infected);
        mix(&o->mass, sizeof o->mass);
        mix(&o->r, sizeof o->r);
    }
    return h;
}

static int64_t birth0, death0, inf0;

static void books(const World *w)
{
    const History *hi = syn_history();
    for (int i = 1; i < hi->n; i++)
        CHECK(hi->s[i].t > hi->s[i - 1].t, "samples out of order at %d", i);
    CHECK(hi->n <= HIST_CAP, "%d samples, over %d", hi->n, HIST_CAP);
    CHECK(hi->stride > 0 && (hi->stride & (hi->stride - 1)) == 0, "stride %d", hi->stride);
    int64_t births = 0, deaths = 0, infections = 0;
    for (int i = 0; i < hi->nepochs; i++) {
        const Epoch *a = &hi->epochs[i];
        CHECK(a->t1 - a->t0 == hi->epoch_base, "epoch %d is %lld long, not %d", i,
              (long long)(a->t1 - a->t0), hi->epoch_base);
        if (i > 0)
            CHECK(a->t0 == hi->epochs[i - 1].t1, "epoch %d starts at %lld, not %lld", i,
                  (long long)a->t0, (long long)hi->epochs[i - 1].t1);
        births += a->births;
        deaths += a->deaths;
        infections += a->infections;
        for (int k = 0; k < CH_COUNT; k++)
            CHECK((a->ch[k].gc != NULL) == (a->ch[k].value >= 0) || a->samples == 0,
                  "epoch %d champion %d: value %g, code %p", i, k, a->ch[k].value,
                  (void *)a->ch[k].gc);
    }
    CHECK(births == w->stats.birth_sum - birth0, "epochs count %lld births, the world %lld",
          (long long)births, (long long)(w->stats.birth_sum - birth0));
    CHECK(deaths == w->stats.death_sum - death0, "epochs count %lld deaths, the world %lld",
          (long long)deaths, (long long)(w->stats.death_sum - death0));
    CHECK(infections == w->stats.infections_sum - inf0, "epochs count %lld infections, the world %lld",
          (long long)infections, (long long)(w->stats.infections_sum - inf0));
    for (int i = 1; i < hi->nev; i++)
        CHECK(hi->ev[i].t >= hi->ev[i - 1].t, "events out of order at %d", i);
    /* an epoch merged from all of them has the best of each */
    Epoch all;
    if (hi->nepochs && syn_history_merge(hi->epochs[0].t0, hi->epochs[hi->nepochs - 1].t1, &all))
        for (int k = 0; k < CH_COUNT; k++)
            for (int i = 0; i < hi->nepochs; i++)
                CHECK(all.ch[k].value >= hi->epochs[i].ch[k].value, "merged champion %d below epoch %d", k, i);
}

static void family_books(const World *w)
{
    int n = syn_family_count(), living = 0, alive = 0, pins = 0;
    for (int i = 0; i < n; i++) {
        const Species *s = syn_family_at(i);
        CHECK(syn_family_get(s->id) == s, "species %u not found by its id", s->id);
        if (s->parent) {
            const Species *p = syn_family_get(s->parent);
            CHECK(p && p < s, "species %u: parent %u missing or founded after it", s->id, s->parent);
            if (p)
                CHECK(s->root == p->root && s->depth == p->depth + 1, "species %u: root %u depth %d, parent's %u %d",
                      s->id, s->root, s->depth, p->root, p->depth);
        } else {
            CHECK(s->root == s->id && s->depth == 0, "root species %u: root %u depth %d", s->id, s->root, s->depth);
        }
        int kids = 0;
        for (int j = i + 1; j < n; j++)
            kids += syn_family_at(j)->parent == s->id;
        CHECK(kids == s->kids, "species %u: %d kids kept, counted %d", s->id, kids, s->kids);
        CHECK(!s->extinct || s->alive == 0, "species %u extinct with %d alive", s->id, s->alive);
        CHECK(!s->extinct || s->notable || s->kids || s->pins, "species %u should have been forgotten", s->id);
        alive += s->alive;
        pins += s->pins;

        /* its family: begun by an ancestor (or itself), named in order */
        const Species *head = syn_family_head(s), *a = s;
        while (a && a != head)
            a = syn_family_get(a->parent);
        CHECK(head && a == head && head->family == head->id, "species %u: family %u not an ancestor's", s->id,
              s->family);
        if (s->parent) {
            const Species *p = syn_family_get(s->parent);
            CHECK(!p || s->family == p->family || s->family == s->id, "species %u: family %u, parent's %u", s->id,
                  s->family, p ? p->family : 0);
        }
        CHECK(!s->notable || s->number > 0, "notable species %u has no number", s->id);
        if (s->number && head)
            CHECK(head->name_no > 0 && s->number <= head->numbered, "species %u: number %d, family %d of %d", s->id,
                  s->number, head->name_no, head->numbered);
        CHECK(s->name_no <= syn_family_named() && (!s->name_no || s->family == s->id),
              "species %u: name %d of %d", s->id, s->name_no, syn_family_named());
        for (int j = i + 1; j < n; j++) {
            const Species *t = syn_family_at(j);
            CHECK(!s->name_no || t->name_no != s->name_no, "species %u and %u share name %d", s->id, t->id, s->name_no);
            CHECK(!s->number || t->family != s->family || t->number != s->number, "species %u and %u both %d",
                  s->id, t->id, s->number);
        }
    }
    for (int i = 0; i < w->norgs; i++) {
        const Organism *o = w->orgs[i];
        if (o->removed || !o->alive)
            continue;
        living++;
        CHECK(syn_family_of(o) != 0, "organism %d in no species", o->id);
    }
    CHECK(alive == living, "species count %d living, the world %d", alive, living);
    const History *hi = syn_history();
    int best = 0;
    for (int i = 0; i < hi->nepochs; i++)
        if (hi->epochs[i].best) {
            best++;
            CHECK(syn_family_get(hi->epochs[i].best) != NULL, "epoch %d's best %u forgotten", i, hi->epochs[i].best);
        }
    CHECK(pins == best, "%d pins for %d epochs' bests", pins, best);
}

/* Saved and loaded back, the tree saves the same; damaged, it is refused
 * and left as it was. */
static void family_round_trip(World *w, bool damaged)
{
    uint8_t *a = NULL, *b = NULL;
    size_t na = syn_family_save(w, &a);
    CHECK(na > 0, "the family tree did not save");
    if (damaged) {
        size_t cut[] = {0, 7, 8, 20, na / 3, na / 2, na - 1};
        for (size_t i = 0; i < sizeof cut / sizeof cut[0]; i++)
            CHECK(cut[i] >= na || !syn_family_load(w, a, cut[i]), "a family tree cut to %zu bytes loaded", cut[i]);
        a[9] ^= 0x55;
        CHECK(!syn_family_load(w, a, na), "a family tree of another format loaded");
        a[9] ^= 0x55;
    }
    CHECK(syn_family_load(w, a, na), "the family tree did not load back");
    size_t nb = syn_family_save(w, &b);
    CHECK(nb == na && memcmp(a, b, na) == 0, "the family tree saved %zu bytes, then %zu different", na, nb);
    free(a);
    free(b);
}

/* The history with the tree, as a world's sidecar keeps them
 * (syn/sidecar.h): after the reset a world's opening makes, loaded back,
 * both save the same; damaged, they are refused and nothing changes. */
static void history_round_trip(World *w, bool damaged)
{
    uint8_t *h1 = NULL, *f1 = NULL, *h2 = NULL, *f2 = NULL;
    size_t hn = syn_history_save(&h1), fn = syn_family_save(w, &f1);
    CHECK(hn > 0 && fn > 0, "the history did not save");
    syn_history_reset(w, true);
    if (damaged && hn > 0 && fn > 0) {
        size_t cut[] = {0, 7, 8, 40, hn / 3, hn / 2, hn - 1};
        for (size_t i = 0; i < sizeof cut / sizeof cut[0]; i++)
            CHECK(cut[i] >= hn || !syn_history_load(w, h1, cut[i], f1, fn, false),
                  "a history cut to %zu bytes loaded", cut[i]);
        CHECK(!syn_history_load(w, h1, hn, f1, fn - 1, false), "a history with a cut tree loaded");
        h1[12] ^= 0x55;
        CHECK(!syn_history_load(w, h1, hn, f1, fn, false), "a history of the other simulation loaded");
        h1[12] ^= 0x55;
    }
    CHECK(syn_history_load(w, h1, hn, f1, fn, false), "the history did not load back");
    size_t hn2 = syn_history_save(&h2), fn2 = syn_family_save(w, &f2);
    CHECK(hn2 == hn && memcmp(h1, h2, hn) == 0, "the history saved %zu bytes, then %zu different",
          hn, hn2);
    CHECK(fn2 == fn && memcmp(f1, f2, fn) == 0,
          "loaded with the history, the tree saved %zu bytes, then %zu different", fn, fn2);
    free(h1);
    free(f1);
    free(h2);
    free(f2);
}

static void removed(void *ud, Organism *o)
{
    (void)ud;
    syn_history_removed(o->world, o);
}

int main(int argc, char **argv)
{
    if (argc != 5) {
        fprintf(stderr, "usage: history_check FRAMES SEED plain|history OUT.bgw\n");
        return 2;
    }
    long frames = strtol(argv[1], NULL, 10);
    bool record = !strcmp(argv[3], "history");
    params_set_defaults(&P);
#ifdef BIO_COLOUR
    /* Color Mod's default world (7000 x 7000, 12,500 organisms) is slow for
     * a check: a smaller one. Recording, its codes are collected every frame,
     * so that one the history held but the collector freed would be caught,
     * and so that the plain run collects at other times: when the collector
     * runs mustn't matter either. */
    P.world_width = P.world_height = 2500;
    P.initial_organisms = 1500;
    if (record)
        jgc_min_fresh = 0;
#endif
    jr_set_seed(&g_rng, strtoll(argv[2], NULL, 10));
    World w;
    world_init(&w);
    if (record) {
        SynGenesis g;
        syn_genesis_begin(&g, &w);
        while (syn_genesis_next(&g))
            ;
    } else {
        world_genesis(&w);
    }
    if (record) {
        w.on_remove = removed;
        birth0 = w.stats.birth_sum;
        death0 = w.stats.death_sum;
        inf0 = w.stats.infections_sum;
        syn_history_reset(&w, false);
    }
    printf("0 %016" PRIx64 "\n", world_hash(&w));
    for (long f = 1; f <= frames; f++) {
        world_time(&w);
        if (record) {
            syn_history_frame(&w);
            if (w.nframes == 0) {
                books(&w);
                family_books(&w);
                /* every 64 time units: the world must run on as if never saved */
                if (w.stats.time % 64 == 0)
                    family_round_trip(&w, w.stats.time % 1024 == 0);
                /* and between, the whole history, as a sidecar holds it */
                if (w.stats.time % 64 == 32)
                    history_round_trip(&w, w.stats.time % 1024 == 32);
            }
        }
        printf("%ld %016" PRIx64 "\n", f, world_hash(&w));
    }
    /* and once at the end, before the world is saved and compared */
    if (record) {
        family_round_trip(&w, true);
        history_round_trip(&w, true);
    }
    printf("rng %016" PRIx64 "\n", g_rng.seed);
    char err[256];
    if (!bgw_save(&w, argv[4], err, sizeof err)) {
        fprintf(stderr, "%s: %s\n", argv[4], err);
        return 1;
    }
    if (record) {
        const History *hi = syn_history();
        int roots = 0;
        for (int i = 0; i < syn_family_count(); i++)
            roots += syn_family_at(i)->parent == 0;
        int named = 0, most = 0;
        for (int i = 0; i < syn_family_count(); i++) {
            named += syn_family_at(i)->number > 0;
            if (syn_family_at(i)->numbered > most)
                most = syn_family_at(i)->numbered;
        }
        fprintf(stderr, "history: %d samples (every %d units), %d epochs of %d units, %d events, %d species kept (%d roots)\n",
                hi->n, hi->stride, hi->nepochs, hi->epoch_base, hi->nev, syn_family_count(), roots);
        char last[48] = "none";
        if (syn_family_named())
            syn_family_title(syn_family_named(), last, sizeof last);
        fprintf(stderr, "names: %d families (to %s), %d species numbered, at most %d in one family\n",
                syn_family_named(), last, named, most);
        syn_history_free();
    }
    world_free(&w);
    return failures ? 1 : 0;
}
