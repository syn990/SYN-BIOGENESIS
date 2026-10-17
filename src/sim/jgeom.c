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

#include "sim/jgeom.h"

#include <math.h>

/* The Makefile builds with -fwrapv: the overflow checks below are Java's,
 * and Java ints wrap. */

bool jrect_intersects(const JRect *t, const JRect *r)
{
    int tw = t->width, th = t->height;
    int rw = r->width, rh = r->height;
    if (rw <= 0 || rh <= 0 || tw <= 0 || th <= 0)
        return false;
    int tx = t->x, ty = t->y;
    int rx = r->x, ry = r->y;
    rw += rx;
    rh += ry;
    tw += tx;
    th += ty;
    return ((rw < rx || rw > tx) && (rh < ry || rh > ty) && (tw < tx || tw > rx) &&
            (th < ty || th > ry));
}

bool jrect_contains(const JRect *r, int X, int Y)
{
    int w = r->width, h = r->height;
    if ((w | h) < 0)
        return false;
    int x = r->x, y = r->y;
    if (X < x || Y < y)
        return false;
    w += x;
    h += y;
    return ((w < x || w > X) && (h < y || h > Y));
}

enum { OUT_LEFT = 1, OUT_TOP = 2, OUT_RIGHT = 4, OUT_BOTTOM = 8 };

static int jrect_outcode(const JRect *r, double x, double y)
{
    int out = 0;
    if (r->width <= 0)
        out |= OUT_LEFT | OUT_RIGHT;
    else if (x < r->x)
        out |= OUT_LEFT;
    else if (x > r->x + (double)r->width)
        out |= OUT_RIGHT;
    if (r->height <= 0)
        out |= OUT_TOP | OUT_BOTTOM;
    else if (y < r->y)
        out |= OUT_TOP;
    else if (y > r->y + (double)r->height)
        out |= OUT_BOTTOM;
    return out;
}

bool jrect_intersects_line(const JRect *r, const JLine *l)
{
    double x1 = l->x1, y1 = l->y1, x2 = l->x2, y2 = l->y2;
    int out1, out2;
    if ((out2 = jrect_outcode(r, x2, y2)) == 0)
        return true;
    while ((out1 = jrect_outcode(r, x1, y1)) != 0) {
        if ((out1 & out2) != 0)
            return false;
        if ((out1 & (OUT_LEFT | OUT_RIGHT)) != 0) {
            double x = r->x;
            if ((out1 & OUT_RIGHT) != 0)
                x += r->width;
            y1 = y1 + (x - x1) * (y2 - y1) / (x2 - x1);
            x1 = x;
        } else {
            double y = r->y;
            if ((out1 & OUT_BOTTOM) != 0)
                y += r->height;
            x1 = x1 + (y - y1) * (x2 - x1) / (y2 - y1);
            y1 = y;
        }
    }
    return true;
}

static int relative_ccw(double x1, double y1, double x2, double y2, double px, double py)
{
    x2 -= x1;
    y2 -= y1;
    px -= x1;
    py -= y1;
    double ccw = px * y2 - py * x2;
    if (ccw == 0.0) {
        ccw = px * x2 + py * y2;
        if (ccw > 0.0) {
            px -= x2;
            py -= y2;
            ccw = px * x2 + py * y2;
            if (ccw < 0.0)
                ccw = 0.0;
        }
    }
    return (ccw < 0.0) ? -1 : ((ccw > 0.0) ? 1 : 0);
}

bool jline_intersects_line(const JLine *a, const JLine *b)
{
    return (relative_ccw(a->x1, a->y1, a->x2, a->y2, b->x1, b->y1) *
                    relative_ccw(a->x1, a->y1, a->x2, a->y2, b->x2, b->y2) <=
                0) &&
           (relative_ccw(b->x1, b->y1, b->x2, b->y2, a->x1, a->y1) *
                    relative_ccw(b->x1, b->y1, b->x2, b->y2, a->x2, a->y2) <=
                0);
}

void jline_intersection(const JLine *s, const JLine *l, double tol, double *ox, double *oy)
{
    double d = (l->x1 - l->x2) * (s->y2 - s->y1) + (s->x2 - s->x1) * (l->y2 - l->y1);
    /* Not fabs: the original treats every negative determinant as "parallel"
     * and takes the midpoint of the four ends. Kept, it changes where the
     * impulse lands. */
    if (d < tol) {
        *ox = (s->x1 + s->x2 + l->x1 + l->x2) / 4.0;
        *oy = (s->y1 + s->y2 + l->y1 + l->y2) / 4.0;
        return;
    }
    double t = ((l->x1 - s->x1) * (s->y2 - s->y1) + (s->x2 - s->x1) * (s->y1 - l->y1)) / d;
    *ox = l->x1 + t * (l->x2 - l->x1);
    *oy = l->y1 + t * (l->y2 - l->y1);
}

int jround(double x)
{
    /* Math.round: floor(x + 1/2) computed without the rounding error of the
     * addition, then (int) of the long result. */
    if (x != x)
        return 0;
    double f = floor(x);
    long long r = (long long)f;
    if (x - f >= 0.5)
        r += 1;
    return (int)r;
}
