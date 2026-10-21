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

#include "syn/telescope.h"

/* Color Mod draws its genes differently; for now the telescope is 0.9's. */
#ifndef BIO_COLOUR

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "io/bgg.h"
#include "io/bgw.h"
#include "io/worldfile.h"
#include "sim/genetic_code.h"
#include "sim/organism.h"
#include "sim/params.h"
#include "sim/world.h"
#include "syn/crack.h"
#include "syn/arena_os.h"
#include "syn/record.h"
#include "syn/round.h"

#define TEL_A 0x5DEECE66DULL
#define TEL_B 0xBULL

/* x -> a x + b (mod 2^48) */
typedef struct {
    uint64_t a, b;
} Affine;

/* f after g */
static Affine compose(Affine f, Affine g)
{
    return (Affine){(f.a * g.a) & TEL_MASK, (f.a * g.b + f.b) & TEL_MASK};
}

/* a^-1 mod 2^64 for odd a, by Newton's iteration: a is its own inverse to
 * 3 bits, and each step doubles the bits that are right. */
static uint64_t inverse_odd(uint64_t a)
{
    uint64_t x = a;
    for (int i = 0; i < 5; i++)
        x *= 2 - a * x;
    return x;
}

/* One draw back: x = A^-1 (x' - B). */
static Affine step_back(void)
{
    uint64_t ai = inverse_odd(TEL_A) & TEL_MASK;
    return (Affine){ai, (0 - ai * TEL_B) & TEL_MASK};
}

uint64_t tel_jump(uint64_t state, int64_t n)
{
    Affine step = n < 0 ? step_back() : (Affine){TEL_A, TEL_B};
    uint64_t k = n < 0 ? 0 - (uint64_t)n : (uint64_t)n;
    Affine r = {1, 0};
    for (; k; k >>= 1) {
        if (k & 1)
            r = compose(step, r);
        step = compose(step, step);
    }
    return (r.a * (state & TEL_MASK) + r.b) & TEL_MASK;
}

/* Bit k of the count: once `x`, the state after the bits below it, agrees
 * with `to` in its low k bits, 2^k draws more keep those bits and flip bit
 * k (the low k + 1 bits go round every 2^(k+1) draws, all values in turn),
 * so they are needed exactly when bit k still disagrees. */
uint64_t tel_draws(uint64_t from, uint64_t to)
{
    Affine step = {TEL_A, TEL_B}; /* 2^k draws */
    uint64_t x = from & TEL_MASK, n = 0;
    to &= TEL_MASK;
    for (int k = 0; k < 48; k++) {
        if ((x ^ to) >> k & 1) {
            x = (step.a * x + step.b) & TEL_MASK;
            n |= 1ULL << k;
        }
        step = compose(step, step);
    }
    return n;
}

uint64_t tel_seed_state(int64_t seed)
{
    return ((uint64_t)seed ^ TEL_A) & TEL_MASK;
}

int64_t tel_state_seed(uint64_t state)
{
    return (int64_t)((state ^ TEL_A) & TEL_MASK);
}

/* nextDouble() is (next(26) << 27 | next(27)) / 2^53: the top 26 bits of
 * one state, then the top 27 bits of the next. The 22 bits below the first
 * are tried one by one (4 million, some milliseconds), and the next state
 * must agree; each state that does is returned as the one before the draw. */
static int crack_double(uint64_t k, uint64_t *out, int n, int max)
{
    uint64_t hi = k >> 27, lo = k & ((1ULL << 27) - 1);
    uint64_t s1 = hi << 22;
    uint64_t s2 = (s1 * TEL_A + TEL_B) & TEL_MASK;
    for (uint64_t x = 0; x < 1ULL << 22; x++) {
        if (s2 >> 21 == lo) {
            if (n < max)
                out[n] = tel_jump(s1, -1);
            n++;
        }
        s1++;
        s2 = (s2 + TEL_A) & TEL_MASK;
    }
    return n;
}

/* gene_randomize_length and gene_randomize_theta, from u = k / 2^53 */
static double length_of(uint64_t k)
{
    return 2.0 + (double)k * 0x1.0p-53 * 16.0;
}

static double theta_of(uint64_t k)
{
    return (double)k * 0x1.0p-53 * 2.0 * M_PI;
}

int tel_crack_length(double length, uint64_t *out, int max)
{
    if (!(length >= 2.0 && length < 18.0))
        return 0;
    /* exact: 2 is a multiple of the length's last place, and the rest is
     * powers of two; from 16 up the length has lost a bit, hence the
     * neighbours */
    int64_t k0 = (int64_t)((length - 2.0) / 16.0 * 0x1.0p53);
    int n = 0;
    for (int64_t k = k0 - 4; k <= k0 + 4; k++)
        if (k >= 0 && k < (1LL << 53) && length_of((uint64_t)k) == length)
            n = crack_double((uint64_t)k, out, n, max);
    return n;
}

int tel_crack_theta(double theta, uint64_t *out, int max)
{
    if (!(theta >= 0.0 && theta < 2.0 * M_PI))
        return 0;
    /* times pi rounds, so a few neighbours can make the same angle */
    int64_t k0 = (int64_t)(theta / (2.0 * M_PI) * 0x1.0p53);
    int n = 0;
    for (int64_t k = k0 - 16; k <= k0 + 16; k++)
        if (k >= 0 && k < (1LL << 53) && theta_of((uint64_t)k) == theta)
            n = crack_double((uint64_t)k, out, n, max);
    return n;
}

/* the draw a state makes as nextDouble(), and the state after it */
static uint64_t draw_double(uint64_t *s)
{
    uint64_t s1 = tel_jump(*s, 1), s2 = tel_jump(s1, 1);
    *s = s2;
    return (s1 >> 22) << 27 | s2 >> 21;
}

static uint64_t dbits(double d)
{
    uint64_t u;
    memcpy(&u, &d, sizeof u);
    return u;
}

static int cmp_u64(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return x < y ? -1 : x > y;
}

static bool parse_u64(const char *s, uint64_t *v)
{
    char *end;
    *v = strtoull(s, &end, 0);
    return *s && !*end;
}

static bool parse_i64(const char *s, int64_t *v)
{
    char *end;
    *v = strtoll(s, &end, 0);
    return *s && !*end;
}

static bool parse_double(const char *s, double *v)
{
    char *end;
    *v = strtod(s, &end);
    return *s && !*end;
}

/* --- the seed of a saved world --- */

/* A gene length in the saved world, and how many genes have it. */
typedef struct {
    uint64_t bits;
    int count;
} Fossil;

static int cmp_fossil(const void *a, const void *b)
{
    const Fossil *x = a, *y = b;
    if (x->count != y->count)
        return y->count - x->count;
    return x->bits < y->bits ? -1 : x->bits > y->bits;
}

static void add_genes(const GeneticCode *gc, uint64_t **v, int *n, int *cap)
{
    if (!gc)
        return;
    for (int j = 0; j < gc->ngenes; j++) {
        if (*n == *cap) {
            *cap = *cap ? *cap * 2 : 1024;
            *v = realloc(*v, (size_t)*cap * sizeof **v);
            if (!*v)
                abort();
        }
        (*v)[(*n)++] = dbits(gc->genes[j].length);
    }
}

static bool same_code(const GeneticCode *g, const GeneticCode *h)
{
    if (g->ngenes != h->ngenes || g->symmetry != h->symmetry || g->mirror != h->mirror ||
        g->disperse_children != h->disperse_children)
        return false;
    for (int j = 0; j < g->ngenes; j++)
        if (dbits(g->genes[j].length) != dbits(h->genes[j].length) ||
            dbits(g->genes[j].theta) != dbits(h->genes[j].theta) ||
            g->genes[j].color != h->genes[j].color)
            return false;
    return true;
}

/* The first difference between two worlds, in what the Java harness
 * (tools/javaref) dumps plus the genes; NULL if there is none. */
