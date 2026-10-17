/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Based on MainWindow.java, VisibleWorld.java, InfoToolbar.java and
 * GeneticCodePanel.java
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

/* The simulator window (MainWindow.java and friends). */
#ifndef UI_APP_H
#define UI_APP_H

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
    FD_IMPORT_PASTE,  /* import a .bgg and paste it at the clicked point */
    FD_IMPORT_LAB,    /* import a .bgg into the laboratory */
    FD_EXPORT_GENES,
    FD_SAVE_IMAGE,
} FilePurpose;

typedef struct {
    UIWin win;
    bool save;
    FilePurpose purpose;
    char ext[16]; /* "bgw|sbw": the files shown; the first is added to bare names */
    char dir[4096];
    char name[256];
    char **entries; /* directories end with '/' */
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

#define LAB_MAX_GENES 100

typedef struct {
    UIWin win;
    Gene genes[LAB_MAX_GENES];
    int ngenes;
    int symmetry; /* 1..8 */
    int mirror;
    int disperse;
    int scroll;
} LabWindow;

typedef struct {
    UIWin win;
    int tab;
    char f[40][32]; /* text of every field, parsed on OK like the Java dialog */
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

#endif
