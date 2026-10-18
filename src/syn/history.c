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

#include "syn/history.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "syn/family.h"

#ifndef HIST_CAP
#define HIST_CAP 4096 /* samples; then pairs merge and a sample covers twice the time */
#endif
#ifndef EPOCHS_CAP
#define EPOCHS_CAP 1024 /* base epochs; then pairs merge and an epoch is twice as long */
#endif
#define EPOCH_BASE0 16   /* time units in a base epoch to start with */
#define MERGE_UNITS 32 /* records this close together are one event, climbing */
#define HOLD 4         /* samples a new leader must hold the lead to count */


static struct {
    HistSample *s;
    int n, stride;
    int64_t commit_t;                      /* time of the last kept sample */
    int64_t birth0, death0, inf0;          /* the world's totals then */
    int64_t ubirth0, udeath0, uinf0;       /* and at the last time unit */
    Epoch *epochs;
    int nepochs, epoch_base;
    int64_t epoch_k0; /* epochs[i] is epoch number epoch_k0 + i */
    Event *ev;
    int nev, cap_ev;
    Line *lines;
    int nlines, cap_lines;
    uint32_t *map; /* line key -> index + 1, open addressing */
    int map_cap;
    int64_t t_start, seen_t;
    bool started;

    /* what the detectors last saw */
    int extinctions;
    int lead_col, col_cand, col_hold;
    int col_zero[HIST_COLOURS], col_peak[HIST_COLOURS];
    bool col_seen[HIST_COLOURS], col_gone[HIST_COLOURS];
    uint32_t lead_line, line_cand;
    int line_hold;
    int pop_record;
    int rec_children, rec_kills, rec_infected;
    bool co2_low, o2_low;
    History view;
} H;

/* ---- small helpers ---- */

/* Holding a code: 0.9 counts references; Color Mod's collector is told
 * what the history holds (history_mark). */
#ifdef BIO_COLOUR
static void keep_code(GeneticCode **slot, GeneticCode *gc)
{
    *slot = gc;
}
#else
#define keep_code gc_assign
#endif

static void *grow(void *p, int *cap, int need, size_t size)
{
    if (need <= *cap)
        return p;
    int c = *cap ? *cap : 64;
    while (c < need)
        c *= 2;
    void *q = realloc(p, (size_t)c * size);
    if (!q)
        abort();
    *cap = c;
    return q;
}

static int commonest_colour(const Organism *o)
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

/* The colours that make events: genes' own (0.9's seven, Color Mod's 46),
 * not the states Color Mod's segments pass through */
static bool gene_colour(int slot)
{
    Role r = syn_col_role(slot);
    return r != ROLE_INACTIVE && r != ROLE_DEAD && strcmp(syn_col_name(slot), "other") != 0;
}

static void num(char *out, size_t n, double v)
{
    /* as fmt_num (ui/app.c), which the history doesn't link against */
    char tmp[64];
    snprintf(tmp, sizeof tmp, "%.1f", fabs(v));
    char *dot = strchr(tmp, '.');
    if (dot && dot[1] == '0')
        *dot = '\0';
    dot = strchr(tmp, '.');
    int ilen = dot ? (int)(dot - tmp) : (int)strlen(tmp);
    char buf[96];
    int k = 0;
    if (v < 0 && strcmp(tmp, "0") != 0)
        buf[k++] = '-';
    for (int i = 0; i < ilen; i++) {
        buf[k++] = tmp[i];
        int left = ilen - i - 1;
        if (left > 0 && left % 3 == 0)
            buf[k++] = ',';
    }
    snprintf(buf + k, sizeof buf - (size_t)k, "%s", dot ? dot : "");
    snprintf(out, n, "%s", buf);
}

static void capital(char *s)
{
    if (s[0] >= 'a' && s[0] <= 'z')
        s[0] = (char)(s[0] - 'a' + 'A');
}

/* ---- lines ---- */

static unsigned map_hash(uint32_t k)
{
    k ^= k >> 16;
    k *= 0x7feb352du;
    k ^= k >> 15;
    return k;
}

