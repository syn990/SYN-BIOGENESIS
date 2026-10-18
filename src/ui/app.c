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

#include "ui/app.h"

#include <SDL_image.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "io/bgg.h"
#include "io/bgw.h"
#include "io/worldfile.h"
#include "sim/params.h"
#ifdef BIO_SYN_EDITION
#include "host.h"
#include "syn/history.h"
#include "syn/stats_screen.h"
#include "syn/syn.h"
#include "synos/synos.h"
#endif

#define MENU_H 24
#define TOOL_H 38
#define STATUS_H 22
#define INFO_H 92

#define MANUAL_URL "http://biogenesis.sourceforge.net/manual.en.php"

#ifdef BIO_SYN_EDITION
#define WINDOW_TITLE "Biogenesis"
#define EDITION_LINE                                                                               \
    "SYN-BIOGENESIS: also Color Mod 2.01 and the SYN-OS interface (View menu, OPTIONS)\n"
/* SYN-OS: the world has been entered from the main menu (CONTINUE goes back to it) */
static bool g_entered;
/* Leaving to run Color Mod (BIO_SWITCH_COLOUR, see host.h); 0: quitting */
static int g_switch;
/* SYN-OS: a status message is on screen (it fades, so keep drawing) */
static bool g_toast_shown;
#else
#define WINDOW_TITLE "Biogenesis"
#define EDITION_LINE ""
#endif

/* ---- small helpers ---- */

void fmt_num(char *out, size_t n, double v)
{
    char tmp[64];
    snprintf(tmp, sizeof tmp, "%.1f", fabs(v));
    char *dot = strchr(tmp, '.');
    if (dot && dot[1] == '0' && dot[2] == '\0')
        *dot = '\0';
    /* group the integer digits */
    char *ip_end = strchr(tmp, '.');
    int ilen = ip_end ? (int)(ip_end - tmp) : (int)strlen(tmp);
    char buf[96];
    int k = 0;
    if (v < 0 && strcmp(tmp, "0") != 0)
        buf[k++] = '-';
    for (int i = 0; i < ilen; i++) {
        buf[k++] = tmp[i];
        int left = ilen - i - 1;
        if (left > 0 && left % 3 == 0)
            buf[k++] = ',';
    }
    snprintf(buf + k, sizeof buf - (size_t)k, "%s", ip_end ? ip_end : "");
    snprintf(out, n, "%s", buf);
}

void app_set_status(App *a, const char *msg)
{
    snprintf(a->status, sizeof a->status, "%s", msg);
}

void app_set_clipboard(App *a, GeneticCode *gc)
{
    if (gc)
        gc_assign(&a->clipboard, gc);
}

static void on_remove(void *ud, Organism *o)
{
    App *a = ud;
#ifdef BIO_SYN_EDITION
    syn_history_removed(&a->world, o);
#endif
    if (a->selected == o)
        a->selected = NULL;
    if (a->tracked == o)
        a->tracked = NULL;
}

bool app_paste_code(App *a, GeneticCode *gc, int x, int y)
{
    Organism *o = org_new_with_code(&a->world, gc);
    if (org_paste(o, x, y)) {
        world_add_organism(&a->world, o, NULL);
        return true;
    }
    org_free(o);
    return false;
}

static void set_window_title(App *a)
{
    char t[1200];
    if (a->game_file[0]) {
        const char *base = strrchr(a->game_file, '/');
        snprintf(t, sizeof t, WINDOW_TITLE " - %s", base ? base + 1 : a->game_file);
    } else {
        snprintf(t, sizeof t, WINDOW_TITLE);
    }
    SDL_SetWindowTitle(a->window, t);
}

static void new_world(App *a)
{
#ifdef BIO_SYN_EDITION
    syn_world_reset();
#endif
    a->selected = NULL;
    a->tracked = NULL;
    world_genesis(&a->world);
#ifdef BIO_SYN_EDITION
    syn_history_reset(&a->world, false);
#endif
    wv_center_on(&a->view, &a->world, a->world.width / 2.0, a->world.height / 2.0);
    a->active = true;
    app_set_status(a, "New world created");
}

static void pause_game(App *a)
{
    a->active = false;
    app_set_status(a, "Game paused");
}

static void play_game(App *a)
{
    a->active = true;
    a->last_tick = SDL_GetPerformanceCounter();
    app_set_status(a, "Game resumed");
}

static bool file_exists(const char *p)
{
    struct stat st;
    return stat(p, &st) == 0;
}

/* Worlds open and save as Biogenesis 0.9's .bgw (the default) or .sbw */
#define WORLD_EXTS BGW_FILE_EXT "|" WORLD_FILE_EXT

static bool save_world_to(App *a, const char *path)
{
    char err[256];
    size_t n = strlen(path);
    bool sbw = n > 4 && !strcasecmp(path + n - 4, "." WORLD_FILE_EXT);
    if (sbw ? worldfile_save(&a->world, path, err, sizeof err)
            : bgw_save(&a->world, path, err, sizeof err)) {
        app_set_status(a, "Writing completed");
        return true;
    }
    msgbox_open(a, MB_INFO, "Write error", err);
    return false;
}

static void quit_now(App *a)
{
    int w, h;
    SDL_GetWindowSize(a->window, &w, &h);
    P.window_width = w;
    P.window_height = h;
    params_save();
    a->running = false;
}

/* MainWindow.quit: offer to save first */
static void request_quit(App *a)
{
#ifdef BIO_SYN_EDITION
    g_switch = 0;
#endif
    msgbox_open(a, MB_QUIT_SAVE, "Save world", "Do you want to save this world before quitting?");
}

#ifdef BIO_SYN_EDITION
/* Color Mod is another simulation: this world goes, saved first if wanted,
 * as when quitting. */
static void request_switch(App *a)
{
    g_switch = BIO_SWITCH_COLOUR;
    msgbox_open(a, MB_QUIT_SAVE, "Save world",
                "Do you want to save this world before switching to Color Mod 2.01?");
}
#endif

static const char *dir_for(App *a, FilePurpose p)
{
    switch (p) {
    case FD_OPEN_WORLD:
    case FD_SAVE_WORLD:
    case FD_SAVE_WORLD_THEN_QUIT:
        return a->last_dir_world;
    case FD_SAVE_IMAGE:
        return a->last_dir_image;
    default:
        return a->last_dir_genes;
    }
}

static void remember_dir(App *a, FilePurpose p, const char *path)
{
    char *dst = (char *)dir_for(a, p);
    snprintf(dst, 1024, "%s", path);
    char *slash = strrchr(dst, '/');
    if (slash && slash != dst)
        *slash = '\0';
}

static void save_game_as(App *a, bool then_quit)
{
    filedlg_open(a, then_quit ? FD_SAVE_WORLD_THEN_QUIT : FD_SAVE_WORLD, true, WORLD_EXTS,
                 a->last_dir_world);
}

static void save_game(App *a)
{
    if (a->game_file[0])
        save_world_to(a, a->game_file);
    else
        save_game_as(a, false);
}

/* What happens once a file has been picked (and overwriting confirmed). */
static void do_file_action(App *a, FilePurpose purpose, const char *path)
{
    char err[256] = "";
    remember_dir(a, purpose, path);
    switch (purpose) {
    case FD_OPEN_WORLD:
        if (bgw_sniff(path) ? bgw_load(&a->world, path, err, sizeof err)
                            : worldfile_load(&a->world, path, err, sizeof err)) {
#ifdef BIO_SYN_EDITION
            syn_world_reset();
            syn_history_reset(&a->world, true);
            {
                const char *base = strrchr(path, '/');
                char what[300];
                snprintf(what, sizeof what, "You opened %s", base ? base + 1 : path);
                syn_history_user(&a->world, 0, 0, what);
            }
            if (synos_active()) {
                synos_set_screen(SYNOS_WORLD);
                g_entered = true;
            }
#endif
            a->selected = NULL;
            a->tracked = NULL;
            snprintf(a->game_file, sizeof a->game_file, "%s", path);
            set_window_title(a);
            wv_clamp(&a->view, &a->world);
            a->active = true;
            app_set_status(a, "World loaded successfully");
        } else {
            msgbox_open(a, MB_INFO, "Read error", err);
        }
        break;
    case FD_SAVE_WORLD:
    case FD_SAVE_WORLD_THEN_QUIT:
        if (save_world_to(a, path)) {
            snprintf(a->game_file, sizeof a->game_file, "%s", path);
            set_window_title(a);
            if (purpose == FD_SAVE_WORLD_THEN_QUIT)
                quit_now(a);
        }
        break;
    case FD_IMPORT_PASTE: {
        GeneticCode *gc = bgg_read(path, err, sizeof err);
        if (gc) {
            app_paste_code(a, gc, a->popup_wx, a->popup_wy);
            gc_unref(gc);
        } else {
            msgbox_open(a, MB_INFO, "Read error", err);
        }
        break;
    }
    case FD_IMPORT_LAB: {
        GeneticCode *gc = bgg_read(path, err, sizeof err);
        if (gc) {
            lab_load_code(&a->lab, gc);
            gc_unref(gc);
        } else {
            msgbox_open(a, MB_INFO, "Read error", err);
        }
        break;
    }
    case FD_EXPORT_GENES:
        if (a->pending_gc && !bgg_write(path, a->pending_gc))
            msgbox_open(a, MB_INFO, "Write error", "This file can't be written");
        else
            app_set_status(a, "Writing completed");
        break;
    case FD_SAVE_IMAGE:
        if (a->pending_image && IMG_SavePNG(a->pending_image, path) != 0)
            msgbox_open(a, MB_INFO, "Write error", IMG_GetError());
        else
            app_set_status(a, "Writing completed");
        break;
    case FD_NONE:
        break;
    }
    if (purpose == FD_EXPORT_GENES)
        gc_assign(&a->pending_gc, NULL);
    if (purpose == FD_SAVE_IMAGE && a->pending_image) {
        SDL_FreeSurface(a->pending_image);
        a->pending_image = NULL;
    }
}

