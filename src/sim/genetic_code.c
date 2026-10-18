/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Translated to C in 2026 from GeneticCode.java and Gene.java
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

#include "sim/genetic_code.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "sim/jgeom.h"
#include "sim/jmath.h"
#include "sim/params.h"

void gene_randomize_color(Gene *g)
{
    int max_prob = P.red_prob + P.green_prob + P.blue_prob + P.cyan_prob + P.white_prob +
                   P.gray_prob + P.yellow_prob;
    int prob = jr_next_int(&g_rng, max_prob);
    int ac = P.red_prob;
    if (prob < ac) {
        g->color = COL_RED;
        return;
    }
    ac += P.green_prob;
    if (prob < ac) {
        g->color = COL_GREEN;
        return;
    }
    ac += P.blue_prob;
    if (prob < ac) {
        g->color = COL_BLUE;
        return;
    }
    ac += P.cyan_prob;
    if (prob < ac) {
        g->color = COL_CYAN;
        return;
    }
    ac += P.white_prob;
    if (prob < ac) {
        g->color = COL_WHITE;
        return;
    }
    ac += P.gray_prob;
    if (prob < ac) {
        g->color = COL_GRAY;
        return;
    }
    g->color = COL_YELLOW;
}

void gene_randomize_length(Gene *g)
{
    g->length = 2.0 + jr_next_double(&g_rng) * 16.0;
}

void gene_randomize_theta(Gene *g)
{
    g->theta = jr_next_double(&g_rng) * 2.0 * M_PI;
}

uint32_t gene_new_id(void)
{
    static uint32_t next;
    return ++next;
}

void gene_randomize(Gene *g)
{
    g->id = gene_new_id();
    gene_randomize_length(g);
    gene_randomize_theta(g);
    gene_randomize_color(g);
}

static GeneticCode *gc_alloc(int ngenes)
{
    GeneticCode *gc = calloc(1, sizeof *gc + (size_t)ngenes * sizeof(Gene));
    if (!gc)
        abort();
    gc->refs = 1;
    gc->ngenes = ngenes;
    return gc;
}

static void gc_calculate_reproduce_energy(GeneticCode *gc)
{
    gc->reproduce_energy = 40 + 3 * gc->ngenes * gc->symmetry;
}

static int random_mirror(void)
{
    return jr_next_int(&g_rng, 2);
}

static int random_symmetry(void)
{
    return jr_next_int(&g_rng, 8) + 1;
}

GeneticCode *gc_new_random(void)
{
    int mirror = random_mirror();
    int symmetry = random_symmetry();
    int nsegments = (1 + jr_next_int(&g_rng, 8)) * symmetry;
    if (nsegments % symmetry != 0)
        nsegments += symmetry - (nsegments % symmetry);
    int ngenes = nsegments / symmetry;
    GeneticCode *gc = gc_alloc(ngenes);
    gc->mirror = mirror;
    gc->symmetry = symmetry;
    for (int i = 0; i < ngenes; i++)
        gene_randomize(&gc->genes[i]);
    gc->disperse_children = jr_next_boolean(&g_rng);
    gc_calculate_reproduce_energy(gc);
    gc->max_age = P.max_age;
    return gc;
}

GeneticCode *gc_new_mutated(const GeneticCode *parent)
{
    int added = -1, removed = -1;
    int ngenes, symmetry, mirror;

    if (bio_random_mutation())
        mirror = random_mirror();
    else
        mirror = parent->mirror;
    if (bio_random_mutation()) {
        symmetry = random_symmetry();
        ngenes = parent->ngenes;
    } else {
        symmetry = parent->symmetry;
        if (bio_random_mutation()) {
            if (jr_next_boolean(&g_rng)) {
                if (parent->ngenes >= 100) {
                    ngenes = parent->ngenes;
                } else {
                    ngenes = parent->ngenes + 1;
                    added = jr_next_int(&g_rng, ngenes);
                }
            } else {
                if (parent->ngenes <= 1) {
                    ngenes = parent->ngenes;
                } else {
                    ngenes = parent->ngenes - 1;
                    removed = jr_next_int(&g_rng, parent->ngenes);
                }
            }
        } else {
            ngenes = parent->ngenes;
        }
    }

    GeneticCode *gc = gc_alloc(ngenes);
    gc->mirror = mirror;
    gc->symmetry = symmetry;
    for (int i = 0, j = 0; i < ngenes; i++, j++) {
        if (removed == j) {
            i--;
            continue;
        }
        if (added == i) {
            gene_randomize(&gc->genes[i]);
            j--;
            continue;
        }
        bool rlen = bio_random_mutation();
        bool rtheta = bio_random_mutation();
        bool rcolor = bio_random_mutation();
        bool rback = bio_random_mutation(); /* drawn but unused, as in 0.9 */
        const Gene *src = &parent->genes[j];
        if (rlen || rtheta || rcolor || rback) {
            Gene *g = &gc->genes[i];
            g->id = gene_new_id();
            if (rlen)
                gene_randomize_length(g);
            else
                g->length = src->length;
            if (rtheta)
                gene_randomize_theta(g);
            else
                g->theta = src->theta;
            if (rcolor)
                gene_randomize_color(g);
            else
                g->color = src->color;
        } else {
            gc->genes[i] = *src;
        }
    }
    if (bio_random_mutation())
        gc->disperse_children = jr_next_boolean(&g_rng);
    else
        gc->disperse_children = parent->disperse_children;
    gc_calculate_reproduce_energy(gc);
    gc->max_age = P.max_age;
    gc->lineage = parent->lineage;
    return gc;
}