static Line *line_of(uint32_t id)
{
    if (!id || !H.map_cap)
        return NULL;
    unsigned m = (unsigned)H.map_cap - 1, h = map_hash(id) & m;
    while (H.map[h]) {
        Line *l = &H.lines[H.map[h] - 1];
        if (l->id == id)
            return l;
        h = (h + 1) & m;
    }
    return NULL;
}

static void map_put(uint32_t id, int index)
{
    if ((H.nlines + 1) * 2 > H.map_cap) {
        int cap = H.map_cap ? H.map_cap * 2 : 1024;
        uint32_t *map = calloc((size_t)cap, sizeof *map);
        if (!map)
            abort();
        free(H.map);
        H.map = map;
        H.map_cap = cap;
        for (int i = 0; i < H.nlines; i++)
            if (i != index)
                map_put(H.lines[i].id, i);
    }
    unsigned m = (unsigned)H.map_cap - 1, h = map_hash(id) & m;
    while (H.map[h])
        h = (h + 1) & m;
    H.map[h] = (uint32_t)index + 1;
}

static Line *new_line(uint32_t id, const Organism *o, int64_t t)
{
    H.lines = grow(H.lines, &H.cap_lines, H.nlines + 1, sizeof *H.lines);
    int index = H.nlines++;
    Line *l = &H.lines[index];
    memset(l, 0, sizeof *l);
    l->id = id;
    l->founder = (int)id;
    l->founded = l->last_seen = t;
    l->colour = (uint8_t)commonest_colour(o);
#ifndef BIO_COLOUR
    /* 0.9's lines are the roots of the species' trees: named after the
     * organism the root began with */
    const Species *root = syn_family_get(id);
    if (root)
        l->founder = root->founder;
#endif
    map_put(id, index);
    return line_of(id);
}

/* The line an organism belongs to, founding it if new. */
static Line *line_for(const Organism *o, int64_t t)
{
    uint32_t key = syn_line_key(o);
    Line *l = line_of(key);
    if (!l && key)
        l = new_line(key, o, t);
    return l;
}

/* ---- events ---- */

static Event *add_event(int64_t t, EventKind kind, int key, const char *text)
{
    /* the same key a short while ago: that event climbs to here instead */
    if (key) {
        for (int i = H.nev - 1; i >= 0 && i >= H.nev - 64; i--) {
            if (H.ev[i].key != key)
                continue;
            if (t - H.ev[i].t <= MERGE_UNITS) {
                Event e = H.ev[i];
                memmove(&H.ev[i], &H.ev[i + 1], (size_t)(H.nev - i - 1) * sizeof *H.ev);
                e.t = t;
                snprintf(e.text, sizeof e.text, "%s", text);
                int at = H.nev - 1;
                while (at > 0 && H.ev[at - 1].t > t)
                    at--;
                memmove(&H.ev[at + 1], &H.ev[at], (size_t)(H.nev - 1 - at) * sizeof *H.ev);
                H.ev[at] = e;
                return &H.ev[at];
            }
            break;
        }
    }
    H.ev = grow(H.ev, &H.cap_ev, H.nev + 1, sizeof *H.ev);
    /* in time order (a record's time can be a unit back) */
    int at = H.nev;
    while (at > 0 && H.ev[at - 1].t > t)
        at--;
    memmove(&H.ev[at + 1], &H.ev[at], (size_t)(H.nev - at) * sizeof *H.ev);
    H.nev++;
    Event *e = &H.ev[at];
    memset(e, 0, sizeof *e);
    e->t = t;
    e->kind = kind;
    e->key = key;
    e->colour = -1;
    snprintf(e->text, sizeof e->text, "%s", text);
    return e;
}

static void event_gc(Event *e, GeneticCode *gc)
{
    keep_code(&e->gc, gc);
}

/* ---- epochs ---- */

static void champion_clear(Champion *c)
{
    memset(c, 0, sizeof *c);
    c->value = -1;
}

static void epoch_init(Epoch *a, int64_t k)
{
    memset(a, 0, sizeof *a);
    a->t0 = k * H.epoch_base;
    a->t1 = a->t0 + H.epoch_base;
    a->pop_min = INT_MAX;
    for (int c = 0; c < CH_COUNT; c++)
        champion_clear(&a->ch[c]);
}

