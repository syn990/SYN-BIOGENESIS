/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Translated to C in 2026 by tools/colour/j2c.py from ParamDialog.java of
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

/* The parameters dialog's fields, generated from ParamDialog.java by
 * tools/colour/j2c_paramdialog.py (see src/colour/ui/params_dialog.c). */
#ifndef COLOUR_PARAMS_FIELDS_H
#define COLOUR_PARAMS_FIELDS_H

static const char *const k_tabs[] = {"General", "World", "Organisms", "Metabolism", "Genes", "Genes 2", "Genes 3"};
#define N_TABS 7
#define GENE_TABS_FROM 4

static const Field k_fields[] = {
    {0, 1, "Time per frame:", "milliseconds", 'i', offsetof(Params, delay)},
    {0, 2, "Time between backups:", NULL, 'i', offsetof(Params, backup_delay)},
    {1, 0, "Width:", NULL, 'i', offsetof(Params, world_width)},
    {1, 0, "Height:", NULL, 'i', offsetof(Params, world_height)},
    {1, 1, "Initial oxygen:", NULL, 'd', offsetof(Params, initial_o2)},
    {1, 1, "Initial carbon dioxide:", NULL, 'd', offsetof(Params, initial_co2)},
    {1, 2, "Initial methane:", NULL, 'd', offsetof(Params, initial_ch4)},
    {1, 3, "CO2 to CH4 divisor:", NULL, 'i', offsetof(Params, co2_to_ch4_divisor)},
    {1, 3, "CH4 to CO2 divisor:", NULL, 'i', offsetof(Params, ch4_to_co2_divisor)},
    {1, 4, "Rubbing coefficient:", NULL, 'd', offsetof(Params, rubbing)},
    {1, 5, "Elasticity coefficient:", NULL, 'd', offsetof(Params, elasticity)},
    {2, 0, "Initial number:", NULL, 'i', offsetof(Params, initial_organisms)},
    {2, 0, "Initial energy:", NULL, 'i', offsetof(Params, initial_energy)},
    {2, 1, "Initial complexity:", NULL, 'i', offsetof(Params, initial_complexity)},
    {2, 1, "Clade complexity:", NULL, 'i', offsetof(Params, clade_complexity)},
    {2, 2, "Meta mutation ‱:", NULL, 'i', offsetof(Params, meta_mutation_rate)},
    {2, 3, "Min mutation ‱:", NULL, 'i', offsetof(Params, min_mutation_rate)},
    {2, 3, "Max mutation ‱:", NULL, 'i', offsetof(Params, max_mutation_rate)},
    {2, 4, "Life expectancy:", NULL, 'i', offsetof(Params, max_age)},
    {2, 4, "Life expectancy divisor:", NULL, 'i', offsetof(Params, age_divisor)},
    {2, 5, "Segment upkeep cost divisor:", NULL, 'i', offsetof(Params, segment_cost_divisor)},
    {2, 6, "Substances drainage divisor:", NULL, 'i', offsetof(Params, drain_subs_divisor)},
    {2, 7, "Energy lost by dead organisms:", NULL, 'd', offsetof(Params, decay_energy)},
    {3, 0, "Obtained energy divisor:", NULL, 'i', offsetof(Params, green_obtained_energy_divisor)},
    {3, 1, "Obtained energy:", NULL, 'd', offsetof(Params, organic_obtained_energy)},
    {3, 2, "Released energy proportion:", NULL, 'd', offsetof(Params, organic_subs_produced)},
    {3, 3, "Cream released energy proportion:", NULL, 'd', offsetof(Params, cream_organic_subs_produced)},
    {3, 4, "Healing delay:", NULL, 'i', offsetof(Params, healing)},
    {3, 4, "Immune system delay:", NULL, 'i', offsetof(Params, immune_system)},
    {3, 5, "Jade healing delay:", NULL, 'i', offsetof(Params, darkjade_delay)},
    {3, 5, "Lavender shield:", NULL, 'i', offsetof(Params, lavender_shield)},
    {3, 6, "Indigo divisor:", NULL, 'd', offsetof(Params, indigo_divisor)},
    {3, 6, "Dodge cost:", NULL, 'd', offsetof(Params, dodge_energy_consumption)},
    {3, 7, "Spike consumer cost:", NULL, 'd', offsetof(Params, mosquito_energy_consumption)},
    {3, 7, "Silver consumer cost:", NULL, 'd', offsetof(Params, experience_energy_consumption)},
    {3, 8, "Non-plant white cost:", NULL, 'd', offsetof(Params, virus_energy_consumption)},
    {3, 8, "Non-plant plague cost:", NULL, 'd', offsetof(Params, scourge_energy_consumption)},
    {3, 9, "Lime effectivity if not alone:", NULL, 'd', offsetof(Params, crowdedlime_energy_consumption)},
    {3, 9, "+ Forest effectivity if not alone:", NULL, 'd', offsetof(Params, crowdedforest_energy_consumption)},
    {3, 10, "+ Photosynthetic effectivity with symbiont:", NULL, 'd', offsetof(Params, symbiont_energy_consumption)},
    {4, 1, "green", NULL, 'i', offsetof(Params, green_prob)},
    {4, 1, "green", NULL, 'd', offsetof(Params, green_energy_consumption)},
    {4, 2, "forest", NULL, 'i', offsetof(Params, forest_prob)},
    {4, 2, "forest", NULL, 'd', offsetof(Params, forest_energy_consumption)},
    {4, 3, "spring", NULL, 'i', offsetof(Params, spring_prob)},
    {4, 3, "spring", NULL, 'd', offsetof(Params, spring_energy_consumption)},
    {4, 4, "summer", NULL, 'i', offsetof(Params, summer_prob)},
    {4, 4, "summer", NULL, 'd', offsetof(Params, summer_energy_consumption)},
    {4, 5, "lime", NULL, 'i', offsetof(Params, lime_prob)},
    {4, 5, "lime", NULL, 'd', offsetof(Params, lime_energy_consumption)},
    {4, 6, "leaf", NULL, 'i', offsetof(Params, leaf_prob)},
    {4, 6, "leaf", NULL, 'd', offsetof(Params, leaf_energy_consumption)},
    {4, 7, "c4plant", NULL, 'i', offsetof(Params, c4_prob)},
    {4, 7, "c4plant", NULL, 'd', offsetof(Params, c4_energy_consumption)},
    {4, 8, "jade", NULL, 'i', offsetof(Params, jade_prob)},
    {4, 8, "jade", NULL, 'd', offsetof(Params, jade_energy_consumption)},
    {4, 9, "grass", NULL, 'i', offsetof(Params, grass_prob)},
    {4, 9, "grass", NULL, 'd', offsetof(Params, grass_energy_consumption)},
    {4, 10, "bark", NULL, 'i', offsetof(Params, bark_prob)},
    {4, 10, "bark", NULL, 'd', offsetof(Params, bark_energy_consumption)},
    {4, 11, "purple", NULL, 'i', offsetof(Params, purple_prob)},
    {4, 11, "purple", NULL, 'd', offsetof(Params, purple_energy_consumption)},
    {4, 12, "dark gray", NULL, 'i', offsetof(Params, darkgray_prob)},
    {4, 12, "dark gray", NULL, 'd', offsetof(Params, darkgray_energy_consumption)},
    {5, 1, "red", NULL, 'i', offsetof(Params, red_prob)},
    {5, 1, "red", NULL, 'd', offsetof(Params, red_energy_consumption)},
    {5, 2, "fire", NULL, 'i', offsetof(Params, fire_prob)},
    {5, 2, "fire", NULL, 'd', offsetof(Params, fire_energy_consumption)},
    {5, 3, "orange", NULL, 'i', offsetof(Params, orange_prob)},
    {5, 3, "orange", NULL, 'd', offsetof(Params, orange_energy_consumption)},
    {5, 4, "maroon", NULL, 'i', offsetof(Params, maroon_prob)},
    {5, 4, "maroon", NULL, 'd', offsetof(Params, maroon_energy_consumption)},
    {5, 5, "pink", NULL, 'i', offsetof(Params, pink_prob)},
    {5, 5, "pink", NULL, 'd', offsetof(Params, pink_energy_consumption)},
    {5, 6, "cream", NULL, 'i', offsetof(Params, cream_prob)},
    {5, 6, "cream", NULL, 'd', offsetof(Params, cream_energy_consumption)},
    {5, 7, "silver", NULL, 'i', offsetof(Params, silver_prob)},
    {5, 7, "silver", NULL, 'd', offsetof(Params, silver_energy_consumption)},
    {5, 8, "spike", NULL, 'i', offsetof(Params, spike_prob)},
    {5, 8, "spike", NULL, 'd', offsetof(Params, spike_energy_consumption)},
    {5, 9, "lilac", NULL, 'i', offsetof(Params, lilac_prob)},
    {5, 9, "lilac", NULL, 'd', offsetof(Params, lilac_energy_consumption)},
    {5, 10, "gray", NULL, 'i', offsetof(Params, gray_prob)},
    {5, 10, "gray", NULL, 'd', offsetof(Params, gray_energy_consumption)},
    {5, 11, "violet", NULL, 'i', offsetof(Params, violet_prob)},
    {5, 11, "violet", NULL, 'd', offsetof(Params, violet_energy_consumption)},
    {5, 12, "olive", NULL, 'i', offsetof(Params, olive_prob)},
    {5, 12, "olive", NULL, 'd', offsetof(Params, olive_energy_consumption)},
    {5, 13, "sky", NULL, 'i', offsetof(Params, sky_prob)},
    {5, 13, "sky", NULL, 'd', offsetof(Params, sky_energy_consumption)},
    {5, 14, "blue", NULL, 'i', offsetof(Params, blue_prob)},
    {5, 14, "blue", NULL, 'd', offsetof(Params, blue_energy_consumption)},
    {5, 15, "ochre", NULL, 'i', offsetof(Params, ochre_prob)},
    {5, 15, "ochre", NULL, 'd', offsetof(Params, ochre_energy_consumption)},
    {5, 16, "fallow", NULL, 'i', offsetof(Params, fallow_prob)},
    {5, 16, "fallow", NULL, 'd', offsetof(Params, fallow_energy_consumption)},
    {5, 17, "spore", NULL, 'i', offsetof(Params, spore_prob)},
    {5, 17, "spore", NULL, 'd', offsetof(Params, spore_energy_consumption)},
    {6, 1, "white", NULL, 'i', offsetof(Params, white_prob)},
    {6, 1, "white", NULL, 'd', offsetof(Params, white_energy_consumption)},
    {6, 2, "plague", NULL, 'i', offsetof(Params, plague_prob)},
    {6, 2, "plague", NULL, 'd', offsetof(Params, plague_energy_consumption)},
    {6, 3, "coral", NULL, 'i', offsetof(Params, coral_prob)},
    {6, 3, "coral", NULL, 'd', offsetof(Params, coral_energy_consumption)},
    {6, 4, "mint", NULL, 'i', offsetof(Params, mint_prob)},
    {6, 4, "mint", NULL, 'd', offsetof(Params, mint_energy_consumption)},
    {6, 5, "lavender", NULL, 'i', offsetof(Params, lavender_prob)},
    {6, 5, "lavender", NULL, 'd', offsetof(Params, lavender_energy_consumption)},
    {6, 6, "magenta", NULL, 'i', offsetof(Params, magenta_prob)},
    {6, 6, "magenta", NULL, 'd', offsetof(Params, magenta_energy_consumption)},
    {6, 7, "rose", NULL, 'i', offsetof(Params, rose_prob)},
    {6, 7, "rose", NULL, 'd', offsetof(Params, rose_energy_consumption)},
    {6, 8, "cyan", NULL, 'i', offsetof(Params, cyan_prob)},
    {6, 8, "cyan", NULL, 'd', offsetof(Params, cyan_energy_consumption)},
    {6, 9, "teal", NULL, 'i', offsetof(Params, teal_prob)},
    {6, 9, "teal", NULL, 'd', offsetof(Params, teal_energy_consumption)},
    {6, 10, "yellow", NULL, 'i', offsetof(Params, yellow_prob)},
    {6, 10, "yellow", NULL, 'd', offsetof(Params, yellow_energy_consumption)},
    {6, 11, "auburn", NULL, 'i', offsetof(Params, auburn_prob)},
    {6, 11, "auburn", NULL, 'd', offsetof(Params, auburn_energy_consumption)},
    {6, 12, "indigo", NULL, 'i', offsetof(Params, indigo_prob)},
    {6, 12, "indigo", NULL, 'd', offsetof(Params, indigo_energy_consumption)},
    {6, 13, "blond", NULL, 'i', offsetof(Params, blond_prob)},
    {6, 13, "blond", NULL, 'd', offsetof(Params, blond_energy_consumption)},
    {6, 14, "flower", NULL, 'i', offsetof(Params, flower_prob)},
    {6, 14, "flower", NULL, 'd', offsetof(Params, flower_energy_consumption)},
    {6, 15, "gold", NULL, 'i', offsetof(Params, gold_prob)},
    {6, 15, "gold", NULL, 'd', offsetof(Params, gold_divisor)},
    {6, 16, "black", NULL, 'i', offsetof(Params, dark_prob)},
    {6, 16, "black", NULL, 'd', offsetof(Params, dark_energy_consumption)},
    {6, 17, "eye", NULL, 'i', offsetof(Params, eye_prob)},
    {6, 17, "eye", NULL, 'd', offsetof(Params, eye_energy_consumption)},
};
#define N_FIELDS ((int)(sizeof k_fields / sizeof k_fields[0]))