void app_file_chosen(App *a, FilePurpose purpose, const char *path)
{
    bool saving = purpose == FD_SAVE_WORLD || purpose == FD_SAVE_WORLD_THEN_QUIT ||
                  purpose == FD_EXPORT_GENES || purpose == FD_SAVE_IMAGE;
    if (saving && file_exists(path)) {
        a->pending_purpose = purpose;
        snprintf(a->pending_path, sizeof a->pending_path, "%s", path);
        msgbox_open(a, MB_OVERWRITE, "This file already exists",
                    "Are you sure that you want to overwrite this file?");
        return;
    }
    do_file_action(a, purpose, path);
}

/* Message box answers come back here: 0 yes/ok, 1 no, 2 cancel */
static void msg_answer(App *a, MsgPurpose p, int answer)
{
#ifdef BIO_SYN_EDITION
    /* cancelled: not switching after all */
    if (p == MB_QUIT_SAVE && answer != 0 && answer != 1)
        g_switch = 0;
#endif
    if (p == MB_QUIT_SAVE) {
        if (answer == 0) {
            if (a->game_file[0]) {
                if (save_world_to(a, a->game_file))
                    quit_now(a);
            } else {
                save_game_as(a, true);
            }
        } else if (answer == 1) {
            quit_now(a);
        }
    } else if (p == MB_OVERWRITE) {
        if (answer == 0) {
            do_file_action(a, a->pending_purpose, a->pending_path);
        } else {
            if (a->pending_purpose == FD_EXPORT_GENES)
                gc_assign(&a->pending_gc, NULL);
            if (a->pending_purpose == FD_SAVE_IMAGE && a->pending_image) {
                SDL_FreeSurface(a->pending_image);
                a->pending_image = NULL;
            }
        }
    }
}

/* ---- message boxes ---- */

static int g_msg_answer = -1;

void msgbox_open(App *a, MsgPurpose purpose, const char *title, const char *text)
{
    a->msg.purpose = purpose;
    snprintf(a->msg.title, sizeof a->msg.title, "%s", title);
    snprintf(a->msg.text, sizeof a->msg.text, "%s", text);
    int w = 380, lines = 1;
    for (const char *p = text; *p; p++)
        lines += *p == '\n';
    /* rough width from the longest line */
    const char *s = text;
    while (*s) {
        const char *e = strchr(s, '\n');
        size_t len = e ? (size_t)(e - s) : strlen(s);
        char line[256];
        snprintf(line, sizeof line, "%.*s", (int)(len < 255 ? len : 255), s);
        int lw = gfx_text_w(line) + 40;
        w = lw > w ? lw : w;
        s += len;
        if (*s)
            s++;
    }
    ui_window_centre(&a->msg.win, w, 110 + lines * (gfx_font_h() + 2));
    a->msg.win.open = true;
}

void msgbox_draw(App *a)
{
    MsgBox *m = &a->msg;
    if (!m->win.open)
        return;
    SDL_Rect c = ui_window_begin(&m->win, ui_id("msgbox", 0), m->title, true);
    int y = c.y + 4;
    const char *s = m->text;
    while (*s) {
        const char *e = strchr(s, '\n');
        size_t len = e ? (size_t)(e - s) : strlen(s);
        char line[256];
        snprintf(line, sizeof line, "%.*s", (int)(len < 255 ? len : 255), s);
        ui_label(c.x + 6, y, line);
        y += gfx_font_h() + 2;
        s += len;
        if (*s)
            s++;
    }
    const char *labels[3];
    int n;
    if (m->purpose == MB_QUIT_SAVE) {
        labels[0] = "Yes";
        labels[1] = "No";
        labels[2] = "Cancel";
        n = 3;
    } else if (m->purpose == MB_OVERWRITE) {
        labels[0] = "Yes";
        labels[1] = "No";
        n = 2;
    } else {
        labels[0] = "OK";
        n = 1;
    }
    int bw = 90, gap = 10, bh = gfx_font_h() + 10;
    int total = n * bw + (n - 1) * gap;
    int bx = c.x + (c.w - total) / 2, by = c.y + c.h - bh;
    int answer = -1;
    for (int i = 0; i < n; i++)
        if (ui_button(ui_id("mb", (uint32_t)i), (SDL_Rect){bx + i * (bw + gap), by, bw, bh},
                      labels[i], true))
            answer = i;
    if (ui_key_pressed(SDLK_RETURN) || ui_key_pressed(SDLK_KP_ENTER))
        answer = 0;
    if (ui_key_pressed(SDLK_ESCAPE))
        answer = n == 1 ? 0 : n - 1;
    ui_window_end();
    if (!m->win.open && answer < 0)
        answer = n - 1; /* closed with the X: cancel/no */
    if (answer >= 0) {
        m->win.open = false;
        g_msg_answer = answer;
    }
}

/* ---- GeneticCodePanel ---- */

void gc_panel(App *a, uint32_t id, SDL_Rect r, GeneticCode *gc)
{
    gfx_fill(r, COL(0, 0, 0));
    if (!gc)
        return;
    int n = gc->ngenes * gc->symmetry;
    GcLine stackbuf[256];
    GcLine *lines = n <= 256 ? stackbuf : malloc((size_t)n * sizeof *lines);
    if (!lines)
        return;
    int w, h;
    int m = gc_layout(gc, lines, n, &w, &h);
    /* The Java panel only scaled down; scaling up too (to 90% of the panel,
     * at most 6x) makes small bodies readable in the laboratory. */
    double scale = fmin((r.w * 0.9) / (w > 0 ? w : 1), (r.h * 0.9) / (h > 0 ? h : 1));
    if (scale > 6)
        scale = 6;
    int cx = r.x + r.w / 2, cy = r.y + r.h / 2;
    gfx_clip_push(r);
    for (int i = 0; i < m; i++) {
        uint8_t cr, cg, cb;
        bio_color_rgb(lines[i].color, &cr, &cg, &cb);
        gfx_line(cx + (int)lround(lines[i].x0 * scale), cy + (int)lround(lines[i].y0 * scale),
                 cx + (int)lround(lines[i].x1 * scale), cy + (int)lround(lines[i].y1 * scale),
                 COL(cr, cg, cb));
    }
    gfx_clip_pop();
    if (lines != stackbuf)
        free(lines);
    if (ui_pressed(r, 3)) {
        int mx, my;
        ui_mouse(&mx, &my);
        const char *items[] = {"Copy"};
        ui_popup_open(id, mx, my, items, NULL, 1);
    }
    if (ui_popup_take(id) == 0)
        app_set_clipboard(a, gc);
}

/* ---- actions ---- */

typedef enum {
    A_NONE,
    A_NEW,
    A_START_STOP,
    A_OPEN,
    A_SAVE,
    A_SAVE_AS,
    A_QUIT,
    A_STATS,
    A_LAB,
    A_INC_CO2,
    A_DEC_CO2,
    A_KILL_ALL,
    A_DISPERSE_ALL,
    A_PARAMS,
    A_MANUAL,
    A_ABOUT,
    A_ZOOM_IN,
    A_ZOOM_OUT,
    A_ZOOM_1,
    A_ABORT_TRACKING,
    /* organism */
    A_TRACK,
    A_FEED,
    A_WEAKEN,
    A_REPRODUCE,
    A_REJUVENATE,
    A_KILL,
    A_COPY,
    A_EXPORT,
    A_SAVE_IMAGE,
    A_REVIVE,
    A_DISPERSE,
    /* empty space */
    A_PASTE,
    A_CREATE_RANDOM,
    A_IMPORT,
#ifdef BIO_SYN_EDITION
    /* the SYN edition's interface and view */
    A_INTERFACE, /* SYN-OS <-> Heritage edition */
    A_SWITCH_COLOUR,
    A_MAIN_MENU,
    A_OPTIONS,
    A_FIT,
    A_FAMILY, /* the selected organism's species, in the family tree */
    A_MINIMAP,
    A_LOOK, /* A_LOOK + a Look */
    A_LOOK_LAST = A_LOOK + LOOK_COUNT - 1,
#endif
} Action;

static void about(App *a)
{
    msgbox_open(a, MB_INFO, "About Biogenesis",
                "Biogenesis\n"
                "http://biogenesis.sourceforge.net/\n"
                "Biogenesis version: 0.9 (SYN-BIOGENESIS, a C port)\n" EDITION_LINE
                "\n"
                "Copyright (c) Joan Queralt Molina 2006-2010\n"
                "Version 0.9 by MarcoDBAA 2018-2019\n"
                "STRtrees (by Vivid Solutions) implemented by Richard David Williams\n"
                "Artwork by Ananda Daydream, Florian Haag (http://toolbaricons.sourceforge.net/)\n"
                "\n"
                "Released under the GNU General Public License, version 2 or later.");
}