/* The species with the most living at once in it: held by a pin. */
static void epoch_best(Epoch *a, uint32_t id, int n, int64_t t, bool hold)
{
    if (n <= a->best_n)
        return;
    if (hold && id != a->best) {
        syn_family_pin(id, 1);
        syn_family_pin(a->best, -1);
    }
    a->best = id;
    a->best_n = n;
    a->best_t = t;
}

/* src into dst. hold: dst owns its champions' codes (src's are given to it
 * or released); otherwise both only borrow. */
static void epoch_merge(Epoch *dst, Epoch *src, bool hold)
{
    if (src->t0 < dst->t0)
        dst->t0 = src->t0;
    if (src->t1 > dst->t1)
        dst->t1 = src->t1;
    dst->samples += src->samples;
    if (src->pop_min < dst->pop_min)
        dst->pop_min = src->pop_min;
    if (src->pop_max > dst->pop_max)
        dst->pop_max = src->pop_max;
    dst->pop_sum += src->pop_sum;
    dst->births += src->births;
    dst->deaths += src->deaths;
    dst->infections += src->infections;
    dst->extinctions += src->extinctions;
    for (int c = 0; c < HIST_COLOURS; c++)
        dst->seg[c] += src->seg[c];
    if (src->lead_n > dst->lead_n) {
        dst->lead_n = src->lead_n;
        dst->lead_line = src->lead_line;
    }
    /* src's pin goes with its best, or is let go */
    if (src->best_n > dst->best_n) {
        if (hold)
            syn_family_pin(dst->best, -1);
        dst->best = src->best;
        dst->best_n = src->best_n;
        dst->best_t = src->best_t;
    } else if (hold) {
        syn_family_pin(src->best, -1);
    }
    for (int k = 0; k < CH_COUNT; k++) {
        Champion *d = &dst->ch[k], *s = &src->ch[k];
        if (s->value > d->value) {
            if (hold)
                keep_code(&d->gc, NULL);
            *d = *s;
            if (hold)
                s->gc = NULL;
        } else if (hold) {
            keep_code(&s->gc, NULL);
        }
    }
}

/* Twice as long epochs: pairs merge. */
static void epochs_compact(void)
{
    int64_t k0 = H.epoch_k0 / 2;
    int n = (int)((H.epoch_k0 + H.nepochs - 1) / 2 - k0) + 1;
    Epoch *na = calloc((size_t)EPOCHS_CAP, sizeof *na);
    if (!na)
        abort();
    H.epoch_base *= 2;
    for (int i = 0; i < n; i++)
        epoch_init(&na[i], k0 + i);
    for (int i = 0; i < H.nepochs; i++)
        epoch_merge(&na[(H.epoch_k0 + i) / 2 - k0], &H.epochs[i], true);
    free(H.epochs);
    H.epochs = na;
    H.nepochs = n;
    H.epoch_k0 = k0;
}

static Epoch *epoch_at(int64_t t)
{
    if (!H.epochs) {
        H.epochs = calloc((size_t)EPOCHS_CAP, sizeof *H.epochs);
        if (!H.epochs)
            abort();
    }
    int64_t k = t / H.epoch_base;
    if (H.nepochs == 0) {
        H.epoch_k0 = k;
        epoch_init(&H.epochs[0], k);
        H.nepochs = 1;
    }
    while (k >= H.epoch_k0 + H.nepochs) {
        if (H.nepochs == EPOCHS_CAP) {
            epochs_compact();
            k = t / H.epoch_base;
            continue;
        }
        epoch_init(&H.epochs[H.nepochs], H.epoch_k0 + H.nepochs);
        H.nepochs++;
    }
    if (k < H.epoch_k0)
        k = H.epoch_k0;
    return &H.epochs[k - H.epoch_k0];
}

static void consider(Champion *c, double v, const Organism *o, int64_t t, bool died, bool hold)
{
    if (v <= c->value)
        return;
    c->value = v;
    c->id = o->id;
    c->generation = syn_generation(o);
    c->line = syn_line_key(o);
    c->t = t;
    c->died = died;
    c->colour = (uint8_t)commonest_colour(o);
    if (hold)
        keep_code(&c->gc, o->gc);
    else
        c->gc = o->gc;
}

