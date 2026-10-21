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

#include "syn/crack.h"

/* Color Mod draws its founders differently; the engine is 0.9's. */
#ifndef BIO_COLOUR

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io/bgg.h"
#include "sim/genetic_code.h"
#include "sim/jrandom.h"
#include "sim/organism.h"
#include "sim/params.h"
#include "sim/world.h"
#include "syn/arena_os.h"
#include "syn/genesis.h"
#include "syn/telescope.h"

/* draws from org_new's start to its founder's gene j's length: org_new's
 * angle (2), the mirroring, arms and genes' number (3), five per gene */
#define GENE_AT(j) (5 + 5 * (int64_t)(j))
/* the most draws one founder takes: 8 genes, dispersal, twelve tries at a
 * place; a colour's draw is retried about once in 2^25 */
#define FOUNDER_MAX (GENE_AT(CRACK_FOUNDER_GENES) + 1 + 24)
/* a later draw further than this from the ancestor isn't of its stream: a
 * world run for years makes fewer, a stranger lands nearer once in 4096 */
#define STREAM 36

static uint64_t dbits(double d)
{
    uint64_t u;
    memcpy(&u, &d, sizeof u);
    return u;
}

GeneticCode *crack_founder_at(uint64_t start)
{
    uint64_t keep = g_rng.seed;
    g_rng.seed = start & TEL_MASK;
    (void)jr_next_double(&g_rng); /* org_new's angle */
    GeneticCode *gc = gc_new_random();
    g_rng.seed = keep;
    return gc;
}

/* --- the ancestor --- */

static bool has_bits(const GeneticCode *gc, uint64_t b, bool theta)
{
    for (int j = 0; j < gc->ngenes; j++)
        if (dbits(theta ? gc->genes[j].theta : gc->genes[j].length) == b)
            return true;
    return false;
}

/* What of the being the founder proves: its lengths and angles found in
 * it (one of them the length cracked), and its genes it has whole. */
static void proof(const GeneticCode *being, const GeneticCode *f, int *draws, int *kept)
{
    *draws = *kept = 0;
    for (int j = 0; j < being->ngenes; j++) {
        const Gene *g = &being->genes[j];
        bool l = has_bits(f, dbits(g->length), false), t = has_bits(f, dbits(g->theta), true);
        *draws += l + t;
        for (int k = 0; k < f->ngenes && l && t; k++)
            if (dbits(f->genes[k].length) == dbits(g->length) && dbits(f->genes[k].theta) == dbits(g->theta) &&
                f->genes[k].color == g->color) {
                (*kept)++;
                break;
            }
    }
}

int crack_changes(const GeneticCode *from, const GeneticCode *to)
{
    /* an alignment of the genes in order: a gene kept costs what of it was
     * drawn again, one dropped or added costs one */
    int n = from->ngenes, m = to->ngenes;
    int *row = malloc((size_t)(m + 1) * 2 * sizeof *row);
    if (!row)
        abort();
    int *prev = row, *cur = row + m + 1;
    for (int b = 0; b <= m; b++)
        prev[b] = b;
    for (int a = 1; a <= n; a++) {
        cur[0] = a;
        const Gene *x = &from->genes[a - 1];
        for (int b = 1; b <= m; b++) {
            const Gene *y = &to->genes[b - 1];
            int keep = (dbits(x->length) != dbits(y->length)) + (dbits(x->theta) != dbits(y->theta)) +
                       (x->color != y->color);
            int best = prev[b - 1] + keep;
            if (prev[b] + 1 < best)
                best = prev[b] + 1;
            if (cur[b - 1] + 1 < best)
                best = cur[b - 1] + 1;
            cur[b] = best;
        }
        int *t = prev;
        prev = cur;
        cur = t;
    }
    int d = prev[m];
    free(row);
    return d + (from->symmetry != to->symmetry) + (from->mirror != to->mirror) +
           (from->disperse_children != to->disperse_children);
}

