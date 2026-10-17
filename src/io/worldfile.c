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

#include "io/worldfile.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char k_magic[8] = {'S', 'Y', 'N', 'B', 'I', 'O', 'W', '\0'};
#define WORLD_FILE_VERSION 1u

static void set_err(char *err, size_t n, const char *fmt, ...)
{
    if (!err || !n)
        return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(err, n, fmt, ap);
    va_end(ap);
}

/* ---- little-endian writer ---- */

typedef struct {
    FILE *f;
    bool ok;
} Wr;

static void w_bytes(Wr *w, const void *p, size_t n)
{
    if (w->ok && fwrite(p, 1, n, w->f) != n)
        w->ok = false;
}

static void w_u64(Wr *w, uint64_t v)
{
    uint8_t b[8];
    for (int i = 0; i < 8; i++)
        b[i] = (uint8_t)(v >> (8 * i));
    w_bytes(w, b, 8);
}

static void w_i32(Wr *w, int32_t v)
{
    uint8_t b[4];
    uint32_t u = (uint32_t)v;
    for (int i = 0; i < 4; i++)
        b[i] = (uint8_t)(u >> (8 * i));
    w_bytes(w, b, 4);
}

static void w_i64(Wr *w, int64_t v)
{
    w_u64(w, (uint64_t)v);
}

static void w_f64(Wr *w, double d)
{
    uint64_t u;
    memcpy(&u, &d, 8);
    w_u64(w, u);
}

static void w_u8(Wr *w, uint8_t v)
{
    w_bytes(w, &v, 1);
}

/* ---- reader ---- */

typedef struct {
    FILE *f;
    bool ok;
} Rd;

static void r_bytes(Rd *r, void *p, size_t n)
{
    if (!r->ok || fread(p, 1, n, r->f) != n) {
        r->ok = false;
        memset(p, 0, n);
    }
}

static uint64_t r_u64(Rd *r)
{
    uint8_t b[8];
    r_bytes(r, b, 8);
    uint64_t v = 0;
    for (int i = 0; i < 8; i++)
        v |= (uint64_t)b[i] << (8 * i);
    return v;
}

static int32_t r_i32(Rd *r)
{
    uint8_t b[4];
    r_bytes(r, b, 4);
    uint32_t v = 0;
    for (int i = 0; i < 4; i++)
        v |= (uint32_t)b[i] << (8 * i);
    return (int32_t)v;
}

static int64_t r_i64(Rd *r)
{
    return (int64_t)r_u64(r);
}

static double r_f64(Rd *r)
{
    uint64_t u = r_u64(r);
    double d;
    memcpy(&d, &u, 8);
    return d;
}

static uint8_t r_u8(Rd *r)
{
    uint8_t v;
    r_bytes(r, &v, 1);
    return v;
}

/* ---- genetic code table: pointer -> index ---- */

typedef struct {
    const GeneticCode **list;
    int n, cap;
    const GeneticCode **slots; /* open addressing, power of two */
    int *slot_idx;
    int nslots;
} CodeTable;

static unsigned hash_ptr(const void *p)
{
    uintptr_t v = (uintptr_t)p;
    v ^= v >> 17;
    v *= 0xed5ad4bbu;
    v ^= v >> 11;
    return (unsigned)v;
}

static void ct_grow(CodeTable *t)
{
    int ns = t->nslots ? t->nslots * 2 : 256;
    const GeneticCode **slots = calloc((size_t)ns, sizeof *slots);
    int *idx = calloc((size_t)ns, sizeof *idx);
    if (!slots || !idx)
        abort();
    for (int i = 0; i < t->n; i++) {
        unsigned h = hash_ptr(t->list[i]) & (unsigned)(ns - 1);
        while (slots[h])
            h = (h + 1) & (unsigned)(ns - 1);
        slots[h] = t->list[i];
        idx[h] = i;
    }
    free(t->slots);
    free(t->slot_idx);
    t->slots = slots;
    t->slot_idx = idx;
    t->nslots = ns;
}