static void observe(Champion ch[CH_COUNT], const Organism *o, int64_t t, bool died, bool hold)
{
    consider(&ch[CH_CHILDREN], o->total_children, o, t, died, hold);
    consider(&ch[CH_KILLS], o->total_kills, o, t, died, hold);
    consider(&ch[CH_INFECTED], o->total_infected, o, t, died, hold);
    consider(&ch[CH_GENERATION], syn_generation(o), o, t, died, hold);
    /* a corpse's energy and mass are what is left of it: those count while alive */
    if (!died) {
        consider(&ch[CH_RICHEST], o->energy, o, t, died, hold);
        consider(&ch[CH_HEAVIEST], o->mass, o, t, died, hold);
    }
}

/* ---- samples ---- */

static void samples_compact(void)
{
    int m = H.n / 2;
    for (int i = 0; i < m; i++) {
        const HistSample *a = &H.s[2 * i], *b = &H.s[2 * i + 1];
        /* the later one's time and lines; everything else averaged, the
         * maximum generation the larger */
        HistSample r = *b;
#define AVG(f) r.f = (a->f + b->f) * 0.5f
        AVG(pop);
        AVG(corpses);
        AVG(births);
        AVG(deaths);
        AVG(infections);
        AVG(o2);
        AVG(co2);
#ifdef BIO_COLOUR
        AVG(ch4);
#endif
        for (int c = 0; c < HIST_COLOURS; c++)
            AVG(seg[c]);
        AVG(gen_mean);
        AVG(energy_mean);
        AVG(mass_total);
        AVG(size_mean);
        AVG(age_mean);
        AVG(lines);
#undef AVG
        r.gen_max = fmaxf(a->gen_max, b->gen_max);
        H.s[i] = r;
    }
    H.n = m;
    H.stride *= 2;
}

/* Once a time unit: the world read, the epochs and detectors fed, and every
 * stride units a sample kept. */
