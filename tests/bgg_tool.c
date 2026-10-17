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

/* C side of tools/javaref/BggInterop.java:
 *   bgg_tool write DIR SEED N   N random codes (same java.util.Random stream)
 *   bgg_tool read FILE...       parse and print each in the same canonical form,
 *                               or "reject" */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io/bgg.h"
#include "sim/params.h"

static uint64_t bits(double d)
{
    uint64_t u;
    memcpy(&u, &d, sizeof u);
    return u;
}

int main(int argc, char **argv)
{
    params_set_defaults(&P);
    if (argc >= 5 && !strcmp(argv[1], "write")) {
        jr_set_seed(&g_rng, atoll(argv[3]));
        int n = atoi(argv[4]);
        for (int i = 0; i < n; i++) {
            char path[1024];
            snprintf(path, sizeof path, "%s/c_%d.bgg", argv[2], i);
            GeneticCode *gc = gc_new_random();
            if (!bgg_write(path, gc))
                return 1;
            gc_unref(gc);
        }
        return 0;
    }
    for (int k = 2; k < argc; k++) {
        char err[256];
        GeneticCode *gc = bgg_read(argv[k], err, sizeof err);
        if (!gc) {
            printf("reject\n");
            continue;
        }
        printf("%d %d %d %d", gc->symmetry, gc->mirror, gc->disperse_children ? 1 : 0,
               gc->ngenes);
        for (int i = 0; i < gc->ngenes; i++)
            printf(" %016" PRIx64 " %016" PRIx64 " %s", bits(gc->genes[i].length),
                   bits(gc->genes[i].theta), bio_color_name(gc->genes[i].color));
        printf("\n");
        gc_unref(gc);
    }
    return 0;
}
