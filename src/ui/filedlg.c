/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Part of a C port of Biogenesis (https://biogenesis.sourceforge.net/).
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

/* The file chooser (JFileChooser in the Java version). */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "compat.h"
#include "ui/app.h"
#ifdef BIO_SYNOS
#include "synos/synos.h"
#endif
#ifdef BIO_SYN_EDITION
#include <math.h>
#include <time.h>

#include "io/bgg.h"
#include "syn/shell.h"
#endif

void filedlg_free(FileDialog *d)
{
    for (int i = 0; i < d->n_entries; i++)
        free(d->entries[i]);
    free(d->entries);
    d->entries = NULL;
    d->n_entries = 0;
}

static int cmp_entry(const void *a, const void *b)
{
    const char *x = *(char *const *)a, *y = *(char *const *)b;
    size_t lx = strlen(x), ly = strlen(y);
    bool dx = lx && x[lx - 1] == '/', dy = ly && y[ly - 1] == '/';
    if (dx != dy)
        return dx ? -1 : 1;
    return strcasecmp(x, y);
}

/* ext: one extension or several separated by '|' */
static bool has_ext(const char *name, const char *ext)
{
    const char *dot = strrchr(name, '.');
    if (!dot)
        return false;
    for (const char *e = ext; *e;) {
        size_t n = strcspn(e, "|");
        if (strlen(dot + 1) == n && !strncasecmp(dot + 1, e, n))
            return true;
        e += n + (e[n] == '|');
    }
    return false;
}

static void scan(FileDialog *d)
{
    filedlg_free(d);
    d->scroll = 0;
    d->selected = -1;
    DIR *dir = opendir(d->dir);
    if (!dir)
        return;
    int cap = 64;
    d->entries = malloc((size_t)cap * sizeof *d->entries);
    if (!d->entries)
        abort();
    struct dirent *e;
    while ((e = readdir(dir))) {
        if (e->d_name[0] == '.')
            continue;
        char full[4400];
        snprintf(full, sizeof full, "%s/%s", d->dir, e->d_name);
        struct stat st;
        if (stat(full, &st) != 0)
            continue;
        bool is_dir = S_ISDIR(st.st_mode);
        if (!is_dir && !has_ext(e->d_name, d->ext))
            continue;
        if (d->n_entries == cap) {
            cap *= 2;
            char **ne = realloc(d->entries, (size_t)cap * sizeof *d->entries);
            if (!ne)
                abort();
            d->entries = ne;
        }
        size_t len = strlen(e->d_name);
        char *s = malloc(len + 2);
        if (!s)
            abort();
        memcpy(s, e->d_name, len);
        if (is_dir)
            s[len++] = '/';
        s[len] = '\0';
        d->entries[d->n_entries++] = s;
    }
    closedir(dir);
    qsort(d->entries, (size_t)d->n_entries, sizeof *d->entries, cmp_entry);
}

static void set_dir(FileDialog *d, const char *path)
{
    char resolved[4096];
    if (realpath(path, resolved))
        snprintf(d->dir, sizeof d->dir, "%s", resolved);
    else
        snprintf(d->dir, sizeof d->dir, "%s", path);
    scan(d);
}

void filedlg_open(App *a, FilePurpose purpose, bool save, const char *ext, const char *dir)
{
    FileDialog *d = &a->fdlg;
    d->purpose = purpose;
    d->save = save;
    snprintf(d->ext, sizeof d->ext, "%s", ext);
    d->name[0] = '\0';
    struct stat st;
    if (!dir || !*dir || stat(dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        const char *home = home_dir();
        dir = home ? home : "/";
    }
    set_dir(d, dir);
    ui_window_centre(&d->win, 600, 470);
    d->win.open = true;
    d->focus_name = true;
}

static void up_dir(FileDialog *d)
{
    char p[4096];
    snprintf(p, sizeof p, "%s", d->dir);
    char *slash = strrchr(p, '/');
    if (!slash)
        return;
    if (slash == p)
        p[1] = '\0';
#ifdef _WIN32
    else if (slash == p + 2 && p[1] == ':')
        p[3] = '\0'; /* a drive's root, C:/ */
#endif
    else
        *slash = '\0';
    set_dir(d, p);
}

static void accept(App *a)
{
    FileDialog *d = &a->fdlg;
    if (!d->name[0])
        return;
    char path[4400];
    if (path_absolute(d->name))
        snprintf(path, sizeof path, "%s", d->name);
    else
        snprintf(path, sizeof path, "%s%s%s", d->dir,
                 d->dir[strlen(d->dir) - 1] == '/' ? "" : "/", d->name);
    path_slashes(path);
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        set_dir(d, path);
        d->name[0] = '\0';
        return;
    }
    if (d->save) {
        /* "File extensions automatically added if not specified" (0.8) */
        const char *base = strrchr(path, '/');
        base = base ? base + 1 : path;
        if (!strchr(base, '.')) {
            size_t len = strlen(path);
            snprintf(path + len, sizeof path - len, ".%.*s", (int)strcspn(d->ext, "|"), d->ext);
        }
    } else if (stat(path, &st) != 0) {
        return;
    }
    d->win.open = false;
    app_file_chosen(a, d->purpose, path);
}

#ifdef BIO_SYN_EDITION
/* ---- SYN-OS's: syn-crypter's three columns ---- */

/* the folder above, listed as the dialog lists its own */
static FileDialog g_parent;
static char g_parent_of[4096];
/* the .bgg picked, read once */
static char g_preview_path[4400];
static char *g_preview_text;
static size_t g_preview_len;
static float g_scroll_px;

static SDL_Rect sdlr(UIRect r)
{
    return (SDL_Rect){(int)floorf(r.x), (int)floorf(r.y), (int)ceilf(r.w), (int)ceilf(r.h)};
}

static const char *purpose_title(const FileDialog *d)
{
    switch (d->purpose) {
    case FD_OPEN_WORLD: return "OPEN A WORLD";
    case FD_SAVE_WORLD:
    case FD_SAVE_WORLD_THEN_QUIT: return "SAVE THE WORLD";
    case FD_IMPORT_PASTE: return "A BEING INTO THE WORLD";
    case FD_IMPORT_LAB: return "A BEING INTO THE LABORATORY";
    case FD_EXPORT_GENES: return "EXPORT ITS GENES";
    case FD_SAVE_IMAGE: return "SAVE ITS IMAGE";
    case FD_LIBRARY_IMPORT: return "A BEING INTO THE LIBRARY";
    case FD_TELESCOPE_WORLD: return "A SAVED WORLD FOR THE TELESCOPE";
    default: return d->save ? "SAVE" : "OPEN";
    }
}

static void human_size(long long n, char *out, size_t cap)
{
    if (n < 1024)
        snprintf(out, cap, "%lld B", n);
    else if (n < 1024 * 1024)
        snprintf(out, cap, "%.1f KB", n / 1024.0);
    else
        snprintf(out, cap, "%.1f MB", n / (1024.0 * 1024.0));
}

static void entry_path(const FileDialog *d, const char *e, char *out, size_t cap)
{
    size_t len = strlen(e);
    if (len && e[len - 1] == '/')
        len--;
    snprintf(out, cap, "%s%s%.*s", d->dir, d->dir[strlen(d->dir) - 1] == '/' ? "" : "/", (int)len, e);
}

/* a list of entries in r: folders in the accent; the one picked filled */
static int column(FileDialog *d, UIRect r, int picked, float *scroll, bool live, bool details)
{
    const Fonts *fo = synos_fonts();
    const Theme *t = skin_theme();
    float rh = skin_text_height(fo->mono_small) + 12;
    int clicked = -1;
    float content = (float)d->n_entries * rh, max = fmaxf(0, content - r.h);
    if (live && ui_hover(sdlr(r)))
        *scroll -= ui_wheel_fy() * rh * 3;
    /* the pick in view */
    if (live && picked >= 0) {
        float py = (float)picked * rh;
        if (py < *scroll)
            *scroll = py;
        if (py + rh > *scroll + r.h)
            *scroll = py + rh - r.h;
    }
    *scroll = fminf(fmaxf(*scroll, 0), max);
    gfx_clip_push(sdlr(r));
    int first = (int)(*scroll / rh);
    for (int i = first; i < d->n_entries; i++) {
        float y = r.y + (float)i * rh - *scroll;
        if (y > r.y + r.h)
            break;
        UIRect it = {r.x, y, r.w, rh};
        const char *e = d->entries[i];
        size_t len = strlen(e);
        bool is_dir = len && e[len - 1] == '/';
        bool hv = live && ui_hover(sdlr(it));
        Rgb fg = is_dir ? skin_tone(TONE_ACCENT) : t->text;
        if (i == picked) {
            skin_fill(it, t->accent, 1);
            fg = skin_tone(TONE_ON_ACCENT);
        } else if (hv) {
            skin_row_hover(it);
        }
        float cy = y + rh * 0.5f;
        skin_text_rgb(fo->icon, is_dir ? ICON_FOLDER : ICON_FILE, r.x + 14,
                      cy - skin_text_height(fo->icon) * 0.5f, fg.r, fg.g, fg.b, ALIGN_CENTER);
        char name[300], fit[300];
        snprintf(name, sizeof name, "%.*s", (int)(is_dir ? len - 1 : len), e);
        float nw = r.w - 34 - (details ? 170 : 8);
        skin_text_rgb(fo->mono_small, skin_fit(fo->mono_small, name, nw, fit, sizeof fit), r.x + 30,
                      cy - skin_text_height(fo->mono_small) * 0.5f, fg.r, fg.g, fg.b, ALIGN_LEFT);
        if (details && !is_dir) {
            char path[4400], sz[32], when[32];
            entry_path(d, e, path, sizeof path);
            struct stat st;
            if (stat(path, &st) == 0) {
                human_size((long long)st.st_size, sz, sizeof sz);
                struct tm *tm = localtime(&st.st_mtime);
                if (!tm || !strftime(when, sizeof when, "%d %b %Y", tm))
                    when[0] = '\0';
                Rgb dim = i == picked ? fg : skin_tone(TONE_MUTED);
                float ty = cy - skin_text_height(fo->mono_small) * 0.5f;
                skin_text_rgb(fo->mono_small, sz, r.x + r.w - 104, ty, dim.r, dim.g, dim.b, ALIGN_RIGHT);
                skin_text_rgb(fo->mono_small, when, r.x + r.w - 8, ty, dim.r, dim.g, dim.b, ALIGN_RIGHT);
            }
        }
        if (hv && ui_pressed(sdlr(it), 1))
            clicked = i;
    }
    gfx_clip_pop();
    if (!d->n_entries)
        skin_text(fo->mono_small, "(nothing here)", r.x + 12, r.y + 8, TONE_MUTED, ALIGN_LEFT);
    return clicked;
}

static void syn_draw(App *a)
{
    FileDialog *d = &a->fdlg;
    const Fonts *fo = synos_fonts();
    int W, H;
    SDL_GetRendererOutputSize(gfx_renderer(), &W, &H);
    SDL_Rect all = {0, 0, W, H};
    skin_fill(ui_rect(0, 0, (float)W, (float)H), skin_theme()->bg, 0.55f);
    ui_layer_begin(ui_id("filedlg", 0), all, true);
    float pw = fminf(1120, (float)W - 32), ph = fminf(740, (float)H - SHELL_BAR_H - 26);
    UIRect panel = {((float)W - pw) / 2, fmaxf(SHELL_BAR_H + 10, ((float)H - ph) / 2), pw, ph};
    skin_panel(panel, false);
    float tbh = skin_text_height(fo->mono_small_bold) + 10;
    UIRect tb = {panel.x + 1, panel.y + 1, panel.w - 2, tbh};
    Rgb tc = skin_titlebar(tb);
    skin_text_rgb(fo->mono_small_bold, purpose_title(d), tb.x + 12, tb.y + 5, tc.r, tc.g, tc.b, ALIGN_LEFT);
    char filt[64] = "";
    for (const char *e = d->ext; *e;) {
        size_t n = strcspn(e, "|");
        snprintf(filt + strlen(filt), sizeof filt - strlen(filt), "%s*.%.*s", filt[0] ? "  " : "", (int)n, e);
        e += n + (e[n] == '|');
    }
    skin_text_rgb(fo->mono_small, filt, tb.x + tb.w - 12, tb.y + 5, tc.r, tc.g, tc.b, ALIGN_RIGHT);

    /* places, then the path a segment a button */
    float x = panel.x + 16, y = panel.y + tbh + 12, bh = 30;
    const char *home = home_dir();
    struct {
        const char *label;
        const char *path;
    } places[] = {{"HOME", home}, {"/", "/"}};
    for (size_t i = 0; i < sizeof places / sizeof places[0]; i++) {
        if (!places[i].path)
            continue;
        float bw = skin_text_width(fo->small_bold, places[i].label) + 24;
        UIRect b = {x, y, bw, bh};
        bool hv = ui_hover(sdlr(b));
        skin_button(b, fo->small_bold, places[i].label, (WidgetState){.hover = hv}, false);
        if (hv && ui_pressed(sdlr(b), 1))
            set_dir(d, places[i].path);
        x += bw + 6;
    }
    x += 10;
    char segs[4096];
    snprintf(segs, sizeof segs, "%s", d->dir);
    /* from the right, as many segments as fit */
    char *parts[128];
    int np = 0;
    for (char *p = strtok(segs, "/"); p && np < 128; p = strtok(NULL, "/"))
        parts[np++] = p;
    int from = 0;
    float avail = panel.x + panel.w - 16 - x, need = 0;
    for (int i = np - 1; i >= 0; i--) {
        need += skin_text_width(fo->mono_small, parts[i]) + 22;
        if (need > avail) {
            from = i + 1;
            break;
        }
    }
    for (int i = from; i < np; i++) {
        float bw = skin_text_width(fo->mono_small, parts[i]) + 16;
        UIRect b = {x, y, bw, bh};
        bool last = i == np - 1, hv = !last && ui_hover(sdlr(b));
        if (hv)
            skin_row_hover(b);
        skin_text(fo->mono_small, parts[i], b.x + 8, y + (bh - skin_text_height(fo->mono_small)) / 2,
                  last ? TONE_TEXT : hv ? TONE_ACCENT : TONE_MUTED, ALIGN_LEFT);
        if (hv && ui_pressed(sdlr(b), 1)) {
            char p[4096] = "";
            for (int k = 0; k <= i; k++) {
                strncat(p, "/", sizeof p - strlen(p) - 1);
                strncat(p, parts[k], sizeof p - strlen(p) - 1);
            }
            set_dir(d, p);
            break;
        }
        x += bw;
        if (!last)
            skin_text(fo->mono_small, "/", x, y + (bh - skin_text_height(fo->mono_small)) / 2, TONE_MUTED,
                      ALIGN_LEFT);
        x += 6;
    }
    y += bh + 12;

    /* the columns: the folder above, this one, the preview */
    float bottom = panel.y + panel.h - 16 - 34 - 12 - skin_text_height(fo->mono_small) - 10;
    float colh = bottom - y, gap = 10;
    float w0 = (panel.w - 32 - 2 * gap) * 0.22f, w1 = (panel.w - 32 - 2 * gap) * 0.46f;
    float w2 = panel.w - 32 - 2 * gap - w0 - w1;
    UIRect c0 = {panel.x + 16, y, w0, colh}, c1 = {c0.x + w0 + gap, y, w1, colh}, c2 = {c1.x + w1 + gap, y, w2, colh};
    skin_well(c0);
    skin_well(c1);
    skin_well(c2);

    char parent[4096];
    snprintf(parent, sizeof parent, "%s", d->dir);
    char *slash = strrchr(parent, '/');
    bool has_parent = slash && strcmp(d->dir, "/") != 0;
    if (has_parent) {
        if (slash == parent)
            parent[1] = '\0';
        else
            *slash = '\0';
    }
    if (strcmp(g_parent_of, d->dir)) {
        snprintf(g_parent_of, sizeof g_parent_of, "%s", d->dir);
        snprintf(g_parent.ext, sizeof g_parent.ext, "%s", d->ext);
        if (has_parent)
            set_dir(&g_parent, parent);
        else
            filedlg_free(&g_parent);
        g_scroll_px = 0;
    }
    /* the parent's pick: the folder this is */
    int ppick = -1;
    const char *here = strrchr(d->dir, '/');
    for (int i = 0; here && i < g_parent.n_entries; i++)
        if (!strncmp(g_parent.entries[i], here + 1, strlen(here + 1)) &&
            g_parent.entries[i][strlen(here + 1)] == '/')
            ppick = i;
    static float pscroll;
    int pc = column(&g_parent, c0, ppick, &pscroll, true, false);
    int cc = column(d, c1, d->selected, &g_scroll_px, true, true);

    /* the keys: up and down pick, Enter opens, Backspace goes up */
    bool typing = ui_text_focused();
    if (d->n_entries && ui_key_pressed(SDLK_DOWN))
        d->selected = d->selected + 1 < d->n_entries ? d->selected + 1 : d->n_entries - 1;
    if (d->n_entries && ui_key_pressed(SDLK_UP))
        d->selected = d->selected > 0 ? d->selected - 1 : 0;
    int open_idx = -1;
    if (cc >= 0) {
        if (ui_double_clicked() && d->selected == cc)
            open_idx = cc;
        d->selected = cc;
    }
    /* a file picked is the name, in the box too if it is being typed in */
    if ((cc >= 0 || ui_key_pressed(SDLK_DOWN) || ui_key_pressed(SDLK_UP)) && d->selected >= 0) {
        const char *e = d->entries[d->selected];
        if (e[strlen(e) - 1] != '/') {
            snprintf(d->name, sizeof d->name, "%s", e);
            if (typing)
                ui_focus(ui_id("fd_name", 0), d->name);
        }
    }
    if (!typing && ui_key_pressed(SDLK_BACKSPACE) && has_parent)
        up_dir(d);
    if (!typing && ui_key_pressed(SDLK_RETURN) && d->selected >= 0)
        open_idx = d->selected;

    /* the preview */
    float px = c2.x + 14, py = c2.y + 12;
    if (d->selected >= 0 && d->selected < d->n_entries) {
        const char *e = d->entries[d->selected];
        size_t len = strlen(e);
        bool is_dir = e[len - 1] == '/';
        char path[4400];
        entry_path(d, e, path, sizeof path);
        char nm[300], fit[300];
        snprintf(nm, sizeof nm, "%.*s", (int)(is_dir ? len - 1 : len), e);
        skin_text(fo->bold, skin_fit(fo->bold, nm, c2.w - 28, fit, sizeof fit), px, py, TONE_TEXT, ALIGN_LEFT);
        py += skin_text_height(fo->bold) + 4;
        struct stat st;
        if (stat(path, &st) == 0) {
            char info[160], sz[32], when[48];
            human_size((long long)st.st_size, sz, sizeof sz);
            struct tm *tm = localtime(&st.st_mtime);
            if (!tm || !strftime(when, sizeof when, "%d %b %Y, %H:%M", tm))
                when[0] = '\0';
            const char *kind = is_dir ? "a folder" : has_ext(e, "bgw") ? "a Biogenesis world"
                             : has_ext(e, "sbw")                      ? "a SYN-BIOGENESIS world"
                             : has_ext(e, "bgg")                      ? "a being's genetic code"
                             : has_ext(e, "png")                      ? "an image"
                                                                     : "a file";
            if (is_dir)
                snprintf(info, sizeof info, "%s, changed %s", kind, when);
            else
                snprintf(info, sizeof info, "%s, %s, changed %s", kind, sz, when);
            skin_text(fo->small, skin_fit(fo->small, info, c2.w - 28, fit, sizeof fit), px, py, TONE_MUTED,
                      ALIGN_LEFT);
            py += skin_text_height(fo->small) + 12;
        }
        /* a being: drawn, as the laboratory draws it */
        if (!is_dir && has_ext(e, "bgg")) {
            if (strcmp(g_preview_path, path)) {
                snprintf(g_preview_path, sizeof g_preview_path, "%s", path);
                free(g_preview_text);
                g_preview_text = NULL;
                g_preview_len = 0;
                FILE *f = fopen(path, "rb");
                if (f) {
                    fseek(f, 0, SEEK_END);
                    long n = ftell(f);
                    fseek(f, 0, SEEK_SET);
                    if (n > 0 && n < 4 << 20 && (g_preview_text = malloc((size_t)n + 1))) {
                        g_preview_len = fread(g_preview_text, 1, (size_t)n, f);
                        g_preview_text[g_preview_len] = '\0';
                    }
                    fclose(f);
                }
            }
            char err[200] = "";
            GeneticCode *gc = g_preview_text ? bgg_parse(g_preview_text, g_preview_len, err, sizeof err) : NULL;
            float side = fminf(c2.w - 28, c2.y + c2.h - py - 12);
            if (gc && side > 40) {
                gc_panel(a, ui_id("fdprev", 0), (SDL_Rect){(int)px, (int)py, (int)side, (int)side}, gc);
            } else if (!gc) {
                skin_text(fo->small, err[0] ? err : "not a genetic code this simulation reads", px, py,
                          TONE_BAD, ALIGN_LEFT);
            }
#ifndef BIO_COLOUR
            if (gc)
                gc_unref(gc);
#endif
        }
    } else {
        skin_text(fo->small, d->save ? "Pick a folder, then name the file below." : "Pick a file.", px, py,
                  TONE_MUTED, ALIGN_LEFT);
    }

    /* the name, and the buttons */
    float ny = panel.y + panel.h - 16 - 34;
    float bw = 120;
    UIRect okb = {panel.x + panel.w - 16 - bw, ny, bw, 34};
    UIRect cancel = {okb.x - 8 - bw, ny, bw, 34};
    skin_text(fo->small_bold, d->save ? "NAME" : "FILE", panel.x + 16, ny + 9, TONE_MUTED, ALIGN_LEFT);
    SDL_Rect namebox = {(int)panel.x + 70, (int)ny, (int)(cancel.x - 16 - (panel.x + 70)), 34};
    /* a name to type when saving; when opening the keys browse */
    if (d->focus_name) {
        if (d->save)
            ui_focus(ui_id("fd_name", 0), d->name);
        d->focus_name = false;
    }
    if (ui_textfield(ui_id("fd_name", 0), namebox, d->name, sizeof d->name) && ui_key_pressed(SDLK_RETURN))
        accept(a);
    bool okh = d->name[0] && ui_hover(sdlr(okb));
    skin_button(okb, fo->bold, d->save ? "SAVE" : "OPEN", (WidgetState){.hover = okh, .active = d->name[0] != 0,
                                                                     .disabled = !d->name[0]}, false);
    if (okh && ui_pressed(sdlr(okb), 1))
        accept(a);
    bool ch = ui_hover(sdlr(cancel));
    skin_button(cancel, fo->bold, "CANCEL", (WidgetState){.hover = ch}, false);
    if ((ch && ui_pressed(sdlr(cancel), 1)) || (ui_key_pressed(SDLK_ESCAPE) && !typing))
        d->win.open = false;
    /* SYN-OS's TUIs' hints */
    static const char *const hints[] = {"Up/Down", "pick", "Enter", "opens", "Backspace", "up a folder",
                                        "Esc", "cancels"};
    float hx = panel.x + 16, hy = ny - 12 - skin_text_height(fo->mono_small);
    for (int i = 0; i < 8; i += 2) {
        float kw = skin_keycap_width(fo->mono_small, hints[i]);
        skin_keycap(ui_rect(hx, hy - 1, kw, 19), fo->mono_small, hints[i]);
        hx += kw + 6;
        hx += skin_text(fo->mono_small, hints[i + 1], hx, hy, TONE_MUTED, ALIGN_LEFT) + 16;
    }
    ui_layer_end();

    if (pc >= 0 && d->win.open) {
        const char *e = g_parent.entries[pc];
        size_t len = strlen(e);
        if (e[len - 1] == '/') {
            char p[4400];
            snprintf(p, sizeof p, "%s/%.*s", g_parent.dir, (int)(len - 1), e);
            set_dir(d, p);
        }
    }
    if (open_idx >= 0 && d->win.open) {
        const char *e = d->entries[open_idx];
        size_t len = strlen(e);
        if (e[len - 1] == '/') {
            char p[4400];
            snprintf(p, sizeof p, "%s/%.*s", d->dir, (int)(len - 1), e);
            set_dir(d, p);
            d->selected = -1;
        } else {
            snprintf(d->name, sizeof d->name, "%s", e);
            accept(a);
        }
    }
}
#endif

void filedlg_draw(App *a)
{
    FileDialog *d = &a->fdlg;
    if (!d->win.open)
        return;
#ifdef BIO_SYN_EDITION
    if (synos_active()) {
        syn_draw(a);
        return;
    }
#endif
    SDL_Rect c = ui_window_begin(&d->win, ui_id("filedlg", 0), d->save ? "Save" : "Open", true);
    int fh = gfx_font_h(), rh = fh + 10;
    int y = c.y;

    ui_label_dim(c.x, y + 5, d->save ? "Save in:" : "Look in:");
    SDL_Rect pathbox = {c.x + 70, y, c.w - 70 - 2 * 74, rh};
    gfx_fill(pathbox, C_FIELD);
    gfx_frame(pathbox, C_BORDER);
    gfx_clip_push(pathbox);
    int tw = gfx_text_w(d->dir);
    int tx = tw > pathbox.w - 10 ? pathbox.x + pathbox.w - 5 - tw : pathbox.x + 5;
    gfx_text(tx, y + 5, d->dir, C_TEXT);
    gfx_clip_pop();
    if (ui_button(ui_id("fd_up", 0), (SDL_Rect){c.x + c.w - 2 * 74 + 4, y, 70, rh}, "Up", true))
        up_dir(d);
    if (ui_button(ui_id("fd_home", 0), (SDL_Rect){c.x + c.w - 70, y, 70, rh}, "Home", true)) {
        const char *home = home_dir();
        if (home)
            set_dir(d, home);
    }
    y += rh + 8;

    int list_h = c.h - (y - c.y) - 3 * (rh + 8);
    SDL_Rect list = {c.x, y, c.w, list_h};
    gfx_fill(list, C_FIELD);
    gfx_frame(list, C_BORDER);
    int ih = fh + 6;
    int visible = (list.h - 4) / ih;
    if (ui_hover(list)) {
        d->scroll -= ui_wheel_y() * 3;
    }
    int maxs = d->n_entries - visible;
    if (d->scroll > maxs)
        d->scroll = maxs;
    if (d->scroll < 0)
        d->scroll = 0;
    gfx_clip_push((SDL_Rect){list.x + 1, list.y + 1, list.w - 2, list.h - 2});
    int open_idx = -1;
    for (int i = d->scroll; i < d->n_entries && i < d->scroll + visible + 1; i++) {
        SDL_Rect it = {list.x + 2, list.y + 2 + (i - d->scroll) * ih, list.w - 4, ih};
        if (i == d->selected)
            gfx_fill(it, C_ACCENT);
        else if (ui_hover(it))
            gfx_fill(it, C_HOVER);
        const char *e = d->entries[i];
        bool is_dir = e[strlen(e) - 1] == '/';
#ifdef BIO_SYNOS
        /* folders in the theme's accent, which reads on light themes too */
        if (synos_active())
            gfx_text(it.x + 6, it.y + 3, e, is_dir ? C_ACCENT : C_TEXT);
        else
#endif
        gfx_text(it.x + 6, it.y + 3, e, is_dir ? COL(150, 190, 255) : C_TEXT);
        if (ui_pressed(it, 1)) {
            if (ui_double_clicked() && d->selected == i)
                open_idx = i;
            d->selected = i;
            if (!is_dir)
                snprintf(d->name, sizeof d->name, "%s", e);
        }
    }
    gfx_clip_pop();
    if (d->n_entries == 0)
        ui_label_dim(list.x + 10, list.y + 8, "(nothing here)");
    y += list_h + 8;

    ui_label_dim(c.x, y + 5, "File name:");
    if (d->focus_name) {
        ui_focus(ui_id("fd_name", 0), d->name);
        d->focus_name = false;
    }
    if (ui_textfield(ui_id("fd_name", 0), (SDL_Rect){c.x + 100, y, c.w - 100, rh}, d->name,
                     sizeof d->name) &&
        ui_key_pressed(SDLK_RETURN))
        accept(a);
    y += rh + 8;
    char filt[64] = "";
    for (const char *e = d->ext; *e;) {
        size_t n = strcspn(e, "|");
        snprintf(filt + strlen(filt), sizeof filt - strlen(filt), "%s*.%.*s",
                 filt[0] ? ", " : "", (int)n, e);
        e += n + (e[n] == '|');
    }
    ui_label_dim(c.x, y + 5, "Files of type:");
    ui_label(c.x + 100, y + 5, filt);

    int by = c.y + c.h - rh;
    if (ui_button(ui_id("fd_ok", 0), (SDL_Rect){c.x + c.w - 2 * 100 - 10, by, 100, rh},
                  d->save ? "Save" : "Open", d->name[0] != '\0'))
        accept(a);
    if (ui_button(ui_id("fd_cancel", 0), (SDL_Rect){c.x + c.w - 100, by, 100, rh}, "Cancel",
                  true) ||
        (ui_key_pressed(SDLK_ESCAPE) && !ui_text_focused()))
        d->win.open = false;
    ui_window_end();

    if (open_idx >= 0 && d->win.open) {
        const char *e = d->entries[open_idx];
        size_t len = strlen(e);
        if (e[len - 1] == '/') {
            char p[4400];
            snprintf(p, sizeof p, "%s/%.*s", d->dir, (int)(len - 1), e);
            set_dir(d, p);
        } else {
            snprintf(d->name, sizeof d->name, "%s", e);
            accept(a);
        }
    }
}