static int ct_index(CodeTable *t, const GeneticCode *gc, bool add)
{
    if (!gc)
        return -1;
    if (t->nslots == 0 || (t->n + 1) * 2 > t->nslots)
        ct_grow(t);
    unsigned h = hash_ptr(gc) & (unsigned)(t->nslots - 1);
    while (t->slots[h]) {
        if (t->slots[h] == gc)
            return t->slot_idx[h];
        h = (h + 1) & (unsigned)(t->nslots - 1);
    }
    if (!add)
        return -1;
    if (t->n == t->cap) {
        t->cap = t->cap ? t->cap * 2 : 256;
        t->list = realloc(t->list, (size_t)t->cap * sizeof *t->list);
        if (!t->list)
            abort();
    }
    t->list[t->n] = gc;
    t->slots[h] = gc;
    t->slot_idx[h] = t->n;
    return t->n++;
}

static void ct_free(CodeTable *t)
{
    free(t->list);
    free(t->slots);
    free(t->slot_idx);
}

static GeneticCode *const *stats_codes(const WorldStats *s, int *n)
{
    static GeneticCode *refs[9];
    refs[0] = s->alive_most_children;
    refs[1] = s->alive_most_kills;
    refs[2] = s->alive_most_infections;
    refs[3] = s->most_children;
    refs[4] = s->most_kills;
    refs[5] = s->most_infections;
    refs[6] = s->last_born;
    refs[7] = s->last_dead;
    refs[8] = s->last_infected;
    *n = 9;
    return refs;
}

static void w_series(Wr *w, const StatsSeries *s)
{
    w_i32(w, s->n);
    for (int i = 0; i < s->n; i++)
        w_f64(w, s->v[i]);
}

static bool r_series(Rd *r, StatsSeries *s)
{
    int n = r_i32(r);
    if (n < 0 || n > STATS_HISTORY)
        return false;
    s->n = n;
    for (int i = 0; i < n; i++)
        s->v[i] = r_f64(r);
    return true;
}

