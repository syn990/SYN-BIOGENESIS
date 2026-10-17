/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Translated to C in 2026 from Utils.java
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

/* Simulation parameters: the tunables of Utils.java and their defaults.
 *
 * Like the original's static fields they are global: the world, organisms
 * and genetic codes all read them directly every frame, and the Parameters
 * dialog edits them in place. Persisted as key=value text in
 * $XDG_CONFIG_HOME/syn-biogenesis/params.cfg (Java used Preferences).
 */
#ifndef SIM_PARAMS_H
#define SIM_PARAMS_H

#include <stdbool.h>

#include "sim/jgeom.h"
#include "sim/jrandom.h"

typedef struct {
    int window_width, window_height;
    int initial_organisms;
    double initial_o2;
    double initial_co2;
    int world_width, world_height;
    int max_age;
    double rubbing;
    double mutation_rate;
    int segment_cost_divisor;
    double organic_obtained_energy;
    int green_obtained_energy_divisor;
    double organic_subs_produced;
    double red_energy_consumption;
    double green_energy_consumption;
    double blue_energy_consumption;
    double cyan_energy_consumption;
    double white_energy_consumption;
    double gray_energy_consumption;
    double yellow_energy_consumption;
    int red_prob, green_prob, blue_prob, cyan_prob, white_prob, gray_prob, yellow_prob;
    int drain_subs_divisor;
    int initial_energy;
    double max_vel;
    double max_rot;
    double elasticity;
    int delay; /* milliseconds per frame */
    double decay_energy;
} Params;

extern Params P;
/* Utils.random: the one random stream every simulation decision draws from. */
extern JRandom g_rng;

#define BIO_TOL 0.0000001

extern const int k_side[8][2];
extern const double k_scale[16];

void params_set_defaults(Params *p);
/* Load/save the user's parameters. Missing file or keys keep defaults. */
void params_load(void);
bool params_save(void);
const char *params_config_dir(void);

/* Utils helpers that draw from g_rng */
int bio_random_sign(void);
bool bio_random_mutation(void);

static inline int bio_between_i(int value, int min, int max)
{
    int a = value < max ? value : max;
    return a > min ? a : min;
}

/* Utils.between: Math.max(Math.min(max, value), min) */
static inline double bio_between(double value, double min, double max)
{
    return jmax(jmin(max, value), min);
}

#endif
