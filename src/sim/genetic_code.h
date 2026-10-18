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

/* Genes and genetic codes (Gene.java, GeneticCode.java).
 *
 * A gene is one segment: a length, an angle relative to the previous
 * segment, and a colour that decides what the segment does. A genetic code
 * is the gene sequence plus how it is repeated: symmetry (1-8 copies around
 * the centre) and mirroring.
 *
 * Codes are reference counted and compared by identity, because Java
 * compared them by reference: a white segment refuses to re-infect an
 * organism already carrying *this very* code, and pasted clones share their
 * source's code object.
 */
#ifndef SIM_GENETIC_CODE_H
#define SIM_GENETIC_CODE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    COL_GREEN,  /* photosynthesis */
    COL_RED,    /* eats other organisms */
    COL_CYAN,   /* cilia: random propulsion */
    COL_BLUE,   /* shield */
    COL_WHITE,  /* infects with its own code */
    COL_GRAY,   /* kills on touch */
    COL_YELLOW, /* extra children per birth */
    COL_BROWN,  /* dead */
    COL_COUNT
} BioColor;

#define GENE_COLORS 7 /* the colours a gene can have: green..yellow */

typedef struct {
    double length;
    double theta;
    uint8_t color; /* BioColor */
    /* Which Java Gene object this is: a child's code shares the genes it
     * copies unchanged with its parent's (GeneticCode(parent)), and saved
     * worlds (.bgw) keep that sharing. Copies keep the id; new genes get
     * gene_new_id(). */
    uint32_t id;
} Gene;

typedef struct GeneticCode {
    int refs;
    /* Set by tools to follow a code's descendants (tools/being): a mutated
     * child keeps its parent's mark. No effect on the simulation. */
    uint32_t lineage;
    int symmetry; /* 1..8 */
    int mirror;   /* 0 or 1 */
    bool disperse_children;
    int reproduce_energy;
    int max_age;
    int ngenes;
    Gene genes[];
} GeneticCode;

void gene_randomize_color(Gene *g);
void gene_randomize_length(Gene *g);
void gene_randomize_theta(Gene *g);
void gene_randomize(Gene *g);
uint32_t gene_new_id(void);

/* new GeneticCode(): a random code. */
GeneticCode *gc_new_random(void);
/* new GeneticCode(parent): a mutated copy. */
GeneticCode *gc_new_mutated(const GeneticCode *parent);
/* new GeneticCode(genes, symmetry, mirror, disperse), the genes new objects */
GeneticCode *gc_new_from_genes(const Gene *genes, int n, int symmetry, int mirror, bool disperse);

GeneticCode *gc_ref(GeneticCode *gc);
void gc_unref(GeneticCode *gc);

/* Replace *slot with gc, managing references (either may be NULL). */
void gc_assign(GeneticCode **slot, GeneticCode *gc);

/* The flat drawing GeneticCode.draw makes: one line per segment, centred on
 * (0,0) in unscaled units. Returns the number of segments written. */
typedef struct {
    int x0, y0, x1, y1;
    uint8_t color;
} GcLine;
int gc_layout(const GeneticCode *gc, GcLine *out, int max, int *width, int *height);

const char *bio_color_name(int c);
/* RGB of each BioColor plus the transient flash colours. */
void bio_color_rgb(int c, uint8_t *r, uint8_t *g, uint8_t *b);

#endif