bool worldfile_save(const World *world, const char *path, char *err, size_t errn)
{
    CodeTable ct = {0};
    int nstat;
    GeneticCode *const *sc = stats_codes(&world->stats, &nstat);
    int live = 0;
    for (int i = 0; i < world->norgs; i++) {
        const Organism *o = world->orgs[i];
        if (o->removed)
            continue;
        live++;
        ct_index(&ct, o->gc, true);
        ct_index(&ct, o->infected_gc, true);
    }
    for (int i = 0; i < nstat; i++)
        ct_index(&ct, sc[i], true);

    FILE *f = fopen(path, "wb");
    if (!f) {
        set_err(err, errn, "Can't write %s", path);
        ct_free(&ct);
        return false;
    }
    Wr w = {f, true};
    w_bytes(&w, k_magic, sizeof k_magic);
    w_i32(&w, (int32_t)WORLD_FILE_VERSION);

    w_i32(&w, world->width);
    w_i32(&w, world->height);
    w_i32(&w, world->population);
    w_i32(&w, world->next_id);
    w_i32(&w, world->nframes);
    w_f64(&w, world->o2);
    w_f64(&w, world->co2);

    w_i32(&w, ct.n);
    for (int i = 0; i < ct.n; i++) {
        const GeneticCode *gc = ct.list[i];
        w_i32(&w, gc->symmetry);
        w_i32(&w, gc->mirror);
        w_u8(&w, gc->disperse_children);
        w_i32(&w, gc->reproduce_energy);
        w_i32(&w, gc->max_age);
        w_i32(&w, gc->ngenes);
        for (int g = 0; g < gc->ngenes; g++) {
            w_f64(&w, gc->genes[g].length);
            w_f64(&w, gc->genes[g].theta);
            w_u8(&w, gc->genes[g].color);
        }
    }

    const WorldStats *s = &world->stats;
    w_i64(&w, s->time);
    w_i32(&w, s->max_population);
    w_i32(&w, s->max_births);
    w_i32(&w, s->max_deaths);
    w_i64(&w, s->max_population_time);
    w_i32(&w, s->min_population);
    w_i64(&w, s->min_population_time);
    w_i32(&w, s->mass_extinctions);
    w_i32(&w, s->mass_extinction_state);
    w_i32(&w, s->created_organisms);
    w_i64(&w, s->population_sum);
    w_i64(&w, s->death_sum);
    w_i32(&w, s->death_last_time);
    w_i64(&w, s->birth_sum);
    w_i32(&w, s->birth_last_time);
    w_i64(&w, s->infections_sum);
    w_f64(&w, s->max_oxygen);
    w_i64(&w, s->max_oxygen_time);
    w_f64(&w, s->min_oxygen);
    w_i64(&w, s->min_oxygen_time);
    w_f64(&w, s->max_co2);
    w_i64(&w, s->max_co2_time);
    w_f64(&w, s->min_co2);
    w_i64(&w, s->min_co2_time);
    w_i32(&w, s->alive_most_children_n);
    w_i32(&w, s->alive_most_kills_n);
    w_i32(&w, s->alive_most_infections_n);
    w_i32(&w, s->most_children_n);
    w_i64(&w, s->most_children_time);
    w_i32(&w, s->most_kills_n);
    w_i64(&w, s->most_kills_time);
    w_i32(&w, s->most_infections_n);
    w_i64(&w, s->most_infections_time);
    for (int i = 0; i < nstat; i++)
        w_i32(&w, ct_index(&ct, sc[i], false));
    w_series(&w, &s->population);
    w_series(&w, &s->deaths);
    w_series(&w, &s->births);
    w_series(&w, &s->oxygen);
    w_series(&w, &s->co2);

    w_i32(&w, live);
    for (int i = 0; i < world->norgs; i++) {
        const Organism *o = world->orgs[i];
        if (o->removed)
            continue;
        w_i32(&w, ct_index(&ct, o->gc, false));
        w_i32(&w, ct_index(&ct, o->infected_gc, false));
        w_i32(&w, o->r.x);
        w_i32(&w, o->r.y);
        w_i32(&w, o->r.width);
        w_i32(&w, o->r.height);
        w_i32(&w, o->n_children);
        w_i32(&w, o->parent_id);
        w_i32(&w, o->id);
        w_i32(&w, o->generation);
        w_i32(&w, o->infected_generation);
        w_i32(&w, o->total_children);
        w_i32(&w, o->total_kills);
        w_i32(&w, o->total_infected);
        w_i32(&w, o->segments);
        w_i32(&w, o->center_x);
        w_i32(&w, o->center_y);
        w_f64(&w, o->dcenter_x);
        w_f64(&w, o->dcenter_y);
        w_i32(&w, o->growth_ratio);
        w_f64(&w, o->mass);
        w_f64(&w, o->inertia);
        w_f64(&w, o->energy);
        w_i32(&w, o->size_rect.x);
        w_i32(&w, o->size_rect.y);
        w_i32(&w, o->size_rect.width);
        w_i32(&w, o->size_rect.height);
        w_f64(&w, o->theta);
        w_f64(&w, o->last_theta);
        w_f64(&w, o->dx);
        w_f64(&w, o->dy);
        w_f64(&w, o->dtheta);
        w_i32(&w, o->age);
        w_u8(&w, o->flash_color);
        w_i32(&w, o->frames_color);
        w_i32(&w, o->time_to_reproduce);
        w_i32(&w, o->has_grown);
        w_u8(&w, o->has_moved);
        w_u8(&w, o->alive);
        for (int k = 0; k < o->segments; k++) {
            w_i32(&w, o->start_x[k]);
            w_i32(&w, o->start_y[k]);
            w_i32(&w, o->end_x[k]);
            w_i32(&w, o->end_y[k]);
            w_f64(&w, o->m1[k]);
            w_f64(&w, o->m2[k]);
            w_f64(&w, o->m[k]);
            w_i32(&w, o->x1[k]);
            w_i32(&w, o->y1[k]);
            w_i32(&w, o->x2[k]);
            w_i32(&w, o->y2[k]);
            w_u8(&w, o->seg_color[k]);
        }
    }
    bool ok = w.ok;
    if (fclose(f) != 0)
        ok = false;
    ct_free(&ct);
    if (!ok)
        set_err(err, errn, "Error writing %s", path);
    return ok;
}

static GeneticCode *code_at(GeneticCode **codes, int ncodes, int idx)
{
    return (idx >= 0 && idx < ncodes) ? codes[idx] : NULL;
}

