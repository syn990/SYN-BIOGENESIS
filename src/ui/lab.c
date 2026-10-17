/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Based on LabWindow.java
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

/* The Genetic Laboratory (LabWindow.java): edit a gene sequence, its
 * symmetry and mirroring, preview the body, and copy the result to the
 * clipboard to paste into the world. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "sim/params.h"
#include "ui/app.h"

/* ColorComboBox order */
static const int k_lab_colors[GENE_COLORS] = {COL_RED,   COL_GREEN, COL_BLUE,  COL_CYAN,
                                              COL_WHITE, COL_GRAY,  COL_YELLOW};
static const char *const k_lab_color_names[GENE_COLORS] = {"red",   "green", "blue",  "cyan",
                                                           "white", "gray",  "yellow"};

void lab_load_code(LabWindow *l, const GeneticCode *gc)
{
    l->ngenes = gc->ngenes < LAB_MAX_GENES ? gc->ngenes : LAB_MAX_GENES;
    memcpy(l->genes, gc->genes, (size_t)l->ngenes * sizeof(Gene));
    l->symmetry = gc->symmetry;
    l->mirror = gc->mirror;
    l->disperse = gc->disperse_children ? 1 : 0;
    l->scroll = 0;
}

void lab_open(App *a, const GeneticCode *gc)
{
    LabWindow *l = &a->lab;
    l->ngenes = 0;
    l->symmetry = 2;
    l->mirror = 0;
    l->disperse = 0;
    l->scroll = 0;
    if (gc)
        lab_load_code(l, gc);
    if (!l->win.open || !l->win.centred)
        ui_window_centre(&l->win, 860, 560);
    l->win.open = true;
}

static void insert_gene(LabWindow *l, int at)
{
    if (l->ngenes >= LAB_MAX_GENES)
        return;
    memmove(&l->genes[at + 1], &l->genes[at], (size_t)(l->ngenes - at) * sizeof(Gene));
    l->genes[at] = (Gene){2.0, 0.0, COL_GREEN};
    l->ngenes++;
}

static void delete_gene(LabWindow *l, int at)
{
    memmove(&l->genes[at], &l->genes[at + 1], (size_t)(l->ngenes - at - 1) * sizeof(Gene));
    l->ngenes--;
}

