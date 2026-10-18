/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Translated to C in 2026 from World.java, WorldStatistics.java and
 * MainWindow.java (saveObject) of Biogenesis Color Mod 2.01
 * (https://sourceforge.net/projects/biogenesis/),
 * Copyright (C) 2006-2010 Joan Queralt Molina,
 * Color Mod Copyright (C) 2012-2022 MarcoDBAA.
 * Follows the 0.9 port's src/io/bgw.c.
 *
 * The collision tree's classes are JTS 1.15.0's
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

/* Color Mod worlds: the Java object graph of src/io/jser.h, with the class
 * descriptors Java 27 writes. Organism, GeneticCode and Gene, every field
 * of them, come from bgw_classes.h (generated from the same class model as
 * the translated simulation); the world, its statistics and its collision
 * tree are here. Shared objects stay shared: a genetic code or gene that
 * several organisms or codes hold is one object in the file, as in Java. */
#include "io/bgw.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io/jser.h"
#include "sim/params.h"

#define F(t, n) {t, n, NULL}
#define FO(t, n, s) {t, n, s}

/* ---- the classes, as Java 27 describes them ---- */

static const JField f_world[] = {
    F('I', "NEXT_ID"), F('D', "_CH4"), F('D', "_CO2"), F('D', "_O2"), F('Z', "_corridorexists"),
    F('I', "_height"), F('I', "_population"), F('I', "_width"), F('I', "nFrames"),
    FO('L', "_organisms", "Ljava/util/List;"),
    FO('L', "colDetTree", "Lorg/locationtech/jts/index/strtree/STRtree;"),
    FO('L', "worldStatistics", "Lbiogenesis/WorldStatistics;"),
};
static const JClass c_world = {"biogenesis.World", U_FILE_VERSION, 0x02, 12, f_world, NULL};

static const JField f_sync_coll[] = {
    FO('L', "c", "Ljava/util/Collection;"),
    FO('L', "mutex", "Ljava/lang/Object;"),
};
static const JClass c_sync_coll = {"java.util.Collections$SynchronizedCollection",
                                   3053995032091335093LL, 0x03, 2, f_sync_coll, NULL};
static const JField f_sync_list[] = {FO('L', "list", "Ljava/util/List;")};
static const JClass c_sync_list = {"java.util.Collections$SynchronizedList",
                                   -7754090372962971524LL, 0x03, 1, f_sync_list, &c_sync_coll};

static const JField f_arraylist[] = {F('I', "size")};
static const JClass c_arraylist = {"java.util.ArrayList", 8683452581122892189LL, 0x03, 1,
                                   f_arraylist, NULL};

static const JField f_rect[] = {F('I', "height"), F('I', "width"), F('I', "x"), F('I', "y")};
static const JClass c_rect = {"java.awt.Rectangle", -4345857070255674764LL, 0x02, 4, f_rect, NULL};

static const JClass c_ints = {"[I", 5600894804908749477LL, 0x02, 0, NULL, NULL};
static const JClass c_doubles = {"[D", 4514449696888150558LL, 0x02, 0, NULL, NULL};
static const JClass c_colors = {"[Ljava.awt.Color;", 4045880353309156309LL, 0x02, 0, NULL, NULL};
static const JClass c_genes = {"[Lbiogenesis.Gene;", -8710523010866530344LL, 0x02, 0, NULL, NULL};

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
    F('I', "maxDeaths"), F('D', "maxMethane"), F('J', "maxMethaneTime"), F('D', "maxOxygen"),
    F('J', "maxOxygenTime"), F('I', "maxPopulation"), F('J', "maxPopulationTime"),
    F('D', "minCarbonDioxide"), F('J', "minCarbonDioxideTime"), F('D', "minMethane"),
    F('J', "minMethaneTime"), F('D', "minOxygen"), F('J', "minOxygenTime"),
    F('I', "minPopulation"), F('J', "minPopulationTime"), F('J', "populationSum"), F('J', "time"),
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
    FO('L', "methaneList", "Ljava/util/List;"), FO('L', "oxygenList", "Ljava/util/List;"),
    FO('L', "populationList", "Ljava/util/List;"),
};
static const JClass c_stats = {"biogenesis.WorldStatistics", U_FILE_VERSION, 0x02, 52, f_stats,
                               NULL};