GeneticCode *gc_new_from_genes(const Gene *genes, int n, int symmetry, int mirror, bool disperse)
{
    GeneticCode *gc = gc_alloc(n);
    if (n > 0)
        memcpy(gc->genes, genes, (size_t)n * sizeof(Gene));
    for (int i = 0; i < n; i++)
        gc->genes[i].id = gene_new_id();
    gc->max_age = P.max_age;
    gc->mirror = mirror;
    gc->symmetry = symmetry;
    gc->disperse_children = disperse;
    gc_calculate_reproduce_energy(gc);
    return gc;
}

GeneticCode *gc_ref(GeneticCode *gc)
{
    if (gc)
        gc->refs++;
    return gc;
}

void gc_unref(GeneticCode *gc)
{
    if (gc && --gc->refs == 0)
        free(gc);
}

void gc_assign(GeneticCode **slot, GeneticCode *gc)
{
    if (*slot == gc)
        return;
    gc_ref(gc);
    gc_unref(*slot);
    *slot = gc;
}

int gc_layout(const GeneticCode *gc, GcLine *out, int max, int *width, int *height)
{
    int n = 0;
    int maxx = 0, minx = 0, maxy = 0, miny = 0;
    for (int i = 0; i < gc->symmetry; i++) {
        double mod = 0, th = 0;
        int px = 0, py = 0;
        for (int j = 0; j < gc->ngenes; j++) {
            const Gene *g = &gc->genes[j];
            int x0, y0;
            mod = g->length;
            if (j == 0) {
                x0 = y0 = 0;
                if (gc->mirror == 0 || i % 2 == 0) {
                    th = g->theta + i * 2 * M_PI / gc->symmetry;
                } else {
                    th = g->theta + (i - 1) * 2 * M_PI / gc->symmetry;
                    th = M_PI - th;
                }
            } else {
                x0 = px;
                y0 = py;
                if (gc->mirror == 0 || i % 2 == 0)
                    th += g->theta;
                else
                    th += -g->theta;
            }
            int x1 = jround(mod * bio_cos(th) + x0);
            int y1 = jround(mod * bio_sin(th) + y0);
            px = x1;
            py = y1;
            if (n < max) {
                out[n].x0 = x0;
                out[n].y0 = y0;
                out[n].x1 = x1;
                out[n].y1 = y1;
                out[n].color = g->color;
                n++;
            }
            maxx = x0 > maxx ? x0 : maxx;
            maxx = x1 > maxx ? x1 : maxx;
            maxy = y0 > maxy ? y0 : maxy;
            maxy = y1 > maxy ? y1 : maxy;
            minx = x0 < minx ? x0 : minx;
            minx = x1 < minx ? x1 : minx;
            miny = y0 < miny ? y0 : miny;
            miny = y1 < miny ? y1 : miny;
        }
    }
    int ox = (-minx - maxx) / 2, oy = (-miny - maxy) / 2;
    for (int k = 0; k < n; k++) {
        out[k].x0 += ox;
        out[k].x1 += ox;
        out[k].y0 += oy;
        out[k].y1 += oy;
    }
    if (width)
        *width = maxx - minx;
    if (height)
        *height = maxy - miny;
    return n;
}

const char *bio_color_name(int c)
{
    static const char *names[COL_COUNT] = {"green", "red",  "cyan",   "blue",
                                           "white", "gray", "yellow", "brown"};
    return (c >= 0 && c < COL_COUNT) ? names[c] : "";
}

void bio_color_rgb(int c, uint8_t *r, uint8_t *g, uint8_t *b)
{
    /* java.awt.Color constants, and Utils.ColorBROWN for corpses */
    static const uint8_t rgb[COL_COUNT][3] = {
        {0, 255, 0},     {255, 0, 0},   {0, 255, 255}, {0, 0, 255},
        {255, 255, 255}, {128, 128, 128}, {255, 255, 0}, {150, 75, 0},
    };
    if (c < 0 || c >= COL_COUNT)
        c = COL_WHITE;
    *r = rgb[c][0];
    *g = rgb[c][1];
    *b = rgb[c][2];
}
