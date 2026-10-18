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

/* The simulation parameters and constants of Color Mod's Utils.java.
 *
 * As in Java they are global: the world, organisms and genetic codes read
 * them directly, and the Parameters dialog edits them in place. Field
 * names are Utils' own in lower case (Utils.RED_PROB is P.red_prob).
 * Persisted as key=value text in $XDG_CONFIG_HOME/syn-biogenesis/
 * colour-params.cfg under Utils' preference keys (Java used Preferences).
 */
#ifndef COLOUR_PARAMS_H
#define COLOUR_PARAMS_H

#include <stdbool.h>

#include "sim/jrt.h"

typedef struct {
    int window_x;
    int window_y;
    int window_width;
    int window_height;
    int window_state;
    int initial_organisms;
    int initial_complexity;
    int clade_complexity;
    double initial_o2;
    double initial_co2;
    double initial_ch4;
    int organisms_vector_size;
    int world_width;
    int world_height;
    int max_age;
    int age_divisor;
    int co2_to_ch4_divisor;
    int ch4_to_co2_divisor;
    double rubbing;
    int meta_mutation_rate;
    int max_mutation_rate;
    int min_mutation_rate;
    int segment_cost_divisor;
    double organic_obtained_energy;
    int green_obtained_energy_divisor;
    double organic_subs_produced;
    double cream_organic_subs_produced;
    int healing;
    int immune_system;
    int lavender_shield;
    double indigo_divisor;
    double gold_divisor;
    double dodge_energy_consumption;
    int darkjade_delay;
    double red_energy_consumption;
    double green_energy_consumption;
    double blue_energy_consumption;
    double cyan_energy_consumption;
    double white_energy_consumption;
    double virus_energy_consumption;
    double gray_energy_consumption;
    double darkgray_energy_consumption;
    double silver_energy_consumption;
    double yellow_energy_consumption;
    double magenta_energy_consumption;
    double pink_energy_consumption;
    double coral_energy_consumption;
    double orange_energy_consumption;
    double forest_energy_consumption;
    double crowdedforest_energy_consumption;
    double symbiont_energy_consumption;
    double spring_energy_consumption;
    double leaf_energy_consumption;
    double lime_energy_consumption;
    double crowdedlime_energy_consumption;
    double summer_energy_consumption;
    double mosquito_energy_consumption;
    double bark_energy_consumption;
    double jade_energy_consumption;
    double grass_energy_consumption;
    double purple_energy_consumption;
    double c4_energy_consumption;
    double violet_energy_consumption;
    double teal_energy_consumption;
    double eye_energy_consumption;
    double maroon_energy_consumption;
    double olive_energy_consumption;
    double mint_energy_consumption;
    double cream_energy_consumption;
    double rose_energy_consumption;
    double dark_energy_consumption;
    double ochre_energy_consumption;
    double sky_energy_consumption;
    double lilac_energy_consumption;
    double fire_energy_consumption;
    double experience_energy_consumption;
    double blond_energy_consumption;
    double flower_energy_consumption;
    double auburn_energy_consumption;
    double indigo_energy_consumption;
    double lavender_energy_consumption;
    double fallow_energy_consumption;
    double spore_energy_consumption;
    double plague_energy_consumption;
    double scourge_energy_consumption;
    double spike_energy_consumption;
    int red_prob;
    int green_prob;
    int blue_prob;
    int cyan_prob;
    int white_prob;
    int gray_prob;
    int darkgray_prob;
    int silver_prob;
    int yellow_prob;
    int magenta_prob;
    int pink_prob;
    int coral_prob;
    int orange_prob;
    int forest_prob;
    int spring_prob;
    int leaf_prob;
    int lime_prob;
    int summer_prob;
    int bark_prob;
    int jade_prob;
    int grass_prob;
    int purple_prob;
    int c4_prob;
    int violet_prob;
    int teal_prob;
    int eye_prob;
    int maroon_prob;
    int olive_prob;
    int mint_prob;
    int cream_prob;
    int rose_prob;
    int dark_prob;
    int ochre_prob;
    int sky_prob;
    int lilac_prob;
    int fire_prob;
    int gold_prob;
    int blond_prob;
    int flower_prob;
    int auburn_prob;
    int indigo_prob;
    int lavender_prob;
    int fallow_prob;
    int spore_prob;
    int plague_prob;
    int spike_prob;
    int drain_subs_divisor;
    int initial_energy;
    double max_vel;
    double max_rot;
    double elasticity;
    int delay;
    bool auto_backup;
    int backup_delay;
    int local_port;
    int max_connections;
    bool accept_connections;
    bool connect_to_server;
    int hardware_acceleration;
    int server_port;
    double decay_energy;
} Params;

