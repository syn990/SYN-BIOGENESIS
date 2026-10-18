/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Part of the SYN edition of SYN-BIOGENESIS, a C port of Biogenesis
 * (https://biogenesis.sourceforge.net/).
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

#include "syn/palette.h"

#include <stdlib.h>
#include <string.h>

#include "sim/params.h"
#include "syn/family.h"

#ifdef BIO_COLOUR

/* Color Mod's colours by job, as its README describes them, then the
 * colours segments turn for a while, from Organism.java. */
static const struct {
    JColor c;
    const char *name;
    Role role;
} k_slots[] = {
    /* photosynthesis, and methanotrophy */
    {JC_GREEN, "green", ROLE_PHOTO},
    {CM_FOREST, "forest", ROLE_PHOTO},
    {CM_SPRING, "spring", ROLE_PHOTO},
    {CM_SUMMER, "summer", ROLE_PHOTO},
    {CM_LIME, "lime", ROLE_PHOTO},
    {CM_LEAF, "leaf", ROLE_PHOTO},
    {CM_C4, "c4plant", ROLE_PHOTO},
    {CM_JADE, "jade", ROLE_PHOTO},
    {CM_GRASS, "grass", ROLE_PHOTO},
    {CM_BARK, "bark", ROLE_PHOTO},
    {CM_PURPLE, "purple", ROLE_PHOTO},
    /* consumers */
    {JC_RED, "red", ROLE_EAT},
    {CM_FIRE, "fire", ROLE_EAT},
    {JC_ORANGE, "orange", ROLE_EAT},
    {CM_MAROON, "maroon", ROLE_EAT},
    {JC_PINK, "pink", ROLE_EAT},
    {CM_CREAM, "cream", ROLE_EAT},
    /* killers and weakeners */
    {CM_SPIKE, "spike", ROLE_KILL},
    {CM_LILAC, "lilac", ROLE_KILL},
    {JC_GRAY, "gray", ROLE_KILL},
    {CM_VIOLET, "violet", ROLE_KILL},
    {CM_OLIVE, "olive", ROLE_KILL},
    {CM_FALLOW, "fallow", ROLE_KILL},
    /* defence */
    {CM_SKY, "sky", ROLE_DEFEND},
    {JC_BLUE, "blue", ROLE_DEFEND},
    {CM_OCHRE, "ochre", ROLE_DEFEND},
    /* infection */
    {JC_WHITE, "white", ROLE_INFECT},
    {CM_PLAGUE, "plague", ROLE_INFECT},
    /* healing, immunity, friends */
    {CM_CORAL, "coral", ROLE_HEAL},
    {CM_MINT, "mint", ROLE_HEAL},
    {CM_LAVENDER, "lavender", ROLE_HEAL},
    {JC_MAGENTA, "magenta", ROLE_HEAL},
    {CM_ROSE, "rose", ROLE_HEAL},
    {CM_INDIGO, "indigo", ROLE_HEAL},
    /* movement */
    {JC_CYAN, "cyan", ROLE_MOVE},
    {CM_TEAL, "teal", ROLE_MOVE},
    /* reproduction */
    {JC_YELLOW, "yellow", ROLE_REPRO},
    {CM_AUBURN, "auburn", ROLE_REPRO},
    {CM_BLOND, "blond", ROLE_REPRO},
    {CM_FLOWER, "flower", ROLE_REPRO},
    {CM_SPORE, "spore", ROLE_REPRO},
    /* eyes */
    {CM_EYE, "eye", ROLE_SENSE},
    /* the rest */
    {JC_LIGHT_GRAY, "silver", ROLE_OTHER},
    {CM_GOLD, "gold", ROLE_OTHER},
    {JC_DARK_GRAY, "dark gray", ROLE_OTHER},
    {CM_DARK, "black", ROLE_OTHER},
    /* what segments turn into for a while */
    {CM_VISION, "vision", ROLE_SENSE},
    {CM_FRUIT, "fruit", ROLE_REPRO},
    {CM_DEEPSKY, "deep sky", ROLE_DEFEND},
    {CM_SPIKEPOINT, "spike point", ROLE_KILL},
    {CM_LIGHTBROWN, "withered", ROLE_INACTIVE},
    {CM_GREENBROWN, "poisoned", ROLE_INACTIVE},
    {CM_BROKEN, "broken", ROLE_INACTIVE},
    {CM_ICE, "ice", ROLE_INACTIVE},
    {CM_LIGHT_BLUE, "numb", ROLE_INACTIVE},
    {CM_WINTER, "winter", ROLE_INACTIVE},
    {CM_DARKGREEN, "dark green", ROLE_INACTIVE},
    {CM_DARKJADE, "dark jade", ROLE_INACTIVE},
    {CM_DARKFIRE, "spent", ROLE_INACTIVE},
    {CM_DARKLILAC, "recharging", ROLE_INACTIVE},
    {CM_DARKOLIVE, "dark olive", ROLE_INACTIVE},
    {CM_OLDBARK, "old bark", ROLE_INACTIVE},
    {CM_DEADBARK, "dead bark", ROLE_INACTIVE},
    {CM_BROWN, "brown", ROLE_INACTIVE},
};
_Static_assert(sizeof k_slots / sizeof k_slots[0] == SYN_NCOL - 2,
               "SYN_NCOL is the slots, other and corpse");
#define SLOT_OTHER (SYN_NCOL - 2)

/* JColor -> slot + 1, open addressing */
#define TAB 256
static uint32_t tab_key[TAB];
static uint8_t tab_val[TAB];
static bool tab_ready;

static unsigned hash(uint32_t c)
{
    c ^= c >> 16;
    c *= 0x45d9f3bu;
    c ^= c >> 16;
    return c & (TAB - 1);
}

static void tab_build(void)
{
    for (int s = 0; s < SYN_NCOL - 2; s++) {
        uint32_t c = (uint32_t)k_slots[s].c;
        unsigned h = hash(c);
        while (tab_val[h] && tab_key[h] != c)
            h = (h + 1) & (TAB - 1);
        if (!tab_val[h]) {
            tab_key[h] = c;
            tab_val[h] = (uint8_t)(s + 1);
        }
    }
    tab_ready = true;
}

int syn_col(uint32_t colour)
{
    if (!tab_ready)
        tab_build();
    unsigned h = hash(colour);
    while (tab_val[h]) {
        if (tab_key[h] == colour)
            return tab_val[h] - 1;
        h = (h + 1) & (TAB - 1);
    }
    return SLOT_OTHER;
}

void syn_col_rgb(int slot, uint8_t *r, uint8_t *g, uint8_t *b)
{
    JColor c = slot >= 0 && slot < SYN_NCOL - 2 ? k_slots[slot].c
               : slot == SYN_DEAD               ? CM_BROWN
                                                : JRGB(150, 150, 150);
    *r = (uint8_t)JCOLOR_R(c);
    *g = (uint8_t)JCOLOR_G(c);
    *b = (uint8_t)JCOLOR_B(c);
}

const char *syn_col_name(int slot)
{
    if (slot >= 0 && slot < SYN_NCOL - 2)
        return k_slots[slot].name;
    return slot == SYN_DEAD ? "dead" : "other";
}

Role syn_col_role(int slot)
{
    if (slot >= 0 && slot < SYN_NCOL - 2)
        return k_slots[slot].role;
    return slot == SYN_DEAD ? ROLE_DEAD : ROLE_OTHER;
}

int syn_seg(const Organism *o, int i)
{
    /* Color Mod's Organism.draw: brown once dead, unless it has eyes */
    return o->alive || o->haseyes ? syn_col((uint32_t)o->seg_color[i]) : SYN_DEAD;
}

int syn_flash(const Organism *o)
{
    return o->frames_color > 0 ? syn_col((uint32_t)o->flash_color) : -1;
}

int syn_genes(const GeneticCode *gc)
{
    return JLEN(gc->genes);
}

int syn_gene_col(const GeneticCode *gc, int i)
{
    return syn_col(gc->genes[i]->color);
}

int syn_generation(const Organism *o)
{
    return o->gc ? o->gc->generation : 0;
}

double syn_reproduce_energy(const Organism *o)
{
    return o->reproduce_energy;
}

double syn_growth_scale(const Organism *o)
{
    int g = o->growth_ratio > 0 && o->growth_ratio <= 16 ? o->growth_ratio - 1 : 0;
    return k_scale[g];
}

uint32_t syn_line_key(const Organism *o)
{
    /* "123", "123+456<789", or "user:123..." from the network corridor */
    const char *s = o->gc ? o->gc->clade_id : NULL;
    if (!s)
        return 0;
    const char *colon = strrchr(s, ':');
    if (colon)
        s = colon + 1;
    uint32_t v = 0;
    for (; *s >= '0' && *s <= '9'; s++)
        v = v * 10 + (uint32_t)(*s - '0');
    return v;
}

#else /* Biogenesis 0.9 */

static const Role k_roles[COL_COUNT] = {ROLE_PHOTO,  ROLE_EAT,  ROLE_MOVE,  ROLE_DEFEND,
                                        ROLE_INFECT, ROLE_KILL, ROLE_REPRO, ROLE_DEAD};

int syn_col(uint32_t colour)
{
    return (int)colour;
}

void syn_col_rgb(int slot, uint8_t *r, uint8_t *g, uint8_t *b)
{
    bio_color_rgb(slot, r, g, b);
}

const char *syn_col_name(int slot)
{
    static const char *const names[COL_COUNT] = {"green", "red",  "cyan",   "blue",
                                                 "white", "gray", "yellow", "brown"};
    return slot >= 0 && slot < COL_COUNT ? names[slot] : "";
}

Role syn_col_role(int slot)
{
    return slot >= 0 && slot < COL_COUNT ? k_roles[slot] : ROLE_OTHER;
}

int syn_seg(const Organism *o, int i)
{
    return o->alive ? o->seg_color[i] : COL_BROWN;
}

int syn_flash(const Organism *o)
{
    /* the flash colours past COL_COUNT are drawn as look_color makes them */
    return o->frames_color > 0 && o->flash_color != FLASH_NONE ? o->flash_color : -1;
}

int syn_genes(const GeneticCode *gc)
{
    return gc->ngenes;
}

int syn_gene_col(const GeneticCode *gc, int i)
{
    return gc->genes[i].color;
}

int syn_generation(const Organism *o)
{
    return o->generation;
}

double syn_reproduce_energy(const Organism *o)
{
    return o->gc ? o->gc->reproduce_energy : 1;
}

double syn_growth_scale(const Organism *o)
{
    int g = o->growth_ratio > 0 && o->growth_ratio <= 16 ? o->growth_ratio - 1 : 0;
    return 1.0 / k_scale[g];
}

uint32_t syn_line_key(const Organism *o)
{
    const Species *s = o->gc ? syn_family_get(o->gc->lineage) : NULL;
    return s ? s->root : 0;
}

#endif

float syn_flash_left(const Organism *o)
{
    return syn_flash(o) >= 0 ? o->frames_color / 10.0f : 0;
}
