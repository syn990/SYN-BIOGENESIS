/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Part of the SYN edition of SYN-BIOGENESIS, a C port of Biogenesis
 * (https://biogenesis.sourceforge.net/).
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

#include "syn/library_screen.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sim/params.h"
#include "syn/arena.h"
#include "syn/batch.h"
#include "syn/library.h"
#include "syn/palette.h"
#include "syn/shell.h"
#include "syn/telescope_screen.h"
#include "synos/synos.h"
#include "ui/app.h"
#include "ui/gfx.h"
#include "ui/ui.h"

#define PAD 24.0f
#define GAP 14.0f
#define MAXF 256
#define MAXB 2000
#define CARD_W 150.0f
#define CARD_H 178.0f
#define ROW_H 30.0f
#define CONFIRM_MS 4000

#ifdef BIO_COLOUR
#define SIM_TITLE "Color Mod 2.01"
#else
#define SIM_TITLE "Biogenesis 0.9"
#endif

/* what a second click within a few seconds confirms */
enum { ASK_NONE, ASK_DELETE, ASK_EMPTY };

static struct {
    Batch b;
    bool ready, scanned;
    char err[300];
    char folders[MAXF][LIB_PATH];
    int counts[MAXF];
    int nf;
    char folder[LIB_PATH]; /* the one shown */
    bool ladder;           /* the arena's ladder shown instead */
    LibEntry *e;
    GeneticCode **gc; /* the folder's codes, read with it */
    int ne;
    int sel; /* the picked being, -1 for none */
    float fscroll, bscroll;
    Uint32 counted_at;
    char name[LIB_NAME];  /* the picked being's name, as edited */
    char fname[LIB_NAME]; /* the folder's, as edited */
    char status[400];
    Uint32 status_at;
    int ask;
    Uint32 ask_at;
    int back;
    bool marker;
} L = {.sel = -1, .back = SYNOS_MENU};

static int g_mx, g_my;

/* ---- the little things every SYN screen has ---- */

static SDL_Rect sdl(UIRect r)
{
    return (SDL_Rect){(int)floorf(r.x), (int)floorf(r.y), (int)ceilf(r.w), (int)ceilf(r.h)};
}

static bool over(UIRect r)
{
    return ui_hover(sdl(r));
}

static bool button(UIRect r, const char *label, bool active, bool enabled)
{
    bool h = enabled && over(r);
    skin_button(r, synos_fonts()->bold, label,
                (WidgetState){.hover = h, .active = active, .disabled = !enabled}, true);
    return h && ui_pressed(sdl(r), 1);
}

static SDL_Color raw(int c, Uint8 a)
{
    uint8_t r, g, b;
    bio_color_rgb(c, &r, &g, &b);
    return (SDL_Color){r, g, b, a};
}

/* A code's body in r, on black, as the laboratory draws it. */
static void body(UIRect r, const GeneticCode *gc)
{
    skin_fill(r, (Rgb){0, 0, 0}, 1);
    if (!gc)
        return;
    int n = syn_genes(gc) * gc->symmetry;
    GcLine stackbuf[256];
    GcLine *lines = n <= 256 ? stackbuf : malloc((size_t)n * sizeof *lines);
    if (!lines)
        return;
    int w, h;
    int m = gc_layout(gc, lines, n, &w, &h);
    float scale = fminf(r.w * 0.86f / (float)(w > 0 ? w : 1), r.h * 0.86f / (float)(h > 0 ? h : 1));
    if (scale > 4)
        scale = 4;
    float cx = r.x + r.w / 2, cy = r.y + r.h / 2, hw = fminf(1.6f, 0.7f + 0.25f * scale);
    batch_begin(&L.b, TX_DISC, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < m; i++)
        batch_capsule(&L.b, cx + (float)lines[i].x0 * scale, cy + (float)lines[i].y0 * scale,
                      cx + (float)lines[i].x1 * scale, cy + (float)lines[i].y1 * scale, hw,
                      raw(lines[i].color, 255));
    batch_flush(&L.b, gfx_renderer());
    if (lines != stackbuf)
        free(lines);
}

/* Text wrapped to width w from (x, y). */
static void para(TTF_Font *f, const char *text, float x, float y, float w, Tone t)
{
    char line[400];
    size_t n = 0;
    float lh = skin_text_height(f) + 4;
    const char *p = text;
    while (*p) {
        const char *q = p;
        while (*q == ' ')
            q++;
        while (*q && *q != ' ')
            q++;
        char cand[400];
        snprintf(cand, sizeof cand, "%.*s%.*s", (int)n, line, (int)(q - p), n ? p : p + strspn(p, " "));
        if (n && skin_text_width(f, cand) > w) {
            line[n] = '\0';
            skin_text(f, line, x, y, t, ALIGN_LEFT);
            y += lh;
            n = 0;
            p += strspn(p, " ");
            continue;
        }
        n = strlen(cand) < sizeof line ? strlen(cand) : sizeof line - 1;
        memcpy(line, cand, n);
        p = q;
    }
    if (n) {
        line[n] = '\0';
        skin_text(f, line, x, y, t, ALIGN_LEFT);
    }
}

static void say(const char *fmt, const char *a, const char *b)
{
    /* cut short if it must be */
    if (snprintf(L.status, sizeof L.status, fmt, a ? a : "", b ? b : "") < 0)
        L.status[0] = '\0';
    L.status_at = SDL_GetTicks();
}

/* ---- codes kept between frames ---- */

/* Color Mod's codes are its collector's, marked while this holds them;
 * 0.9's are counted (bgg_read gives one reference). */
#ifdef BIO_COLOUR
static void drop(GeneticCode *gc)
{
    (void)gc;
}

static void library_mark(void)
{
    for (int i = 0; i < L.ne; i++)
        jgc_mark(L.gc[i]);
}
#else
static void drop(GeneticCode *gc)
{
    gc_unref(gc);
}
#endif

static void forget(void)
{
    for (int i = 0; i < L.ne; i++)
        drop(L.gc[i]);
    free(L.e);
    free(L.gc);
    L.e = NULL;
    L.gc = NULL;
    L.ne = 0;
}

static int folder_index(const char *f)
{
    for (int i = 0; i < L.nf; i++)
        if (!strcmp(L.folders[i], f))
            return i;
    return -1;
}

/* The folders and their counts again, and the shown folder's beings; the
 * picked being stays picked if it is still there (or is named keep). */
static void rescan(const char *keep)
{
#ifdef BIO_COLOUR
    if (!L.marker) {
        jgc_add_marker(library_mark);
        L.marker = true;
    }
#endif
    if (!L.ready)
        L.ready = syn_library_ready(L.err, sizeof L.err);
    char picked[LIB_NAME] = "";
    if (keep)
        snprintf(picked, sizeof picked, "%s", keep);
    else if (!L.ladder && L.sel >= 0 && L.sel < L.ne)
        snprintf(picked, sizeof picked, "%s", L.e[L.sel].name);
    L.nf = syn_library_folders(L.folders, MAXF);
    for (int i = 0; i < L.nf; i++)
        L.counts[i] = syn_library_count(L.folders[i]);
    if (folder_index(L.folder) < 0)
        L.folder[0] = '\0';
    forget();
    L.sel = -1;
    if (!L.ladder) {
        L.e = malloc(MAXB * sizeof *L.e);
        L.ne = L.e ? syn_library_list(L.folder, L.e, MAXB) : 0;
        L.gc = calloc((size_t)(L.ne ? L.ne : 1), sizeof *L.gc);
        if (!L.gc)
            abort();
        char err[200];
        for (int i = 0; i < L.ne; i++) {
            L.gc[i] = syn_library_read(L.folder, L.e[i].name, err, sizeof err);
            if (picked[0] && !strcmp(L.e[i].name, picked))
                L.sel = i;
        }
    }
    snprintf(L.name, sizeof L.name, "%s", L.sel >= 0 ? L.e[L.sel].name : "");
    const char *slash = strrchr(L.folder, '/');
    snprintf(L.fname, sizeof L.fname, "%s", slash ? slash + 1 : L.folder);
    L.counted_at = SDL_GetTicks();
    L.scanned = true;
}

static void show(const char *folder, bool ladder)
{
    snprintf(L.folder, sizeof L.folder, "%s", folder);
    L.ladder = ladder;
    L.bscroll = 0;
    L.ask = ASK_NONE;
    rescan("");
}

/* ---- what can be done ---- */

/* the picked being: its code, its name, and (for the ladder) its entry */
static const GeneticCode *picked(const char **name, const ArenaEntry **entry)
{
    *name = "";
    *entry = NULL;
    if (L.ladder) {
        const ArenaEntry *e = L.sel >= 0 && L.sel < syn_arena_count() ? syn_arena_at(L.sel) : NULL;
        if (!e)
            return NULL;
        *name = e->name;
        *entry = e;
        return e->gc;
    }
    if (L.sel < 0 || L.sel >= L.ne)
        return NULL;
    *name = L.e[L.sel].name;
    return L.gc[L.sel];
}

/* in the world: at its middle, else the nearest room around it */
static void place(App *a, const GeneticCode *gc, const char *name)
{
    int cx = a->world.width / 2, cy = a->world.height / 2;
    for (int ring = 0; ring < 12; ring++) {
        int steps = ring ? ring * 8 : 1;
        for (int k = 0; k < steps; k++) {
            double ang = 2 * M_PI * k / steps;
            int x = cx + (int)lround(ring * 60 * cos(ang)), y = cy + (int)lround(ring * 60 * sin(ang));
            if (app_paste_code(a, (GeneticCode *)gc, x, y)) {
                char where[64];
                snprintf(where, sizeof where, "%d, %d", x, y);
                say("%s is in the world at %s", name, where);
                return;
            }
        }
    }
    say("No room for %s near the middle of the world%s", name, "");
}

static void enter_arena(const GeneticCode *gc, const char *name)
{
    char err[200];
    const ArenaEntry *e = syn_arena_enter(gc, name, err, sizeof err);
    if (e)
        say("%s is on the arena's ladder at level 1, for the arena's settings as they are now%s",
            e->name, "");
    else
        say("Not on the ladder: %s%s", err, "");
}

static void keep_from_ladder(App *a, const ArenaEntry *e)
{
    char origin[200], st[120];
    syn_arena_standing(e, st, sizeof st);
    snprintf(origin, sizeof origin, "the arena's ladder: level %d, %d wins; %s", e->level, e->wins, st);
    syn_library_keep(a, e->gc, "Arena", e->name, origin);
}

/* ---- the folders ---- */

static void folders_view(UIRect r)
{
    const Fonts *fo = synos_fonts();
    skin_text(fo->small_bold, "FOLDERS", r.x, r.y, TONE_MUTED, ALIGN_LEFT);
    float top = r.y + skin_text_height(fo->small_bold) + 8;
    /* the folder buttons at the bottom; the name field above them */
    float by = r.y + r.h - 34, fy = by - 8 - 30;
    UIRect list = {r.x, top, r.w, fy - 10 - top};
    int rows = L.nf + 2; /* the folders, a gap, the ladder */
    float maxs = fmaxf(0, (float)rows * ROW_H - list.h);
    if (over(list))
        L.fscroll -= ui_wheel_fy() * ROW_H;
    L.fscroll = fminf(fmaxf(L.fscroll, 0), maxs);
    gfx_clip_push(sdl(list));
    for (int i = 0; i < rows; i++) {
        if (i == L.nf)
            continue;
        UIRect row = {list.x, list.y + (float)i * ROW_H - L.fscroll, list.w, ROW_H};
        if (row.y + row.h < list.y || row.y > list.y + list.h)
            continue;
        bool ladder = i == L.nf + 1;
        bool on = ladder ? L.ladder : !L.ladder && !strcmp(L.folders[i], L.folder);
        if (on)
            skin_fill(row, skin_theme()->accent, 0.22f);
        else if (over(row) && over(list))
            skin_row_hover(row);
        char label[LIB_PATH], count[16], fit[LIB_PATH];
        int depth = 0;
        if (ladder) {
            snprintf(label, sizeof label, "ARENA LADDER");
            snprintf(count, sizeof count, "%d", syn_arena_count());
        } else {
            const char *f = L.folders[i], *slash = strrchr(f, '/');
            for (const char *c = f; *c; c++)
                depth += *c == '/';
            depth += *f ? 1 : 0;
            snprintf(label, sizeof label, "%s", !*f ? "Library" : slash ? slash + 1 : f);
            snprintf(count, sizeof count, "%d", L.counts[i]);
        }
        float x = row.x + 8 + 16.0f * (float)depth;
        float cw = skin_text_width(fo->small, count);
        skin_fit(fo->body, label, row.x + row.w - 16 - cw - x, fit, sizeof fit);
        float ty = row.y + (ROW_H - skin_text_height(fo->body)) / 2;
        skin_text(fo->body, fit, x, ty, ladder ? TONE_ACCENT : TONE_TEXT, ALIGN_LEFT);
        skin_text(fo->small, count, row.x + row.w - 8, ty + 2, TONE_MUTED, ALIGN_RIGHT);
        if (over(row) && over(list)) {
            if (ladder)
                ui_tooltip("The arena's beings for its settings as they are now, best first: keep "
                           "any of them in the library");
            if (ui_pressed(sdl(row), 1))
                show(ladder ? "" : L.folders[i], ladder);
        }
    }
    gfx_clip_pop();

    /* the shown folder's name, to change; then NEW FOLDER, REMOVE */
    bool fixed = L.ladder || !L.folder[0] || !strcmp(L.folder, LIB_BIN);
    UIRect field = {r.x, fy, r.w, 30};
    if (fixed) {
        skin_input(field, false, false, false);
        skin_text(fo->small, L.ladder ? "the arena's, read only" : !L.folder[0] ? "the library itself"
                                                                                  : "the Bin",
                  field.x + 8, field.y + 7, TONE_MUTED, ALIGN_LEFT);
    } else if (ui_textfield(ui_id("libfolder", 0), sdl(field), L.fname, sizeof L.fname)) {
        char got[LIB_PATH], err[300];
        if (syn_library_rename_folder(L.folder, L.fname, got, sizeof got, L.err, sizeof L.err)) {
            snprintf(L.folder, sizeof L.folder, "%s", got);
            say("The folder is %s now%s", got, "");
        } else {
            snprintf(err, sizeof err, "%s", L.err);
            say("Not renamed: %s%s", err, "");
        }
        rescan(NULL);
    }
    if (over(field) && !fixed)
        ui_tooltip("The folder's name: type a new one and press Enter");
    float bw = (r.w - 8) / 2;
    UIRect nb = {r.x, by, bw, 34}, rb = {r.x + bw + 8, by, bw, 34};
    if (button(nb, "NEW FOLDER", false, !L.ladder)) {
        char got[LIB_PATH], err[300];
        if (syn_library_new_folder(L.folder, "New folder", got, sizeof got, err, sizeof err)) {
            show(got, false);
            say("Made %s: name it in the field above%s", got, "");
        } else {
            say("No folder made: %s%s", err, "");
        }
    }
    if (over(nb))
        ui_tooltip("A folder inside the one shown");
    bool bin = !L.ladder && !strcmp(L.folder, LIB_BIN);
    if (bin) {
        bool asked = L.ask == ASK_EMPTY && SDL_GetTicks() - L.ask_at < CONFIRM_MS;
        if (button(rb, asked ? "SURE?" : "EMPTY BIN", asked, L.ne > 0)) {
            if (asked) {
                char n[16];
                snprintf(n, sizeof n, "%d", syn_library_empty_bin());
                L.ask = ASK_NONE;
                rescan(NULL);
                say("%s beings removed for good%s", n, "");
            } else {
                L.ask = ASK_EMPTY;
                L.ask_at = SDL_GetTicks();
            }
        }
        if (over(rb))
            ui_tooltip("Remove the beings in the Bin for good (click twice)");
    } else {
        bool can = !fixed && L.ne == 0;
        if (button(rb, "REMOVE", false, can)) {
            char err[300], was[LIB_PATH];
            snprintf(was, sizeof was, "%s", L.folder);
            if (syn_library_remove_folder(L.folder, err, sizeof err)) {
                const char *slash = strrchr(was, '/');
                char up[LIB_PATH];
                snprintf(up, sizeof up, "%.*s", slash ? (int)(slash - was) : 0, was);
                show(up, false);
                say("Removed the folder %s%s", was, "");
            } else {
                say("Not removed: %s%s", err, "");
            }
        }
        if (over(rb))
            ui_tooltip(can ? "Remove this folder" : "Only an empty folder can be removed");
    }
}

/* ---- the beings ---- */

static void cards_view(App *a, UIRect r)
{
    const Fonts *fo = synos_fonts();
    char title[LIB_PATH + 64];
    int n = L.ladder ? syn_arena_count() : L.ne;
    if (L.ladder)
        snprintf(title, sizeof title, "THE ARENA'S LADDER  ·  %d being%s for its settings now", n,
                 n == 1 ? "" : "s");
    else
        snprintf(title, sizeof title, "%s  ·  %d being%s", L.folder[0] ? L.folder : "LIBRARY", n,
                 n == 1 ? "" : "s");
    char fit[LIB_PATH + 64];
    skin_text(fo->small_bold, skin_fit(fo->small_bold, title, r.w, fit, sizeof fit), r.x, r.y,
              TONE_MUTED, ALIGN_LEFT);
    float top = r.y + skin_text_height(fo->small_bold) + 8;
    UIRect grid = {r.x, top, r.w, r.y + r.h - top};
    if (n == 0) {
        const char *hint = L.ladder ? "The arena has no beings for its settings yet: start its battles."
                           : !strcmp(L.folder, LIB_BIN)
                               ? "The Bin is empty. Deleted beings come here first."
                               : "No beings here yet. Keep one from the world (its Keep button), the "
                                 "laboratory or the arena's ladder, or IMPORT a .bgg file.";
        para(fo->body, hint, grid.x, grid.y + 10, grid.w, TONE_MUTED);
        return;
    }
    int cols = (int)floorf((grid.w + GAP) / (CARD_W + GAP));
    if (cols < 1)
        cols = 1;
    int rows = (n + cols - 1) / cols;
    float maxs = fmaxf(0, (float)rows * (CARD_H + GAP) - GAP - grid.h);
    if (over(grid))
        L.bscroll -= ui_wheel_fy() * (CARD_H + GAP) / 2;
    L.bscroll = fminf(fmaxf(L.bscroll, 0), maxs);
    uint32_t pop = ui_id("libcard", 0);
    int take = ui_popup_take(pop);
    gfx_clip_push(sdl(grid));
    for (int i = 0; i < n; i++) {
        UIRect c = {grid.x + (float)(i % cols) * (CARD_W + GAP),
                    grid.y + (float)(i / cols) * (CARD_H + GAP) - L.bscroll, CARD_W, CARD_H};
        if (c.y + c.h < grid.y || c.y > grid.y + grid.h)
            continue;
        const GeneticCode *gc;
        const char *nm, *sub;
        char subbuf[200];
        if (L.ladder) {
            const ArenaEntry *e = syn_arena_at(i);
            gc = e->gc;
            nm = e->name;
            snprintf(subbuf, sizeof subbuf, "level %d  ·  %d wins", e->level, e->wins);
            sub = subbuf;
        } else {
            gc = L.gc[i];
            nm = L.e[i].name;
            sub = L.e[i].origin[0] ? L.e[i].origin : L.e[i].saved;
        }
        bool on = i == L.sel;
        skin_fill(c, on ? skin_theme()->accent : skin_theme()->bg_alt, on ? 0.30f : 0.6f);
        UIRect pic = {c.x + 6, c.y + 6, c.w - 12, c.w - 22};
        body(pic, gc);
        if (!gc)
            skin_text(fo->small, "can't be read", pic.x + pic.w / 2, pic.y + pic.h / 2 - 8, TONE_BAD,
                      ALIGN_CENTER);
        char f1[LIB_NAME + 8], f2[200];
        float ty = pic.y + pic.h + 6;
        skin_text(fo->bold, skin_fit(fo->bold, nm, c.w - 12, f1, sizeof f1), c.x + 6, ty, TONE_TEXT,
                  ALIGN_LEFT);
        skin_text(fo->small, skin_fit(fo->small, sub, c.w - 12, f2, sizeof f2), c.x + 6,
                  ty + skin_text_height(fo->bold) + 2, TONE_MUTED, ALIGN_LEFT);
        if (over(c) && over(grid)) {
            if (!on)
                skin_fill(c, skin_theme()->accent, 0.10f);
            char tip[400];
            if (L.ladder) {
                snprintf(tip, sizeof tip, "%s: %s. Double-click for its genes.", nm, syn_arena_at(i)->plan);
            } else {
                snprintf(tip, sizeof tip, "%s%s%s%s%s. Double-click for its genes.", nm,
                         L.e[i].origin[0] ? ", from " : "", L.e[i].origin, L.e[i].saved[0] ? ", kept " : "",
                         L.e[i].saved);
            }
            ui_tooltip(tip);
            if (ui_pressed(sdl(c), 1) || ui_pressed(sdl(c), 3)) {
                if (L.sel != i)
                    L.ask = ASK_NONE;
                L.sel = i;
                snprintf(L.name, sizeof L.name, "%s", nm);
                if (ui_pressed(sdl(c), 1) && ui_double_clicked() && gc)
                    lab_open(a, gc);
            }
            if (ui_pressed(sdl(c), 3) && gc) {
                const char *items[] = {"Open in the laboratory", "Copy genetic code", "Place in the world",
                                       L.ladder ? "Keep in the library" : "Put on the arena's ladder"};
                ui_popup_open(pop, g_mx, g_my, items, NULL, 4);
            }
        }
    }
    gfx_clip_pop();
    const char *nm;
    const ArenaEntry *entry;
    const GeneticCode *gc = take >= 0 ? picked(&nm, &entry) : NULL;
    if (gc) {
        if (take == 0)
            lab_open(a, gc);
        else if (take == 1) {
            app_set_clipboard(a, (GeneticCode *)gc);
            say("%s copied: paste it into the world from its menu%s", nm, "");
        } else if (take == 2)
            place(a, gc, nm);
        else if (take == 3 && entry)
            keep_from_ladder(a, entry);
        else if (take == 3)
            enter_arena(gc, nm);
    }
}

/* the picked being's name and what can be done with it */
static void picked_view(App *a, UIRect r)
{
    const Fonts *fo = synos_fonts();
    const char *nm;
    const ArenaEntry *entry;
    const GeneticCode *gc = picked(&nm, &entry);
    skin_separator(r.x, r.y, r.w);
    float y = r.y + 12;
    if (!gc) {
        para(fo->body, "Pick a being to rename it, move it, put it on the arena's ladder or place it in the world.",
             r.x, y + 8, r.w, TONE_MUTED);
        return;
    }
    UIRect field = {r.x, y, 300, 34};
    if (entry) {
        skin_text(fo->heading, nm, r.x, y + 2, TONE_TEXT, ALIGN_LEFT);
    } else {
        if (ui_textfield(ui_id("libname", 0), sdl(field), L.name, sizeof L.name)) {
            char got[LIB_NAME], err[300], was[LIB_NAME];
            snprintf(was, sizeof was, "%s", nm);
            if (syn_library_rename(L.folder, was, L.name, got, sizeof got, err, sizeof err)) {
                rescan(got);
                if (strcmp(was, got))
                    say("%s is %s now", was, got);
            } else {
                say("Not renamed: %s%s", err, "");
            }
        }
        if (over(field))
            ui_tooltip("Its name: type a new one and press Enter");
    }
    /* where it came from */
    char about[400];
    if (entry) {
        char st[120];
        syn_arena_standing(entry, st, sizeof st);
        snprintf(about, sizeof about, "Level %d on the ladder, %d wins, %s. %s", entry->level, entry->wins,
                 st, entry->plan);
    } else {
        const LibEntry *e = &L.e[L.sel];
        snprintf(about, sizeof about, "%s%s%s%s", e->origin[0] ? "From " : "Put here by hand",
                 e->origin, e->saved[0] ? ", kept " : "", e->saved);
    }
    char fit[400];
    float ax = field.x + field.w + 16;
    skin_text(fo->small, skin_fit(fo->small, about, r.x + r.w - ax, fit, sizeof fit), ax, y + 10,
              TONE_MUTED, ALIGN_LEFT);

    /* the actions */
    y += 34 + 12;
    struct {
        const char *label, *tip;
        bool show;
    } act[] = {
        {"TO THE ARENA", "On the arena's ladder at level 1, to fight others of its level", !entry},
        {"KEEP IN THE LIBRARY", "A copy in the library's Arena folder", entry != NULL},
        {"INTO THE WORLD", "Into the world, at its middle or the nearest room", true},
        {"GENES", "Its genes in the genetic laboratory", true},
        {"COPY", "Its code to paste into the world from its menu", true},
        {"MOVE TO...", "Into another folder", !entry},
        {!strcmp(L.folder, LIB_BIN) ? "DELETE FOR GOOD" : "DELETE",
         !strcmp(L.folder, LIB_BIN) ? "Remove it for good (click twice)" : "To the Bin", !entry},
        {"WHERE IT LIVES", "In the telescope: the seeds and arena rounds its genes were drawn in, as scenes to open",
#ifdef BIO_COLOUR
         false
#else
         true
#endif
        },
    };
    float x = r.x;
    uint32_t mpop = ui_id("libmove", 0);
    int to = ui_popup_take(mpop);
    if (to >= 0 && to < L.nf) {
        char err[300];
        char dest[LIB_PATH];
        snprintf(dest, sizeof dest, "%s", L.folders[to]);
        if (syn_library_move(L.folder, nm, dest, err, sizeof err)) {
            say("Moved to %s%s", dest[0] ? dest : "the library", "");
            rescan("");
        } else {
            say("Not moved: %s%s", err, "");
        }
        return;
    }
    for (size_t i = 0; i < sizeof act / sizeof *act; i++) {
        if (!act[i].show)
            continue;
        bool asked = i == 6 && L.ask == ASK_DELETE && SDL_GetTicks() - L.ask_at < CONFIRM_MS;
        const char *label = asked ? "SURE?" : act[i].label;
        float bw = skin_text_width(fo->bold, label) + 32;
        /* a second row when the first is full */
        if (x > r.x && x + bw > r.x + r.w) {
            x = r.x;
            y += 34 + 8;
        }
        UIRect b = {x, y, bw, 34};
        x += bw + 8;
        bool hit = button(b, label, asked, true);
        if (over(b))
            ui_tooltip(act[i].tip);
        if (!hit)
            continue;
        switch (i) {
        case 0:
            enter_arena(gc, nm);
            break;
        case 1:
            keep_from_ladder(a, entry);
            break;
        case 2:
            place(a, gc, nm);
            break;
        case 3:
            lab_open(a, gc);
            break;
        case 4:
            app_set_clipboard(a, (GeneticCode *)gc);
            say("%s copied: paste it into the world from its menu%s", nm, "");
            break;
        case 5: {
            const char *items[32];
            int k = 0;
            for (int f = 0; f < L.nf && k < 32; f++)
                items[k++] = L.folders[f][0] ? L.folders[f] : "Library";
            ui_popup_open(mpop, (int)b.x, (int)(b.y + b.h), items, NULL, k);
            break;
        }
        case 6: {
            bool bin = !strcmp(L.folder, LIB_BIN);
            if (bin && !asked) {
                L.ask = ASK_DELETE;
                L.ask_at = SDL_GetTicks();
                break;
            }
            char err[300], was[LIB_NAME];
            snprintf(was, sizeof was, "%s", nm);
            L.ask = ASK_NONE;
            if (syn_library_delete(L.folder, was, err, sizeof err)) {
                say(bin ? "%s removed for good%s" : "%s is in the Bin%s", was, "");
                rescan("");
            } else {
                say("Not deleted: %s%s", err, "");
            }
            break;
        }
        case 7:
            syn_telescope_find(gc, nm);
            synos_set_screen(SYNOS_TELESCOPE);
            return;
        }
        break;
    }
}

/* ---- the screen ---- */

void syn_library_open_from(int screen)
{
    L.back = screen;
}

void syn_library_frame(App *a, int w, int h)
{
    const Fonts *fo = synos_fonts();
    ui_mouse(&g_mx, &g_my);
    if (!L.scanned)
        rescan(NULL);
    /* files put in or taken out by hand show within a few seconds */
    if (SDL_GetTicks() - L.counted_at > 3000) {
        static char f[MAXF][LIB_PATH];
        int nf = syn_library_folders(f, MAXF);
        bool changed = nf != L.nf || (!L.ladder && syn_library_count(L.folder) != L.ne);
        for (int i = 0; i < nf && !changed; i++)
            changed = strcmp(f[i], L.folders[i]) || syn_library_count(f[i]) != L.counts[i];
        L.counted_at = SDL_GetTicks();
        if (changed)
            rescan(NULL);
    }
    bool keys = !ui_any_modal() && !ui_text_focused() && !ui_popup_showing();
    if (keys && ui_key_pressed(SDLK_ESCAPE)) {
        synos_set_screen((SynosScreen)L.back);
        return;
    }
    if (L.ladder && L.sel >= syn_arena_count())
        L.sel = -1;

    skin_fill(ui_rect(0, 0, (float)w, (float)h), (Rgb){0, 0, 0}, 0.5f);
    /* below the shell's bar (syn/shell.h); Esc goes back */
    float top = SHELL_BAR_H + 10;
    float pw = fminf((float)w - 48, 2400), ph = (float)h - top - 24;
    UIRect panel = {((float)w - pw) / 2, top, pw, ph};
    skin_panel(panel, true);

    float x = panel.x + PAD, y = panel.y + 14, right = panel.x + panel.w - PAD;
    x += skin_text(fo->big, "LIBRARY", x, y, TONE_TEXT, ALIGN_LEFT) + 14;
    x += skin_text(fo->body, SIM_TITLE, x, y + 9, TONE_MUTED, ALIGN_LEFT) + 18;
    UIRect imp = {right - 170, y, 170, 36};
    char rootfit[600];
    skin_text(fo->small, skin_fit(fo->small, syn_library_root(), imp.x - 16 - x, rootfit, sizeof rootfit), x,
              y + 12, TONE_MUTED, ALIGN_LEFT);
    if (button(imp, "IMPORT .BGG...", false, true))
        filedlg_open(a, FD_LIBRARY_IMPORT, false, "bgg", a->last_dir_genes);
    if (over(imp))
        ui_tooltip("Copy a genetic code file (.bgg) into the folder shown");
    y += 56;

    float bottom = panel.y + panel.h - PAD;
    float status_h = skin_text_height(fo->small) + 8;
    float pick_h = 12 + 34 + 12 + 34 + 8 + 34 + 8;
    UIRect left = {panel.x + PAD, y, 290, bottom - y};
    UIRect rightr = {left.x + left.w + 24, y, right - left.x - left.w - 24, bottom - y - pick_h - status_h};
    if (!L.ready) {
        skin_text(fo->body, L.err, rightr.x, rightr.y, TONE_BAD, ALIGN_LEFT);
        return;
    }
    folders_view(left);
    cards_view(a, rightr);
    picked_view(a, (UIRect){rightr.x, rightr.y + rightr.h + 8, rightr.w, pick_h});
    if (L.status[0] && SDL_GetTicks() - L.status_at < 8000)
        skin_text(fo->small, L.status, rightr.x, bottom - status_h + 6, TONE_ACCENT, ALIGN_LEFT);
}

bool syn_library_keep(App *a, const GeneticCode *gc, const char *folder, const char *name,
                      const char *origin)
{
    char err[300], got[LIB_NAME], msg[400];
    if (!gc)
        return false;
    if (!syn_library_ready(err, sizeof err) ||
        !syn_library_put(folder, name, gc, origin, got, sizeof got, err, sizeof err)) {
        snprintf(msg, sizeof msg, "Not kept: %s", err);
        app_set_status(a, msg);
        say("%s%s", msg, "");
        return false;
    }
    snprintf(msg, sizeof msg, "Kept in the library: %s/%s", folder, got);
    app_set_status(a, msg);
    say("%s%s", msg, "");
    if (L.scanned)
        rescan(NULL);
    return true;
}

void syn_library_imported(App *a, const char *path)
{
    const char *to = L.ladder || !L.folder[0] ? "Imported" : L.folder;
    char err[300], got[LIB_NAME];
    if (!syn_library_ready(err, sizeof err) ||
        !syn_library_import(path, to, got, sizeof got, err, sizeof err)) {
        char msg[400];
        snprintf(msg, sizeof msg, "Not imported: %s", err);
        app_set_status(a, msg);
        say("%s%s", msg, "");
        return;
    }
    if (strcmp(to, L.folder) || L.ladder)
        snprintf(L.folder, sizeof L.folder, "%s", to);
    L.ladder = false;
    rescan(got);
    say("Imported as %s/%s", to, got);
}

void syn_library_shot_prepare(void)
{
    const char *f = getenv("SYN_BIOGENESIS_SHOT_LIBRARY");
    if (f && !strcmp(f, "ladder"))
        show("", true);
    else
        show(f ? f : "", false);
    if (L.ne > 0)
        L.sel = 0;
    else if (L.ladder && syn_arena_count() > 0)
        L.sel = 0;
    snprintf(L.name, sizeof L.name, "%s", L.sel >= 0 && !L.ladder ? L.e[0].name : "");
}

void syn_library_screen_free(void)
{
    forget();
    batch_free(&L.b);
    memset(&L, 0, sizeof L);
    L.sel = -1;
    L.back = SYNOS_MENU;
}