#ifdef BIO_SYN_EDITION
/* What the user does to the world goes on the history's timeline; repeats
 * close together are one event. */
static void note_action(App *a, Action act)
{
    World *w = &a->world;
    Organism *b = a->selected;
    bool alive = b && b->alive, dead = b && !b->alive;
    switch (act) {
    case A_INC_CO2:
        syn_history_user(w, 'c', 500, "You added CO2");
        break;
    case A_DEC_CO2:
        syn_history_user(w, 'd', 500, "You took CO2 away");
        break;
    case A_KILL_ALL:
        syn_history_user(w, 0, w->population, "You killed every organism");
        break;
    case A_DISPERSE_ALL:
        syn_history_user(w, 0, world_n_corpses(w), "You dispersed every corpse");
        break;
    case A_FEED:
        if (alive)
            syn_history_user(w, 'f', 0, "You fed an organism");
        break;
    case A_WEAKEN:
        if (alive)
            syn_history_user(w, 'w', 0, "You weakened an organism");
        break;
    case A_REPRODUCE:
        if (alive)
            syn_history_user(w, 'r', 0, "You made an organism reproduce");
        break;
    case A_REJUVENATE:
        if (alive)
            syn_history_user(w, 'j', 0, "You rejuvenated an organism");
        break;
    case A_KILL:
        if (alive)
            syn_history_user(w, 'k', 0, "You killed an organism");
        break;
    case A_REVIVE:
        if (dead)
            syn_history_user(w, 'v', 0, "You revived a corpse");
        break;
    case A_DISPERSE:
        if (dead)
            syn_history_user(w, 'x', 0, "You dispersed a corpse");
        break;
    case A_PASTE:
        if (a->clipboard)
            syn_history_user(w, 'p', 0, "You placed a copied organism");
        break;
    case A_CREATE_RANDOM:
        syn_history_user(w, 'n', 0, "You created a random organism");
        break;
    default:
        break;
    }
}
#endif

static void do_action(App *a, Action act)
{
    World *w = &a->world;
    Organism *b = a->selected;
#ifdef BIO_SYN_EDITION
    note_action(a, act);
    /* SYN-OS has a statistics screen of its own */
    if (act == A_STATS && synos_active()) {
        synos_set_screen(SYNOS_STATS);
        return;
    }
#endif
    switch (act) {
    case A_NONE:
        break;
    case A_NEW:
        new_world(a);
        break;
    case A_START_STOP:
        if (a->active)
            pause_game(a);
        else
            play_game(a);
        break;
    case A_OPEN:
        filedlg_open(a, FD_OPEN_WORLD, false, WORLD_EXTS, a->last_dir_world);
        break;
    case A_SAVE:
        save_game(a);
        break;
    case A_SAVE_AS:
        save_game_as(a, false);
        break;
    case A_QUIT:
        request_quit(a);
        break;
    case A_STATS:
        stats_open(a);
        break;
    case A_LAB:
        lab_open(a, a->clipboard);
        break;
    case A_INC_CO2:
        world_add_co2(w, 500);
        break;
    case A_DEC_CO2:
        world_decrease_co2(w, 500);
        break;
    case A_KILL_ALL:
        world_kill_all(w);
        break;
    case A_DISPERSE_ALL:
        world_disperse_all(w);
        break;
    case A_PARAMS:
        params_open(a);
        break;
    case A_MANUAL:
        SDL_OpenURL(MANUAL_URL);
        break;
    case A_ABOUT:
        about(a);
        break;
    case A_ZOOM_IN:
    case A_ZOOM_OUT:
    case A_ZOOM_1: {
#ifdef BIO_SYN_EDITION
        if (synos_active()) {
            if (act == A_ZOOM_1)
                syn_view_actual_size(&a->view, w);
            else
                syn_view_zoom(&a->view, w, act == A_ZOOM_IN ? 1.25 : 0.8);
            break;
        }
#endif
        SDL_Rect v = a->view.view;
        double f = act == A_ZOOM_IN ? 1.25 : act == A_ZOOM_OUT ? 0.8 : 1.0 / a->view.zoom;
        wv_zoom_at(&a->view, w, f, v.x + v.w / 2, v.y + v.h / 2);
        break;
    }
    case A_ABORT_TRACKING:
        a->tracked = NULL;
        break;
    case A_TRACK:
        if (b && b->alive)
            a->tracked = a->tracked == b ? NULL : b;
        break;
    case A_FEED:
        if (b && b->alive) {
            /* as in the original: the CO2 is added, not taken */
            double q = jmin(10, w->co2);
            world_add_co2(w, q);
            b->energy += q;
            world_add_o2(w, q);
        }
        break;
    case A_WEAKEN:
        if (b && b->alive)
            org_use_energy(b, b->energy / 2);
        break;
    case A_REPRODUCE:
        if (b && b->alive)
            org_reproduce(b);
        break;
    case A_REJUVENATE:
        if (b && b->alive)
            b->age = 0;
        break;
    case A_KILL:
        if (b && b->alive)
            org_die(b, NULL);
        break;
    case A_COPY:
        if (b && b->alive)
            app_set_clipboard(a, b->gc);
        break;
    case A_EXPORT:
        if (b && b->alive) {
            gc_assign(&a->pending_gc, b->gc);
            filedlg_open(a, FD_EXPORT_GENES, true, "bgg", a->last_dir_genes);
        }
        break;
    case A_SAVE_IMAGE:
        if (b && b->alive) {
            if (a->pending_image)
                SDL_FreeSurface(a->pending_image);
            a->pending_image = wv_organism_image(b);
            filedlg_open(a, FD_SAVE_IMAGE, true, "png", a->last_dir_image);
        }
        break;
    case A_REVIVE:
        if (b && !b->alive)
            org_revive(b);
        break;
    case A_DISPERSE:
        if (b && !b->alive)
            org_use_energy(b, b->energy);
        break;
    case A_PASTE:
        if (a->clipboard)
            app_paste_code(a, a->clipboard, a->popup_wx, a->popup_wy);
        break;
    case A_CREATE_RANDOM: {
        GeneticCode *gc = gc_new_random();
        app_paste_code(a, gc, a->popup_wx, a->popup_wy);
        gc_unref(gc);
        break;
    }
    case A_IMPORT:
        filedlg_open(a, FD_IMPORT_PASTE, false, "bgg", a->last_dir_genes);
        break;
#ifdef BIO_SYN_EDITION
    case A_INTERFACE:
        synos_set_active(!synos_active());
        if (synos_active()) {
            /* straight into the world: the menu is for arriving */
            synos_set_screen(SYNOS_WORLD);
            g_entered = true;
        }
        break;
    case A_SWITCH_COLOUR:
        request_switch(a);
        break;
    case A_MAIN_MENU:
        synos_set_screen(SYNOS_MENU);
        break;
    case A_OPTIONS:
        synos_set_screen(SYNOS_OPTIONS);
        break;
    case A_FIT:
        syn_view_fit(&a->view, w);
        break;
    case A_FAMILY:
        if (b)
            syn_stats_family(b);
        break;
    case A_MINIMAP:
        syn_set_minimap(!syn_minimap());
        break;
    default:
        if (act >= A_LOOK && act <= A_LOOK_LAST)
            syn_set_look((Look)(act - A_LOOK));
        break;
#endif
    }
}

/* ---- menus ---- */

typedef struct {
    const char *label;
    Action act;
} MenuItem;

static const MenuItem menu_game[] = {{"New", A_NEW},
                                     {"Pause  (P)", A_START_STOP},
                                     {"Open...", A_OPEN},
                                     {"Save  (S)", A_SAVE},
                                     {"Save as...", A_SAVE_AS},
                                     {"-", A_NONE},
                                     {"Quit", A_QUIT}};
static const MenuItem menu_world[] = {{"Statistics", A_STATS},
                                      {"Genetic Laboratory", A_LAB},
                                      {"Increase CO2", A_INC_CO2},
                                      {"Decrease CO2", A_DEC_CO2},
                                      {"Kill all", A_KILL_ALL},
                                      {"Disperse all", A_DISPERSE_ALL},
                                      {"-", A_NONE},
                                      {"Parameters...", A_PARAMS}};
#ifdef BIO_SYN_EDITION
/* the Heritage edition's: the original's, and the ways to SYN-OS and Color Mod */
static const MenuItem menu_view[] = {{"Zoom in  (+)", A_ZOOM_IN},
                                     {"Zoom out  (-)", A_ZOOM_OUT},
                                     {"Actual size  (0)", A_ZOOM_1},
                                     {"Abort tracking", A_ABORT_TRACKING},
                                     {"-", A_NONE},
                                     {"SYN-OS interface", A_INTERFACE},
                                     {"Color Mod 2.01...", A_SWITCH_COLOUR}};
#else
static const MenuItem menu_view[] = {{"Zoom in  (+)", A_ZOOM_IN},
                                     {"Zoom out  (-)", A_ZOOM_OUT},
                                     {"Actual size  (0)", A_ZOOM_1},
                                     {"Abort tracking", A_ABORT_TRACKING}};