extern Params P;
/* Utils.random: the one random stream every simulation decision draws from. */
extern JRandom g_rng;

#define BIO_TOL 0.0000001
#define U_FILE_VERSION 700
#define U_VERSION 900

extern const int k_side[8][2];
extern const double k_scale[16];
extern const double k_springscale[16];

/* Color Mod's colours (Utils.Color...) */
#define CM_DARK_GREEN JRGB(0, 178, 0)
#define CM_DARK_RED JRGB(178, 0, 0)
#define CM_DARK_CYAN JRGB(0, 178, 178)
#define CM_DARK_BLUE JRGB(0, 0, 178)
#define CM_DARK_MAGENTA JRGB(178, 0, 178)
#define CM_DARK_PINK JRGB(178, 122, 122)
#define CM_DARK_ORANGE JRGB(178, 140, 0)
#define CM_DARK_WHITE JRGB(178, 178, 178)
#define CM_DARK_GRAY JRGB(89, 89, 89)
#define CM_DARK_YELLOW JRGB(178, 178, 0)
#define CM_BROWN JRGB(150, 75, 0)
#define CM_LIGHTBROWN JRGB(128, 112, 64)
#define CM_GREENBROWN JRGB(128, 132, 64)
#define CM_DARKGREEN JRGB(54, 84, 54)
#define CM_LIGHT_BLUE JRGB(0, 0, 128)
#define CM_VIOLET JRGB(128, 0, 128)
#define CM_FOREST JRGB(0, 128, 0)
#define CM_BARK JRGB(96, 128, 64)
#define CM_OLDBARK JRGB(80, 40, 0)
#define CM_DEADBARK JRGB(128, 96, 64)
#define CM_JADE JRGB(0, 168, 107)
#define CM_SPRING JRGB(0, 255, 128)
#define CM_LEAF JRGB(92, 184, 0)
#define CM_LIME JRGB(176, 255, 0)
#define CM_SUMMER JRGB(128, 255, 64)
#define CM_WINTER JRGB(54, 84, 0)
#define CM_TEAL JRGB(0, 128, 128)
#define CM_EYE JRGB(0, 64, 64)
#define CM_VISION JRGB(232, 145, 70)
#define CM_MINT JRGB(160, 224, 160)
#define CM_MAROON JRGB(128, 0, 0)
#define CM_OLIVE JRGB(176, 176, 0)
#define CM_DARKOLIVE JRGB(88, 88, 0)
#define CM_CREAM JRGB(208, 192, 140)
#define CM_ROSE JRGB(255, 0, 128)
#define CM_CORAL JRGB(255, 100, 138)
#define CM_OCHRE JRGB(204, 119, 34)
#define CM_SKY JRGB(128, 192, 255)
#define CM_DEEPSKY JRGB(64, 96, 255)
#define CM_ICE JRGB(64, 96, 128)
#define CM_LILAC JRGB(192, 128, 255)
#define CM_DARKLILAC JRGB(96, 64, 96)
#define CM_FIRE JRGB(255, 100, 0)
#define CM_DARKFIRE JRGB(232, 146, 70)
#define CM_GRASS JRGB(144, 176, 64)
#define CM_PURPLE JRGB(168, 0, 84)
#define CM_C4 JRGB(96, 192, 96)
#define CM_DARKJADE JRGB(0, 84, 54)
#define CM_GOLD JRGB(212, 175, 55)
#define CM_BLOND JRGB(255, 255, 128)
#define CM_FLOWER JRGB(128, 128, 255)
#define CM_AUBURN JRGB(128, 48, 48)
#define CM_INDIGO JRGB(111, 0, 255)
#define CM_LAVENDER JRGB(128, 96, 176)
#define CM_FALLOW JRGB(150, 113, 23)
#define CM_SPORE JRGB(0, 80, 160)
#define CM_FRUIT JRGB(255, 128, 192)
#define CM_PLAGUE JRGB(255, 192, 255)
#define CM_SPIKE JRGB(164, 132, 100)
#define CM_SPIKEPOINT JRGB(164, 132, 99)
#define CM_BROKEN JRGB(96, 128, 96)
#define CM_DARK JRGB(56, 28, 14)

void params_set_defaults(Params *p);
/* Load/save the user's parameters. Missing file or keys keep defaults. */
void params_load(void);
bool params_save(void);
const char *params_config_dir(void);

/* Utils helpers that draw from g_rng */
int bio_random_sign(void);
bool bio_random_mutation(void);

/* Utils.between(int, int, int) */
static inline int bio_between_i(int value, int min, int max)
{
    return j_imax(j_imin(max, value), min);
}

/* Utils.between(double, double, double) */
static inline double bio_between(double value, double min, double max)
{
    return jmax(jmin(max, value), min);
}

#endif
