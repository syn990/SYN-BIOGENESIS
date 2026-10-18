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

#include "syn/family.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "syn/palette.h"

/* The species in the order they were founded, so a parent always comes
 * before its children (forgetting keeps the order). */
static struct {
    Species *sp;
    int n, cap;
    uint32_t *map; /* id -> index + 1, open addressing */
    int map_cap;
#ifdef BIO_COLOUR
    uint32_t *kmap; /* clade -> index + 1 */
#endif
    uint32_t next_id; /* never reset: codes copied earlier keep their marks */
    int seen_next;    /* the world's next organism id when last looked at */
    int pop;
    uint32_t top;
    int top_n;
    unsigned version;
    bool started;
} F = {.next_id = 1};

/* ---- holding codes (as syn/history.c) ---- */

#ifdef BIO_COLOUR
static void hold(GeneticCode **slot, GeneticCode *gc)
{
    *slot = gc;
}

static void family_mark(void)
{
    for (int i = 0; i < F.n; i++)
        jgc_mark(F.sp[i].gc);
}
#else
#define hold gc_assign
#endif

/* ---- the maps ---- */

static unsigned mix32(uint32_t k)
{
    k ^= k >> 16;
    k *= 0x7feb352du;
    k ^= k >> 15;
    return k;
}

static uint64_t fnv(uint64_t h, const void *p, size_t n)
{
    const unsigned char *b = p;
    for (size_t i = 0; i < n; i++) {
        h ^= b[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

#define FNV0 0xcbf29ce484222325ULL

static void map_rebuild(void)
{
    int cap = 1024;
    while (cap < F.n * 2 + 2)
        cap *= 2;
    if (cap != F.map_cap) {
        free(F.map);
        F.map = malloc((size_t)cap * sizeof *F.map);
#ifdef BIO_COLOUR
        free(F.kmap);
        F.kmap = malloc((size_t)cap * sizeof *F.kmap);
        if (!F.kmap)
            abort();
#endif
        if (!F.map)
            abort();
        F.map_cap = cap;
    }
    memset(F.map, 0, (size_t)cap * sizeof *F.map);
#ifdef BIO_COLOUR
    memset(F.kmap, 0, (size_t)cap * sizeof *F.kmap);
#endif
    unsigned m = (unsigned)cap - 1;
    for (int i = 0; i < F.n; i++) {
        unsigned h = mix32(F.sp[i].id) & m;
        while (F.map[h])
            h = (h + 1) & m;
        F.map[h] = (uint32_t)i + 1;
#ifdef BIO_COLOUR
        h = mix32((uint32_t)(F.sp[i].key ^ (F.sp[i].key >> 32))) & m;
        while (F.kmap[h])
            h = (h + 1) & m;
        F.kmap[h] = (uint32_t)i + 1;
#endif
    }
}

static Species *by_id(uint32_t id)
{
    if (!id || !F.map_cap)
        return NULL;
    unsigned m = (unsigned)F.map_cap - 1, h = mix32(id) & m;
    while (F.map[h]) {
        Species *s = &F.sp[F.map[h] - 1];
        if (s->id == id)
            return s;
        h = (h + 1) & m;
    }
    return NULL;
}

#ifdef BIO_COLOUR
static Species *by_key(uint64_t key)
{
    if (!F.map_cap)
        return NULL;
    unsigned m = (unsigned)F.map_cap - 1, h = mix32((uint32_t)(key ^ (key >> 32))) & m;
    while (F.kmap[h]) {
        Species *s = &F.sp[F.kmap[h] - 1];
        if (s->key == key)
            return s;
        h = (h + 1) & m;
    }
    return NULL;
}
#endif

/* ---- species ---- */

static int commonest(const Organism *o)
{
    int count[SYN_NCOL] = {0};
    for (int s = 0; s < o->segments; s++) {
        int slot = syn_seg(o, s);
        if (slot >= 0 && slot < SYN_NCOL)
            count[slot]++;
    }
    int best = 0;
    for (int c = 1; c < SYN_NCOL; c++)
        if (count[c] > count[best])
            best = c;
    return best;
}

/* A new species; o (may be NULL) is the organism it begins with. */
static Species *add(uint32_t parent, uint64_t key, int founder, const Organism *o, int64_t t)
{
    if (F.n == F.cap) {
        F.cap = F.cap ? F.cap * 2 : 256;
        F.sp = realloc(F.sp, (size_t)F.cap * sizeof *F.sp);
        if (!F.sp)
            abort();
    }
    Species *s = &F.sp[F.n++];
    memset(s, 0, sizeof *s);
    s->id = F.next_id++;
    s->key = key;
    s->founder = founder;
    s->born = s->last_seen = s->peak_t = t;
    s->root = s->id;
    Species *p = by_id(parent);
    if (p) {
        s->parent = p->id;
        s->root = p->root;
        s->depth = p->depth + 1;
        p->kids++;
    }
    if (o) {
        s->members = 1;
        s->colour = (uint8_t)commonest(o);
        hold(&s->gc, o->gc);
    }
    /* the maps at most half full */
    if (F.n * 2 + 2 > F.map_cap)
        map_rebuild();
    else {
        unsigned m = (unsigned)F.map_cap - 1, h = mix32(s->id) & m;
        while (F.map[h])
            h = (h + 1) & m;
        F.map[h] = (uint32_t)F.n;
#ifdef BIO_COLOUR
        h = mix32((uint32_t)(key ^ (key >> 32))) & m;
        while (F.kmap[h])
            h = (h + 1) & m;
        F.kmap[h] = (uint32_t)F.n;
#endif
    }
    F.version++;
    return &F.sp[F.n - 1];
}

#ifdef BIO_COLOUR

/* "123", "123+456<789", or "user:123..." from the network corridor */
static const char *clade(const Organism *o)
{
    return o->gc && o->gc->clade_id ? o->gc->clade_id : NULL;
}

/* where the last step begins, or NULL for a root */
static const char *last_step(const char *c, size_t len)
{
    const char *start = c;
    for (size_t i = 0; i < len; i++)
        if (c[i] == ':')
            start = c + i + 1;
    for (const char *p = c + len; p > start; p--)
        if (p[-1] == '+' || p[-1] == '-' || p[-1] == '<' || p[-1] == '>')
            return p - 1;
    return NULL;
}

static int number_at(const char *p, const char *end)
{
    while (p < end && (*p < '0' || *p > '9'))
        p++;
    int v = 0;
    for (; p < end && *p >= '0' && *p <= '9'; p++)
        v = v * 10 + (*p - '0');
    return v;
}

/* The species of the clade c[0..len), and of each one before it, founded
 * as needed: those never seen alive have no code. */
static Species *clade_species(const char *c, size_t len, const Organism *o, int64_t t)
{
    uint64_t key = fnv(FNV0, c, len);
    Species *s = by_key(key);
    if (s)
        return s;
    const char *step = last_step(c, len);
    uint32_t parent = 0;
    if (step) {
        Species *p = clade_species(c, (size_t)(step - c), NULL, t);
        parent = p->id;
    }
    const char *from = step ? step : c;
    return add(parent, key, number_at(from, c + len), o, t);
}

static Species *place(Organism *o, int64_t t)
{
    const char *c = clade(o);
    if (!c)
        return NULL;
    size_t len = strlen(c);
    Species *s = by_key(fnv(FNV0, c, len));
    if (s) {
        s->members++;
        if (!s->gc && o->gc) {
            hold(&s->gc, o->gc);
            s->colour = (uint8_t)commonest(o);
        }
        return s;
    }
    return clade_species(c, len, o, t);
}

static Species *found(const Organism *o)
{
    const char *c = clade(o);
    return c ? by_key(fnv(FNV0, c, strlen(c))) : NULL;
}

#else

/* The body plan: what makes a species. */
static uint64_t plan_key(const GeneticCode *gc)
{
    uint64_t h = FNV0;
    int v[3] = {gc->symmetry, gc->mirror, gc->ngenes};
    h = fnv(h, v, sizeof v);
    for (int i = 0; i < gc->ngenes; i++)
        h = fnv(h, &gc->genes[i].color, 1);
    return h;
}

/* A newborn (or a code found without a species): into its parent's
 * species if the plan is the same, else a new one under it. */
static Species *place(Organism *o, int64_t t)
{
    GeneticCode *gc = o->gc;
    if (!gc)
        return NULL;
    uint64_t key = plan_key(gc);
    Species *p = by_id(gc->lineage);
    if (p && p->key == key) {
        p->members++;
        return p;
    }
    Species *s = add(p ? p->id : 0, key, o->id, o, t);
    gc->lineage = s->id;
    return s;
}

static Species *found(const Organism *o)
{
    if (!o->gc)
        return NULL;
    Species *s = by_id(o->gc->lineage);
    return s && s->key == plan_key(o->gc) ? s : NULL;
}

#endif

/* ---- the public side ---- */

static void release(void)
{
    for (int i = 0; i < F.n; i++)
        hold(&F.sp[i].gc, NULL);
    F.n = 0;
    if (F.map_cap)
        map_rebuild();
    F.top = 0;
    F.top_n = 0;
    F.version++;
}

void syn_family_reset(World *w)
{
    release();
#ifdef BIO_COLOUR
    jgc_add_marker(family_mark);
#endif
    F.started = true;
    F.seen_next = w->next_id;
    int64_t t = w->stats.time;
#ifdef BIO_COLOUR
    syn_family_unit(w, t);
#else
    /* the world as found: one species per plan, a root each */
    for (int i = 0; i < w->norgs; i++) {
        Organism *o = w->orgs[i];
        if (o->removed || !o->alive || !o->gc)
            continue;
        uint64_t key = plan_key(o->gc);
        Species *s = NULL;
        for (int j = 0; j < F.n && !s; j++)
            if (F.sp[j].key == key)
                s = &F.sp[j];
        if (s)
            s->members++;
        else
            s = add(0, key, o->id, o, t);
        o->gc->lineage = s->id;
    }
    syn_family_unit(w, t);
#endif
}

void syn_family_frame(World *w)
{
    if (!F.started || w->next_id == F.seen_next)
        return;
    int lo = F.seen_next;
    F.seen_next = w->next_id;
    for (int i = 0; i < w->norgs; i++) {
        Organism *o = w->orgs[i];
        if (!o->removed && o->id >= lo)
            place(o, w->stats.time);
    }
}

void syn_family_unit(World *w, int64_t t)
{
    if (!F.started)
        return;
    for (int i = 0; i < F.n; i++)
        F.sp[i].alive = F.sp[i].carriers = 0;
    int pop = 0;
    for (int i = 0; i < w->norgs; i++) {
        Organism *o = w->orgs[i];
        if (o->removed || !o->alive)
            continue;
        pop++;
        /* pasted, created or revived since: a species of its own */
        Species *s = found(o);
        if (!s)
            s = place(o, t);
        if (s)
            s->alive++;
#ifndef BIO_COLOUR
        /* an infected organism's next child takes the infecting code's species */
        if (o->infected_gc) {
            Species *c = by_id(o->infected_gc->lineage);
            if (c)
                c->carriers++;
        }
#endif
    }
    F.pop = pop;
    F.top = 0;
    F.top_n = 0;
    bool forget = false;
    for (int i = 0; i < F.n; i++) {
        Species *s = &F.sp[i];
        if (s->alive > 0) {
            s->last_seen = t;
            s->extinct = false;
            if (s->alive > s->peak) {
                s->peak = s->alive;
                s->peak_t = t;
            }
            if (s->peak >= FAM_NOTABLE && s->alive * 50 >= pop)
                s->notable = true;
            if (s->alive > F.top_n) {
                F.top_n = s->alive;
                F.top = s->id;
            }
        } else if (s->carriers == 0) {
            s->extinct = true;
        }
    }
    /* the forgotten, children first: a parent can follow its last child */
    for (int i = F.n - 1; i >= 0; i--) {
        Species *s = &F.sp[i];
        if (!s->extinct || s->notable || s->kids > 0 || s->pins > 0)
            continue;
        Species *p = by_id(s->parent);
        if (p)
            p->kids--;
        hold(&s->gc, NULL);
        s->id = 0;
        forget = true;
    }
    if (forget) {
        int k = 0;
        for (int i = 0; i < F.n; i++)
            if (F.sp[i].id)
                F.sp[k++] = F.sp[i];
        F.n = k;
        map_rebuild();
    }
    F.version++;
}

void syn_family_free(void)
{
    release();
    free(F.sp);
    free(F.map);
#ifdef BIO_COLOUR
    free(F.kmap);
    F.kmap = NULL;
#endif
    F.sp = NULL;
    F.map = NULL;
    F.cap = F.map_cap = 0;
    F.started = false;
}

uint32_t syn_family_of(const Organism *o)
{
    const Species *s = o ? found(o) : NULL;
    return s ? s->id : 0;
}

const Species *syn_family_get(uint32_t id)
{
    return by_id(id);
}

int syn_family_count(void)
{
    return F.n;
}

const Species *syn_family_at(int i)
{
    return i >= 0 && i < F.n ? &F.sp[i] : NULL;
}

int syn_family_index(uint32_t id)
{
    const Species *s = by_id(id);
    return s ? (int)(s - F.sp) : -1;
}

uint32_t syn_family_top(int *alive)
{
    if (alive)
        *alive = F.top_n;
    return F.top;
}

void syn_family_pin(uint32_t id, int d)
{
    Species *s = by_id(id);
    if (s)
        s->pins += d;
}

unsigned syn_family_version(void)
{
    return F.version;
}

/* ---- in words ---- */

static const char *nth(int i)
{
    static char b[16];
    int n = i + 1;
    const char *suf = (n % 100 >= 11 && n % 100 <= 13) ? "th"
                      : n % 10 == 1                    ? "st"
                      : n % 10 == 2                    ? "nd"
                      : n % 10 == 3                    ? "rd"
                                                       : "th";
    snprintf(b, sizeof b, "%d%s", n, suf);
    return b;
}

#define ARROW " to " /* the interface font has no arrow */

void syn_family_change(const Species *s, char *out, size_t n)
{
    out[0] = '\0';
    const Species *p = s ? by_id(s->parent) : NULL;
    if (!p || !p->gc || !s->gc) {
        if (p)
            snprintf(out, n, "a clade of its own");
        return;
    }
    const GeneticCode *a = p->gc, *b = s->gc;
    size_t k = 0;
#define PUT(...)                                                                                   \
    do {                                                                                           \
        if (k < n) {                                                                               \
            if (k)                                                                                 \
                k += (size_t)snprintf(out + k, n - k, ", ");                                       \
            if (k < n)                                                                             \
                k += (size_t)snprintf(out + k, n - k, __VA_ARGS__);                                \
        }                                                                                          \
    } while (0)
    if (a->symmetry != b->symmetry)
        PUT("%d" ARROW "%d arm%s", a->symmetry, b->symmetry, b->symmetry > 1 ? "s" : "");
    if (a->mirror != b->mirror)
        PUT("mirroring %s", b->mirror ? "on" : "off");
    int na = syn_genes(a), nb = syn_genes(b);
    if (na == nb) {
        int changed = 0;
        for (int i = 0; i < na; i++) {
            int ca = syn_gene_col(a, i), cb = syn_gene_col(b, i);
            if (ca == cb)
                continue;
            if (++changed <= 2)
                PUT("%s segment %s" ARROW "%s", nth(i), syn_col_name(ca), syn_col_name(cb));
        }
        if (changed > 2)
            PUT("%d more", changed - 2);
    } else if (nb == na + 1 || nb == na - 1) {
        /* one segment more or less: where */
        const GeneticCode *lo = nb < na ? b : a, *hi = nb < na ? a : b;
        int nl = nb < na ? nb : na, at = nl;
        for (int i = 0; i < nl; i++)
            if (syn_gene_col(lo, i) != syn_gene_col(hi, i)) {
                at = i;
                break;
            }
        bool same = true;
        for (int i = at; i < nl && same; i++)
            same = syn_gene_col(lo, i) == syn_gene_col(hi, i + 1);
        if (same)
            PUT("%s %s segment (%s)", nb > na ? "+" : "-", syn_col_name(syn_gene_col(hi, at)),
                nth(at));
        else
            PUT("%d" ARROW "%d segments, recoloured", na, nb);
    } else {
        PUT("%d" ARROW "%d segments", na, nb);
    }
    if (!k)
        snprintf(out, n, "lengths and angles only");
#undef PUT
}

void syn_family_plan(const GeneticCode *gc, char *out, size_t n)
{
    if (!gc) {
        snprintf(out, n, "never seen alive here");
        return;
    }
    int ng = syn_genes(gc);
    size_t k = (size_t)snprintf(out, n, "%d arm%s%s; %d segment%s%s:", gc->symmetry, gc->symmetry > 1 ? "s" : "",
                                gc->mirror ? ", mirrored" : "", ng, ng != 1 ? "s" : "",
                                gc->symmetry > 1 ? " per arm" : "");
    /* each colour once, in the order it first appears */
    int seen[64], cnt[64], m = 0;
    for (int i = 0; i < ng; i++) {
        int c = syn_gene_col(gc, i), j = 0;
        while (j < m && seen[j] != c)
            j++;
        if (j == m && m < 64) {
            seen[m] = c;
            cnt[m++] = 0;
        }
        if (j < m)
            cnt[j]++;
    }
    for (int j = 0; j < m && k < n; j++) {
        if (cnt[j] > 1)
            k += (size_t)snprintf(out + k, n - k, "%s %d %s", j ? "," : "", cnt[j], syn_col_name(seen[j]));
        else
            k += (size_t)snprintf(out + k, n - k, "%s %s", j ? "," : "", syn_col_name(seen[j]));
    }
}
