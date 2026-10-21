/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Part of the colour build, a C port of Biogenesis Color Mod 2.01
 * (https://sourceforge.net/projects/biogenesis/), Copyright (C) 2006-2010
 * Joan Queralt Molina, Color Mod Copyright (C) 2012-2022 MarcoDBAA.
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

/* The colour build's window (Color Mod's MainWindow.java and friends).
 *
 * It takes the place of src/ui/app.h: the colour build puts src/colour
 * first on the include path, so the shared window code (file dialog,
 * widgets, world view) compiles against this App. Every genetic code the
 * window holds is a collector root (jgc_add_root), so a sweep between
 * frames leaves it alone. */
#ifndef COLOUR_UI_APP_H
#define COLOUR_UI_APP_H

#include <SDL.h>
#include <stdbool.h>

#include "sim/world.h"
#include "ui/ui.h"
#include "ui/worldview.h"

int app_run(void);

/* ---- shared between the window and its dialogs ---- */

typedef enum {
    FD_NONE,
    FD_OPEN_WORLD,
    FD_SAVE_WORLD,
    FD_SAVE_WORLD_THEN_QUIT,
    FD_IMPORT_PASTE, /* import a .bgg and paste it at the clicked point */
    FD_IMPORT_LAB,   /* import a .bgg into the laboratory */
    FD_EXPORT_GENES,
    FD_SAVE_IMAGE,
    FD_LIBRARY_IMPORT,  /* a .bgg copied into the being library (SYN-OS) */
    FD_TELESCOPE_WORLD, /* a saved world whose seed the telescope finds (SYN-OS) */
} FilePurpose;

typedef struct {
    UIWin win;
    bool save;
    FilePurpose purpose;
    char ext[16];
    char dir[4096];
    char name[256];
    char **entries;
    int n_entries;
    int scroll;
    int selected;
    bool focus_name;
} FileDialog;

typedef enum { MB_INFO, MB_QUIT_SAVE, MB_OVERWRITE } MsgPurpose;

typedef struct {
    UIWin win;
    MsgPurpose purpose;
    char title[64];
    char text[512];
} MsgBox;

/* The genetic laboratory (LabWindow.java): the code being put together.
 * The genes are the laboratory's own until "Copy to clipboard" makes a
 * genetic code of them. */
#define LAB_MAX_GENES 100
#define LAB_REACTIONS 36

typedef struct {
    double length;
    double theta;
    JColor color;
    int branch;
    int reaction[LAB_REACTIONS]; /* in Gene's constructor order */
} LabGene;

typedef struct {
    UIWin win;
    LabGene genes[LAB_MAX_GENES];
    int ngenes;
    /* the code's traits, as the laboratory's combo boxes hold them */
    int symmetry, mirror, mutationrate, activity;
    int modifiescream, modifiesfallow, modifiesspore, adaptspore;
    bool plague, disperse, generation_battle, sibling_battle, altruist, familial, social,
        peaceful, passive, clockwise, mimicall, modifiespink, modifieslilac, modifiessky, selfish;
    int scroll;
    int open_gene; /* the gene whose reactions are shown, -1 for none */
} LabWindow;

/* The parameters dialog (ParamDialog.java): one text field per parameter */
#define PARAMS_MAX 200

typedef struct {
    UIWin win;
    int tab;
    int scroll;
    char f[PARAMS_MAX][32];
} ParamsDialog;

typedef struct {
    UIWin win;
    unsigned last_refresh;
} StatsWindow;

typedef struct App {
    SDL_Window *window;
    SDL_Renderer *ren;
    World world;
    WorldView view;
    bool running;
    bool active; /* the life process is running (not paused) */
    Organism *selected;
    Organism *tracked;
    GeneticCode *clipboard; /* VisibleWorld.clippedGeneticCode */
    int popup_wx, popup_wy; /* where the right click happened, in world coordinates */
    char game_file[1024];
    char status[160];

    /* timing */
    double acc_ms;
    uint64_t last_tick;
    int fps;
    int fps_count;
    double fps_clock;

    FileDialog fdlg;
    MsgBox msg;
    LabWindow lab;
    ParamsDialog params;
    StatsWindow stats;
    char last_dir_world[1024];
    char last_dir_genes[1024];
    char last_dir_image[1024];
    /* what the file dialog / overwrite confirmation is acting on */
    GeneticCode *pending_gc;
    SDL_Surface *pending_image;
    FilePurpose pending_purpose;
    char pending_path[4400];
} App;

void app_set_status(App *a, const char *msg);
/* Paste a genetic code as a new organism at world (x, y). */
bool app_paste_code(App *a, GeneticCode *gc, int x, int y);
void app_set_clipboard(App *a, GeneticCode *gc);
void app_file_chosen(App *a, FilePurpose purpose, const char *path);

void filedlg_open(App *a, FilePurpose purpose, bool save, const char *ext, const char *dir);
void filedlg_draw(App *a);
void filedlg_free(FileDialog *d);

void msgbox_open(App *a, MsgPurpose purpose, const char *title, const char *text);
void msgbox_draw(App *a);

void lab_open(App *a, const GeneticCode *gc);
void lab_draw(App *a);
void lab_load_code(LabWindow *l, const GeneticCode *gc);

void params_open(App *a);
void params_draw(App *a);

void stats_open(App *a);
void stats_draw(App *a);

/* GeneticCodePanel: draws a code scaled into r; right click offers Copy. */
void gc_panel(App *a, uint32_t id, SDL_Rect r, GeneticCode *gc);

/* Java NumberFormat with at most one decimal: 1,234.5 */
void fmt_num(char *out, size_t n, double v);

/* ---- Color Mod's colours (colours.c) ---- */

typedef struct {
    JColor color;
    const char *name; /* Utils.colorToString, in English */
} ColourName;

/* The 46 colours a gene can have, in Utils.colorToString's order. */
extern const ColourName k_gene_colours[];
extern const int k_n_gene_colours;
/* A colour's name, or "" for the transient ones (flashes, dead bark...). */
const char *colour_name(JColor c);

/* GeneticCode.draw's layout: one line per segment, centred on (0,0), in
 * unscaled units. Returns the number of segments written. */
typedef struct {
    int x0, y0, x1, y1;
    JColor color;
} GcLine;
int gc_layout(const GeneticCode *gc, GcLine *out, int max, int *width, int *height);

/* VisibleWorld's Revive action, which Color Mod extends. */
void colour_revive(World *w, Organism *b);

#endif