void lab_draw(App *a)
{
    LabWindow *l = &a->lab;
    if (!l->win.open)
        return;
    SDL_Rect c = ui_window_begin(&l->win, ui_id("lab", 0), "Genetic Laboratory", false);
    int fh = gfx_font_h(), rh = fh + 10;
    char buf[64];
    int energy = 40 + 3 * l->symmetry * l->ngenes;

    /* general panel */
    int y = c.y;
    ui_label_dim(c.x, y + 5, "Segments:");
    snprintf(buf, sizeof buf, "%d", l->ngenes * l->symmetry);
    ui_label(c.x + 80, y + 5, buf);
    ui_label_dim(c.x + 200, y + 5, "Energy to reproduce:");
    snprintf(buf, sizeof buf, "%d", energy);
    ui_label(c.x + 350, y + 5, buf);
    ui_label_dim(c.x + 470, y + 5, "Life expectancy:");
    snprintf(buf, sizeof buf, "%d", P.max_age);
    ui_label(c.x + 590, y + 5, buf);
    y += rh + 4;
    static const char *const syms[] = {"1", "2", "3", "4", "5", "6", "7", "8"};
    static const char *const noyes[] = {"No", "Yes"};
    int sym_idx = l->symmetry - 1;
    ui_label_dim(c.x, y + 5, "Symmetry:");
    if (ui_combo(ui_id("lab_sym", 0), (SDL_Rect){c.x + 80, y, 70, rh}, syms, 8, &sym_idx))
        l->symmetry = sym_idx + 1;
    ui_label_dim(c.x + 200, y + 5, "Mirror:");
    ui_combo(ui_id("lab_mirror", 0), (SDL_Rect){c.x + 260, y, 70, rh}, noyes, 2, &l->mirror);
    ui_label_dim(c.x + 470, y + 5, "Disperse children:");
    ui_combo(ui_id("lab_disp", 0), (SDL_Rect){c.x + 600, y, 70, rh}, noyes, 2, &l->disperse);
    y += rh + 10;

    /* genes table */
    int bottom = c.y + c.h - rh - 10;
    SDL_Rect table = {c.x, y, 560, bottom - y};
    SDL_Rect preview = {table.x + table.w + 10, y, c.w - table.w - 10, bottom - y};
    gfx_fill(table, C_FIELD);
    gfx_frame(table, C_BORDER);
    int cols[] = {table.x + 8, table.x + 50, table.x + 160, table.x + 270, table.x + 400,
                  table.x + 475};
    ui_label_dim(cols[0], y + 4, "Gene");
    ui_label_dim(cols[1], y + 4, "Length");
    ui_label_dim(cols[2], y + 4, "Rotation");
    ui_label_dim(cols[3], y + 4, "Color");
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
        Gene *g = &l->genes[i];
        snprintf(buf, sizeof buf, "%d:", i);
        ui_label(cols[0], ry + 5, buf);
        ui_spinner(ui_id("lab_len", (uint32_t)i), (SDL_Rect){cols[1], ry, 100, rh}, &g->length,
                   2.0, 18.0, 0.1, 1, false);
        /* ThetaSpinner: whole degrees 0..359, wrapping */
        double deg = round(g->theta * 180.0 / M_PI);
        while (deg < 0)
            deg += 360;
        while (deg > 359)
            deg -= 360;
        if (ui_spinner(ui_id("lab_theta", (uint32_t)i), (SDL_Rect){cols[2], ry, 100, rh}, &deg,
                       0, 359, 1, 0, true))
            g->theta = deg * M_PI / 180.0;
        int sel = 0;
        for (int k = 0; k < GENE_COLORS; k++)
            if (k_lab_colors[k] == g->color)
                sel = k;
        if (ui_color_combo(ui_id("lab_col", (uint32_t)i), (SDL_Rect){cols[3], ry, 120, rh},
                           k_lab_colors, k_lab_color_names, GENE_COLORS, &sel))
            g->color = (uint8_t)k_lab_colors[sel];
        if (ui_button(ui_id("lab_ins", (uint32_t)i), (SDL_Rect){cols[4], ry, 70, rh}, "Insert",
                      l->ngenes < LAB_MAX_GENES))
            act_insert = i;
        if (ui_button(ui_id("lab_del", (uint32_t)i), (SDL_Rect){cols[5], ry, 70, rh}, "Delete",
                      true))
            act_delete = i;
    }
    gfx_clip_pop();
    if (act_insert >= 0)
        insert_gene(l, act_insert);
    else if (act_delete >= 0)
        delete_gene(l, act_delete);

    /* body preview */
    GeneticCode *code = NULL;
    if (l->ngenes > 0)
        code = gc_new_from_genes(l->genes, l->ngenes, l->symmetry, l->mirror, false);
    gc_panel(a, ui_id("lab_preview", 0), preview, code);
    gfx_frame(preview, C_BORDER);

    /* buttons */
    int by = c.y + c.h - rh, bx = c.x;
    if (ui_button(ui_id("lab_ok", 0), (SDL_Rect){bx, by, 150, rh}, "Copy to clipboard",
                  l->ngenes > 0)) {
        GeneticCode *gc = gc_new_from_genes(l->genes, l->ngenes, l->symmetry, l->mirror,
                                            l->disperse != 0);
        app_set_clipboard(a, gc);
        gc_unref(gc);
        app_set_status(a, "Genetic code copied: right-click the world and Paste");
        l->win.open = false;
    }
    if (ui_button(ui_id("lab_cancel", 0), (SDL_Rect){bx + 160, by, 100, rh}, "Cancel", true))
        l->win.open = false;
    if (ui_button(ui_id("lab_clear", 0), (SDL_Rect){bx + 270, by, 100, rh}, "Clear", true))
        l->ngenes = 0;
    if (ui_button(ui_id("lab_import", 0), (SDL_Rect){bx + 380, by, 100, rh}, "Import...", true))
        filedlg_open(a, FD_IMPORT_LAB, false, "bgg", a->last_dir_genes);
    ui_window_end();
    gc_unref(code);
}
