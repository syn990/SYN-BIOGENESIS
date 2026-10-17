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
const double k_scale[16] = {1.00, 1.12, 1.25, 1.40, 1.57, 1.76, 1.97, 2.21,
                            2.47, 2.77, 3.11, 3.48, 3.90, 4.36, 4.89, 5.47};

void params_set_defaults(Params *p)
{
    p->window_width = 1024;
    p->window_height = 768;
    p->initial_organisms = 15;
    p->initial_o2 = 0;
    p->initial_co2 = 5000;
    p->world_width = 1000;
    p->world_height = 1000;
    p->max_age = 30;
    p->rubbing = 0.98;
    p->mutation_rate = 0.05;
    p->segment_cost_divisor = 5000;
    p->organic_obtained_energy = 0.5;
    p->green_obtained_energy_divisor = 500;
    p->organic_subs_produced = 0.1;
    p->red_energy_consumption = 0;
    p->green_energy_consumption = 0;
    p->blue_energy_consumption = 0;
    p->cyan_energy_consumption = 0;
    p->white_energy_consumption = 1;
    p->gray_energy_consumption = 1;
    p->yellow_energy_consumption = 0;
    p->red_prob = 10;
    p->green_prob = 30;
    p->blue_prob = 10;
    p->cyan_prob = 20;
    p->white_prob = 10;
    p->gray_prob = 10;
    p->yellow_prob = 10;
    p->drain_subs_divisor = 5000;
    p->initial_energy = 50;
    p->max_vel = 5;
    p->max_rot = M_PI / 16;
    p->elasticity = 0.8;
    p->delay = 50;
    p->decay_energy = 0.1;
}

int bio_random_sign(void)
{
    return (jr_next_int(&g_rng, 2) << 1) - 1;
}

bool bio_random_mutation(void)
{
    return jr_next_double(&g_rng) < P.mutation_rate;
}

/* ---- persistence ---- */

typedef struct {
    const char *key;
    char type; /* 'i' or 'd' */
    size_t offset;
} ParamField;

#define PI_(name) {#name, 'i', offsetof(Params, name)}
#define PD_(name) {#name, 'd', offsetof(Params, name)}

static const ParamField k_fields[] = {
    PI_(window_width), PI_(window_height), PI_(initial_organisms), PD_(initial_o2),
    PD_(initial_co2), PI_(world_width), PI_(world_height), PI_(max_age), PD_(rubbing),
    PD_(mutation_rate), PI_(segment_cost_divisor), PD_(organic_obtained_energy),
    PI_(green_obtained_energy_divisor), PD_(organic_subs_produced),
    PD_(red_energy_consumption), PD_(green_energy_consumption), PD_(blue_energy_consumption),
    PD_(cyan_energy_consumption), PD_(white_energy_consumption), PD_(gray_energy_consumption),
    PD_(yellow_energy_consumption), PI_(red_prob), PI_(green_prob), PI_(blue_prob),
    PI_(cyan_prob), PI_(white_prob), PI_(gray_prob), PI_(yellow_prob), PI_(drain_subs_divisor),
    PI_(initial_energy), PD_(max_vel), PD_(max_rot), PD_(elasticity), PI_(delay),
    PD_(decay_energy),
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
    snprintf(out, n, "%s/params.cfg", params_config_dir());
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
            char *end;
            errno = 0;
            if (k_fields[i].type == 'i') {
                long v = strtol(val, &end, 10);
                if (end != val && !errno)
                    *(int *)((char *)&P + k_fields[i].offset) = (int)v;
            } else {
                double v = strtod(val, &end);
                if (end != val && !errno && isfinite(v))
                    *(double *)((char *)&P + k_fields[i].offset) = v;
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
    fprintf(f, "# SYN-BIOGENESIS parameters\n");
    for (size_t i = 0; i < sizeof k_fields / sizeof k_fields[0]; i++) {
        const char *base = (const char *)&P + k_fields[i].offset;
        if (k_fields[i].type == 'i')
            fprintf(f, "%s=%d\n", k_fields[i].key, *(const int *)base);
        else
            fprintf(f, "%s=%.17g\n", k_fields[i].key, *(const double *)base);
    }
    return fclose(f) == 0;
}