int crack_ancestors(const GeneticCode *being, CrackAncestor *out, int max)
{
    int n = 0;
    for (int j = 0; j < being->ngenes; j++) {
        uint64_t st[2];
        int k = tel_crack_length(being->genes[j].length, st, 2);
        for (int m = 0; m < k && m < 2; m++)
            for (int at = 0; at < CRACK_FOUNDER_GENES; at++) {
                uint64_t start = tel_jump(st[m], -GENE_AT(at));
                bool seen = false;
                for (int c = 0; c < n && !seen; c++)
                    seen = out[c].start == start;
                if (seen)
                    continue;
                GeneticCode *f = crack_founder_at(start);
                int draws, kept;
                proof(being, f, &draws, &kept);
                /* the cracked length is in it by construction: anything
                 * more is a 53-bit coincidence or the founder itself */
                if (at < f->ngenes && draws >= 2 && n < max) {
                    out[n++] = (CrackAncestor){start, f, at, draws - 1, kept, crack_changes(f, being)};
                    f = NULL;
                }
                gc_unref(f);
            }
    }
    /* the fewest mutations from it first, then the best proved */
    for (int a = 1; a < n; a++)
        for (int b = a; b > 0 && (out[b].changes < out[b - 1].changes ||
                                  (out[b].changes == out[b - 1].changes && out[b].proof > out[b - 1].proof));
             b--) {
            CrackAncestor t = out[b];
            out[b] = out[b - 1];
            out[b - 1] = t;
        }
    return n;
}

void crack_ancestors_free(CrackAncestor *a, int n)
{
    for (int i = 0; i < n; i++) {
        gc_unref(a[i].gc);
        a[i].gc = NULL;
    }
}

/* the angle gene_randomize_theta draws from this state */
static uint64_t angle_from(uint64_t state)
{
    uint64_t s1 = tel_jump(state, 1), s2 = tel_jump(s1, 1);
    uint64_t k = (s1 >> 22) << 27 | s2 >> 21;
    return dbits((double)k * 0x1.0p-53 * 2.0 * M_PI);
}

int crack_draws(const GeneticCode *being, const CrackAncestor *a, CrackDraw *out, int max)
{
    int n = 0;
    for (int j = 0; j < being->ngenes && n < max; j++) {
        const Gene *g = &being->genes[j];
        bool angle = a && has_bits(a->gc, dbits(g->length), false);
        if (angle && has_bits(a->gc, dbits(g->theta), true))
            continue;
        CrackDraw d = {j, angle, false, 0, dbits(angle ? g->theta : g->length), -1};
        uint64_t st[2];
        if ((angle ? tel_crack_theta(g->theta, st, 2) : tel_crack_length(g->length, st, 2)) > 0) {
            d.state = st[0];
            /* gene_randomize draws the angle straight after the length */
            d.whole = !angle && angle_from(tel_jump(st[0], 2)) == dbits(g->theta);
            if (a) {
                uint64_t at = tel_draws(a->start, d.state);
                d.place = at < (1ULL << STREAM) ? (int64_t)at : -1;
            }
        }
        out[n++] = d;
    }
    /* With no ancestor, the stream has no known start, but its draws still
     * have an order: each placed after the earliest of them, the one the
     * most of the others follow closely. */
    if (!a) {
        int ref = -1, best = 0;
        for (int i = 0; i < n; i++) {
            int near = 0;
            for (int k = 0; k < n && out[i].state; k++)
                near += out[k].state && tel_draws(out[i].state, out[k].state) < (1ULL << STREAM);
            if (near > best) {
                best = near;
                ref = i;
            }
        }
        for (int k = 0; k < n && ref >= 0; k++) {
            uint64_t at = out[k].state ? tel_draws(out[ref].state, out[k].state) : TEL_MASK;
            out[k].place = at < (1ULL << STREAM) ? (int64_t)at : -1;
        }
    }
    /* the soonest first; those of no stream last */
    for (int x = 1; x < n; x++)
        for (int y = x; y > 0; y--) {
            int64_t p = out[y - 1].place < 0 ? INT64_MAX : out[y - 1].place;
            int64_t q = out[y].place < 0 ? INT64_MAX : out[y].place;
            if (q >= p)
                break;
            CrackDraw t = out[y];
            out[y] = out[y - 1];
            out[y - 1] = t;
        }
    return n;
}

/* --- the fitness, and the chance of being the bloodline left --- */

