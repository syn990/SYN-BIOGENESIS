/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Translated to C in 2026 from World.java
 * of Biogenesis 0.9 (https://biogenesis.sourceforge.net/),
 * Copyright (C) 2006-2010 Joan Queralt Molina.
 *
 * The collision index is translated from JTS 1.15.0's STRtree
 * (https://github.com/locationtech/jts), Copyright (c) 2016 Vivid
 * Solutions, under the Eclipse Distribution License v1.0: see LICENSE.JTS.
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

#include "sim/world.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "sim/params.h"

static void *xrealloc(void *p, size_t n)
{
    void *q = realloc(p, n ? n : 1);
    if (!q)
        abort();
    return q;
}

void world_init(World *w)
{
    memset(w, 0, sizeof *w);
    w->width = P.world_width;
    w->height = P.world_height;
    stats_init(&w->stats);
}

static void world_bury(World *w)
{
    for (int i = 0; i < w->ngrave; i++)
        org_free(w->grave[i]);
    w->ngrave = 0;
}

void world_clear(World *w)
{
    for (int i = 0; i < w->norgs; i++) {
        if (w->on_remove)
            w->on_remove(w->ud, w->orgs[i]);
        org_free(w->orgs[i]);
    }
    w->norgs = 0;
    world_bury(w);
    w->tree_valid = false;
    w->str_built = false;
    w->snapshot_n = 0;
    w->str_nnodes = 0;
    w->str_nkids = 0;
    w->str_root = 0;
}

void world_free(World *w)
{
    world_clear(w);
    stats_release(&w->stats);
    free(w->orgs);
    free(w->grave);
    free(w->snapshot);
    free(w->str_nodes);
    free(w->str_kids);
    free(w->str_keys);
    free(w->str_a);
    free(w->str_b);
    free(w->str_x);
    free(w->cand);
    memset(w, 0, sizeof *w);
}

int world_new_id(World *w)
{
    return w->next_id++;
}

int64_t world_time_units(const World *w)
{
    return w->stats.time;
}

int world_n_corpses(const World *w)
{
    return w->norgs - w->population;
}

void world_increase_population(World *w)
{
    w->population++;
    stats_population_increase(&w->stats, w->population);
}

void world_decrease_population(World *w)
{
    w->population--;
    stats_population_decrease(&w->stats, w->population);
}

void world_add_o2(World *w, double q)
{
    w->o2 += q;
}

void world_add_co2(World *w, double q)
{
    w->co2 += q;
}

void world_decrease_o2(World *w, double q)
{
    w->o2 -= jmin(q, w->o2);
}

void world_decrease_co2(World *w, double q)
{
    w->co2 -= jmin(q, w->co2);
}

double world_respiration(World *w, double q)
{
    double d = jmin(q, w->o2);
    w->o2 -= d;
    w->co2 += d;
    return d;
}

double world_photosynthesis(World *w, double q)
{
    q /= P.green_obtained_energy_divisor;
    q = jmin(jmin(q, q * w->co2 / P.drain_subs_divisor), w->co2);
    w->co2 -= q;
    w->o2 += q;
    return q;
}

void world_append_raw(World *w, Organism *o)
{
    if (w->norgs == w->cap) {
        w->cap = w->cap ? w->cap * 2 : 64;
        w->orgs = xrealloc(w->orgs, (size_t)w->cap * sizeof *w->orgs);
    }
    w->orgs[w->norgs++] = o;
}

void world_add_organism(World *w, Organism *child, Organism *parent)
{
    world_append_raw(w, child);
    if (parent)
        stats_organism_born(&w->stats, child, parent);
    stats_organism_created(&w->stats);
    world_increase_population(w);
}

void world_organism_has_died(World *w, Organism *dying, Organism *killer)
{
    stats_organism_die(&w->stats, dying, killer);
}

void world_organism_has_been_infected(World *w, Organism *infected, Organism *infector)
{
    stats_organism_infects(&w->stats, infected, infector);
}

void world_genesis(World *w)
{
    world_clear(w);
    w->nframes = 0;
    w->o2 = P.initial_o2;
    w->co2 = P.initial_co2;
    w->next_id = 0;
    w->population = 0;
    w->width = P.world_width;
    w->height = P.world_height;
    stats_release(&w->stats);
    stats_init(&w->stats);
    for (int i = 0; i < P.initial_organisms; i++) {
        Organism *b = org_new(w);
        if (org_random_create(b))
            world_add_organism(w, b, NULL);
        else
            org_free(b);
    }
}

void world_disperse_all(World *w)
{
    for (int i = 0; i < w->norgs; i++) {
        Organism *b = w->orgs[i];
        if (!b->removed && !b->alive)
            org_use_energy(b, b->energy);
    }
}

void world_kill_all(World *w)
{
    for (int i = 0; i < w->norgs; i++) {
        Organism *b = w->orgs[i];
        if (!b->removed && b->alive)
            org_die(b, NULL);
    }
}

Organism *world_find_organism_at(World *w, int x, int y)
{
    Organism *dead = NULL;
    for (int i = 0; i < w->norgs; i++) {
        Organism *b = w->orgs[i];
        if (b->removed)
            continue;
        if (jrect_contains(&b->r, x, y)) {
            if (b->alive)
                return b;
            dead = b;
        }
    }
    return dead;
}

Organism *world_fast_check_hit(World *w, Organism *b1)
{
    /* Java returned b1 itself on a hit; callers only test for non-null. */
    for (int i = 0; i < w->norgs; i++) {
        Organism *b = w->orgs[i];
        if (b != b1 && !b->removed && jrect_intersects(&b1->r, &b->r))
            return b1;
    }
    return NULL;
}

/* ---- collision index ----
 *
 * World.time inserts every organism's frame-start bounding box into a new
 * JTS STRtree (jts-core 1.15.0, node capacity 10), and checkHit visits what
 * STRtree.query returns in the order it returns it. The first candidate in
 * contact wins, so that order is part of the simulation. This is the same
 * tree: sort-tile-recursive packing with stable sorts on box centres
 * (Collections.sort), the same vertical slices, the same depth-first query.
 */

typedef struct StrKey {
    double key;
    int pos; /* place in the list being sorted: keeps the sort stable */
    int ref;
} StrKey;

#define STR_NODE_CAPACITY 10

/* Envelope(x1, x2, y1, y2) */
static void env_init(double *e, double x1, double x2, double y1, double y2)
{
    e[0] = x1 < x2 ? x1 : x2;
    e[1] = x1 < x2 ? x2 : x1;
    e[2] = y1 < y2 ? y1 : y2;
    e[3] = y1 < y2 ? y2 : y1;
}

/* Envelope.intersects: inclusive on every side */
static bool env_intersects(const double *a, const double *b)
{
    return !(b[0] > a[1] || b[1] < a[0] || b[2] > a[3] || b[3] < a[2]);
}

static const double *ref_env(const World *w, int ref)
{
    return ref >= 0 ? w->snapshot[ref]->env : w->str_nodes[~ref].env;
}

static int cmp_key(const void *a, const void *b)
{
    const StrKey *x = a, *y = b;
    if (x->key != y->key)
        return x->key > y->key ? 1 : -1;
    return (x->pos > y->pos) - (x->pos < y->pos);
}

/* Sorts refs[0..n) by box centre, x (axis 0) or y (axis 2), stably. */
static void str_sort(World *w, int *refs, int n, int axis)
{
    for (int i = 0; i < n; i++) {
        const double *e = ref_env(w, refs[i]);
        w->str_keys[i] = (StrKey){(e[axis] + e[axis + 1]) / 2.0, i, refs[i]};
    }
    qsort(w->str_keys, (size_t)n, sizeof *w->str_keys, cmp_key);
    for (int i = 0; i < n; i++)
        refs[i] = w->str_keys[i].ref;
}

/* AbstractSTRtree.createParentBoundables, for one vertical slice: sorted by
 * centre y, then cut into nodes of 10 in that order. */
static void str_pack_slice(World *w, int *refs, int n, int level, int *out, int *nout)
{
    str_sort(w, refs, n, 2);
    for (int i = 0; i < n; i += STR_NODE_CAPACITY) {
        if (w->str_nnodes == w->str_nodes_cap) {
            w->str_nodes_cap = w->str_nodes_cap ? w->str_nodes_cap * 2 : 64;
            w->str_nodes = xrealloc(w->str_nodes, (size_t)w->str_nodes_cap * sizeof *w->str_nodes);
        }
        StrNode *nd = &w->str_nodes[w->str_nnodes];
        nd->level = level;
        /* the root's bounds are computed by the first query, the others'
         * when the next level up is sorted; a saved tree is always queried */
        nd->has_bounds = true;
        nd->kid0 = w->str_nkids;
        nd->nkids = n - i < STR_NODE_CAPACITY ? n - i : STR_NODE_CAPACITY;
        for (int k = 0; k < nd->nkids; k++) {
            int ref = refs[i + k];
            const double *e = ref_env(w, ref);
            w->str_kids[w->str_nkids++] = ref;
            if (k == 0) {
                memcpy(nd->env, e, sizeof nd->env);
            } else {
                if (e[0] < nd->env[0]) nd->env[0] = e[0];
                if (e[1] > nd->env[1]) nd->env[1] = e[1];
                if (e[2] < nd->env[2]) nd->env[2] = e[2];
                if (e[3] > nd->env[3]) nd->env[3] = e[3];
            }
        }
        out[(*nout)++] = ~w->str_nnodes++;
    }
}

/* STRtree.createParentBoundables: sorted by centre x, cut into
 * ceil(sqrt(ceil(n / 10))) vertical slices, each packed into nodes. */
static int str_parents(World *w, const int *refs, int n, int level, int *out)
{
    int min_leaf_count = (int)ceil(n / (double)STR_NODE_CAPACITY);
    int *sorted = w->str_x;
    memcpy(sorted, refs, (size_t)n * sizeof *sorted);
    str_sort(w, sorted, n, 0);
    int slice_count = (int)ceil(sqrt(min_leaf_count));
    int slice_capacity = (int)ceil(n / (double)slice_count);
    int nout = 0;
    for (int j = 0, i = 0; j < slice_count; j++) {
        int len = n - i < slice_capacity ? n - i : slice_capacity;
        str_pack_slice(w, sorted + i, len, level, out, &nout);
        i += len;
    }
    return nout;
}

static void build_tree(World *w)
{
    int n = w->norgs;
    if (w->snapshot_cap < n) {
        w->snapshot_cap = n * 2;
        w->snapshot = xrealloc(w->snapshot, (size_t)w->snapshot_cap * sizeof *w->snapshot);
    }
    if (w->str_scratch_cap < n) {
        w->str_scratch_cap = n * 2;
        size_t c = (size_t)w->str_scratch_cap;
        w->str_keys = xrealloc(w->str_keys, c * sizeof *w->str_keys);
        w->str_a = xrealloc(w->str_a, c * sizeof *w->str_a);
        w->str_b = xrealloc(w->str_b, c * sizeof *w->str_b);
        w->str_x = xrealloc(w->str_x, c * sizeof *w->str_x);
        /* each item and each node but the root is one node's child */
        w->str_kids_cap = (int)c * 2;
        w->str_kids = xrealloc(w->str_kids, (size_t)w->str_kids_cap * sizeof *w->str_kids);
    }
    int *level = w->str_a, *parents = w->str_b;
    for (int i = 0; i < n; i++) {
        Organism *o = w->orgs[i];
        w->snapshot[i] = o;
        env_init(o->env, o->r.x, (double)o->r.x + o->r.width, o->r.y,
                 (double)o->r.y + o->r.height);
        level[i] = i;
    }
    w->snapshot_n = n;
    w->str_nnodes = 0;
    w->str_nkids = 0;
    w->str_root = 0;
    w->str_built = false;
    w->tree_valid = true;
    if (n == 0)
        return;
    /* AbstractSTRtree.createHigherLevels */
    for (int lv = 0;; lv++) {
        int np = str_parents(w, level, n, lv, parents);
        if (np == 1) {
            w->str_root = parents[0];
            return;
        }
        int *t = level;
        level = parents;
        parents = t;
        n = np;
    }
}

/* AbstractSTRtree.queryInternal */
static void str_query(World *w, int node, const double *q, int *n)
{
    const StrNode *nd = &w->str_nodes[node];
    for (int k = 0; k < nd->nkids; k++) {
        int ref = w->str_kids[nd->kid0 + k];
        if (!env_intersects(ref_env(w, ref), q))
            continue;
        if (ref < 0) {
            str_query(w, ~ref, q, n);
        } else {
            if (*n == w->cand_cap) {
                w->cand_cap = w->cand_cap ? w->cand_cap * 2 : 64;
                w->cand = xrealloc(w->cand, (size_t)w->cand_cap * sizeof *w->cand);
            }
            w->cand[(*n)++] = w->snapshot[ref];
        }
    }
}

Organism *world_check_hit(World *w, Organism *org1)
{
    if (!w->tree_valid)
        return NULL;
    w->str_built = true;
    if (w->str_root == 0)
        return NULL;
    double q[4];
    env_init(q, org1->r.x, (double)org1->r.x + org1->r.width, org1->r.y,
             (double)org1->r.y + org1->r.height);
    int n = 0;
    if (env_intersects(w->str_nodes[~w->str_root].env, q))
        str_query(w, ~w->str_root, q, &n);
    for (int i = 0; i < n; i++) {
        Organism *c = w->cand[i];
        if (c == org1)
            continue;
        if (!jrect_intersects(&org1->r, &c->r))
            continue;
        if (c->energy >= BIO_TOL && org1->energy >= BIO_TOL) {
            if (org_contact(org1, c))
                return org1;
        }
    }
    return NULL;
}

void world_time(World *w)
{
    world_bury(w);
    build_tree(w);
    int l = w->norgs;
    for (int i = 0; i < l; i++) {
        Organism *b = w->orgs[i];
        if (!org_move(b))
            b->removed = true;
    }
    /* Compact out the organisms that ran out of energy, keeping order. The
     * collision index still references them, so they wait in the grave. */
    int k = 0;
    for (int i = 0; i < w->norgs; i++) {
        Organism *b = w->orgs[i];
        if (b->removed) {
            if (w->on_remove)
                w->on_remove(w->ud, b);
            if (w->ngrave == w->grave_cap) {
                w->grave_cap = w->grave_cap ? w->grave_cap * 2 : 16;
                w->grave = xrealloc(w->grave, (size_t)w->grave_cap * sizeof *w->grave);
            }
            w->grave[w->ngrave++] = b;
        } else {
            w->orgs[k++] = b;
        }
    }
    w->norgs = k;
    w->tree_valid = false;
    w->nframes++;
    if (w->nframes % 256 == 0) {
        w->nframes = 0;
        stats_event_time(&w->stats, w->population, w->o2, w->co2);
    }
}