#endif
static const MenuItem menu_help[] = {{"User Manual", A_MANUAL}, {"About Biogenesis", A_ABOUT}};

static const MenuItem menu_alive[] = {{"Track", A_TRACK},
                                      {"Feed", A_FEED},
                                      {"Weaken", A_WEAKEN},
                                      {"Force reproduction", A_REPRODUCE},
                                      {"Rejuvenate", A_REJUVENATE},
                                      {"Kill", A_KILL},
                                      {"Copy", A_COPY},
                                      {"Export...", A_EXPORT},
                                      {"Save image...", A_SAVE_IMAGE}};
static const MenuItem menu_dead[] = {{"Revive", A_REVIVE}, {"Disperse", A_DISPERSE}};
static const MenuItem menu_void[] = {
    {"Paste", A_PASTE}, {"Create randomly", A_CREATE_RANDOM}, {"Import...", A_IMPORT}};

typedef struct {
    const char *title;
    const MenuItem *items;
    int n;
} Menu;

static const Menu menus[] = {{"Game", menu_game, 7},
                             {"World", menu_world, 8},
                             {"View", menu_view, (int)(sizeof menu_view / sizeof menu_view[0])},
                             {"Help", menu_help, 2}};
#define N_MENUS 4

#ifdef BIO_SYN_EDITION
/* the SYN-OS interface's menus, in its top bar */
static const MenuItem synos_game[] = {{"New", A_NEW},
                                      {"Pause  (P)", A_START_STOP},
                                      {"Open...", A_OPEN},
                                      {"Save  (S)", A_SAVE},
                                      {"Save as...", A_SAVE_AS},
                                      {"-", A_NONE},
                                      {"Main menu  (Esc)", A_MAIN_MENU},
                                      {"Options...", A_OPTIONS},
                                      {"Color Mod 2.01...", A_SWITCH_COLOUR},
                                      {"-", A_NONE},
                                      {"Quit", A_QUIT}};
static const MenuItem synos_view[] = {{"Zoom in  (+)", A_ZOOM_IN},
                                      {"Zoom out  (-)", A_ZOOM_OUT},
                                      {"Actual size  (0)", A_ZOOM_1},
                                      {"Fit world  (Home)", A_FIT},
                                      {"Minimap  (M)", A_MINIMAP},
                                      {"Abort tracking", A_ABORT_TRACKING},
                                      {"-", A_NONE},
                                      {"Classic", A_LOOK + LOOK_CLASSIC},
                                      {"Biological", A_LOOK + LOOK_BIOLOGICAL},
                                      {"Galaxy", A_LOOK + LOOK_GALAXY},
                                      {"Digital (SYN-OS)", A_LOOK + LOOK_DIGITAL},
                                      {"-", A_NONE},
                                      {"Heritage edition", A_INTERFACE}};
static const Menu synos_menus[] = {
    {"Game", synos_game, (int)(sizeof synos_game / sizeof synos_game[0])},
    {"World", menu_world, 8},
    {"View", synos_view, (int)(sizeof synos_view / sizeof synos_view[0])},
    {"Help", menu_help, 2}};
#endif

static void open_menu(App *a, uint32_t owner, int x, int y, const MenuItem *items, int n)
{
    const char *labels[32];
    bool enabled[32];
    for (int i = 0; i < n && i < 32; i++) {
        labels[i] = items[i].label;
        enabled[i] = true;
        switch (items[i].act) {
        case A_START_STOP:
            labels[i] = a->active ? "Pause  (P)" : "Resume  (P)";
            break;
        case A_TRACK:
            labels[i] = a->tracked == a->selected ? "Abort tracking" : "Track";
            break;
        case A_PASTE:
            enabled[i] = a->clipboard != NULL;
            break;
        case A_ABORT_TRACKING:
            enabled[i] = a->tracked != NULL;
            break;
#ifdef BIO_SYN_EDITION
        case A_MINIMAP:
            labels[i] = syn_minimap() ? "Hide minimap  (M)" : "Show minimap  (M)";
            break;
        default:
            /* the look in use is marked */
            if ((int)items[i].act == A_LOOK + (int)syn_look()) {
                static char current[64];
                snprintf(current, sizeof current, "\xe2\x80\xa2 %s", items[i].label);
                labels[i] = current;
            }
            break;
#else
        default:
            break;
#endif
        }
    }
    ui_popup_open(owner, x, y, labels, enabled, n);
}

static Action menubar(App *a, int width)
{
    Action act = A_NONE;
    SDL_Rect bar = {0, 0, width, MENU_H};
    gfx_fill(bar, C_PANEL);
    gfx_line(0, MENU_H - 1, width, MENU_H - 1, C_BORDER);
    int x = 4;
    for (int m = 0; m < N_MENUS; m++) {
        uint32_t id = ui_id("menu", (uint32_t)m);
        int w = gfx_text_w(menus[m].title) + 20;
        SDL_Rect r = {x, 0, w, MENU_H - 1};
        bool open = ui_popup_open_for(id);
        if (open || ui_hover(r))
            gfx_fill(r, open ? C_ACCENT : C_HOVER);
        gfx_text_in(r, menus[m].title, C_TEXT, true);
        if (ui_pressed(r, 1))
            open_menu(a, id, r.x, MENU_H, menus[m].items, menus[m].n);
        int got = ui_popup_take(id);
        if (got >= 0)
            act = menus[m].items[got].act;
        x += w;
    }
    return act;
}

/* ---- toolbar ---- */

typedef struct {
    const char *icon;
    const char *label;
    const char *tip;
    Action act;
} Tool;

static Action toolbar(App *a, int width)
{
    Action act = A_NONE;
    SDL_Rect bar = {0, MENU_H, width, TOOL_H};
    gfx_fill(bar, C_PANEL);
    gfx_line(0, MENU_H + TOOL_H - 1, width, MENU_H + TOOL_H - 1, C_BORDER);
    int x = 6, y = MENU_H + 3, s = TOOL_H - 6;
    Tool base[] = {
        {"menu_new", "New", "New world", A_NEW},
        {a->active ? "menu_stop" : "menu_start", a->active ? "Pause" : "Resume",
         a->active ? "Pause process" : "Resume process", A_START_STOP},
        {"menu_save", "Save", "Save world", A_SAVE},
        {"menu_increase_co2", "+CO2", "Increase CO2", A_INC_CO2},
        {"menu_decrease_co2", "-CO2", "Decrease CO2", A_DEC_CO2},
        {"menu_stop_tracking", "Untrack", "Abort tracking", A_ABORT_TRACKING},
    };
    for (size_t i = 0; i < sizeof base / sizeof base[0]; i++) {
        bool en = base[i].act != A_ABORT_TRACKING || a->tracked;
        if (ui_icon_button(ui_id("tool", (uint32_t)i), (SDL_Rect){x, y, s + 6, s},
                           gfx_icon(base[i].icon), base[i].label, en, base[i].tip))
            act = base[i].act;
        x += s + 8;
    }
    Organism *b = a->selected;
    if (b) {
        x += 6;
        gfx_line(x, y + 2, x, y + s - 2, C_BORDER);
        x += 10;
        Tool alive[] = {
            {"menu_feed", "Feed", "Feed organism", A_FEED},
            {"menu_weaken", "Weaken", "Weaken organism", A_WEAKEN},
            {"menu_kill", "Kill", "Kill organism", A_KILL},
            {"menu_copy", "Copy", "Copy genetic code", A_COPY},
            {"menu_save_image", "Image", "Save image", A_SAVE_IMAGE},
            {"menu_track", "Track",
             a->tracked == b ? "Abort tracking organism" : "Track organism", A_TRACK},
        };
        Tool dead[] = {
            {"menu_revive", "Revive", "Revive organism", A_REVIVE},
            {"menu_disperse", "Disperse", "Disperse organism", A_DISPERSE},
        };
        Tool *t = b->alive ? alive : dead;
        int n = b->alive ? 6 : 2;
        for (int i = 0; i < n; i++) {
            if (ui_icon_button(ui_id("otool", (uint32_t)i), (SDL_Rect){x, y, s + 6, s},
                               gfx_icon(t[i].icon), t[i].label, true, t[i].tip))
                act = t[i].act;
            x += s + 8;
        }
        if (b->alive && a->tracked == b)
            gfx_frame((SDL_Rect){x - s - 8, y, s + 6, s}, C_ACCENT);
    }
    return act;
}

/* ---- info toolbar (InfoToolbar.java) ---- */