double crack_fitness(const GeneticCode *gc, double co2)
{
    Organism o;
    memset(&o, 0, sizeof o);
    o.gc = (GeneticCode *)gc;
    o.growth_ratio = 1;
    org_create(&o);
    org_symmetric(&o);
    double green = 0, mass = o.mass;
    int children = 1;
    for (int i = 0; i < o.segments; i++) {
        green += o.seg_color[i] == COL_GREEN ? o.m[i] : 0;
        children += o.seg_color[i] == COL_YELLOW;
    }
    free(o.m1);
    /* world_photosynthesis and org_breath, grown */
    double c = co2 / P.drain_subs_divisor;
    double gain = green / P.green_obtained_energy_divisor * (c < 1 ? c : 1) - mass / P.segment_cost_divisor;
    if (gain <= 0)
        return 0;
    /* each child, and the parent after, keep reproduce_energy / (k + 1) */
    int k = children < 1 ? 1 : children > 8 ? 8 : children;
    double need = gc->reproduce_energy * k / (k + 1.0);
    /* fifteen steps of growth, one each eight frames, then twenty frames
     * at least between litters */
    double t = need / gain;
    t = (t < 20 ? 20 : t) + 15 * 8;
    if (t > (gc->max_age + 1) * 256.0)
        return 0;
    return log(k + 1.0) / t;
}

/* Fitted on replayed worlds (docs/crack): the chance a founder's bloodline
 * is the one left goes as its fitness to this power, against the others'.
 * A founder that can't feed itself as a plant still wins sometimes (it
 * hunts, or is the last standing): it counts as this much fitness. */
#define WIN_POWER 2.5
#define WIN_FLOOR 0.0004

double crack_win_chance(const double *fit, int n, int i)
{
    double sum = 0, mine = 0;
    for (int k = 0; k < n; k++) {
        double f = pow(fit[k] > WIN_FLOOR ? fit[k] : WIN_FLOOR, WIN_POWER);
        sum += f;
        if (k == i)
            mine = f;
    }
    return sum > 0 ? mine / sum : 0;
}

/* --- the seeds that make the ancestor --- */

/* The world's genesis from state s0 (P's), up to the founder begun at
 * `start`: its slot, or -1 once past it. */
static int slot_from(World *w, uint64_t s0, uint64_t start, uint64_t within)
{
    SynGenesis g;
    g_rng.seed = s0;
    syn_genesis_begin(&g, w);
    for (int i = 0; i < g.total; i++) {
        if (g_rng.seed == start)
            return i;
        if (tel_draws(s0, g_rng.seed) > within)
            return -1;
        syn_genesis_next(&g);
    }
    return -1;
}

static int cmp_seed(const void *a, const void *b)
{
    const CrackSeed *x = a, *y = b;
    if (x->chance != y->chance)
        return x->chance > y->chance ? -1 : 1;
    return x->seed < y->seed ? -1 : x->seed > y->seed;
}

int crack_basin(uint64_t start, int64_t back, CrackSeed *out, int max)
{
    uint64_t keep = g_rng.seed;
    if (back <= 0)
        back = (int64_t)P.initial_organisms * FOUNDER_MAX;
    World w;
    world_init(&w);
    int n = 0;
    uint64_t s0 = start & TEL_MASK;
    GeneticCode *f = crack_founder_at(start);
    for (int64_t k = 0; k <= back; k++, s0 = tel_jump(s0, -1)) {
        int slot = slot_from(&w, s0, start, (uint64_t)k);
        if (slot < 0)
            continue;
        /* the whole genesis: which organism it is, the one drawn from start
         * (the slot counts founders that found no place too; if it found
         * none itself, nothing in this world comes of it), and the
         * founders it is up against */
        g_rng.seed = s0;
        world_genesis(&w);
        int nf = w.norgs < 1024 ? w.norgs : 1024, me = -1;
        for (int i = 0; i < nf && me < 0; i++) {
            const GeneticCode *q = w.orgs[i]->gc;
            if (q->ngenes == f->ngenes && q->symmetry == f->symmetry && q->mirror == f->mirror &&
                dbits(q->genes[0].length) == dbits(f->genes[0].length) &&
                dbits(q->genes[0].theta) == dbits(f->genes[0].theta))
                me = i;
        }
        if (me < 0)
            continue;
        if (n < max) {
            double fit[1024];
            for (int i = 0; i < nf; i++)
                fit[i] = crack_fitness(w.orgs[i]->gc, w.co2);
            CrackSeed s = {s0, tel_state_seed(s0), slot, me, nf, fit[me], 0, crack_win_chance(fit, nf, me)};
            for (int i = 0; i < nf; i++)
                if (i != me && fit[i] > s.rival)
                    s.rival = fit[i];
            out[n] = s;
        }
        n++;
    }
    gc_unref(f);
    world_free(&w);
    g_rng.seed = keep;
    if (out && max > 0)
        qsort(out, (size_t)(n < max ? n : max), sizeof *out, cmp_seed);
    return n;
}

