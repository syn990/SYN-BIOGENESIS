/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Part of a C port of Biogenesis (https://biogenesis.sourceforge.net/).
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

/* See GeomCheck.java: same inputs, C results, compared by tests/run.sh. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "sim/jgeom.h"
#include "sim/jrandom.h"

int main(int argc, char **argv)
{
    if (argc < 3)
        return 2;
    JRandom r;
    jr_set_seed(&r, atoll(argv[1]));
    int n = atoi(argv[2]);
    for (int i = 0; i < n; i++) {
        JRect a, b;
        a.x = jr_next_int(&r, 41) - 20;
        a.y = jr_next_int(&r, 41) - 20;
        a.width = jr_next_int(&r, 23) - 2;
        a.height = jr_next_int(&r, 23) - 2;
        b.x = jr_next_int(&r, 41) - 20;
        b.y = jr_next_int(&r, 41) - 20;
        b.width = jr_next_int(&r, 23) - 2;
        b.height = jr_next_int(&r, 23) - 2;
        int px = jr_next_int(&r, 41) - 20;
        int py = jr_next_int(&r, 41) - 20;
        double c[8];
        for (int k = 0; k < 8; k++)
            c[k] = jr_next_boolean(&r) ? jr_next_int(&r, 61) - 30 : jr_next_double(&r) * 60 - 30;
        JLine l1 = {c[0], c[1], c[2], c[3]}, l2 = {c[4], c[5], c[6], c[7]};
        double d = jr_next_double(&r) * 20 - 10;
        printf("%d%d%d%d %d %d\n", jrect_intersects(&a, &b), jrect_contains(&a, px, py),
               jrect_intersects_line(&a, &l1), jline_intersects_line(&l1, &l2), jround(d),
               jround(floor(d) + 0.5));
    }
    return 0;
}
