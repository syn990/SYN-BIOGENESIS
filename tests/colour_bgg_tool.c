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

/* C side of tools/javaref/colour/ColourBggInterop.java, for the colour build:
 *   colour_bgg_tool write DIR SEED N   N random codes (same java.util.Random stream)
 *   colour_bgg_tool read FILE...       parse and print each in the same canonical
 *                                      form, or "reject" */
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

static void canonical(const GeneticCode *g)
{
    printf("%d %d %d %d %d %d %d %d %d%d%d%d%d%d%d%d%d%d%d%d%d%d%d %d", g->symmetry, g->mirror,
           g->mutationrate, g->activity, g->modifiescream, g->modifiesfallow, g->modifiesspore,
           g->adaptspore, g->plague, g->disperse_children, g->generation_battle,
           g->sibling_battle, g->altruist, g->familial, g->social, g->peaceful, g->passive,
           g->clockwise, g->mimicall, g->modifiespink, g->modifieslilac, g->modifiessky,
           g->selfish, JLEN(g->genes));
    for (int i = 0; i < JLEN(g->genes); i++) {
        const Gene *e = g->genes[i];
        const int r[GENE_REACTIONS] = {
            e->redreaction,        e->greenreaction,      e->bluereaction,     e->plaguereaction,
            e->scourgereaction,    e->whitereaction,      e->grayreaction,     e->silverreaction,
            e->defaultreaction,    e->consumerreaction,   e->plantreaction,    e->magentareaction,
            e->pinkreaction,       e->coralreaction,      e->orangereaction,   e->barkreaction,
            e->violetreaction,     e->virusreaction,      e->maroonreaction,   e->olivereaction,
            e->mintreaction,       e->creamreaction,      e->spikereaction,    e->fallowreaction,
            e->lightbluereaction,  e->ochrereaction,      e->skyreaction,      e->lilacreaction,
            e->firereaction,       e->lightbrownreaction, e->greenbrownreaction, e->brownreaction,
            e->icereaction,        e->brokenreaction,     e->sickreaction,     e->friendreaction,
        };
        printf(" %016" PRIx64 " %016" PRIx64 " %08x %d", bits(e->length), bits(e->theta),
               (unsigned)e->color, e->branch);
        for (int k = 0; k < GENE_REACTIONS; k++)
            printf(",%d", r[k]);
    }
    printf("\n");
}

int main(int argc, char **argv)
{
    params_set_defaults(&P);
    if (argc >= 5 && !strcmp(argv[1], "write")) {
        jr_set_seed(&g_rng, atoll(argv[3]));
        int n = atoi(argv[4]);
        for (int i = 0; i < n; i++) {
            char path[4096];
            snprintf(path, sizeof path, "%s/c_%d.bgg", argv[2], i);
            if (!bgg_write(path, gc_new_random())) {
                perror(path);
                return 1;
            }
        }
        return 0;
    }
    if (argc >= 2 && !strcmp(argv[1], "read")) {
        for (int i = 2; i < argc; i++) {
            char err[256];
            GeneticCode *g = bgg_read(argv[i], err, sizeof err);
            if (g)
                canonical(g);
            else
                printf("reject\n");
        }
        return 0;
    }
    fprintf(stderr, "usage: colour_bgg_tool write DIR SEED N | read FILE...\n");
    return 2;
}
