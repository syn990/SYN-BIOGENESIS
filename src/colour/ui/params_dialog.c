/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Based on ParamDialog.java of Biogenesis Color Mod 2.01
 * (https://sourceforge.net/projects/biogenesis/),
 * Copyright (C) 2006-2010 Joan Queralt Molina,
 * Color Mod Copyright (C) 2012-2022 MarcoDBAA;
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

/* Color Mod's parameters dialog: seven tabs, 132 fields, every one with
 * the check ParamDialog.checkParams makes (params_fields.h, generated from
 * the Java). Fields are text, parsed on OK; a value that doesn't parse or
 * fails its check keeps the old one, as in the original. */
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sim/params.h"
#include "syn/genesis_screen.h"
#include "ui/app.h"

typedef struct {
    int tab;
    int row;
    const char *label;
    const char *suffix;
    char type; /* 'i' or 'd' */
    size_t off;
} Field;

#include "ui/params_fields.h"

_Static_assert(N_FIELDS <= PARAMS_MAX, "ParamsDialog.f is too small");

static void fmt_field(char *out, size_t n, const Field *f, const Params *p)
{
    const char *base = (const char *)p + f->off;
    if (f->type == 'i') {
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

static int g_auto_backup; /* the "Automatic backups" check box: 0 no, 1 yes */

static void load_fields(ParamsDialog *d, const Params *p)
{
    for (int i = 0; i < N_FIELDS; i++)
        fmt_field(d->f[i], sizeof d->f[i], &k_fields[i], p);
    g_auto_backup = p->auto_backup ? 1 : 0;
}

void params_open(App *a)
{
    ParamsDialog *d = &a->params;
    load_fields(d, &P);
    d->scroll = 0;
    ui_window_centre(&d->win, 660, 620);
    d->win.open = true;
}

static void apply(ParamsDialog *d)
{
    _Static_assert(sizeof k_apply_order / sizeof k_apply_order[0] == N_FIELDS,
                   "every field is checked once");
    for (int n = 0; n < N_FIELDS; n++) {
        int i = k_apply_order[n];
        const Field *f = &k_fields[i];
        char *base = (char *)&P + f->off;
        char *end;
        errno = 0;
        if (f->type == 'i') {
            long v = strtol(d->f[i], &end, 10);
            if (end != d->f[i] && !*end && !errno && v <= 2147483647L && v >= -2147483647L - 1 &&
                field_ok(i, (double)v))
                *(int *)base = (int)v;
        } else {
            double v = strtod(d->f[i], &end);
            if (end != d->f[i] && !*end && !errno && isfinite(v) && field_ok(i, v))
                *(double *)base = v;
        }
    }
    P.auto_backup = g_auto_backup == 1;
    params_save();
}

void params_draw(App *a)
{
    ParamsDialog *d = &a->params;
    if (!d->win.open)
        return;
    SDL_Rect c = ui_window_begin(&d->win, ui_id("params", 0), "Parameters configuration", true);
    int fh = gfx_font_h(), rh = fh + 8;
    int old_tab = d->tab;
    ui_tabs(ui_id("ptabs", 0), (SDL_Rect){c.x, c.y, c.w, rh}, k_tabs, N_TABS, &d->tab);
    if (d->tab != old_tab)
        d->scroll = 0;
    SDL_Rect body = {c.x, c.y + rh + 8, c.w, c.h - 2 * rh - 22};
    bool genes = d->tab >= GENE_TABS_FROM;
    /* rows of this tab, scrolled when they don't fit */
    int rows = 0;
    for (int i = 0; i < N_FIELDS; i++)
        if (k_fields[i].tab == d->tab && (genes ? k_fields[i].type == 'i' : true))
            rows++;
    int line = rh + 4;
    /* how long a world of the initial number takes to make, when it is
     * long: under its field, in rows of its own */
    for (int i = 0; i < N_FIELDS; i++)
        if (k_fields[i].tab == d->tab && k_fields[i].off == offsetof(Params, initial_organisms))
            rows += (syn_genesis_warning(0, 0, d->f[i], false) + line - 1) / line;
    int visible = body.h / line - (genes ? 1 : 0) - (d->tab == 0 ? 2 : 0);
    if (ui_hover(body) && ui_wheel_y())
        d->scroll -= ui_wheel_y();
    if (d->scroll > rows - visible)
        d->scroll = rows - visible;
    if (d->scroll < 0)
        d->scroll = 0;
    int y = body.y;
    gfx_clip_push(body);
    if (genes) {
        ui_label_dim(c.x + 20, y + 4, k_gene_tab_heads[0]);
        ui_label_dim(c.x + 200, y + 4, k_gene_tab_heads[1]);
        ui_label_dim(c.x + 380, y + 4, k_gene_tab_heads[2]);
        y += line;
    }
    if (d->tab == 0) {
        ui_label(c.x + 10, y + 4, "Automatic backups:");
        static const char *const noyes[] = {"No", "Yes"};
        ui_combo(ui_id("pautob", 0), (SDL_Rect){c.x + 360, y, 160, rh}, noyes, 2, &g_auto_backup);
        y += line;
    }
    int k = 0;
    for (int i = 0; i < N_FIELDS; i++) {
        const Field *f = &k_fields[i];
        if (f->tab != d->tab)
            continue;
        if (genes) {
            /* a colour per row: its probability, then its cost */
            if (f->type == 'i') {
                if (k++ < d->scroll)
                    continue;
                ui_label(c.x + 20, y + 4, f->label);
                ui_textfield(ui_id("pf", (uint32_t)i), (SDL_Rect){c.x + 200, y, 150, rh}, d->f[i],
                             sizeof d->f[i]);
            } else if (k > d->scroll) {
                ui_textfield(ui_id("pf", (uint32_t)i), (SDL_Rect){c.x + 380, y, 150, rh}, d->f[i],
                             sizeof d->f[i]);
                y += line;
            }
            continue;
        }
        if (k++ < d->scroll)
            continue;
        ui_label(c.x + 10, y + 4, f->label);
        ui_textfield(ui_id("pf", (uint32_t)i), (SDL_Rect){c.x + 360, y, 160, rh}, d->f[i],
                     sizeof d->f[i]);
        if (f->suffix)
            ui_label_dim(c.x + 530, y + 4, f->suffix);
        y += line;
        if (f->off == offsetof(Params, initial_organisms))
            y += syn_genesis_warning(c.x + 10, y, d->f[i], true);
    }
    if (d->tab == 1 && y + 2 * line < body.y + body.h) {
        y += 6;
        ui_label_dim(c.x + 10, y, "Size and atmosphere apply to the next new world.");
    }
    gfx_clip_pop();
    if (rows > visible)
        ui_label_dim(c.x + 10, c.y + c.h - 2 * rh, "Scroll for more");

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

/* ---- the same fields, for SYN-OS's PARAMETERS screen ---- */

/* the "Automatic backups" choice is the field after the last, its text Yes or No */
static char g_backup_text[8] = "No";

static void backup_text(void)
{
    snprintf(g_backup_text, sizeof g_backup_text, "%s", g_auto_backup ? "Yes" : "No");
}

int params_count(void)
{
    return N_FIELDS + 1;
}

int params_tab_count(void)
{
    return N_TABS;
}

const char *params_tab(int t)
{
    static char up[N_TABS][24];
    if (t < 0 || t >= N_TABS)
        return "";
    for (int i = 0; k_tabs[t][i] && i < 23; i++)
        up[t][i] = (char)toupper((unsigned char)k_tabs[t][i]);
    return up[t];
}

bool params_tab_genes(int t)
{
    return t >= GENE_TABS_FROM;
}

const char *params_tab_note(int t)
{
    if (t >= GENE_TABS_FROM)
        return "Probability: how often a random gene is each colour, against the others. Effectivity: "
               "as Color Mod's own dialog has it, for each colour.";
    if (t == 1)
        return "Size and atmosphere apply to the next new world.";
    return NULL;
}

void params_info(int i, ParamsField *out)
{
    if (i == N_FIELDS) {
        *out = (ParamsField){.tab = 0, .label = "Automatic backups:", .type = 'c',
                             .help = "A backup beside the world's file every so often, its time in its name."};
        return;
    }
    const Field *f = &k_fields[i];
    size_t o = f->off;
    *out = (ParamsField){
        .tab = f->tab,
        .label = f->label,
        .unit = f->suffix,
        .type = f->type,
        .next_world = o == offsetof(Params, world_width) || o == offsetof(Params, world_height) ||
                      o == offsetof(Params, initial_o2) || o == offsetof(Params, initial_co2) ||
                      o == offsetof(Params, initial_ch4) || o == offsetof(Params, initial_organisms) ||
                      o == offsetof(Params, initial_energy),
        .genesis = o == offsetof(Params, initial_organisms),
    };
    if (o == offsetof(Params, delay))
        out->help = "How long a frame takes at least. 1 runs the world as fast as this computer allows; the "
                    "bar's speed is this setting.";
}

char *params_text(App *a, int i)
{
    return i == N_FIELDS ? g_backup_text : a->params.f[i];
}

size_t params_text_cap(void)
{
    return sizeof ((ParamsDialog *)0)->f[0];
}

/* as apply reads it */
static bool read_field(int i, const char *t)
{
    const Field *f = &k_fields[i];
    char *end;
    errno = 0;
    if (f->type == 'i') {
        long v = strtol(t, &end, 10);
        return end != t && !*end && !errno && v <= 2147483647L && v >= -2147483647L - 1 &&
               field_ok(i, (double)v);
    }
    double v = strtod(t, &end);
    return end != t && !*end && !errno && isfinite(v) && field_ok(i, v);
}

bool params_valid(App *a, int i)
{
    return i == N_FIELDS || read_field(i, a->params.f[i]);
}

bool params_changed(App *a, int i)
{
    if (i == N_FIELDS)
        return (strcmp(g_backup_text, "Yes") == 0) != P.auto_backup;
    char now[32];
    fmt_field(now, sizeof now, &k_fields[i], &P);
    return strcmp(now, a->params.f[i]) != 0;
}

void params_default_text(int i, char *out, size_t n)
{
    Params def;
    params_set_defaults(&def);
    if (i == N_FIELDS)
        snprintf(out, n, "%s", def.auto_backup ? "Yes" : "No");
    else
        fmt_field(out, n, &k_fields[i], &def);
}

bool params_is_default(App *a, int i)
{
    char def[32];
    params_default_text(i, def, sizeof def);
    return strcmp(def, params_text(a, i)) == 0;
}

void params_reload(App *a)
{
    load_fields(&a->params, &P);
    backup_text();
}

void params_defaults(App *a)
{
    Params def;
    params_set_defaults(&def);
    load_fields(&a->params, &def);
    backup_text();
}

void params_commit(App *a)
{
    g_auto_backup = strcmp(g_backup_text, "Yes") == 0;
    apply(&a->params);
    for (int i = 0; i < N_FIELDS; i++)
        if (read_field(i, a->params.f[i]))
            fmt_field(a->params.f[i], sizeof a->params.f[i], &k_fields[i], &P);
    backup_text();
}