/* --- the commands --- */

static bool parse_i64(const char *s, int64_t *v)
{
    char *end;
    *v = strtoll(s, &end, 0);
    return *s && !*end;
}

/* the parameters as saved (or the defaults), in a world of its own if given */
static void params_for(bool defaults, int w, int h, int n)
{
    params_set_defaults(&P);
    if (!defaults)
        params_load();
    if (n >= 0) {
        P.world_width = w;
        P.world_height = h;
        P.initial_organisms = n;
    }
}

static void say_code(const char *head, const GeneticCode *gc)
{
    printf("%s%d gene%s, %d arm%s%s:", head, gc->ngenes, gc->ngenes == 1 ? "" : "s", gc->symmetry,
           gc->symmetry == 1 ? "" : "s", gc->mirror ? ", mirrored" : "");
    for (int j = 0; j < gc->ngenes; j++)
        printf(" %s %.2f", bio_color_name(gc->genes[j].color), gc->genes[j].length);
    printf("\n");
}

/* syn-biogenesis --telescope crack BEING.bgg: the algebra alone, no world
 * run: its ancestor, its later draws, and the seeds that make it. */
static int cmd_crack(int argc, char **argv)
{
    const char *path = NULL;
    bool defaults = false;
    int w = 0, h = 0, n = -1;
    int64_t back = 0;
    int top = 20;
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--defaults"))
            defaults = true;
        else if (!strcmp(argv[i], "--world") && i + 3 < argc) {
            w = atoi(argv[i + 1]);
            h = atoi(argv[i + 2]);
            n = atoi(argv[i + 3]);
            i += 3;
        } else if (!strcmp(argv[i], "--back") && i + 1 < argc && parse_i64(argv[i + 1], &back))
            i++;
        else if (!strcmp(argv[i], "--seeds") && i + 1 < argc)
            top = atoi(argv[++i]);
        else if (!path && argv[i][0] != '-')
            path = argv[i];
        else
            return 2;
    }
    if (!path)
        return 2;
    params_for(defaults, w, h, n);
    char err[256];
    GeneticCode *gc = bgg_read(path, err, sizeof err);
    if (!gc) {
        fprintf(stderr, "%s: %s\n", path, err);
        return 1;
    }
    say_code("the being: ", gc);
    CrackAncestor anc[8];
    int na = crack_ancestors(gc, anc, 8);
    if (!na)
        printf("no genesis ancestor: none of its genes is a founder's gene it still has with another to prove "
               "it (its seed has faded, or it was made by hand)\n");
    for (int a = 0; a < na; a++) {
        printf("@ancestor %d 0x%012" PRIx64 " %d %d %d %d\n", a, anc[a].start, anc[a].gene, anc[a].proof,
               anc[a].kept, anc[a].changes);
        if (a == 0)
            printf("its genesis ancestor, begun at state 0x%012" PRIx64 " (%d more of its draws prove it):\n",
                   anc[a].start, anc[a].proof);
        else
            printf("or, read a gene or more along, begun at state 0x%012" PRIx64 ":\n", anc[a].start);
        say_code("  ", anc[a].gc);
        printf("  %d of its %d genes unchanged since, %d mutation%s at the fewest\n", anc[a].kept, gc->ngenes,
               anc[a].changes, anc[a].changes == 1 ? "" : "s");
    }
    CrackDraw dr[256];
    int nd = crack_draws(gc, na ? &anc[0] : NULL, dr, 256);
    if (!na && nd && dr[0].place == 0)
        printf("its draws in the order they were made, counted from the earliest (gene %d), in a stream whose"
               " start is unknown:\n", dr[0].gene + 1);
    for (int d = 0; d < nd; d++) {
        const char *what = dr[d].angle ? "its angle drawn again" : dr[d].whole ? "added whole" : "its length drawn again";
        printf("@draw %d %" PRId64 " %c\n", dr[d].gene + 1, dr[d].place, dr[d].angle ? 'a' : dr[d].whole ? 'w' : 'l');
        if (!dr[d].state)
            printf("  gene %d: %s the generator never draws (edited by hand)\n", dr[d].gene + 1,
                   dr[d].angle ? "an angle" : "a length");
        else if (!na && dr[d].place == 0)
            printf("  gene %d: %s, the earliest\n", dr[d].gene + 1, what);
        else if (dr[d].place >= 0)
            printf("  gene %d: %s %" PRId64 " draws after %s\n", dr[d].gene + 1, what, dr[d].place,
                   na ? "its ancestor was begun" : "the earliest");
        else
            printf("  gene %d: %s, in another stream%s\n", dr[d].gene + 1, what,
                   na ? " than its ancestor's (an infection, or a being put together)" : "");
    }
    for (int a = 0; a < na && a < 1; a++) {
        CrackSeed *s = malloc(4096 * sizeof *s);
        if (!s)
            abort();
        double t0 = arena_os_now();
        int ns = crack_basin(anc[a].start, back, s, 4096);
        printf("@basin %d %d %d %d\n", P.world_width, P.world_height, P.initial_organisms, ns);
        printf("%d seed%s make it a founder, with %d x %d and %d first organisms (%.2f s):\n", ns,
               ns == 1 ? "" : "s", P.world_width, P.world_height, P.initial_organisms, arena_os_now() - t0);
        for (int i = 0; i < ns && i < 4096; i++) {
            printf("@seed %" PRId64 " %d %d %.6f %.6g %.6g\n", s[i].seed, s[i].slot, s[i].founders, s[i].chance,
                   s[i].fit, s[i].rival);
            if (i < top)
                printf("  seed %" PRId64 ": founder %d of %d, its bloodline left %.0f%% of the time%s\n", s[i].seed,
                       s[i].slot + 1, s[i].founders, 100 * s[i].chance,
                       s[i].fit > s[i].rival ? " (the fittest founder there)" : "");
        }
        free(s);
    }
    crack_ancestors_free(anc, na);
    gc_unref(gc);
    return 0;
}

