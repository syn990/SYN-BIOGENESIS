/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Based on MainWindow.java, VisibleWorld.java, InfoToolbar.java and
 * GeneticCodePanel.java of Biogenesis Color Mod 2.01
 * (https://sourceforge.net/projects/biogenesis/),
 * Copyright (C) 2006-2010 Joan Queralt Molina,
 * Color Mod Copyright (C) 2012-2022 MarcoDBAA;
 * rewritten for SDL2 in 2026 (from the 0.9 port's src/ui/app.c).
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

#include "compat.h"
#include "host.h"
#include "io/bgg.h"
#include "io/bgw.h"
#include "sim/params.h"
#include "syn/arena.h"
#include "syn/arena_screen.h"
#include "syn/library_screen.h"
#include "syn/genesis_screen.h"
#include "syn/history.h"
#include "syn/sidecar.h"
#include "syn/params_screen.h"
#include "syn/shell.h"
#include "syn/stats_screen.h"
#include "syn/syn.h"
#include "synos/synos.h"

#define MENU_H 24
#define TOOL_H 38
#define STATUS_H 22
#define INFO_H 96

#define MANUAL_URL "http://biogenesis.sourceforge.net/manual.en.php"

#define WINDOW_TITLE "Biogenesis Color Mod"

/* Leaving to run Biogenesis 0.9 (BIO_SWITCH_BIOGENESIS, see host.h); 0: quitting */
static int g_switch;
/* SYN-OS: the world has been entered from the main menu (CONTINUE goes back to it) */
static bool g_entered;
/* SYN-OS: a status message is on screen (it fades, so keep drawing) */
/* The generator's state when this world was begun here, which gives its
 * seed; a world opened from a file doesn't say. */
static uint64_t g_made_state;
static bool g_made_known;

/* the seed that starts java.util.Random at a state (setSeed's scrambling undone) */
static long long state_seed(uint64_t state)
{
    return (long long)((state ^ 0x5DEECE66DULL) & ((1ULL << 48) - 1));
}
/* how the last new world's making ended (syn/genesis_screen.h) */
static SynGenesisEnd g_genesis_end;
static void quit_now(App *a);

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
    /* SYN-OS says it as mako does */
    if (synos_active())
        syn_shell_notify(msg, false);
}

void app_set_clipboard(App *a, GeneticCode *gc)
{
    if (gc)
        a->clipboard = gc;
}

static void on_remove(void *ud, Organism *o)
{
    App *a = ud;
    syn_history_removed(&a->world, o);
    if (a->selected == o)
        a->selected = NULL;
    if (a->tracked == o)
        a->tracked = NULL;
}

bool app_paste_code(App *a, GeneticCode *gc, int x, int y)
{
    Organism *o = org_new_with_code(&a->world, gc);
    if (org_paste_organism(o, x, y)) {
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
    syn_world_reset();
    a->selected = NULL;
    a->tracked = NULL;
    /* made a little at a time, shown being made when it takes a while */
    g_made_state = g_rng.seed;
    g_made_known = true;
    SynGenesis g;
    SynGenesisEnd end = g_genesis_end = syn_genesis_run(&g, &a->world, a->ren);
    syn_history_reset(&a->world, false);
    wv_center_on(&a->view, &a->world, a->world.width / 2.0, a->world.height / 2.0);
    a->active = true;
    if (end == SYN_GENESIS_QUIT) {
        g_switch = 0;
        quit_now(a);
        return;
    }
    if (end == SYN_GENESIS_STOPPED) {
        char msg[160];
        syn_genesis_stopped_text(&g, msg, sizeof msg);
        app_set_status(a, msg);
        return;
    }
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

/* Worlds open and save as Color Mod's .bgw */
#define WORLD_EXTS BGW_FILE_EXT

static bool save_world_to(App *a, const char *path)
{
    char err[256];
    if (bgw_save(&a->world, path, err, sizeof err)) {
        /* the history and family tree, beside it */
        syn_sidecar_save(&a->world, path);
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
    g_switch = 0;
    msgbox_open(a, MB_QUIT_SAVE, "Save world", "Do you want to save this world before quitting?");
}

/* Biogenesis 0.9 is another simulation: this world goes, saved first if
 * wanted, as when quitting. */
static void request_switch(App *a)
{
    g_switch = BIO_SWITCH_BIOGENESIS;
    msgbox_open(a, MB_QUIT_SAVE, "Save world",
                "Do you want to save this world before switching to Biogenesis 0.9?");
}

static const char *dir_for(App *a, FilePurpose p)
{
    switch (p) {
    case FD_OPEN_WORLD:
    case FD_SAVE_WORLD:
    case FD_SAVE_WORLD_THEN_QUIT:
    case FD_TELESCOPE_WORLD:
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
        if (bgw_load(&a->world, path, err, sizeof err)) {
            syn_world_reset();
            syn_history_reset(&a->world, true);
            g_made_known = false;
            syn_sidecar_load(&a->world, path);
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
            a->selected = NULL;
            a->tracked = NULL;
            snprintf(a->game_file, sizeof a->game_file, "%s", path);
            set_window_title(a);
            wv_clamp(&a->view, &a->world);
            a->active = true;
            app_set_status(a, "World loaded successfully");
            /* MainWindow: a backup reopened while backups are on is warned
             * about (its name matches ".*?@[0-9]*.bgw$") */
            const char *at = strrchr(path, '@');
            size_t d = at ? strspn(at + 1, "0123456789") : 0;
            if (P.auto_backup && at && strlen(at + 1 + d) == 4 &&
                !strcmp(at + 1 + d + 1, BGW_FILE_EXT))
                msgbox_open(a, MB_INFO, "Backup file",
                            "WARNING - This backup file will overwrite later backups unless:\n"
                            "-It is saved with a different base filename OR\n"
                            "-Automatic backup is turned off.");
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
        } else {
            msgbox_open(a, MB_INFO, "Read error", err);
        }
        break;
    }
    case FD_IMPORT_LAB: {
        GeneticCode *gc = bgg_read(path, err, sizeof err);
        if (gc) {
            lab_load_code(&a->lab, gc);
        } else {
            msgbox_open(a, MB_INFO, "Read error", err);
        }
        break;
    }
    case FD_LIBRARY_IMPORT:
        syn_library_imported(a, path);
        break;
    case FD_TELESCOPE_WORLD: /* Biogenesis 0.9's only */
        break;
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
        a->pending_gc = NULL;
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
    /* cancelled: not switching after all */
    if (p == MB_QUIT_SAVE && answer != 0 && answer != 1)
        g_switch = 0;
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
                a->pending_gc = NULL;
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
    /* SYN-OS asks as SYN-DEBT does; the answers are the same */
    if (synos_active()) {
        int W, H;
        SDL_GetRendererOutputSize(a->ren, &W, &H);
        static const char *const quit[] = {"SAVE", "DON'T SAVE", "CANCEL"};
        static const char *const over[] = {"REPLACE IT", "NO"};
        static const char *const info[] = {"OK"};
        const char *const *labels = m->purpose == MB_QUIT_SAVE ? quit : m->purpose == MB_OVERWRITE ? over : info;
        int n = m->purpose == MB_QUIT_SAVE ? 3 : m->purpose == MB_OVERWRITE ? 2 : 1;
        int answer = syn_shell_dialog(W, H, m->title, m->text, labels, n);
        if (answer >= 0) {
            m->win.open = false;
            g_msg_answer = answer;
        }
        return;
    }
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
    int n = JLEN(gc->genes) * gc->symmetry;
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
        JColor k = lines[i].color;
        gfx_line(cx + (int)lround(lines[i].x0 * scale), cy + (int)lround(lines[i].y0 * scale),
                 cx + (int)lround(lines[i].x1 * scale), cy + (int)lround(lines[i].y1 * scale),
                 COL(JCOLOR_R(k), JCOLOR_G(k), JCOLOR_B(k)));
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
    A_BACKUP,
    A_QUIT,
    A_STATS,
    A_LAB,
    A_INC_CO2,
    A_DEC_CO2,
    A_INC_CH4,
    A_DEC_CH4,
    A_KILL_ALL,
    A_DISPERSE_ALL,
    A_PARAMS,
    A_MANUAL,
    A_ABOUT,
    A_SWITCH_BIOGENESIS,
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
    /* the SYN edition's interface and view */
    A_INTERFACE, /* SYN-OS <-> Heritage edition */
    A_MAIN_MENU,
    A_OPTIONS,
    A_FIT,
    A_FAMILY, /* the selected organism's species, in the family tree */
    A_MINIMAP,
    A_KEEP, /* the selected organism's code into the being library */
    /* SYN-OS's screens and shell */
    A_WORLD,
    A_LIBRARY,
    A_ARENA,
    A_CONSOLE,
    A_LAUNCHER,
    A_FULLSCREEN,
    A_STEP,  /* one frame on, paused */
    A_GENES, /* the selected organism's genes in the laboratory */
    A_LOOK,  /* A_LOOK + a Look */
    A_LOOK_LAST = A_LOOK + LOOK_COUNT - 1,
} Action;

static void about(App *a)
{
    if (synos_active()) {
        msgbox_open(a, MB_INFO, "About Biogenesis",
                    "Biogenesis 0.9, Color Mod 2.01, rewritten in C: SYN-BIOGENESIS\n"
                    "http://biogenesis.sourceforge.net/\n"
                    "Biogenesis 0.9 and the Heritage edition: OPTIONS, or the commands (/)\n"
                    "\n"
                    "Copyright (c) Joan Queralt Molina 2006-2010\n"
                    "Version 0.9 by MarcoDBAA 2018-2019\n"
                    "Color Mod by MarcoDBAA 2012-2022\n"
                    "STRtrees (by Vivid Solutions) implemented by Richard David Williams\n"
                    "Artwork by Ananda Daydream, Florian Haag (http://toolbaricons.sourceforge.net/)\n"
                    "Noto Sans; SYN Terminal, from Terminus by Dimitar Zhekov; Font Awesome Free: "
                    "the SIL Open Font License\n"
                    "\n"
                    "Released under the GNU General Public License, version 2 or later.");
        return;
    }
    msgbox_open(a, MB_INFO, "About Biogenesis",
                "Biogenesis\n"
                "http://biogenesis.sourceforge.net/\n"
                "Biogenesis version: 0.9, Color Mod 2.01 (SYN-BIOGENESIS colour build, a C port)\n"
                "\n"
                "Copyright (c) Joan Queralt Molina 2006-2010\n"
                "Version 0.9 by MarcoDBAA 2018-2019\n"
                "Color Mod by MarcoDBAA 2012-2022\n"
                "STRtrees (by Vivid Solutions) implemented by Richard David Williams\n"
                "Artwork by Ananda Daydream, Florian Haag (http://toolbaricons.sourceforge.net/)\n"
                "\n"
                "Released under the GNU General Public License, version 2 or later.");
}

/* MainWindow.BackupGameAction: the world saved next to its file, the
 * time in its name (world@00042.bgw); with no file yet, Save as */
static void backup_game(App *a)
{
    if (!a->game_file[0]) {
        save_game_as(a, false);
        return;
    }
    /* filename.replaceFirst("(@[0-9]*)?." + "bgw" + "$", "@" + %05d time + ".bgw"):
     * the '.' is any character, and "bgw" must end the name exactly */
    char path[1100];
    snprintf(path, sizeof path, "%s", a->game_file);
    size_t n = strlen(path);
    char out[1200];
    if (n >= 4 && !strcmp(path + n - 3, BGW_FILE_EXT)) {
        path[n - 4] = '\0';
        char *at = strrchr(path, '@');
        if (at && strspn(at + 1, "0123456789") == strlen(at + 1))
            *at = '\0';
        snprintf(out, sizeof out, "%s@%05lld." BGW_FILE_EXT, path,
                 (long long)world_get_time(&a->world));
    } else {
        snprintf(out, sizeof out, "%s", a->game_file); /* no match: the name is kept */
    }
    save_world_to(a, out);
}

/* MainWindow's life process: with automatic backups on, one backup when
 * the time reaches a multiple of BACKUP_DELAY */
static bool g_backed_up;

static void auto_backup(App *a)
{
    int64_t t = world_get_time(&a->world);
    if (P.auto_backup && P.backup_delay > 0 && t % P.backup_delay == 0 && t > 0) {
        if (!g_backed_up) {
            backup_game(a);
            g_backed_up = true;
        }
    } else {
        g_backed_up = false;
    }
}

/* What the user does to the world goes on the history's timeline; repeats
 * close together are one event. */
static void note_action(App *a, Action act)
{
    World *w = &a->world;
    Organism *b = a->selected;
    bool alive = b && b->alive, dead = b && !b->alive;
    int corpses = 0;
    switch (act) {
    case A_INC_CO2:
        syn_history_user(w, 'c', 1000, "You added CO2");
        break;
    case A_DEC_CO2:
        syn_history_user(w, 'd', 1000, "You took CO2 away");
        break;
    case A_INC_CH4:
        syn_history_user(w, 'm', 1000, "You added methane");
        break;
    case A_DEC_CH4:
        syn_history_user(w, 'M', 1000, "You took methane away");
        break;
    case A_KILL_ALL:
        syn_history_user(w, 0, w->population, "You killed every organism");
        break;
    case A_DISPERSE_ALL:
        for (int i = 0; i < w->norgs; i++)
            corpses += !w->orgs[i]->removed && !w->orgs[i]->alive;
        syn_history_user(w, 0, corpses, "You dispersed every corpse");
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

static void do_action(App *a, Action act)
{
    World *w = &a->world;
    Organism *b = a->selected;
    note_action(a, act);
    /* SYN-OS has a statistics screen of its own */
    if (act == A_STATS && synos_active()) {
        synos_set_screen(SYNOS_STATS);
        return;
    }
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
    case A_BACKUP:
        backup_game(a);
        break;
    case A_QUIT:
        request_quit(a);
        break;
    case A_STATS:
        stats_open(a);
        break;
    case A_LAB:
        /* SYN-OS: back to the code being made, if there is one */
        if (synos_active()) {
            if (!a->lab.win.open)
                lab_open(a, a->clipboard);
            synos_set_screen(SYNOS_LAB);
            break;
        }
        lab_open(a, a->clipboard);
        break;
    case A_INC_CO2:
        world_add_co2(w, 1000);
        break;
    case A_DEC_CO2:
        world_decrease_co2(w, 1000);
        break;
    case A_INC_CH4:
        world_add_ch4(w, 1000);
        break;
    case A_DEC_CH4:
        world_decrease_ch4(w, 1000);
        break;
    case A_KILL_ALL:
        world_kill_all(w);
        break;
    case A_DISPERSE_ALL:
        world_disperse_all(w);
        break;
    case A_PARAMS:
        /* SYN-OS has a screen of its own for them */
        if (synos_active()) {
            syn_params_open(a);
            synos_set_screen(SYNOS_PARAMS);
            break;
        }
        params_open(a);
        break;
    case A_MANUAL:
        SDL_OpenURL(MANUAL_URL);
        break;
    case A_ABOUT:
        about(a);
        break;
    case A_SWITCH_BIOGENESIS:
        request_switch(a);
        break;
    case A_ZOOM_IN:
    case A_ZOOM_OUT:
    case A_ZOOM_1: {
        if (synos_active()) {
            if (act == A_ZOOM_1)
                syn_view_actual_size(&a->view, w);
            else
                syn_view_zoom(&a->view, w, act == A_ZOOM_IN ? 1.25 : 0.8);
            break;
        }
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
            a->pending_gc = b->gc;
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
            colour_revive(w, b);
        break;
    case A_DISPERSE:
        if (b && !b->alive)
            org_use_energy(b, b->energy);
        break;
    case A_PASTE:
        if (a->clipboard)
            app_paste_code(a, a->clipboard, a->popup_wx, a->popup_wy);
        break;
    case A_CREATE_RANDOM:
        app_paste_code(a, gc_new_random(), a->popup_wx, a->popup_wy);
        break;
    case A_IMPORT:
        filedlg_open(a, FD_IMPORT_PASTE, false, "bgg", a->last_dir_genes);
        break;
    case A_INTERFACE:
        synos_set_active(!synos_active());
        if (synos_active()) {
            /* straight into the world: the menu is for arriving */
            synos_set_screen(SYNOS_WORLD);
            g_entered = true;
        }
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
    case A_WORLD:
        /* the laboratory keeps its code until it is closed */
        synos_set_screen(SYNOS_WORLD);
        break;
    case A_LIBRARY:
        syn_library_open_from(synos_screen());
        synos_set_screen(SYNOS_LIBRARY);
        break;
    case A_ARENA:
        synos_set_screen(SYNOS_ARENA);
        break;
    case A_CONSOLE:
        syn_shell_console_toggle();
        break;
    case A_LAUNCHER:
        syn_shell_launcher_open();
        break;
    case A_FULLSCREEN:
        synos_set_fullscreen(!synos_fullscreen());
        break;
    case A_STEP:
        if (!a->active) {
            world_time(w);
            syn_history_frame(w);
        }
        break;
    case A_GENES:
        if (b) {
            lab_open(a, b->gc);
            synos_set_screen(SYNOS_LAB);
        }
        break;
    case A_FAMILY:
        if (b)
            syn_stats_family(b);
        break;
    case A_MINIMAP:
        syn_set_minimap(!syn_minimap());
        break;
    case A_KEEP:
        if (b) {
            char sp[64], origin[200];
            syn_stats_species(b, sp, sizeof sp);
            const char *f = strrchr(a->game_file, '/');
            snprintf(origin, sizeof origin, "%.120s, organism %d at time %lld",
                     a->game_file[0] ? (f ? f + 1 : a->game_file) : "a new world", b->id,
                     (long long)a->world.stats.time);
            syn_library_keep(a, b->gc, "Saved", sp[0] ? sp : "ORGANISM", origin);
        }
        break;
    default:
        if (act >= A_LOOK && act <= A_LOOK_LAST)
            syn_set_look((Look)(act - A_LOOK));
        break;
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
                                     {"Backup  (B)", A_BACKUP},
                                     {"Save as...", A_SAVE_AS},
                                     {"-", A_NONE},
                                     {"Quit", A_QUIT}};
static const MenuItem menu_actions[] = {{"Increase CO2", A_INC_CO2},
                                        {"Decrease CO2", A_DEC_CO2},
                                        {"Increase CH4", A_INC_CH4},
                                        {"Decrease CH4", A_DEC_CH4},
                                        {"Kill all", A_KILL_ALL},
                                        {"Disperse all", A_DISPERSE_ALL}};
static const MenuItem menu_world[] = {{"Statistics", A_STATS},
                                      {"Genetic Laboratory", A_LAB},
                                      {"Parameters...", A_PARAMS}};
/* the Heritage edition's: Color Mod's, and the ways to SYN-OS and Biogenesis 0.9 */
static const MenuItem menu_view[] = {{"Zoom in  (+)", A_ZOOM_IN},
                                     {"Zoom out  (-)", A_ZOOM_OUT},
                                     {"Actual size  (0)", A_ZOOM_1},
                                     {"Abort tracking", A_ABORT_TRACKING},
                                     {"-", A_NONE},
                                     {"SYN-OS interface", A_INTERFACE},
                                     {"Biogenesis 0.9...", A_SWITCH_BIOGENESIS}};
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
/* SYN-OS's: the same, and keeping it in the being library */
static const MenuItem synos_alive[] = {{"Track", A_TRACK},
                                       {"Feed", A_FEED},
                                       {"Weaken", A_WEAKEN},
                                       {"Force reproduction", A_REPRODUCE},
                                       {"Rejuvenate", A_REJUVENATE},
                                       {"Kill", A_KILL},
                                       {"Copy", A_COPY},
                                       {"Export...", A_EXPORT},
                                       {"Keep in the library", A_KEEP},
                                       {"Save image...", A_SAVE_IMAGE}};
#define ALIVE_MENU (synos_active() ? synos_alive : menu_alive)
#define ALIVE_ITEMS (synos_active() ? 10 : 9)
static const MenuItem menu_void[] = {
    {"Paste", A_PASTE}, {"Create randomly", A_CREATE_RANDOM}, {"Import...", A_IMPORT}};

typedef struct {
    const char *title;
    const MenuItem *items;
    int n;
} Menu;

#define N_ITEMS(m) ((int)(sizeof m / sizeof m[0]))
static const Menu menus[] = {{"Game", menu_game, N_ITEMS(menu_game)},
                             {"Actions", menu_actions, N_ITEMS(menu_actions)},
                             {"World", menu_world, N_ITEMS(menu_world)},
                             {"View", menu_view, N_ITEMS(menu_view)},
                             {"Help", menu_help, N_ITEMS(menu_help)}};
#define N_MENUS 5

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
        {"menu_backup", "Backup", "Backup world", A_BACKUP},
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
    int colw = 170, x0 = r.x + 84, y0 = r.y + 4, lh = gfx_font_h() + 4;
    struct {
        const char *label;
        double value;
    } cells[3][4] = {
        {{"ID:", o->id}, {"Generation:", o->gc->generation}, {"Age:", o->age >> 8},
         {"Energy:", o->energy}},
        {{"Children:", o->total_children},
         {"Victims:", o->total_kills},
         {"Infected:", o->total_infected},
         {"Mass:", o->mass}},
        {{NULL, 0},
         {"Reproduction:", o->reproduce_energy},
         {"Life expectancy:", o->max_age},
         {"Mutation \xe2\x80\xb1:", o->gc->mutationrate}},
    };
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 4; col++) {
            if (!cells[row][col].label)
                continue;
            int x = x0 + col * colw, y = y0 + row * lh;
            ui_label_dim(x, y, cells[row][col].label);
            fmt_num(v, sizeof v, cells[row][col].value);
            ui_label(x + gfx_text_w(cells[row][col].label) + 10, y, v);
        }
    }
    if (ui_button(ui_id("examine", 0), (SDL_Rect){x0, y0 + 2 * lh - 2, 130, lh}, "Examine genes",
                  true))
        lab_open(a, o->gc);
    int y = y0 + 3 * lh;
    char clade[300];
    snprintf(clade, sizeof clade, "Clade: %s", o->gc->clade_id ? o->gc->clade_id : "-1");
    gfx_clip_push((SDL_Rect){x0, y, r.w - x0 - 120, lh});
    ui_label(x0, y, clade);
    gfx_clip_pop();
    if (!o->alive)
        gfx_text(r.x + r.w - 100, y, "(dead)", COL(150, 75, 0));
    else if (a->tracked == o)
        gfx_text(r.x + r.w - 100, y, "(tracking)", C_ACCENT);
}

/* ---- status bar ---- */

static void status_bar(App *a, SDL_Rect r)
{
    gfx_fill(r, C_PANEL);
    gfx_line(r.x, r.y, r.x + r.w, r.y, C_BORDER);
    char o2[32], co2[32], ch4[32], line[400];
    fmt_num(o2, sizeof o2, a->world.o2);
    fmt_num(co2, sizeof co2, a->world.co2);
    fmt_num(ch4, sizeof ch4, a->world.ch4);
    snprintf(line, sizeof line,
             "FPS: %d     Time: %lld     Current population: %d     O2: %s     CO2: %s     CH4: %s"
             "     %s",
             a->active ? a->fps : 0, (long long)world_get_time(&a->world), a->world.population,
             o2, co2, ch4, a->status);
    gfx_text(r.x + 8, r.y + (r.h - gfx_font_h()) / 2, line, C_TEXT);
    char z[32];
    snprintf(z, sizeof z, "%d%%", (int)lround(a->view.zoom * 100));
    gfx_text(r.x + r.w - gfx_text_w(z) - 10, r.y + (r.h - gfx_font_h()) / 2, z, C_TEXT_DIM);
}

/* ---- the world panel's mouse handling (VisibleWorld) ---- */

/* a left click on the world this frame (a press that didn't drag) */
static bool g_click;
static int g_click_x, g_click_y;

static int synos_menu_items(App *a, Organism *b, ShellCommand *c);

static void world_mouse(App *a)
{
    SDL_Rect v = a->view.view;
    /* the Heritage edition has the scrollbars */
    SDL_Rect in = synos_active() ? v : (SDL_Rect){v.x, v.y, v.w - 12, v.h - 12};
    int mx, my;
    ui_mouse(&mx, &my);
    double wx, wy;
    wv_screen_to_world(&a->view, mx, my, &wx, &wy);
    int ix = (int)floor(wx), iy = (int)floor(wy);
    /* SYN-OS selects on release, so that a drag pans instead */
    if (synos_active()) {
        if (g_click) {
            double cx, cy;
            wv_screen_to_world(&a->view, g_click_x, g_click_y, &cx, &cy);
            a->selected = world_find_organism_from_position(&a->world, (int)floor(cx), (int)floor(cy));
        }
    } else if (ui_pressed(in, 1)) {
        a->selected = world_find_organism_from_position(&a->world, ix, iy);
    }
    uint32_t pop = ui_id("orgpopup", 0);
    /* SYN-OS: labwc's menu, with what was clicked as its title */
    if (synos_active() && ui_pressed(in, 3)) {
        a->popup_wx = ix;
        a->popup_wy = iy;
        Organism *b = world_find_organism_from_position(&a->world, ix, iy);
        a->selected = b;
        ShellCommand items[24];
        int n = synos_menu_items(a, b, items);
        char title[64];
        if (b)
            snprintf(title, sizeof title, "%s %d", b->alive ? "ORGANISM" : "CORPSE", b->id);
        else
            snprintf(title, sizeof title, "THE WORLD AT %d, %d", ix, iy);
        syn_shell_menu_open(mx, my, title, items, n);
        return;
    }
    if (ui_pressed(in, 3)) {
        a->popup_wx = ix;
        a->popup_wy = iy;
        Organism *b = world_find_organism_from_position(&a->world, ix, iy);
        if (b) {
            a->selected = b;
            if (b->alive)
                open_menu(a, pop, mx, my, ALIVE_MENU, ALIVE_ITEMS);
            else
                open_menu(a, pop + 1, mx, my, menu_dead, 2);
        } else {
            open_menu(a, pop + 2, mx, my, menu_void, 3);
        }
    }
    int got;
    if ((got = ui_popup_take(pop)) >= 0)
        do_action(a, ALIVE_MENU[got].act);
    if ((got = ui_popup_take(pop + 1)) >= 0)
        do_action(a, menu_dead[got].act);
    if ((got = ui_popup_take(pop + 2)) >= 0)
        do_action(a, menu_void[got].act);
}

/* ---- the SYN-OS interface's world: the shell (syn/shell.h) ---- */

#define CMD(...) (c[n++] = (ShellCommand){__VA_ARGS__})

/* this frame's commands */
static ShellCommand g_cmds[128];
static int g_ncmds;

/* Everything the window does, as the launcher, the console and the menus
 * offer it: Heritage's menus and toolbar, and the SYN edition's screens. */
static int synos_commands(App *a, ShellCommand *c)
{
    Organism *b = a->selected;
    bool alive = b && b->alive, dead = b && !b->alive;
    SynosScreen scr = synos_screen();
    int n = 0;
    const char *s = "WORLD";
    CMD(.id = A_NEW, .section = s, .label = "New world", .word = "new", .icon = ICON_SEEDLING,
        .tip = "A new world, made from the parameters");
    CMD(.id = A_START_STOP, .section = s, .label = a->active ? "Pause" : "Resume", .key = "P",
        .word = "pause", .icon = a->active ? ICON_PAUSE : ICON_PLAY);
    CMD(.id = A_STEP, .section = s, .label = "One frame on", .key = ".", .word = "step",
        .icon = ICON_STEP, .disabled = a->active, .tip = "While paused: the world one frame on");
    CMD(.id = A_OPEN, .section = s, .label = "Open a world...", .key = "Ctrl+O", .word = "open",
        .icon = ICON_OPEN);
    CMD(.id = A_SAVE, .section = s, .label = "Save", .key = "S", .word = "save", .icon = ICON_SAVE);
    CMD(.id = A_SAVE_AS, .section = s, .label = "Save as...", .key = "Ctrl+Shift+S", .word = "saveas",
        .icon = ICON_SAVE);
    CMD(.id = A_BACKUP, .section = s, .label = "Backup", .key = "B", .word = "backup", .icon = ICON_BACKUP,
        .tip = "Saved beside its file, the time in its name");
    s = "ATMOSPHERE";
    CMD(.id = A_INC_CO2, .section = s, .label = "Increase CO2", .word = "co2+", .icon = ICON_SMOG,
        .tip = "1000 more carbon dioxide in the atmosphere");
    CMD(.id = A_DEC_CO2, .section = s, .label = "Decrease CO2", .word = "co2-", .icon = ICON_LEAF,
        .tip = "1000 less carbon dioxide, no more than there is");
    CMD(.id = A_INC_CH4, .section = s, .label = "Increase CH4", .word = "ch4+", .icon = ICON_FIRE,
        .tip = "1000 more methane in the atmosphere");
    CMD(.id = A_DEC_CH4, .section = s, .label = "Decrease CH4", .word = "ch4-", .icon = ICON_LEAF,
        .tip = "1000 less methane, no more than there is");
    s = "EVERYONE";
    CMD(.id = A_KILL_ALL, .section = s, .label = "Kill all", .word = "killall", .icon = ICON_SKULL);
    CMD(.id = A_DISPERSE_ALL, .section = s, .label = "Disperse all corpses", .word = "disperseall",
        .icon = ICON_WIND);
    s = "NEW LIFE";
    CMD(.id = A_CREATE_RANDOM, .section = s, .label = "Create a random organism", .word = "random",
        .icon = ICON_WAND);
    CMD(.id = A_PASTE, .section = s, .label = "Paste the copied organism", .key = "Ctrl+V",
        .word = "paste", .icon = ICON_PASTE, .disabled = !a->clipboard);
    CMD(.id = A_IMPORT, .section = s, .label = "Import a .bgg...", .word = "import", .icon = ICON_IMPORT);
    s = "THE ORGANISM SELECTED";
    CMD(.id = A_TRACK, .section = s, .label = a->tracked && a->tracked == b ? "Stop tracking" : "Track",
        .key = "T", .word = "track", .icon = ICON_CROSSHAIRS, .disabled = !alive);
    CMD(.id = A_FEED, .section = s, .label = "Feed", .key = "F", .word = "feed", .icon = ICON_BOLT,
        .disabled = !alive);
    CMD(.id = A_WEAKEN, .section = s, .label = "Weaken", .word = "weaken", .icon = ICON_HEART_CRACK,
        .disabled = !alive);
    CMD(.id = A_REPRODUCE, .section = s, .label = "Force reproduction", .word = "reproduce",
        .icon = ICON_SEEDLING, .disabled = !alive);
    CMD(.id = A_REJUVENATE, .section = s, .label = "Rejuvenate", .word = "rejuvenate",
        .icon = ICON_ROTATE, .disabled = !alive);
    CMD(.id = A_KILL, .section = s, .label = "Kill", .key = "Del", .word = "kill", .icon = ICON_SKULL,
        .disabled = !alive);
    CMD(.id = A_COPY, .section = s, .label = "Copy its genetic code", .key = "Ctrl+C", .word = "copy",
        .icon = ICON_COPY, .disabled = !alive);
    CMD(.id = A_EXPORT, .section = s, .label = "Export its genes...", .word = "export",
        .icon = ICON_EXPORT, .disabled = !alive);
    CMD(.id = A_KEEP, .section = s, .label = "Keep in the library", .word = "keep",
        .icon = ICON_BOOKMARK, .disabled = !b);
    CMD(.id = A_SAVE_IMAGE, .section = s, .label = "Save its image...", .word = "image",
        .icon = ICON_IMAGE, .disabled = !alive);
    CMD(.id = A_GENES, .section = s, .label = "Its genes in the laboratory", .key = "G",
        .word = "genes", .icon = ICON_DNA, .disabled = !b);
    CMD(.id = A_FAMILY, .section = s, .label = "Its family", .word = "family", .icon = ICON_SITEMAP,
        .disabled = !b);
    CMD(.id = A_REVIVE, .section = s, .label = "Revive", .word = "revive", .icon = ICON_HEART_PULSE,
        .disabled = !dead);
    CMD(.id = A_DISPERSE, .section = s, .label = "Disperse", .word = "disperse", .icon = ICON_WIND,
        .disabled = !dead);
    s = "VIEW";
    CMD(.id = A_ZOOM_IN, .section = s, .label = "Zoom in", .key = "+", .word = "in", .icon = ICON_ZOOM_IN);
    CMD(.id = A_ZOOM_OUT, .section = s, .label = "Zoom out", .key = "-", .word = "out",
        .icon = ICON_ZOOM_OUT);
    CMD(.id = A_ZOOM_1, .section = s, .label = "Actual size", .key = "0", .word = "actual",
        .icon = ICON_SEARCH);
    CMD(.id = A_FIT, .section = s, .label = "Fit the world", .key = "Home", .word = "fit",
        .icon = ICON_EXPAND);
    CMD(.id = A_MINIMAP, .section = s, .label = "Minimap", .key = "M", .word = "minimap", .icon = ICON_MAP,
        .on = syn_minimap());
    CMD(.id = A_ABORT_TRACKING, .section = s, .label = "Stop tracking", .word = "untrack",
        .icon = ICON_CROSSHAIRS, .disabled = !a->tracked);
    for (int l = 0; l < LOOK_COUNT; l++)
        CMD(.id = A_LOOK + l, .section = s, .label = syn_look_name((Look)l), .key = l == 0 ? "L" : NULL,
            .icon = ICON_PALETTE, .on = syn_look() == (Look)l,
            .tip = l == 0 ? "L goes through the looks" : NULL);
    s = "SCREENS";
    CMD(.id = A_WORLD, .section = s, .label = "World", .word = "world", .icon = ICON_GLOBE,
        .on = scr == SYNOS_WORLD);
    CMD(.id = A_STATS, .section = s, .label = "Statistics", .word = "stats", .icon = ICON_CHART,
        .on = scr == SYNOS_STATS);
    CMD(.id = A_LAB, .section = s, .label = "Genetic laboratory", .word = "lab", .icon = ICON_FLASK,
        .on = scr == SYNOS_LAB);
    CMD(.id = A_LIBRARY, .section = s, .label = "Being library", .word = "library", .icon = ICON_BOOK,
        .on = scr == SYNOS_LIBRARY);
    CMD(.id = A_ARENA, .section = s, .label = "Arena", .word = "arena", .icon = ICON_TROPHY,
        .on = scr == SYNOS_ARENA);
    CMD(.id = A_PARAMS, .section = s, .label = "Parameters", .word = "parameters", .icon = ICON_SLIDERS,
        .on = scr == SYNOS_PARAMS);
    CMD(.id = A_OPTIONS, .section = s, .label = "Options", .word = "options", .icon = ICON_GEAR);
    CMD(.id = A_MAIN_MENU, .section = s, .label = "Main menu", .key = "Esc", .word = "menu",
        .icon = ICON_HOUSE);
    s = "SYSTEM";
    CMD(.id = A_CONSOLE, .section = s, .label = "Console", .key = "`", .word = NULL, .icon = ICON_TERMINAL,
        .on = syn_shell_console_showing());
    CMD(.id = A_FULLSCREEN, .section = s, .label = "Fullscreen", .key = "F11", .word = "fullscreen",
        .icon = ICON_EXPAND, .on = synos_fullscreen());
    CMD(.id = A_SWITCH_BIOGENESIS, .section = s, .label = "Biogenesis 0.9...", .word = "biogenesis",
        .icon = ICON_PALETTE);
    CMD(.id = A_INTERFACE, .section = s, .label = "Heritage edition", .word = "heritage", .icon = ICON_BACKUP,
        .tip = "Biogenesis' own window, as the original has it");
    CMD(.id = A_MANUAL, .section = s, .label = "User manual", .word = "manual", .icon = ICON_INFO);
    CMD(.id = A_ABOUT, .section = s, .label = "About Biogenesis", .word = "about", .icon = ICON_INFO);
    CMD(.id = A_QUIT, .section = s, .label = "Quit", .key = "Ctrl+Q", .word = "quit", .icon = ICON_POWER);
    return n;
}

/* the commands a right click on an organism, a corpse or empty space offers */
static int synos_menu_items(App *a, Organism *b, ShellCommand *c)
{
    int n = 0;
    if (b && b->alive) {
        CMD(.id = A_TRACK, .label = a->tracked == b ? "Stop tracking" : "Track", .key = "T",
            .icon = ICON_CROSSHAIRS);
        CMD(.id = A_FEED, .label = "Feed", .key = "F", .icon = ICON_BOLT);
        CMD(.id = A_WEAKEN, .label = "Weaken", .icon = ICON_HEART_CRACK);
        CMD(.id = A_REPRODUCE, .label = "Force reproduction", .icon = ICON_SEEDLING);
        CMD(.id = A_REJUVENATE, .label = "Rejuvenate", .icon = ICON_ROTATE);
        CMD(.id = A_KILL, .label = "Kill", .key = "Del", .icon = ICON_SKULL);
        CMD(.id = 0, .label = "-");
        CMD(.id = A_COPY, .label = "Copy", .key = "Ctrl+C", .icon = ICON_COPY);
        CMD(.id = A_EXPORT, .label = "Export...", .icon = ICON_EXPORT);
        CMD(.id = A_KEEP, .label = "Keep in the library", .icon = ICON_BOOKMARK);
        CMD(.id = A_SAVE_IMAGE, .label = "Save image...", .icon = ICON_IMAGE);
        CMD(.id = 0, .label = "-");
        CMD(.id = A_GENES, .label = "Its genes", .key = "G", .icon = ICON_DNA);
        CMD(.id = A_FAMILY, .label = "Its family", .icon = ICON_SITEMAP);
    } else if (b) {
        CMD(.id = A_REVIVE, .label = "Revive", .icon = ICON_HEART_PULSE);
        CMD(.id = A_DISPERSE, .label = "Disperse", .icon = ICON_WIND);
        CMD(.id = 0, .label = "-");
        CMD(.id = A_FAMILY, .label = "Its family", .icon = ICON_SITEMAP);
    } else {
        CMD(.id = A_PASTE, .label = "Paste", .key = "Ctrl+V", .icon = ICON_PASTE, .disabled = !a->clipboard);
        CMD(.id = A_CREATE_RANDOM, .label = "Create randomly", .icon = ICON_WAND);
        CMD(.id = A_IMPORT, .label = "Import...", .icon = ICON_IMPORT);
    }
    return n;
}
#undef CMD

static void card_body(void *ud, SDL_Rect r)
{
    App *a = ud;
    if (a->selected)
        gc_panel(a, ui_id("synigc", 0), r, a->selected->gc);
}

/* InfoToolbar's figures and the organism's actions: its card */
static Action synos_card(App *a, int W, int H)
{
    Organism *o = a->selected;
    ShellCard c = {0};
    snprintf(c.title, sizeof c.title, "ORGANISM %d", o->id);
    c.state = !o->alive ? "DEAD" : a->tracked == o ? "TRACKING" : "ALIVE";
    c.state_tone = !o->alive ? TONE_WARN : a->tracked == o ? TONE_ACCENT : TONE_GOOD;
    /* its species, as the family tree names it */
    char sp[64];
    syn_stats_species(o, sp, sizeof sp);
    if (sp[0])
        snprintf(c.subtitle, sizeof c.subtitle, "SPECIES %s", sp);
    /* Color Mod's clade, its family as it names it */
    if (o->gc->clade_id) {
        size_t k = strlen(c.subtitle);
        snprintf(c.subtitle + k, sizeof c.subtitle - k, "%sclade %.40s", k ? "   " : "", o->gc->clade_id);
    }
    struct {
        const char *label;
        double value;
    } cells[] = {{"GENERATION", o->gc->generation},  {"AGE", o->age >> 8},
                 {"ENERGY", o->energy},              {"MASS", o->mass},
                 {"LIFESPAN", o->max_age},           {"CHILDREN", o->total_children},
                 {"VICTIMS", o->total_kills},        {"INFECTED", o->total_infected},
                 {"BIRTH AT", o->reproduce_energy},  {"MUTATES", o->gc->mutationrate}};
    for (int i = 0; i < 10; i++) {
        c.cells[i].label = cells[i].label;
        fmt_num(c.cells[i].value, sizeof c.cells[i].value, cells[i].value);
        c.cells[i].tone = TONE_TEXT;
    }
    /* its mutation rate is in ten-thousandths: as a percentage */
    snprintf(c.cells[9].value, sizeof c.cells[9].value, "%.2f%%", o->gc->mutationrate / 100.0);
    c.ncells = 10;
    /* what it has towards a child, and how much of its life has gone */
    c.bar_label[0] = "TO A CHILD";
    c.bar[0] = o->reproduce_energy > 0 ? o->energy / o->reproduce_energy : 0;
    c.bar_tone[0] = TONE_GOOD;
    c.bar_label[1] = "LIFE";
    c.bar[1] = o->max_age > 0 ? (double)(o->age >> 8) / o->max_age : 0;
    c.bar_tone[1] = c.bar[1] > 0.8 ? TONE_BAD : TONE_WARN;
    ShellCommand *t = c.actions;
    int n = 0;
    if (o->alive) {
        t[n++] = (ShellCommand){.id = A_FEED, .label = "Feed", .icon = ICON_BOLT, .tip = "Feed it  (F)"};
        t[n++] = (ShellCommand){.id = A_WEAKEN, .label = "Weaken", .icon = ICON_HEART_CRACK};
        t[n++] = (ShellCommand){.id = A_KILL, .label = "Kill", .icon = ICON_SKULL, .tip = "Kill it  (Del)"};
        t[n++] = (ShellCommand){.id = A_COPY, .label = "Copy", .icon = ICON_COPY,
                                .tip = "Copy its genetic code  (Ctrl+C)"};
        t[n++] = (ShellCommand){.id = A_TRACK, .label = a->tracked == o ? "Untrack" : "Track",
                                .icon = ICON_CROSSHAIRS, .on = a->tracked == o, .tip = "Follow it  (T)"};
        t[n++] = (ShellCommand){.id = A_GENES, .label = "Genes", .icon = ICON_DNA,
                                .tip = "Its genes in the laboratory  (G)"};
        t[n++] = (ShellCommand){.id = A_FAMILY, .label = "Family", .icon = ICON_SITEMAP,
                                .tip = "Its species in the family tree"};
        t[n++] = (ShellCommand){.id = A_KEEP, .label = "Keep", .icon = ICON_BOOKMARK,
                                .tip = "Keep its genetic code in the being library"};
    } else {
        t[n++] = (ShellCommand){.id = A_REVIVE, .label = "Revive", .icon = ICON_HEART_PULSE};
        t[n++] = (ShellCommand){.id = A_DISPERSE, .label = "Disperse", .icon = ICON_WIND};
        t[n++] = (ShellCommand){.id = A_FAMILY, .label = "Family", .icon = ICON_SITEMAP};
        t[n++] = (ShellCommand){.id = A_KEEP, .label = "Keep", .icon = ICON_BOOKMARK};
    }
    c.nactions = n;
    c.draw_body = card_body;
    c.ud = a;
    return (Action)syn_shell_card(W, H, &c);
}

/* The bar: the screens, then the world's figures */
static Action synos_bar(App *a, int W)
{
    SynosScreen scr = synos_screen();
    bool lab = scr == SYNOS_LAB, params = scr == SYNOS_PARAMS;
    ShellTab tabs[] = {
        {"WORLD", ICON_GLOBE, A_WORLD, scr == SYNOS_WORLD && !lab, false},
        {"STATISTICS", ICON_CHART, A_STATS, scr == SYNOS_STATS, false},
        {"LABORATORY", ICON_FLASK, A_LAB, lab, false},
        {"LIBRARY", ICON_BOOK, A_LIBRARY, scr == SYNOS_LIBRARY, false},
        {"ARENA", ICON_TROPHY, A_ARENA, scr == SYNOS_ARENA, false},
        {"PARAMETERS", ICON_SLIDERS, A_PARAMS, params, false},
    };
    ShellModule m[9];
    memset(m, 0, sizeof m);
    int n = 0;
    /* the process: running or paused */
    m[n].icon = a->active ? ICON_PLAY : ICON_PAUSE;
    snprintf(m[n].text, sizeof m[n].text, "%s", a->active ? "" : "PAUSED");
    m[n].level = a->active ? 0 : 1;
    m[n].id = A_START_STOP;
    m[n++].tip = a->active ? "Running. Click to pause  (P)" : "Paused. Click to go on  (P)";
    m[n++].custom = synos_speed;
    char v[32];
    fmt_num(v, sizeof v, (double)world_get_time(&a->world));
    m[n].icon = ICON_CLOCK;
    snprintf(m[n].text, sizeof m[n].text, "%s", v);
    m[n++].tip = "Time, in time units of 256 frames";
    fmt_num(v, sizeof v, a->world.population);
    m[n].icon = ICON_USERS;
    snprintf(m[n].text, sizeof m[n].text, "%s", v);
    /* waybar's critical: nothing left alive */
    m[n].level = a->world.population == 0 ? 2 : 0;
    m[n++].tip = a->world.population == 0 ? "Population: nothing is alive" : "Population: organisms alive";
    fmt_num(v, sizeof v, a->world.o2);
    m[n].icon = ICON_LEAF;
    snprintf(m[n].text, sizeof m[n].text, "O2 %s", v);
    m[n++].tip = "Oxygen in the atmosphere";
    fmt_num(v, sizeof v, a->world.co2);
    m[n].icon = ICON_SMOG;
    snprintf(m[n].text, sizeof m[n].text, "CO2 %s", v);
    m[n++].tip = "Carbon dioxide in the atmosphere";
    fmt_num(v, sizeof v, a->world.ch4);
    m[n].icon = ICON_FIRE;
    snprintf(m[n].text, sizeof m[n].text, "CH4 %s", v);
    m[n++].tip = "Methane in the atmosphere";
    m[n].icon = ICON_DISPLAY;
    snprintf(m[n].text, sizeof m[n].text, "%d FPS", a->active ? a->fps : 0);
    /* waybar's warning: well short of the speed asked for */
    m[n].level = a->active && P.delay > 1 && a->fps > 0 && a->fps < 700 / P.delay ? 1 : 0;
    m[n++].tip = "Frames a second the world gets";
    m[n].icon = ICON_SEARCH;
    snprintf(m[n].text, sizeof m[n].text, "%d%%", (int)lround(a->view.zoom * 100));
    m[n].id = A_FIT;
    m[n++].tip = "Zoom. Click to fit the world  (Home)";
    return (Action)syn_shell_bar(W, "BIOGENESIS", tabs, (int)(sizeof tabs / sizeof tabs[0]), m, n, A_MAIN_MENU);
}

/* ---- the console: every command by its word, and the technical ones ---- */

/* the console's own words, beside the commands' */
static const char *const k_console_words[] = {
    "help", "clear", "seed", "generator", "run", "speed", "co2", "ch4", "find", "goto", "zoom", "look",
    "theme", "info",
};

static void console_help(const ShellCommand *c, int n)
{
    syn_shell_print(SHELL_HEAD, "THE CONSOLE");
    syn_shell_print(SHELL_OUT, "  help               this");
    syn_shell_print(SHELL_OUT, "  clear              an empty console  (Ctrl+L)");
    syn_shell_print(SHELL_OUT, "  info               the world: its size, time, life, file, seed");
    syn_shell_print(SHELL_OUT, "  speed N|max        frames a second");
    syn_shell_print(SHELL_OUT, "  run N              N frames on, as fast as they go");
    syn_shell_print(SHELL_OUT, "  co2 + [N] | - [N]  Increase or Decrease CO2, N times (ch4 too)");
    syn_shell_print(SHELL_OUT, "  find ID            select organism ID and fly to it");
    syn_shell_print(SHELL_OUT, "  goto X Y [ZOOM]    the view on a point of the world");
    syn_shell_print(SHELL_OUT, "  zoom Z             the view at a zoom (1 is actual size)");
    syn_shell_print(SHELL_OUT, "  look NAME          classic, biological, galaxy, digital");
    syn_shell_print(SHELL_OUT, "  theme [NAME]       the SYN-OS theme; alone, every one");
    syn_shell_print(SHELL_HEAD, "THE RANDOM GENERATOR (java.util.Random, 48 bits)");
    syn_shell_print(SHELL_OUT, "  seed               the seed this world was made from, when it was made here");
    syn_shell_print(SHELL_OUT, "  generator [STATE]  the generator's state now, or set it (0x... or decimal)");
    syn_shell_list(c, n);
}

/* a number, or say what's wrong with it */
static bool console_number(const char *s, double *v)
{
    char *end;
    *v = strtod(s, &end);
    if (end == s || *end || !isfinite(*v)) {
        syn_shell_print(SHELL_ERROR, "not a number: %s", s);
        return false;
    }
    return true;
}

static void console_run(App *a, const char *text, const ShellCommand *cmds, int ncmds, Action *act)
{
    char line[256];
    snprintf(line, sizeof line, "%s", text);
    char *v[12];
    int argc = syn_shell_split(line, v, 12);
    if (!argc)
        return;
    World *w = &a->world;
    const char *k = v[0];
    if (!strcasecmp(k, "help") || !strcmp(k, "?")) {
        console_help(cmds, ncmds);
    } else if (!strcasecmp(k, "clear")) {
        syn_shell_clear();
    } else if (!strcasecmp(k, "info")) {
        const char *f = strrchr(a->game_file, '/');
        syn_shell_print(SHELL_OUT, "%s, %d x %d, time %lld",
                        a->game_file[0] ? (f ? f + 1 : a->game_file) : "a new world", w->width, w->height,
                        (long long)world_get_time(w));
        syn_shell_print(SHELL_OUT, "%d alive, %d corpses; O2 %.6g, CO2 %.6g, CH4 %.6g", w->population,
                        world_get_ncorpses(w), w->o2, w->co2, w->ch4);
        if (g_made_known)
            syn_shell_print(SHELL_OUT, "made from seed %lld (and every seed equal to it mod 2^48)",
                            (long long)state_seed(g_made_state));
    } else if (!strcasecmp(k, "seed")) {
        if (g_made_known)
            syn_shell_print(SHELL_OUT, "seed %lld: the generator began this world at 0x%012llx",
                            (long long)state_seed(g_made_state), (unsigned long long)g_made_state);
        else
            syn_shell_print(SHELL_NOTE, "this world came from a file; the telescope finds its seed");
    } else if (!strcasecmp(k, "generator")) {
        if (argc > 1) {
            char *end;
            unsigned long long st = strtoull(v[1], &end, 0);
            if (*end) {
                syn_shell_print(SHELL_ERROR, "not a state: %s", v[1]);
                return;
            }
            g_rng.seed = st & ((1ULL << 48) - 1);
            syn_shell_print(SHELL_GOOD, "the world goes on from another thread");
        }
        syn_shell_print(SHELL_OUT, "the generator: 0x%012llx", (unsigned long long)g_rng.seed);
    } else if (!strcasecmp(k, "run")) {
        double x;
        if (argc < 2 || !console_number(v[1], &x) || x < 1)
            return;
        Uint32 t0 = SDL_GetTicks();
        long long done = 0;
        for (; done < (long long)x; done++) {
            world_time(w);
            syn_history_frame(w);
        }
        if (a->tracked && !a->tracked->alive)
            a->tracked = NULL;
        syn_shell_print(SHELL_OUT, "%lld frames in %.2f s; time %lld", done,
                        (SDL_GetTicks() - t0) / 1000.0, (long long)world_get_time(w));
    } else if (!strcasecmp(k, "speed")) {
        if (argc > 1) {
            double x = 0;
            if (!strcasecmp(v[1], "max"))
                P.delay = 1;
            else if (console_number(v[1], &x) && x > 0)
                P.delay = x >= 1000 ? 1 : (int)lround(1000.0 / x) < 1 ? 1 : (int)lround(1000.0 / x);
        }
        syn_shell_print(SHELL_OUT, P.delay <= 1 ? "speed: as fast as it goes" : "speed: %d frames a second",
                        (int)lround(1000.0 / P.delay));
    } else if (!strcasecmp(k, "co2") || !strcasecmp(k, "ch4")) {
        bool ch4 = !strcasecmp(k, "ch4");
        if (argc < 2 || (strcmp(v[1], "+") && strcmp(v[1], "-"))) {
            syn_shell_print(SHELL_ERROR, "%s + [N] or %s - [N]", k, k);
            return;
        }
        double times = 1;
        if (argc > 2 && !console_number(v[2], &times))
            return;
        for (int i = 0; i < (int)times && i < 10000; i++)
            do_action(a, ch4 ? (v[1][0] == '+' ? A_INC_CH4 : A_DEC_CH4)
                             : (v[1][0] == '+' ? A_INC_CO2 : A_DEC_CO2));
        syn_shell_print(SHELL_OUT, "O2 %.6g, CO2 %.6g, CH4 %.6g", w->o2, w->co2, w->ch4);
    } else if (!strcasecmp(k, "find")) {
        double x;
        if (argc < 2 || !console_number(v[1], &x))
            return;
        Organism *o = NULL;
        for (int i = 0; i < w->norgs; i++)
            if (w->orgs[i]->id == (int)x)
                o = w->orgs[i];
        if (!o) {
            syn_shell_print(SHELL_ERROR, "no organism %d in the world", (int)x);
            return;
        }
        a->selected = o;
        a->tracked = o->alive ? o : NULL;
        syn_shell_print(SHELL_OUT, "organism %d, %s, at %.0f, %.0f", o->id, o->alive ? "alive" : "dead",
                        o->dcenter_x, o->dcenter_y);
    } else if (!strcasecmp(k, "goto")) {
        double x, y, z;
        if (argc < 3 || !console_number(v[1], &x) || !console_number(v[2], &y))
            return;
        a->tracked = NULL;
        if (argc > 3 && console_number(v[3], &z) && z > 0)
            syn_view_zoom(&a->view, w, z / a->view.zoom);
        wv_center_on(&a->view, w, x, y);
    } else if (!strcasecmp(k, "zoom")) {
        double z;
        if (argc < 2 || !console_number(v[1], &z) || z <= 0)
            return;
        syn_view_zoom(&a->view, w, z / a->view.zoom);
    } else if (!strcasecmp(k, "look")) {
        for (int l = 0; argc > 1 && l < LOOK_COUNT; l++)
            if (!strncasecmp(syn_look_name((Look)l), v[1], strlen(v[1]))) {
                syn_set_look((Look)l);
                syn_shell_print(SHELL_OUT, "the look: %s", syn_look_name((Look)l));
                return;
            }
        syn_shell_print(SHELL_ERROR, "look classic | biological | galaxy | digital");
    } else if (!strcasecmp(k, "theme")) {
        int nt = synos_theme_count();
        if (argc < 2) {
            for (int i = 0; i < nt; i++)
                syn_shell_print(i == synos_theme_index() ? SHELL_GOOD : SHELL_OUT, "  %s",
                                theme_short_name(synos_theme_at(i)));
            return;
        }
        for (int i = 0; i < nt; i++)
            if (!strcasecmp(theme_short_name(synos_theme_at(i)), v[1]) ||
                !strcasecmp(synos_theme_at(i)->name, v[1])) {
                synos_set_theme(i);
                syn_shell_print(SHELL_OUT, "the theme: %s", synos_theme_at(i)->name);
                return;
            }
        syn_shell_print(SHELL_ERROR, "no theme %s (theme lists them)", v[1]);
    } else {
        const ShellCommand *c = syn_shell_lookup(cmds, ncmds, k);
        if (!c) {
            syn_shell_print(SHELL_ERROR, "%s: no such command (help lists them)", k);
            return;
        }
        if (c->disabled) {
            syn_shell_print(SHELL_ERROR, "%s: not now", c->label);
            return;
        }
        /* open NAME, save NAME: straight to the file */
        if ((c->id == A_OPEN || c->id == A_SAVE || c->id == A_SAVE_AS) && argc > 1) {
            if (c->id == A_OPEN)
                do_file_action(a, FD_OPEN_WORLD, v[1]);
            else
                save_world_to(a, v[1]);
            return;
        }
        *act = (Action)c->id;
    }
}

/* The SYN-OS interface's frame: the world, and over it the shell, the main
 * menu or the options, or another screen. */
static Action synos_frame(App *a, int W, int H, bool keys)
{
    Action act = A_NONE;
    SynosScreen scr = synos_screen();
    bool shell = scr != SYNOS_MENU && scr != SYNOS_OPTIONS;
    bool world = scr == SYNOS_WORLD;
    g_ncmds = synos_commands(a, g_cmds);
    syn_view_hud(world, SHELL_HINTS_H);
    if (world) {
        bool dragged;
        g_click = syn_view_input(&a->view, &a->world, keys && !syn_shell_modal(), &g_click_x, &g_click_y,
                                 &dragged);
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
    syn_view_draw(&a->view, &a->world, world ? a->selected : NULL);

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
        case SYNOS_OPEN_ARENA:
            synos_set_screen(SYNOS_ARENA);
            break;
        case SYNOS_OPEN_LIBRARY:
            syn_library_open_from(SYNOS_MENU);
            synos_set_screen(SYNOS_LIBRARY);
            break;
        case SYNOS_QUIT:
            act = A_QUIT;
            break;
        default:
            break;
        }
    } else if (scr == SYNOS_OPTIONS) {
        if (synos_options_frame(W, H) == SYNOS_SWITCH_SIM)
            act = A_SWITCH_BIOGENESIS;
    } else if (scr == SYNOS_STATS) {
        syn_stats_frame(a, W, H);
    } else if (scr == SYNOS_ARENA) {
        syn_arena_frame(a, W, H);
    } else if (scr == SYNOS_LIBRARY) {
        syn_library_frame(a, W, H);
    } else if (scr == SYNOS_PARAMS) {
        syn_params_frame(a, W, H);
    } else if (scr == SYNOS_LAB) {
        /* the laboratory itself is drawn with the windows, docked here */
        skin_fill(ui_rect(0, 0, (float)W, (float)H), (Rgb){0, 0, 0}, 0.5f);
        bool any_keys = !ui_text_focused() && !ui_any_modal() && !ui_popup_showing();
        if (!a->lab.win.open || (any_keys && !syn_shell_modal() && ui_key_pressed(SDLK_ESCAPE))) {
            a->lab.win.open = false;
            synos_set_screen(SYNOS_WORLD);
        }
    } else {
        if (a->selected) {
            Action t = synos_card(a, W, H);
            if (t != A_NONE)
                act = t;
        }
        static const char *const hints[] = {"/", "commands", "`", "console", "P", "pause", "Home", "fit",
                                            "M", "map", "L", "look", "Esc", "menu"};
        char right[200];
        const char *f = strrchr(a->game_file, '/');
        int k = snprintf(right, sizeof right, "%s   %d x %d",
                         a->game_file[0] ? (f ? f + 1 : a->game_file) : "a new world", a->world.width,
                         a->world.height);
        if (g_made_known && k > 0 && (size_t)k < sizeof right)
            snprintf(right + k, sizeof right - (size_t)k, "   seed %lld", (long long)state_seed(g_made_state));
        syn_shell_hints(W, H, hints, 7, right);
    }
    if (shell) {
        Action t = synos_bar(a, W);
        if (t != A_NONE)
            act = t;
    }
    return act;
}

/* Over everything, the windows too: the console, the notifications, the
 * menu a right click opened and the launcher. */
static Action synos_overlays(App *a, int W, int H)
{
    Action act = A_NONE;
    SynosScreen scr = synos_screen();
    if (scr == SYNOS_MENU || scr == SYNOS_OPTIONS)
        return act;
    ShellCommand *cmds = g_cmds;
    int ncmds = g_ncmds;
        /* the console completes its words and the commands' */
    static const char *words[200];
    int nw = 0;
    for (int i = 0; i < (int)(sizeof k_console_words / sizeof k_console_words[0]); i++)
        words[nw++] = k_console_words[i];
    for (int i = 0; i < ncmds && nw < 200; i++)
        if (cmds[i].word)
            words[nw++] = cmds[i].word;
    bool centre = false;
    const char *line = syn_shell_console(W, SHELL_BAR_H, H, words, nw);
    if (line) {
        console_run(a, line, cmds, ncmds, &act);
        centre = true;
    }
    syn_shell_notifications(W, SHELL_BAR_H);
    int got = syn_shell_menu(W, H);
    if (got)
        act = (Action)got;
    got = syn_shell_launcher(W, H, cmds, ncmds);
    if (got) {
        act = (Action)got;
        centre = true;
    }
    /* made or pasted from the launcher or the console: in the middle of the view */
    if (centre && (act == A_PASTE || act == A_CREATE_RANDOM)) {
        double cx, cy;
        wv_screen_to_world(&a->view, W / 2, H / 2, &cx, &cy);
        a->popup_wx = (int)floor(cx);
        a->popup_wy = (int)floor(cy);
    }
    return act;
}

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
        syn_set_alpha(1);
        return;
    }
    double interval = P.delay > 0 ? P.delay : 1;
    a->acc_ms += dt;
    uint64_t budget = now + SDL_GetPerformanceFrequency() * 14 / 1000;
    while (a->acc_ms >= interval) {
        /* slow enough to see each frame: remember the poses to move between */
        if (interval >= 8)
            syn_before_step(&a->world);
        world_time(&a->world);
        syn_history_frame(&a->world);
        auto_backup(a);
        a->fps_count++;
        a->acc_ms -= interval;
        if (SDL_GetPerformanceCounter() > budget)
            break;
    }
    /* behind schedule: run as fast as possible instead of catching up */
    if (a->acc_ms > interval)
        a->acc_ms = interval;
    syn_set_alpha(interval >= 8 ? a->acc_ms / interval : 1);
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
    /* SYN-OS: the world fills the window, the bars and panels float over it */
    bool synos = synos_active();
    if (synos)
        a->view.view = (SDL_Rect){0, 0, W, H};
    else
        a->view.view = (SDL_Rect){0, MENU_H + TOOL_H, W, H - MENU_H - TOOL_H - STATUS_H - info_h};
    bool keys = !ui_text_focused() && !ui_any_modal();
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

    if (keys) {
        if (ui_key_pressed(SDLK_p) || ui_key_pressed(SDLK_SPACE))
            act = A_START_STOP;
        if (ui_key_pressed(SDLK_s) && !(ui_mods() & KMOD_CTRL))
            act = A_SAVE;
        if (ui_key_pressed(SDLK_b) && !(ui_mods() & KMOD_CTRL))
            act = A_BACKUP;
        if (ui_key_pressed(SDLK_s) && (ui_mods() & KMOD_CTRL))
            act = (ui_mods() & KMOD_SHIFT) ? A_SAVE_AS : A_SAVE;
        if (ui_key_pressed(SDLK_o) && (ui_mods() & KMOD_CTRL))
            act = A_OPEN;
        if (ui_key_pressed(SDLK_q) && (ui_mods() & KMOD_CTRL))
            act = A_QUIT;
        /* SYN-OS: with nothing selected, Esc is the main menu; a menu open
         * takes it for itself */
        bool shell_open = synos && (syn_shell_menu_showing() || syn_shell_launcher_showing());
        if (synos && !shell_open && !a->selected && !ui_popup_showing() && ui_key_pressed(SDLK_ESCAPE))
            act = A_MAIN_MENU;
        if (synos && !shell_open) {
            bool ctrl = (ui_mods() & KMOD_CTRL) != 0;
            Organism *b = a->selected;
            if (ui_key_pressed(SDLK_SLASH))
                act = A_LAUNCHER;
            if (ui_key_pressed(SDLK_BACKQUOTE))
                act = A_CONSOLE;
            if (ui_key_pressed(SDLK_F11))
                act = A_FULLSCREEN;
            if (ui_key_pressed(SDLK_PERIOD))
                act = A_STEP;
            if (b && ui_key_pressed(SDLK_t))
                act = A_TRACK;
            if (b && ui_key_pressed(SDLK_g))
                act = A_GENES;
            if (b && b->alive && ui_key_pressed(SDLK_f))
                act = A_FEED;
            if (b && b->alive && ui_key_pressed(SDLK_DELETE))
                act = A_KILL;
            if (b && b->alive && ctrl && ui_key_pressed(SDLK_c))
                act = A_COPY;
            if (ctrl && ui_key_pressed(SDLK_v) && a->clipboard) {
                double cx, cy;
                int mx, my;
                ui_mouse(&mx, &my);
                wv_screen_to_world(&a->view, mx, my, &cx, &cy);
                a->popup_wx = (int)floor(cx);
                a->popup_wy = (int)floor(cy);
                act = A_PASTE;
            }
        }
        if (!shell_open && ui_key_pressed(SDLK_ESCAPE))
            a->selected = NULL;
    }
    ui_layer_end();

    /* windows, back to front; modal ones last; the SYN-OS main menu and
     * options hide the world's windows until it is back */
    SynosScreen scr = synos_screen();
    if (synos) {
        /* SYN-OS: the laboratory is a screen of its own */
        if (scr == SYNOS_WORLD || scr == SYNOS_STATS || scr == SYNOS_ARENA || scr == SYNOS_LIBRARY) {
            stats_draw(a);
            params_draw(a);
        }
        if (scr == SYNOS_LAB)
            lab_draw(a);
        Action t = synos_overlays(a, W, H);
        if (t != A_NONE)
            act = t;
    } else {
        stats_draw(a);
        lab_draw(a);
        params_draw(a);
    }
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
 * SYN_BIOGENESIS_SHOT_LOAD=world.bgw loads a world first, and
 * SYN_BIOGENESIS_SHOT_VIEW=x,y,zoom centres the view there. */
static void shot_prepare(App *a, const char *ui)
{
    const char *load = getenv("SYN_BIOGENESIS_SHOT_LOAD");
    if (load) {
        char err[256];
        if (!bgw_load(&a->world, load, err, sizeof err))
            fprintf(stderr, "%s: %s\n", load, err);
        wv_clamp(&a->view, &a->world);
        syn_history_reset(&a->world, true);
        syn_sidecar_load(&a->world, load);
        /* a world from a file: its seed isn't known, and it is the file's */
        g_made_known = false;
        if (synos_active()) {
            snprintf(a->game_file, sizeof a->game_file, "%s", load);
            syn_shell_notify_clear();
        }
    }
    const char *sim = getenv("SYN_BIOGENESIS_SHOT_SIM");
    long n = sim ? strtol(sim, NULL, 10) : 0;
    for (long i = 0; i < n; i++) {
        world_time(&a->world);
        syn_history_frame(&a->world);
    }
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
    /* Color Mod's default window (640x480) is too small for this one's
     * dialogs; a size the user chose is kept */
    int ww = P.window_width > 640 ? P.window_width : 1100;
    int wh = P.window_height > 480 ? P.window_height : 760;
    g_switch = 0;
    g_entered = false;
    g_genesis_end = SYN_GENESIS_DONE;
    /* the window Biogenesis 0.9 had, if it ran first (host.h) */
    a->window = bio_host.window;
    if (a->window) {
        SDL_SetWindowTitle(a->window, WINDOW_TITLE);
    } else {
        a->window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                     ww, wh, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    }
    if (!a->window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetWindowMinimumSize(a->window, 640, 420);
    a->ren = bio_host.renderer;
    if (!a->ren)
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
    bio_host.window = a->window;
    bio_host.renderer = a->ren;
    SDL_RenderSetClipRect(a->ren, NULL);
    if (!syn_init(a->ren))
        fprintf(stderr, "SYN edition: textures: %s\n", SDL_GetError());
    if (!synos_init(a->ren, a->window, "BIOGENESIS", "COLOR MOD 2.01"))
        fprintf(stderr, "SYN edition: no fonts or themes for the SYN-OS interface (assets)\n");
    set_icon(a->window);
    SDL_StopTextInput(); /* on only while a text field has focus */

    const char *home = home_dir();
    char cwd[1024];
    const char *start = getcwd(cwd, sizeof cwd) ? path_slashes(cwd) : (home ? home : "/");
    snprintf(a->last_dir_world, sizeof a->last_dir_world, "%s", start);
    snprintf(a->last_dir_genes, sizeof a->last_dir_genes, "%s", start);
    snprintf(a->last_dir_image, sizeof a->last_dir_image, "%s", start);

    world_init(&a->world);
    a->world.on_remove = on_remove;
    a->world.ud = a;
    /* the codes the window holds, for the collector */
    jgc_add_root((void **)&a->clipboard);
    jgc_add_root((void **)&a->pending_gc);
    wv_init(&a->view);
    int W, H;
    SDL_GetRendererOutputSize(a->ren, &W, &H);
    a->view.view = (SDL_Rect){0, MENU_H + TOOL_H, W, H - MENU_H - TOOL_H - STATUS_H};
    new_world(a);
    /* stopped part way, it says so */
    if (g_genesis_end != SYN_GENESIS_STOPPED)
        a->status[0] = '\0';
    /* the window was closed while it was being made */
    a->running = g_genesis_end != SYN_GENESIS_QUIT;
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
        /* SYN_BIOGENESIS_SHOT_SCREEN = menu | options | world | stats | arena
         * (world with a window or a selection) */
        const char *scr = getenv("SYN_BIOGENESIS_SHOT_SCREEN");
        if (scr ? !strcmp(scr, "world") : getenv("SYN_BIOGENESIS_SHOT_UI") != NULL) {
            synos_set_screen(SYNOS_WORLD);
            g_entered = true;
        } else if (scr && !strcmp(scr, "options")) {
            synos_set_screen(SYNOS_OPTIONS);
        } else if (scr && !strcmp(scr, "stats")) {
            synos_set_screen(SYNOS_STATS);
            g_entered = true;
        } else if (scr && !strcmp(scr, "arena")) {
            /* SYN_BIOGENESIS_SHOT_ARENA runs rounds first (syn/arena_screen.h) */
            syn_arena_shot_prepare();
            synos_set_screen(SYNOS_ARENA);
        } else if (scr && !strcmp(scr, "params")) {
            /* SYN_BIOGENESIS_SHOT_TAB names the tab (syn/params_screen.c) */
            syn_params_open(a);
            synos_set_screen(SYNOS_PARAMS);
            g_entered = true;
        } else if (scr && !strcmp(scr, "library")) {
            /* SYN_BIOGENESIS_SHOT_LIBRARY names the folder, or "ladder" */
            syn_library_shot_prepare();
            synos_set_screen(SYNOS_LIBRARY);
        }
    }

    while (a->running) {
        SDL_Event e;
        bool idle = !a->active && !a->fdlg.win.open && !shot && !script_text;
        /* SYN-OS keeps drawing while the view moves and on its menus */
        idle = idle && !(synos_active() &&
                         (syn_animating() || synos_screen() != SYNOS_WORLD || syn_shell_animating()));
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
        /* the arena's rounds go on whatever is shown */
        syn_arena_poll();
        frame(a);
        synos_end_frame();
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
    a->pending_gc = NULL;
    a->clipboard = NULL;
    world_free(&a->world);
    syn_history_free();
    syn_stats_free();
    /* the arena's workers stop with the simulation they run */
    syn_arena_free();
    syn_arena_screen_free();
    syn_library_screen_free();
    wv_free(&a->view);
    synos_shutdown();
    syn_shutdown();
    gfx_shutdown();
    /* the launcher keeps the window for Biogenesis 0.9, or closes it */
    return g_switch;
}
