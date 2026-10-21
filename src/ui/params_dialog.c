/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Based on ParamDialog.java
 * of Biogenesis 0.9 (https://biogenesis.sourceforge.net/),
 * Copyright (C) 2006-2010 Joan Queralt Molina;
 * rewritten for SDL2 in 2026.
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

/* Parameters configuration (ParamDialog.java). Fields are text, parsed on
 * OK; a value that doesn't parse or is out of range keeps the old one,
 * with the same limits as the original. */
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sim/params.h"
#include "ui/app.h"
#ifdef BIO_SYN_EDITION
#include "syn/genesis_screen.h"
#endif

typedef enum { F_INT, F_DBL } FType;

typedef enum {
    R_POS,  /* > 0 */
    R_NNEG, /* >= 0 */
    R_UNIT, /* 0..1 */
} Range;

typedef struct {
    int tab;
    const char *label;
    FType type;
    size_t off;
    Range range;
} Field;

#define FI(tab, label, name, range) {tab, label, F_INT, offsetof(Params, name), range}
#define FD(tab, label, name, range) {tab, label, F_DBL, offsetof(Params, name), range}

enum { T_GENERAL, T_WORLD, T_ORGANISMS, T_METABOLISM, T_GENES };

static const Field k_fields[] = {
    FI(T_GENERAL, "Time per frame (milliseconds):", delay, R_POS),
    FI(T_WORLD, "Width:", world_width, R_POS),
    FI(T_WORLD, "Height:", world_height, R_POS),
    FD(T_WORLD, "Initial oxygen:", initial_o2, R_NNEG),
    FD(T_WORLD, "Initial carbon dioxide:", initial_co2, R_NNEG),
    FD(T_WORLD, "Rubbing coefficient:", rubbing, R_UNIT),
    FD(T_WORLD, "Elasticity coefficient:", elasticity, R_UNIT),
    FI(T_ORGANISMS, "Initial number:", initial_organisms, R_NNEG),
    FI(T_ORGANISMS, "Initial energy:", initial_energy, R_POS),
    FI(T_ORGANISMS, "Life expectancy:", max_age, R_POS),
    FD(T_ORGANISMS, "Mutation rate (0-1):", mutation_rate, R_UNIT),
    FI(T_ORGANISMS, "Segment upkeep cost divisor:", segment_cost_divisor, R_POS),
    FI(T_ORGANISMS, "Substances drainage divisor:", drain_subs_divisor, R_POS),
    FD(T_ORGANISMS, "Energy lost by dead organisms:", decay_energy, R_NNEG),
    FI(T_METABOLISM, "Photosynthesis: obtained energy divisor:", green_obtained_energy_divisor,
       R_POS),
    FD(T_METABOLISM, "Chemoorganotrophy: obtained energy:", organic_obtained_energy, R_NNEG),
    FD(T_METABOLISM, "Chemoorganotrophy: released energy proportion:", organic_subs_produced,
       R_UNIT),
    /* genes tab: probability and cost per colour, in table order */
    FI(T_GENES, "red", red_prob, R_NNEG),
    FD(T_GENES, "red", red_energy_consumption, R_NNEG),
    FI(T_GENES, "green", green_prob, R_POS),
    FD(T_GENES, "green", green_energy_consumption, R_NNEG),
    FI(T_GENES, "blue", blue_prob, R_NNEG),
    FD(T_GENES, "blue", blue_energy_consumption, R_NNEG),
    FI(T_GENES, "cyan", cyan_prob, R_NNEG),
    FD(T_GENES, "cyan", cyan_energy_consumption, R_NNEG),
    FI(T_GENES, "white", white_prob, R_NNEG),
    FD(T_GENES, "white", white_energy_consumption, R_NNEG),
    FI(T_GENES, "gray", gray_prob, R_NNEG),
    FD(T_GENES, "gray", gray_energy_consumption, R_NNEG),
    FI(T_GENES, "yellow", yellow_prob, R_NNEG),
    FD(T_GENES, "yellow", yellow_energy_consumption, R_NNEG),
};
#define N_FIELDS ((int)(sizeof k_fields / sizeof k_fields[0]))

static void fmt_field(char *out, size_t n, const Field *f, const Params *p)
{
    const char *base = (const char *)p + f->off;
    if (f->type == F_INT) {
        snprintf(out, n, "%d", *(const int *)base);
    } else {
        double v = *(const double *)base;
        /* shortest text that reads back as the same double */
        for (int prec = 1; prec <= 17; prec++) {
            snprintf(out, n, "%.*g", prec, v);
            if (strtod(out, NULL) == v)
                break;
        }
    }
}

static void load_fields(ParamsDialog *d, const Params *p)
{
    for (int i = 0; i < N_FIELDS; i++)
        fmt_field(d->f[i], sizeof d->f[i], &k_fields[i], p);
}

void params_open(App *a)
{
    ParamsDialog *d = &a->params;
    load_fields(d, &P);
    ui_window_centre(&d->win, 620, 470);
    d->win.open = true;
}

static bool in_range(double v, Range r)
{
    switch (r) {
    case R_POS: return v > 0;
    case R_NNEG: return v >= 0;
    case R_UNIT: return v >= 0 && v <= 1;
    }
    return false;
}

static void apply(ParamsDialog *d)
{
    for (int i = 0; i < N_FIELDS; i++) {
        const Field *f = &k_fields[i];
        char *base = (char *)&P + f->off;
        char *end;
        errno = 0;
        if (f->type == F_INT) {
            long v = strtol(d->f[i], &end, 10);
            if (end != d->f[i] && !*end && !errno && v <= 2147483647L && in_range((double)v, f->range))
                *(int *)base = (int)v;
        } else {
            double v = strtod(d->f[i], &end);
            if (end != d->f[i] && !*end && !errno && isfinite(v) && in_range(v, f->range))
                *(double *)base = v;
        }
    }
    params_save();
}

void params_draw(App *a)
{
    ParamsDialog *d = &a->params;
    if (!d->win.open)
        return;
    SDL_Rect c =
        ui_window_begin(&d->win, ui_id("params", 0), "Parameters configuration", true);
    int fh = gfx_font_h(), rh = fh + 10;
    static const char *const tabs[] = {"General", "World", "Organisms", "Metabolism", "Genes"};
    ui_tabs(ui_id("ptabs", 0), (SDL_Rect){c.x, c.y, c.w, rh}, tabs, 5, &d->tab);
    int y = c.y + rh + 14;
    if (d->tab == T_GENES) {
        ui_label_dim(c.x + 10, y, "Color");
        ui_label_dim(c.x + 140, y, "Probability");
        ui_label_dim(c.x + 300, y, "Cost");
        y += fh + 10;
    }
    for (int i = 0; i < N_FIELDS; i++) {
        const Field *f = &k_fields[i];
        if (f->tab != d->tab)
            continue;
        if (d->tab == T_GENES) {
            /* two fields per row: probability, then cost */
            if (f->type == F_INT) {
                ui_label(c.x + 10, y + 5, f->label);
                ui_textfield(ui_id("pf", (uint32_t)i), (SDL_Rect){c.x + 140, y, 120, rh},
                             d->f[i], sizeof d->f[i]);
            } else {
                ui_textfield(ui_id("pf", (uint32_t)i), (SDL_Rect){c.x + 300, y, 120, rh},
                             d->f[i], sizeof d->f[i]);
                y += rh + 6;
            }
            continue;
        }
        ui_label(c.x + 10, y + 5, f->label);
        ui_textfield(ui_id("pf", (uint32_t)i), (SDL_Rect){c.x + 360, y, 160, rh}, d->f[i],
                     sizeof d->f[i]);
        y += rh + 8;
#ifdef BIO_SYN_EDITION
        /* how long a world of that many takes to make, when it is long */
        if (f->off == offsetof(Params, initial_organisms))
            y += syn_genesis_warning(c.x + 10, y, d->f[i], true);
#endif
    }
    if (d->tab == T_GENERAL) {
        y += 6;
        ui_label_dim(c.x + 10, y, "The window shows the frames per second actually reached.");
        ui_label_dim(c.x + 10, y + fh + 4,
                     "1 ms runs the simulation as fast as this computer allows.");
    }
    if (d->tab == T_WORLD) {
        y += 6;
        ui_label_dim(c.x + 10, y, "Size and atmosphere apply to the next new world.");
    }

    int by = c.y + c.h - rh;
    bool ok = ui_button(ui_id("p_ok", 0), (SDL_Rect){c.x + c.w - 330, by, 100, rh}, "OK", true);
    bool cancel =
        ui_button(ui_id("p_cancel", 0), (SDL_Rect){c.x + c.w - 220, by, 100, rh}, "Cancel", true);
    if (ui_button(ui_id("p_def", 0), (SDL_Rect){c.x + c.w - 110, by, 110, rh}, "Default values",
                  true)) {
        Params def;
        params_set_defaults(&def);
        load_fields(d, &def);
    }
    if (!ui_text_focused() && ui_key_pressed(SDLK_ESCAPE))
        cancel = true;
    ui_window_end();
    if (ok) {
        ui_unfocus();
        apply(d);
        d->win.open = false;
    } else if (cancel) {
        d->win.open = false;
    }
}

#ifdef BIO_SYN_EDITION
/* ---- the same fields, for SYN-OS's PARAMETERS screen ---- */

static const char *const k_tab_names[] = {"GENERAL", "WORLD", "ORGANISMS", "METABOLISM", "GENES"};

/* what each does, as the simulation uses it (src/sim/), in k_fields' order */
static const char *const k_help[N_FIELDS] = {
    "How long a frame takes at least. 1 runs the world as fast as this computer allows; the bar's "
    "speed is this setting.",
    "The width of a new world.",
    "The height of a new world.",
    "The oxygen a new world starts with. Organisms breathe it into carbon dioxide.",
    "The carbon dioxide a new world starts with. Green segments turn it back into oxygen and energy.",
    "The share of its speed an organism keeps each frame: 1 never slows down, 0 stops at once.",
    "How much of their speed two organisms keep when they collide: 1 bounces off fully.",
    "How many organisms a new world is made with.",
    "The energy each of them starts with, no more than the world's carbon dioxide.",
    "How long an organism lives, in time units of 256 frames.",
    "The chance of each mutation when an organism is born: 0 never, 1 always.",
    "Each frame an organism breathes its mass divided by this. Higher is cheaper to live.",
    "Below this much carbon dioxide, photosynthesis slows in proportion.",
    "The energy a corpse loses each frame until it is gone.",
    "The light a green segment turns into energy is divided by this: higher, less energy.",
    "A red segment's bite takes its mass times this from the other organism's energy.",
    "The share of what a bite takes that the biter spends again.",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

int params_count(void)
{
    return N_FIELDS;
}

int params_tab_count(void)
{
    return 5;
}

const char *params_tab(int t)
{
    return t >= 0 && t < 5 ? k_tab_names[t] : "";
}

bool params_tab_genes(int t)
{
    return t == T_GENES;
}

const char *params_tab_note(int t)
{
    if (t == T_GENES)
        return "Probability: how often a random gene is each colour, against the others. Cost: the "
               "energy each act of that colour uses: a red bite, a green's photosynthesis, a blue "
               "block, a cyan push, a white infection, a gray kill, a yellow child.";
    if (t == T_WORLD)
        return "Size and atmosphere apply to the next new world.";
    return NULL;
}

void params_info(int i, ParamsField *out)
{
    const Field *f = &k_fields[i];
    size_t o = f->off;
    *out = (ParamsField){
        .tab = f->tab,
        .label = f->label,
        .help = k_help[i],
        .type = f->type == F_INT ? 'i' : 'd',
        .next_world = o == offsetof(Params, world_width) || o == offsetof(Params, world_height) ||
                      o == offsetof(Params, initial_o2) || o == offsetof(Params, initial_co2) ||
                      o == offsetof(Params, initial_organisms) || o == offsetof(Params, initial_energy),
        .genesis = o == offsetof(Params, initial_organisms),
    };
}

char *params_text(App *a, int i)
{
    return a->params.f[i];
}

size_t params_text_cap(void)
{
    return sizeof ((ParamsDialog *)0)->f[0];
}

/* as apply reads it */
static bool read_field(const Field *f, const char *t, double *v)
{
    char *end;
    errno = 0;
    if (f->type == F_INT) {
        long n = strtol(t, &end, 10);
        *v = (double)n;
        return end != t && !*end && !errno && n <= 2147483647L && in_range(*v, f->range);
    }
    *v = strtod(t, &end);
    return end != t && !*end && !errno && isfinite(*v) && in_range(*v, f->range);
}

bool params_valid(App *a, int i)
{
    double v;
    return read_field(&k_fields[i], a->params.f[i], &v);
}

bool params_changed(App *a, int i)
{
    char now[32];
    fmt_field(now, sizeof now, &k_fields[i], &P);
    return strcmp(now, a->params.f[i]) != 0;
}

void params_default_text(int i, char *out, size_t n)
{
    Params def;
    params_set_defaults(&def);
    fmt_field(out, n, &k_fields[i], &def);
}

bool params_is_default(App *a, int i)
{
    char def[32];
    params_default_text(i, def, sizeof def);
    return strcmp(def, a->params.f[i]) == 0;
}

void params_reload(App *a)
{
    load_fields(&a->params, &P);
}

void params_defaults(App *a)
{
    Params def;
    params_set_defaults(&def);
    load_fields(&a->params, &def);
}

void params_commit(App *a)
{
    apply(&a->params);
    /* what was taken, as it now reads; a refused text stays to be put right */
    for (int i = 0; i < N_FIELDS; i++)
        if (params_valid(a, i))
            fmt_field(a->params.f[i], sizeof a->params.f[i], &k_fields[i], &P);
}
#endif
