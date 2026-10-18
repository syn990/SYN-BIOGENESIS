/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Part of the colour build, a C port of Biogenesis Color Mod 2.01
 * (https://sourceforge.net/projects/biogenesis/), Copyright (C) 2006-2010
 * Joan Queralt Molina, Color Mod Copyright (C) 2012-2022 MarcoDBAA.
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

/* The parts of Java that the translated Color Mod classes lean on
 * (tools/colour/j2c.py writes calls to these):
 *
 * - java.awt.Color as a packed ARGB value: Color Mod compares colours only
 *   with equals, and no two of its colours share an RGB value, so == on
 *   the packed value is Color.equals;
 * - Java's conversions: (int) of a double, Math.round, Math.abs, min/max;
 * - arrays that know their length, each owned by one field;
 * - String as an owned, copied char *;
 * - Point, Point2D.Double, Vector2D as values;
 * - a mark-and-sweep collector for the objects Java shares by reference
 *   (genetic codes and genes).
 */
#ifndef COLOUR_JRT_H
#define COLOUR_JRT_H

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sim/jgeom.h"
#include "sim/jmath.h"
#include "sim/jrandom.h"

/* ---- java.awt.Color ---- */

typedef uint32_t JColor; /* 0xAARRGGBB */

#define JRGB(r, g, b) ((JColor)(0xff000000u | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b)))
#define JCOLOR_R(c) ((int)(((c) >> 16) & 0xff))
#define JCOLOR_G(c) ((int)(((c) >> 8) & 0xff))
#define JCOLOR_B(c) ((int)((c) & 0xff))

#define JC_BLACK JRGB(0, 0, 0)
#define JC_BLUE JRGB(0, 0, 255)
#define JC_CYAN JRGB(0, 255, 255)
#define JC_DARK_GRAY JRGB(64, 64, 64)
#define JC_GRAY JRGB(128, 128, 128)
#define JC_GREEN JRGB(0, 255, 0)
#define JC_LIGHT_GRAY JRGB(192, 192, 192)
#define JC_MAGENTA JRGB(255, 0, 255)
#define JC_ORANGE JRGB(255, 200, 0)
#define JC_PINK JRGB(255, 175, 175)
#define JC_RED JRGB(255, 0, 0)
#define JC_WHITE JRGB(255, 255, 255)
#define JC_YELLOW JRGB(255, 255, 0)

/* Color.darker(): each channel times 0.7, truncated */
JColor jcolor_darker(JColor c);
/* A colour's channels, for the window's shared widgets, which know colours
 * as ints: in the colour build the int is the packed RGB value (defined in
 * src/colour/ui/colours.c). */
void bio_color_rgb(int c, uint8_t *r, uint8_t *g, uint8_t *b);

/* ---- numbers ---- */

/* Double.MIN_VALUE, the smallest positive double */
#define JDOUBLE_MIN_VALUE 0x1p-1074

/* (int) and (long) of a double: NaN is 0, out of range saturates */
static inline int32_t j_d2i(double d)
{
    if (d != d)
        return 0;
    if (d >= 2147483647.0)
        return INT32_MAX;
    if (d <= -2147483648.0)
        return INT32_MIN;
    return (int32_t)d;
}

static inline int64_t j_d2l(double d)
{
    if (d != d)
        return 0;
    if (d >= 9223372036854775807.0)
        return INT64_MAX;
    if (d <= -9223372036854775808.0)
        return INT64_MIN;
    return (int64_t)d;
}

/* Math.round(double), as java.lang.Math computes it since Java 8 */
int64_t j_lround(double a);

static inline int32_t j_iabs(int32_t a)
{
    return a < 0 ? (int32_t)(0u - (uint32_t)a) : a;
}

static inline int64_t j_labs(int64_t a)
{
    return a < 0 ? (int64_t)(0ull - (uint64_t)a) : a;
}

static inline int32_t j_imin(int32_t a, int32_t b)
{
    return a <= b ? a : b;
}

static inline int32_t j_imax(int32_t a, int32_t b)
{
    return a >= b ? a : b;
}

static inline int64_t j_lmin(int64_t a, int64_t b)
{
    return a <= b ? a : b;
}

static inline int64_t j_lmax(int64_t a, int64_t b)
{
    return a >= b ? a : b;
}

/* ---- arrays ----
 * A length header before the elements. Each array belongs to the one field
 * it was created for (j2c checks that fields are only given new arrays), so
 * a field's old array is freed when it gets a new one. */

void *jarr_new(int n, size_t elem);
void *jarr_dup(const void *a);
void jarr_free(void *a);
static inline int jarr_len(const void *a)
{
    return ((const int *)a)[-4];
}

#define JARR_NEW(T, n) ((T *)jarr_new((n), sizeof(T)))
#define JLEN(a) jarr_len(a)
#define JARR_SET(dst, src)                                                                       \
    do {                                                                                       \
        void *jarr_src_ = (src);                                                               \
        jarr_free(dst);                                                                        \
        (dst) = jarr_src_;                                                                     \
    } while (0)

/* ---- strings ----
 * Java's are immutable objects shared by reference, and a saved world
 * keeps the sharing (a child's code holds its parent's clade name, every
 * "01234567" is the one interned literal). So a String here is a collected
 * char array that never changes, and assigning one shares it. */

#define JSTR_LOOP_MAX 4096
/* a String in a concatenation: Java writes null as "null" */
#define JSTR_JAVA(s) ((s) ? (s) : "null")

/* a new String with s's text */
char *jstr_new(const char *s);
/* a new String, formatted */
char *jstr_newf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
/* the interned String of a literal: the same object every time */
char *jstr_lit(const char *text);
/* String.replace(char, char): s itself when nothing changes, as in Java */
char *jstr_replace(char *s, char a, char b);
void jstr_mark(const char *s);
void jstr_copy_to(char *buf, size_t n, const char *s);

/* ---- geometry values ---- */

typedef struct {
    double x, y;
} JPoint; /* java.awt.geom.Point2D.Double */

typedef struct {
    int x, y;
} JIPoint; /* java.awt.Point */

static inline void jline_set(JLine *l, double x1, double y1, double x2, double y2)
{
    l->x1 = x1;
    l->y1 = y1;
    l->x2 = x2;
    l->y2 = y2;
}

/* ExLine2DDouble.getIntersection */
static inline JPoint jline_get_intersection(const JLine *a, const JLine *b)
{
    JPoint p;
    jline_intersection(a, b, 0.0000001, &p.x, &p.y);
    return p;
}

/* Point2D.distanceSq: from p to q */
static inline double jpoint_distance_sq(JPoint p, JPoint q)
{
    double px = q.x - p.x;
    double py = q.y - p.y;
    return px * px + py * py;
}

/* Vector2D.java */
typedef struct {
    double modulus, theta;
} Vector2D;

static inline double vec_get_x(const Vector2D *v)
{
    return v->modulus * bio_cos(v->theta);
}

static inline double vec_get_y(const Vector2D *v)
{
    return v->modulus * bio_sin(v->theta);
}

static inline void vec_set_polar(Vector2D *v, double modulus, double theta)
{
    v->modulus = modulus;
    v->theta = theta;
}

static inline void vec_invert_x(Vector2D *v)
{
    v->theta = M_PI - v->theta;
}

static inline void vec_invert_y(Vector2D *v)
{
    v->theta = 2.0 * M_PI - v->theta;
}

/* ---- the collector ----
 * Genetic codes and genes are shared by reference in Java: an infected
 * organism carries its infector's code, a child's code shares its parent's
 * unchanged genes, the statistics remember codes of organisms long gone.
 * They are allocated here and freed by jgc_collect, which marks what the
 * roots reach (the world's organisms and statistics, and anything the
 * window holds through jgc_add_root) and frees the rest. It runs between
 * frames, when no C local holds a code. */

typedef struct JGcKind {
    const char *name;
    void (*mark)(void *obj);     /* mark what obj refers to */
    void (*finalize)(void *obj); /* free what obj owns (arrays, strings) */
} JGcKind;

typedef struct {
    uint32_t index;
    uint32_t mark;
    const JGcKind *kind;
} JGcHead; /* first member of every collected object */

void *jgc_alloc(size_t size, const JGcKind *kind);
void jgc_mark(void *obj);
/* Roots outside the world: a pointer variable the collector reads at
 * collection time. */
void jgc_add_root(void **slot);
void jgc_remove_root(void **slot);
/* Roots that move (the SYN edition's history): fn is called at collection
 * time and jgc_marks what it holds. */
void jgc_add_marker(void (*fn)(void));
/* Mark from the extra roots, then free everything unmarked. The caller marks
 * the world first (world_mark). */
void jgc_sweep(void);
size_t jgc_count(void);
size_t jgc_allocated_since_sweep(void);
/* World.time collects once this many objects are new (and as many as half
 * the live ones); 0 collects every frame (SYN_BIOGENESIS_GC_EVERY_FRAME,
 * for testing). */
extern size_t jgc_min_fresh;

/* ---- network corridors: not ported ---- */

typedef struct OutCorridor OutCorridor;

static inline bool out_corridor_can_send(const OutCorridor *c)
{
    (void)c;
    return false;
}

struct Organism;
static inline bool out_corridor_send(OutCorridor *c, struct Organism *o)
{
    (void)c;
    (void)o;
    return false;
}

#endif