static const JClass c_number = {"java.lang.Number", -8742448824652078965LL, 0x02, 0, NULL, NULL};
static const JField f_double[] = {F('D', "value")};
static const JClass c_double = {"java.lang.Double", -9172774392245257468LL, 0x02, 1, f_double,
                                &c_number};

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
    Map orgs, codes, genes, strings;
    Map colors; /* by value: equal colours are one object */
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

/* A String, one object per C string as Java has one per String object.
 * jser.h makes strings only when reading, so this is graph_add's work done
 * here: jgraph_free frees it with the rest. */
static JObj *w_string(Out *o, const char *s)
{
    if (!s)
        return NULL;
    JObj *x = map_get(&o->strings, s);
    if (x)
        return x;
    x = calloc(1, sizeof *x);
    if (!x)
        abort();
    x->kind = JK_STRING;
    x->str = strdup(s); /* the graph's own copy, freed with it */
    x->handle = -1;
    if (o->g.nobjs == o->g.cap) {
        o->g.cap = o->g.cap ? o->g.cap * 2 : 256;
        o->g.objs = realloc(o->g.objs, (size_t)o->g.cap * sizeof *o->g.objs);
        if (!o->g.objs)
            abort();
    }
    o->g.objs[o->g.nobjs++] = x;
    map_put(&o->strings, s, x);
    return x;
}