bool worldfile_load(World *dst, const char *path, char *err, size_t errn)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        set_err(err, errn, "This file can't be read");
        return false;
    }
    Rd r = {f, true};
    char magic[8];
    r_bytes(&r, magic, 8);
    if (!r.ok || memcmp(magic, k_magic, 8) != 0) {
        fclose(f);
        set_err(err, errn,
                "It seems that this file doesn't contain a SYN-BIOGENESIS world "
                "(Java .bgw worlds can't be read)");
        return false;
    }
    if ((uint32_t)r_i32(&r) != WORLD_FILE_VERSION) {
        fclose(f);
        set_err(err, errn, "Unsupported world file version");
        return false;
    }

    World w;
    memset(&w, 0, sizeof w);
    GeneticCode **codes = NULL;
    int ncodes = 0;
    bool bad = false;

    w.width = r_i32(&r);
    w.height = r_i32(&r);
    w.population = r_i32(&r);
    w.next_id = r_i32(&r);
    w.nframes = r_i32(&r);
    w.o2 = r_f64(&r);
    w.co2 = r_f64(&r);
    if (w.width <= 0 || w.height <= 0 || w.width > 1000000 || w.height > 1000000)
        bad = true;

    ncodes = bad ? 0 : r_i32(&r);
    if (ncodes < 0 || ncodes > 50000000)
        bad = true;
    if (!bad && ncodes > 0) {
        codes = calloc((size_t)ncodes, sizeof *codes);
        if (!codes)
            abort();
    }
    for (int i = 0; !bad && r.ok && i < ncodes; i++) {
        int sym = r_i32(&r), mir = r_i32(&r);
        bool disp = r_u8(&r);
        int repro = r_i32(&r), max_age = r_i32(&r), ng = r_i32(&r);
        if (sym < 1 || sym > 8 || ng < 1 || ng > 100000) {
            bad = true;
            break;
        }
        Gene *genes = malloc((size_t)ng * sizeof *genes);
        if (!genes)
            abort();
        for (int g = 0; g < ng; g++) {
            genes[g].length = r_f64(&r);
            genes[g].theta = r_f64(&r);
            genes[g].color = r_u8(&r);
            if (genes[g].color >= GENE_COLORS)
                bad = true;
        }
        codes[i] = gc_new_from_genes(genes, ng, sym, mir, disp);
        codes[i]->reproduce_energy = repro;
        codes[i]->max_age = max_age;
        free(genes);
    }

    WorldStats *s = &w.stats;
    if (!bad) {
        s->time = r_i64(&r);
        s->max_population = r_i32(&r);
        s->max_births = r_i32(&r);
        s->max_deaths = r_i32(&r);
        s->max_population_time = r_i64(&r);
        s->min_population = r_i32(&r);
        s->min_population_time = r_i64(&r);
        s->mass_extinctions = r_i32(&r);
        s->mass_extinction_state = r_i32(&r);
        s->created_organisms = r_i32(&r);
        s->population_sum = r_i64(&r);
        s->death_sum = r_i64(&r);
        s->death_last_time = r_i32(&r);
        s->birth_sum = r_i64(&r);
        s->birth_last_time = r_i32(&r);
        s->infections_sum = r_i64(&r);
        s->max_oxygen = r_f64(&r);
        s->max_oxygen_time = r_i64(&r);
        s->min_oxygen = r_f64(&r);
        s->min_oxygen_time = r_i64(&r);
        s->max_co2 = r_f64(&r);
        s->max_co2_time = r_i64(&r);
        s->min_co2 = r_f64(&r);
        s->min_co2_time = r_i64(&r);
        s->alive_most_children_n = r_i32(&r);
        s->alive_most_kills_n = r_i32(&r);
        s->alive_most_infections_n = r_i32(&r);
        s->most_children_n = r_i32(&r);
        s->most_children_time = r_i64(&r);
        s->most_kills_n = r_i32(&r);
        s->most_kills_time = r_i64(&r);
        s->most_infections_n = r_i32(&r);
        s->most_infections_time = r_i64(&r);
        GeneticCode **slots[9] = {&s->alive_most_children, &s->alive_most_kills,
                                  &s->alive_most_infections, &s->most_children,
                                  &s->most_kills,          &s->most_infections,
                                  &s->last_born,           &s->last_dead,
                                  &s->last_infected};
        for (int i = 0; i < 9; i++)
            gc_assign(slots[i], code_at(codes, ncodes, r_i32(&r)));
        if (!r_series(&r, &s->population) || !r_series(&r, &s->deaths) ||
            !r_series(&r, &s->births) || !r_series(&r, &s->oxygen) || !r_series(&r, &s->co2))
            bad = true;
    }

    int norgs = bad ? 0 : r_i32(&r);
    if (norgs < 0 || norgs > 10000000)
        bad = true;
    for (int i = 0; !bad && r.ok && i < norgs; i++) {
        GeneticCode *gc = code_at(codes, ncodes, r_i32(&r));
        GeneticCode *igc = code_at(codes, ncodes, r_i32(&r));
        if (!gc) {
            bad = true;
            break;
        }
        Organism *o = calloc(1, sizeof *o);
        if (!o)
            abort();
        o->world = dst;
        o->gc = gc_ref(gc);
        o->infected_gc = gc_ref(igc);
        o->r.x = r_i32(&r);
        o->r.y = r_i32(&r);
        o->r.width = r_i32(&r);
        o->r.height = r_i32(&r);
        o->n_children = r_i32(&r);
        o->parent_id = r_i32(&r);
        o->id = r_i32(&r);
        o->generation = r_i32(&r);
        o->infected_generation = r_i32(&r);
        o->total_children = r_i32(&r);
        o->total_kills = r_i32(&r);
        o->total_infected = r_i32(&r);
        int segs = r_i32(&r);
        if (segs != gc->ngenes * gc->symmetry) {
            org_free(o);
            bad = true;
            break;
        }
        org_create(o);
        o->center_x = r_i32(&r);
        o->center_y = r_i32(&r);
        o->dcenter_x = r_f64(&r);
        o->dcenter_y = r_f64(&r);
        o->growth_ratio = r_i32(&r);
        o->mass = r_f64(&r);
        o->inertia = r_f64(&r);
        o->energy = r_f64(&r);
        o->size_rect.x = r_i32(&r);
        o->size_rect.y = r_i32(&r);
        o->size_rect.width = r_i32(&r);
        o->size_rect.height = r_i32(&r);
        o->theta = r_f64(&r);
        o->last_theta = r_f64(&r);
        o->dx = r_f64(&r);
        o->dy = r_f64(&r);
        o->dtheta = r_f64(&r);
        o->age = r_i32(&r);
        o->flash_color = r_u8(&r);
        o->frames_color = r_i32(&r);
        o->time_to_reproduce = r_i32(&r);
        o->has_grown = r_i32(&r);
        o->has_moved = r_u8(&r);
        o->alive = r_u8(&r);
        for (int k = 0; k < o->segments; k++) {
            o->start_x[k] = r_i32(&r);
            o->start_y[k] = r_i32(&r);
            o->end_x[k] = r_i32(&r);
            o->end_y[k] = r_i32(&r);
            o->m1[k] = r_f64(&r);
            o->m2[k] = r_f64(&r);
            o->m[k] = r_f64(&r);
            o->x1[k] = r_i32(&r);
            o->y1[k] = r_i32(&r);
            o->x2[k] = r_i32(&r);
            o->y2[k] = r_i32(&r);
            o->seg_color[k] = r_u8(&r);
            if (o->seg_color[k] >= COL_COUNT)
                bad = true;
        }
        if (o->growth_ratio < 1 || o->growth_ratio > 16)
            bad = true;
        org_prepare_angles(o);
        world_append_raw(&w, o);
    }
    fclose(f);
    for (int i = 0; i < ncodes; i++)
        gc_unref(codes[i]); /* organisms and stats hold their own refs */
    free(codes);

    if (bad || !r.ok) {
        set_err(err, errn, "This world file is damaged or truncated");
        world_free(&w);
        return false;
    }
    /* success: swap in, keeping the destination's callbacks */
    void (*cb)(void *, Organism *) = dst->on_remove;
    void *ud = dst->ud;
    world_free(dst);
    *dst = w;
    dst->on_remove = cb;
    dst->ud = ud;
    for (int i = 0; i < dst->norgs; i++)
        dst->orgs[i]->world = dst;
    return true;
}