static void unit(World *w)
{
    int64_t t = w->stats.time;
    WorldStats *st = &w->stats;
    int pop = 0, corpses = 0;
    double seg[HIST_COLOURS] = {0}, gen_sum = 0, energy = 0, mass = 0, size = 0, age = 0;
    int gen_max = 0;
    for (int i = 0; i < H.nlines; i++)
        H.lines[i].alive = 0;
    syn_family_unit(w, t);
    Epoch *ep = epoch_at(t);
    int top_n;
    uint32_t top = syn_family_top(&top_n);
    if (top)
        epoch_best(ep, top, top_n, t, true);
    for (int i = 0; i < w->norgs; i++) {
        Organism *o = w->orgs[i];
        if (o->removed)
            continue;
        if (!o->alive) {
            corpses++;
            continue;
        }
        pop++;
        for (int s = 0; s < o->segments; s++) {
            int slot = syn_seg(o, s);
            if (slot >= 0 && slot < HIST_COLOURS)
                seg[slot]++;
        }
        int gen = syn_generation(o);
        gen_sum += gen;
        if (gen > gen_max)
            gen_max = gen;
        energy += o->energy;
        mass += o->mass;
        size += o->segments;
        age += o->age >> 8;
        Line *l = line_for(o, t);
        if (l)
            l->alive++;
        observe(ep->ch, o, t, false, true);
    }

    /* the lines: who leads, who is new, who is gone */
    HistSample smp;
    memset(&smp, 0, sizeof smp);
    int living = 0;
    for (int i = 0; i < H.nlines; i++) {
        Line *l = &H.lines[i];
        if (l->alive > 0) {
            living++;
            l->last_seen = t;
            if (l->alive > l->peak) {
                l->peak = l->alive;
                l->peak_t = t;
            }
            if (l->peak >= 10 && l->peak * 20 >= pop)
                l->notable = true;
            /* the top lines, biggest first */
            int j = HIST_TOP - 1;
            if (l->alive > smp.top_n[j]) {
                while (j > 0 && l->alive > smp.top_n[j - 1]) {
                    smp.top_n[j] = smp.top_n[j - 1];
                    smp.top_line[j] = smp.top_line[j - 1];
                    j--;
                }
                smp.top_n[j] = (float)l->alive;
                smp.top_line[j] = l->id;
            }
        } else if (!l->extinct && l->peak > 0) {
            l->extinct = true;
            if (l->notable) {
                char text[96], pk[24], pt[24], lasted[24];
                snprintf(text, sizeof text, "The line of #%d dies out", l->founder);
                Event *e = add_event(t, EV_EXTINCTION, 0, text);
                num(pk, sizeof pk, l->peak);
                num(pt, sizeof pt, (double)l->peak_t);
                num(lasted, sizeof lasted, (double)(t - l->founded));
                snprintf(e->detail, sizeof e->detail, "after %s units; at its peak %s living (t %s)",
                         lasted, pk, pt);
                e->line = l->id;
                e->colour = l->colour;
            }
        }
    }
    if (smp.top_n[0] > ep->lead_n) {
        ep->lead_n = (int)smp.top_n[0];
        ep->lead_line = smp.top_line[0];
    }
    uint32_t lead = smp.top_n[0] >= 5 ? smp.top_line[0] : 0;
    if (lead && lead != H.lead_line) {
        if (lead == H.line_cand) {
            if (++H.line_hold >= HOLD) {
                H.lead_line = lead;
                Line *l = line_of(lead);
                if (l) {
                    l->led = l->notable = true;
                    char text[96], n[32], all[32], ft[32];
                    snprintf(text, sizeof text, "The line of #%d takes the lead", l->founder);
                    Event *e = add_event(t, EV_LINE, 0, text);
                    num(n, sizeof n, l->alive);
                    num(all, sizeof all, pop);
                    num(ft, sizeof ft, (double)l->founded);
                    snprintf(e->detail, sizeof e->detail, "%s living of %s; founded at t %s", n, all, ft);
                    e->line = l->id;
                    e->colour = l->colour;
                }
            }
        } else {
            H.line_cand = lead;
            H.line_hold = 1;
        }
    }

    /* the colours */
    int col = -1;
    for (int c = 0; c < HIST_COLOURS; c++) {
        ep->seg[c] += seg[c];
        if (seg[c] > 0) {
            if (seg[c] > H.col_peak[c])
                H.col_peak[c] = (int)seg[c];
            if (H.col_gone[c]) {
                H.col_gone[c] = false;
                char text[96];
                snprintf(text, sizeof text, "%s returns", syn_col_name(c));
                capital(text);
                Event *e = add_event(t, EV_COLOUR, 0, text);
                e->colour = c;
                snprintf(e->detail, sizeof e->detail, "%.0f segments", seg[c]);
            }
            /* a gene colour that was ever common enough to notice */
            H.col_seen[c] = gene_colour(c) && H.col_peak[c] >= 20;
            H.col_zero[c] = 0;
            if (gene_colour(c) && (col < 0 || seg[c] > seg[col]))
                col = c;
        } else if (H.col_seen[c] && !H.col_gone[c] && ++H.col_zero[c] >= 2) {
            H.col_gone[c] = true;
            char text[96];
            snprintf(text, sizeof text, "The last %s segment is gone", syn_col_name(c));
            Event *e = add_event(t, EV_EXTINCTION, 0, text);
            e->colour = c;
            snprintf(e->detail, sizeof e->detail, "no living organism has %s", syn_col_name(c));
        }
    }
    if (col >= 0 && col != H.lead_col) {
        if (col == H.col_cand) {
            if (++H.col_hold >= HOLD) {
                H.lead_col = col;
                char text[96];
                snprintf(text, sizeof text, "%s takes the lead", syn_col_name(col));
                capital(text);
                Event *e = add_event(t, EV_COLOUR, 0, text);
                e->colour = col;
                double all = 0;
                for (int c = 0; c < HIST_COLOURS; c++)
                    all += seg[c];
                snprintf(e->detail, sizeof e->detail, "%.0f%% of the living segments",
                         all > 0 ? 100 * seg[col] / all : 0);
            }
        } else {
            H.col_cand = col;
            H.col_hold = 1;
        }
    }

    /* the world's own counters */
    int births = (int)(st->birth_sum - H.ubirth0), deaths = (int)(st->death_sum - H.udeath0);
    int infections = (int)(st->infections_sum - H.uinf0);
    H.ubirth0 = st->birth_sum;
    H.udeath0 = st->death_sum;
    H.uinf0 = st->infections_sum;
    ep->samples++;
    ep->pop_sum += pop;
    if (pop < ep->pop_min)
        ep->pop_min = pop;
    if (pop > ep->pop_max)
        ep->pop_max = pop;
    ep->births += births;
    ep->deaths += deaths;
    ep->infections += infections;
    if (st->mass_extinctions > H.extinctions) {
        H.extinctions = st->mass_extinctions;
        ep->extinctions++;
        Event *e = add_event(t, EV_EXTINCTION, 0, "Mass extinction");
        char dn[24], pn[24];
        num(dn, sizeof dn, deaths);
        num(pn, sizeof pn, pop);
        snprintf(e->detail, sizeof e->detail, "%s deaths in the last unit, %s living", dn, pn);
    }
    /* the records as the world keeps them, at the times it gives */
    if (st->max_population > H.pop_record) {
        H.pop_record = st->max_population;
        char text[96], n[32];
        num(n, sizeof n, st->max_population);
        snprintf(text, sizeof text, "Population record: %s", n);
        Event *e = add_event(st->max_population_time, EV_RECORD, 'P', text);
        e->value = st->max_population;
        char ln[24];
        num(ln, sizeof ln, living);
        snprintf(e->detail, sizeof e->detail, "the most alive at once so far; %s lines living now",
                 ln);
    }
    struct {
        int n;
        int64_t t;
        int *seen;
        GeneticCode *gc;
        int key;
        const char *what;
    } rec[] = {{st->most_children_n, st->most_children_time, &H.rec_children, st->most_children, 'C',
                "children"},
               {st->most_kills_n, st->most_kills_time, &H.rec_kills, st->most_kills, 'K', "victims"},
               {st->most_infections_n, st->most_infections_time, &H.rec_infected, st->most_infections,
                'I', "infections"}};
    for (int i = 0; i < 3; i++) {
        if (rec[i].n <= *rec[i].seen)
            continue;
        *rec[i].seen = rec[i].n;
        char text[96], rn[24];
        num(rn, sizeof rn, rec[i].n);
        snprintf(text, sizeof text, "Record: %s %s", rn, rec[i].what);
        Event *e = add_event(rec[i].t, EV_RECORD, rec[i].key, text);
        e->value = rec[i].n;
        snprintf(e->detail, sizeof e->detail, "the most any organism has had");
        event_gc(e, rec[i].gc);
    }
    double gas = w->o2 + w->co2;
    struct {
        double v;
        bool *low;
        const char *name;
    } gases[] = {{w->co2, &H.co2_low, "CO2"}, {w->o2, &H.o2_low, "O2"}};
    for (int i = 0; i < 2 && gas > 0; i++) {
        char text[96], v[32];
        num(v, sizeof v, gases[i].v);
        if (!*gases[i].low && gases[i].v < 0.02 * gas) {
            *gases[i].low = true;
            snprintf(text, sizeof text, "%s runs out", gases[i].name);
            Event *e = add_event(t, EV_ATMOSPHERE, 0, text);
            snprintf(e->detail, sizeof e->detail, "%s left, under 2%% of the air", v);
        } else if (*gases[i].low && gases[i].v > 0.05 * gas) {
            *gases[i].low = false;
            snprintf(text, sizeof text, "%s recovers", gases[i].name);
            Event *e = add_event(t, EV_ATMOSPHERE, 0, text);
            snprintf(e->detail, sizeof e->detail, "%s, over 5%% of the air", v);
        }
    }

    /* a sample every stride units */
    if (H.n > 0 && t - H.commit_t < H.stride)
        return;
    if (H.n == HIST_CAP)
        samples_compact();
    double span = H.n > 0 && t > H.commit_t ? (double)(t - H.commit_t) : 1;
    smp.t = t;
    smp.pop = (float)pop;
    smp.corpses = (float)corpses;
    smp.births = (float)((st->birth_sum - H.birth0) / span);
    smp.deaths = (float)((st->death_sum - H.death0) / span);
    smp.infections = (float)((st->infections_sum - H.inf0) / span);
    smp.o2 = (float)w->o2;
    smp.co2 = (float)w->co2;
#ifdef BIO_COLOUR
    smp.ch4 = (float)w->ch4;
#endif
    for (int c = 0; c < HIST_COLOURS; c++)
        smp.seg[c] = (float)seg[c];
    smp.gen_mean = pop ? (float)(gen_sum / pop) : 0;
    smp.gen_max = (float)gen_max;
    smp.energy_mean = pop ? (float)(energy / pop) : 0;
    smp.mass_total = (float)mass;
    smp.size_mean = pop ? (float)(size / pop) : 0;
    smp.age_mean = pop ? (float)(age / pop) : 0;
    smp.lines = (float)living;
    H.s[H.n++] = smp;
    H.commit_t = t;
    H.birth0 = st->birth_sum;
    H.death0 = st->death_sum;
    H.inf0 = st->infections_sum;
}

