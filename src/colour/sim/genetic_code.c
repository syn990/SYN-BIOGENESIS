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

#include "sim/genetic_code.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sim/params.h"

static void gc_random_mirror(GeneticCode *gc);
static void gc_random_symmetry(GeneticCode *gc);
static void gc_random_mutationrate(GeneticCode *gc);
static void gc_increase_mutationrate(GeneticCode *gc);
static void gc_decrease_mutationrate(GeneticCode *gc);
static void gc_random_activity(GeneticCode *gc);
static void gc_random_genes(GeneticCode *gc);
static void gc_random_plague(GeneticCode *gc);
static void gc_random_disperse_children(GeneticCode *gc);
static void gc_random_generation_battle(GeneticCode *gc);
static void gc_random_sibling_battle(GeneticCode *gc);
static void gc_random_altruist(GeneticCode *gc);
static void gc_random_familial(GeneticCode *gc);
static void gc_random_social(GeneticCode *gc);
static void gc_random_peaceful(GeneticCode *gc);
static void gc_random_passive(GeneticCode *gc);
static void gc_random_clockwise(GeneticCode *gc);
static void gc_random_mimic_all(GeneticCode *gc);
static void gc_random_modifiespink(GeneticCode *gc);
static void gc_random_modifiescream(GeneticCode *gc);
static void gc_random_modifiesspore(GeneticCode *gc);
static void gc_random_adaptspore(GeneticCode *gc);
static void gc_random_modifieslilac(GeneticCode *gc);
static void gc_random_modifiesfallow(GeneticCode *gc);
static void gc_random_modifiessky(GeneticCode *gc);
static void gc_random_selfish(GeneticCode *gc);

static void gene_finalize(void *obj);
static void gene_mark_refs(void *obj);
static const JGcKind gene_kind = {"Gene", gene_mark_refs, gene_finalize};

static void gene_finalize(void *obj)
{
    Gene *g = obj;
    (void)g;
}

static void gene_mark_refs(void *obj)
{
    Gene *g = obj;
    (void)g;
}

/* Object.clone: a field-by-field copy. Arrays are copied too, since
 * each belongs to one field; strings are shared, as in Java. */
static Gene *gene_shallow_clone(Gene *g)
{
    Gene *c = jgc_alloc(sizeof *c, &gene_kind);
    JGcHead h = c->gch;
    *c = *g;
    c->gch = h;
    return c;
}


Gene *gene_alloc_blank(void)
{
    return jgc_alloc(sizeof(Gene), &gene_kind);
}

static Gene *gene_alloc(void)
{
    Gene *g = jgc_alloc(sizeof *g, &gene_kind);
    return g;
}

void gene_randomize_color(Gene *g)
{
    int max_prob = P.red_prob + P.green_prob + P.blue_prob + P.cyan_prob + P.white_prob + P.gray_prob + P.yellow_prob + P.magenta_prob + P.pink_prob + P.coral_prob + P.orange_prob + P.forest_prob + P.spring_prob + P.leaf_prob + P.summer_prob + P.lime_prob + P.bark_prob + P.violet_prob + P.teal_prob + P.eye_prob + P.maroon_prob + P.olive_prob + P.mint_prob + P.cream_prob + P.rose_prob + P.dark_prob + P.ochre_prob + P.sky_prob + P.lilac_prob + P.silver_prob + P.fire_prob + P.darkgray_prob + P.gold_prob + P.blond_prob + P.flower_prob + P.auburn_prob + P.plague_prob + P.spike_prob + P.indigo_prob + P.lavender_prob + P.fallow_prob + P.spore_prob + P.jade_prob + P.c4_prob + P.grass_prob + P.purple_prob;
    int prob = jr_next_int(&g_rng, max_prob);
    int ac_prob = P.red_prob;
    if (prob < ac_prob) {
        g->color = JC_RED;
        return;
    }
    ac_prob += P.green_prob;
    if (prob < ac_prob) {
        g->color = JC_GREEN;
        return;
    }
    ac_prob += P.blue_prob;
    if (prob < ac_prob) {
        g->color = JC_BLUE;
        return;
    }
    ac_prob += P.cyan_prob;
    if (prob < ac_prob) {
        g->color = JC_CYAN;
        return;
    }
    ac_prob += P.white_prob;
    if (prob < ac_prob) {
        g->color = JC_WHITE;
        return;
    }
    ac_prob += P.gray_prob;
    if (prob < ac_prob) {
        g->color = JC_GRAY;
        return;
    }
    ac_prob += P.magenta_prob;
    if (prob < ac_prob) {
        g->color = JC_MAGENTA;
        return;
    }
    ac_prob += P.pink_prob;
    if (prob < ac_prob) {
        g->color = JC_PINK;
        return;
    }
    ac_prob += P.coral_prob;
    if (prob < ac_prob) {
        g->color = CM_CORAL;
        return;
    }
    ac_prob += P.orange_prob;
    if (prob < ac_prob) {
        g->color = JC_ORANGE;
        return;
    }
    ac_prob += P.forest_prob;
    if (prob < ac_prob) {
        g->color = CM_FOREST;
        return;
    }
    ac_prob += P.spring_prob;
    if (prob < ac_prob) {
        g->color = CM_SPRING;
        return;
    }
    ac_prob += P.leaf_prob;
    if (prob < ac_prob) {
        g->color = CM_LEAF;
        return;
    }
    ac_prob += P.summer_prob;
    if (prob < ac_prob) {
        g->color = CM_SUMMER;
        return;
    }
    ac_prob += P.lime_prob;
    if (prob < ac_prob) {
        g->color = CM_LIME;
        return;
    }
    ac_prob += P.bark_prob;
    if (prob < ac_prob) {
        g->color = CM_BARK;
        return;
    }
    ac_prob += P.violet_prob;
    if (prob < ac_prob) {
        g->color = CM_VIOLET;
        return;
    }
    ac_prob += P.teal_prob;
    if (prob < ac_prob) {
        g->color = CM_TEAL;
        return;
    }
    ac_prob += P.eye_prob;
    if (prob < ac_prob) {
        g->color = CM_EYE;
        return;
    }
    ac_prob += P.maroon_prob;
    if (prob < ac_prob) {
        g->color = CM_MAROON;
        return;
    }
    ac_prob += P.olive_prob;
    if (prob < ac_prob) {
        g->color = CM_OLIVE;
        return;
    }
    ac_prob += P.mint_prob;
    if (prob < ac_prob) {
        g->color = CM_MINT;
        return;
    }
    ac_prob += P.cream_prob;
    if (prob < ac_prob) {
        g->color = CM_CREAM;
        return;
    }
    ac_prob += P.rose_prob;
    if (prob < ac_prob) {
        g->color = CM_ROSE;
        return;
    }
    ac_prob += P.dark_prob;
    if (prob < ac_prob) {
        g->color = CM_DARK;
        return;
    }
    ac_prob += P.ochre_prob;
    if (prob < ac_prob) {
        g->color = CM_OCHRE;
        return;
    }
    ac_prob += P.sky_prob;
    if (prob < ac_prob) {
        g->color = CM_SKY;
        return;
    }
    ac_prob += P.lilac_prob;
    if (prob < ac_prob) {
        g->color = CM_LILAC;
        return;
    }
    ac_prob += P.silver_prob;
    if (prob < ac_prob) {
        g->color = JC_LIGHT_GRAY;
        return;
    }
    ac_prob += P.fire_prob;
    if (prob < ac_prob) {
        g->color = CM_FIRE;
        return;
    }
    ac_prob += P.darkgray_prob;
    if (prob < ac_prob) {
        g->color = JC_DARK_GRAY;
        return;
    }
    ac_prob += P.gold_prob;
    if (prob < ac_prob) {
        g->color = CM_GOLD;
        return;
    }
    ac_prob += P.blond_prob;
    if (prob < ac_prob) {
        g->color = CM_BLOND;
        return;
    }
    ac_prob += P.flower_prob;
    if (prob < ac_prob) {
        g->color = CM_FLOWER;
        return;
    }
    ac_prob += P.auburn_prob;
    if (prob < ac_prob) {
        g->color = CM_AUBURN;
        return;
    }
    ac_prob += P.plague_prob;
    if (prob < ac_prob) {
        g->color = CM_PLAGUE;
        return;
    }
    ac_prob += P.spike_prob;
    if (prob < ac_prob) {
        g->color = CM_SPIKE;
        return;
    }
    ac_prob += P.indigo_prob;
    if (prob < ac_prob) {
        g->color = CM_INDIGO;
        return;
    }
    ac_prob += P.lavender_prob;
    if (prob < ac_prob) {
        g->color = CM_LAVENDER;
        return;
    }
    ac_prob += P.fallow_prob;
    if (prob < ac_prob) {
        g->color = CM_FALLOW;
        return;
    }
    ac_prob += P.spore_prob;
    if (prob < ac_prob) {
        g->color = CM_SPORE;
        return;
    }
    ac_prob += P.jade_prob;
    if (prob < ac_prob) {
        g->color = CM_JADE;
        return;
    }
    ac_prob += P.c4_prob;
    if (prob < ac_prob) {
        g->color = CM_C4;
        return;
    }
    ac_prob += P.grass_prob;
    if (prob < ac_prob) {
        g->color = CM_GRASS;
        return;
    }
    ac_prob += P.purple_prob;
    if (prob < ac_prob) {
        g->color = CM_PURPLE;
        return;
    }
    g->color = JC_YELLOW;
}