/* A world run until one bloodline is left (gc->lineage, which the
 * simulation only copies, set per founder here): its founder's index, or
 * -1 if all died, or -2 if none was left alone within the frames; *t the
 * time unit it happened. */
static int lone_bloodline(World *w, int64_t frames, int *t)
{
    for (int i = 0; i < w->norgs; i++)
        w->orgs[i]->gc->lineage = (uint32_t)i + 1;
    for (int64_t f = 0; f < frames; f++) {
        world_time(w);
        if (w->nframes)
            continue;
        uint32_t line = 0;
        bool more = false;
        for (int i = 0; i < w->norgs && !more; i++) {
            const Organism *q = w->orgs[i];
            if (q->removed || !q->alive)
                continue;
            if (!line)
                line = q->gc->lineage;
            else
                more = q->gc->lineage != line;
        }
        *t = (int)w->stats.time;
        if (!line)
            return -1;
        if (!more)
            return (int)line - 1;
    }
    return -2;
}

/* The commonest code alive (the first such), with how many have it. */
static const GeneticCode *commonest(const World *w, int *count)
{
    const GeneticCode *best = NULL;
    *count = 0;
    for (int i = 0; i < w->norgs; i++) {
        const Organism *q = w->orgs[i];
        if (q->removed || !q->alive)
            continue;
        int c = 0;
        for (int k = 0; k < w->norgs; k++) {
            const Organism *r = w->orgs[k];
            c += !r->removed && r->alive && tel_code_distance(r->gc, q->gc) == 0;
        }
        if (c > *count) {
            *count = c;
            best = q->gc;
        }
    }
    return best;
}