/* new Color(r, g, b): value, and the rest null as java.awt.Color leaves it */
static JObj *w_color(Out *o, JColor c)
{
    if (!c)
        return NULL;
    const void *key = (const void *)(uintptr_t)c;
    JObj *x = map_get(&o->colors, key);
    if (!x) {
        x = jobj_new(&o->g, &c_color);
        jobj_set(x, "value", vi((int32_t)c));
        map_put(&o->colors, key, x);
    }
    return x;
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

static JObj *w_ints(Out *o, const int *v)
{
    if (!v)
        return NULL;
    JObj *a = jarray_new(&o->g, &c_ints, JLEN(v));
    for (int i = 0; i < JLEN(v); i++)
        a->vals[i].i = v[i];
    return a;
}

static JObj *w_doubles(Out *o, const double *v)
{
    if (!v)
        return NULL;
    JObj *a = jarray_new(&o->g, &c_doubles, JLEN(v));
    for (int i = 0; i < JLEN(v); i++)
        a->vals[i].d = v[i];
    return a;
}

static JObj *w_colors(Out *o, const JColor *v)
{
    if (!v)
        return NULL;
    JObj *a = jarray_new(&o->g, &c_colors, JLEN(v));
    for (int i = 0; i < JLEN(v); i++)
        a->vals[i].o = w_color(o, v[i]);
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

static JObj *w_gene(Out *o, const Gene *g);
static JObj *w_code(Out *o, const GeneticCode *gc);

static JObj *w_genes(Out *o, Gene *const *v)
{
    if (!v)
        return NULL;
    JObj *a = jarray_new(&o->g, &c_genes, JLEN(v));
    for (int i = 0; i < JLEN(v); i++)
        a->vals[i].o = w_gene(o, v[i]);
    return a;
}

/* ---- reading ---- */

typedef struct {
    World *w;
    Map orgs;  /* JObj -> Organism */
    Map codes; /* JObj -> GeneticCode */
    Map genes; /* JObj -> Gene */
    Map strings; /* JObj -> String */
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

static const char *class_of(const JObj *o)
{
    return !o ? "?" : o->kind == JK_STRING ? "java.lang.String" : o->cls->name;
}

static int64_t r_i(In *in, const JObj *o, const char *name, char type)
{
    JVal *v = o && o->kind == JK_OBJECT ? jobj_field(o, name, type) : NULL;
    if (!v) {
        bad(in, "This world file has no %s.%s.", class_of(o), name);
        return 0;
    }
    return v->i;
}

static double r_d(In *in, const JObj *o, const char *name)
{
    JVal *v = o && o->kind == JK_OBJECT ? jobj_field(o, name, 'D') : NULL;
    if (!v) {
        bad(in, "This world file has no %s.%s.", class_of(o), name);
        return 0;
    }
    return v->d;
}

/* an object or array field */
static JObj *r_field(In *in, const JObj *o, const char *name, char type)
{
    JVal *v = o && o->kind == JK_OBJECT ? jobj_field(o, name, type) : NULL;
    if (!v) {
        bad(in, "This world file has no %s.%s.", class_of(o), name);
        return NULL;
    }
    return v->o;
}

static JObj *r_o(In *in, const JObj *o, const char *name)
{
    return r_field(in, o, name, 'L');
}

static JObj *r_array(In *in, const JObj *o, const char *name, const char *cls)
{
    JObj *a = r_field(in, o, name, '[');
    if (a && (a->kind != JK_ARRAY || strcmp(a->cls->name, cls) != 0)) {
        bad(in, "%s.%s in the file isn't a %s.", class_of(o), name, cls);
        return NULL;
    }
    return a;
}

static char *r_string(In *in, const JObj *o, const char *name)
{
    JObj *s = r_o(in, o, name);
    if (!s)
        return NULL;
    if (s->kind != JK_STRING) {
        bad(in, "%s.%s in the file isn't a string.", class_of(o), name);
        return NULL;
    }
    char *c = map_get(&in->strings, s);
    if (!c) {
        c = jstr_new(s->str);
        map_put(&in->strings, s, c);
    }
    return c;
}

static JColor color_value(In *in, const JObj *c)
{
    if (!c)
        return 0;
    if (!jobj_is(c, "java.awt.Color")) {
        bad(in, "A colour in the file isn't one.");
        return 0;
    }
    JColor v = (JColor)(uint32_t)r_i(in, c, "value", 'I');
    if (!v)
        bad(in, "A colour in the file is transparent black, which Color Mod never makes.");
    return v;
}

static JColor r_color(In *in, const JObj *o, const char *name)
{
    return color_value(in, r_o(in, o, name));
}

static int *r_ints(In *in, const JObj *o, const char *name)
{
    JObj *a = r_array(in, o, name, "[I");
    if (!a)
        return NULL;
    int *v = JARR_NEW(int, a->nvals);
    for (int i = 0; i < a->nvals; i++)
        v[i] = (int)a->vals[i].i;
    return v;
}

static double *r_doubles(In *in, const JObj *o, const char *name)
{
    JObj *a = r_array(in, o, name, "[D");
    if (!a)
        return NULL;
    double *v = JARR_NEW(double, a->nvals);
    for (int i = 0; i < a->nvals; i++)
        v[i] = a->vals[i].d;
    return v;
}

static JColor *r_colors(In *in, const JObj *o, const char *name)
{
    JObj *a = r_array(in, o, name, "[Ljava.awt.Color;");
    if (!a)
        return NULL;
    JColor *v = JARR_NEW(JColor, a->nvals);
    for (int i = 0; i < a->nvals; i++)
        v[i] = color_value(in, a->vals[i].o);
    return v;
}

static void r_rect(In *in, const JObj *x, JRect *r)
{
    if (!x)
        return; /* never null in Color Mod; an empty rectangle */
    if (!jobj_is(x, "java.awt.Rectangle")) {
        bad(in, "A rectangle in the file isn't one.");
        return;
    }
    r->x = (int)r_i(in, x, "x", 'I');
    r->y = (int)r_i(in, x, "y", 'I');
    r->width = (int)r_i(in, x, "width", 'I');
    r->height = (int)r_i(in, x, "height", 'I');
}

static Gene *r_gene(In *in, const JObj *x);
static GeneticCode *r_code(In *in, const JObj *x);

static Gene **r_genes(In *in, const JObj *o, const char *name)
{
    JObj *a = r_array(in, o, name, "[Lbiogenesis.Gene;");
    if (!a)
        return NULL;
    Gene **v = JARR_NEW(Gene *, a->nvals);
    for (int i = 0; i < a->nvals; i++)
        v[i] = r_gene(in, a->vals[i].o);
    return v;
}

#include "io/bgw_classes.h"

/* ---- the translated classes, through bgw_classes.h ---- */

static JObj *w_gene(Out *o, const Gene *g)
{
    if (!g)
        return NULL;
    JObj *x = map_get(&o->genes, g);
    if (x)
        return x;
    x = jobj_new(&o->g, &c_gene);
    map_put(&o->genes, g, x);
    w_gene_fields(o, x, g);
    return x;
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
    w_code_fields(o, x, gc);
    return x;
}

static JObj *w_org(Out *o, const Organism *b)
{
    JObj *x = map_get(&o->orgs, b);
    if (x)
        return x;
    x = jobj_new(&o->g, &c_organism);
    map_put(&o->orgs, b, x);
    /* an Organism is a java.awt.Rectangle */
    jobj_set(x, "x", vi(b->r.x));
    jobj_set(x, "y", vi(b->r.y));
    jobj_set(x, "width", vi(b->r.width));
    jobj_set(x, "height", vi(b->r.height));
    w_organism_fields(o, x, b);
    return x;
}

static Gene *r_gene(In *in, const JObj *x)
{
    if (!x || in->bad)
        return NULL;
    Gene *g = map_get(&in->genes, x);
    if (g)
        return g;
    if (!jobj_is(x, "biogenesis.Gene"))
        return bad(in, "A gene in the file isn't one."), NULL;
    g = gene_alloc_blank();
    map_put(&in->genes, x, g);
    r_gene_fields(in, x, g);
    if (!g->color)
        bad(in, "A gene in the file has no colour.");
    return g;
}

static GeneticCode *r_code(In *in, const JObj *x)
{
    if (!x || in->bad)
        return NULL;
    GeneticCode *gc = map_get(&in->codes, x);
    if (gc)
        return gc;
    if (!jobj_is(x, "biogenesis.GeneticCode"))
        return bad(in, "A genetic code in the file isn't one."), NULL;
    gc = gc_alloc_blank();
    map_put(&in->codes, x, gc);
    r_code_fields(in, x, gc);
    /* what the simulation indexes by */
    if (!in->bad && (!gc->genes || JLEN(gc->genes) < 1 || gc->symmetry < 1 || gc->symmetry > 8))
        return bad(in, "A genetic code in the file is damaged."), NULL;
    for (int i = 0; !in->bad && i < JLEN(gc->genes); i++)
        if (!gc->genes[i])
            bad(in, "A genetic code in the file has a missing gene.");
    return in->bad ? NULL : gc;
}

/* Every per-segment array the rules index by segment must have one entry
 * per segment: Java would stop at the first out-of-range index, C must
 * never get there. */
static void check_segments(In *in, const Organism *o)
{
    int n = o->segments;
    if (n != JLEN(o->gc->genes) * o->gc->symmetry)
        bad(in, "An organism's segments don't match its genetic code.");
    if (o->growth_ratio < 1 || o->growth_ratio > 16)
        bad(in, "An organism in the file has an impossible size.");
#define SEG(a)                                                                                     \
    if (!in->bad && (!(a) || JLEN(a) != n))                                                        \
        bad(in, "An organism's " #a " doesn't match its segments.");
    SEG(o->start_x) SEG(o->start_y) SEG(o->end_x) SEG(o->end_y) SEG(o->m1) SEG(o->m2) SEG(o->m)
    SEG(o->mphoto) SEG(o->x1) SEG(o->y1) SEG(o->x2) SEG(o->y2) SEG(o->seg_color)
    SEG(o->segred_reaction) SEG(o->seggreen_reaction) SEG(o->segblue_reaction)
    SEG(o->segplague_reaction) SEG(o->segscourge_reaction) SEG(o->segwhite_reaction)
    SEG(o->seggray_reaction) SEG(o->segdefault_reaction) SEG(o->segconsumer_reaction)
    SEG(o->segplant_reaction) SEG(o->segmagenta_reaction) SEG(o->segpink_reaction)
    SEG(o->segcoral_reaction) SEG(o->segorange_reaction) SEG(o->segbark_reaction)
    SEG(o->segviolet_reaction) SEG(o->segvirus_reaction) SEG(o->segmaroon_reaction)
    SEG(o->segolive_reaction) SEG(o->segmint_reaction) SEG(o->segcream_reaction)
    SEG(o->segspike_reaction) SEG(o->segfallow_reaction) SEG(o->seglightblue_reaction)
    SEG(o->segochre_reaction) SEG(o->segsky_reaction) SEG(o->seglilac_reaction)
    SEG(o->segsilver_reaction) SEG(o->segfire_reaction) SEG(o->seglightbrown_reaction)
    SEG(o->seggreenbrown_reaction) SEG(o->segbrown_reaction) SEG(o->segice_reaction)
    SEG(o->segbroken_reaction) SEG(o->segsick_reaction) SEG(o->segfriend_reaction)
#undef SEG
}

static Organism *r_org(In *in, const JObj *x)
{
    Organism *o = map_get(&in->orgs, x);
    if (o || in->bad)
        return o;
    if (!jobj_is(x, "biogenesis.Organism"))
        return bad(in, "An organism in the file isn't one."), NULL;
    o = org_alloc_blank();
    map_put(&in->orgs, x, o);
    if (in->nmade == in->made_cap) {
        in->made_cap = in->made_cap ? in->made_cap * 2 : 64;
        in->made = realloc(in->made, (size_t)in->made_cap * sizeof *in->made);
        if (!in->made)
            abort();
    }
    in->made[in->nmade++] = o;
    o->r.x = (int)r_i(in, x, "x", 'I');
    o->r.y = (int)r_i(in, x, "y", 'I');
    o->r.width = (int)r_i(in, x, "width", 'I');
    o->r.height = (int)r_i(in, x, "height", 'I');
    r_organism_fields(in, x, o);
    if (!in->bad && !o->gc)
        bad(in, "An organism in the file has no genetic code.");
    if (!in->bad)
        check_segments(in, o);
    return o;
}

/* ---- the world, its tree and its statistics ---- */

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
    jobj_set(x, "maxMethane", vd(s->max_methane));
    jobj_set(x, "maxMethaneTime", vi(s->max_methane_time));
    jobj_set(x, "maxOxygen", vd(s->max_oxygen));
    jobj_set(x, "maxOxygenTime", vi(s->max_oxygen_time));
    jobj_set(x, "maxPopulation", vi(s->max_population));
    jobj_set(x, "maxPopulationTime", vi(s->max_population_time));
    jobj_set(x, "minCarbonDioxide", vd(s->min_co2));
    jobj_set(x, "minCarbonDioxideTime", vi(s->min_co2_time));
    jobj_set(x, "minMethane", vd(s->min_methane));
    jobj_set(x, "minMethaneTime", vi(s->min_methane_time));
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
    jobj_set(x, "methaneList", vo(w_series(o, &s->methane)));
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
    jobj_set(x, "_CH4", vd(w->ch4));
    jobj_set(x, "_CO2", vd(w->co2));
    jobj_set(x, "_O2", vd(w->o2));
    jobj_set(x, "_corridorexists", vi(w->corridor_exists));
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
    free(o.strings.e);
    free(o.colors.e);
    FILE *f = fopen(path, "wb");
    bool ok = f && fwrite(bytes, 1, len, f) == len;
    if (f && fclose(f) != 0)
        ok = false;
    free(bytes);
    if (!ok)
        set_err(err, errn, "Can't write the file");
    return ok;
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
        bad(in, "Unsupported list class %s.", class_of(l));
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
    } else {
        int n;
        JObj **items = r_list(in, r_o(in, t, "itemBoundables"), &n);
        for (int i = 0; i < n && !in->bad; i++)
            r_item(in, items[i]);
        free(items);
    }
    /* Java keeps the tree until the next frame; pasting asks it */
    w->tree_valid = !in->bad && w->str_built;
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
    s->max_methane = r_d(in, x, "maxMethane");
    s->max_methane_time = r_i(in, x, "maxMethaneTime", 'J');
    s->max_oxygen = r_d(in, x, "maxOxygen");
    s->max_oxygen_time = r_i(in, x, "maxOxygenTime", 'J');
    s->max_population = (int)r_i(in, x, "maxPopulation", 'I');
    s->max_population_time = r_i(in, x, "maxPopulationTime", 'J');
    s->min_co2 = r_d(in, x, "minCarbonDioxide");
    s->min_co2_time = r_i(in, x, "minCarbonDioxideTime", 'J');
    s->min_methane = r_d(in, x, "minMethane");
    s->min_methane_time = r_i(in, x, "minMethaneTime", 'J');
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
        *codes[i].slot = r_code(in, r_o(in, x, codes[i].name));
    r_series(in, r_o(in, x, "populationList"), &s->population);
    r_series(in, r_o(in, x, "deathList"), &s->deaths);
    r_series(in, r_o(in, x, "birthList"), &s->births);
    r_series(in, r_o(in, x, "oxygenList"), &s->oxygen);
    r_series(in, r_o(in, x, "carbonDioxideList"), &s->co2);
    r_series(in, r_o(in, x, "methaneList"), &s->methane);
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
            if (cap >= ((size_t)1 << 31))
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
    } else if (!jobj_field(root, "_CH4", 'D')) {
        bad(&in, "This is a Biogenesis world without Color Mod's methane: open it with "
                 "syn-biogenesis.");
    } else {
        w.next_id = (int)r_i(&in, root, "NEXT_ID", 'I');
        w.ch4 = r_d(&in, root, "_CH4");
        w.co2 = r_d(&in, root, "_CO2");
        w.o2 = r_d(&in, root, "_O2");
        w.corridor_exists = false; /* corridors are transient: none after a load */
        w.width = (int)r_i(&in, root, "_width", 'I');
        w.height = (int)r_i(&in, root, "_height", 'I');
        w.population = (int)r_i(&in, root, "_population", 'I');
        w.nframes = (int)r_i(&in, root, "nFrames", 'I');
        if (w.width <= 0 || w.height <= 0 || w.width > 1000000 || w.height > 1000000)
            bad(&in, "The world in the file has an impossible size.");
        int n = 0;
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
    Map listed = {0};
    for (int i = 0; i < w.norgs; i++)
        map_put(&listed, w.orgs[i], w.orgs[i]);
    for (int i = 0; i < in.nmade; i++) {
        Organism *o = in.made[i];
        if (map_get(&listed, o))
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
    free(listed.e);
    free(in.orgs.e);
    free(in.codes.e);
    free(in.genes.e);
    free(in.strings.e);
    free(in.made);
    jgraph_free(&g);
    if (in.bad) {
        /* organisms not yet in the world are freed with it: put them there */
        for (int i = 0; i < w.ngrave; i++)
            world_append_raw(&w, w.grave[i]);
        w.ngrave = 0;
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