static void info_bar(App *a, SDL_Rect r)
{
    Organism *o = a->selected;
    gfx_fill(r, C_PANEL);
    gfx_line(r.x, r.y, r.x + r.w, r.y, C_BORDER);
    gc_panel(a, ui_id("infogc", 0), (SDL_Rect){r.x + 8, r.y + 6, 60, 60}, o->gc);
    char v[64];
    int colw = 150, x0 = r.x + 84, y0 = r.y + 6, lh = gfx_font_h() + 6;
    struct {
        const char *label;
        double value;
        bool num;
    } cells[2][4] = {
        {{"ID:", o->id, true},
         {"Generation:", o->generation, true},
         {"Age:", o->age >> 8, true},
         {"Energy:", o->energy, true}},
        {{"Children:", o->total_children, true},
         {"Victims:", o->total_kills, true},
         {"Infected:", o->total_infected, true},
         {"Mass:", o->mass, true}},
    };
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 4; col++) {
            int x = x0 + col * colw, y = y0 + row * lh;
            ui_label_dim(x, y, cells[row][col].label);
            fmt_num(v, sizeof v, cells[row][col].value);
            ui_label(x + 80, y, v);
        }
    }
    int y = y0 + 2 * lh;
    if (ui_button(ui_id("examine", 0), (SDL_Rect){x0, y - 2, 130, lh}, "Examine genes", true))
        lab_open(a, o->gc);
    ui_label_dim(x0 + colw, y, "Reproduction:");
    fmt_num(v, sizeof v, o->gc->reproduce_energy);
    ui_label(x0 + colw + 100, y, v);
    if (!o->alive)
        gfx_text(x0 + 2 * colw, y, "(dead)", COL(150, 75, 0));
    else if (a->tracked == o)
        gfx_text(x0 + 2 * colw, y, "(tracking)", C_ACCENT);
}

/* ---- status bar ---- */

static void status_bar(App *a, SDL_Rect r)
{
    gfx_fill(r, C_PANEL);
    gfx_line(r.x, r.y, r.x + r.w, r.y, C_BORDER);
    char o2[32], co2[32], line[400];
    fmt_num(o2, sizeof o2, a->world.o2);
    fmt_num(co2, sizeof co2, a->world.co2);
    snprintf(line, sizeof line,
             "FPS: %d     Time: %lld     Current population: %d     O2: %s     CO2: %s     %s",
             a->active ? a->fps : 0, (long long)world_time_units(&a->world), a->world.population,
             o2, co2, a->status);
    gfx_text(r.x + 8, r.y + (r.h - gfx_font_h()) / 2, line, C_TEXT);
    char z[32];
    snprintf(z, sizeof z, "%d%%", (int)lround(a->view.zoom * 100));
    gfx_text(r.x + r.w - gfx_text_w(z) - 10, r.y + (r.h - gfx_font_h()) / 2, z, C_TEXT_DIM);
}

/* ---- the world panel's mouse handling (VisibleWorld) ---- */

#ifdef BIO_SYN_EDITION
/* a left click on the world this frame (a press that didn't drag) */
static bool g_click;
static int g_click_x, g_click_y;
#endif

static void world_mouse(App *a)
{
    SDL_Rect v = a->view.view;
#ifdef BIO_SYN_EDITION
    /* the Heritage edition has the scrollbars */
    SDL_Rect in = synos_active() ? v : (SDL_Rect){v.x, v.y, v.w - 12, v.h - 12};
#else
    SDL_Rect in = {v.x, v.y, v.w - 12, v.h - 12};
#endif
    int mx, my;
    ui_mouse(&mx, &my);
    double wx, wy;
    wv_screen_to_world(&a->view, mx, my, &wx, &wy);
    int ix = (int)floor(wx), iy = (int)floor(wy);
#ifdef BIO_SYN_EDITION
    /* SYN-OS selects on release, so that a drag pans instead */
    if (synos_active()) {
        if (g_click) {
            double cx, cy;
            wv_screen_to_world(&a->view, g_click_x, g_click_y, &cx, &cy);
            a->selected = world_find_organism_at(&a->world, (int)floor(cx), (int)floor(cy));
        }
    } else
#endif
    if (ui_pressed(in, 1)) {
        a->selected = world_find_organism_at(&a->world, ix, iy);
    }
    uint32_t pop = ui_id("orgpopup", 0);
    if (ui_pressed(in, 3)) {
        a->popup_wx = ix;
        a->popup_wy = iy;
        Organism *b = world_find_organism_at(&a->world, ix, iy);
        if (b) {
            a->selected = b;
            if (b->alive)
                open_menu(a, pop, mx, my, menu_alive, 9);
            else
                open_menu(a, pop + 1, mx, my, menu_dead, 2);
        } else {
            open_menu(a, pop + 2, mx, my, menu_void, 3);
        }
    }
    int got;
    if ((got = ui_popup_take(pop)) >= 0)
        do_action(a, menu_alive[got].act);
    if ((got = ui_popup_take(pop + 1)) >= 0)
        do_action(a, menu_dead[got].act);
    if ((got = ui_popup_take(pop + 2)) >= 0)
        do_action(a, menu_void[got].act);
}

#ifdef BIO_SYN_EDITION
/* ---- the SYN-OS interface's world screen ---- */

#define SYN_BAR_H 46

/* label over value, as SYN-TREK's HUD; returns the width taken */
static float synos_figure(float x, float y, const char *label, const char *value, bool right)
{
    const Fonts *fo = synos_fonts();
    float w = fmaxf(skin_text_width(fo->small, label), skin_text_width(fo->bold, value));
    if (right)
        x -= w;
    skin_text(fo->small, label, x, y, TONE_MUTED, ALIGN_LEFT);
    skin_text(fo->bold, value, x, y + skin_text_height(fo->small) - 1, TONE_TEXT, ALIGN_LEFT);
    return w;
}

static Action synos_top_bar(App *a, int W)
{
    const Fonts *fo = synos_fonts();
    Action act = A_NONE;
    /* the world's figures, right of the tools, or on a second row when the
     * window is too narrow for them */
    char time_s[32], pop[32], o2[32], co2[32], fps[16], zoom[16];
    fmt_num(time_s, sizeof time_s, (double)world_time_units(&a->world));
    fmt_num(pop, sizeof pop, a->world.population);
    fmt_num(o2, sizeof o2, a->world.o2);
    fmt_num(co2, sizeof co2, a->world.co2);
    snprintf(fps, sizeof fps, "%d", a->active ? a->fps : 0);
    snprintf(zoom, sizeof zoom, "%d%%", (int)lround(a->view.zoom * 100));
    const char *fig[][2] = {{"ZOOM", zoom}, {"FPS", fps},          {"CO2", co2},
                            {"O2", o2},     {"POPULATION", pop}, {"TIME", time_s}};
    int nfig = (int)(sizeof fig / sizeof fig[0]);
    float figs_w = -24;
    for (int i = 0; i < nfig; i++)
        figs_w += fmaxf(skin_text_width(fo->small, fig[i][0]), skin_text_width(fo->bold, fig[i][1])) + 24;
    float left_w = 18 + skin_text_width(fo->heading, "BIOGENESIS") + 18 + 14 +
                   6 * (float)(SYN_BAR_H - 14 + 14);
    for (int m = 0; m < N_MENUS; m++)
        left_w += skin_text_width(fo->bold, synos_menus[m].title) + 28 + 4;
    bool two_rows = left_w + 10 + figs_w + 18 > (float)W;
    int bar_h = two_rows ? SYN_BAR_H + 40 : SYN_BAR_H;

    SDL_Rect bar = {0, 0, W, bar_h};
    ui_layer_begin(ui_id("synbar", 0), bar, false);
    skin_bar(synos_rect(bar));
    float x = 18;
    x += skin_text(fo->heading, "BIOGENESIS", x, (SYN_BAR_H - skin_text_height(fo->heading)) / 2,
                   TONE_ACCENT, ALIGN_LEFT) + 18;
    for (int m = 0; m < N_MENUS; m++) {
        uint32_t id = ui_id("synmenu", (uint32_t)m);
        const Menu *mn = &synos_menus[m];
        int w = (int)skin_text_width(fo->bold, mn->title) + 28;
        SDL_Rect r = {(int)x, 7, w, SYN_BAR_H - 14};
        bool open = ui_popup_open_for(id);
        skin_tab(synos_rect(r), fo->bold, mn->title,
                 (WidgetState){.hover = ui_hover(r), .active = open});
        if (ui_pressed(r, 1))
            open_menu(a, id, r.x, SYN_BAR_H, mn->items, mn->n);
        int got = ui_popup_take(id);
        if (got >= 0)
            act = mn->items[got].act;
        x += (float)w + 4;
    }
    x += 14;
    Tool tools[] = {
        {"menu_new", "New", "New world", A_NEW},
        {a->active ? "menu_stop" : "menu_start", a->active ? "Pause" : "Resume",
         a->active ? "Pause process  (P)" : "Resume process  (P)", A_START_STOP},
        {"menu_save", "Save", "Save world  (S)", A_SAVE},
        {"menu_increase_co2", "+CO2", "Increase CO2", A_INC_CO2},
        {"menu_decrease_co2", "-CO2", "Decrease CO2", A_DEC_CO2},
        {"menu_stop_tracking", "Untrack", "Abort tracking", A_ABORT_TRACKING},
    };
    int s = SYN_BAR_H - 14;
    for (size_t i = 0; i < sizeof tools / sizeof tools[0]; i++) {
        bool en = tools[i].act != A_ABORT_TRACKING || a->tracked;
        if (ui_icon_button(ui_id("syntool", (uint32_t)i), (SDL_Rect){(int)x, 7, s + 8, s},
                           gfx_icon(tools[i].icon), tools[i].label, en, tools[i].tip))
            act = tools[i].act;
        x += (float)s + 14;
    }
    float rx = (float)W - 18;
    float fy = (SYN_BAR_H - skin_text_height(fo->small) - skin_text_height(fo->bold) + 1) / 2;
    if (two_rows)
        fy += SYN_BAR_H - 8;
    for (int i = 0; i < nfig; i++)
        rx -= synos_figure(rx, fy, fig[i][0], fig[i][1], true) + 24;
    ui_layer_end();
    return act;
}