static const char *world_difference(const World *a, const World *b, char *why, size_t n)
{
#define DIFF(...)                                                                                 \
    do {                                                                                          \
        snprintf(why, n, __VA_ARGS__);                                                            \
        return why;                                                                               \
    } while (0)
    if (a->width != b->width || a->height != b->height)
        DIFF("the world's size");
    if (a->stats.time != b->stats.time || a->nframes != b->nframes)
        DIFF("the time");
    if (a->norgs != b->norgs || a->population != b->population)
        DIFF("%d organisms (%d alive) against %d (%d alive)", a->norgs, a->population, b->norgs,
             b->population);
    if (dbits(a->o2) != dbits(b->o2) || dbits(a->co2) != dbits(b->co2))
        DIFF("the O2 or CO2");
    if (a->next_id != b->next_id)
        DIFF("the next organism's id");
    for (int i = 0; i < a->norgs; i++) {
        const Organism *p = a->orgs[i], *q = b->orgs[i];
        if (p->id != q->id || p->alive != q->alive || p->age != q->age ||
            p->growth_ratio != q->growth_ratio || p->total_kills != q->total_kills)
            DIFF("organism %d (id %d)", i, p->id);
        if (dbits(p->energy) != dbits(q->energy) || dbits(p->dcenter_x) != dbits(q->dcenter_x) ||
            dbits(p->dcenter_y) != dbits(q->dcenter_y) || dbits(p->theta) != dbits(q->theta) ||
            dbits(p->dx) != dbits(q->dx) || dbits(p->dy) != dbits(q->dy) ||
            dbits(p->dtheta) != dbits(q->dtheta))
            DIFF("organism %d (id %d): its energy, place or movement", i, p->id);
        if (p->r.x != q->r.x || p->r.y != q->r.y || p->r.width != q->r.width ||
            p->r.height != q->r.height)
            DIFF("organism %d (id %d): its bounds", i, p->id);
        if (!same_code(p->gc, q->gc))
            DIFF("organism %d (id %d): its genetic code", i, p->id);
    }
    return NULL;
#undef DIFF
}

/* The founders world_genesis makes from this state: whether one of their
 * genes has the length wanted (any, for 0), and then how many of their
 * genes' lengths are in the saved world (of how many). */
static bool founders_have(World *scratch, uint64_t state, uint64_t want, const uint64_t *have,
                          int nhave, int *found, int *total)
{
    g_rng.seed = state;
    world_genesis(scratch);
    bool hit = want == 0;
    for (int i = 0; i < scratch->norgs && !hit; i++) {
        const GeneticCode *gc = scratch->orgs[i]->gc;
        for (int j = 0; j < gc->ngenes; j++)
            if (dbits(gc->genes[j].length) == want)
                hit = true;
    }
    if (!hit)
        return false;
    *found = *total = 0;
    for (int i = 0; i < scratch->norgs; i++) {
        const GeneticCode *gc = scratch->orgs[i]->gc;
        for (int j = 0; j < gc->ngenes; j++) {
            uint64_t b = dbits(gc->genes[j].length);
            (*total)++;
            if (bsearch(&b, have, (size_t)nhave, sizeof *have, cmp_u64))
                (*found)++;
        }
    }
    return true;
}

/* A seed whose founders have the gene: the state it starts at, how many
 * steps before the gene it is, and how many of its founders' genes have
 * lengths still in the saved world. */
typedef struct {
    uint64_t state;
    int64_t draws;
    int found, total;
    int fossil;
} Candidate;

/* most founder genes still in the world first, then the nearest */
static int cmp_candidate(const void *a, const void *b)
{
    const Candidate *x = a, *y = b;
    if (x->found != y->found)
        return y->found - x->found;
    return x->draws < y->draws ? -1 : x->draws > y->draws;
}

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + ts.tv_nsec / 1e9;
}

/* A saved world's statistics keep records with the time unit each was set
 * in: the highest and lowest O2, CO2 and population, and the organisms
 * with the most children, kills and infections. Once a replay is past that
 * time its record must be the saved one exactly, and a different world
 * almost never matches them, so a wrong seed is turned away at the
 * earliest record instead of at the saved tick. The O2 and CO2 records are
 * set as a time unit ends (with the unit's new number), the others during
 * one (with the number of units already ended). */