/* ---- the public side ---- */

#ifdef BIO_COLOUR
/* Color Mod's collector keeps the codes the history shows */
static void history_mark(void)
{
    for (int i = 0; i < H.nepochs; i++)
        for (int k = 0; k < CH_COUNT; k++)
            jgc_mark(H.epochs[i].ch[k].gc);
    for (int i = 0; i < H.nev; i++)
        jgc_mark(H.ev[i].gc);
}
#endif

static void release_all(void)
{
    for (int i = 0; i < H.nepochs; i++)
        for (int k = 0; k < CH_COUNT; k++)
            keep_code(&H.epochs[i].ch[k].gc, NULL);
    for (int i = 0; i < H.nev; i++)
        keep_code(&H.ev[i].gc, NULL);
    H.nepochs = 0;
    H.nev = 0;
    H.nlines = 0;
    if (H.map)
        memset(H.map, 0, (size_t)H.map_cap * sizeof *H.map);
    H.n = 0;
}

void syn_history_reset(World *w, bool loaded)
{
    release_all();
#ifdef BIO_COLOUR
    jgc_add_marker(history_mark);
#endif
    if (!H.s) {
        H.s = malloc((size_t)HIST_CAP * sizeof *H.s);
        if (!H.s)
            abort();
    }
    WorldStats *st = &w->stats;
    H.stride = 1;
    H.epoch_base = EPOCH_BASE0;
    H.t_start = H.seen_t = st->time;
    H.birth0 = H.ubirth0 = st->birth_sum;
    H.death0 = H.udeath0 = st->death_sum;
    H.inf0 = H.uinf0 = st->infections_sum;
    H.extinctions = st->mass_extinctions;
    H.lead_col = H.col_cand = -1;
    H.col_hold = 0;
    memset(H.col_zero, 0, sizeof H.col_zero);
    memset(H.col_peak, 0, sizeof H.col_peak);
    memset(H.col_seen, 0, sizeof H.col_seen);
    memset(H.col_gone, 0, sizeof H.col_gone);
    H.lead_line = H.line_cand = 0;
    H.line_hold = 0;
    H.pop_record = loaded ? st->max_population : 0;
    H.rec_children = st->most_children_n;
    H.rec_kills = st->most_kills_n;
    H.rec_infected = st->most_infections_n;
    H.co2_low = H.o2_low = false;
    H.started = true;
    syn_family_reset(w);
    /* every line there is from the start (Color Mod's come from its clades) */
    for (int i = 0; i < w->norgs; i++)
        if (!w->orgs[i]->removed && w->orgs[i]->alive)
            line_for(w->orgs[i], st->time);

    char text[96], n[32], nl[24];
    num(n, sizeof n, w->population);
    num(nl, sizeof nl, H.nlines);
    if (loaded) {
        char tt[32];
        num(tt, sizeof tt, (double)st->time);
        snprintf(text, sizeof text, "World opened at t %s", tt);
        Event *e = add_event(st->time, EV_START, 0, text);
        snprintf(e->detail, sizeof e->detail,
                 "%s living, %s lines from here; what happened before is in its records", n,
                 nl);
    } else {
        snprintf(text, sizeof text, "Genesis: %s organisms", n);
        Event *e = add_event(st->time, EV_START, 0, text);
        snprintf(e->detail, sizeof e->detail, "a %d x %d world, each organism founding a line",
                 w->width, w->height);
    }
    unit(w);
}