/* InfoToolbar's figures and the organism's actions, on a panel over the world */
static Action synos_info_panel(App *a, int H)
{
    const Fonts *fo = synos_fonts();
    Organism *o = a->selected;
    Action act = A_NONE;
    SDL_Rect r = {16, H - 16 - 178, 750, 178};
    ui_layer_begin(ui_id("syninfo", 0), r, false);
    skin_panel(synos_rect(r), true);
    int by = r.y + r.h - 14 - 28;
    gc_panel(a, ui_id("synigc", 0), (SDL_Rect){r.x + 14, r.y + 14, 88, 88}, o->gc);
    if (ui_button(ui_id("synexam", 0), (SDL_Rect){r.x + 14, by, 88, 28}, "Genes", true))
        lab_open(a, o->gc);

    float x0 = (float)r.x + 120, y = (float)r.y + 10;
    char head[64];
    snprintf(head, sizeof head, "ORGANISM %d", o->id);
    float hw = skin_text(fo->heading, head, x0, y, TONE_TEXT, ALIGN_LEFT);
    const char *state = !o->alive ? "DEAD" : a->tracked == o ? "TRACKING" : "ALIVE";
    skin_text(fo->small_bold, state, x0 + hw + 14, y + 5,
              !o->alive ? TONE_WARN : a->tracked == o ? TONE_ACCENT : TONE_GOOD, ALIGN_LEFT);
    y += skin_text_height(fo->heading) + 4;
    struct {
        const char *label;
        double value;
    } cells[] = {{"GENERATION", o->generation},     {"AGE", o->age >> 8},
                 {"ENERGY", o->energy},             {"MASS", o->mass},
                 {"CHILDREN", o->total_children},   {"VICTIMS", o->total_kills},
                 {"INFECTED", o->total_infected},   {"REPRODUCTION", o->gc->reproduce_energy}};
    float colw = 130;
    for (int i = 0; i < 8; i++) {
        char v[64];
        fmt_num(v, sizeof v, cells[i].value);
        synos_figure(x0 + colw * (float)(i % 4),
                     y + (float)(i / 4) * (skin_text_height(fo->small) + skin_text_height(fo->bold) + 4),
                     cells[i].label, v, false);
    }

    /* what can be done to it, as the toolbar and the popup offer */
    static const Tool alive[] = {
        {NULL, "Feed", "Feed organism", A_FEED},       {NULL, "Weaken", "Weaken organism", A_WEAKEN},
        {NULL, "Kill", "Kill organism", A_KILL},       {NULL, "Copy", "Copy genetic code", A_COPY},
        {NULL, "Image", "Save image", A_SAVE_IMAGE},   {NULL, "Track", "Track organism", A_TRACK},
        {NULL, "Family", "Its species in the family tree", A_FAMILY},
    };
    static const Tool dead[] = {
        {NULL, "Revive", "Revive organism", A_REVIVE},
        {NULL, "Disperse", "Disperse organism", A_DISPERSE},
        {NULL, "Family", "Its species in the family tree", A_FAMILY},
    };
    const Tool *t = o->alive ? alive : dead;
    int n = o->alive ? 7 : 3;
    int bx = (int)x0;
    for (int i = 0; i < n; i++) {
        const char *label = t[i].act == A_TRACK && a->tracked == o ? "Untrack" : t[i].label;
        if (ui_button(ui_id("synotool", (uint32_t)i), (SDL_Rect){bx, by, 82, 28}, label, true))
            act = t[i].act;
        if (ui_hover((SDL_Rect){bx, by, 82, 28}))
            ui_tooltip(t[i].tip);
        bx += 88;
    }
    ui_layer_end();
    return act;
}

/* The status line's messages ("World loaded successfully") come and go. */
static void synos_toast(App *a, int W, int H)
{
    static char shown[sizeof a->status];
    static Uint32 since;
    if (strcmp(shown, a->status) != 0) {
        snprintf(shown, sizeof shown, "%s", a->status);
        since = SDL_GetTicks();
    }
    Uint32 age = SDL_GetTicks() - since;
    g_toast_shown = shown[0] && age <= 3500;
    if (!g_toast_shown)
        return;
    const Fonts *fo = synos_fonts();
    float alpha = age > 3000 ? 1.0f - (float)(age - 3000) / 500.0f : 1.0f;
    float w = skin_text_width(fo->body, shown) + 40, h = skin_text_height(fo->body) + 18;
    UIRect r = {((float)W - w) / 2, (float)H - 24 - h, w, h};
    skin_set_alpha(alpha);
    skin_panel(r, true);
    skin_text(fo->body, shown, r.x + w / 2, r.y + 9, TONE_TEXT, ALIGN_CENTER);
    skin_set_alpha(1);
}

/* The SYN-OS interface's frame: the world, and over it the top bar and
 * panels, the main menu or the options. */
static Action synos_frame(App *a, int W, int H, bool keys)
{
    Action act = A_NONE;
    SynosScreen scr = synos_screen();
    if (scr == SYNOS_WORLD) {
        bool dragged;
        g_click = syn_view_input(&a->view, &a->world, keys, &g_click_x, &g_click_y, &dragged);
        /* moving the view by hand lets go of the tracked organism */
        if (dragged)
            a->tracked = NULL;
        if (a->tracked)
            syn_view_follow(&a->view, &a->world, a->tracked);
        world_mouse(a);
    } else {
        g_click = false;
        syn_view_idle(&a->view, &a->world);
        if (a->tracked)
            syn_view_follow(&a->view, &a->world, a->tracked);
    }
    syn_view_draw(&a->view, &a->world, scr == SYNOS_WORLD ? a->selected : NULL);

    if (scr == SYNOS_MENU) {
        switch (synos_menu_frame(W, H, g_entered)) {
        case SYNOS_CONTINUE:
            synos_set_screen(SYNOS_WORLD);
            break;
        case SYNOS_NEW_WORLD:
            new_world(a);
            synos_set_screen(SYNOS_WORLD);
            g_entered = true;
            break;
        case SYNOS_LOAD_WORLD:
            act = A_OPEN;
            break;
        case SYNOS_OPEN_OPTIONS:
            synos_set_screen(SYNOS_OPTIONS);
            break;
        case SYNOS_QUIT:
            act = A_QUIT;
            break;
        default:
            break;
        }
    } else if (scr == SYNOS_OPTIONS) {
        if (synos_options_frame(W, H) == SYNOS_SWITCH_SIM)
            act = A_SWITCH_COLOUR;
    } else if (scr == SYNOS_STATS) {
        syn_stats_frame(a, W, H);
    } else {
        Action t = synos_top_bar(a, W);
        if (t != A_NONE)
            act = t;
        if (a->selected) {
            t = synos_info_panel(a, H);
            if (t != A_NONE)
                act = t;
        }
        synos_toast(a, W, H);
    }
    return act;
}
#endif

/* ---- main loop ---- */

static void run_simulation(App *a)
{
    uint64_t now = SDL_GetPerformanceCounter();
    double dt = (now - a->last_tick) * 1000.0 / SDL_GetPerformanceFrequency();
    a->last_tick = now;
    a->fps_clock += dt;
    if (a->fps_clock >= 1000.0) {
        a->fps = (int)lround(a->fps_count * 1000.0 / a->fps_clock);
        a->fps_count = 0;
        a->fps_clock = 0;
    }
    /* file pickers pause the process, as JFileChooser did */
    if (!a->active || a->fdlg.win.open || a->msg.win.open) {
#ifdef BIO_SYN_EDITION
        syn_set_alpha(1);
#endif
        return;
    }
    double interval = P.delay > 0 ? P.delay : 1;
    a->acc_ms += dt;
    uint64_t budget = now + SDL_GetPerformanceFrequency() * 14 / 1000;
    while (a->acc_ms >= interval) {
#ifdef BIO_SYN_EDITION
        /* slow enough to see each frame: remember the poses to move between */
        if (interval >= 8)
            syn_before_step(&a->world);
#endif
        world_time(&a->world);
#ifdef BIO_SYN_EDITION
        syn_history_frame(&a->world);
#endif
        a->fps_count++;
        a->acc_ms -= interval;
        if (SDL_GetPerformanceCounter() > budget)
            break;
    }
    /* behind schedule: run as fast as possible instead of catching up */
    if (a->acc_ms > interval)
        a->acc_ms = interval;
#ifdef BIO_SYN_EDITION
    syn_set_alpha(interval >= 8 ? a->acc_ms / interval : 1);
#endif
    if (a->tracked && !a->tracked->alive)
        a->tracked = NULL;
}

