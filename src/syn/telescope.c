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

#include "io/bgw.h"
#include "io/worldfile.h"
#include "sim/genetic_code.h"
#include "sim/organism.h"
#include "sim/params.h"
#include "sim/world.h"
#include "syn/arena_os.h"

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
           "                       the end, and every line as it comes\n");
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
    return usage(strcmp(cmd, "--help") && strcmp(cmd, "-h") && strcmp(cmd, "") ? 2 : 0);
}

#endif
