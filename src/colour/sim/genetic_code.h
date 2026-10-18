/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Translated to C in 2026 by tools/colour/j2c.py from Gene.java and GeneticCode.java of
 * Biogenesis Color Mod 2.01 (https://sourceforge.net/projects/biogenesis/),
 * Copyright (C) 2006-2010 Joan Queralt Molina,
 * Color Mod Copyright (C) 2012-2022 MarcoDBAA.
 * Generated: edit the translator or the Java, not this file.
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

/* Genes and genetic codes (Gene.java, GeneticCode.java) of Color Mod.

 * A gene is one segment: length, angle, colour, the branch it grows from,
 * and how it reacts to each colour it touches. A genetic code is the
 * genes, their symmetry and mirroring, and the code's traits (mutation
 * rate, activity, behaviours, clade). Both are collected objects (see
 * jgc_alloc in jrt.h), shared by reference as in Java.
 */
#ifndef COLOUR_GENETIC_CODE_H
#define COLOUR_GENETIC_CODE_H

#include <stdbool.h>
#include <stdint.h>

#include "sim/jrt.h"

typedef struct Gene Gene;

/* java.util.List<Gene>, for the constructor the file readers use */
typedef struct {
    Gene **items;
    int n;
} GeneList;

Gene **gene_list_to_array(const GeneList *l, Gene **a);

typedef struct Gene {
    JGcHead gch; /* the collector's, not Java's */
    double length;
    double theta;
    JColor color; /* Segment's color */
    int branch; /* Where a segment branches from another segment (-1 is no branch). */
    int redreaction; /* Segment's reactions */
    int greenreaction;
    int bluereaction;
    int plaguereaction;
    int scourgereaction;
    int whitereaction;
    int grayreaction;
    int silverreaction;
    int defaultreaction;
    int consumerreaction;
    int plantreaction;
    int magentareaction;
    int pinkreaction;
    int coralreaction;
    int orangereaction;
    int barkreaction;
    int violetreaction;
    int virusreaction;
    int maroonreaction;
    int olivereaction;
    int mintreaction;
    int creamreaction;
    int spikereaction;
    int fallowreaction;
    int lightbluereaction;
    int ochrereaction;
    int skyreaction;
    int lilacreaction;
    int firereaction;
    int lightbrownreaction;
    int greenbrownreaction;
    int brownreaction;
    int icereaction;
    int brokenreaction;
    int sickreaction;
    int friendreaction;
} Gene;

typedef struct GeneticCode {
    JGcHead gch; /* the collector's, not Java's */
    Gene **genes; /* Array with the genes. Every gene is represented by symmetry */
    int symmetry; /* The symmetry used when drawing the organism. Possible values are */
    int mirror; /* Mirroring indicates if symmetric segments are drawn in the same */
    int mutationrate; /* The mutationrate of this organism. Possible values are */
    int activity; /* The seasons of the summer segments. Possible values are */
    /* If the number of genes or the symmetry changed, the clade has to be updated. */
    bool update_clade;
    bool plague; /* Version of the plague. Possible values are */
    bool disperse_children; /* Indicates if children will be placed moving towards the same */
    bool generation_battle; /* Indicates if parents attack children and children attack parents. */
    bool sibling_battle; /* Indicates if siblings battle each other. */
    /* Indicates if organism will heal other altruist organisms or remove their infections. */
    bool altruist;
    /* Indicates if organism will heal related organisms or remove their infections. */
    bool familial;
    /* Indicates if organisms with rose segments are social towards other organisms with rose segments, */
    bool social;
    /* Indicates if organisms with rose segments are peaceful towards other organisms with rose segments. */
    bool peaceful;
    bool passive; /* Indicates if organisms with teal segments will move around the world. */
    /* Indicates if passive organisms with cyan segments will rotate clockwise or counter-clockwise */
    bool clockwise;
    /* Indicates if organisms will change the color of all dark segments or only of the segment, that is touched. */
    bool mimicall;
    bool modifiespink; /* Modifies the function of pink */
    int modifiescream; /* Modifies the function of enhanced cream */
    int modifiesspore; /* Modifies the function of spore */
    int adaptspore; /* Modifies the function of spore further */
    bool modifieslilac; /* Modifies the function of lilac */
    int modifiesfallow; /* Modifies the function of fallow */
    bool modifiessky; /* Modifies the function of sky */
    /* Indicates if organisms will always retain half of their energy if they reproduce. */
    bool selfish;
    int generation; /* Generation number */
    char *clade_id; /* Identification number of this organisms clade. */
} GeneticCode;

/* A zeroed object for the world reader: deserialization runs no
 * constructor and no field initialiser. */
Gene *gene_alloc_blank(void);
void gene_randomize_color(Gene *g);
void gene_randomizered_reaction(Gene *g);
void gene_randomizegreen_reaction(Gene *g);
void gene_randomizeblue_reaction(Gene *g);
void gene_randomizeplague_reaction(Gene *g);
void gene_randomizescourge_reaction(Gene *g);
void gene_randomizewhite_reaction(Gene *g);
void gene_randomizegray_reaction(Gene *g);
void gene_randomizesilver_reaction(Gene *g);
void gene_randomizedefault_reaction(Gene *g);
void gene_randomizeconsumer_reaction(Gene *g);
void gene_randomizeplant_reaction(Gene *g);
void gene_randomizemagenta_reaction(Gene *g);
void gene_randomizepink_reaction(Gene *g);
void gene_randomizecoral_reaction(Gene *g);
void gene_randomizeorange_reaction(Gene *g);
void gene_randomizebark_reaction(Gene *g);
void gene_randomizeviolet_reaction(Gene *g);
void gene_randomizevirus_reaction(Gene *g);
void gene_randomizemaroon_reaction(Gene *g);
void gene_randomizeolive_reaction(Gene *g);
void gene_randomizemint_reaction(Gene *g);
void gene_randomizecream_reaction(Gene *g);
void gene_randomizespike_reaction(Gene *g);
void gene_randomizefallow_reaction(Gene *g);
void gene_randomizelightblue_reaction(Gene *g);
void gene_randomizeochre_reaction(Gene *g);
void gene_randomizesky_reaction(Gene *g);
void gene_randomizelilac_reaction(Gene *g);
void gene_randomizefire_reaction(Gene *g);
void gene_randomizelightbrown_reaction(Gene *g);
void gene_randomizegreenbrown_reaction(Gene *g);
void gene_randomizebrown_reaction(Gene *g);
void gene_randomizeice_reaction(Gene *g);
void gene_randomizebroken_reaction(Gene *g);
void gene_randomizesick_reaction(Gene *g);
void gene_randomizefriend_reaction(Gene *g);
void gene_randomize_length(Gene *g);
void gene_randomize_theta(Gene *g);
void gene_randomize(Gene *g);
void *gene_clone(Gene *g);
double gene_get_length(Gene *g);
double gene_get_theta(Gene *g);
JColor gene_get_color(Gene *g);
int gene_get_branch(Gene *g);
int gene_getred_reaction(Gene *g);
int gene_getgreen_reaction(Gene *g);
int gene_getblue_reaction(Gene *g);
int gene_getplague_reaction(Gene *g);
int gene_getscourge_reaction(Gene *g);
int gene_getwhite_reaction(Gene *g);
int gene_getgray_reaction(Gene *g);
int gene_getsilver_reaction(Gene *g);
int gene_getdefault_reaction(Gene *g);
int gene_getconsumer_reaction(Gene *g);
int gene_getplant_reaction(Gene *g);
int gene_getmagenta_reaction(Gene *g);
int gene_getpink_reaction(Gene *g);
int gene_getcoral_reaction(Gene *g);
int gene_getorange_reaction(Gene *g);
int gene_getbark_reaction(Gene *g);
int gene_getviolet_reaction(Gene *g);
int gene_getvirus_reaction(Gene *g);
int gene_getmaroon_reaction(Gene *g);
int gene_getolive_reaction(Gene *g);
int gene_getmint_reaction(Gene *g);
int gene_getcream_reaction(Gene *g);
int gene_getspike_reaction(Gene *g);
int gene_getfallow_reaction(Gene *g);
int gene_getlightblue_reaction(Gene *g);
int gene_getochre_reaction(Gene *g);
int gene_getsky_reaction(Gene *g);
int gene_getlilac_reaction(Gene *g);
int gene_getfire_reaction(Gene *g);
int gene_getlightbrown_reaction(Gene *g);
int gene_getgreenbrown_reaction(Gene *g);
int gene_getbrown_reaction(Gene *g);
int gene_getice_reaction(Gene *g);
int gene_getbroken_reaction(Gene *g);
int gene_getsick_reaction(Gene *g);
int gene_getfriend_reaction(Gene *g);
void gene_set_color(Gene *g, JColor color);
void gene_set_branch(Gene *g, int branch);
void gene_setred_reaction(Gene *g, int redreaction);
void gene_setgreen_reaction(Gene *g, int greenreaction);
void gene_setblue_reaction(Gene *g, int bluereaction);
void gene_setplague_reaction(Gene *g, int plaguereaction);
void gene_setscourge_reaction(Gene *g, int scourgereaction);
void gene_setwhite_reaction(Gene *g, int whitereaction);
void gene_setgray_reaction(Gene *g, int grayreaction);
void gene_setsilver_reaction(Gene *g, int silverreaction);
void gene_setdefault_reaction(Gene *g, int defaultreaction);
void gene_setconsumer_reaction(Gene *g, int consumerreaction);
void gene_setplant_reaction(Gene *g, int plantreaction);
void gene_setmagenta_reaction(Gene *g, int magentareaction);
void gene_setpink_reaction(Gene *g, int pinkreaction);
void gene_setcoral_reaction(Gene *g, int coralreaction);
void gene_setorange_reaction(Gene *g, int orangereaction);
void gene_setbark_reaction(Gene *g, int barkreaction);
void gene_setviolet_reaction(Gene *g, int violetreaction);
void gene_setvirus_reaction(Gene *g, int virusreaction);
void gene_setmaroon_reaction(Gene *g, int maroonreaction);
void gene_setolive_reaction(Gene *g, int olivereaction);
void gene_setmint_reaction(Gene *g, int mintreaction);
void gene_setcream_reaction(Gene *g, int creamreaction);
void gene_setspike_reaction(Gene *g, int spikereaction);
void gene_setfallow_reaction(Gene *g, int fallowreaction);
void gene_setlightblue_reaction(Gene *g, int lightbluereaction);
void gene_setochre_reaction(Gene *g, int ochrereaction);
void gene_setsky_reaction(Gene *g, int skyreaction);
void gene_setlilac_reaction(Gene *g, int lilacreaction);
void gene_setfire_reaction(Gene *g, int firereaction);
void gene_setlightbrown_reaction(Gene *g, int lightbrownreaction);
void gene_setgreenbrown_reaction(Gene *g, int greenbrownreaction);
void gene_setbrown_reaction(Gene *g, int brownreaction);
void gene_setice_reaction(Gene *g, int icereaction);
void gene_setbroken_reaction(Gene *g, int brokenreaction);
void gene_setsick_reaction(Gene *g, int sickreaction);
void gene_setfriend_reaction(Gene *g, int friendreaction);
void gene_set_length(Gene *g, double length);
void gene_set_theta(Gene *g, double theta);
Gene *gene_new(void);
Gene *gene_new_full(double length, double theta, JColor color, int branch, int redreaction, int greenreaction, int bluereaction, int plaguereaction, int scourgereaction, int whitereaction, int grayreaction, int silverreaction, int defaultreaction, int consumerreaction, int plantreaction, int magentareaction, int pinkreaction, int coralreaction, int orangereaction, int barkreaction, int violetreaction, int virusreaction, int maroonreaction, int olivereaction, int mintreaction, int creamreaction, int spikereaction, int fallowreaction, int lightbluereaction, int ochrereaction, int skyreaction, int lilacreaction, int firereaction, int lightbrownreaction, int greenbrownreaction, int brownreaction, int icereaction, int brokenreaction, int sickreaction, int friendreaction);
/* A zeroed object for the world reader: deserialization runs no
 * constructor and no field initialiser. */
GeneticCode *gc_alloc_blank(void);
int gc_get_symmetry(GeneticCode *gc);
int gc_get_mirror(GeneticCode *gc);
int gc_get_mutationrate(GeneticCode *gc);
int gc_get_activity(GeneticCode *gc);
bool gc_get_update_clade(GeneticCode *gc);
bool gc_get_plague(GeneticCode *gc);
bool gc_get_disperse_children(GeneticCode *gc);
bool gc_get_generation_battle(GeneticCode *gc);
bool gc_get_sibling_battle(GeneticCode *gc);
bool gc_get_altruist(GeneticCode *gc);
bool gc_get_familial(GeneticCode *gc);
bool gc_get_social(GeneticCode *gc);
bool gc_get_peaceful(GeneticCode *gc);
bool gc_get_passive(GeneticCode *gc);
bool gc_get_clockwise(GeneticCode *gc);
bool gc_get_mimic_all(GeneticCode *gc);
bool gc_get_modifiespink(GeneticCode *gc);
int gc_get_modifiescream(GeneticCode *gc);
int gc_get_modifiesspore(GeneticCode *gc);
int gc_get_adaptspore(GeneticCode *gc);
bool gc_get_modifieslilac(GeneticCode *gc);
int gc_get_modifiesfallow(GeneticCode *gc);
bool gc_get_modifiessky(GeneticCode *gc);
bool gc_get_selfish(GeneticCode *gc);
int gc_get_generation(GeneticCode *gc);
char *gc_getclade_id(GeneticCode *gc);
Gene *gc_get_gene(GeneticCode *gc, int i);
int gc_get_n_genes(GeneticCode *gc);
void *gc_clone(GeneticCode *gc);
GeneticCode *gc_new_random(void);
GeneticCode *gc_new_from_genes(GeneList *genes, int symmetry, int mirror, int mutationrate, int activity, int modifiescream, int modifiesfallow, int modifiesspore, int adaptspore, bool plague, bool disperseChildren, bool generationBattle, bool siblingBattle, bool altruist, bool familial, bool social, bool peaceful, bool passive, bool clockwise, bool mimicall, bool modifiespink, bool modifieslilac, bool modifiessky, bool selfish);
GeneticCode *gc_new_mutated(GeneticCode *parentCode);

#endif
