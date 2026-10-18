/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Translated to C in 2026 from Utils.java (colorToString),
 * GeneticCode.java (draw) and VisibleWorld.java (ReviveAction) of
 * Biogenesis Color Mod 2.01 (https://sourceforge.net/projects/biogenesis/),
 * Copyright (C) 2006-2010 Joan Queralt Molina,
 * Color Mod Copyright (C) 2012-2022 MarcoDBAA.
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

#include <stdint.h>

#include "sim/params.h"
#include "ui/app.h"

/* Utils.colorToString's order and English names (messages_en.properties) */
const ColourName k_gene_colours[] = {
    {JC_GREEN, "green"},       {CM_FOREST, "forest"},     {CM_SPRING, "spring"},
    {CM_SUMMER, "summer"},     {CM_LIME, "lime"},         {CM_LEAF, "leaf"},
    {CM_C4, "c4plant"},        {CM_JADE, "jade"},         {CM_GRASS, "grass"},
    {CM_BARK, "bark"},         {CM_PURPLE, "purple"},     {JC_RED, "red"},
    {CM_FIRE, "fire"},         {JC_ORANGE, "orange"},     {CM_MAROON, "maroon"},
    {JC_PINK, "pink"},         {CM_CREAM, "cream"},       {JC_LIGHT_GRAY, "silver"},
    {CM_SPIKE, "spike"},       {CM_LILAC, "lilac"},       {JC_GRAY, "gray"},
    {CM_VIOLET, "violet"},     {CM_OLIVE, "olive"},       {CM_SKY, "sky"},
    {JC_BLUE, "blue"},         {CM_OCHRE, "ochre"},       {CM_FALLOW, "fallow"},
    {CM_SPORE, "spore"},       {JC_WHITE, "white"},       {CM_PLAGUE, "plague"},
    {CM_CORAL, "coral"},       {CM_MINT, "mint"},         {CM_LAVENDER, "lavender"},
    {JC_MAGENTA, "magenta"},   {CM_ROSE, "rose"},         {JC_CYAN, "cyan"},
    {CM_TEAL, "teal"},         {JC_YELLOW, "yellow"},     {CM_AUBURN, "auburn"},
    {CM_INDIGO, "indigo"},     {CM_BLOND, "blond"},       {CM_FLOWER, "flower"},
    {JC_DARK_GRAY, "dark gray"}, {CM_GOLD, "gold"},       {CM_DARK, "black"},
    {CM_EYE, "eye"},
};
const int k_n_gene_colours = (int)(sizeof k_gene_colours / sizeof k_gene_colours[0]);

const char *colour_name(JColor c)
{
    for (int i = 0; i < k_n_gene_colours; i++)
        if (k_gene_colours[i].color == c)
            return k_gene_colours[i].name;
    return "";
}

/* For the shared widgets (ui_color_combo): in the colour build a colour is
 * its packed RGB value. */
void bio_color_rgb(int c, uint8_t *r, uint8_t *g, uint8_t *b)
{
    *r = (uint8_t)JCOLOR_R((JColor)c);
    *g = (uint8_t)JCOLOR_G((JColor)c);
    *b = (uint8_t)JCOLOR_B((JColor)c);
}

/* GeneticCode.draw, without the drawing: segments in Color Mod's branching
 * layout, centred on (0, 0). */
int gc_layout(const GeneticCode *gc, GcLine *out, int max, int *width, int *height)
{
    int sym = gc->symmetry, ng = JLEN(gc->genes);
    int n = sym * ng;
    if (n > max || n <= 0) {
        *width = *height = 0;
        return 0;
    }
    int *x0 = calloc((size_t)n * 4, sizeof *x0);
    if (!x0)
        return 0;
    int *y0 = x0 + n, *x1 = y0 + n, *y1 = x1 + n;
#define AT(a, i, j) a[(i) * ng + (j)]
    int maxx = 0, minx = 0, maxy = 0, miny = 0;
    Vector2D v = {0, 0};
    for (int i = 0; i < sym; i++) {
        for (int j = 0; j < ng; j++) {
            const Gene *g = gc->genes[j];
            v.modulus = g->length;
            int br = g->branch;
            bool root = j == 0 || br == 0;
            if (root) {
                AT(x0, i, j) = AT(y0, i, j) = 0;
                if (gc->mirror == 0 || i % 2 == 0) {
                    v.theta = g->theta + i * 2 * M_PI / sym;
                } else {
                    v.theta = g->theta + (i - 1) * 2 * M_PI / sym;
                    vec_invert_x(&v);
                }
            } else {
                int from = br == -1 ? j - 1 : br - 1;
                AT(x0, i, j) = AT(x1, i, from);
                AT(y0, i, j) = AT(y1, i, from);
                v.theta += (gc->mirror == 0 || i % 2 == 0) ? g->theta : -g->theta;
            }
            AT(x1, i, j) = (int)j_lround(vec_get_x(&v) + AT(x0, i, j));
            AT(y1, i, j) = (int)j_lround(vec_get_y(&v) + AT(y0, i, j));
            maxx = j_imax(maxx, j_imax(AT(x0, i, j), AT(x1, i, j)));
            maxy = j_imax(maxy, j_imax(AT(y0, i, j), AT(y1, i, j)));
            minx = j_imin(minx, j_imin(AT(x0, i, j), AT(x1, i, j)));
            miny = j_imin(miny, j_imin(AT(y0, i, j), AT(y1, i, j)));
        }
    }
    int k = 0;
    for (int i = 0; i < sym; i++) {
        for (int j = 0; j < ng; j++, k++) {
            out[k] = (GcLine){AT(x0, i, j) + (-minx - maxx) / 2, AT(y0, i, j) + (-miny - maxy) / 2,
                              AT(x1, i, j) + (-minx - maxx) / 2, AT(y1, i, j) + (-miny - maxy) / 2,
                              gc->genes[j]->color};
        }
    }
#undef AT
    free(x0);
    *width = maxx - minx;
    *height = maxy - miny;
    return k;
}

/* VisibleWorld.ReviveAction: Color Mod also gives the organism a new life
 * expectancy and clears what it had become. */
void colour_revive(World *w, Organism *b)
{
    if (!b || b->alive)
        return;
    for (int i = 0; i < b->segments; i++)
        b->seg_color[i] = gene_get_color(gc_get_gene(b->gc, i % gc_get_n_genes(b->gc)));
    b->active = true;
    b->alive = true;
    b->has_moved = true;
    b->age = 1;
    b->max_age = P.max_age + (b->segments / P.age_divisor);
    b->isaplant = false;
    b->isaconsumer = false;
    b->isafungus = false;
    b->isenhanced = false;
    b->isakiller = false;
    b->isgray = false;
    b->isreproductive = false;
    b->isinfectious = false;
    b->iscoral = false;
    b->isauburn = false;
    b->isblond = false;
    b->issilver = false;
    b->segmentpretoucheffects = 0;
    b->transfersenergy = false;
    b->lengthfriend = -1;
    b->thetafriend = -1;
    b->jadefactor = 0;
    b->skyversion = 0;
    b->fallowversion = 0;
    b->plagueversion = 0;
    b->reproducelate = 0;
    b->candodge = false;
    b->dodge = false;
    b->hasdodged = true;
    b->isfrozen = false;
    b->allfrozen = false;
    b->isinjured = false;
    org_segments_create_effects(b);
    world_increase_population(w);
}