void syn_history_frame(World *w)
{
    if (!H.started)
        syn_history_reset(w, true);
    syn_family_frame(w);
    if (w->stats.time != H.seen_t) {
        H.seen_t = w->stats.time;
        unit(w);
    }
}

void syn_history_removed(const World *w, const Organism *o)
{
    if (!H.started || !H.epochs)
        return;
    observe(epoch_at(w->stats.time)->ch, o, w->stats.time, true, true);
}

void syn_history_user(const World *w, int key, double amount, const char *text)
{
    if (!H.started)
        return;
    int64_t t = w->stats.time;
    /* repeats within a few units are one event */
    for (int i = H.nev - 1; key && i >= 0 && i >= H.nev - 8; i--) {
        Event *e = &H.ev[i];
        if (e->key == key && e->kind == EV_USER && t - e->t <= 4) {
            e->value += amount;
            e->count++;
            char v[32];
            num(v, sizeof v, e->value);
            if (amount != 0)
                snprintf(e->detail, sizeof e->detail, "%s in all, %d times", v, e->count);
            else
                snprintf(e->detail, sizeof e->detail, "%d times", e->count);
            return;
        }
    }
    Event *e = add_event(t, EV_USER, 0, text);
    e->key = key;
    e->count = 1;
    e->value = amount;
    if (amount != 0) {
        char v[32];
        num(v, sizeof v, amount);
        snprintf(e->detail, sizeof e->detail, "%s", v);
    }
}