static void frame(App *a)
{
    int W, H;
    SDL_GetRendererOutputSize(a->ren, &W, &H);
    gfx_fill((SDL_Rect){0, 0, W, H}, C_BG);
    ui_begin_frame();
    ui_layer_begin(UI_BASE_LAYER, (SDL_Rect){0, 0, W, H}, false);

    int info_h = a->selected ? INFO_H : 0;
#ifdef BIO_SYN_EDITION
    /* SYN-OS: the world fills the window, the bars and panels float over it */
    bool synos = synos_active();
    if (synos)
        a->view.view = (SDL_Rect){0, 0, W, H};
    else
#endif
    a->view.view = (SDL_Rect){0, MENU_H + TOOL_H, W, H - MENU_H - TOOL_H - STATUS_H - info_h};
    bool keys = !ui_text_focused() && !ui_any_modal();
#ifdef BIO_SYN_EDITION
    /* the main menu and options take the keys for themselves */
    bool world_keys = keys && (!synos || synos_screen() == SYNOS_WORLD);
    Action act = A_NONE;
    if (synos) {
        act = synos_frame(a, W, H, world_keys);
    } else {
        if (a->tracked)
            wv_center_on(&a->view, &a->world, a->tracked->dcenter_x, a->tracked->dcenter_y);
        wv_input(&a->view, &a->world, keys);
        world_mouse(a);
        wv_draw(&a->view, &a->world, a->selected);
        act = menubar(a, W);
        Action t = toolbar(a, W);
        if (t != A_NONE)
            act = t;
        if (a->selected)
            info_bar(a, (SDL_Rect){0, H - STATUS_H - info_h, W, info_h});
        status_bar(a, (SDL_Rect){0, H - STATUS_H, W, STATUS_H});
    }
    keys = world_keys;
#else
    if (a->tracked)
        wv_center_on(&a->view, &a->world, a->tracked->dcenter_x, a->tracked->dcenter_y);
    wv_input(&a->view, &a->world, keys);
    world_mouse(a);
    wv_draw(&a->view, &a->world, a->selected);

    Action act = menubar(a, W);
    Action t = toolbar(a, W);
    if (t != A_NONE)
        act = t;
    if (a->selected)
        info_bar(a, (SDL_Rect){0, H - STATUS_H - info_h, W, info_h});
    status_bar(a, (SDL_Rect){0, H - STATUS_H, W, STATUS_H});
#endif

    if (keys) {
        if (ui_key_pressed(SDLK_p) || ui_key_pressed(SDLK_SPACE))
            act = A_START_STOP;
        if (ui_key_pressed(SDLK_s) && !(ui_mods() & KMOD_CTRL))
            act = A_SAVE;
        if (ui_key_pressed(SDLK_s) && (ui_mods() & KMOD_CTRL))
            act = (ui_mods() & KMOD_SHIFT) ? A_SAVE_AS : A_SAVE;
        if (ui_key_pressed(SDLK_o) && (ui_mods() & KMOD_CTRL))
            act = A_OPEN;
        if (ui_key_pressed(SDLK_q) && (ui_mods() & KMOD_CTRL))
            act = A_QUIT;
#ifdef BIO_SYN_EDITION
        /* SYN-OS: with nothing selected, Esc is the main menu */
        if (synos && !a->selected && !ui_popup_showing() && ui_key_pressed(SDLK_ESCAPE))
            act = A_MAIN_MENU;
#endif
        if (ui_key_pressed(SDLK_ESCAPE))
            a->selected = NULL;
    }
    ui_layer_end();

    /* windows, back to front; modal ones last */
#ifdef BIO_SYN_EDITION
    /* the SYN-OS main menu and options hide the world's windows until it is back */
    if (!synos || synos_screen() == SYNOS_WORLD || synos_screen() == SYNOS_STATS) {
#endif
    stats_draw(a);
    lab_draw(a);
    params_draw(a);
#ifdef BIO_SYN_EDITION
    }
#endif
    filedlg_draw(a);
    g_msg_answer = -1;
    MsgPurpose mp = a->msg.purpose;
    msgbox_draw(a);
    ui_end_frame();
    gfx_end_frame();

    if (g_msg_answer >= 0)
        msg_answer(a, mp, g_msg_answer);
    if (act != A_NONE)
        do_action(a, act);
}

/* Test hook: SYN_BIOGENESIS_SHOT=out.png renders a few frames (after
 * SYN_BIOGENESIS_SHOT_SIM simulation frames), optionally with a window open
 * (SYN_BIOGENESIS_SHOT_UI = lab | stats | params | about | file | select),
 * saves the screen and exits. Lets the UI be checked under the offscreen
 * video driver.
 * SYN_BIOGENESIS_SHOT_LOAD=world.bgw|.sbw loads a world first, and
 * SYN_BIOGENESIS_SHOT_VIEW=x,y,zoom centres the view there. */
static void shot_prepare(App *a, const char *ui)
{
    const char *load = getenv("SYN_BIOGENESIS_SHOT_LOAD");
    if (load) {
        char err[256];
        if (!(bgw_sniff(load) ? bgw_load(&a->world, load, err, sizeof err)
                              : worldfile_load(&a->world, load, err, sizeof err)))
            fprintf(stderr, "%s: %s\n", load, err);
        wv_clamp(&a->view, &a->world);
#ifdef BIO_SYN_EDITION
        syn_history_reset(&a->world, true);
#endif
    }
    const char *sim = getenv("SYN_BIOGENESIS_SHOT_SIM");
    long n = sim ? strtol(sim, NULL, 10) : 0;
#ifdef BIO_SYN_EDITION
    for (long i = 0; i < n; i++) {
        world_time(&a->world);
        syn_history_frame(&a->world);
    }
#else
    for (long i = 0; i < n; i++)
        world_time(&a->world);
#endif
    double vx, vy, vz;
    const char *view = getenv("SYN_BIOGENESIS_SHOT_VIEW");
    if (view && sscanf(view, "%lf,%lf,%lf", &vx, &vy, &vz) == 3) {
        a->view.zoom = vz;
        wv_center_on(&a->view, &a->world, vx, vy);
    }
    if (!ui)
        return;
    Organism *best = NULL;
    for (int i = 0; i < a->world.norgs; i++)
        if (a->world.orgs[i]->alive &&
            (!best || a->world.orgs[i]->segments > best->segments))
            best = a->world.orgs[i];
    if (!strcmp(ui, "select") || !strcmp(ui, "lab")) {
        a->selected = best;
        if (best) {
            a->view.zoom = 3;
            wv_center_on(&a->view, &a->world, best->dcenter_x, best->dcenter_y);
        }
    }
    if (!strcmp(ui, "lab") && best)
        lab_open(a, best->gc);
    else if (!strcmp(ui, "stats"))
        stats_open(a);
    else if (!strcmp(ui, "params"))
        params_open(a);
    else if (!strcmp(ui, "about"))
        do_action(a, A_ABOUT);
    else if (!strcmp(ui, "file"))
        filedlg_open(a, FD_SAVE_WORLD, true, WORLD_EXTS, a->last_dir_world);
}

static void shot_save(App *a, const char *path)
{
    int w, h;
    SDL_GetRendererOutputSize(a->ren, &w, &h);
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (s && SDL_RenderReadPixels(a->ren, NULL, SDL_PIXELFORMAT_ARGB8888, s->pixels, s->pitch) == 0)
        IMG_SavePNG(s, path);
    else
        fprintf(stderr, "screenshot failed: %s\n", SDL_GetError());
    if (s)
        SDL_FreeSurface(s);
}

/* Test hook: SYN_BIOGENESIS_SCRIPT plays synthetic input, commands split by
 * ';':  wait N | move X Y | click X Y [l|r|m] | key NAME | type TEXT |
 * shot FILE | close (window close button) | quit (immediate).  Used with the offscreen driver to drive the UI the way
 * a person would (menus, popups, dialogs, typing file names). */
typedef struct {
    char *buf;
    char *cmd[256];
    int n, i;
    int wait;
    int phase;
    char shot[1024];
} Script;

static void script_load(Script *s, const char *text)
{
    memset(s, 0, sizeof *s);
    s->buf = strdup(text);
    for (char *tok = strtok(s->buf, ";"); tok && s->n < 256; tok = strtok(NULL, ";")) {
        while (*tok == ' ')
            tok++;
        if (*tok)
            s->cmd[s->n++] = tok;
    }
}

static Uint32 g_script_window;

static void push_mouse(int type, int x, int y, int button)
{
    SDL_Event e;
    memset(&e, 0, sizeof e);
    e.type = (Uint32)type;
    e.motion.windowID = g_script_window; /* same offset in every event */
    if (type == SDL_MOUSEMOTION) {
        e.motion.x = x;
        e.motion.y = y;
    } else {
        e.button.x = x;
        e.button.y = y;
        e.button.button = (Uint8)button;
        e.button.clicks = 1;
        e.button.state = type == SDL_MOUSEBUTTONDOWN ? SDL_PRESSED : SDL_RELEASED;
    }
    SDL_PushEvent(&e);
}

