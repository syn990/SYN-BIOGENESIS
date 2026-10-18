/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Based on LabWindow.java of Biogenesis Color Mod 2.01
 * (https://sourceforge.net/projects/biogenesis/),
 * Copyright (C) 2006-2010 Joan Queralt Molina,
 * Color Mod Copyright (C) 2012-2022 MarcoDBAA;
 * rewritten for SDL2 in 2026 (from the 0.9 port's src/ui/lab.c).
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

/* Color Mod's Genetic Laboratory: a gene sequence (length, rotation,
 * colour, the segment each grows from, and how each reacts to 36 kinds of
 * touch), the code's 22 traits and its mutation rate, a preview, and
 * "Copy to clipboard" to paste the result into the world. The genes are
 * copies, as LabWindow clones them. */
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io/bgg.h"
#include "sim/params.h"
#include "ui/app.h"

/* LabWindow's reaction columns, in its order, as indices into LabGene's
 * reaction[] (Gene's constructor order) with LabWindow's English headers */
static const struct {
    int index;
    const char *label;
} k_react_cols[LAB_REACTIONS] = {
    {1, "green"},      {15, "bark"},     {0, "red"},        {28, "fire"},     {14, "orange"},
    {18, "maroon"},    {12, "pink"},     {21, "cream"},     {7, "silver"},    {22, "spike"},
    {27, "lilac"},     {6, "gray"},      {16, "violet"},    {19, "olive"},    {26, "sky"},
    {2, "blue"},       {25, "ochre"},    {23, "fallow"},    {5, "white"},     {17, "virus"},
    {3, "plague"},     {4, "-virus"},    {13, "coral"},     {20, "mint"},     {11, "magenta"},
    {8, "default"},    {9, "-consumer"}, {10, "-plant"},    {32, "frozen"},   {24, "navy"},
    {29, "beige"},     {30, "wilted"},   {33, "broken"},    {31, "brown"},    {34, "sick"},
    {35, "friend"},
};

static void gene_to_lab(const Gene *g, LabGene *l)
{
    *l = (LabGene){g->length, g->theta, g->color, g->branch, {0}};
    const int r[LAB_REACTIONS] = {
        g->redreaction,        g->greenreaction,    g->bluereaction,     g->plaguereaction,
        g->scourgereaction,    g->whitereaction,    g->grayreaction,     g->silverreaction,
        g->defaultreaction,    g->consumerreaction, g->plantreaction,    g->magentareaction,
        g->pinkreaction,       g->coralreaction,    g->orangereaction,   g->barkreaction,
        g->violetreaction,     g->virusreaction,    g->maroonreaction,   g->olivereaction,
        g->mintreaction,       g->creamreaction,    g->spikereaction,    g->fallowreaction,
        g->lightbluereaction,  g->ochrereaction,    g->skyreaction,      g->lilacreaction,
        g->firereaction,       g->lightbrownreaction, g->greenbrownreaction, g->brownreaction,
        g->icereaction,        g->brokenreaction,   g->sickreaction,     g->friendreaction,
    };
    memcpy(l->reaction, r, sizeof r);
}

static Gene *lab_to_gene(const LabGene *l)
{
    const int *r = l->reaction;
    return gene_new_full(l->length, l->theta, l->color, l->branch, r[0], r[1], r[2], r[3], r[4],
                         r[5], r[6], r[7], r[8], r[9], r[10], r[11], r[12], r[13], r[14], r[15],
                         r[16], r[17], r[18], r[19], r[20], r[21], r[22], r[23], r[24], r[25],
                         r[26], r[27], r[28], r[29], r[30], r[31], r[32], r[33], r[34], r[35]);
}

/* LabWindow's field initialisers */
static void lab_defaults(LabWindow *l)
{
    l->ngenes = 0;
    l->symmetry = 2;
    l->mirror = 0;
    l->mutationrate = (P.min_mutation_rate + P.max_mutation_rate) / 2;
    l->activity = 2;
    l->modifiescream = 2;
    l->modifiesfallow = 2;
    l->modifiesspore = 4;
    l->adaptspore = 4;
    l->plague = l->disperse = l->generation_battle = l->sibling_battle = l->altruist = false;
    l->familial = l->social = l->peaceful = l->passive = l->clockwise = l->mimicall = false;
    l->modifiespink = l->modifieslilac = l->modifiessky = l->selfish = false;
    l->scroll = 0;
    l->open_gene = -1;
}

/* LabWindow.importGeneticCode */
void lab_load_code(LabWindow *l, const GeneticCode *gc)
{
    l->mutationrate = gc->mutationrate;
    l->activity = gc->activity;
    l->modifiescream = gc->modifiescream;
    l->modifiesfallow = gc->modifiesfallow;
    l->modifiesspore = gc->modifiesspore;
    l->adaptspore = gc->adaptspore;
    l->plague = gc->plague;
    l->disperse = gc->disperse_children;
    l->generation_battle = gc->generation_battle;
    l->sibling_battle = gc->sibling_battle;
    l->altruist = gc->altruist;
    l->familial = gc->familial;
    l->social = gc->social;
    l->peaceful = gc->peaceful;
    l->passive = gc->passive;
    l->clockwise = gc->clockwise;
    l->mimicall = gc->mimicall;
    l->modifiespink = gc->modifiespink;
    l->modifieslilac = gc->modifieslilac;
    l->modifiessky = gc->modifiessky;
    l->selfish = gc->selfish;
    l->mirror = gc->mirror;
    l->symmetry = gc->symmetry;
    int n = JLEN(gc->genes);
    l->ngenes = n < LAB_MAX_GENES ? n : LAB_MAX_GENES;
    for (int i = 0; i < l->ngenes; i++)
        gene_to_lab(gc->genes[i], &l->genes[i]);
    l->scroll = 0;
    l->open_gene = -1;
}

void lab_open(App *a, const GeneticCode *gc)
{
    LabWindow *l = &a->lab;
    lab_defaults(l);
    if (gc)
        lab_load_code(l, gc);
    if (!l->win.open || !l->win.centred)
        ui_window_centre(&l->win, 1060, 700);
    l->win.open = true;
}

/* new Gene(2.0, 0.0, Color.GREEN, -1, 0, ...) */
static void insert_gene(LabWindow *l, int at)
{
    if (l->ngenes >= LAB_MAX_GENES)
        return;
    memmove(&l->genes[at + 1], &l->genes[at], (size_t)(l->ngenes - at) * sizeof(LabGene));
    l->genes[at] = (LabGene){.length = 2.0, .theta = 0.0, .color = JC_GREEN, .branch = -1};
    l->ngenes++;
    if (l->open_gene >= at)
        l->open_gene++;
}

static void delete_gene(LabWindow *l, int at)
{
    memmove(&l->genes[at], &l->genes[at + 1], (size_t)(l->ngenes - at - 1) * sizeof(LabGene));
    l->ngenes--;
    if (l->open_gene == at)
        l->open_gene = -1;
    else if (l->open_gene > at)
        l->open_gene--;
}

static GeneticCode *make_code(const LabWindow *l)
{
    Gene *genes[LAB_MAX_GENES];
    for (int i = 0; i < l->ngenes; i++)
        genes[i] = lab_to_gene(&l->genes[i]);
    GeneList list = {genes, l->ngenes};
    return gc_new_from_genes(&list, l->symmetry, l->mirror, l->mutationrate, l->activity,
                             l->modifiescream, l->modifiesfallow, l->modifiesspore, l->adaptspore,
                             l->plague, l->disperse, l->generation_battle, l->sibling_battle,
                             l->altruist, l->familial, l->social, l->peaceful, l->passive,
                             l->clockwise, l->mimicall, l->modifiespink, l->modifieslilac,
                             l->modifiessky, l->selfish);
}

/* a combo whose items are the numbers lo..hi */
static void num_combo(uint32_t id, SDL_Rect r, int lo, int hi, int *v)
{
    static char text[80][4];
    static const char *items[80];
    int n = hi - lo + 1;
    for (int i = 0; i < n; i++) {
        snprintf(text[i], sizeof text[i], "%d", lo + i);
        items[i] = text[i];
    }
    int sel = *v - lo;
    if (ui_combo(id, r, items, n, &sel))
        *v = lo + sel;
}

static void yes_combo(uint32_t id, SDL_Rect r, bool *v)
{
    static const char *const noyes[] = {"No", "Yes"};
    int sel = *v ? 1 : 0;
    if (ui_combo(id, r, noyes, 2, &sel))
        *v = sel == 1;
}

void lab_draw(App *a)
{
    LabWindow *l = &a->lab;
    if (!l->win.open)
        return;
    SDL_Rect c = ui_window_begin(&l->win, ui_id("lab", 0), "Genetic Laboratory", false);
    int fh = gfx_font_h(), rh = fh + 8;
    char buf[64];

    /* general panel: what the code comes to, and its mutation rate */
    int energy = 40 + 3 * l->symmetry * l->ngenes;
    int life = P.max_age + ((l->ngenes * l->symmetry) / P.age_divisor);
    int y = c.y;
    ui_label_dim(c.x, y + 4, "Segments:");
    snprintf(buf, sizeof buf, "%d", l->ngenes * l->symmetry);
    ui_label(c.x + 80, y + 4, buf);
    ui_label_dim(c.x + 160, y + 4, "Energy to reproduce:");
    snprintf(buf, sizeof buf, "%d", energy);
    ui_label(c.x + 310, y + 4, buf);
    ui_label_dim(c.x + 390, y + 4, "Life expectancy:");
    snprintf(buf, sizeof buf, "%d", life);
    ui_label(c.x + 510, y + 4, buf);
    ui_label_dim(c.x + 600, y + 4, "Mutation \xe2\x80\xb1:");
    static char mut[16];
    static int mut_for = -1;
    if (mut_for != l->mutationrate && !ui_text_focused()) {
        snprintf(mut, sizeof mut, "%d", l->mutationrate);
        mut_for = l->mutationrate;
    }
    if (ui_textfield(ui_id("lab_mut", 0), (SDL_Rect){c.x + 720, y, 90, rh}, mut, sizeof mut)) {
        /* accepted within the parameters' mutation range, as LabWindow does */
        char *end;
        errno = 0;
        long v = strtol(mut, &end, 10);
        if (end != mut && !*end && !errno && v >= P.min_mutation_rate && v <= P.max_mutation_rate)
            l->mutationrate = (int)v;
        mut_for = -1;
    }
    y += rh + 6;

    /* the code's traits, in LabWindow's order */
    struct {
        const char *label;
        int kind; /* 0 yes/no, 1 number */
        bool *b;
        int *v, lo, hi;
    } traits[] = {
        {"Symmetry:", 1, NULL, &l->symmetry, 1, 8},
        {"Mirror:", 1, NULL, &l->mirror, 0, 1},
        {"Disperse children:", 0, &l->disperse, NULL, 0, 0},
        {"Selfish Mother:", 0, &l->selfish, NULL, 0, 0},
        {"Familial:", 0, &l->familial, NULL, 0, 0},
        {"Altruist:", 0, &l->altruist, NULL, 0, 0},
        {"Activity:", 1, NULL, &l->activity, 0, 2},
        {"Generation conflict:", 0, &l->generation_battle, NULL, 0, 0},
        {"Sibling rivalry:", 0, &l->sibling_battle, NULL, 0, 0},
        {"Social:", 0, &l->social, NULL, 0, 0},
        {"Peaceful:", 0, &l->peaceful, NULL, 0, 0},
        {"Mod cream:", 1, NULL, &l->modifiescream, 1, 3},
        {"Mod spore:", 1, NULL, &l->modifiesspore, 1, 12},
        {"Mod pink:", 0, &l->modifiespink, NULL, 0, 0},
        {"Mod lilac:", 0, &l->modifieslilac, NULL, 0, 0},
        {"Mod plague:", 0, &l->plague, NULL, 0, 0},
        {"Mod sky:", 0, &l->modifiessky, NULL, 0, 0},
        {"Mod fallow:", 1, NULL, &l->modifiesfallow, 1, 4},
        {"Fine-tune spore:", 1, NULL, &l->adaptspore, 1, 70},
        {"Mod black:", 0, &l->mimicall, NULL, 0, 0},
        {"Passive:", 0, &l->passive, NULL, 0, 0},
        {"Clockwise:", 0, &l->clockwise, NULL, 0, 0},
    };
    int ntraits = (int)(sizeof traits / sizeof traits[0]);
    int tcols = 4, tw = c.w / tcols;
    for (int i = 0; i < ntraits; i++) {
        int tx = c.x + (i % tcols) * tw, ty = y + (i / tcols) * (rh + 3);
        ui_label_dim(tx, ty + 4, traits[i].label);
        SDL_Rect box = {tx + 150, ty, 70, rh};
        uint32_t id = ui_id("lab_trait", (uint32_t)i);
        if (traits[i].kind == 0) {
            yes_combo(id, box, traits[i].b);
        } else if (traits[i].v == &l->mirror) {
            bool m = l->mirror != 0;
            yes_combo(id, box, &m);
            l->mirror = m ? 1 : 0;
        } else {
            num_combo(id, box, traits[i].lo, traits[i].hi, traits[i].v);
        }
    }
    y += ((ntraits + tcols - 1) / tcols) * (rh + 3) + 8;

    /* genes table */
    int bottom = c.y + c.h - rh - 10;
    SDL_Rect table = {c.x, y, 620, bottom - y};
    SDL_Rect side = {table.x + table.w + 10, y, c.w - table.w - 10, bottom - y};
    gfx_fill(table, C_FIELD);
    gfx_frame(table, C_BORDER);
    int cols[] = {table.x + 6,   table.x + 52,  table.x + 146, table.x + 240,
                  table.x + 370, table.x + 446, table.x + 494, table.x + 556};
    ui_label_dim(cols[0], y + 4, "Gene");
    ui_label_dim(cols[1], y + 4, "Length");
    ui_label_dim(cols[2], y + 4, "Rotation");
    ui_label_dim(cols[3], y + 4, "Color");
    ui_label_dim(cols[4], y + 4, "Branch");
    int row_h = rh + 4;
    SDL_Rect rows = {table.x + 1, y + fh + 10, table.w - 2, table.h - fh - 11};
    int visible = rows.h / row_h;
    int total = l->ngenes + 1; /* + the Add row */
    if (ui_hover(table))
        l->scroll -= ui_wheel_y() * 2;
    if (l->scroll > total - visible)
        l->scroll = total - visible;
    if (l->scroll < 0)
        l->scroll = 0;
    static int colours[64];
    static const char *names[64];
    for (int k = 0; k < k_n_gene_colours; k++) {
        colours[k] = (int)k_gene_colours[k].color;
        names[k] = k_gene_colours[k].name;
    }
    gfx_clip_push(rows);
    int act_insert = -1, act_delete = -1;
    for (int i = l->scroll; i < total && i < l->scroll + visible; i++) {
        int ry = rows.y + (i - l->scroll) * row_h;
        if (i == l->ngenes) {
            if (ui_button(ui_id("lab_add", 0), (SDL_Rect){cols[0], ry, 120, rh}, "Add",
                          l->ngenes < LAB_MAX_GENES))
                insert_gene(l, l->ngenes);
            break;
        }
        LabGene *g = &l->genes[i];
        snprintf(buf, sizeof buf, "%d:", i + 1);
        ui_label(cols[0], ry + 4, buf);
        ui_spinner(ui_id("lab_len", (uint32_t)i), (SDL_Rect){cols[1], ry, 88, rh}, &g->length, 2.0,
                   18.0, 0.1, 1, false);
        /* ThetaSpinner: whole degrees 0..359, wrapping */
        double deg = round(g->theta * 180.0 / M_PI);
        while (deg < 0)
            deg += 360;
        while (deg > 359)
            deg -= 360;
        if (ui_spinner(ui_id("lab_theta", (uint32_t)i), (SDL_Rect){cols[2], ry, 88, rh}, &deg, 0,
                       359, 1, 0, true))
            g->theta = deg * M_PI / 180.0;
        int sel = 0;
        for (int k = 0; k < k_n_gene_colours; k++)
            if (k_gene_colours[k].color == g->color)
                sel = k;
        if (ui_color_combo(ui_id("lab_col", (uint32_t)i), (SDL_Rect){cols[3], ry, 124, rh},
                           colours, names, k_n_gene_colours, &sel))
            g->color = k_gene_colours[sel].color;
        double br = g->branch;
        if (ui_spinner(ui_id("lab_branch", (uint32_t)i), (SDL_Rect){cols[4], ry, 70, rh}, &br, -1,
                       999, 1, 0, false))
            g->branch = (int)br;
        bool open = l->open_gene == i;
        if (ui_button(ui_id("lab_react", (uint32_t)i), (SDL_Rect){cols[5], ry, 44, rh},
                      open ? "<<" : ">>", true))
            l->open_gene = open ? -1 : i;
        if (ui_button(ui_id("lab_ins", (uint32_t)i), (SDL_Rect){cols[6], ry, 60, rh}, "Insert",
                      l->ngenes < LAB_MAX_GENES))
            act_insert = i;
        if (ui_button(ui_id("lab_del", (uint32_t)i), (SDL_Rect){cols[7], ry, 60, rh}, "Delete",
                      true))
            act_delete = i;
    }
    gfx_clip_pop();
    if (act_insert >= 0)
        insert_gene(l, act_insert);
    else if (act_delete >= 0)
        delete_gene(l, act_delete);

    /* beside the table: the open gene's reactions, else the body */
    if (l->open_gene >= 0 && l->open_gene < l->ngenes) {
        LabGene *g = &l->genes[l->open_gene];
        snprintf(buf, sizeof buf, "Gene %d: reactions (0-5)", l->open_gene + 1);
        ui_label_dim(side.x, side.y + 4, buf);
        int per_col = (LAB_REACTIONS + 2) / 3, cw = side.w / 3;
        for (int k = 0; k < LAB_REACTIONS; k++) {
            int rx = side.x + (k / per_col) * cw, ry = side.y + rh + 4 + (k % per_col) * (rh + 2);
            ui_label(rx, ry + 4, k_react_cols[k].label);
            double v = g->reaction[k_react_cols[k].index];
            if (ui_spinner(ui_id("lab_rv", (uint32_t)k), (SDL_Rect){rx + 82, ry, 60, rh}, &v, 0, 5,
                           1, 0, false))
                g->reaction[k_react_cols[k].index] = (int)v;
        }
    } else {
        /* LabWindow's preview: the traits don't change the drawing */
        GeneticCode *preview = l->ngenes > 0 ? make_code(l) : NULL;
        gc_panel(a, ui_id("lab_preview", 0), side, preview);
        gfx_frame(side, C_BORDER);
    }

    /* buttons */
    int by = c.y + c.h - rh, bx = c.x;
    if (ui_button(ui_id("lab_ok", 0), (SDL_Rect){bx, by, 150, rh}, "Copy to clipboard", true)) {
        if (l->ngenes > 0) {
            app_set_clipboard(a, make_code(l));
            app_set_status(a, "Genetic code copied: right-click the world and Paste");
        }
        l->win.open = false;
    }
    if (ui_button(ui_id("lab_cancel", 0), (SDL_Rect){bx + 160, by, 100, rh}, "Cancel", true))
        l->win.open = false;
    if (ui_button(ui_id("lab_clear", 0), (SDL_Rect){bx + 270, by, 100, rh}, "Clear", true)) {
        l->ngenes = 0;
        l->open_gene = -1;
    }
    if (ui_button(ui_id("lab_import", 0), (SDL_Rect){bx + 380, by, 100, rh}, "Import...", true))
        filedlg_open(a, FD_IMPORT_LAB, false, "bgg", a->last_dir_genes);
    ui_window_end();
}