typedef struct {
    int64_t seed;
    int win;
    double chance, fit, rival;
} Forged;

static int cmp_forged(const void *a, const void *b)
{
    const Forged *x = a, *y = b;
    if (x->chance != y->chance)
        return x->chance > y->chance ? -1 : 1;
    return x->seed < y->seed ? -1 : x->seed > y->seed;
}

/* syn-biogenesis --telescope forge: a winner from the dice. Each seed's
 * founders are made (genesis only, no world run) and weighed; the seeds
 * whose likeliest bloodline is likeliest are run until one bloodline is
 * left, and the being it is left as is the winner. */
static int cmd_forge(int argc, char **argv)
{
    bool defaults = false, progress = false;
    int w = 0, h = 0, n = -1, top = 8;
    int64_t from = 1, scan = 20000, units = 1500;
    const char *out = NULL;
    for (int i = 2; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "--defaults"))
            defaults = true;
        else if (!strcmp(a, "--progress"))
            progress = true;
        else if (!strcmp(a, "--world") && i + 3 < argc) {
            w = atoi(argv[i + 1]);
            h = atoi(argv[i + 2]);
            n = atoi(argv[i + 3]);
            i += 3;
        } else if (!strcmp(a, "--from") && i + 1 < argc && parse_i64(argv[i + 1], &from))
            i++;
        else if (!strcmp(a, "--scan") && i + 1 < argc && parse_i64(argv[i + 1], &scan))
            i++;
        else if (!strcmp(a, "--units") && i + 1 < argc && parse_i64(argv[i + 1], &units))
            i++;
        else if (!strcmp(a, "--top") && i + 1 < argc)
            top = atoi(argv[++i]);
        else if (!strcmp(a, "--out") && i + 1 < argc)
            out = argv[++i];
        else
            return 2;
    }
    if (scan < 1 || top < 1 || units < 1)
        return 2;
    if (progress) {
        arena_os_child_begin();
        setvbuf(stdout, NULL, _IOLBF, 0);
    }
    params_for(defaults, w, h, n);
    printf("@set 0 %d %d %d\n", P.world_width, P.world_height, P.initial_organisms);
    Forged *fg = malloc((size_t)scan * sizeof *fg);
    if (!fg)
        abort();
    World world;
    world_init(&world);
    double t0 = arena_os_now();
    for (int64_t s = 0; s < scan; s++) {
        jr_set_seed(&g_rng, from + s);
        world_genesis(&world);
        double fit[1024] = {0};
        int nf = world.norgs < 1024 ? world.norgs : 1024, best = 0;
        if (!nf) {
            fg[s] = (Forged){from + s, 0, 0, 0, 0};
            continue;
        }
        for (int i = 0; i < nf; i++) {
            fit[i] = crack_fitness(world.orgs[i]->gc, world.co2);
            if (fit[i] > fit[best])
                best = i;
        }
        Forged f = {from + s, best, crack_win_chance(fit, nf, best), fit[best], 0};
        for (int i = 0; i < nf; i++)
            if (i != best && fit[i] > f.rival)
                f.rival = fit[i];
        fg[s] = f;
        if (progress && (s + 1) % 1000 == 0)
            printf("@scan %" PRId64 " %" PRId64 "\n", s + 1, scan);
    }
    double t_scan = arena_os_now() - t0;
    qsort(fg, (size_t)scan, sizeof *fg, cmp_forged);
    printf("%" PRId64 " seeds weighed from their founders alone in %.1f s; the %d likeliest run:\n", scan, t_scan,
           top);
    int right = 0, ran = 0;
    for (int c = 0; c < top && c < scan; c++) {
        printf("@progress 0 %d %d\n", c + 1, top);
        jr_set_seed(&g_rng, fg[c].seed);
        world_genesis(&world);
        /* the founders as genesis made them, for the one whose bloodline
         * is left */
        int nf = world.norgs < 1024 ? world.norgs : 1024;
        GeneticCode *founder[1024];
        for (int i = 0; i < nf; i++)
            founder[i] = gc_ref(world.orgs[i]->gc);
        int t = 0;
        int lone = lone_bloodline(&world, units * 256, &t);
        ran++;
        right += lone == fg[c].win;
        int copies = 0;
        const GeneticCode *being = lone >= 0 ? commonest(&world, &copies) : NULL;
        int changes = being && lone < nf ? crack_changes(founder[lone], being) : -1;
        printf("@forge %" PRId64 " %d %d %d %.6f %d %d\n", fg[c].seed, fg[c].win, lone, t, fg[c].chance, copies,
               changes);
        if (lone == -1)
            printf("  seed %" PRId64 ": all died by time %d\n", fg[c].seed, t);
        else if (lone == -2)
            printf("  seed %" PRId64 ": still more than one bloodline at time %d\n", fg[c].seed, t);
        else
            printf("  seed %" PRId64 ": founder %d expected (%.0f%%), founder %d's bloodline left alone at time %d,"
                   " as %d of its commonest being (%d mutation%s from that founder)\n",
                   fg[c].seed, fg[c].win + 1, 100 * fg[c].chance, lone + 1, t, copies, changes,
                   changes == 1 ? "" : "s");
        if (out && being) {
            char p[1200];
            snprintf(p, sizeof p, "%s/seed-%" PRId64 ".bgg", out, fg[c].seed);
            FILE *fo = fopen(p, "wb");
            if (fo) {
                bgg_write_file(fo, being);
                fclose(fo);
                printf("@file %" PRId64 " %s\n", fg[c].seed, p);
            }
        }
        for (int i = 0; i < nf; i++)
            gc_unref(founder[i]);
    }
    printf("the expected founder's bloodline was the one left in %d of %d\n", right, ran);
    printf("@done\n");
    world_free(&world);
    free(fg);
    return 0;
}