static void script_step(App *a, Script *s)
{
    if (s->wait > 0) {
        s->wait--;
        return;
    }
    if (s->i >= s->n)
        return;
    const char *c = s->cmd[s->i];
    int x = 0, y = 0, n = 0;
    char arg[256] = "";
    if (sscanf(c, "wait %d", &n) == 1) {
        s->wait = n;
        s->i++;
    } else if (sscanf(c, "move %d %d", &x, &y) == 2) {
        push_mouse(SDL_MOUSEMOTION, x, y, 0);
        s->i++;
    } else if (sscanf(c, "click %d %d %255s", &x, &y, arg) >= 2) {
        int b = arg[0] == 'r' ? SDL_BUTTON_RIGHT : arg[0] == 'm' ? SDL_BUTTON_MIDDLE : SDL_BUTTON_LEFT;
        if (s->phase == 0) {
            push_mouse(SDL_MOUSEMOTION, x, y, 0);
            push_mouse(SDL_MOUSEBUTTONDOWN, x, y, b);
            s->phase = 1;
        } else {
            push_mouse(SDL_MOUSEBUTTONUP, x, y, b);
            s->phase = 0;
            s->i++;
            s->wait = 1;
        }
#ifdef BIO_SYN_EDITION
    } else if (sscanf(c, "down %d %d %255s", &x, &y, arg) >= 2) {
        /* press and hold, for dragging with move */
        push_mouse(SDL_MOUSEMOTION, x, y, 0);
        push_mouse(SDL_MOUSEBUTTONDOWN, x, y, arg[0] == 'm' ? SDL_BUTTON_MIDDLE : SDL_BUTTON_LEFT);
        s->i++;
    } else if (sscanf(c, "up %d %d %255s", &x, &y, arg) >= 2) {
        push_mouse(SDL_MOUSEMOTION, x, y, 0);
        push_mouse(SDL_MOUSEBUTTONUP, x, y, arg[0] == 'm' ? SDL_BUTTON_MIDDLE : SDL_BUTTON_LEFT);
        s->i++;
        s->wait = 1;
    } else if (sscanf(c, "wheel %d", &n) == 1) {
        SDL_Event e;
        memset(&e, 0, sizeof e);
        e.type = SDL_MOUSEWHEEL;
        e.wheel.windowID = g_script_window;
        e.wheel.y = n;
        e.wheel.preciseY = (float)n;
        e.wheel.direction = SDL_MOUSEWHEEL_NORMAL;
        SDL_PushEvent(&e);
        s->i++;
        s->wait = 1;
#endif
    } else if (sscanf(c, "key %255s", arg) == 1) {
        SDL_Event e;
        memset(&e, 0, sizeof e);
        e.type = SDL_KEYDOWN;
        e.key.windowID = g_script_window;
        e.key.keysym.sym = SDL_GetKeyFromName(arg);
        SDL_PushEvent(&e);
        e.type = SDL_KEYUP;
        SDL_PushEvent(&e);
        s->i++;
        s->wait = 1;
    } else if (!strncmp(c, "type ", 5)) {
        /* sdl2-compat can't carry pushed SDL_TEXTINPUT events to SDL3 */
        ui_inject_text(c + 5);
        s->i++;
        s->wait = 1;
    } else if (sscanf(c, "shot %1023s", s->shot) == 1) {
        s->i++;
    } else if (!strcmp(c, "close")) {
        /* the window manager's close button */
        SDL_Event e;
        memset(&e, 0, sizeof e);
        e.type = SDL_QUIT;
        SDL_PushEvent(&e);
        s->i++;
        s->wait = 1;
    } else if (!strcmp(c, "quit")) {
        a->running = false;
        s->i++;
    } else {
        fprintf(stderr, "script: bad command '%s'\n", c);
        s->i++;
    }
}

static void set_icon(SDL_Window *win)
{
    const char *p = gfx_asset("icons/biogenesis.png");
    SDL_Surface *s = p ? IMG_Load(p) : NULL;
    if (s) {
        SDL_SetWindowIcon(win, s);
        SDL_FreeSurface(s);
    }
}

int app_run(void)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
    static App app;
    App *a = &app;
    memset(a, 0, sizeof *a);
    int ww = P.window_width > 320 ? P.window_width : 1024;
    int wh = P.window_height > 240 ? P.window_height : 768;
#ifdef BIO_SYN_EDITION
    /* the window Color Mod had, if it ran first (host.h) */
    g_entered = false;
    g_switch = 0;
    a->window = bio_host.window;
    if (a->window)
        SDL_SetWindowTitle(a->window, WINDOW_TITLE);
    else
#endif
    a->window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ww,
                                 wh, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!a->window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetWindowMinimumSize(a->window, 640, 420);
#ifdef BIO_SYN_EDITION
    a->ren = bio_host.renderer;
    if (!a->ren)
#endif
    a->ren = SDL_CreateRenderer(a->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!a->ren)
        a->ren = SDL_CreateRenderer(a->window, -1, SDL_RENDERER_SOFTWARE);
    if (!a->ren) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(a->window);
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderDrawBlendMode(a->ren, SDL_BLENDMODE_BLEND);
    if (!gfx_init(a->ren)) {
        SDL_DestroyRenderer(a->ren);
        SDL_DestroyWindow(a->window);
        SDL_Quit();
        return 1;
    }
#ifdef BIO_SYN_EDITION
    bio_host.window = a->window;
    bio_host.renderer = a->ren;
    SDL_RenderSetClipRect(a->ren, NULL);
    if (!syn_init(a->ren))
        fprintf(stderr, "SYN edition: textures: %s\n", SDL_GetError());
    if (!synos_init(a->ren, a->window, "BIOGENESIS", "SYN EDITION"))
        fprintf(stderr, "SYN edition: no fonts or themes for the SYN-OS interface (assets)\n");
#endif
    set_icon(a->window);
    SDL_StopTextInput(); /* on only while a text field has focus */

    const char *home = getenv("HOME");
    char cwd[1024];
    const char *start = getcwd(cwd, sizeof cwd) ? cwd : (home ? home : "/");
    snprintf(a->last_dir_world, sizeof a->last_dir_world, "%s", start);
    snprintf(a->last_dir_genes, sizeof a->last_dir_genes, "%s", start);
    snprintf(a->last_dir_image, sizeof a->last_dir_image, "%s", start);

    world_init(&a->world);
    a->world.on_remove = on_remove;
    a->world.ud = a;
    wv_init(&a->view);
    int W, H;
    SDL_GetRendererOutputSize(a->ren, &W, &H);
    a->view.view = (SDL_Rect){0, MENU_H + TOOL_H, W, H - MENU_H - TOOL_H - STATUS_H};
    new_world(a);
    a->status[0] = '\0';
    a->running = true;
    a->last_tick = SDL_GetPerformanceCounter();

    const char *shot = getenv("SYN_BIOGENESIS_SHOT");
    int shot_frames = 0;
    Script script = {0};
    const char *script_text = getenv("SYN_BIOGENESIS_SCRIPT");
    if (script_text)
        script_load(&script, script_text);
    g_script_window = SDL_GetWindowID(a->window);
    if (shot) {
        a->active = false;
        shot_prepare(a, getenv("SYN_BIOGENESIS_SHOT_UI"));
#ifdef BIO_SYN_EDITION
        /* SYN_BIOGENESIS_SHOT_SCREEN = menu | options | world (the default
         * with a window or a selection) */
        const char *scr = getenv("SYN_BIOGENESIS_SHOT_SCREEN");
        if (scr ? !strcmp(scr, "world") : getenv("SYN_BIOGENESIS_SHOT_UI") != NULL) {
            synos_set_screen(SYNOS_WORLD);
            g_entered = true;
        } else if (scr && !strcmp(scr, "options")) {
            synos_set_screen(SYNOS_OPTIONS);
        } else if (scr && !strcmp(scr, "stats")) {
            /* SYN_BIOGENESIS_SHOT_TAB picks the tab (syn/stats_screen.c) */
            synos_set_screen(SYNOS_STATS);
            g_entered = true;
        }
#endif
    }

    while (a->running) {
        SDL_Event e;
        bool idle = !a->active && !a->fdlg.win.open && !shot && !script_text;
#ifdef BIO_SYN_EDITION
        /* SYN-OS keeps drawing while the view moves and on its menus */
        idle = idle && !(synos_active() &&
                         (syn_animating() || synos_screen() != SYNOS_WORLD || g_toast_shown));
#endif
        if (script_text)
            script_step(a, &script);
        /* paused: sleep until something happens instead of spinning */
        if (idle && SDL_WaitEventTimeout(&e, 250)) {
            if (e.type == SDL_QUIT)
                request_quit(a);
            ui_handle_event(&e);
        }
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                request_quit(a);
            ui_handle_event(&e);
        }
        run_simulation(a);
        frame(a);
#ifdef BIO_SYN_EDITION
        synos_end_frame();
#endif
        if (shot && !script_text && ++shot_frames == 4) {
            shot_save(a, shot);
            a->running = false;
        }
        if (script.shot[0]) {
            shot_save(a, script.shot);
            script.shot[0] = '\0';
        }
        SDL_RenderPresent(a->ren);
    }

    free(script.buf);
    filedlg_free(&a->fdlg);
    if (a->pending_image)
        SDL_FreeSurface(a->pending_image);
    gc_assign(&a->pending_gc, NULL);
    gc_assign(&a->clipboard, NULL);
    world_free(&a->world);
#ifdef BIO_SYN_EDITION
    syn_history_free();
    syn_stats_free();
#endif
    wv_free(&a->view);
#ifdef BIO_SYN_EDITION
    synos_shutdown();
    syn_shutdown();
#endif
    gfx_shutdown();
#ifdef BIO_SYN_EDITION
    /* the launcher keeps the window for Color Mod, or closes it */
    return g_switch;
#endif
    SDL_DestroyRenderer(a->ren);
    SDL_DestroyWindow(a->window);
    SDL_Quit();
    return 0;
}