void syn_history_free(void)
{
    release_all();
    syn_family_free();
    free(H.s);
    free(H.epochs);
    free(H.ev);
    free(H.lines);
    free(H.map);
    H.map = NULL;
    H.map_cap = 0;
    H.s = NULL;
    H.epochs = NULL;
    H.ev = NULL;
    H.lines = NULL;
    H.cap_ev = H.cap_lines = 0;
    H.started = false;
}

const History *syn_history(void)
{
    History *v = &H.view;
    v->s = H.s;
    v->n = H.n;
    v->stride = H.stride;
    v->epochs = H.epochs;
    v->nepochs = H.nepochs;
    v->epoch_base = H.epoch_base ? H.epoch_base : EPOCH_BASE0;
    v->ev = H.ev;
    v->nev = H.nev;
    v->t_start = H.t_start;
    v->t_now = H.seen_t;
    return v;
}

const Line *syn_history_line(uint32_t id)
{
    return line_of(id);
}

void syn_history_alive_best(const World *w, Champion out[CH_COUNT])
{
    for (int k = 0; k < CH_COUNT; k++)
        champion_clear(&out[k]);
    for (int i = 0; i < w->norgs; i++) {
        const Organism *o = w->orgs[i];
        if (!o->removed && o->alive)
            observe(out, o, w->stats.time, false, false);
    }
}

bool syn_history_merge(int64_t t0, int64_t t1, Epoch *out)
{
    memset(out, 0, sizeof *out);
    out->t0 = t0;
    out->t1 = t1;
    out->pop_min = INT_MAX;
    for (int k = 0; k < CH_COUNT; k++)
        champion_clear(&out->ch[k]);
    bool any = false;
    for (int i = 0; i < H.nepochs; i++) {
        Epoch *a = &H.epochs[i];
        if (a->t1 <= t0 || a->t0 >= t1 || a->samples == 0)
            continue;
        epoch_merge(out, a, false);
        any = true;
    }
    out->t0 = t0;
    out->t1 = t1;
    return any;
}

const char *syn_champion_name(ChampionKind k)
{
    static const char *const names[CH_COUNT] = {"CHILDREN", "VICTIMS",    "INFECTED",
                                                "RICHEST",  "GENERATION", "HEAVIEST"};
    return k >= 0 && k < CH_COUNT ? names[k] : "";
}

void syn_champion_value(ChampionKind k, double v, char *out, size_t n)
{
    if (v < 0) {
        snprintf(out, n, "-");
        return;
    }
    (void)k;
    num(out, n, v);
}