/* syn-biogenesis --telescope basins N: how many seeds make a founder at a
 * state, over N states from a fixed generator (docs/crack) */
static int cmd_basins(int argc, char **argv)
{
    int64_t count = 0;
    bool defaults = false;
    int w = 0, h = 0, n = -1;
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--defaults"))
            defaults = true;
        else if (!strcmp(argv[i], "--world") && i + 3 < argc) {
            w = atoi(argv[i + 1]);
            h = atoi(argv[i + 2]);
            n = atoi(argv[i + 3]);
            i += 3;
        } else if (!count && parse_i64(argv[i], &count))
            ;
        else
            return 2;
    }
    if (count < 1)
        return 2;
    params_for(defaults, w, h, n);
    JRandom r;
    jr_set_seed(&r, 20261021);
    int64_t hist[65] = {0}, sum = 0, most = 0;
    double t0 = arena_os_now();
    for (int64_t i = 0; i < count; i++) {
        uint64_t start = (uint64_t)(jr_next_double(&r) * 0x1.0p48) & TEL_MASK;
        int k = crack_basin(start, 0, NULL, 0);
        hist[k < 64 ? k : 64]++;
        sum += k;
        most = k > most ? k : most;
        printf("@basin %" PRId64 " 0x%012" PRIx64 " %d\n", i, start, k);
    }
    printf("%" PRId64 " states, %d x %d with %d first organisms (%.1f s): seeds making a founder there, %.2f on"
           " average, %" PRId64 " at most\n",
           count, P.world_width, P.world_height, P.initial_organisms, arena_os_now() - t0, (double)sum / count, most);
    for (int k = 0; k < 64; k++)
        if (hist[k])
            printf("  %d: %" PRId64 "\n", k, hist[k]);
    if (hist[64])
        printf("  64 or more: %" PRId64 "\n", hist[64]);
    return 0;
}

int crack_main(int argc, char **argv)
{
    if (argc >= 2 && !strcmp(argv[1], "basins"))
        return cmd_basins(argc, argv);
    if (argc >= 2 && !strcmp(argv[1], "crack"))
        return cmd_crack(argc, argv);
    if (argc >= 2 && !strcmp(argv[1], "forge"))
        return cmd_forge(argc, argv);
    return 2;
}

#else

int crack_main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    return 2;
}

#endif
