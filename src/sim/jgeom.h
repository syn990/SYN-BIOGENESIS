/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Translated to C in 2026 from ExLine2DDouble.java
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

/* The java.awt geometry Biogenesis leans on, with Java's exact semantics.
 *
 * Organism extends java.awt.Rectangle, and collisions are found with
 * Rectangle.intersectsLine and Line2D.linesIntersect. Their edge cases
 * (empty rectangles, inclusive vs exclusive borders, collinear segments)
 * decide whether two organisms touch, so they are reproduced here rather
 * than replaced with "equivalent" maths. tests/geom_check compares them
 * with the JVM on random inputs.
 */
#ifndef SIM_JGEOM_H
#define SIM_JGEOM_H

#include <stdbool.h>

typedef struct {
    int x, y, width, height;
} JRect;

typedef struct {
    double x1, y1, x2, y2;
} JLine;

static inline void jrect_set(JRect *r, int x, int y, int w, int h)
{
    r->x = x;
    r->y = y;
    r->width = w;
    r->height = h;
}

/* Rectangle.intersects(Rectangle) */
bool jrect_intersects(const JRect *t, const JRect *r);
/* Rectangle.contains(int, int) */
bool jrect_contains(const JRect *r, int X, int Y);
/* Rectangle2D.intersectsLine(Line2D) with Rectangle's outcode */
bool jrect_intersects_line(const JRect *r, const JLine *l);
/* Line2D.intersectsLine(Line2D) */
bool jline_intersects_line(const JLine *a, const JLine *b);
/* ExLine2DDouble.getIntersection (Biogenesis' own helper) */
void jline_intersection(const JLine *self, const JLine *l, double tol, double *ox, double *oy);

/* Math.min / Math.max for doubles, including NaN and -0.0 handling.
 * The strict comparisons settle almost every call; the rest follows
 * Java: NaN wins, and -0.0 counts as smaller than +0.0. */
static inline double jmin(double a, double b)
{
    if (a < b)
        return a;
    if (b < a)
        return b;
    if (a != a)
        return a;
    if (b != b)
        return b;
    return (a == 0.0 && __builtin_signbit(b)) ? b : a;
}

static inline double jmax(double a, double b)
{
    if (a > b)
        return a;
    if (b > a)
        return b;
    if (a != a)
        return a;
    if (b != b)
        return b;
    return (a == 0.0 && __builtin_signbit(a)) ? b : a;
}
/* Math.round(double) narrowed to int, as (int) Math.round(x) does. */
int jround(double x);

#endif
