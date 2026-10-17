/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Translated to C in 2026 from World.java
 * of Biogenesis 0.9 (https://biogenesis.sourceforge.net/),
 * Copyright (C) 2006-2010 Joan Queralt Molina.
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

#include <stdlib.h>
#include <string.h>

#include "sim/params.h"

#define GRID_CELL 32

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

void world_clear(World *w)
{
    for (int i = 0; i < w->norgs; i++) {
        if (w->on_remove)
            w->on_remove(w->ud, w->orgs[i]);
        org_free(w->orgs[i]);
    }
    w->norgs = 0;
    w->grid_valid = false;
}

void world_free(World *w)
{
    world_clear(w);
    stats_release(&w->stats);
    free(w->orgs);
    free(w->cell_start);
    free(w->cell_items);
    free(w->snapshot);
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

/* ---- collision index ---- */

static int clampi(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static void cell_range(const World *w, double minx, double maxx, double miny, double maxy,
                       int *cx0, int *cx1, int *cy0, int *cy1)
{
    /* floor division, clamped: anything outside the world lands in the
     * border cells, so inclusive envelope overlaps always share a cell */
    int x0 = (int)minx, x1 = (int)maxx, y0 = (int)miny, y1 = (int)maxy;
    x0 = x0 < 0 ? -1 : x0 / w->cell;
    x1 = x1 < 0 ? -1 : x1 / w->cell;
    y0 = y0 < 0 ? -1 : y0 / w->cell;
    y1 = y1 < 0 ? -1 : y1 / w->cell;
    *cx0 = clampi(x0, 0, w->grid_w - 1);
    *cx1 = clampi(x1, 0, w->grid_w - 1);
    *cy0 = clampi(y0, 0, w->grid_h - 1);
    *cy1 = clampi(y1, 0, w->grid_h - 1);
}

static void build_grid(World *w)
{
    w->cell = GRID_CELL;
    int gw = w->width / w->cell + 1, gh = w->height / w->cell + 1;
    if (gw != w->grid_w || gh != w->grid_h || !w->cell_start) {
        w->grid_w = gw;
        w->grid_h = gh;
        w->cell_start = xrealloc(w->cell_start, ((size_t)gw * gh + 1) * sizeof(int));
    }
    int ncells = gw * gh;
    if (w->snapshot_cap < w->norgs) {
        w->snapshot_cap = w->norgs * 2;
        w->snapshot = xrealloc(w->snapshot, (size_t)w->snapshot_cap * sizeof *w->snapshot);
    }
    w->snapshot_n = w->norgs;
    memset(w->cell_start, 0, ((size_t)ncells + 1) * sizeof(int));

    /* pass 1: envelopes and counts */
    long total = 0;
    for (int i = 0; i < w->norgs; i++) {
        Organism *o = w->orgs[i];
        w->snapshot[i] = o;
        o->tree_idx = i;
        o->env[0] = o->r.x;
        o->env[1] = (double)o->r.x + o->r.width;
        o->env[2] = o->r.y;
        o->env[3] = (double)o->r.y + o->r.height;
        int cx0, cx1, cy0, cy1;
        cell_range(w, o->env[0], o->env[1], o->env[2], o->env[3], &cx0, &cx1, &cy0, &cy1);
        for (int cy = cy0; cy <= cy1; cy++)
            for (int cx = cx0; cx <= cx1; cx++)
                w->cell_start[cy * gw + cx + 1]++;
        total += (long)(cx1 - cx0 + 1) * (cy1 - cy0 + 1);
    }
    for (int c = 0; c < ncells; c++)
        w->cell_start[c + 1] += w->cell_start[c];
    if (w->cell_items_cap < total) {
        w->cell_items_cap = (int)total * 2;
        w->cell_items = xrealloc(w->cell_items, (size_t)w->cell_items_cap * sizeof(int));
    }
    /* pass 2: fill, using a moving cursor per cell */
    int *cursor = malloc(((size_t)ncells + 1) * sizeof(int));
    if (!cursor)
        abort();
    memcpy(cursor, w->cell_start, ((size_t)ncells + 1) * sizeof(int));
    for (int i = 0; i < w->norgs; i++) {
        Organism *o = w->orgs[i];
        int cx0, cx1, cy0, cy1;
        cell_range(w, o->env[0], o->env[1], o->env[2], o->env[3], &cx0, &cx1, &cy0, &cy1);
        for (int cy = cy0; cy <= cy1; cy++)
            for (int cx = cx0; cx <= cx1; cx++)
                w->cell_items[cursor[cy * gw + cx]++] = i;
    }
    free(cursor);
    w->grid_valid = true;
}

static int cmp_tree_idx(const void *a, const void *b)
{
    const Organism *x = *(Organism *const *)a, *y = *(Organism *const *)b;
    return (x->tree_idx > y->tree_idx) - (x->tree_idx < y->tree_idx);
}

Organism *world_check_hit(World *w, Organism *org1)
{
    if (!w->grid_valid)
        return NULL;
    double qx0 = org1->r.x, qx1 = (double)org1->r.x + org1->r.width;
    double qy0 = org1->r.y, qy1 = (double)org1->r.y + org1->r.height;
    int cx0, cx1, cy0, cy1;
    cell_range(w, qx0, qx1, qy0, qy1, &cx0, &cx1, &cy0, &cy1);
    if (++w->stamp == 0)
        w->stamp = 1;
    int n = 0;
    for (int cy = cy0; cy <= cy1; cy++) {
        for (int cx = cx0; cx <= cx1; cx++) {
            int c = cy * w->grid_w + cx;
            for (int k = w->cell_start[c]; k < w->cell_start[c + 1]; k++) {
                Organism *o = w->snapshot[w->cell_items[k]];
                if (o->stamp == w->stamp)
                    continue;
                o->stamp = w->stamp;
                /* JTS Envelope.intersects: inclusive on every side */
                if (o->env[0] > qx1 || o->env[1] < qx0 || o->env[2] > qy1 || o->env[3] < qy0)
                    continue;
                if (n == w->cand_cap) {
                    w->cand_cap = w->cand_cap ? w->cand_cap * 2 : 64;
                    w->cand = xrealloc(w->cand, (size_t)w->cand_cap * sizeof *w->cand);
                }
                w->cand[n++] = o;
            }
        }
    }
    if (n > 1)
        qsort(w->cand, (size_t)n, sizeof *w->cand, cmp_tree_idx);
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
    build_grid(w);
    int l = w->norgs;
    for (int i = 0; i < l; i++) {
        Organism *b = w->orgs[i];
        if (!org_move(b))
            b->removed = true;
    }
    /* Compact out the organisms that ran out of energy, keeping order. The
     * collision index still references them, so they are freed only now. */
    int k = 0;
    for (int i = 0; i < w->norgs; i++) {
        Organism *b = w->orgs[i];
        if (b->removed) {
            if (w->on_remove)
                w->on_remove(w->ud, b);
            org_free(b);
        } else {
            w->orgs[k++] = b;
        }
    }
    w->norgs = k;
    w->grid_valid = false;
    w->nframes++;
    if (w->nframes % 256 == 0) {
        w->nframes = 0;
        stats_event_time(&w->stats, w->population, w->o2, w->co2);
    }
}