void gene_randomizered_reaction(Gene *g)
{
    g->redreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizegreen_reaction(Gene *g)
{
    g->greenreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizeblue_reaction(Gene *g)
{
    g->bluereaction = jr_next_int(&g_rng, 6);
}

void gene_randomizeplague_reaction(Gene *g)
{
    g->plaguereaction = jr_next_int(&g_rng, 6);
}

void gene_randomizescourge_reaction(Gene *g)
{
    g->scourgereaction = jr_next_int(&g_rng, 6);
}

void gene_randomizewhite_reaction(Gene *g)
{
    g->whitereaction = jr_next_int(&g_rng, 6);
}

void gene_randomizegray_reaction(Gene *g)
{
    g->grayreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizesilver_reaction(Gene *g)
{
    g->silverreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizedefault_reaction(Gene *g)
{
    g->defaultreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizeconsumer_reaction(Gene *g)
{
    g->consumerreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizeplant_reaction(Gene *g)
{
    g->plantreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizemagenta_reaction(Gene *g)
{
    g->magentareaction = jr_next_int(&g_rng, 6);
}

void gene_randomizepink_reaction(Gene *g)
{
    g->pinkreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizecoral_reaction(Gene *g)
{
    g->coralreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizeorange_reaction(Gene *g)
{
    g->orangereaction = jr_next_int(&g_rng, 6);
}

void gene_randomizebark_reaction(Gene *g)
{
    g->barkreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizeviolet_reaction(Gene *g)
{
    g->violetreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizevirus_reaction(Gene *g)
{
    g->virusreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizemaroon_reaction(Gene *g)
{
    g->maroonreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizeolive_reaction(Gene *g)
{
    g->olivereaction = jr_next_int(&g_rng, 6);
}

void gene_randomizemint_reaction(Gene *g)
{
    g->mintreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizecream_reaction(Gene *g)
{
    g->creamreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizespike_reaction(Gene *g)
{
    g->spikereaction = jr_next_int(&g_rng, 6);
}

void gene_randomizefallow_reaction(Gene *g)
{
    g->fallowreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizelightblue_reaction(Gene *g)
{
    g->lightbluereaction = jr_next_int(&g_rng, 6);
}

void gene_randomizeochre_reaction(Gene *g)
{
    g->ochrereaction = jr_next_int(&g_rng, 6);
}

void gene_randomizesky_reaction(Gene *g)
{
    g->skyreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizelilac_reaction(Gene *g)
{
    g->lilacreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizefire_reaction(Gene *g)
{
    g->firereaction = jr_next_int(&g_rng, 6);
}

void gene_randomizelightbrown_reaction(Gene *g)
{
    g->lightbrownreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizegreenbrown_reaction(Gene *g)
{
    g->greenbrownreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizebrown_reaction(Gene *g)
{
    g->brownreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizeice_reaction(Gene *g)
{
    g->icereaction = jr_next_int(&g_rng, 6);
}

void gene_randomizebroken_reaction(Gene *g)
{
    g->brokenreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizesick_reaction(Gene *g)
{
    g->sickreaction = jr_next_int(&g_rng, 6);
}

void gene_randomizefriend_reaction(Gene *g)
{
    g->friendreaction = jr_next_int(&g_rng, 6);
}

void gene_randomize_length(Gene *g)
{
    g->length = 2.0 + jr_next_double(&g_rng) * 16.0;
}

void gene_randomize_theta(Gene *g)
{
    g->theta = jr_next_double(&g_rng) * 2.0 * M_PI;
}

/* Randomize the component of this gene: final point and color. Coordinates
 * are given a random number between -13 and -2 or 2 and 13. Color is given
 * a random color. The probability of each color is taken from user
 * preferences.
 */
void gene_randomize(Gene *g)
{
    gene_randomize_length(g);
    gene_randomize_theta(g);
    gene_randomize_color(g);
    gene_randomizered_reaction(g);
    gene_randomizegreen_reaction(g);
    gene_randomizeblue_reaction(g);
    gene_randomizeplague_reaction(g);
    gene_randomizescourge_reaction(g);
    gene_randomizewhite_reaction(g);
    gene_randomizegray_reaction(g);
    gene_randomizesilver_reaction(g);
    gene_randomizedefault_reaction(g);
    gene_randomizeconsumer_reaction(g);
    gene_randomizeplant_reaction(g);
    gene_randomizemagenta_reaction(g);
    gene_randomizepink_reaction(g);
    gene_randomizecoral_reaction(g);
    gene_randomizeorange_reaction(g);
    gene_randomizebark_reaction(g);
    gene_randomizeviolet_reaction(g);
    gene_randomizevirus_reaction(g);
    gene_randomizemaroon_reaction(g);
    gene_randomizeolive_reaction(g);
    gene_randomizemint_reaction(g);
    gene_randomizecream_reaction(g);
    gene_randomizespike_reaction(g);
    gene_randomizefallow_reaction(g);
    gene_randomizelightblue_reaction(g);
    gene_randomizeochre_reaction(g);
    gene_randomizesky_reaction(g);
    gene_randomizelilac_reaction(g);
    gene_randomizefire_reaction(g);
    gene_randomizelightbrown_reaction(g);
    gene_randomizegreenbrown_reaction(g);
    gene_randomizebrown_reaction(g);
    gene_randomizeice_reaction(g);
    gene_randomizebroken_reaction(g);
    gene_randomizesick_reaction(g);
    gene_randomizefriend_reaction(g);
}

/* Return an exact copy of this gene. */
void *gene_clone(Gene *g)
{
    Gene *newGen = NULL;
    newGen = gene_shallow_clone(g);
    return newGen;
}

double gene_get_length(Gene *g)
{
    return g->length;
}

double gene_get_theta(Gene *g)
{
    return g->theta;
}

/* Returns the segment's color.
 * 
 * @return the segment's color
 */
JColor gene_get_color(Gene *g)
{
    return g->color;
}

/* Returns the segment's branching point.
 * 
 * @return the segment's branching point
 */
int gene_get_branch(Gene *g)
{
    return g->branch;
}

/* Returns the segment's reactions.
 * 
 * @return the segment's reactions
 */
int gene_getred_reaction(Gene *g)
{
    return g->redreaction;
}

int gene_getgreen_reaction(Gene *g)
{
    return g->greenreaction;
}

int gene_getblue_reaction(Gene *g)
{
    return g->bluereaction;
}

int gene_getplague_reaction(Gene *g)
{
    return g->plaguereaction;
}

int gene_getscourge_reaction(Gene *g)
{
    return g->scourgereaction;
}

int gene_getwhite_reaction(Gene *g)
{
    return g->whitereaction;
}

int gene_getgray_reaction(Gene *g)
{
    return g->grayreaction;
}

int gene_getsilver_reaction(Gene *g)
{
    return g->silverreaction;
}

int gene_getdefault_reaction(Gene *g)
{
    return g->defaultreaction;
}

int gene_getconsumer_reaction(Gene *g)
{
    return g->consumerreaction;
}

int gene_getplant_reaction(Gene *g)
{
    return g->plantreaction;
}

int gene_getmagenta_reaction(Gene *g)
{
    return g->magentareaction;
}

int gene_getpink_reaction(Gene *g)
{
    return g->pinkreaction;
}

int gene_getcoral_reaction(Gene *g)
{
    return g->coralreaction;
}

int gene_getorange_reaction(Gene *g)
{
    return g->orangereaction;
}

int gene_getbark_reaction(Gene *g)
{
    return g->barkreaction;
}

int gene_getviolet_reaction(Gene *g)
{
    return g->violetreaction;
}

int gene_getvirus_reaction(Gene *g)
{
    return g->virusreaction;
}

int gene_getmaroon_reaction(Gene *g)
{
    return g->maroonreaction;
}

int gene_getolive_reaction(Gene *g)
{
    return g->olivereaction;
}

int gene_getmint_reaction(Gene *g)
{
    return g->mintreaction;
}

int gene_getcream_reaction(Gene *g)
{
    return g->creamreaction;
}

int gene_getspike_reaction(Gene *g)
{
    return g->spikereaction;
}

int gene_getfallow_reaction(Gene *g)
{
    return g->fallowreaction;
}

int gene_getlightblue_reaction(Gene *g)
{
    return g->lightbluereaction;
}

int gene_getochre_reaction(Gene *g)
{
    return g->ochrereaction;
}

int gene_getsky_reaction(Gene *g)
{
    return g->skyreaction;
}

int gene_getlilac_reaction(Gene *g)
{
    return g->lilacreaction;
}

int gene_getfire_reaction(Gene *g)
{
    return g->firereaction;
}

int gene_getlightbrown_reaction(Gene *g)
{
    return g->lightbrownreaction;
}

int gene_getgreenbrown_reaction(Gene *g)
{
    return g->greenbrownreaction;
}

int gene_getbrown_reaction(Gene *g)
{
    return g->brownreaction;
}

int gene_getice_reaction(Gene *g)
{
    return g->icereaction;
}

int gene_getbroken_reaction(Gene *g)
{
    return g->brokenreaction;
}

int gene_getsick_reaction(Gene *g)
{
    return g->sickreaction;
}

int gene_getfriend_reaction(Gene *g)
{
    return g->friendreaction;
}

/* Assign a color to the segment.
 * 
 * @param color
 *            The color to assign
 */
void gene_set_color(Gene *g, JColor color)
{
    g->color = color;
}

/* Assign a branching point to the segment.
 * 
 * @param branch
 *            The branching point to assign
 */
void gene_set_branch(Gene *g, int branch)
{
    g->branch = branch;
}

/* Assign a reaction to the segment.
 * 
 * @param reaction
 *            The reaction to assign
 */
void gene_setred_reaction(Gene *g, int redreaction)
{
    g->redreaction = redreaction;
}

void gene_setgreen_reaction(Gene *g, int greenreaction)
{
    g->greenreaction = greenreaction;
}

void gene_setblue_reaction(Gene *g, int bluereaction)
{
    g->bluereaction = bluereaction;
}

void gene_setplague_reaction(Gene *g, int plaguereaction)
{
    g->plaguereaction = plaguereaction;
}

void gene_setscourge_reaction(Gene *g, int scourgereaction)
{
    g->scourgereaction = scourgereaction;
}

void gene_setwhite_reaction(Gene *g, int whitereaction)
{
    g->whitereaction = whitereaction;
}

void gene_setgray_reaction(Gene *g, int grayreaction)
{
    g->grayreaction = grayreaction;
}

void gene_setsilver_reaction(Gene *g, int silverreaction)
{
    g->silverreaction = silverreaction;
}

void gene_setdefault_reaction(Gene *g, int defaultreaction)
{
    g->defaultreaction = defaultreaction;
}

void gene_setconsumer_reaction(Gene *g, int consumerreaction)
{
    g->consumerreaction = consumerreaction;
}

void gene_setplant_reaction(Gene *g, int plantreaction)
{
    g->plantreaction = plantreaction;
}

void gene_setmagenta_reaction(Gene *g, int magentareaction)
{
    g->magentareaction = magentareaction;
}

void gene_setpink_reaction(Gene *g, int pinkreaction)
{
    g->pinkreaction = pinkreaction;
}

void gene_setcoral_reaction(Gene *g, int coralreaction)
{
    g->coralreaction = coralreaction;
}

void gene_setorange_reaction(Gene *g, int orangereaction)
{
    g->orangereaction = orangereaction;
}

void gene_setbark_reaction(Gene *g, int barkreaction)
{
    g->barkreaction = barkreaction;
}

void gene_setviolet_reaction(Gene *g, int violetreaction)
{
    g->violetreaction = violetreaction;
}

void gene_setvirus_reaction(Gene *g, int virusreaction)
{
    g->virusreaction = virusreaction;
}

void gene_setmaroon_reaction(Gene *g, int maroonreaction)
{
    g->maroonreaction = maroonreaction;
}

void gene_setolive_reaction(Gene *g, int olivereaction)
{
    g->olivereaction = olivereaction;
}

void gene_setmint_reaction(Gene *g, int mintreaction)
{
    g->mintreaction = mintreaction;
}

void gene_setcream_reaction(Gene *g, int creamreaction)
{
    g->creamreaction = creamreaction;
}

void gene_setspike_reaction(Gene *g, int spikereaction)
{
    g->spikereaction = spikereaction;
}

void gene_setfallow_reaction(Gene *g, int fallowreaction)
{
    g->fallowreaction = fallowreaction;
}

void gene_setlightblue_reaction(Gene *g, int lightbluereaction)
{
    g->lightbluereaction = lightbluereaction;
}

void gene_setochre_reaction(Gene *g, int ochrereaction)
{
    g->ochrereaction = ochrereaction;
}

void gene_setsky_reaction(Gene *g, int skyreaction)
{
    g->skyreaction = skyreaction;
}

void gene_setlilac_reaction(Gene *g, int lilacreaction)
{
    g->lilacreaction = lilacreaction;
}

void gene_setfire_reaction(Gene *g, int firereaction)
{
    g->firereaction = firereaction;
}

void gene_setlightbrown_reaction(Gene *g, int lightbrownreaction)
{
    g->lightbrownreaction = lightbrownreaction;
}

void gene_setgreenbrown_reaction(Gene *g, int greenbrownreaction)
{
    g->greenbrownreaction = greenbrownreaction;
}

void gene_setbrown_reaction(Gene *g, int brownreaction)
{
    g->brownreaction = brownreaction;
}

void gene_setice_reaction(Gene *g, int icereaction)
{
    g->icereaction = icereaction;
}

void gene_setbroken_reaction(Gene *g, int brokenreaction)
{
    g->brokenreaction = brokenreaction;
}

void gene_setsick_reaction(Gene *g, int sickreaction)
{
    g->sickreaction = sickreaction;
}

void gene_setfriend_reaction(Gene *g, int friendreaction)
{
    g->friendreaction = friendreaction;
}

void gene_set_length(Gene *g, double length)
{
    g->length = length;
}

void gene_set_theta(Gene *g, double theta)
{
    g->theta = theta;
}

/* Void constructor. Creates the gene but leave it uninitialized. */
Gene *gene_new(void)
{
    Gene *g = gene_alloc();
    return g;
}

/* Creates a gene with the specified final point and color.
 * 
 * @param x
 *            x coordinate of the final point
 * @param y
 *            t coordinate of the final point
 * @param color
 *            segment's color
 * @param reaction
 *            segment's reaction
 */
Gene *gene_new_full(double length, double theta, JColor color, int branch, int redreaction, int greenreaction, int bluereaction, int plaguereaction, int scourgereaction, int whitereaction, int grayreaction, int silverreaction, int defaultreaction, int consumerreaction, int plantreaction, int magentareaction, int pinkreaction, int coralreaction, int orangereaction, int barkreaction, int violetreaction, int virusreaction, int maroonreaction, int olivereaction, int mintreaction, int creamreaction, int spikereaction, int fallowreaction, int lightbluereaction, int ochrereaction, int skyreaction, int lilacreaction, int firereaction, int lightbrownreaction, int greenbrownreaction, int brownreaction, int icereaction, int brokenreaction, int sickreaction, int friendreaction)
{
    Gene *g = gene_alloc();
    g->length = length;
    g->theta = theta;
    g->color = color;
    g->branch = branch;
    g->redreaction = redreaction;
    g->greenreaction = greenreaction;
    g->bluereaction = bluereaction;
    g->plaguereaction = plaguereaction;
    g->scourgereaction = scourgereaction;
    g->whitereaction = whitereaction;
    g->grayreaction = grayreaction;
    g->silverreaction = silverreaction;
    g->defaultreaction = defaultreaction;
    g->consumerreaction = consumerreaction;
    g->plantreaction = plantreaction;
    g->magentareaction = magentareaction;
    g->pinkreaction = pinkreaction;
    g->coralreaction = coralreaction;
    g->orangereaction = orangereaction;
    g->barkreaction = barkreaction;
    g->violetreaction = violetreaction;
    g->virusreaction = virusreaction;
    g->maroonreaction = maroonreaction;
    g->olivereaction = olivereaction;
    g->mintreaction = mintreaction;
    g->creamreaction = creamreaction;
    g->spikereaction = spikereaction;
    g->fallowreaction = fallowreaction;
    g->lightbluereaction = lightbluereaction;
    g->ochrereaction = ochrereaction;
    g->skyreaction = skyreaction;
    g->lilacreaction = lilacreaction;
    g->firereaction = firereaction;
    g->lightbrownreaction = lightbrownreaction;
    g->greenbrownreaction = greenbrownreaction;
    g->brownreaction = brownreaction;
    g->icereaction = icereaction;
    g->brokenreaction = brokenreaction;
    g->sickreaction = sickreaction;
    g->friendreaction = friendreaction;
    return g;
}

static void gc_finalize(void *obj);
static void gc_mark_refs(void *obj);
static const JGcKind gc_kind = {"GeneticCode", gc_mark_refs, gc_finalize};

static void gc_finalize(void *obj)
{
    GeneticCode *gc = obj;
    jarr_free(gc->genes);
}

static void gc_mark_refs(void *obj)
{
    GeneticCode *gc = obj;
    if (gc->genes)
        for (int i = 0; i < JLEN(gc->genes); i++)
            jgc_mark(gc->genes[i]);
    jstr_mark(gc->clade_id);
}

/* Object.clone: a field-by-field copy. Arrays are copied too, since
 * each belongs to one field; strings are shared, as in Java. */
static GeneticCode *gc_shallow_clone(GeneticCode *gc)
{
    GeneticCode *c = jgc_alloc(sizeof *c, &gc_kind);
    JGcHead h = c->gch;
    *c = *gc;
    c->gch = h;
    c->genes = jarr_dup(gc->genes);
    return c;
}


GeneticCode *gc_alloc_blank(void)
{
    return jgc_alloc(sizeof(GeneticCode), &gc_kind);
}

static GeneticCode *gc_alloc(void)
{
    GeneticCode *gc = jgc_alloc(sizeof *gc, &gc_kind);
    return gc;
}

/* Returns the symmetry applied to organisms with this genetic code
 * 
 * @return  a value of 1 - 8.
 */
int gc_get_symmetry(GeneticCode *gc)
{
    return gc->symmetry;
}

/* Returns if mirroring is applied to organisms with this genetic code
 * 
 * @return  0 if no mirroring is applied, 1 if mirroring is applied.
 */
int gc_get_mirror(GeneticCode *gc)
{
    return gc->mirror;
}

/* Returns the mutationrate applied to organisms with this genetic code
 * 
 * @return  a value of min mutationrate - max mutationrate.
 */
int gc_get_mutationrate(GeneticCode *gc)
{
    return gc->mutationrate;
}

/* Returns the season activity applied to organisms with this genetic code
 * 
 * @return  a value of 0 - 2.
 */
int gc_get_activity(GeneticCode *gc)
{
    return gc->activity;
}

/* Returns if the clade needs to be updated
 * 
 * @return  true or false.
 */
bool gc_get_update_clade(GeneticCode *gc)
{
    return gc->update_clade;
}

/* Returns the plague version applied to organisms with this genetic code
 * 
 * @return  true or false.
 */
bool gc_get_plague(GeneticCode *gc)
{
    return gc->plague;
}

/* Returns if organisms with this genetic code will disperse their children or not.
 * 
 * @return  true if the organism disperses its children, false otherwise.
 */
bool gc_get_disperse_children(GeneticCode *gc)
{
    return gc->disperse_children;
}

/* Returns if organisms with this genetic code will attack their parents and children.
 * 
 * @return  true if the organism attacks its parent and children, false otherwise.
 */
bool gc_get_generation_battle(GeneticCode *gc)
{
    return gc->generation_battle;
}

/* Returns if organisms with this genetic code will attack their siblings.
 * 
 * @return  true if the organism attacks its siblings, false otherwise.
 */
bool gc_get_sibling_battle(GeneticCode *gc)
{
    return gc->sibling_battle;
}

/* Returns if organisms with this genetic code will be altruistic or not.
 * 
 * @return  true if the organism is altruistic, false otherwise.
 */
bool gc_get_altruist(GeneticCode *gc)
{
    return gc->altruist;
}

/* Returns if organisms with this genetic code will be familial or not.
 * 
 * @return  true if the organism is familial, false otherwise.
 */
bool gc_get_familial(GeneticCode *gc)
{
    return gc->familial;
}

/* Returns if organisms with this genetic code will be social or not.
 * 
 * @return  true if the organism is social, false otherwise.
 */
bool gc_get_social(GeneticCode *gc)
{
    return gc->social;
}

/* Returns if organisms with this genetic code will be peaceful or not.
 * 
 * @return  true if the organism is peaceful, false otherwise.
 */
bool gc_get_peaceful(GeneticCode *gc)
{
    return gc->peaceful;
}

/* Returns if organisms with this genetic code will be passive or not.
 * 
 * @return  true if the organism is passive, false otherwise.
 */
bool gc_get_passive(GeneticCode *gc)
{
    return gc->passive;
}

/* Returns if organisms with this genetic code will turn clockwise or not.
 * 
 * @return  true if the organism turns clockwise, false otherwise.
 */
bool gc_get_clockwise(GeneticCode *gc)
{
    return gc->clockwise;
}

/* Returns if organisms with this genetic code will change the color of all dark segments or not.
 * 
 * @return  true if the organism changes all colors, false otherwise.
 */
bool gc_get_mimic_all(GeneticCode *gc)
{
    return gc->mimicall;
}

/* Returns if the function of pink is modified or not.
 * 
 * @return  true if the function of pink is modified, false otherwise.
 */
bool gc_get_modifiespink(GeneticCode *gc)
{
    return gc->modifiespink;
}

/* Returns the specialization of enhanced parasites
 * 
 * @return  a value of 1 - 3.
 */
int gc_get_modifiescream(GeneticCode *gc)
{
    return gc->modifiescream;
}

/* Returns the specialization of spores
 * 
 * @return  a value of 1 - 12.
 */
int gc_get_modifiesspore(GeneticCode *gc)
{
    return gc->modifiesspore;
}

/* Returns the further specialization of spores
 * 
 * @return  a value of 1 - 70.
 */
int gc_get_adaptspore(GeneticCode *gc)
{
    return gc->adaptspore;
}

/* Returns if the function of lilac is modified or not.
 * 
 * @return  true if the function of lilac is modified, false otherwise.
 */
bool gc_get_modifieslilac(GeneticCode *gc)
{
    return gc->modifieslilac;
}

/* Returns the specialization of fallow
 * 
 * @return  a value of 1 - 4.
 */
int gc_get_modifiesfallow(GeneticCode *gc)
{
    return gc->modifiesfallow;
}

/* Returns if the function of sky is modified or not.
 * 
 * @return  true if the function of sky is modified, false otherwise.
 */
bool gc_get_modifiessky(GeneticCode *gc)
{
    return gc->modifiessky;
}

/* Returns if organisms with this genetic code will always retain half of their energy if they reproduce or not.
 * 
 * @return  true if the organism retains half of its energy, false otherwise.
 */
bool gc_get_selfish(GeneticCode *gc)
{
    return gc->selfish;
}

/* Returns the generation number of this organism.
 * 
 * @return  The generation number of this organism.
 */
int gc_get_generation(GeneticCode *gc)
{
    return gc->generation;
}

/* Returns the identification number of this organisms clade.
 * 
 * @return  The identification number of this organisms clade.
 */
char *gc_getclade_id(GeneticCode *gc)
{
    return gc->clade_id;
}

/* Return a reference to a gene.
 * 
 * @param i  The index of the gene in the genetic code.
 * @return  A reference to the gene
 */
Gene *gc_get_gene(GeneticCode *gc, int i)
{
    return gc->genes[i];
}

/* Return the number of genes of this code
 * 
 * @return  The number of genes
 */
int gc_get_n_genes(GeneticCode *gc)
{
    return JLEN(gc->genes);
}

/* Gives mirror a random value (0 or 1) */
static void gc_random_mirror(GeneticCode *gc)
{
    gc->mirror = jr_next_int(&g_rng, 2);
}

/* Gives symmetry a random value (1 - 8) */
static void gc_random_symmetry(GeneticCode *gc)
{
    gc->symmetry = jr_next_int(&g_rng, 8) + 1;
}

/* Gives mutationrate a random value (min mutationrate - max mutationrate) */
static void gc_random_mutationrate(GeneticCode *gc)
{
    gc->mutationrate = jr_next_int(&g_rng, P.max_mutation_rate - P.min_mutation_rate + 1) + P.min_mutation_rate;
}

/* Gives mutationrate a random value (old mutationrate - max mutationrate) */
static void gc_increase_mutationrate(GeneticCode *gc)
{
    if (gc->mutationrate < P.max_mutation_rate) {
        gc->mutationrate = jr_next_int(&g_rng, P.max_mutation_rate - gc->mutationrate + 1) + gc->mutationrate;
    }
}

/* Gives mutationrate a random value (min mutationrate - old mutationrate) */
static void gc_decrease_mutationrate(GeneticCode *gc)
{
    if (gc->mutationrate > P.min_mutation_rate) {
        gc->mutationrate = jr_next_int(&g_rng, gc->mutationrate - P.min_mutation_rate + 1) + P.min_mutation_rate;
    }
}

/* Gives activity a random value (0 - 2) */
static void gc_random_activity(GeneticCode *gc)
{
    gc->activity = jr_next_int(&g_rng, 3);
}

/* Create a random genes array making sure that there will be more or equal than
 * 1 and less or equal than the initial complexity value.
 * It needs symmetry to have a valid value.
 */
static void gc_random_genes(GeneticCode *gc)
{
    int nSegments = (1 + jr_next_int(&g_rng, P.initial_complexity)) * gc->symmetry;
    // 1 - Initial complexity value
    if (nSegments % gc->symmetry != 0)
        nSegments += (gc->symmetry - (nSegments % gc->symmetry));
    int nGenes = nSegments / gc->symmetry;
    JARR_SET(gc->genes, JARR_NEW(Gene *, nGenes));
    for (int i = 0; i < nGenes; i++) {
        gc->genes[i] = gene_new();
        gene_randomize(gc->genes[i]);
        if (jr_next_boolean(&g_rng)) {
            gene_set_branch(gc->genes[i], -1);
        } else {
            gene_set_branch(gc->genes[i], jr_next_int(&g_rng, i + 1));
        }
    }
}

/* Gives plague version a random value (true or false) */
static void gc_random_plague(GeneticCode *gc)
{
    gc->plague = jr_next_boolean(&g_rng);
}

/* Decide randomly if organisms with this genetic code will try to
 * disperse their children or not.
 */
static void gc_random_disperse_children(GeneticCode *gc)
{
    gc->disperse_children = jr_next_boolean(&g_rng);
}

/* Decide randomly if organisms with this genetic code will
 * attack their parents and children or not.
 */
static void gc_random_generation_battle(GeneticCode *gc)
{
    gc->generation_battle = jr_next_boolean(&g_rng);
}

/* Decide randomly if organisms with this genetic code will
 * attack their siblings or not.
 */
static void gc_random_sibling_battle(GeneticCode *gc)
{
    gc->sibling_battle = jr_next_boolean(&g_rng);
}

/* Decide randomly if organisms with this genetic code will be
 * altruistic or not.
 */
static void gc_random_altruist(GeneticCode *gc)
{
    gc->altruist = jr_next_boolean(&g_rng);
}

/* Decide randomly if organisms with this genetic code will be
 * familial or not.
 */
static void gc_random_familial(GeneticCode *gc)
{
    gc->familial = jr_next_boolean(&g_rng);
}

/* Decide randomly if organisms with this genetic code will be
 * social or not.
 */
static void gc_random_social(GeneticCode *gc)
{
    gc->social = jr_next_boolean(&g_rng);
}

/* Decide randomly if organisms with this genetic code will be
 * peaceful or not.
 */
static void gc_random_peaceful(GeneticCode *gc)
{
    gc->peaceful = jr_next_boolean(&g_rng);
}

/* Decide randomly if organisms with this genetic code will be
 * passive or not.
 */
static void gc_random_passive(GeneticCode *gc)
{
    gc->passive = jr_next_boolean(&g_rng);
}

/* Decide randomly if organisms with this genetic code will turn
 * clockwise or not.
 */
static void gc_random_clockwise(GeneticCode *gc)
{
    gc->clockwise = jr_next_boolean(&g_rng);
}

/* Decide randomly if organisms with this genetic code will mimic
 * all dark segments or not.
 */
static void gc_random_mimic_all(GeneticCode *gc)
{
    gc->mimicall = jr_next_boolean(&g_rng);
}

/* Decide randomly if the function of pink is modified or not. */
static void gc_random_modifiespink(GeneticCode *gc)
{
    gc->modifiespink = jr_next_boolean(&g_rng);
}

/* Gives modifiescream a random value (1 - 3) */
static void gc_random_modifiescream(GeneticCode *gc)
{
    gc->modifiescream = jr_next_int(&g_rng, 3) + 1;
}

/* Gives modifiesspore a random value (1 - 12) */
static void gc_random_modifiesspore(GeneticCode *gc)
{
    gc->modifiesspore = jr_next_int(&g_rng, 12) + 1;
}

/* Gives adaptspore a random value (1 - 70) */
static void gc_random_adaptspore(GeneticCode *gc)
{
    gc->adaptspore = jr_next_int(&g_rng, 70) + 1;
}

/* Decide randomly if the function of lilac is modified or not. */
static void gc_random_modifieslilac(GeneticCode *gc)
{
    gc->modifieslilac = jr_next_boolean(&g_rng);
}

/* Gives modifiesfallow a random value (1 - 4) */
static void gc_random_modifiesfallow(GeneticCode *gc)
{
    gc->modifiesfallow = jr_next_int(&g_rng, 4) + 1;
}

/* Decide randomly if the function of sky is modified or not. */
static void gc_random_modifiessky(GeneticCode *gc)
{
    gc->modifiessky = jr_next_boolean(&g_rng);
}

/* Decide randomly if organisms with this genetic code will always retain half of their energy if they reproduce or not. */
static void gc_random_selfish(GeneticCode *gc)
{
    gc->selfish = jr_next_boolean(&g_rng);
}

void *gc_clone(GeneticCode *gc)
{
    GeneticCode *newCode = NULL;
    newCode = gc_shallow_clone(gc);
    JARR_SET(newCode->genes, JARR_NEW(Gene *, JLEN(gc->genes)));
    for (int i = 0; i < JLEN(gc->genes); i++)
        newCode->genes[i] = gene_clone(gc->genes[i]);
    return newCode;
}

/* Creates a new random genetic code. */
GeneticCode *gc_new_random(void)
{
    GeneticCode *gc = gc_alloc();
    gc_random_mirror(gc);
    gc_random_symmetry(gc);
    gc_random_genes(gc);
    gc_random_mutationrate(gc);
    gc_random_activity(gc);
    gc_random_modifiescream(gc);
    gc_random_modifiesfallow(gc);
    gc_random_modifiesspore(gc);
    gc_random_adaptspore(gc);
    gc_random_plague(gc);
    gc_random_disperse_children(gc);
    gc_random_generation_battle(gc);
    gc_random_sibling_battle(gc);
    gc_random_altruist(gc);
    gc_random_familial(gc);
    gc_random_social(gc);
    gc_random_peaceful(gc);
    gc_random_passive(gc);
    gc_random_clockwise(gc);
    gc_random_mimic_all(gc);
    gc_random_modifiespink(gc);
    gc_random_modifieslilac(gc);
    gc_random_modifiessky(gc);
    gc_random_selfish(gc);
    return gc;
}

/* Creates a genetic code given its content.
 * No check about the validity of the information is done.
 * 
 * @param genes  A list containing the genes
 * @param symmetry  The symmetry that an organism with this genetic code will have.
 * @param mirror  0 if the organism won't be mirrored, 1 if it will.
 * @param mutationrate  The mutationrate that an organism with this genetic code will have.
 * @param activity  The summer activity that an organism with this genetic code will have.
 * @param modifiescream The specialization of an enhanced parasite.
 * @param modifiesfallow The specialization of fallow.
 * @param modifiesspore The specialization of spores.
 * @param adaptspore The further specialization of spores.
 * @param plague  The plague version that an organism with this genetic code will have.
 * @param disperseChildren  true if the organism will disperse its children.
 * @param generationBattle  true if the organism attacks its parent and children.
 * @param siblingBattle  true if the organism attacks its siblings.
 * @param altruist  true if the organism is an altruist.
 * @param familial  true if the organism is familial.
 * @param social  true if the organism is social.
 * @param peaceful  true if the organism is peaceful.
 * @param passive  true if the organism is passive.
 * @param clockwise  true if the organism turns clockwise.
 * @param mimicall  true if the organism mimics all dark segments.
 * @param modifiespink  true if the function of pink is modified.
 * @param modifieslilac  true if the function of lilac is modified.
 * @param modifiessky  true if the function of sky is modified.
 * @param selfish  true if the organism retains half of its energy if it reproduces.
 */
GeneticCode *gc_new_from_genes(GeneList *genes, int symmetry, int mirror, int mutationrate, int activity, int modifiescream, int modifiesfallow, int modifiesspore, int adaptspore, bool plague, bool disperseChildren, bool generationBattle, bool siblingBattle, bool altruist, bool familial, bool social, bool peaceful, bool passive, bool clockwise, bool mimicall, bool modifiespink, bool modifieslilac, bool modifiessky, bool selfish)
{
    GeneticCode *gc = gc_alloc();
    int nGenes = genes->n;
    JARR_SET(gc->genes, JARR_NEW(Gene *, nGenes));
    gene_list_to_array(genes, gc->genes);
    gc->mirror = mirror;
    gc->symmetry = symmetry;
    gc->mutationrate = mutationrate;
    gc->activity = activity;
    gc->modifiescream = modifiescream;
    gc->modifiesfallow = modifiesfallow;
    gc->modifiesspore = modifiesspore;
    gc->adaptspore = adaptspore;
    gc->plague = plague;
    gc->disperse_children = disperseChildren;
    gc->generation_battle = generationBattle;
    gc->sibling_battle = siblingBattle;
    gc->altruist = altruist;
    gc->familial = familial;
    gc->social = social;
    gc->peaceful = peaceful;
    gc->passive = passive;
    gc->clockwise = clockwise;
    gc->mimicall = mimicall;
    gc->modifiespink = modifiespink;
    gc->modifieslilac = modifieslilac;
    gc->modifiessky = modifiessky;
    gc->selfish = selfish;
    return gc;
}

/* Creates a new genetic code based on the father genetic code but
 * applying random mutations to it.
 * 
 * @param parentCode  The genetic code that this code will be based on.
 */
GeneticCode *gc_new_mutated(GeneticCode *parentCode)
{
    GeneticCode *gc = gc_alloc();
    int i = 0;
    int j = 0;
    int addedGene = -1;
    int removedGene = -1;
    int clonedGene = 0;
    int nGenes = 0;
    bool randomLength = false;
    bool randomTheta = false;
    bool randomBranch = false;
    bool repairBranch = false;
    bool randomredReaction = false;
    bool randomgreenReaction = false;
    bool randomblueReaction = false;
    bool randomplagueReaction = false;
    bool randomscourgeReaction = false;
    bool randomwhiteReaction = false;
    bool randomgrayReaction = false;
    bool randomsilverReaction = false;
    bool randomdefaultReaction = false;
    bool randomconsumerReaction = false;
    bool randomplantReaction = false;
    bool randommagentaReaction = false;
    bool randompinkReaction = false;
    bool randomcoralReaction = false;
    bool randomorangeReaction = false;
    bool randombarkReaction = false;
    bool randomvioletReaction = false;
    bool randomvirusReaction = false;
    bool randommaroonReaction = false;
    bool randomoliveReaction = false;
    bool randommintReaction = false;
    bool randomcreamReaction = false;
    bool randomspikeReaction = false;
    bool randomfallowReaction = false;
    bool randomlightblueReaction = false;
    bool randomochreReaction = false;
    bool randomskyReaction = false;
    bool randomlilacReaction = false;
    bool randomfireReaction = false;
    bool randomlightbrownReaction = false;
    bool randomgreenbrownReaction = false;
    bool randombrownReaction = false;
    bool randomiceReaction = false;
    bool randombrokenReaction = false;
    bool randomsickReaction = false;
    bool randomfriendReaction = false;
    bool randomColor = false;
    gc->mutationrate = gc_get_mutationrate(parentCode);
    if (bio_random_mutation()) {
        // mutate mutation rate
        if (jr_next_int(&g_rng, 10) < 1) {
            // large change
            if (jr_next_boolean(&g_rng)) {
                // increase
                gc_increase_mutationrate(gc);
            } else {
                // decrease
                gc_decrease_mutationrate(gc);
            }
        } else {
            // small change
            if (jr_next_boolean(&g_rng)) {
                // increase
                gc->mutationrate = gc->mutationrate + jr_next_int(&g_rng, 2) + 1;
            } else {
                // decrease
                gc->mutationrate = gc->mutationrate - jr_next_int(&g_rng, 2) - 1;
            }
        }
    }
    // check if organism is inside allowed mutation rates
    if (gc->mutationrate > P.max_mutation_rate) {
        gc->mutationrate = P.max_mutation_rate;
    } else {
        if (gc->mutationrate < P.min_mutation_rate) {
            gc->mutationrate = P.min_mutation_rate;
        }
    }
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
        gc_random_mirror(gc);
    else
        gc->mirror = gc_get_mirror(parentCode);
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate) {
        // change symmetry
        gc_random_symmetry(gc);
        nGenes = gc_get_n_genes(parentCode);
        gc->update_clade = true;
    } else {
        // keep symmetry
        gc->symmetry = gc_get_symmetry(parentCode);
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate) {
            // change number of segments
            if (jr_next_boolean(&g_rng)) {
                // increase segments
                if (gc_get_n_genes(parentCode) >= 100)
                    nGenes = gc_get_n_genes(parentCode);
                else {
                    nGenes = gc_get_n_genes(parentCode) + 1;
                    addedGene = jr_next_int(&g_rng, nGenes);
                    gc->update_clade = true;
                }
            } else {
                // decrease segments
                if (gc_get_n_genes(parentCode) <= 1)
                    nGenes = gc_get_n_genes(parentCode);
                else {
                    nGenes = gc_get_n_genes(parentCode) - 1;
                    removedGene = jr_next_int(&g_rng, gc_get_n_genes(parentCode));
                    gc->update_clade = true;
                    repairBranch = true;
                }
            }
        } else {
            // keep number of segments
            nGenes = gc_get_n_genes(parentCode);
        }
    }
    // Create genes
    JARR_SET(gc->genes, JARR_NEW(Gene *, nGenes));
    for (i = 0, j = 0; i < nGenes; i++, j++) {
        if (removedGene == j) {
            i--;
            continue;
        }
        if (addedGene == i) {
            if (jr_next_int(&g_rng, 8) < 1) {
                // Clone the gene after or before itself, and always randomize rotation
                if (i == 0) {
                    clonedGene = -1;
                } else {
                    if (i < nGenes - 1) {
                        if (jr_next_boolean(&g_rng)) {
                            clonedGene = -1;
                        } else {
                            clonedGene = 1;
                            j--;
                        }
                    } else {
                        clonedGene = 1;
                        j--;
                    }
                }
            } else {
                // Create a new gene
                gc->genes[i] = gene_new();
                gene_randomize(gc->genes[i]);
                if (jr_next_boolean(&g_rng)) {
                    gene_set_branch(gc->genes[i], -1);
                } else {
                    gene_set_branch(gc->genes[i], jr_next_int(&g_rng, i + 1));
                }
                j--;
                continue;
            }
        }
        randomLength = randomTheta = randomBranch = randomredReaction = randomgreenReaction = randomblueReaction = randomplagueReaction = randomscourgeReaction = randomwhiteReaction = randomgrayReaction = randomsilverReaction = randomdefaultReaction = randomconsumerReaction = randomplantReaction = randommagentaReaction = randompinkReaction = randomcoralReaction = randomorangeReaction = randombarkReaction = randomvioletReaction = randomvirusReaction = randommaroonReaction = randomoliveReaction = randommintReaction = randomcreamReaction = randomspikeReaction = randomfallowReaction = randomlightblueReaction = randomochreReaction = randomskyReaction = randomlilacReaction = randomfireReaction = randomlightbrownReaction = randomgreenbrownReaction = randombrownReaction = randomiceReaction = randombrokenReaction = randomsickReaction = randomfriendReaction = randomColor = false;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomLength = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomTheta = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomBranch = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomredReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomgreenReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomblueReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomplagueReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomscourgeReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomwhiteReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomgrayReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomsilverReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomdefaultReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomconsumerReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomplantReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randommagentaReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randompinkReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomcoralReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomorangeReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randombarkReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomvioletReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomvirusReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randommaroonReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomoliveReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randommintReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomcreamReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomspikeReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomfallowReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomlightblueReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomochreReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomskyReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomlilacReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomfireReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomlightbrownReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomgreenbrownReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randombrownReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomiceReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randombrokenReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomsickReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomfriendReaction = true;
        if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
            randomColor = true;
        if (randomLength || randomTheta || randomBranch || repairBranch || randomredReaction || randomgreenReaction || randomblueReaction || randomplagueReaction || randomscourgeReaction || randomwhiteReaction || randomgrayReaction || randomsilverReaction || randomdefaultReaction || randomconsumerReaction || randomplantReaction || randommagentaReaction || randompinkReaction || randomcoralReaction || randomorangeReaction || randombarkReaction || randomvioletReaction || randomvirusReaction || randommaroonReaction || randomoliveReaction || randommintReaction || randomcreamReaction || randomspikeReaction || randomfallowReaction || randomlightblueReaction || randomochreReaction || randomskyReaction || randomlilacReaction || randomfireReaction || randomlightbrownReaction || randomgreenbrownReaction || randombrownReaction || randomiceReaction || randombrokenReaction || randomsickReaction || randomfriendReaction || randomColor || clonedGene != 0) {
            gc->genes[i] = gene_new();
            if (clonedGene != 0) {
                if ((jr_next_boolean(&g_rng)) || (randomLength)) {
                    gene_randomize_length(gc->genes[i]);
                } else {
                    gene_set_length(gc->genes[i], gene_get_length(gc_get_gene(parentCode, j)));
                }
                gene_randomize_theta(gc->genes[i]);
                if ((jr_next_int(&g_rng, 5) < 1) || (randomBranch)) {
                    if (jr_next_boolean(&g_rng)) {
                        gene_set_branch(gc->genes[i], -1);
                    } else {
                        gene_set_branch(gc->genes[i], jr_next_int(&g_rng, i + 1));
                    }
                } else {
                    gene_set_branch(gc->genes[i], gene_get_branch(gc_get_gene(parentCode, j)));
                    if (clonedGene == 1) {
                        if (gene_get_branch(gc->genes[i]) == -1) {
                            if (jr_next_boolean(&g_rng)) {
                                gene_set_branch(gc->genes[i], i - 1);
                            } else {
                                if (jr_next_boolean(&g_rng)) {
                                    gene_set_branch(gc->genes[i], i);
                                }
                            }
                        } else {
                            if (jr_next_boolean(&g_rng)) {
                                if (jr_next_boolean(&g_rng)) {
                                    gene_set_branch(gc->genes[i], -1);
                                } else {
                                    gene_set_branch(gc->genes[i], i);
                                }
                            }
                        }
                    }
                }
            } else {
                if (randomLength)
                    gene_randomize_length(gc->genes[i]);
                else
                    gene_set_length(gc->genes[i], gene_get_length(gc_get_gene(parentCode, j)));
                if (randomTheta)
                    gene_randomize_theta(gc->genes[i]);
                else
                    gene_set_theta(gc->genes[i], gene_get_theta(gc_get_gene(parentCode, j)));
                if (randomBranch) {
                    if (jr_next_boolean(&g_rng)) {
                        gene_set_branch(gc->genes[i], -1);
                    } else {
                        gene_set_branch(gc->genes[i], jr_next_int(&g_rng, i + 1));
                    }
                } else {
                    gene_set_branch(gc->genes[i], gene_get_branch(gc_get_gene(parentCode, j)));
                    if ((repairBranch) && (gene_get_branch(gc->genes[i]) > i)) {
                        gene_set_branch(gc->genes[i], i);
                    }
                }
            }
            if (randomredReaction)
                gene_randomizered_reaction(gc->genes[i]);
            else
                gene_setred_reaction(gc->genes[i], gene_getred_reaction(gc_get_gene(parentCode, j)));
            if (randomgreenReaction)
                gene_randomizegreen_reaction(gc->genes[i]);
            else
                gene_setgreen_reaction(gc->genes[i], gene_getgreen_reaction(gc_get_gene(parentCode, j)));
            if (randomblueReaction)
                gene_randomizeblue_reaction(gc->genes[i]);
            else
                gene_setblue_reaction(gc->genes[i], gene_getblue_reaction(gc_get_gene(parentCode, j)));
            if (randomplagueReaction)
                gene_randomizeplague_reaction(gc->genes[i]);
            else
                gene_setplague_reaction(gc->genes[i], gene_getplague_reaction(gc_get_gene(parentCode, j)));
            if (randomscourgeReaction)
                gene_randomizescourge_reaction(gc->genes[i]);
            else
                gene_setscourge_reaction(gc->genes[i], gene_getscourge_reaction(gc_get_gene(parentCode, j)));
            if (randomwhiteReaction)
                gene_randomizewhite_reaction(gc->genes[i]);
            else
                gene_setwhite_reaction(gc->genes[i], gene_getwhite_reaction(gc_get_gene(parentCode, j)));
            if (randomgrayReaction)
                gene_randomizegray_reaction(gc->genes[i]);
            else
                gene_setgray_reaction(gc->genes[i], gene_getgray_reaction(gc_get_gene(parentCode, j)));
            if (randomsilverReaction)
                gene_randomizesilver_reaction(gc->genes[i]);
            else
                gene_setsilver_reaction(gc->genes[i], gene_getsilver_reaction(gc_get_gene(parentCode, j)));
            if (randomdefaultReaction)
                gene_randomizedefault_reaction(gc->genes[i]);
            else
                gene_setdefault_reaction(gc->genes[i], gene_getdefault_reaction(gc_get_gene(parentCode, j)));
            if (randomconsumerReaction)
                gene_randomizeconsumer_reaction(gc->genes[i]);
            else
                gene_setconsumer_reaction(gc->genes[i], gene_getconsumer_reaction(gc_get_gene(parentCode, j)));
            if (randomplantReaction)
                gene_randomizeplant_reaction(gc->genes[i]);
            else
                gene_setplant_reaction(gc->genes[i], gene_getplant_reaction(gc_get_gene(parentCode, j)));
            if (randommagentaReaction)
                gene_randomizemagenta_reaction(gc->genes[i]);
            else
                gene_setmagenta_reaction(gc->genes[i], gene_getmagenta_reaction(gc_get_gene(parentCode, j)));
            if (randompinkReaction)
                gene_randomizepink_reaction(gc->genes[i]);
            else
                gene_setpink_reaction(gc->genes[i], gene_getpink_reaction(gc_get_gene(parentCode, j)));
            if (randomcoralReaction)
                gene_randomizecoral_reaction(gc->genes[i]);
            else
                gene_setcoral_reaction(gc->genes[i], gene_getcoral_reaction(gc_get_gene(parentCode, j)));
            if (randomorangeReaction)
                gene_randomizeorange_reaction(gc->genes[i]);
            else
                gene_setorange_reaction(gc->genes[i], gene_getorange_reaction(gc_get_gene(parentCode, j)));
            if (randombarkReaction)
                gene_randomizebark_reaction(gc->genes[i]);
            else
                gene_setbark_reaction(gc->genes[i], gene_getbark_reaction(gc_get_gene(parentCode, j)));
            if (randomvioletReaction)
                gene_randomizeviolet_reaction(gc->genes[i]);
            else
                gene_setviolet_reaction(gc->genes[i], gene_getviolet_reaction(gc_get_gene(parentCode, j)));
            if (randomvirusReaction)
                gene_randomizevirus_reaction(gc->genes[i]);
            else
                gene_setvirus_reaction(gc->genes[i], gene_getvirus_reaction(gc_get_gene(parentCode, j)));
            if (randommaroonReaction)
                gene_randomizemaroon_reaction(gc->genes[i]);
            else
                gene_setmaroon_reaction(gc->genes[i], gene_getmaroon_reaction(gc_get_gene(parentCode, j)));
            if (randomoliveReaction)
                gene_randomizeolive_reaction(gc->genes[i]);
            else
                gene_setolive_reaction(gc->genes[i], gene_getolive_reaction(gc_get_gene(parentCode, j)));
            if (randommintReaction)
                gene_randomizemint_reaction(gc->genes[i]);
            else
                gene_setmint_reaction(gc->genes[i], gene_getmint_reaction(gc_get_gene(parentCode, j)));
            if (randomcreamReaction)
                gene_randomizecream_reaction(gc->genes[i]);
            else
                gene_setcream_reaction(gc->genes[i], gene_getcream_reaction(gc_get_gene(parentCode, j)));
            if (randomspikeReaction)
                gene_randomizespike_reaction(gc->genes[i]);
            else
                gene_setspike_reaction(gc->genes[i], gene_getspike_reaction(gc_get_gene(parentCode, j)));
            if (randomfallowReaction)
                gene_randomizefallow_reaction(gc->genes[i]);
            else
                gene_setfallow_reaction(gc->genes[i], gene_getfallow_reaction(gc_get_gene(parentCode, j)));
            if (randomlightblueReaction)
                gene_randomizelightblue_reaction(gc->genes[i]);
            else
                gene_setlightblue_reaction(gc->genes[i], gene_getlightblue_reaction(gc_get_gene(parentCode, j)));
            if (randomochreReaction)
                gene_randomizeochre_reaction(gc->genes[i]);
            else
                gene_setochre_reaction(gc->genes[i], gene_getochre_reaction(gc_get_gene(parentCode, j)));
            if (randomskyReaction)
                gene_randomizesky_reaction(gc->genes[i]);
            else
                gene_setsky_reaction(gc->genes[i], gene_getsky_reaction(gc_get_gene(parentCode, j)));
            if (randomlilacReaction)
                gene_randomizelilac_reaction(gc->genes[i]);
            else
                gene_setlilac_reaction(gc->genes[i], gene_getlilac_reaction(gc_get_gene(parentCode, j)));
            if (randomfireReaction)
                gene_randomizefire_reaction(gc->genes[i]);
            else
                gene_setfire_reaction(gc->genes[i], gene_getfire_reaction(gc_get_gene(parentCode, j)));
            if (randomlightbrownReaction)
                gene_randomizelightbrown_reaction(gc->genes[i]);
            else
                gene_setlightbrown_reaction(gc->genes[i], gene_getlightbrown_reaction(gc_get_gene(parentCode, j)));
            if (randomgreenbrownReaction)
                gene_randomizegreenbrown_reaction(gc->genes[i]);
            else
                gene_setgreenbrown_reaction(gc->genes[i], gene_getgreenbrown_reaction(gc_get_gene(parentCode, j)));
            if (randombrownReaction)
                gene_randomizebrown_reaction(gc->genes[i]);
            else
                gene_setbrown_reaction(gc->genes[i], gene_getbrown_reaction(gc_get_gene(parentCode, j)));
            if (randomiceReaction)
                gene_randomizeice_reaction(gc->genes[i]);
            else
                gene_setice_reaction(gc->genes[i], gene_getice_reaction(gc_get_gene(parentCode, j)));
            if (randombrokenReaction)
                gene_randomizebroken_reaction(gc->genes[i]);
            else
                gene_setbroken_reaction(gc->genes[i], gene_getbroken_reaction(gc_get_gene(parentCode, j)));
            if (randomsickReaction)
                gene_randomizesick_reaction(gc->genes[i]);
            else
                gene_setsick_reaction(gc->genes[i], gene_getsick_reaction(gc_get_gene(parentCode, j)));
            if (randomfriendReaction)
                gene_randomizefriend_reaction(gc->genes[i]);
            else
                gene_setfriend_reaction(gc->genes[i], gene_getfriend_reaction(gc_get_gene(parentCode, j)));
            if (randomColor)
                gene_randomize_color(gc->genes[i]);
            else
                gene_set_color(gc->genes[i], gene_get_color(gc_get_gene(parentCode, j)));
            if (clonedGene != 0) {
                if (clonedGene == -1) {
                    j--;
                }
                clonedGene = 0;
            }
        } else {
            gc->genes[i] = gc_get_gene(parentCode, j);
        }
    }
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
        gc_random_activity(gc);
    else
        gc->activity = gc_get_activity(parentCode);
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
        gc_random_modifiescream(gc);
    else
        gc->modifiescream = gc_get_modifiescream(parentCode);
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
        gc_random_modifiesfallow(gc);
    else
        gc->modifiesfallow = gc_get_modifiesfallow(parentCode);
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
        gc_random_modifiesspore(gc);
    else
        gc->modifiesspore = gc_get_modifiesspore(parentCode);
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
        gc_random_adaptspore(gc);
    else
        gc->adaptspore = gc_get_adaptspore(parentCode);
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
        gc_random_plague(gc);
    else
        gc->plague = gc_get_plague(parentCode);
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
        gc_random_disperse_children(gc);
    else
        gc->disperse_children = gc_get_disperse_children(parentCode);
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
        gc_random_generation_battle(gc);
    else
        gc->generation_battle = gc_get_generation_battle(parentCode);
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
        gc_random_sibling_battle(gc);
    else
        gc->sibling_battle = gc_get_sibling_battle(parentCode);
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
        gc_random_altruist(gc);
    else
        gc->altruist = gc_get_altruist(parentCode);
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
        gc_random_familial(gc);
    else
        gc->familial = gc_get_familial(parentCode);
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
        gc_random_social(gc);
    else
        gc->social = gc_get_social(parentCode);
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
        gc_random_peaceful(gc);
    else
        gc->peaceful = gc_get_peaceful(parentCode);
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
        gc_random_passive(gc);
    else
        gc->passive = gc_get_passive(parentCode);
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
        gc_random_clockwise(gc);
    else
        gc->clockwise = gc_get_clockwise(parentCode);
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
        gc_random_mimic_all(gc);
    else
        gc->mimicall = gc_get_mimic_all(parentCode);
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
        gc_random_modifiespink(gc);
    else
        gc->modifiespink = gc_get_modifiespink(parentCode);
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
        gc_random_modifieslilac(gc);
    else
        gc->modifieslilac = gc_get_modifieslilac(parentCode);
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
        gc_random_modifiessky(gc);
    else
        gc->modifiessky = gc_get_modifiessky(parentCode);
    if (jr_next_int(&g_rng, 10000) < gc->mutationrate)
        gc_random_selfish(gc);
    else
        gc->selfish = gc_get_selfish(parentCode);
    // Add 1 to the generation number
    gc->generation = gc_get_generation(parentCode) + 1;
    // Add the clade identification
    gc->clade_id = gc_getclade_id(parentCode);
    return gc;
}

Gene **gene_list_to_array(const GeneList *l, Gene **a)
{
    /* List.toArray(T[] a), for an array of the list's size */
    for (int i = 0; i < l->n && i < JLEN(a); i++)
        a[i] = l->items[i];
    return a;
}
