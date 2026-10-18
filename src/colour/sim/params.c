/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Translated to C in 2026 by tools/colour/j2c.py from Utils.java of
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

#include "sim/params.h"

#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

Params P;
JRandom g_rng;

const int k_side[8][2] = {{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};
const double k_scale[16] = {1.00, 0.90, 0.80, 0.71, 0.63, 0.56, 0.50, 0.45, 0.40, 0.36, 0.32, 0.28, 0.25, 0.22, 0.20, 0.18};
const double k_springscale[16] = {0.01, 0.009, 0.008, 0.0071, 0.0063, 0.0056, 0.005, 0.0045, 0.004, 0.0036, 0.0032, 0.0028, 0.0025, 0.0022, 0.002, 0.0018};

void params_set_defaults(Params *p)
{
    p->window_x = 0;
    p->window_y = 0;
    p->window_width = 640;
    p->window_height = 480;
    p->window_state = 0;
    p->initial_organisms = 12500;
    p->initial_complexity = 2;
    p->clade_complexity = 25;
    p->initial_o2 = 0;
    p->initial_co2 = 500000;
    p->initial_ch4 = 0;
    p->organisms_vector_size = 50;
    p->world_width = 7000;
    p->world_height = 7000;
    p->max_age = 6;
    p->age_divisor = 4;
    p->co2_to_ch4_divisor = 999;
    p->ch4_to_co2_divisor = 1000;
    p->rubbing = 0.98;
    p->meta_mutation_rate = 1000;
    p->max_mutation_rate = 100;
    p->min_mutation_rate = 0;
    p->segment_cost_divisor = 5000;
    p->organic_obtained_energy = 2.0;
    p->green_obtained_energy_divisor = 1000;
    p->organic_subs_produced = 0.1;
    p->cream_organic_subs_produced = 0.0125;
    p->healing = 40000;
    p->immune_system = 40000;
    p->lavender_shield = 10000;
    p->indigo_divisor = 10.0;
    p->gold_divisor = 2;
    p->dodge_energy_consumption = 0.0;
    p->darkjade_delay = 4;
    p->red_energy_consumption = 0.0;
    p->green_energy_consumption = 1.1;
    p->blue_energy_consumption = 0.0;
    p->cyan_energy_consumption = 0.0;
    p->white_energy_consumption = 0.1;
    p->virus_energy_consumption = 0.01;
    p->gray_energy_consumption = 0.1;
    p->darkgray_energy_consumption = 10.0;
    p->silver_energy_consumption = 0.1;
    p->yellow_energy_consumption = 0.0;
    p->magenta_energy_consumption = 0.01;
    p->pink_energy_consumption = 0.0;
    p->coral_energy_consumption = 0.01;
    p->orange_energy_consumption = 0.0;
    p->forest_energy_consumption = 1.0;
    p->crowdedforest_energy_consumption = 1.0;
    p->symbiont_energy_consumption = 0.6;
    p->spring_energy_consumption = 1.0;
    p->leaf_energy_consumption = 1.15;
    p->lime_energy_consumption = 1.25;
    p->crowdedlime_energy_consumption = 0.5;
    p->summer_energy_consumption = 1.65;
    p->mosquito_energy_consumption = 0.0;
    p->bark_energy_consumption = 0.7;
    p->jade_energy_consumption = 1.0;
    p->grass_energy_consumption = 1.0;
    p->purple_energy_consumption = 4.0;
    p->c4_energy_consumption = 0.55;
    p->violet_energy_consumption = 0.1;
    p->teal_energy_consumption = 0.0;
    p->eye_energy_consumption = 0.0;
    p->maroon_energy_consumption = 0.0;
    p->olive_energy_consumption = 0.1;
    p->mint_energy_consumption = 0.075;
    p->cream_energy_consumption = 0.0;
    p->rose_energy_consumption = 0.01;
    p->dark_energy_consumption = 0.01;
    p->ochre_energy_consumption = 0.4;
    p->sky_energy_consumption = 0.05;
    p->lilac_energy_consumption = 0.0;
    p->fire_energy_consumption = 0.0;
    p->experience_energy_consumption = 0.0;
    p->blond_energy_consumption = 0.037;
    p->flower_energy_consumption = 0.01;
    p->auburn_energy_consumption = 0.0;
    p->indigo_energy_consumption = 1.0;
    p->lavender_energy_consumption = 1.0;
    p->fallow_energy_consumption = 0.0;
    p->spore_energy_consumption = 0.0;
    p->plague_energy_consumption = 10.0;
    p->scourge_energy_consumption = 0.1;
    p->spike_energy_consumption = 0.0;
    p->red_prob = 1;
    p->green_prob = 3;
    p->blue_prob = 2;
    p->cyan_prob = 8;
    p->white_prob = 4;
    p->gray_prob = 1;
    p->darkgray_prob = 5;
    p->silver_prob = 1;
    p->yellow_prob = 2;
    p->magenta_prob = 5;
    p->pink_prob = 2;
    p->coral_prob = 1;
    p->orange_prob = 1;
    p->forest_prob = 3;
    p->spring_prob = 3;
    p->leaf_prob = 3;
    p->lime_prob = 3;
    p->summer_prob = 3;
    p->bark_prob = 3;
    p->jade_prob = 3;
    p->grass_prob = 3;
    p->purple_prob = 3;
    p->c4_prob = 3;
    p->violet_prob = 1;
    p->teal_prob = 8;
    p->eye_prob = 4;
    p->maroon_prob = 2;
    p->olive_prob = 1;
    p->mint_prob = 2;
    p->cream_prob = 1;
    p->rose_prob = 2;
    p->dark_prob = 1;
    p->ochre_prob = 1;
    p->sky_prob = 1;
    p->lilac_prob = 1;
    p->fire_prob = 1;
    p->gold_prob = 2;
    p->blond_prob = 2;
    p->flower_prob = 2;
    p->auburn_prob = 2;
    p->indigo_prob = 2;
    p->lavender_prob = 2;
    p->fallow_prob = 1;
    p->spore_prob = 2;
    p->plague_prob = 1;
    p->spike_prob = 1;
    p->drain_subs_divisor = 5000;
    p->initial_energy = 40;
    p->max_vel = 5.0;
    p->max_rot = M_PI / 16.0;
    p->elasticity = 0.8;
    p->delay = 2;
    p->auto_backup = false;
    p->backup_delay = 100;
    p->local_port = 8888;
    p->max_connections = 1;
    p->accept_connections = false;
    p->connect_to_server = false;
    p->hardware_acceleration = 0;
    p->server_port = 0;
    p->decay_energy = 0.1;
}

int bio_random_sign(void)
{
    return (jr_next_int(&g_rng, 2) << 1) - 1;
}

bool bio_random_mutation(void)
{
    return jr_next_int(&g_rng, 10000) < P.meta_mutation_rate;
}

/* ---- persistence ---- */

typedef struct {
    const char *key;
    char type; /* 'i', 'd' or 'b' */
    size_t offset;
} ParamField;

static const ParamField k_fields[] = {
    {"WINDOW_WIDTH", 'i', offsetof(Params, window_width)},
    {"WINDOW_HEIGHT", 'i', offsetof(Params, window_height)},
    {"INITIAL_ORGANISMS", 'i', offsetof(Params, initial_organisms)},
    {"INITIAL_COMPLEXITY", 'i', offsetof(Params, initial_complexity)},
    {"CLADE_COMPLEXITY", 'i', offsetof(Params, clade_complexity)},
    {"INITIAL_O2", 'd', offsetof(Params, initial_o2)},
    {"INITIAL_CO2", 'd', offsetof(Params, initial_co2)},
    {"INITIAL_CH4", 'd', offsetof(Params, initial_ch4)},
    {"ORGANISMS_VECTOR_SIZE", 'i', offsetof(Params, organisms_vector_size)},
    {"WORLD_WIDTH", 'i', offsetof(Params, world_width)},
    {"WORLD_HEIGHT", 'i', offsetof(Params, world_height)},
    {"MAX_AGE", 'i', offsetof(Params, max_age)},
    {"AGE_DIVISOR", 'i', offsetof(Params, age_divisor)},
    {"CO2_TO_CH4_DIVISOR", 'i', offsetof(Params, co2_to_ch4_divisor)},
    {"CH4_TO_CO2_DIVISOR", 'i', offsetof(Params, ch4_to_co2_divisor)},
    {"RUBBING", 'd', offsetof(Params, rubbing)},
    {"META_MUTATION_RATE", 'i', offsetof(Params, meta_mutation_rate)},
    {"MAX_MUTATION_RATE", 'i', offsetof(Params, max_mutation_rate)},
    {"MIN_MUTATION_RATE", 'i', offsetof(Params, min_mutation_rate)},
    {"SEGMENT_COST_DIVISOR", 'i', offsetof(Params, segment_cost_divisor)},
    {"ORGANIC_OBTAINED_ENERGY", 'd', offsetof(Params, organic_obtained_energy)},
    {"GREEN_OBTAINED_ENERGY_DIVISOR", 'i', offsetof(Params, green_obtained_energy_divisor)},
    {"ORGANIC_SUBS_PRODUCED", 'd', offsetof(Params, organic_subs_produced)},
    {"CREAM_ORGANIC_SUBS_PRODUCED", 'd', offsetof(Params, cream_organic_subs_produced)},
    {"HEALING", 'i', offsetof(Params, healing)},
    {"IMMUNE_SYSTEM", 'i', offsetof(Params, immune_system)},
    {"LAVENDER_SHIELD", 'i', offsetof(Params, lavender_shield)},
    {"INDIGO_DIVISOR", 'd', offsetof(Params, indigo_divisor)},
    {"GOLD_DIVISOR", 'd', offsetof(Params, gold_divisor)},
    {"DODGE_ENERGY_CONSUMPTION", 'd', offsetof(Params, dodge_energy_consumption)},
    {"DARKJADE_DELAY", 'i', offsetof(Params, darkjade_delay)},
    {"RED_ENERGY_CONSUMPTION", 'd', offsetof(Params, red_energy_consumption)},
    {"GREEN_ENERGY_CONSUMPTION", 'd', offsetof(Params, green_energy_consumption)},
    {"BLUE_ENERGY_CONSUMPTION", 'd', offsetof(Params, blue_energy_consumption)},
    {"CYAN_ENERGY_CONSUMPTION", 'd', offsetof(Params, cyan_energy_consumption)},
    {"WHITE_ENERGY_CONSUMPTION", 'd', offsetof(Params, white_energy_consumption)},
    {"VIRUS_ENERGY_CONSUMPTION", 'd', offsetof(Params, virus_energy_consumption)},
    {"GRAY_ENERGY_CONSUMPTION", 'd', offsetof(Params, gray_energy_consumption)},
    {"DARKGRAY_ENERGY_CONSUMPTION", 'd', offsetof(Params, darkgray_energy_consumption)},
    {"SILVER_ENERGY_CONSUMPTION", 'd', offsetof(Params, silver_energy_consumption)},
    {"YELLOW_ENERGY_CONSUMPTION", 'd', offsetof(Params, yellow_energy_consumption)},
    {"MAGENTA_ENERGY_CONSUMPTION", 'd', offsetof(Params, magenta_energy_consumption)},
    {"PINK_ENERGY_CONSUMPTION", 'd', offsetof(Params, pink_energy_consumption)},
    {"CORAL_ENERGY_CONSUMPTION", 'd', offsetof(Params, coral_energy_consumption)},
    {"ORANGE_ENERGY_CONSUMPTION", 'd', offsetof(Params, orange_energy_consumption)},
    {"FOREST_ENERGY_CONSUMPTION", 'd', offsetof(Params, forest_energy_consumption)},
    {"CROWDEDFOREST_ENERGY_CONSUMPTION", 'd', offsetof(Params, crowdedforest_energy_consumption)},
    {"SYMBIONT_ENERGY_CONSUMPTION", 'd', offsetof(Params, symbiont_energy_consumption)},
    {"SPRING_ENERGY_CONSUMPTION", 'd', offsetof(Params, spring_energy_consumption)},
    {"LEAF_ENERGY_CONSUMPTION", 'd', offsetof(Params, leaf_energy_consumption)},
    {"LIME_ENERGY_CONSUMPTION", 'd', offsetof(Params, lime_energy_consumption)},
    {"CROWDEDLIME_ENERGY_CONSUMPTION", 'd', offsetof(Params, crowdedlime_energy_consumption)},
    {"SUMMER_ENERGY_CONSUMPTION", 'd', offsetof(Params, summer_energy_consumption)},
    {"MOSQUITO_ENERGY_CONSUMPTION", 'd', offsetof(Params, mosquito_energy_consumption)},
    {"BARK_ENERGY_CONSUMPTION", 'd', offsetof(Params, bark_energy_consumption)},
    {"JADE_ENERGY_CONSUMPTION", 'd', offsetof(Params, jade_energy_consumption)},
    {"GRASS_ENERGY_CONSUMPTION", 'd', offsetof(Params, grass_energy_consumption)},
    {"PURPLE_ENERGY_CONSUMPTION", 'd', offsetof(Params, purple_energy_consumption)},
    {"C4_ENERGY_CONSUMPTION", 'd', offsetof(Params, c4_energy_consumption)},
    {"VIOLET_ENERGY_CONSUMPTION", 'd', offsetof(Params, violet_energy_consumption)},
    {"TEAL_ENERGY_CONSUMPTION", 'd', offsetof(Params, teal_energy_consumption)},
    {"EYE_ENERGY_CONSUMPTION", 'd', offsetof(Params, eye_energy_consumption)},
    {"MAROON_ENERGY_CONSUMPTION", 'd', offsetof(Params, maroon_energy_consumption)},
    {"OLIVE_ENERGY_CONSUMPTION", 'd', offsetof(Params, olive_energy_consumption)},
    {"MINT_ENERGY_CONSUMPTION", 'd', offsetof(Params, mint_energy_consumption)},
    {"CREAM_ENERGY_CONSUMPTION", 'd', offsetof(Params, cream_energy_consumption)},
    {"ROSE_ENERGY_CONSUMPTION", 'd', offsetof(Params, rose_energy_consumption)},
    {"DARK_ENERGY_CONSUMPTION", 'd', offsetof(Params, dark_energy_consumption)},
    {"OCHRE_ENERGY_CONSUMPTION", 'd', offsetof(Params, ochre_energy_consumption)},
    {"SKY_ENERGY_CONSUMPTION", 'd', offsetof(Params, sky_energy_consumption)},
    {"LILAC_ENERGY_CONSUMPTION", 'd', offsetof(Params, lilac_energy_consumption)},
    {"FIRE_ENERGY_CONSUMPTION", 'd', offsetof(Params, fire_energy_consumption)},
    {"EXPERIENCE_ENERGY_CONSUMPTION", 'd', offsetof(Params, experience_energy_consumption)},
    {"BLOND_ENERGY_CONSUMPTION", 'd', offsetof(Params, blond_energy_consumption)},
    {"FLOWER_ENERGY_CONSUMPTION", 'd', offsetof(Params, flower_energy_consumption)},
    {"AUBURN_ENERGY_CONSUMPTION", 'd', offsetof(Params, auburn_energy_consumption)},
    {"INDIGO_ENERGY_CONSUMPTION", 'd', offsetof(Params, indigo_energy_consumption)},
    {"LAVENDER_ENERGY_CONSUMPTION", 'd', offsetof(Params, lavender_energy_consumption)},
    {"FALLOW_ENERGY_CONSUMPTION", 'd', offsetof(Params, fallow_energy_consumption)},
    {"SPORE_ENERGY_CONSUMPTION", 'd', offsetof(Params, spore_energy_consumption)},
    {"PLAGUE_ENERGY_CONSUMPTION", 'd', offsetof(Params, plague_energy_consumption)},
    {"SCOURGE_ENERGY_CONSUMPTION", 'd', offsetof(Params, scourge_energy_consumption)},
    {"SPIKE_ENERGY_CONSUMPTION", 'd', offsetof(Params, spike_energy_consumption)},
    {"RED_PROB", 'i', offsetof(Params, red_prob)},
    {"GREEN_PROB", 'i', offsetof(Params, green_prob)},
    {"BLUE_PROB", 'i', offsetof(Params, blue_prob)},
    {"CYAN_PROB", 'i', offsetof(Params, cyan_prob)},
    {"WHITE_PROB", 'i', offsetof(Params, white_prob)},
    {"GRAY_PROB", 'i', offsetof(Params, gray_prob)},
    {"DARKGRAY_PROB", 'i', offsetof(Params, darkgray_prob)},
    {"SILVER_PROB", 'i', offsetof(Params, silver_prob)},
    {"YELLOW_PROB", 'i', offsetof(Params, yellow_prob)},
    {"MAGENTA_PROB", 'i', offsetof(Params, magenta_prob)},
    {"PINK_PROB", 'i', offsetof(Params, pink_prob)},
    {"CORAL_PROB", 'i', offsetof(Params, coral_prob)},
    {"ORANGE_PROB", 'i', offsetof(Params, orange_prob)},
    {"FOREST_PROB", 'i', offsetof(Params, forest_prob)},
    {"SPRING_PROB", 'i', offsetof(Params, spring_prob)},
    {"LEAF_PROB", 'i', offsetof(Params, leaf_prob)},
    {"LIME_PROB", 'i', offsetof(Params, lime_prob)},
    {"SUMMER_PROB", 'i', offsetof(Params, summer_prob)},
    {"BARK_PROB", 'i', offsetof(Params, bark_prob)},
    {"JADE_PROB", 'i', offsetof(Params, jade_prob)},
    {"GRASS_PROB", 'i', offsetof(Params, grass_prob)},
    {"PURPLE_PROB", 'i', offsetof(Params, purple_prob)},
    {"C4_PROB", 'i', offsetof(Params, c4_prob)},
    {"VIOLET_PROB", 'i', offsetof(Params, violet_prob)},
    {"TEAL_PROB", 'i', offsetof(Params, teal_prob)},
    {"EYE_PROB", 'i', offsetof(Params, eye_prob)},
    {"MAROON_PROB", 'i', offsetof(Params, maroon_prob)},
    {"OLIVE_PROB", 'i', offsetof(Params, olive_prob)},
    {"MINT_PROB", 'i', offsetof(Params, mint_prob)},
    {"CREAM_PROB", 'i', offsetof(Params, cream_prob)},
    {"ROSE_PROB", 'i', offsetof(Params, rose_prob)},
    {"DARK_PROB", 'i', offsetof(Params, dark_prob)},
    {"OCHRE_PROB", 'i', offsetof(Params, ochre_prob)},
    {"SKY_PROB", 'i', offsetof(Params, sky_prob)},
    {"LILAC_PROB", 'i', offsetof(Params, lilac_prob)},
    {"FIRE_PROB", 'i', offsetof(Params, fire_prob)},
    {"GOLD_PROB", 'i', offsetof(Params, gold_prob)},
    {"BLOND_PROB", 'i', offsetof(Params, blond_prob)},
    {"FLOWER_PROB", 'i', offsetof(Params, flower_prob)},
    {"AUBURN_PROB", 'i', offsetof(Params, auburn_prob)},
    {"INDIGO_PROB", 'i', offsetof(Params, indigo_prob)},
    {"LAVENDER_PROB", 'i', offsetof(Params, lavender_prob)},
    {"FALLOW_PROB", 'i', offsetof(Params, fallow_prob)},
    {"SPORE_PROB", 'i', offsetof(Params, spore_prob)},
    {"PLAGUE_PROB", 'i', offsetof(Params, plague_prob)},
    {"SPIKE_PROB", 'i', offsetof(Params, spike_prob)},
    {"DRAIN_SUBS_DIVISOR", 'i', offsetof(Params, drain_subs_divisor)},
    {"INITIAL_ENERGY", 'i', offsetof(Params, initial_energy)},
    {"MAX_VEL", 'd', offsetof(Params, max_vel)},
    {"MAX_ROT", 'd', offsetof(Params, max_rot)},
    {"ELASTICITY", 'd', offsetof(Params, elasticity)},
    {"DELAY", 'i', offsetof(Params, delay)},
    {"AUTO_BACKUP", 'b', offsetof(Params, auto_backup)},
    {"BACKUP_DELAY", 'i', offsetof(Params, backup_delay)},
    {"DECAY_ENERGY", 'd', offsetof(Params, decay_energy)},
};

const char *params_config_dir(void)
{
    static char dir[1024];
    if (dir[0])
        return dir;
    const char *xdg = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");
    if (xdg && *xdg)
        snprintf(dir, sizeof dir, "%s/syn-biogenesis", xdg);
    else if (home && *home)
        snprintf(dir, sizeof dir, "%s/.config/syn-biogenesis", home);
    else
        snprintf(dir, sizeof dir, ".");
    return dir;
}

static void params_path(char *out, size_t n)
{
    snprintf(out, n, "%s/colour-params.cfg", params_config_dir());
}

void params_load(void)
{
    char path[1100];
    params_path(path, sizeof path);
    FILE *f = fopen(path, "r");
    if (!f)
        return;
    char line[256];
    while (fgets(line, sizeof line, f)) {
        char *eq = strchr(line, '=');
        if (!eq || line[0] == '#')
            continue;
        *eq = '\0';
        const char *val = eq + 1;
        for (size_t i = 0; i < sizeof k_fields / sizeof k_fields[0]; i++) {
            if (strcmp(line, k_fields[i].key) != 0)
                continue;
            char *base = (char *)&P + k_fields[i].offset;
            char *end;
            errno = 0;
            if (k_fields[i].type == 'i') {
                long v = strtol(val, &end, 10);
                if (end != val && !errno)
                    *(int *)base = (int)v;
            } else if (k_fields[i].type == 'b') {
                *(bool *)base = !strncmp(val, "true", 4);
            } else {
                double v = strtod(val, &end);
                if (end != val && !errno && isfinite(v))
                    *(double *)base = v;
            }
            break;
        }
    }
    fclose(f);
}

bool params_save(void)
{
    const char *dir = params_config_dir();
    char parent[1024];
    snprintf(parent, sizeof parent, "%s", dir);
    char *slash = strrchr(parent, '/');
    if (slash) {
        *slash = '\0';
        mkdir(parent, 0755);
    }
    mkdir(dir, 0755);
    char path[1100];
    params_path(path, sizeof path);
    FILE *f = fopen(path, "w");
    if (!f)
        return false;
    fprintf(f, "# SYN-BIOGENESIS colour build parameters (Color Mod 2.01)\n");
    for (size_t i = 0; i < sizeof k_fields / sizeof k_fields[0]; i++) {
        const char *base = (const char *)&P + k_fields[i].offset;
        if (k_fields[i].type == 'i')
            fprintf(f, "%s=%d\n", k_fields[i].key, *(const int *)base);
        else if (k_fields[i].type == 'b')
            fprintf(f, "%s=%s\n", k_fields[i].key, *(const bool *)base ? "true" : "false");
        else
            fprintf(f, "%s=%.17g\n", k_fields[i].key, *(const double *)base);
    }
    return fclose(f) == 0;
}