/* ParamDialog.checkParams: whether a value is taken */
static bool field_ok(int i, double v)
{
    switch (i) {
    case 0: return v > 0; /* DELAY */
    case 1: return v > 0; /* BACKUP_DELAY */
    case 2: return v > 0; /* WORLD_WIDTH */
    case 3: return v > 0; /* WORLD_HEIGHT */
    case 4: return v >= 0; /* INITIAL_O2 */
    case 5: return v >= 0; /* INITIAL_CO2 */
    case 6: return v >= 0; /* INITIAL_CH4 */
    case 7: return v > 0; /* CO2_TO_CH4_DIVISOR */
    case 8: return v > 0; /* CH4_TO_CO2_DIVISOR */
    case 9: return v >= 0 && v <= 1; /* RUBBING */
    case 10: return v >= 0 && v <= 1; /* ELASTICITY */
    case 11: return v >= 0; /* INITIAL_ORGANISMS */
    case 12: return v >= 0; /* INITIAL_ENERGY */
    case 13: return v > 0; /* INITIAL_COMPLEXITY */
    case 14: return v >= 0; /* CLADE_COMPLEXITY */
    case 15: return v >= 0 && v <= 10000; /* META_MUTATION_RATE */
    case 16: return v >= 0 && v <= P.max_mutation_rate; /* MIN_MUTATION_RATE */
    case 17: return v >= 0 && v <= 10000; /* MAX_MUTATION_RATE */
    case 18: return v >= 0; /* MAX_AGE */
    case 19: return v > 0; /* AGE_DIVISOR */
    case 20: return v > 0; /* SEGMENT_COST_DIVISOR */
    case 21: return v > 0; /* DRAIN_SUBS_DIVISOR */
    case 22: return v >= 0; /* DECAY_ENERGY */
    case 23: return v > 0; /* GREEN_OBTAINED_ENERGY_DIVISOR */
    case 24: return v >= 0; /* ORGANIC_OBTAINED_ENERGY */
    case 25: return v >= 0 && v <= 1; /* ORGANIC_SUBS_PRODUCED */
    case 26: return v >= 0 && v <= 1; /* CREAM_ORGANIC_SUBS_PRODUCED */
    case 27: return v >= 1; /* HEALING */
    case 28: return v >= 1; /* IMMUNE_SYSTEM */
    case 29: return v >= 1; /* DARKJADE_DELAY */
    case 30: return v >= 0; /* LAVENDER_SHIELD */
    case 31: return v > 0; /* INDIGO_DIVISOR */
    case 32: return v >= 0; /* DODGE_ENERGY_CONSUMPTION */
    case 33: return v >= 0; /* MOSQUITO_ENERGY_CONSUMPTION */
    case 34: return v >= 0; /* EXPERIENCE_ENERGY_CONSUMPTION */
    case 35: return v >= 0; /* VIRUS_ENERGY_CONSUMPTION */
    case 36: return v >= 0; /* SCOURGE_ENERGY_CONSUMPTION */
    case 37: return v > 0; /* CROWDEDLIME_ENERGY_CONSUMPTION */
    case 38: return v > 0; /* CROWDEDFOREST_ENERGY_CONSUMPTION */
    case 39: return v > 0; /* SYMBIONT_ENERGY_CONSUMPTION */
    case 40: return v >= 0; /* GREEN_PROB */
    case 41: return v > 0; /* GREEN_ENERGY_CONSUMPTION */
    case 42: return v >= 0; /* FOREST_PROB */
    case 43: return v > 0; /* FOREST_ENERGY_CONSUMPTION */
    case 44: return v >= 0; /* SPRING_PROB */
    case 45: return v > 0; /* SPRING_ENERGY_CONSUMPTION */
    case 46: return v >= 0; /* SUMMER_PROB */
    case 47: return v > 0; /* SUMMER_ENERGY_CONSUMPTION */
    case 48: return v >= 0; /* LIME_PROB */
    case 49: return v > 0; /* LIME_ENERGY_CONSUMPTION */
    case 50: return v >= 0; /* LEAF_PROB */
    case 51: return v > 0; /* LEAF_ENERGY_CONSUMPTION */
    case 52: return v >= 0; /* C4_PROB */
    case 53: return v > 0; /* C4_ENERGY_CONSUMPTION */
    case 54: return v >= 0; /* JADE_PROB */
    case 55: return v > 0; /* JADE_ENERGY_CONSUMPTION */
    case 56: return v >= 0; /* GRASS_PROB */
    case 57: return v > 0; /* GRASS_ENERGY_CONSUMPTION */
    case 58: return v >= 0; /* BARK_PROB */
    case 59: return v > 0; /* BARK_ENERGY_CONSUMPTION */
    case 60: return v >= 0; /* PURPLE_PROB */
    case 61: return v > 0; /* PURPLE_ENERGY_CONSUMPTION */
    case 62: return v >= 0; /* DARKGRAY_PROB */
    case 63: return v >= 0; /* DARKGRAY_ENERGY_CONSUMPTION */
    case 64: return v >= 0; /* RED_PROB */
    case 65: return v >= 0; /* RED_ENERGY_CONSUMPTION */
    case 66: return v >= 0; /* FIRE_PROB */
    case 67: return v >= 0; /* FIRE_ENERGY_CONSUMPTION */
    case 68: return v >= 0; /* ORANGE_PROB */
    case 69: return v >= 0; /* ORANGE_ENERGY_CONSUMPTION */
    case 70: return v >= 0; /* MAROON_PROB */
    case 71: return v >= 0; /* MAROON_ENERGY_CONSUMPTION */
    case 72: return v >= 0; /* PINK_PROB */
    case 73: return v >= 0; /* PINK_ENERGY_CONSUMPTION */
    case 74: return v >= 0; /* CREAM_PROB */
    case 75: return v >= 0; /* CREAM_ENERGY_CONSUMPTION */
    case 76: return v >= 0; /* SILVER_PROB */
    case 77: return v >= 0; /* SILVER_ENERGY_CONSUMPTION */
    case 78: return v >= 0; /* SPIKE_PROB */
    case 79: return v >= 0; /* SPIKE_ENERGY_CONSUMPTION */
    case 80: return v >= 0; /* LILAC_PROB */
    case 81: return v >= 0; /* LILAC_ENERGY_CONSUMPTION */
    case 82: return v >= 0; /* GRAY_PROB */
    case 83: return v >= 0; /* GRAY_ENERGY_CONSUMPTION */
    case 84: return v >= 0; /* VIOLET_PROB */
    case 85: return v >= 0; /* VIOLET_ENERGY_CONSUMPTION */
    case 86: return v >= 0; /* OLIVE_PROB */
    case 87: return v >= 0; /* OLIVE_ENERGY_CONSUMPTION */
    case 88: return v >= 0; /* SKY_PROB */
    case 89: return v >= 0; /* SKY_ENERGY_CONSUMPTION */
    case 90: return v >= 0; /* BLUE_PROB */
    case 91: return v >= 0; /* BLUE_ENERGY_CONSUMPTION */
    case 92: return v >= 0; /* OCHRE_PROB */
    case 93: return v >= 0; /* OCHRE_ENERGY_CONSUMPTION */
    case 94: return v >= 0; /* FALLOW_PROB */
    case 95: return v >= 0; /* FALLOW_ENERGY_CONSUMPTION */
    case 96: return v >= 0; /* SPORE_PROB */
    case 97: return v >= 0; /* SPORE_ENERGY_CONSUMPTION */
    case 98: return v >= 0; /* WHITE_PROB */
    case 99: return v >= 0; /* WHITE_ENERGY_CONSUMPTION */
    case 100: return v >= 0; /* PLAGUE_PROB */
    case 101: return v >= 0; /* PLAGUE_ENERGY_CONSUMPTION */
    case 102: return v >= 0; /* CORAL_PROB */
    case 103: return v >= 0; /* CORAL_ENERGY_CONSUMPTION */
    case 104: return v >= 0; /* MINT_PROB */
    case 105: return v >= 0; /* MINT_ENERGY_CONSUMPTION */
    case 106: return v >= 0; /* LAVENDER_PROB */
    case 107: return v > 0; /* LAVENDER_ENERGY_CONSUMPTION */
    case 108: return v >= 0; /* MAGENTA_PROB */
    case 109: return v >= 0; /* MAGENTA_ENERGY_CONSUMPTION */
    case 110: return v >= 0; /* ROSE_PROB */
    case 111: return v > 0; /* ROSE_ENERGY_CONSUMPTION */
    case 112: return v >= 0; /* CYAN_PROB */
    case 113: return v >= 0; /* CYAN_ENERGY_CONSUMPTION */
    case 114: return v >= 0; /* TEAL_PROB */
    case 115: return v >= 0; /* TEAL_ENERGY_CONSUMPTION */
    case 116: return v >= 0; /* YELLOW_PROB */
    case 117: return v >= 0; /* YELLOW_ENERGY_CONSUMPTION */
    case 118: return v >= 0; /* AUBURN_PROB */
    case 119: return v >= 0; /* AUBURN_ENERGY_CONSUMPTION */
    case 120: return v >= 0; /* INDIGO_PROB */
    case 121: return v >= 1; /* INDIGO_ENERGY_CONSUMPTION */
    case 122: return v >= 0; /* BLOND_PROB */
    case 123: return v >= 0; /* BLOND_ENERGY_CONSUMPTION */
    case 124: return v >= 0; /* FLOWER_PROB */
    case 125: return v > 0; /* FLOWER_ENERGY_CONSUMPTION */
    case 126: return v >= 0; /* GOLD_PROB */
    case 127: return v > 0; /* GOLD_DIVISOR */
    case 128: return v >= 0; /* DARK_PROB */
    case 129: return v >= 0; /* DARK_ENERGY_CONSUMPTION */
    case 130: return v >= 0; /* EYE_PROB */
    case 131: return v >= 0; /* EYE_ENERGY_CONSUMPTION */
    }
    return false;
}

/* the order checkParams takes them in */
static const int k_apply_order[] = {2, 3, 0, 1, 4, 5, 6, 9, 10, 11, 12, 13, 14, 18, 19, 7, 8, 15, 17, 16, 65, 41, 91, 113, 99, 83, 117, 109, 73, 103, 69, 43, 45, 51, 47, 49, 53, 55, 57, 61, 59, 85, 115, 131, 71, 87, 105, 75, 111, 129, 93, 89, 81, 77, 67, 63, 127, 123, 125, 119, 101, 79, 121, 107, 95, 97, 64, 40, 90, 112, 98, 82, 116, 108, 72, 102, 68, 42, 44, 50, 46, 48, 52, 54, 56, 60, 58, 84, 114, 130, 70, 86, 104, 74, 110, 128, 92, 88, 80, 76, 66, 62, 126, 122, 124, 118, 100, 78, 120, 106, 94, 96, 20, 21, 23, 24, 25, 26, 27, 28, 30, 31, 33, 37, 38, 39, 34, 32, 29, 35, 36, 22};

static const char *const k_gene_tab_heads[] = {"Color", "Probability:", "Effectivity:"};

#endif