static const char *record_difference(const WorldStats *r, const WorldStats *s)
{
    int64_t t = r->time;
#define AT_END(field, name)                                                                       \
    if (s->field##_time <= t &&                                                                   \
        (dbits(r->field) != dbits(s->field) || r->field##_time != s->field##_time))               \
        return name;
#define DURING(field, value, name)                                                                \
    if (s->field##_time < t && (r->value != s->value || r->field##_time != s->field##_time))      \
        return name;
    AT_END(max_oxygen, "the highest O2")
    AT_END(min_oxygen, "the lowest O2")
    AT_END(max_co2, "the highest CO2")
    AT_END(min_co2, "the lowest CO2")
    DURING(max_population, max_population, "the highest population")
    DURING(min_population, min_population, "the lowest population")
    DURING(most_children, most_children_n, "the most children")
    DURING(most_kills, most_kills_n, "the most kills")
    DURING(most_infections, most_infections_n, "the most infections")
    return NULL;
#undef AT_END
#undef DURING
}

/* The first time unit at which some record can turn a wrong seed away.
 * Records of time unit 0 are left out: most are the values every world
 * starts with. */
static int64_t first_record(const WorldStats *s)
{
    int64_t at_end[] = {s->max_oxygen_time, s->min_oxygen_time, s->max_co2_time,
                        s->min_co2_time};
    int64_t during[] = {s->max_population_time, s->min_population_time, s->most_children_time,
                        s->most_kills_time, s->most_infections_time};
    int64_t m = INT64_MAX;
    for (size_t i = 0; i < sizeof at_end / sizeof *at_end; i++)
        if (at_end[i] >= 1 && at_end[i] < m)
            m = at_end[i];
    for (size_t i = 0; i < sizeof during / sizeof *during; i++)
        if (during[i] >= 1 && during[i] + 1 < m)
            m = during[i] + 1;
    return m == INT64_MAX ? 0 : m;
}

/* The saved world's oldest living organism: its age gives the tick it was
 * born at, so a replay that is the saved world has an organism with its
 * id, its genes and exactly the age it should have from a few ticks after
 * that birth on. Returns the tick to check at (0 for none). */
static int64_t oldest_check(const World *saved, int64_t ticks, const Organism **oldest)
{
    *oldest = NULL;
    for (int i = 0; i < saved->norgs; i++) {
        const Organism *o = saved->orgs[i];
        if (o->alive && (!*oldest || o->age > (*oldest)->age))
            *oldest = o;
    }
    if (!*oldest || (*oldest)->age < 16)
        return 0;
    return ticks - (*oldest)->age + 8;
}

/* Replays the world from this state to the saved tick, checking the
 * saved records and the oldest organism's birth as it passes them:
 * whether it is the saved world. */
static bool replay_matches(uint64_t state, const World *saved, int64_t ticks)
{
    const Organism *oldest;
    int64_t born_check = oldest_check(saved, ticks, &oldest);
    World r;
    world_init(&r);
    g_rng.seed = state;
    world_genesis(&r);
    double t1 = now_s();
    const char *d = NULL;
    char why[160];
    int64_t t = 0;
    while (t < ticks && !d) {
        int64_t units = r.stats.time;
        world_time(&r);
        t++;
        if (r.stats.time != units)
            d = record_difference(&r.stats, &saved->stats);
        if (t == born_check) {
            const Organism *o = NULL;
            for (int i = 0; i < r.norgs && !o; i++)
                if (r.orgs[i]->id == oldest->id)
                    o = r.orgs[i];
            if (!o || !o->alive || o->age != oldest->age - (ticks - t) ||
                !same_code(o->gc, oldest->gc)) {
                snprintf(why, sizeof why, "organism %d, the oldest alive at the save,", oldest->id);
                d = why;
            }
        }
    }
    if (!d)
        d = world_difference(&r, saved, why, sizeof why);
    if (d && t < ticks) {
        printf("    turned away at tick %" PRId64 ": %s is not the saved world's\n", t, d);
    } else if (d) {
        printf("    replayed to tick %" PRId64 ": not this world (%s differs)\n", ticks, d);
    } else {
        printf("    replayed to tick %" PRId64 " in %.1f s: identical to the saved world\n", ticks,
               now_s() - t1);
        printf("seed %" PRId64 " (and every seed equal to it mod 2^48)\n", tel_state_seed(state));
        printf("the generator at the saved tick: 0x%012" PRIx64 "\n", g_rng.seed);
    }
    world_free(&r);
    return !d;
}

static int cmd_world(int argc, char **argv)
{
    const char *path = NULL;
    bool defaults = false, replay = true, strong = false, given_seed = false, progress = false;
    int64_t back = 4096, only_seed = 0;
    int max_fossils = 0;
    for (int i = 2; i < argc; i++) {
        const char *a = argv[i], *v = i + 1 < argc ? argv[i + 1] : NULL;
        if (!strcmp(a, "--defaults")) {
            defaults = true;
        } else if (!strcmp(a, "--no-replay")) {
            replay = false;
        } else if (!strcmp(a, "--strong")) {
            strong = true;
        } else if (!strcmp(a, "--progress")) {
            progress = true;
        } else if (!strcmp(a, "--seed") && v && parse_i64(v, &only_seed)) {
            given_seed = true;
            i++;
        } else if (!strcmp(a, "--back") && v && parse_i64(v, &back) && back >= 0) {
            i++;
        } else if (!strcmp(a, "--lengths") && v) {
            max_fossils = atoi(v);
            i++;
        } else if (!path && a[0] != '-') {
            path = a;
        } else {
            return 2;
        }
    }
    if (!path)
        return 2;
    /* for the window (syn/telescope_screen.c): each line as it is
     * printed, the answer on a line of its own, and gone with the window */
    if (progress) {
        arena_os_child_begin();
        setvbuf(stdout, NULL, _IOLBF, 0);
    }

    params_set_defaults(&P);
    if (!defaults)
        params_load();
    World saved;
    world_init(&saved);
    char err[256];
    bool ok = bgw_sniff(path) ? bgw_load(&saved, path, err, sizeof err)
                              : worldfile_load(&saved, path, err, sizeof err);
    if (!ok) {
        fprintf(stderr, "%s: %s\n", path, err);
        return 1;
    }
    /* the save says what size the world was made at */
    P.world_width = saved.width;
    P.world_height = saved.height;
    int64_t ticks = saved.stats.time * 256 + saved.nframes;

    /* every gene length in the world, alive and dead, and the codes
     * waiting to infect */
    uint64_t *all = NULL;
    int nall = 0, cap = 0;
    for (int i = 0; i < saved.norgs; i++) {
        add_genes(saved.orgs[i]->gc, &all, &nall, &cap);
        add_genes(saved.orgs[i]->infected_gc, &all, &nall, &cap);
    }
    qsort(all, (size_t)nall, sizeof *all, cmp_u64);
    Fossil *fos = malloc((size_t)(nall ? nall : 1) * sizeof *fos);
    uint64_t *have = malloc((size_t)(nall ? nall : 1) * sizeof *have);
    if (!fos || !have)
        abort();
    int nfos = 0;
    for (int i = 0; i < nall; i++) {
        if (nfos && fos[nfos - 1].bits == all[i]) {
            fos[nfos - 1].count++;
        } else {
            have[nfos] = all[i];
            fos[nfos++] = (Fossil){all[i], 1};
        }
    }
    free(all);
    qsort(fos, (size_t)nfos, sizeof *fos, cmp_fossil);
    printf("%s: %d x %d, tick %" PRId64 ", %d organisms (%d alive), %d different gene lengths\n",
           path, saved.width, saved.height, ticks, saved.norgs, saved.population, nfos);

    World scratch;
    world_init(&scratch);
    if (given_seed) {
        uint64_t x = tel_seed_state(only_seed);
        int found, total;
        founders_have(&scratch, x, 0, have, nfos, &found, &total);
        printf("  seed %" PRId64 ": %d of its founders' %d genes still in the world\n",
               tel_state_seed(x), found, total);
        int r = replay && !replay_matches(x, &saved, ticks);
        world_free(&scratch);
        world_free(&saved);
        free(fos);
        free(have);
        return r;
    }

    /* A founder's gene still in the world was drawn at tick 0, a few
     * hundred draws after the seed: crack its length, step back up to
     * `back` draws, and make the founders from each state on the way.
     * Starting a founder's worth of draws later or earlier makes a near
     * twin of the world, with most of the same founders, so the seeds
     * found are ranked by how many of their founders' genes are still in
     * the saved world, and replayed in that order until one is the world.
     * A seed whose founders share no other gene with the world has nothing
     * but the gene itself for it, and every gene has dozens of those: they
     * wait for a second pass, after every gene's better-supported seeds. */
    int64_t early = first_record(&saved.stats) * 256;
    const Organism *oldest;
    int64_t born = oldest_check(&saved, ticks, &oldest);
    if (early > 0 && early < ticks)
        printf("the save's records go back to tick %" PRId64 "\n", early);
    if (born > 0)
        printf("its oldest organism, %d, was born %d ticks before the save\n", oldest->id,
               oldest->age);
    if ((early > 0 && early < ticks) || born > 0)
        printf("a wrong seed is turned away by tick %" PRId64 "\n",
               early > 0 && early < ticks && (born <= 0 || early < born) ? early : born);
    Affine prev = step_back();
    double t0 = now_s();
    int tried = 0, replays = 0, listed = 0;
    int64_t found_seed = 0;
    uint64_t found_state = 0;
    int64_t geneses = 0;
    Candidate *cand = NULL, *weak = NULL;
    int ncand = 0, candcap = 0, nweak = 0, weakcap = 0;
    int64_t *tested = NULL;
    int ntested = 0, testedcap = 0;
    int status = 1;
    if (max_fossils <= 0 || max_fossils > nfos)
        max_fossils = nfos;
    for (int pass = 0; pass < 2 && status; pass++) {
        if (pass == 1) {
            if (strong || !nweak || !replay)
                break;
            printf("%d seed%s whose founders share only the gene they were found from:\n", nweak,
                   nweak == 1 ? "" : "s");
        }
        for (int f = 0; f < (pass ? 1 : max_fossils) && status; f++) {
            if (pass == 0) {
                double length;
                memcpy(&length, &fos[f].bits, sizeof length);
                uint64_t st[8];
                int nst = tel_crack_length(length, st, 8);
                tried++;
                ncand = 0;
                for (int k = 0; k < nst && k < 8; k++) {
                    uint64_t x = st[k];
                    for (int64_t nb = 0; nb <= back;
                         nb++, x = (prev.a * x + prev.b) & TEL_MASK) {
                        int found, total;
                        geneses++;
                        if (!founders_have(&scratch, x, fos[f].bits, have, nfos, &found, &total))
                            continue;
                        bool seen = false;
                        for (int c = 0; c < ncand && !seen; c++)
                            seen = cand[c].state == x;
                        if (seen)
                            continue;
                        Candidate c = {x, nb, found, total, f};
                        if (found < 2) {
                            if (nweak == weakcap) {
                                weakcap = weakcap ? weakcap * 2 : 64;
                                weak = realloc(weak, (size_t)weakcap * sizeof *weak);
                                if (!weak)
                                    abort();
                            }
                            weak[nweak++] = c;
                            continue;
                        }
                        if (ncand == candcap) {
                            candcap = candcap ? candcap * 2 : 64;
                            cand = realloc(cand, (size_t)candcap * sizeof *cand);
                            if (!cand)
                                abort();
                        }
                        cand[ncand++] = c;
                    }
                }
                if (!ncand)
                    continue;
                qsort(cand, (size_t)ncand, sizeof *cand, cmp_candidate);
                printf("gene length %.17g (%d in the world): %d seed%s whose founders have it and"
                       " more\n",
                       length, fos[f].count, ncand, ncand == 1 ? "" : "s");
            }
            Candidate *list = pass ? weak : cand;
            int n = pass ? nweak : ncand;
            for (int c = 0; c < n && status; c++) {
                int64_t seed = tel_state_seed(list[c].state);
                bool seen = false;
                for (int k = 0; k < ntested && !seen; k++)
                    seen = tested[k] == seed;
                if (seen)
                    continue;
                double length;
                memcpy(&length, &fos[list[c].fossil].bits, sizeof length);
                printf("  seed %" PRId64 ": gene length %.17g drawn %" PRId64
                       " steps after it; %d of its founders' %d genes in the world\n",
                       seed, length, list[c].draws, list[c].found, list[c].total);
                if (!replay) {
                    listed++;
                    continue;
                }
                if (ntested == testedcap) {
                    testedcap = testedcap ? testedcap * 2 : 64;
                    tested = realloc(tested, (size_t)testedcap * sizeof *tested);
                    if (!tested)
                        abort();
                }
                tested[ntested++] = seed;
                replays++;
                if (replay_matches(list[c].state, &saved, ticks)) {
                    status = 0;
                    found_seed = seed;
                    found_state = g_rng.seed;
                }
            }
        }
    }
    if (progress)
        printf(status ? "@none\n" : "@found %" PRId64 " 0x%012" PRIx64 "\n",
               found_seed, found_state);
    if (!replay) {
        printf("%d seed%s whose founders share two genes or more with the world, %d sharing only"
               " one, from %d gene lengths in %.1f s (none replayed)\n",
               listed, listed == 1 ? "" : "s", nweak, tried, now_s() - t0);
        status = listed ? 0 : 1;
    } else if (status) {
        printf("no seed found: %d gene lengths tried, %" PRId64 " draws back each, %d replay%s,"
               " in %.1f s\n",
               tried, back, replays, replays == 1 ? "" : "s", now_s() - t0);
        if (strong && nweak)
            printf("%d seeds whose founders share only one gene with the world were not replayed"
                   " (--strong)\n",
                   nweak);
    } else {
        printf("%d gene length%s, %" PRId64 " founder sets made, %d replay%s, in %.1f s\n", tried,
               tried == 1 ? "" : "s", geneses, replays, replays == 1 ? "" : "s", now_s() - t0);
    }
    free(weak);
    free(cand);
    free(tested);
    world_free(&scratch);
    world_free(&saved);
    free(fos);
    free(have);
    return status;
}

/* --- the fade: what of the seed a running world still shows --- */

/* Every gene length was one draw: a mark at its place in the stream. A
 * table of the marks seen, by the length's bits, with the place each was
 * drawn at (counted from the seed's first state, -1: no state before
 * `now` draws it, so not this world's draw). */
typedef struct {
    uint64_t *bits;
    int64_t *at;
    size_t cap, n;
} Marks;

static size_t mark_slot(const Marks *m, uint64_t bits)
{
    size_t i = (size_t)((bits * 0x9e3779b97f4a7c15ULL) >> 20) & (m->cap - 1);
    while (m->bits[i] && m->bits[i] != bits)
        i = (i + 1) & (m->cap - 1);
    return i;
}

/* The place `length` was drawn at, before `now` draws from `s0`. */
static int64_t mark_at(Marks *m, double length, uint64_t s0, int64_t now)
{
    uint64_t bits = dbits(length);
    if (m->n * 2 >= m->cap) {
        Marks g = {calloc(m->cap * 2, sizeof *g.bits), malloc(m->cap * 2 * sizeof *g.at),
                   m->cap * 2, 0};
        if (!g.bits || !g.at)
            abort();
        for (size_t i = 0; i < m->cap; i++)
            if (m->bits[i]) {
                size_t j = mark_slot(&g, m->bits[i]);
                g.bits[j] = m->bits[i];
                g.at[j] = m->at[i];
                g.n++;
            }
        free(m->bits);
        free(m->at);
        *m = g;
    }
    size_t i = mark_slot(m, bits);
    if (m->bits[i])
        return m->at[i];
    uint64_t st[8];
    int k = tel_crack_length(length, st, 8);
    int64_t at = -1;
    for (int j = 0; j < k && j < 8; j++) {
        uint64_t d = tel_draws(s0, st[j]);
        if ((int64_t)d < now && (at < 0 || (int64_t)d < at))
            at = (int64_t)d;
    }
    m->bits[i] = bits;
    m->at[i] = at;
    m->n++;
    return at;
}

/* The frame a draw was made in, from the draws after each frame. */
static int64_t frame_of(const int64_t *after, int64_t frames, int64_t draw)
{
    int64_t lo = 0, hi = frames;
    while (lo < hi) {
        int64_t mid = lo + (hi - lo) / 2;
        if (after[mid] > draw)
            hi = mid;
        else
            lo = mid + 1;
    }
    return lo;
}

/* A world run from a seed, and every EVERY frames what of the seed is
 * still in it, read the way the telescope reads a save: every living
 * gene's length cracked to its state and counted from the seed. */
static int cmd_fade(int argc, char **argv)
{
    int64_t seed, frames, every;
    if (argc < 5 || !parse_i64(argv[2], &seed) || !parse_i64(argv[3], &frames) ||
        !parse_i64(argv[4], &every) || frames < 1 || every < 1)
        return 2;
    bool defaults = false, marks = false;
    int w = 0, h = 0, n = -1;
    for (int i = 5; i < argc; i++) {
        if (!strcmp(argv[i], "--defaults"))
            defaults = true;
        else if (!strcmp(argv[i], "--marks"))
            marks = true;
        else if (!strcmp(argv[i], "--world") && i + 3 < argc) {
            w = atoi(argv[i + 1]);
            h = atoi(argv[i + 2]);
            n = atoi(argv[i + 3]);
            i += 3;
        } else
            return 2;
    }
    params_set_defaults(&P);
    if (!defaults)
        params_load();
    if (w > 0 && h > 0 && n >= 0) {
        P.world_width = w;
        P.world_height = h;
        P.initial_organisms = n;
    }
    World wd;
    world_init(&wd);
    jr_set_seed(&g_rng, seed);
    uint64_t s0 = g_rng.seed;
    world_genesis(&wd);
    /* each founder its own bloodline (the mark a mutated child keeps) */
    for (int i = 0; i < wd.norgs; i++)
        wd.orgs[i]->gc->lineage = (uint32_t)i + 1;
    int64_t *after = malloc((size_t)(frames + 1) * sizeof *after);
    if (!after)
        abort();
    after[0] = (int64_t)tel_draws(s0, g_rng.seed);
    Marks mk = {calloc(1024, sizeof *mk.bits), malloc(1024 * sizeof *mk.at), 1024, 0};
    if (!mk.bits || !mk.at)
        abort();
    printf("# seed %" PRId64 ", %d x %d, %d first organisms, genesis made in %" PRId64 " draws\n",
           seed, P.world_width, P.world_height, wd.norgs, after[0]);
    printf("# S tick alive draws genes founder_genes oldest_draw oldest_tick unread lines o2 co2"
           " green red cyan blue white gray yellow genes_per symmetry\n");
    int64_t *birth = NULL;
    size_t nbirth = 0, capbirth = 0;
    for (int64_t f = 0; f <= frames; f++) {
        if (f > 0) {
            world_time(&wd);
            after[f] = (int64_t)tel_draws(s0, g_rng.seed);
        }
        if (f % every)
            continue;
        int64_t now = after[f];
        int alive = 0, genes = 0, founder = 0, unread = 0;
        int64_t oldest = -1;
        double col[GENE_COLORS] = {0}, mass = 0, sym = 0;
        uint32_t *lin = malloc((size_t)(wd.norgs + 1) * sizeof *lin);
        int nlin = 0;
        nbirth = 0;
        for (int i = 0; i < wd.norgs; i++) {
            const Organism *o = wd.orgs[i];
            if (o->removed || !o->alive)
                continue;
            alive++;
            const GeneticCode *gc = o->gc;
            bool seen = false;
            for (int k = 0; k < nlin && !seen; k++)
                seen = lin[k] == gc->lineage;
            if (!seen)
                lin[nlin++] = gc->lineage;
            sym += gc->symmetry;
            for (int j = 0; j < gc->ngenes; j++) {
                const Gene *g = &gc->genes[j];
                genes++;
                double m = g->length * gc->symmetry;
                if (g->color < GENE_COLORS)
                    col[g->color] += m;
                mass += m;
                int64_t at = mark_at(&mk, g->length, s0, now);
                if (at < 0) {
                    unread++;
                    continue;
                }
                if (at < after[0])
                    founder++;
                if (oldest < 0 || at < oldest)
                    oldest = at;
                if (marks) {
                    if (nbirth == capbirth) {
                        capbirth = capbirth ? capbirth * 2 : 256;
                        birth = realloc(birth, capbirth * sizeof *birth);
                        if (!birth)
                            abort();
                    }
                    birth[nbirth++] = at < after[0] ? 0 : frame_of(after, f, at);
                }
            }
        }
        free(lin);
        printf("S %" PRId64 " %d %" PRId64 " %d %d %" PRId64 " %" PRId64 " %d %d %.6g %.6g", f,
               alive, now, genes, founder, oldest,
               oldest < 0 ? -1 : oldest < after[0] ? 0 : frame_of(after, f, oldest), unread, nlin,
               wd.o2, wd.co2);
        for (int c = 0; c < GENE_COLORS; c++)
            printf(" %.4f", mass > 0 ? col[c] / mass : 0);
        printf(" %.3f %.3f\n", alive ? (double)genes / alive : 0, alive ? sym / alive : 0);
        if (marks && nbirth) {
            /* the living genes by the time unit they were drawn in */
            qsort(birth, nbirth, sizeof *birth, cmp_u64);
            for (size_t a = 0; a < nbirth;) {
                size_t b = a;
                while (b < nbirth && birth[b] / 256 == birth[a] / 256)
                    b++;
                printf("M %" PRId64 " %" PRId64 " %zu\n", f, birth[a] / 256, b - a);
                a = b;
            }
        }
        fflush(stdout);
        if (!alive)
            break;
    }
    free(birth);
    free(mk.bits);
    free(mk.at);
    free(after);
    world_free(&wd);
    return 0;
}

/* --- where a being lives: its genes read back to the seeds they came from --- */

int tel_code_distance(const GeneticCode *a, const GeneticCode *b)
{
    int d = (a->symmetry != b->symmetry) + (a->mirror != b->mirror);
    int n = a->ngenes < b->ngenes ? a->ngenes : b->ngenes;
    d += a->ngenes > b->ngenes ? a->ngenes - b->ngenes : b->ngenes - a->ngenes;
    for (int j = 0; j < n; j++)
        d += dbits(a->genes[j].length) != dbits(b->genes[j].length) ||
             dbits(a->genes[j].theta) != dbits(b->genes[j].theta) || a->genes[j].color != b->genes[j].color;
    return d;
}

/* The parameters a search runs with: as saved (or the defaults), in a
 * world of its own size and first organisms if given. */
typedef struct {
    int w, h, n; /* n < 0: the parameters' own */
} FindSet;

static void set_params(const FindSet *s, bool defaults)
{
    params_set_defaults(&P);
    if (!defaults)
        params_load();
    if (s->n >= 0) {
        P.world_width = s->w;
        P.world_height = s->h;
        P.initial_organisms = s->n;
    }
}

#define ROUND_ARC 30

/* What a run shows of the being: a frame and what is there. */
typedef struct {
    int64_t frame;
    const char *kind; /* founder, born, alive, most, nearest, ... */
    int n, d;
} Scene;

#define SCENES 24

/* A world from its first state, run and watched for the being once a
 * time unit: when it is alive (the first time, and when most), else its
 * nearest relative. A seed a few draws from the true one makes a near
 * twin: much the same stream, so the being's genes are all in it too, but
 * another world, in which they never come together; with `twins`, such a
 * world is given up a little after its stream has drawn the being's
 * newest gene (at place `newest` from s0) with nothing like it alive. The
 * frame that draw was made in is where the being was born, when it is
 * alive straight after. Whether it was seen alive. */
static bool watch(World *w, uint64_t s0, const int64_t *place, const int *gene, int nplace,
                  const GeneticCode *gc, int64_t frames_max, bool twins, Scene *sc, int *nsc, int64_t *to)
{
    int64_t genesis = (int64_t)tel_draws(s0, g_rng.seed), newest = -1;
    int half = gc->ngenes / 2;
    *nsc = 0;
    /* its genes the founders have, then each other one where it was drawn */
    bool drawn[256] = {0};
    for (int k = 0; k < nplace && k < 256; k++) {
        newest = place[k] > newest ? place[k] : newest;
        if (place[k] < genesis && *nsc < SCENES) {
            sc[(*nsc)++] = (Scene){0, "founder", gene[k], 0};
            drawn[k] = true;
        }
    }
    int64_t alive_at = -1, near_at = -1, drawn_at = newest < genesis ? 0 : -1, peak_at = -1;
    int near = half + 1, copies = 0, peak = 0;
    int64_t f;
    for (f = 1; f <= frames_max; f++) {
        world_time(w);
        if (drawn_at < 0 || !twins) {
            int64_t now = (int64_t)tel_draws(s0, g_rng.seed);
            if (drawn_at < 0 && now > newest)
                drawn_at = f;
            for (int k = 0; k < nplace && k < 256; k++)
                if (!drawn[k] && now > place[k]) {
                    drawn[k] = true;
                    if (*nsc < SCENES - 3)
                        sc[(*nsc)++] = (Scene){f, "gene", gene[k], 0};
                }
        }
        if (w->nframes)
            continue;
        int here = 0, best = 1 << 30;
        for (int i = 0; i < w->norgs; i++) {
            const Organism *q = w->orgs[i];
            if (q->removed || !q->alive)
                continue;
            int d = tel_code_distance(q->gc, gc);
            here += d == 0;
            best = d < best ? d : best;
        }
        if (here && alive_at < 0) {
            alive_at = f;
            copies = here;
            if (drawn_at > 0 && f - drawn_at <= 256 && *nsc < SCENES)
                sc[(*nsc)++] = (Scene){drawn_at, "born", here, 0};
            if (*nsc < SCENES)
                sc[(*nsc)++] = (Scene){f, "alive", here, 0};
        }
        if (here > peak) {
            peak = here;
            peak_at = f;
        }
        if (!here && alive_at < 0 && best < near) {
            near = best;
            near_at = f;
        }
        if (w->population == 0)
            break;
        if (twins && alive_at < 0 && drawn_at >= 0 && f - drawn_at > 256 * 8 && near > half)
            break;
    }
    if (peak_at >= 0 && peak > copies && *nsc < SCENES)
        sc[(*nsc)++] = (Scene){peak_at, "most", peak, 0};
    if (alive_at < 0 && near_at >= 0 && *nsc < SCENES)
        sc[(*nsc)++] = (Scene){near_at, "nearest", near, near};
    *to = f > frames_max ? frames_max : f;
    return alive_at >= 0;
}

static void say_scenes(const char *head, const Scene *sc, int n)
{
    for (int i = 0; i < n; i++)
        printf("%s %" PRId64 " %s %d %d\n", head, sc[i].frame, sc[i].kind, sc[i].n, sc[i].d);
}

#define BASIN 4096 /* seeds of an ancestor's basin kept, far more than any has */

/* A seed of the ancestor's basin run and watched. Each of the being's
 * later draws is checked as the stream passes it: the gene drawn there
 * must be in a child just made, or this world isn't the being's and is
 * given up at once. Then, once a time unit, the being (alive, the most of
 * it) and the bloodlines: the ancestor's left alone, and the being alive
 * while it is (it reigns: the winning state), or the bloodline gone.
 * Whether the being was seen alive; why it ended, in words. */
static bool watch_basin(World *w, uint64_t s0, int org, const CrackDraw *dr, int nd, const GeneticCode *gc,
                        int64_t frames_max, Scene *sc, int *nsc, int64_t *to, char *why, size_t nwhy)
{
    int64_t place[256];
    uint64_t bits[256];
    int pg[256], np = 0;
    bool angle[256];
    for (int d = 0; d < nd && np < 256; d++)
        if (dr[d].place >= 0) {
            place[np] = (int64_t)tel_draws(s0, dr[d].state);
            bits[np] = dr[d].bits;
            angle[np] = dr[d].angle;
            pg[np++] = dr[d].gene + 1;
        }
    for (int i = 0; i < w->norgs; i++)
        w->orgs[i]->gc->lineage = (uint32_t)i + 1;
    uint32_t line = (uint32_t)org + 1;
    *nsc = 0;
    sc[(*nsc)++] = (Scene){0, "ancestor", org + 1, 0};
    int64_t alive_at = -1, lone_at = -1, peak_at = -1, f;
    int peak = 0, copies = 0, next = 0;
    snprintf(why, nwhy, "run to frame %" PRId64 " without it", frames_max);
    for (f = 0; f <= frames_max; f++) {
        if (f)
            world_time(w);
        if (next < np) {
            int64_t now = (int64_t)tel_draws(s0, g_rng.seed);
            for (; next < np && now > place[next]; next++) {
                bool born = false;
                for (int i = 0; i < w->norgs && !born; i++) {
                    const GeneticCode *q = w->orgs[i]->gc;
                    if (w->orgs[i]->removed || !q)
                        continue;
                    for (int j = 0; j < q->ngenes && !born; j++)
                        born = dbits(angle[next] ? q->genes[j].theta : q->genes[j].length) == bits[next];
                }
                if (!born) {
                    snprintf(why, nwhy, "not its world: its gene %d's draw, at tick %" PRId64 ", went to no child",
                             pg[next], f);
                    *to = f;
                    return false;
                }
                if (*nsc < SCENES - 4)
                    sc[(*nsc)++] = (Scene){f, "gene", pg[next], 0};
            }
        }
        if (f && w->nframes)
            continue;
        int here = 0, lines = 0;
        bool mine = false;
        uint32_t first = 0;
        for (int i = 0; i < w->norgs; i++) {
            const Organism *q = w->orgs[i];
            if (q->removed || !q->alive)
                continue;
            here += tel_code_distance(q->gc, gc) == 0;
            mine |= q->gc->lineage == line;
            if (!first)
                first = q->gc->lineage, lines = 1;
            else if (lines == 1 && q->gc->lineage != first)
                lines = 2;
        }
        if (here && alive_at < 0) {
            alive_at = f;
            copies = here;
            if (*nsc < SCENES)
                sc[(*nsc)++] = (Scene){f, "alive", here, 0};
        }
        if (here > peak) {
            peak = here;
            peak_at = f;
        }
        if (!mine) {
            if (*nsc < SCENES)
                sc[(*nsc)++] = (Scene){f, "gone", 0, 0};
            snprintf(why, nwhy, "its ancestor's bloodline died out at time %" PRId64 "%s", f / 256,
                     alive_at >= 0 ? ", after it had lived" : "");
            break;
        }
        if (lines == 1 && lone_at < 0) {
            lone_at = f;
            if (*nsc < SCENES)
                sc[(*nsc)++] = (Scene){f, "lone", w->population, here};
        }
        if (lone_at >= 0 && here) {
            if (*nsc < SCENES)
                sc[(*nsc)++] = (Scene){f, "reigns", here, w->population};
            snprintf(why, nwhy, "it reigns at time %" PRId64 ": its ancestor's bloodline alone, %d of it alive",
                     f / 256, here);
            break;
        }
        /* alone, with all its draws made and no sign of it for a while */
        if (lone_at >= 0 && next >= np && f - lone_at > 256 * 64) {
            snprintf(why, nwhy, "its ancestor's bloodline alone from time %" PRId64 ", but not as it", lone_at / 256);
            break;
        }
    }
    if (peak_at >= 0 && peak > copies && *nsc < SCENES)
        sc[(*nsc)++] = (Scene){peak_at, "most", peak, 0};
    *to = f > frames_max ? frames_max : f;
    return alive_at >= 0;
}

/* A round of the arena's record as it began (syn/round.h), from the
 * parameters as saved: false if they aren't the round's any more, or a
 * fighter's code is missing. */
static bool round_world(World *w, const RecordRound *r, char *why, size_t n)
{
    params_set_defaults(&P);
    params_load();
    if (round_fingerprint(r->map) != r->settings) {
        snprintf(why, n, "made with other parameters than these");
        return false;
    }
    round_apply_map(&P, r->map);
    GeneticCode *code[REC_FIGHTERS] = {0};
    bool ok = true;
    for (int k = 0; k < r->nf; k++) {
        char err[160];
        code[k] = r->f[k].bgg ? bgg_parse((const char *)r->f[k].bgg, r->f[k].bgg_n, err, sizeof err) : NULL;
        ok &= code[k] != NULL;
    }
    if (ok) {
        int first[REC_FIGHTERS], placed[REC_FIGHTERS];
        world_init(w);
        round_begin(w, r->seed, code, r->nf, r->wild, NULL, first, placed);
    } else {
        snprintf(why, n, "a fighter's code isn't in the record");
    }
    for (int k = 0; k < r->nf; k++)
        gc_unref(code[k]);
    return ok;
}

static const char *record_path(char *out, size_t n)
{
    snprintf(out, n, "%s/arena-0.9.record", params_config_dir());
    return out;
}

/* syn-biogenesis --telescope find CODE.bgg: where in the shape a being
 * lives. Each of its genes' lengths is cracked to the state that drew it;
 * stepping back from that state up to `back` draws, every seed whose
 * founders have the gene is one it came from, and its other genes are
 * placed in that seed's stream by counting the draws to them. The seeds
 * with the most of its genes are run, watched for it. */
static int cmd_find(int argc, char **argv)
{
    const char *path = NULL, *record = NULL;
    bool defaults = false, progress = false, rounds = true;
    int64_t back = 0, units = 2000;
    int top = 12, rtop = 16;
    FindSet sets[4];
    int nsets = 0;
    for (int i = 2; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "--defaults"))
            defaults = true;
        else if (!strcmp(a, "--progress"))
            progress = true;
        else if (!strcmp(a, "--no-record"))
            rounds = false;
        else if (!strcmp(a, "--record") && i + 1 < argc)
            record = argv[++i];
        else if (!strcmp(a, "--back") && i + 1 < argc && parse_i64(argv[i + 1], &back))
            i++;
        else if (!strcmp(a, "--units") && i + 1 < argc && parse_i64(argv[i + 1], &units))
            i++;
        else if (!strcmp(a, "--seeds") && i + 1 < argc)
            top = atoi(argv[++i]);
        else if (!strcmp(a, "--rounds") && i + 1 < argc)
            rtop = atoi(argv[++i]);
        else if (!strcmp(a, "--saved") && nsets < 4)
            sets[nsets++] = (FindSet){0, 0, -1};
        else if (!strcmp(a, "--world") && i + 3 < argc && nsets < 4) {
            sets[nsets++] = (FindSet){atoi(argv[i + 1]), atoi(argv[i + 2]), atoi(argv[i + 3])};
            i += 3;
        } else if (!path && a[0] != '-')
            path = a;
        else
            return 2;
    }
    if (!path || back < 0 || units < 1)
        return 2;
    if (!nsets)
        sets[nsets++] = (FindSet){0, 0, -1};
    if (progress) {
        arena_os_child_begin();
        setvbuf(stdout, NULL, _IOLBF, 0);
    }
    char err[256];
    GeneticCode *gc = bgg_read(path, err, sizeof err);
    if (!gc) {
        fprintf(stderr, "%s: %s\n", path, err);
        printf("@none\n");
        return 1;
    }
    /* its genes' lengths, and the states that drew them */
    uint64_t have[256], gst[256 * 2];
    int gno[256 * 2]; /* the gene (from 1) each state drew */
    int nhave = 0, ngst = 0;
    for (int j = 0; j < gc->ngenes && nhave < 256; j++) {
        uint64_t b = dbits(gc->genes[j].length);
        bool seen = false;
        for (int k = 0; k < nhave && !seen; k++)
            seen = have[k] == b;
        if (!seen)
            have[nhave++] = b;
    }
    qsort(have, (size_t)nhave, sizeof *have, cmp_u64);
    int cracked = 0;
    for (int j = 0; j < nhave; j++) {
        double length;
        memcpy(&length, &have[j], sizeof length);
        uint64_t st[2];
        int k = tel_crack_length(length, st, 2);
        cracked += k > 0;
        int no = 0;
        for (int g = 0; g < gc->ngenes && !no; g++)
            if (dbits(gc->genes[g].length) == have[j])
                no = g + 1;
        for (int m = 0; m < k && m < 2; m++) {
            gno[ngst] = no;
            gst[ngst++] = st[m];
        }
    }
    printf("%s: %d genes, %d lengths, %d of them drawn by the generator\n", path, gc->ngenes, nhave, cracked);
    /* The arena's rounds, each an arc of the ring of states from its seed:
     * a gene whose place from a round's seed is within the draws a round
     * makes (a few million; 2^ROUND_ARC is more than any) was drawn in
     * that round, and by chance in a given other one about once in 2^18. */
    if (rounds) {
        char rp[1200];
        syn_record_set_file(record ? record : record_path(rp, sizeof rp));
        int nr = syn_record_count();
        typedef struct {
            int i, hits;
            int64_t newest;
            bool final, genesis;
        } RHit;
        RHit *rh = malloc((size_t)(nr ? nr : 1) * sizeof *rh);
        if (!rh)
            abort();
        int nrh = 0;
        for (int i = 0; i < nr; i++) {
            const RecordRound *r = syn_record_at(i);
            uint64_t s0 = tel_seed_state(r->seed);
            RHit h = {i, 0, -1, r->final && tel_code_distance(r->final, gc) == 0,
                      r->genesis && tel_code_distance(r->genesis, gc) == 0};
            for (int g = 0; g < ngst; g++) {
                int64_t at = (int64_t)tel_draws(s0, gst[g]);
                if (at < (1LL << ROUND_ARC)) {
                    h.hits++;
                    h.newest = at > h.newest ? at : h.newest;
                }
            }
            if (h.hits || h.final || h.genesis)
                rh[nrh++] = h;
        }
        /* the being itself first, then the most of its genes */
        for (int a = 1; a < nrh; a++)
            for (int b = a; b > 0; b--) {
                RHit *x = &rh[b - 1], *y = &rh[b];
                int kx = (x->final || x->genesis) * 1000 + x->hits, ky = (y->final || y->genesis) * 1000 + y->hits;
                if (ky <= kx)
                    break;
                RHit t = *x;
                *x = *y;
                *y = t;
            }
        printf("@rounds %d %d\n", nr, nrh);
        printf("%d of the arena's %d recorded round%s drew its genes or ended with it\n", nrh, nr, nr == 1 ? "" : "s");
        for (int c = 0; c < nrh && c < rtop; c++) {
            const RecordRound *r = syn_record_at(rh[c].i);
            char head[80];
            snprintf(head, sizeof head, "@rscene %d", rh[c].i);
            printf("@round %d %d %" PRId64 "\n", rh[c].i, r->cycle, r->seed);
            printf("  round %d (seed %" PRId64 "): %d of its genes drawn in it%s%s\n", r->cycle, r->seed,
                   rh[c].hits, rh[c].final ? ", it is the being the round ended with" : "",
                   rh[c].genesis ? ", it is the bloodline's genesis being" : "");
            if (rh[c].genesis)
                printf("%s 0 genesis 1 0\n", head);
            if (rh[c].final)
                printf("%s %" PRId64 " final %d 0\n", head, (int64_t)r->t_end * 256, r->pop);
            World w;
            char why[160];
            if (!round_world(&w, r, why, sizeof why)) {
                printf("  round %d can't be put together again: %s\n", r->cycle, why);
                continue;
            }
            int64_t place[512];
            int pg[512], np = 0;
            uint64_t s0 = tel_seed_state(r->seed);
            for (int g = 0; g < ngst; g++) {
                int64_t at = (int64_t)tel_draws(s0, gst[g]);
                if (at < (1LL << ROUND_ARC)) {
                    place[np] = at;
                    pg[np++] = gno[g];
                }
            }
            Scene sc[SCENES];
            int nsc;
            int64_t to;
            bool lives = watch(&w, s0, place, pg, np, gc, (int64_t)r->t_end * 256, false, sc, &nsc, &to);
            world_free(&w);
            say_scenes(head, sc, nsc);
            printf("  round %d: %s\n", r->cycle, lives ? "it lives here" : "a relative of it");
        }
        free(rh);
    }
    /* Its genesis ancestor, drawn again from one of its genes, and every
     * seed whose genesis makes it (syn/crack.h): each run with the being's
     * later draws checked as its stream passes them, and watched for the
     * being and for the ancestor's bloodline left alone. */
    /* founders drawn again take their colours from the parameters */
    set_params(&sets[0], defaults);
    CrackAncestor anc[8];
    int na = crack_ancestors(gc, anc, 8);
    CrackDraw dr[256];
    int nd = 0;
    bool later = false;
    if (na) {
        const CrackAncestor *a = &anc[0];
        nd = crack_draws(gc, a, dr, 256);
        printf("@ancestor 0x%012" PRIx64 " %d %d %d\n", a->start, a->proof, a->kept, a->changes);
        printf("its genesis ancestor: %d gene%s, %d arm%s, begun at state 0x%012" PRIx64
               "; %d of its genes kept since, %d mutation%s at the fewest\n",
               a->gc->ngenes, a->gc->ngenes == 1 ? "" : "s", a->gc->symmetry, a->gc->symmetry == 1 ? "" : "s",
               a->start, a->kept, a->changes, a->changes == 1 ? "" : "s");
        for (int d = 0; d < nd; d++) {
            printf("@draw %d %" PRId64 " %c\n", dr[d].gene + 1, dr[d].place,
                   dr[d].angle ? 'a' : dr[d].whole ? 'w' : 'l');
            later |= dr[d].place >= 0;
        }
    } else {
        printf("@ancestor none\n");
        printf("no genesis ancestor: none of its genes is a founder's it still has with another to prove it"
               " (its seed has faded)\n");
    }
    for (int si = 0; si < nsets && na; si++) {
        set_params(&sets[si], defaults);
        printf("@set %d %d %d %d\n", si, P.world_width, P.world_height, P.initial_organisms);
        CrackSeed *bs = malloc(BASIN * sizeof *bs);
        if (!bs)
            abort();
        int nb = crack_basin(anc[0].start, back, bs, BASIN);
        nb = nb < BASIN ? nb : BASIN;
        printf("@basin %d %d\n", si, nb);
        printf("%d seed%s make its ancestor a founder, with %d x %d and %d first organisms\n", nb,
               nb == 1 ? "" : "s", P.world_width, P.world_height, P.initial_organisms);
        for (int c = 0; c < nb; c++)
            printf("@seed %d %" PRId64 " %d %.4f\n", si, bs[c].seed, bs[c].slot, bs[c].chance);
        /* With later draws it lives in one of them at most, found by its
         * draws; without, in each, and the question is where it wins. */
        int runs = later ? nb : nb < top ? nb : top;
        bool lives = false;
        for (int c = 0; c < runs && !(later && lives); c++) {
            printf("@progress %d %d %d\n", si, c + 1, runs);
            set_params(&sets[si], defaults);
            World w;
            world_init(&w);
            g_rng.seed = bs[c].state;
            world_genesis(&w);
            Scene sc[SCENES];
            int nsc;
            int64_t to;
            char why[200];
            bool here = watch_basin(&w, bs[c].state, bs[c].org, dr, nd, gc, units * 256, sc, &nsc, &to, why,
                                    sizeof why);
            world_free(&w);
            if (here || !later) {
                char head[80];
                snprintf(head, sizeof head, "@scene %d %" PRId64, si, bs[c].seed);
                say_scenes(head, sc, nsc);
            }
            printf("  seed %" PRId64 " (founder %d, %.0f%% to be the bloodline left): %s\n", bs[c].seed,
                   bs[c].slot + 1, 100 * bs[c].chance, why);
            lives |= here;
        }
        free(bs);
    }
    printf("@done\n");
    crack_ancestors_free(anc, na);
    gc_unref(gc);
    return 0;
}

/* syn-biogenesis --telescope scene SEED FRAMES OUT.bgw: the world SEED
 * makes, run FRAMES frames and saved, with the generator's state there
 * (@at STATE) to open it on its own thread. */
static int cmd_scene(int argc, char **argv)
{
    int64_t seed, frames;
    bool round = argc > 2 && !strcmp(argv[2], "--round");
    int a0 = round ? 3 : 2;
    if (argc < a0 + 3 || !parse_i64(argv[a0], &seed) || !parse_i64(argv[a0 + 1], &frames) || frames < 0)
        return 2;
    const char *out = argv[a0 + 2], *record = NULL;
    bool defaults = false, progress = false;
    FindSet set = {0, 0, -1};
    for (int i = a0 + 3; i < argc; i++) {
        if (!strcmp(argv[i], "--defaults"))
            defaults = true;
        else if (!strcmp(argv[i], "--record") && i + 1 < argc)
            record = argv[++i];
        else if (!strcmp(argv[i], "--progress"))
            progress = true;
        else if (!strcmp(argv[i], "--world") && i + 3 < argc) {
            set = (FindSet){atoi(argv[i + 1]), atoi(argv[i + 2]), atoi(argv[i + 3])};
            i += 3;
        } else
            return 2;
    }
    if (progress) {
        arena_os_child_begin();
        setvbuf(stdout, NULL, _IOLBF, 0);
    }
    World w;
    if (round) {
        /* the record's round number `seed`, as it began */
        char rp[1200], why[160];
        syn_record_set_file(record ? record : record_path(rp, sizeof rp));
        const RecordRound *r = seed >= 0 && seed < syn_record_count() ? syn_record_at((int)seed) : NULL;
        if (!r || !round_world(&w, r, why, sizeof why)) {
            fprintf(stderr, "round %" PRId64 ": %s\n", seed, r ? why : "not in the record");
            printf("@none\n");
            return 1;
        }
    } else {
        set_params(&set, defaults);
        world_init(&w);
        jr_set_seed(&g_rng, seed);
        world_genesis(&w);
    }
    for (int64_t f = 0; f < frames; f++) {
        world_time(&w);
        if (progress && f % (256 * 100) == 0)
            printf("@frame %" PRId64 "\n", f);
    }
    char err[256];
    if (!bgw_save(&w, out, err, sizeof err)) {
        fprintf(stderr, "%s: %s\n", out, err);
        printf("@none\n");
        world_free(&w);
        return 1;
    }
    printf("@at 0x%012" PRIx64 "\n", g_rng.seed);
    world_free(&w);
    return 0;
}

/* --- the command --- */

static int usage(int r)
{
    printf("usage: syn-biogenesis --telescope COMMAND ...\n"
           "  Biogenesis 0.9's random generator (java.util.Random, 48 bits) as algebra.\n"
           "  STATE is the generator's 48-bit state (0x... or decimal).\n"
           "  jump STATE N         the state N draws later (N < 0: earlier)\n"
           "  seed SEED            the state a seed starts the generator at, and the seed in\n"
           "                       [0, 2^48) that makes the same world\n"
           "  gene LENGTH [ANGLE]  every state that draws a gene of exactly this length (and\n"
           "                       then this angle); give the numbers with all their digits\n"
           "  world FILE           the seed a saved world (.bgw or .sbw) was made from, found\n"
           "                       from its founders' genes, then proved by replaying it from\n"
           "                       tick 0 to the saved tick\n"
           "    --defaults         replay with the default parameters, not the saved ones\n"
           "                       (the world's size is always the save's)\n"
           "    --back N           draws to step back from each gene (default 4096)\n"
           "    --lengths K        try only the K commonest gene lengths\n"
           "    --no-replay        list the seeds found, replay none\n"
           "    --strong           replay only seeds whose founders share two genes or more\n"
           "                       with the world\n"
           "    --seed S           no search: how many of seed S's founders' genes are in\n"
           "                       the world, and whether replaying S gives it\n"
           "    --progress         for the window: a line \"@found SEED STATE\" or \"@none\" at\n"
           "                       the end, and every line as it comes\n"
           "  fade SEED FRAMES EVERY   runs the world SEED makes and, every EVERY frames,\n"
           "                       reads every living gene's place in the stream of draws:\n"
           "                       founder genes left, the oldest mark, the bloodlines left\n"
           "                       and what the living are made of (tools/fade)\n"
           "    --defaults         the default parameters, not the saved ones\n"
           "    --world W H N      a world W x H with N first organisms\n"
           "    --marks            also each living gene's time unit of birth\n"
           "  find CODE.bgg        where a being lives: its genesis ancestor (as crack finds\n"
           "                       it) and every seed that makes it, the likeliest first, each\n"
           "                       run with the being's later draws checked as its stream\n"
           "                       passes them and watched for the being and its ancestor's\n"
           "                       bloodline; and the arena's recorded rounds that drew its\n"
           "                       genes. Lines \"@scene SET SEED FRAME KIND N D\", KIND one of\n"
           "                       ancestor, gene, alive, most, lone, reigns, gone\n"
           "    --saved            search the parameters as saved (the default)\n"
           "    --world W H N      search a world W x H with N first organisms (up to 4 sets)\n"
           "    --defaults         the default parameters, not the saved ones\n"
           "    --units U          run each seed at most U time units (default 2000)\n"
           "    --seeds K          for a being with no later draw (found in each seed, not\n"
           "                       one), run the K likeliest seeds (default 12)\n"
           "    --back N           draws before the ancestor to look for its seeds (default:\n"
           "                       as far as the founders reach)\n"
           "    --rounds K         run at most the K rounds of the record with most of its\n"
           "                       genes (default 16)\n"
           "    --record FILE      the arena's record to search (default: the configuration\n"
           "                       folder's); --no-record: none\n"
           "  crack CODE.bgg       the algebra alone, no world run: the being's genesis\n"
           "                       ancestor drawn again from one of its genes, its later draws\n"
           "                       in order, and every seed whose genesis makes the ancestor,\n"
           "                       with the chance its bloodline is the one left\n"
           "    --defaults, --world W H N, --back N (draws to look back), --seeds K (shown)\n"
           "  forge                a winner from the dice: seeds weighed by their founders\n"
           "                       alone, the likeliest run until one bloodline is left\n"
           "    --from SEED        the first seed weighed (default 1)\n"
           "    --scan N           how many seeds to weigh (default 20000)\n"
           "    --top K            how many of them to run (default 8)\n"
           "    --units U          the most time units each runs (default 1500)\n"
           "    --out DIR          each winner's being written there as seed-SEED.bgg\n"
           "    --defaults, --world W H N, --progress\n"
           "  basins N             how many seeds make a founder at a state, over N states\n"
           "                       (docs/crack); --defaults, --world W H N\n"
           "  scene SEED FRAMES OUT.bgw   the world SEED makes at that frame, saved, and the\n"
           "                       generator's state there (\"@at STATE\"); --defaults, --world\n"
           "  scene --round I FRAMES OUT.bgw   the same for round I of the record (from 0),\n"
           "                       as the round began: its seed and its fighters; --record FILE\n");
    return r;
}

int syn_telescope_main(int argc, char **argv)
{
    const char *cmd = argc > 1 ? argv[1] : "";
    uint64_t s;
    int64_t n;
    double d = 0, e = 0;
    if (!strcmp(cmd, "jump") && argc == 4 && parse_u64(argv[2], &s) && parse_i64(argv[3], &n)) {
        printf("0x%012" PRIx64 "\n", tel_jump(s & TEL_MASK, n));
        return 0;
    }
    if (!strcmp(cmd, "seed") && argc == 3 && parse_i64(argv[2], &n)) {
        uint64_t st = tel_seed_state(n);
        printf("seed %" PRId64 " starts the generator at 0x%012" PRIx64
               "; the same world as seed %" PRId64 "\n",
               n, st, tel_state_seed(st));
        return 0;
    }
    if (!strcmp(cmd, "gene") && (argc == 3 || argc == 4) && parse_double(argv[2], &d) &&
        (argc == 3 || parse_double(argv[3], &e))) {
        uint64_t st[64];
        int k = tel_crack_length(d, st, 64), shown = 0;
        for (int i = 0; i < k && i < 64; i++) {
            if (argc == 4) {
                uint64_t x = tel_jump(st[i], 2);
                if (theta_of(draw_double(&x)) != e)
                    continue;
            }
            printf("0x%012" PRIx64 "\n", st[i]);
            shown++;
        }
        if (!shown)
            printf("no state draws this %s\n", argc == 4 ? "gene" : "length");
        return shown ? 0 : 1;
    }
    if (!strcmp(cmd, "world")) {
        int r = cmd_world(argc, argv);
        return r == 2 ? usage(2) : r;
    }
    if (!strcmp(cmd, "find")) {
        int r = cmd_find(argc, argv);
        return r == 2 ? usage(2) : r;
    }
    if (!strcmp(cmd, "scene")) {
        int r = cmd_scene(argc, argv);
        return r == 2 ? usage(2) : r;
    }
    if (!strcmp(cmd, "fade")) {
        int r = cmd_fade(argc, argv);
        return r == 2 ? usage(2) : r;
    }
    if (!strcmp(cmd, "crack") || !strcmp(cmd, "forge") || !strcmp(cmd, "basins")) {
        int r = crack_main(argc, argv);
        return r == 2 ? usage(2) : r;
    }
    return usage(strcmp(cmd, "--help") && strcmp(cmd, "-h") && strcmp(cmd, "") ? 2 : 0);
}

#endif
