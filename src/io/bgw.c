/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Reads and writes the world files of Biogenesis 0.9
 * (https://biogenesis.sourceforge.net/), Copyright (C) 2006-2010 Joan
 * Queralt Molina: the serialized form of World.java, Organism.java,
 * GeneticCode.java, Gene.java and WorldStatistics.java, and of the JDK and
 * JTS classes they hold.
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

#include "io/bgw.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io/jser.h"
#include "sim/params.h"

/* ---- the classes, as Java 27 describes them ---- */

#define F(t, n) {t, n, NULL}
#define FO(t, n, s) {t, n, s}

static const JField f_world[] = {
    F('I', "NEXT_ID"), F('D', "_CO2"), F('D', "_O2"), F('I', "_height"),
    F('I', "_population"), F('I', "_width"), F('I', "nFrames"),
    FO('L', "_organisms", "Ljava/util/List;"),
    FO('L', "colDetTree", "Lorg/locationtech/jts/index/strtree/STRtree;"),
    FO('L', "worldStatistics", "Lbiogenesis/WorldStatistics;"),
};
static const JClass c_world = {"biogenesis.World", 700, 0x02, 10, f_world, NULL};

static const JField f_sync_coll[] = {
    FO('L', "c", "Ljava/util/Collection;"),
    FO('L', "mutex", "Ljava/lang/Object;"),
};
static const JClass c_sync_coll = {"java.util.Collections$SynchronizedCollection",
                                   3053995032091335093LL, 0x03, 2, f_sync_coll, NULL};
static const JField f_sync_list[] = {FO('L', "list", "Ljava/util/List;")};
/* Java 8 writes 0x02 here; Java 27 0x03 (it has a writeObject) */
static const JClass c_sync_list = {"java.util.Collections$SynchronizedList",
                                   -7754090372962971524LL, 0x03, 1, f_sync_list, &c_sync_coll};

static const JField f_arraylist[] = {F('I', "size")};
static const JClass c_arraylist = {"java.util.ArrayList", 8683452581122892189LL, 0x03, 1,
                                   f_arraylist, NULL};

static const JField f_rect[] = {F('I', "height"), F('I', "width"), F('I', "x"), F('I', "y")};
static const JClass c_rect = {"java.awt.Rectangle", -4345857070255674764LL, 0x02, 4, f_rect, NULL};

static const JField f_org[] = {
    F('D', "_I"), F('I', "_ID"), F('I', "_age"), F('I', "_centerX"), F('I', "_centerY"),
    F('D', "_dCenterX"), F('D', "_dCenterY"), F('D', "_energy"), F('I', "_framesColor"),
    F('I', "_generation"), F('I', "_growthRatio"), F('I', "_infectedGeneration"),
    F('D', "_lastTheta"), F('D', "_mass"), F('I', "_nChildren"), F('I', "_nTotalChildren"),
    F('I', "_nTotalInfected"), F('I', "_nTotalKills"), F('I', "_parentID"), F('I', "_segments"),
    F('D', "_theta"), F('I', "_timeToReproduce"), F('Z', "alive"), F('D', "dtheta"),
    F('D', "dx"), F('D', "dy"), F('I', "hasGrown"), F('Z', "hasMoved"),
    FO('L', "_color", "Ljava/awt/Color;"), FO('[', "_endPointX", "[I"),
    FO('[', "_endPointY", "[I"), FO('L', "_geneticCode", "Lbiogenesis/GeneticCode;"),
    FO('L', "_infectedGeneticCode", "Lbiogenesis/GeneticCode;"), FO('[', "_m", "[D"),
    FO('[', "_m1", "[D"), FO('[', "_m2", "[D"), FO('[', "_segColor", "[Ljava/awt/Color;"),
    FO('L', "_sizeRect", "Ljava/awt/Rectangle;"), FO('[', "_startPointX", "[I"),
    FO('[', "_startPointY", "[I"), FO('L', "_world", "Lbiogenesis/World;"),
    FO('L', "lastFrame", "Ljava/awt/Rectangle;"), FO('[', "x1", "[I"), FO('[', "x2", "[I"),
    FO('[', "y1", "[I"), FO('[', "y2", "[I"),
};
static const JClass c_org = {"biogenesis.Organism", 700, 0x02, 46, f_org, &c_rect};

static const JClass c_ints = {"[I", 5600894804908749477LL, 0x02, 0, NULL, NULL};
static const JClass c_doubles = {"[D", 4514449696888150558LL, 0x02, 0, NULL, NULL};
static const JClass c_colors = {"[Ljava.awt.Color;", 4045880353309156309LL, 0x02, 0, NULL, NULL};
static const JClass c_genes = {"[Lbiogenesis.Gene;", -8710523010866530344LL, 0x02, 0, NULL, NULL};

static const JField f_code[] = {
    F('Z', "_disperseChildren"), F('I', "_max_age"), F('I', "_mirror"),
    F('I', "_reproduceEnergy"), F('I', "_symmetry"), FO('[', "_genes", "[Lbiogenesis/Gene;"),
};
static const JClass c_code = {"biogenesis.GeneticCode", 700, 0x02, 6, f_code, NULL};

static const JField f_gene[] = {F('D', "_length"), F('D', "_theta"),
                                FO('L', "_color", "Ljava/awt/Color;")};
static const JClass c_gene = {"biogenesis.Gene", 700, 0x02, 3, f_gene, NULL};

static const JField f_color[] = {
    F('F', "falpha"), F('I', "value"), FO('L', "cs", "Ljava/awt/color/ColorSpace;"),
    FO('[', "frgbvalue", "[F"), FO('[', "fvalue", "[F"),
};
static const JClass c_color = {"java.awt.Color", 118526816881161077LL, 0x02, 5, f_color, NULL};

static const JField f_abstract_tree[] = {
    F('Z', "built"), F('I', "nodeCapacity"), FO('L', "itemBoundables", "Ljava/util/ArrayList;"),
    FO('L', "root", "Lorg/locationtech/jts/index/strtree/AbstractNode;"),
};
static const JClass c_abstract_tree = {"org.locationtech.jts.index.strtree.AbstractSTRtree",
                                       -3886435814360241337LL, 0x02, 4, f_abstract_tree, NULL};
static const JClass c_tree = {"org.locationtech.jts.index.strtree.STRtree", 259274702368956900LL,
                              0x02, 0, NULL, &c_abstract_tree};
static const JField f_abstract_node[] = {
    F('I', "level"), FO('L', "bounds", "Ljava/lang/Object;"),
    FO('L', "childBoundables", "Ljava/util/ArrayList;"),
};
static const JClass c_abstract_node = {"org.locationtech.jts.index.strtree.AbstractNode",
                                       6493722185909573708LL, 0x02, 3, f_abstract_node, NULL};
static const JClass c_node = {"org.locationtech.jts.index.strtree.STRtree$STRtreeNode",
                              3887880253333058187LL, 0x02, 0, NULL, &c_abstract_node};
static const JField f_env[] = {F('D', "maxx"), F('D', "maxy"), F('D', "minx"), F('D', "miny")};
static const JClass c_env = {"org.locationtech.jts.geom.Envelope", 5873921885273102420LL, 0x02, 4,
                             f_env, NULL};
static const JField f_item[] = {FO('L', "bounds", "Ljava/lang/Object;"),
                                FO('L', "item", "Ljava/lang/Object;")};
static const JClass c_item = {"org.locationtech.jts.index.strtree.ItemBoundable",
                              -5476789078882979168LL, 0x02, 2, f_item, NULL};

static const JField f_stats[] = {
    F('I', "aliveBeingMostChildrenNumber"), F('I', "aliveBeingMostInfectionsNumber"),
    F('I', "aliveBeingMostKillsNumber"), F('I', "beingMostChildrenNumber"),
    F('J', "beingMostChildrenTime"), F('I', "beingMostInfectionsNumber"),
    F('J', "beingMostInfectionsTime"), F('I', "beingMostKillsNumber"),
    F('J', "beingMostKillsTime"), F('I', "birthLastTime"), F('J', "birthSum"),
    F('I', "createdOrganisms"), F('I', "deathLastTime"), F('J', "deathSum"),
    F('J', "infectionsSum"), F('I', "massExtintionState"), F('I', "massExtintions"),
    F('I', "maxBirths"), F('D', "maxCarbonDioxide"), F('J', "maxCarbonDioxideTime"),
    F('I', "maxDeaths"), F('D', "maxOxygen"), F('J', "maxOxygenTime"), F('I', "maxPopulation"),
    F('J', "maxPopulationTime"), F('D', "minCarbonDioxide"), F('J', "minCarbonDioxideTime"),
    F('D', "minOxygen"), F('J', "minOxygenTime"), F('I', "minPopulation"),
    F('J', "minPopulationTime"), F('J', "populationSum"), F('J', "time"),
    FO('L', "aliveBeingMostChildren", "Lbiogenesis/GeneticCode;"),
    FO('L', "aliveBeingMostInfections", "Lbiogenesis/GeneticCode;"),
    FO('L', "aliveBeingMostKills", "Lbiogenesis/GeneticCode;"),
    FO('L', "beingMostChildren", "Lbiogenesis/GeneticCode;"),
    FO('L', "beingMostInfections", "Lbiogenesis/GeneticCode;"),
    FO('L', "beingMostKills", "Lbiogenesis/GeneticCode;"),
    FO('L', "birthList", "Ljava/util/List;"), FO('L', "carbonDioxideList", "Ljava/util/List;"),
    FO('L', "deathList", "Ljava/util/List;"),
    FO('L', "lastBornBeing", "Lbiogenesis/GeneticCode;"),
    FO('L', "lastDeadBeing", "Lbiogenesis/GeneticCode;"),
    FO('L', "lastInfectedBeing", "Lbiogenesis/GeneticCode;"),
    FO('L', "oxygenList", "Ljava/util/List;"), FO('L', "populationList", "Ljava/util/List;"),
};
static const JClass c_stats = {"biogenesis.WorldStatistics", 700, 0x02, 47, f_stats, NULL};

static const JClass c_number = {"java.lang.Number", -8742448824652078965LL, 0x02, 0, NULL, NULL};
static const JField f_double[] = {F('D', "value")};
static const JClass c_double = {"java.lang.Double", -9172774392245257468LL, 0x02, 1, f_double,
                                &c_number};

/* java.awt.Color constants (getRGB) of each BioColor, and the darker()
 * shades Organism.getTypeColor also accepts */
static const uint32_t k_rgb[COL_COUNT] = {
    [COL_GREEN] = 0xff00ff00, [COL_RED] = 0xffff0000,  [COL_CYAN] = 0xff00ffff,
    [COL_BLUE] = 0xff0000ff,  [COL_WHITE] = 0xffffffff, [COL_GRAY] = 0xff808080,
    [COL_YELLOW] = 0xffffff00, [COL_BROWN] = 0xff964b00,
};
static const uint32_t k_rgb_dark[COL_COUNT] = {
    [COL_GREEN] = 0xff00b200, [COL_RED] = 0xffb20000,  [COL_CYAN] = 0xff00b2b2,
    [COL_BLUE] = 0xff0000b2,  [COL_WHITE] = 0xffb2b2b2, [COL_GRAY] = 0xff595959,
    [COL_YELLOW] = 0xffb2b200, [COL_BROWN] = 0,
};

static void set_err(char *err, size_t n, const char *fmt, ...)
{
    if (!err || !n)
        return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(err, n, fmt, ap);
    va_end(ap);
}

/* ---- identity maps ---- */

typedef struct {
    const void *k;
    void *v;
} MapE;

typedef struct {
    MapE *e;
    size_t cap, n;
} Map;

static size_t map_slot(const Map *m, const void *k)
{
    uintptr_t h = (uintptr_t)k;
    h ^= h >> 17;
    h *= 0x9e3779b97f4a7c15ULL;
    size_t i = (size_t)(h ^ (h >> 29)) & (m->cap - 1);
    while (m->e[i].k && m->e[i].k != k)
        i = (i + 1) & (m->cap - 1);
    return i;
}

static void *map_get(const Map *m, const void *k)
{
    return m->cap ? m->e[map_slot(m, k)].v : NULL;
}

static void map_put(Map *m, const void *k, void *v)
{
    if ((m->n + 1) * 2 > m->cap) {
        Map b = {calloc(m->cap ? m->cap * 2 : 64, sizeof(MapE)), m->cap ? m->cap * 2 : 64, 0};
        if (!b.e)
            abort();
        for (size_t i = 0; i < m->cap; i++)
            if (m->e[i].k)
                map_put(&b, m->e[i].k, m->e[i].v);
        free(m->e);
        *m = b;
    }
    size_t i = map_slot(m, k);
    if (!m->e[i].k)
        m->n++;
    m->e[i].k = k;
    m->e[i].v = v;
}

/* ---- writing ---- */

typedef struct {
    JGraph g;
    const World *w;
    JObj *world;
    Map orgs, codes, genes;
    JObj *colors[COL_COUNT];
} Out;

static JVal vi(int64_t i)
{
    return (JVal){.i = i};
}

static JVal vd(double d)
{
    return (JVal){.d = d};
}

static JVal vo(JObj *o)
{
    return (JVal){.o = o};
}

static JObj *w_color(Out *o, int c)
{
    if (!o->colors[c]) {
        JObj *x = jobj_new(&o->g, &c_color);
        jobj_set(x, "value", vi((int32_t)k_rgb[c]));
        o->colors[c] = x;
    }
    return o->colors[c];
}

static JObj *w_rect(Out *o, const JRect *r)
{
    JObj *x = jobj_new(&o->g, &c_rect);
    jobj_set(x, "x", vi(r->x));
    jobj_set(x, "y", vi(r->y));
    jobj_set(x, "width", vi(r->width));
    jobj_set(x, "height", vi(r->height));
    return x;
}

static JObj *w_ints(Out *o, const int *v, int n)
{
    JObj *a = jarray_new(&o->g, &c_ints, n);
    for (int i = 0; i < n; i++)
        a->vals[i].i = v[i];
    return a;
}

static JObj *w_doubles(Out *o, const double *v, int n)
{
    JObj *a = jarray_new(&o->g, &c_doubles, n);
    for (int i = 0; i < n; i++)
        a->vals[i].d = v[i];
    return a;
}

static JObj *w_list(Out *o, JObj **items, int n)
{
    JObj *l = jobj_new(&o->g, &c_arraylist);
    jobj_set(l, "size", vi(n));
    uint8_t b[4] = {(uint8_t)(n >> 24), (uint8_t)(n >> 16), (uint8_t)(n >> 8), (uint8_t)n};
    jobj_add_bytes(l, &c_arraylist, b, 4);
    for (int i = 0; i < n; i++)
        jobj_add_object(l, &c_arraylist, items[i]);
    return l;
}

static JObj *w_code(Out *o, const GeneticCode *gc)
{
    if (!gc)
        return NULL;
    JObj *x = map_get(&o->codes, gc);
    if (x)
        return x;
    x = jobj_new(&o->g, &c_code);
    map_put(&o->codes, gc, x);
    jobj_set(x, "_disperseChildren", vi(gc->disperse_children));
    jobj_set(x, "_max_age", vi(gc->max_age));
    jobj_set(x, "_mirror", vi(gc->mirror));
    jobj_set(x, "_reproduceEnergy", vi(gc->reproduce_energy));
    jobj_set(x, "_symmetry", vi(gc->symmetry));
    JObj *genes = jarray_new(&o->g, &c_genes, gc->ngenes);
    for (int i = 0; i < gc->ngenes; i++) {
        const Gene *g = &gc->genes[i];
        const void *key = (const void *)(uintptr_t)g->id;
        JObj *ge = g->id ? map_get(&o->genes, key) : NULL;
        if (!ge) {
            ge = jobj_new(&o->g, &c_gene);
            jobj_set(ge, "_length", vd(g->length));
            jobj_set(ge, "_theta", vd(g->theta));
            jobj_set(ge, "_color", vo(w_color(o, g->color)));
            if (g->id)
                map_put(&o->genes, key, ge);
        }
        genes->vals[i].o = ge;
    }
    jobj_set(x, "_genes", vo(genes));
    return x;
}

static JObj *w_org(Out *o, const Organism *b)
{
    JObj *x = map_get(&o->orgs, b);
    if (x)
        return x;
    x = jobj_new(&o->g, &c_org);
    map_put(&o->orgs, b, x);
    int n = b->segments;
    jobj_set(x, "x", vi(b->r.x));
    jobj_set(x, "y", vi(b->r.y));
    jobj_set(x, "width", vi(b->r.width));
    jobj_set(x, "height", vi(b->r.height));
    jobj_set(x, "_I", vd(b->inertia));
    jobj_set(x, "_ID", vi(b->id));
    jobj_set(x, "_age", vi(b->age));
    jobj_set(x, "_centerX", vi(b->center_x));
    jobj_set(x, "_centerY", vi(b->center_y));
    jobj_set(x, "_dCenterX", vd(b->dcenter_x));
    jobj_set(x, "_dCenterY", vd(b->dcenter_y));
    jobj_set(x, "_energy", vd(b->energy));
    jobj_set(x, "_framesColor", vi(b->frames_color));
    jobj_set(x, "_generation", vi(b->generation));
    jobj_set(x, "_growthRatio", vi(b->growth_ratio));
    jobj_set(x, "_infectedGeneration", vi(b->infected_generation));
    jobj_set(x, "_lastTheta", vd(b->last_theta));
    jobj_set(x, "_mass", vd(b->mass));
    jobj_set(x, "_nChildren", vi(b->n_children));
    jobj_set(x, "_nTotalChildren", vi(b->total_children));
    jobj_set(x, "_nTotalInfected", vi(b->total_infected));
    jobj_set(x, "_nTotalKills", vi(b->total_kills));
    jobj_set(x, "_parentID", vi(b->parent_id));
    jobj_set(x, "_segments", vi(n));
    jobj_set(x, "_theta", vd(b->theta));
    jobj_set(x, "_timeToReproduce", vi(b->time_to_reproduce));
    jobj_set(x, "alive", vi(b->alive));
    jobj_set(x, "dtheta", vd(b->dtheta));
    jobj_set(x, "dx", vd(b->dx));
    jobj_set(x, "dy", vd(b->dy));
    jobj_set(x, "hasGrown", vi(b->has_grown));
    jobj_set(x, "hasMoved", vi(b->has_moved));
    jobj_set(x, "_color",
             vo(b->flash_color < COL_COUNT ? w_color(o, b->flash_color) : NULL));
    jobj_set(x, "_endPointX", vo(w_ints(o, b->end_x, n)));
    jobj_set(x, "_endPointY", vo(w_ints(o, b->end_y, n)));
    jobj_set(x, "_geneticCode", vo(w_code(o, b->gc)));
    jobj_set(x, "_infectedGeneticCode", vo(w_code(o, b->infected_gc)));
    jobj_set(x, "_m", vo(w_doubles(o, b->m, n)));
    jobj_set(x, "_m1", vo(w_doubles(o, b->m1, n)));
    jobj_set(x, "_m2", vo(w_doubles(o, b->m2, n)));
    JObj *cols = jarray_new(&o->g, &c_colors, n);
    for (int i = 0; i < n; i++)
        cols->vals[i].o = w_color(o, b->seg_color[i]);
    jobj_set(x, "_segColor", vo(cols));
    jobj_set(x, "_sizeRect", vo(w_rect(o, &b->size_rect)));
    jobj_set(x, "_startPointX", vo(w_ints(o, b->start_x, n)));
    jobj_set(x, "_startPointY", vo(w_ints(o, b->start_y, n)));
    jobj_set(x, "_world", vo(o->world));
    jobj_set(x, "lastFrame", vo(w_rect(o, &b->last_frame)));
    jobj_set(x, "x1", vo(w_ints(o, b->x1, n)));
    jobj_set(x, "x2", vo(w_ints(o, b->x2, n)));
    jobj_set(x, "y1", vo(w_ints(o, b->y1, n)));
    jobj_set(x, "y2", vo(w_ints(o, b->y2, n)));
    return x;
}

static JObj *w_env(Out *o, const double *e)
{
    JObj *x = jobj_new(&o->g, &c_env);
    jobj_set(x, "minx", vd(e[0]));
    jobj_set(x, "maxx", vd(e[1]));
    jobj_set(x, "miny", vd(e[2]));
    jobj_set(x, "maxy", vd(e[3]));
    return x;
}

static JObj *w_item(Out *o, int ref)
{
    const Organism *b = o->w->snapshot[ref];
    JObj *x = jobj_new(&o->g, &c_item);
    jobj_set(x, "bounds", vo(w_env(o, b->env)));
    jobj_set(x, "item", vo(w_org(o, b)));
    return x;
}

static JObj *w_node(Out *o, int node)
{
    const StrNode *nd = &o->w->str_nodes[node];
    JObj *x = jobj_new(&o->g, &c_node);
    jobj_set(x, "level", vi(nd->level));
    jobj_set(x, "bounds", vo(nd->has_bounds ? w_env(o, nd->env) : NULL));
    JObj **kids = malloc((size_t)(nd->nkids ? nd->nkids : 1) * sizeof *kids);
    if (!kids)
        abort();
    for (int k = 0; k < nd->nkids; k++) {
        int ref = o->w->str_kids[nd->kid0 + k];
        kids[k] = ref >= 0 ? w_item(o, ref) : w_node(o, ~ref);
    }
    jobj_set(x, "childBoundables", vo(w_list(o, kids, nd->nkids)));
    free(kids);
    return x;
}

/* World.colDetTree: as JTS leaves it after the last frame */
static JObj *w_tree(Out *o)
{
    const World *w = o->w;
    JObj *t = jobj_new(&o->g, &c_tree);
    jobj_set(t, "nodeCapacity", vi(10));
    if (!w->str_built) {
        /* never queried, so never built: the items as inserted */
        int n = w->snapshot_n;
        JObj **items = malloc((size_t)(n ? n : 1) * sizeof *items);
        if (!items)
            abort();
        for (int i = 0; i < n; i++)
            items[i] = w_item(o, i);
        jobj_set(t, "itemBoundables", vo(w_list(o, items, n)));
        free(items);
        return t;
    }
    jobj_set(t, "built", vi(1));
    if (w->str_root) {
        jobj_set(t, "root", vo(w_node(o, ~w->str_root)));
    } else {
        JObj *root = jobj_new(&o->g, &c_node); /* createNode(0) */
        jobj_set(root, "childBoundables", vo(w_list(o, NULL, 0)));
        jobj_set(t, "root", vo(root));
    }
    return t;
}

static JObj *w_series(Out *o, const StatsSeries *s)
{
    JObj *items[STATS_HISTORY];
    for (int i = 0; i < s->n; i++) {
        items[i] = jobj_new(&o->g, &c_double);
        jobj_set(items[i], "value", vd(s->v[i]));
    }
    return w_list(o, items, s->n);
}

static JObj *w_stats(Out *o)
{
    const WorldStats *s = &o->w->stats;
    JObj *x = jobj_new(&o->g, &c_stats);
    jobj_set(x, "aliveBeingMostChildrenNumber", vi(s->alive_most_children_n));
    jobj_set(x, "aliveBeingMostInfectionsNumber", vi(s->alive_most_infections_n));
    jobj_set(x, "aliveBeingMostKillsNumber", vi(s->alive_most_kills_n));
    jobj_set(x, "beingMostChildrenNumber", vi(s->most_children_n));
    jobj_set(x, "beingMostChildrenTime", vi(s->most_children_time));
    jobj_set(x, "beingMostInfectionsNumber", vi(s->most_infections_n));
    jobj_set(x, "beingMostInfectionsTime", vi(s->most_infections_time));
    jobj_set(x, "beingMostKillsNumber", vi(s->most_kills_n));
    jobj_set(x, "beingMostKillsTime", vi(s->most_kills_time));
    jobj_set(x, "birthLastTime", vi(s->birth_last_time));
    jobj_set(x, "birthSum", vi(s->birth_sum));
    jobj_set(x, "createdOrganisms", vi(s->created_organisms));
    jobj_set(x, "deathLastTime", vi(s->death_last_time));
    jobj_set(x, "deathSum", vi(s->death_sum));
    jobj_set(x, "infectionsSum", vi(s->infections_sum));
    jobj_set(x, "massExtintionState", vi(s->mass_extinction_state));
    jobj_set(x, "massExtintions", vi(s->mass_extinctions));
    jobj_set(x, "maxBirths", vi(s->max_births));
    jobj_set(x, "maxCarbonDioxide", vd(s->max_co2));
    jobj_set(x, "maxCarbonDioxideTime", vi(s->max_co2_time));
    jobj_set(x, "maxDeaths", vi(s->max_deaths));
    jobj_set(x, "maxOxygen", vd(s->max_oxygen));
    jobj_set(x, "maxOxygenTime", vi(s->max_oxygen_time));
    jobj_set(x, "maxPopulation", vi(s->max_population));
    jobj_set(x, "maxPopulationTime", vi(s->max_population_time));
    jobj_set(x, "minCarbonDioxide", vd(s->min_co2));
    jobj_set(x, "minCarbonDioxideTime", vi(s->min_co2_time));
    jobj_set(x, "minOxygen", vd(s->min_oxygen));
    jobj_set(x, "minOxygenTime", vi(s->min_oxygen_time));
    jobj_set(x, "minPopulation", vi(s->min_population));
    jobj_set(x, "minPopulationTime", vi(s->min_population_time));
    jobj_set(x, "populationSum", vi(s->population_sum));
    jobj_set(x, "time", vi(s->time));
    jobj_set(x, "aliveBeingMostChildren", vo(w_code(o, s->alive_most_children)));
    jobj_set(x, "aliveBeingMostInfections", vo(w_code(o, s->alive_most_infections)));
    jobj_set(x, "aliveBeingMostKills", vo(w_code(o, s->alive_most_kills)));
    jobj_set(x, "beingMostChildren", vo(w_code(o, s->most_children)));
    jobj_set(x, "beingMostInfections", vo(w_code(o, s->most_infections)));
    jobj_set(x, "beingMostKills", vo(w_code(o, s->most_kills)));
    jobj_set(x, "birthList", vo(w_series(o, &s->births)));
    jobj_set(x, "carbonDioxideList", vo(w_series(o, &s->co2)));
    jobj_set(x, "deathList", vo(w_series(o, &s->deaths)));
    jobj_set(x, "lastBornBeing", vo(w_code(o, s->last_born)));
    jobj_set(x, "lastDeadBeing", vo(w_code(o, s->last_dead)));
    jobj_set(x, "lastInfectedBeing", vo(w_code(o, s->last_infected)));
    jobj_set(x, "oxygenList", vo(w_series(o, &s->oxygen)));
    jobj_set(x, "populationList", vo(w_series(o, &s->population)));
    return x;
}

bool bgw_save(const World *w, const char *path, char *err, size_t errn)
{
    Out o = {.w = w};
    o.world = jobj_new(&o.g, &c_world);
    JObj *x = o.world;
    jobj_set(x, "NEXT_ID", vi(w->next_id));
    jobj_set(x, "_CO2", vd(w->co2));
    jobj_set(x, "_O2", vd(w->o2));
    jobj_set(x, "_height", vi(w->height));
    jobj_set(x, "_population", vi(w->population));
    jobj_set(x, "_width", vi(w->width));
    jobj_set(x, "nFrames", vi(w->nframes));
    /* Collections.synchronizedList(new ArrayList) */
    JObj **orgs = malloc((size_t)(w->norgs ? w->norgs : 1) * sizeof *orgs);
    if (!orgs)
        abort();
    for (int i = 0; i < w->norgs; i++)
        orgs[i] = w_org(&o, w->orgs[i]);
    JObj *list = w_list(&o, orgs, w->norgs);
    free(orgs);
    JObj *sync = jobj_new(&o.g, &c_sync_list);
    jobj_set(sync, "c", vo(list));
    jobj_set(sync, "mutex", vo(sync));
    jobj_set(sync, "list", vo(list));
    jobj_set(x, "_organisms", vo(sync));
    jobj_set(x, "colDetTree", vo(w_tree(&o)));
    jobj_set(x, "worldStatistics", vo(w_stats(&o)));

    size_t len;
    uint8_t *bytes = jser_write(x, &len);
    jgraph_free(&o.g);
    free(o.orgs.e);
    free(o.codes.e);
    free(o.genes.e);
    FILE *f = fopen(path, "wb");
    bool ok = f && fwrite(bytes, 1, len, f) == len;
    if (f && fclose(f) != 0)
        ok = false;
    free(bytes);
    if (!ok)
        set_err(err, errn, "Can't write the file");
    return ok;
}

/* ---- reading ---- */

typedef struct {
    World *w;
    Map orgs;  /* JObj -> Organism */
    Map codes; /* JObj -> GeneticCode */
    Map genes; /* JObj -> gene id */
    Organism **made; /* every organism read, to find the ones only the tree holds */
    int nmade, made_cap;
    char *err;
    size_t errn;
    bool bad;
} In;

static bool bad(In *in, const char *fmt, ...)
{
    if (!in->bad && in->err && in->errn) {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(in->err, in->errn, fmt, ap);
        va_end(ap);
    }
    in->bad = true;
    return false;
}

static int64_t r_i(In *in, const JObj *o, const char *name, char type)
{
    JVal *v = jobj_field(o, name, type);
    if (!v) {
        bad(in, "This world file has no %s.%s.", o ? o->cls->name : "?", name);
        return 0;
    }
    return v->i;
}

static double r_d(In *in, const JObj *o, const char *name)
{
    JVal *v = jobj_field(o, name, 'D');
    if (!v) {
        bad(in, "This world file has no %s.%s.", o ? o->cls->name : "?", name);
        return 0;
    }
    return v->d;
}

static JObj *r_o(In *in, const JObj *o, const char *name)
{
    JVal *v = jobj_field(o, name, 'L');
    if (!v) {
        bad(in, "This world file has no %s.%s.", o ? o->cls->name : "?", name);
        return NULL;
    }
    return v->o;
}

/* the objects of a java.util.List (synchronized or not) */
static JObj **r_list(In *in, const JObj *l, int *n)
{
    *n = 0;
    if (!l)
        return NULL;
    if (jobj_is(l, "java.util.Collections$SynchronizedList") ||
        jobj_is(l, "java.util.Collections$SynchronizedRandomAccessList")) {
        JVal *inner = jobj_field(l, "list", 'L');
        if (!inner)
            inner = jobj_field(l, "c", 'L');
        return inner && inner->o != l ? r_list(in, inner->o, n) : NULL;
    }
    if (!jobj_is(l, "java.util.ArrayList")) {
        bad(in, "Unsupported list class %s.", l->kind == JK_STRING ? "string" : l->cls->name);
        return NULL;
    }
    int na;
    const JAnn *a = jobj_ann(l, "java.util.ArrayList", &na);
    JObj **items = malloc((size_t)(na ? na : 1) * sizeof *items);
    if (!items)
        abort();
    for (int i = 0; i < na; i++)
        if (a[i].is_object)
            items[(*n)++] = a[i].obj;
    if (*n != (int)r_i(in, l, "size", 'I')) {
        free(items);
        *n = 0;
        bad(in, "A list in the file is inconsistent.");
        return NULL;
    }
    return items;
}

static int r_color(In *in, const JObj *c)
{
    if (!c || !jobj_is(c, "java.awt.Color"))
        return bad(in, "A colour in the file is missing."), COL_GREEN;
    uint32_t v = (uint32_t)r_i(in, c, "value", 'I');
    for (int i = 0; i < COL_COUNT; i++)
        if (v == k_rgb[i] || (k_rgb_dark[i] && v == k_rgb_dark[i]))
            return i;
    bad(in, "The file uses a colour (%08x) Biogenesis 0.9 doesn't have.", v);
    return COL_GREEN;
}

static GeneticCode *r_code(In *in, const JObj *x)
{
    if (!x)
        return NULL;
    GeneticCode *gc = map_get(&in->codes, x);
    if (gc)
        return gc;
    if (!jobj_is(x, "biogenesis.GeneticCode"))
        return bad(in, "A genetic code in the file isn't one."), NULL;
    const JObj *ga = r_o(in, x, "_genes");
    int sym = (int)r_i(in, x, "_symmetry", 'I');
    if (in->bad || !ga || ga->kind != JK_ARRAY || ga->nvals < 1 || sym < 1 || sym > 8)
        return bad(in, "A genetic code in the file is damaged."), NULL;
    Gene *genes = malloc((size_t)ga->nvals * sizeof *genes);
    if (!genes)
        abort();
    for (int i = 0; i < ga->nvals; i++) {
        const JObj *g = ga->vals[i].o;
        if (!jobj_is(g, "biogenesis.Gene")) {
            free(genes);
            return bad(in, "A gene in the file is damaged."), NULL;
        }
        genes[i].length = r_d(in, g, "_length");
        genes[i].theta = r_d(in, g, "_theta");
        genes[i].color = (uint8_t)r_color(in, r_o(in, g, "_color"));
        if (genes[i].color >= GENE_COLORS)
            bad(in, "A gene in the file has a dead organism's colour.");
    }
    gc = gc_new_from_genes(genes, ga->nvals, sym, (int)r_i(in, x, "_mirror", 'I'),
                           r_i(in, x, "_disperseChildren", 'Z') != 0);
    free(genes);
    gc->reproduce_energy = (int)r_i(in, x, "_reproduceEnergy", 'I');
    gc->max_age = (int)r_i(in, x, "_max_age", 'I');
    /* a gene object shared between codes keeps one id */
    for (int i = 0; i < ga->nvals; i++) {
        uint32_t id = (uint32_t)(uintptr_t)map_get(&in->genes, ga->vals[i].o);
        if (!id) {
            id = gene_new_id();
            map_put(&in->genes, ga->vals[i].o, (void *)(uintptr_t)id);
        }
        gc->genes[i].id = id;
    }
    map_put(&in->codes, x, gc); /* owned by the map until the end */
    return gc;
}

static void r_rect(In *in, const JObj *x, JRect *r)
{
    if (!x)
        return; /* never null in 0.9; an empty rectangle */
    if (!jobj_is(x, "java.awt.Rectangle")) {
        bad(in, "A rectangle in the file isn't one.");
        return;
    }
    r->x = (int)r_i(in, x, "x", 'I');
    r->y = (int)r_i(in, x, "y", 'I');
    r->width = (int)r_i(in, x, "width", 'I');
    r->height = (int)r_i(in, x, "height", 'I');
}

static bool r_ints(In *in, const JObj *x, const char *name, int *out, int n)
{
    const JObj *a = r_o(in, x, name);
    if (!a || a->kind != JK_ARRAY || strcmp(a->cls->name, "[I") != 0 || a->nvals != n)
        return bad(in, "An organism's %s doesn't match its segments.", name);
    for (int i = 0; i < n; i++)
        out[i] = (int)a->vals[i].i;
    return true;
}

static bool r_doubles(In *in, const JObj *x, const char *name, double *out, int n)
{
    const JObj *a = r_o(in, x, name);
    if (!a || a->kind != JK_ARRAY || strcmp(a->cls->name, "[D") != 0 || a->nvals != n)
        return bad(in, "An organism's %s doesn't match its segments.", name);
    for (int i = 0; i < n; i++)
        out[i] = a->vals[i].d;
    return true;
}

static Organism *r_org(In *in, const JObj *x)
{
    Organism *o = map_get(&in->orgs, x);
    if (o || in->bad)
        return o;
    if (!jobj_is(x, "biogenesis.Organism"))
        return bad(in, "An organism in the file isn't one."), NULL;
    GeneticCode *gc = r_code(in, r_o(in, x, "_geneticCode"));
    if (!gc)
        return bad(in, "An organism in the file has no genetic code."), NULL;
    o = calloc(1, sizeof *o);
    if (!o)
        abort();
    map_put(&in->orgs, x, o);
    if (in->nmade == in->made_cap) {
        in->made_cap = in->made_cap ? in->made_cap * 2 : 64;
        in->made = realloc(in->made, (size_t)in->made_cap * sizeof *in->made);
        if (!in->made)
            abort();
    }
    in->made[in->nmade++] = o;
    o->world = in->w;
    o->gc = gc_ref(gc);
    o->infected_gc = gc_ref(r_code(in, r_o(in, x, "_infectedGeneticCode")));
    /* an Organism is a java.awt.Rectangle */
    o->r.x = (int)r_i(in, x, "x", 'I');
    o->r.y = (int)r_i(in, x, "y", 'I');
    o->r.width = (int)r_i(in, x, "width", 'I');
    o->r.height = (int)r_i(in, x, "height", 'I');
    int n = (int)r_i(in, x, "_segments", 'I');
    if (n != gc->ngenes * gc->symmetry)
        return bad(in, "An organism's segments don't match its genetic code."), o;
    org_create(o);
    o->n_children = (int)r_i(in, x, "_nChildren", 'I');
    o->parent_id = (int)r_i(in, x, "_parentID", 'I');
    o->id = (int)r_i(in, x, "_ID", 'I');
    o->generation = (int)r_i(in, x, "_generation", 'I');
    o->infected_generation = (int)r_i(in, x, "_infectedGeneration", 'I');
    o->total_children = (int)r_i(in, x, "_nTotalChildren", 'I');
    o->total_kills = (int)r_i(in, x, "_nTotalKills", 'I');
    o->total_infected = (int)r_i(in, x, "_nTotalInfected", 'I');
    r_ints(in, x, "_startPointX", o->start_x, n);
    r_ints(in, x, "_startPointY", o->start_y, n);
    r_ints(in, x, "_endPointX", o->end_x, n);
    r_ints(in, x, "_endPointY", o->end_y, n);
    r_doubles(in, x, "_m1", o->m1, n);
    r_doubles(in, x, "_m2", o->m2, n);
    r_doubles(in, x, "_m", o->m, n);
    r_ints(in, x, "x1", o->x1, n);
    r_ints(in, x, "y1", o->y1, n);
    r_ints(in, x, "x2", o->x2, n);
    r_ints(in, x, "y2", o->y2, n);
    const JObj *cols = r_o(in, x, "_segColor");
    if (!cols || cols->kind != JK_ARRAY || cols->nvals != n)
        return bad(in, "An organism's colours don't match its segments."), o;
    for (int i = 0; i < n; i++)
        o->seg_color[i] = (uint8_t)r_color(in, cols->vals[i].o);
    o->center_x = (int)r_i(in, x, "_centerX", 'I');
    o->center_y = (int)r_i(in, x, "_centerY", 'I');
    o->dcenter_x = r_d(in, x, "_dCenterX");
    o->dcenter_y = r_d(in, x, "_dCenterY");
    o->growth_ratio = (int)r_i(in, x, "_growthRatio", 'I');
    if (o->growth_ratio < 1 || o->growth_ratio > 16)
        bad(in, "An organism in the file has an impossible size.");
    o->mass = r_d(in, x, "_mass");
    o->inertia = r_d(in, x, "_I");
    o->energy = r_d(in, x, "_energy");
    r_rect(in, r_o(in, x, "_sizeRect"), &o->size_rect);
    o->theta = r_d(in, x, "_theta");
    o->last_theta = r_d(in, x, "_lastTheta");
    o->dx = r_d(in, x, "dx");
    o->dy = r_d(in, x, "dy");
    o->dtheta = r_d(in, x, "dtheta");
    o->age = (int)r_i(in, x, "_age", 'I');
    const JObj *flash = r_o(in, x, "_color");
    o->flash_color = flash ? (uint8_t)r_color(in, flash) : FLASH_NONE;
    o->frames_color = (int)r_i(in, x, "_framesColor", 'I');
    o->time_to_reproduce = (int)r_i(in, x, "_timeToReproduce", 'I');
    o->has_grown = (int)r_i(in, x, "hasGrown", 'I');
    o->has_moved = r_i(in, x, "hasMoved", 'Z') != 0;
    o->alive = r_i(in, x, "alive", 'Z') != 0;
    r_rect(in, r_o(in, x, "lastFrame"), &o->last_frame);
    org_prepare_angles(o);
    return o;
}

static void r_env(In *in, const JObj *x, double *e)
{
    if (!jobj_is(x, "org.locationtech.jts.geom.Envelope")) {
        bad(in, "The collision tree in the file is damaged.");
        return;
    }
    e[0] = r_d(in, x, "minx");
    e[1] = r_d(in, x, "maxx");
    e[2] = r_d(in, x, "miny");
    e[3] = r_d(in, x, "maxy");
}

static int r_item(In *in, const JObj *x)
{
    World *w = in->w;
    Organism *o = jobj_is(x, "org.locationtech.jts.index.strtree.ItemBoundable")
                      ? r_org(in, r_o(in, x, "item"))
                      : NULL;
    if (!o)
        return bad(in, "The collision tree in the file is damaged."), 0;
    r_env(in, r_o(in, x, "bounds"), o->env);
    if (w->snapshot_n == w->snapshot_cap) {
        w->snapshot_cap = w->snapshot_cap ? w->snapshot_cap * 2 : 64;
        w->snapshot = realloc(w->snapshot, (size_t)w->snapshot_cap * sizeof *w->snapshot);
        if (!w->snapshot)
            abort();
    }
    w->snapshot[w->snapshot_n] = o;
    return w->snapshot_n++;
}

static int r_node(In *in, const JObj *x, int depth)
{
    World *w = in->w;
    if (depth > 64 || !jobj_is(x, "org.locationtech.jts.index.strtree.STRtree$STRtreeNode"))
        return bad(in, "The collision tree in the file is damaged."), 0;
    if (w->str_nnodes == w->str_nodes_cap) {
        w->str_nodes_cap = w->str_nodes_cap ? w->str_nodes_cap * 2 : 64;
        w->str_nodes = realloc(w->str_nodes, (size_t)w->str_nodes_cap * sizeof *w->str_nodes);
        if (!w->str_nodes)
            abort();
    }
    int idx = w->str_nnodes++;
    StrNode nd = {.level = (int)r_i(in, x, "level", 'I')};
    const JObj *b = r_o(in, x, "bounds");
    nd.has_bounds = b != NULL;
    if (b)
        r_env(in, b, nd.env);
    int nk;
    JObj **kids = r_list(in, r_o(in, x, "childBoundables"), &nk);
    nd.kid0 = w->str_nkids;
    nd.nkids = nk;
    if (w->str_nkids + nk > w->str_kids_cap) {
        while (w->str_nkids + nk > w->str_kids_cap)
            w->str_kids_cap = w->str_kids_cap ? w->str_kids_cap * 2 : 64;
        w->str_kids = realloc(w->str_kids, (size_t)w->str_kids_cap * sizeof *w->str_kids);
        if (!w->str_kids)
            abort();
    }
    w->str_nkids += nk;
    w->str_nodes[idx] = nd;
    for (int k = 0; k < nk && !in->bad; k++) {
        int ref = jobj_is(kids[k], "org.locationtech.jts.index.strtree.ItemBoundable")
                      ? r_item(in, kids[k])
                      : ~r_node(in, kids[k], depth + 1);
        w->str_kids[nd.kid0 + k] = ref;
    }
    free(kids);
    return idx;
}

static void r_tree(In *in, const JObj *t)
{
    World *w = in->w;
    if (!t)
        return; /* an empty tree */
    if (!jobj_is(t, "org.locationtech.jts.index.strtree.STRtree")) {
        bad(in, "The collision tree in the file is damaged.");
        return;
    }
    w->str_built = r_i(in, t, "built", 'Z') != 0;
    const JObj *root = r_o(in, t, "root");
    if (w->str_built && root) {
        w->str_root = ~r_node(in, root, 0);
        return;
    }
    int n;
    JObj **items = r_list(in, r_o(in, t, "itemBoundables"), &n);
    for (int i = 0; i < n && !in->bad; i++)
        r_item(in, items[i]);
    free(items);
}

static void r_series(In *in, const JObj *l, StatsSeries *s)
{
    int n;
    JObj **items = r_list(in, l, &n);
    s->n = 0;
    for (int i = (n > STATS_HISTORY ? n - STATS_HISTORY : 0); i < n; i++) {
        if (!jobj_is(items[i], "java.lang.Double")) {
            bad(in, "The statistics in the file are damaged.");
            break;
        }
        s->v[s->n++] = r_d(in, items[i], "value");
    }
    free(items);
}

static void r_stats(In *in, const JObj *x)
{
    WorldStats *s = &in->w->stats;
    if (!jobj_is(x, "biogenesis.WorldStatistics")) {
        bad(in, "The file has no statistics.");
        return;
    }
    s->alive_most_children_n = (int)r_i(in, x, "aliveBeingMostChildrenNumber", 'I');
    s->alive_most_infections_n = (int)r_i(in, x, "aliveBeingMostInfectionsNumber", 'I');
    s->alive_most_kills_n = (int)r_i(in, x, "aliveBeingMostKillsNumber", 'I');
    s->most_children_n = (int)r_i(in, x, "beingMostChildrenNumber", 'I');
    s->most_children_time = r_i(in, x, "beingMostChildrenTime", 'J');
    s->most_infections_n = (int)r_i(in, x, "beingMostInfectionsNumber", 'I');
    s->most_infections_time = r_i(in, x, "beingMostInfectionsTime", 'J');
    s->most_kills_n = (int)r_i(in, x, "beingMostKillsNumber", 'I');
    s->most_kills_time = r_i(in, x, "beingMostKillsTime", 'J');
    s->birth_last_time = (int)r_i(in, x, "birthLastTime", 'I');
    s->birth_sum = r_i(in, x, "birthSum", 'J');
    s->created_organisms = (int)r_i(in, x, "createdOrganisms", 'I');
    s->death_last_time = (int)r_i(in, x, "deathLastTime", 'I');
    s->death_sum = r_i(in, x, "deathSum", 'J');
    s->infections_sum = r_i(in, x, "infectionsSum", 'J');
    s->mass_extinction_state = (int)r_i(in, x, "massExtintionState", 'I');
    s->mass_extinctions = (int)r_i(in, x, "massExtintions", 'I');
    s->max_births = (int)r_i(in, x, "maxBirths", 'I');
    s->max_co2 = r_d(in, x, "maxCarbonDioxide");
    s->max_co2_time = r_i(in, x, "maxCarbonDioxideTime", 'J');
    s->max_deaths = (int)r_i(in, x, "maxDeaths", 'I');
    s->max_oxygen = r_d(in, x, "maxOxygen");
    s->max_oxygen_time = r_i(in, x, "maxOxygenTime", 'J');
    s->max_population = (int)r_i(in, x, "maxPopulation", 'I');
    s->max_population_time = r_i(in, x, "maxPopulationTime", 'J');
    s->min_co2 = r_d(in, x, "minCarbonDioxide");
    s->min_co2_time = r_i(in, x, "minCarbonDioxideTime", 'J');
    s->min_oxygen = r_d(in, x, "minOxygen");
    s->min_oxygen_time = r_i(in, x, "minOxygenTime", 'J');
    s->min_population = (int)r_i(in, x, "minPopulation", 'I');
    s->min_population_time = r_i(in, x, "minPopulationTime", 'J');
    s->population_sum = r_i(in, x, "populationSum", 'J');
    s->time = r_i(in, x, "time", 'J');
    struct {
        const char *name;
        GeneticCode **slot;
    } codes[] = {
        {"aliveBeingMostChildren", &s->alive_most_children},
        {"aliveBeingMostInfections", &s->alive_most_infections},
        {"aliveBeingMostKills", &s->alive_most_kills},
        {"beingMostChildren", &s->most_children},
        {"beingMostInfections", &s->most_infections},
        {"beingMostKills", &s->most_kills},
        {"lastBornBeing", &s->last_born},
        {"lastDeadBeing", &s->last_dead},
        {"lastInfectedBeing", &s->last_infected},
    };
    for (size_t i = 0; i < sizeof codes / sizeof codes[0]; i++)
        gc_assign(codes[i].slot, r_code(in, r_o(in, x, codes[i].name)));
    r_series(in, r_o(in, x, "populationList"), &s->population);
    r_series(in, r_o(in, x, "deathList"), &s->deaths);
    r_series(in, r_o(in, x, "birthList"), &s->births);
    r_series(in, r_o(in, x, "oxygenList"), &s->oxygen);
    r_series(in, r_o(in, x, "carbonDioxideList"), &s->co2);
}

bool bgw_sniff(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    unsigned char b[2] = {0};
    bool yes = fread(b, 1, 2, f) == 2 && b[0] == 0xac && b[1] == 0xed;
    fclose(f);
    return yes;
}

bool bgw_load(World *dst, const char *path, char *err, size_t errn)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        set_err(err, errn, "This file can't be read");
        return false;
    }
    size_t cap = 1 << 16, len = 0, got;
    uint8_t *data = malloc(cap);
    if (!data)
        abort();
    while ((got = fread(data + len, 1, cap - len, f)) > 0) {
        len += got;
        if (len == cap) {
            if (cap >= ((size_t)1 << 30))
                break;
            cap *= 2;
            data = realloc(data, cap);
            if (!data)
                abort();
        }
    }
    fclose(f);

    JGraph g = {0};
    JObj *root = jser_read(&g, data, len, err, errn);
    free(data);
    if (!root) {
        jgraph_free(&g);
        return false;
    }
    World w;
    world_init(&w);
    In in = {.w = &w, .err = err, .errn = errn};
    if (!jobj_is(root, "biogenesis.World")) {
        bad(&in, "This file doesn't contain a Biogenesis world.");
    } else {
        w.next_id = (int)r_i(&in, root, "NEXT_ID", 'I');
        w.co2 = r_d(&in, root, "_CO2");
        w.o2 = r_d(&in, root, "_O2");
        w.width = (int)r_i(&in, root, "_width", 'I');
        w.height = (int)r_i(&in, root, "_height", 'I');
        w.population = (int)r_i(&in, root, "_population", 'I');
        w.nframes = (int)r_i(&in, root, "nFrames", 'I');
        if (w.width <= 0 || w.height <= 0 || w.width > 1000000 || w.height > 1000000)
            bad(&in, "The world in the file has an impossible size.");
        int n;
        JObj **orgs = in.bad ? NULL : r_list(&in, r_o(&in, root, "_organisms"), &n);
        for (int i = 0; i < n && !in.bad; i++) {
            Organism *o = r_org(&in, orgs[i]);
            if (o)
                world_append_raw(&w, o);
        }
        free(orgs);
        if (!in.bad)
            r_tree(&in, r_o(&in, root, "colDetTree"));
        if (!in.bad)
            r_stats(&in, r_o(&in, root, "worldStatistics"));
    }
    /* organisms only the tree holds were removed in the last frame */
    for (int i = 0; i < in.nmade; i++) {
        Organism *o = in.made[i];
        bool listed = false;
        for (int k = 0; k < w.norgs && !listed; k++)
            listed = w.orgs[k] == o;
        if (listed)
            continue;
        o->removed = true;
        if (w.ngrave == w.grave_cap) {
            w.grave_cap = w.grave_cap ? w.grave_cap * 2 : 16;
            w.grave = realloc(w.grave, (size_t)w.grave_cap * sizeof *w.grave);
            if (!w.grave)
                abort();
        }
        w.grave[w.ngrave++] = o;
    }
    for (size_t i = 0; i < in.codes.cap; i++)
        if (in.codes.e[i].k)
            gc_unref(in.codes.e[i].v); /* organisms and stats hold their own */
    free(in.orgs.e);
    free(in.codes.e);
    free(in.genes.e);
    free(in.made);
    jgraph_free(&g);
    if (in.bad) {
        world_free(&w);
        return false;
    }
    void (*cb)(void *, Organism *) = dst->on_remove;
    void *ud = dst->ud;
    world_free(dst);
    *dst = w;
    dst->on_remove = cb;
    dst->ud = ud;
    for (int i = 0; i < dst->norgs; i++)
        dst->orgs[i]->world = dst;
    for (int i = 0; i < dst->ngrave; i++)
        dst->grave[i]->world = dst;
    return true;
}
