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

#include "ui/app.h"
#ifdef BIO_SYNOS
#include "synos/synos.h"
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
        const char *home = getenv("HOME");
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
    if (d->name[0] == '/')
        snprintf(path, sizeof path, "%s", d->name);
    else
        snprintf(path, sizeof path, "%s%s%s", d->dir,
                 d->dir[strlen(d->dir) - 1] == '/' ? "" : "/", d->name);
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

void filedlg_draw(App *a)
{
    FileDialog *d = &a->fdlg;
    if (!d->win.open)
        return;
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
        const char *home = getenv("HOME");
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
