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

#include "syn/shell.h"

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "synos/synos.h"
#include "ui/ui.h"

#define NOTES 6
#define LOG_LINES 600
#define LOG_LEN 240
#define HISTORY 48
#define MENU_ITEMS 32

static struct {
    /* notifications, newest last */
    struct {
        char text[200];
        bool urgent;
        Uint32 at;
    } note[NOTES];
    int nnote;

    bool launcher;
    char query[64];
    int qcur;
    int sel; /* the row picked, in the rows shown */
    int first;

    bool console;
    Uint32 console_at; /* when it opened or closed, for the slide */
    char line[256];
    int lcur;
    char log[LOG_LINES][LOG_LEN];
    unsigned char kind[LOG_LINES];
    int nlog, head; /* a ring: the oldest at head once full */
    int scroll;     /* rows up from the newest */
    char hist[HISTORY][256];
    int nhist, hpos;
    char entered[256];
    float console_h;

    bool menu;
    int mx, my;
    char title[64];
    ShellCommand items[MENU_ITEMS];
    int nitems;
} SH;

static SDL_Rect sdl(UIRect r)
{
    return (SDL_Rect){(int)floorf(r.x), (int)floorf(r.y), (int)ceilf(r.w), (int)ceilf(r.h)};
}

static bool over(UIRect r)
{
    return ui_hover(sdl(r));
}

static bool clicked(UIRect r)
{
    return ui_pressed(sdl(r), 1);
}

static float text_w(TTF_Font *f, const char *s)
{
    return s && s[0] ? skin_text_width(f, s) : 0;
}

/* text of one colour, its top at y */
static float ink(TTF_Font *f, const char *s, float x, float y, Rgb c, Align al)
{
    return skin_text_rgb(f, s, x, y, c.r, c.g, c.b, al);
}

/* text centred on the line cy */
static float mid(TTF_Font *f, const char *s, float x, float cy, Rgb c, Align al)
{
    return ink(f, s, x, cy - skin_text_height(f) * 0.5f, c, al);
}

/* ---- the bar ---- */

int syn_shell_bar(int w, const char *name, const ShellTab *tabs, int ntabs,
                  const ShellModule *mods, int nmods, int menu_id)
{
    const Fonts *fo = synos_fonts();
    int got = 0;
    UIRect bar = {0, 0, (float)w, SHELL_BAR_H};
    ui_layer_begin(ui_id("shellbar", 0), sdl(bar), false);
    skin_bar(bar);
    const float pad = 6, bh = SHELL_BAR_H - 2 * pad, cy = SHELL_BAR_H * 0.5f - 1;
    const float iw = 18; /* an icon's column */

    /* what each piece takes: tabs with their labels, or only the one
     * shown, then the name, then modules from the last */
    float tab_full[16], tab_icon[16];
    for (int i = 0; i < ntabs && i < 16; i++) {
        tab_icon[i] = iw + 24;
        tab_full[i] = iw + 8 + text_w(fo->bold, tabs[i].label) + 26;
    }
    float mod_w[16];
    for (int i = 0; i < nmods && i < 16; i++)
        mod_w[i] = mods[i].custom ? mods[i].custom(0, 0, false) + 20
                                  : (mods[i].icon ? iw + (mods[i].text[0] ? 6 : 0) : 0) +
                                        text_w(fo->mono_small_bold, mods[i].text) + 20;
    float menu_w = text_w(fo->bold, "MENU") + 36;
    float mark_w = 30 + text_w(fo->heading, name) + 14;
    /* as the window narrows: only the screen shown keeps its name, then
     * figures go from the last down to the first four, then the name by the
     * glyph, then every tab is its icon, then the rest of the figures */
    int level = 0, shown = nmods < 16 ? nmods : 16;
    bool show_name = true;
    for (;;) {
        float left = 10 + (show_name ? mark_w : 34) + 12;
        for (int i = 0; i < ntabs && i < 16; i++)
            left += (level == 0 || (level == 1 && tabs[i].active) ? tab_full[i] : tab_icon[i]) + 4;
        float right = 10 + menu_w + 8;
        for (int i = 0; i < shown; i++)
            right += mod_w[i] + 4;
        if (left + right <= (float)w)
            break;
        if (level == 0)
            level = 1;
        else if (shown > 4)
            shown--;
        else if (show_name)
            show_name = false;
        else if (level == 1)
            level = 2;
        else if (shown > 0)
            shown--;
        else
            break;
    }
    /* and back in, the figures there is room for after all */
    for (;;) {
        float left = 10 + (show_name ? mark_w : 34) + 12, right = 10 + menu_w + 8;
        for (int i = 0; i < ntabs && i < 16; i++)
            left += (level == 0 || (level == 1 && tabs[i].active) ? tab_full[i] : tab_icon[i]) + 4;
        for (int i = 0; i <= shown && i < nmods && i < 16; i++)
            right += mod_w[i] + 4;
        if (shown >= nmods || shown >= 16 || left + right > (float)w)
            break;
        shown++;
    }

    /* the glyph, and the name beside it: the launcher */
    float x = 10;
    UIRect mark = {x, pad, show_name ? mark_w : 34, bh};
    bool mh = over(mark);
    if (mh)
        skin_fill(mark, skin_theme()->panel_hover, 0.6f);
    skin_glyph(x + 15, cy + 1, 17, fo->icon);
    if (show_name)
        mid(fo->heading, name, x + 30, cy, skin_tone(TONE_ACCENT), ALIGN_LEFT);
    if (mh) {
        ui_tooltip("Every command   ( / )");
        if (clicked(mark))
            syn_shell_launcher_open();
    }
    x += mark.w + 12;

    for (int i = 0; i < ntabs && i < 16; i++) {
        bool full = level == 0 || (level == 1 && tabs[i].active);
        UIRect r = {x, pad, full ? tab_full[i] : tab_icon[i], bh};
        bool h = !tabs[i].disabled && over(r);
        Tone t = skin_tab_chrome(r, (WidgetState){.hover = h, .active = tabs[i].active});
        Rgb c = skin_tone(tabs[i].disabled ? TONE_MUTED : t);
        float tx = r.x + 13;
        if (tabs[i].icon)
            mid(fo->icon, tabs[i].icon, tx + iw * 0.5f, cy, c, ALIGN_CENTER);
        if (full)
            mid(fo->bold, tabs[i].label, tx + iw + 8, cy, c, ALIGN_LEFT);
        if (h) {
            if (!full)
                ui_tooltip(tabs[i].label);
            if (clicked(r))
                got = tabs[i].id;
        }
        x += r.w + 4;
    }

    /* MENU at the right, the modules before it */
    float rx = (float)w - 10 - menu_w;
    UIRect mb = {rx, pad, menu_w, bh};
    bool hb = over(mb);
    Tone mt = skin_button_chrome(mb, (WidgetState){.hover = hb}, false);
    mid(fo->bold, "MENU", mb.x + mb.w * 0.5f, cy, skin_tone(mt), ALIGN_CENTER);
    if (hb) {
        ui_tooltip("The main menu   ( Esc )");
        if (clicked(mb))
            got = menu_id;
    }
    rx -= 8;
    for (int i = 0; i < shown; i++) {
        UIRect r = {rx - mod_w[i], pad, mod_w[i], bh};
        bool h = over(r);
        Tone t = skin_module(r, (WidgetState){.hover = h && mods[i].id}, mods[i].level);
        if (mods[i].custom) {
            mods[i].custom(r.x + r.w - 11, cy, true);
        } else {
            float tx = r.x + 10;
            if (mods[i].icon) {
                Rgb ic = skin_tone(mods[i].level || t == TONE_ON_ACCENT ? t : TONE_ACCENT);
                mid(fo->icon, mods[i].icon, tx + iw * 0.5f, cy, ic, ALIGN_CENTER);
                tx += iw + 6;
            }
            mid(fo->mono_small_bold, mods[i].text, tx, cy, skin_tone(t), ALIGN_LEFT);
        }
        if (h) {
            if (mods[i].tip)
                ui_tooltip(mods[i].tip);
            if (mods[i].id && clicked(r))
                got = mods[i].id;
        }
        rx = r.x - 4;
    }
    ui_layer_end();
    return got;
}

/* ---- the hints ---- */

void syn_shell_hints(int w, int h, const char *const *keys, int nkeys, const char *right)
{
    const Fonts *fo = synos_fonts();
    const Theme *t = skin_theme();
    UIRect r = {0, (float)(h - SHELL_HINTS_H), (float)w, SHELL_HINTS_H};
    if (t->style == STYLE_WIN95) {
        skin_fill(r, t->panel, 1);
    } else if (t->style != STYLE_FLATLINE) {
        skin_fill(r, t->bg, 0.86f);
    }
    skin_separator(0, r.y, (float)w);
    float cy = r.y + r.h * 0.5f, x = 12;
    float right_w = right ? text_w(fo->mono_small, right) + 24 : 0;
    for (int i = 0; i + 1 < nkeys * 2 && i / 2 < nkeys; i += 2) {
        float kw = skin_keycap_width(fo->mono_small, keys[i]);
        float vw = text_w(fo->mono_small, keys[i + 1]);
        if (x + kw + 6 + vw > (float)w - right_w)
            break;
        skin_keycap(ui_rect(x, cy - 9, kw, 19), fo->mono_small, keys[i]);
        x += kw + 6;
        mid(fo->mono_small, keys[i + 1], x, cy, skin_tone(TONE_MUTED), ALIGN_LEFT);
        x += vw + 16;
    }
    if (right && x < (float)w - right_w)
        mid(fo->mono_small, right, (float)w - 12, cy, skin_tone(TONE_MUTED), ALIGN_RIGHT);
}

/* ---- notifications ---- */

void syn_shell_notify(const char *text, bool urgent)
{
    if (!text || !text[0])
        return;
    if (SH.nnote == NOTES) {
        memmove(SH.note, SH.note + 1, sizeof SH.note[0] * (NOTES - 1));
        SH.nnote--;
    }
    snprintf(SH.note[SH.nnote].text, sizeof SH.note[0].text, "%s", text);
    SH.note[SH.nnote].urgent = urgent;
    SH.note[SH.nnote].at = SDL_GetTicks();
    SH.nnote++;
    syn_shell_print(urgent ? SHELL_ERROR : SHELL_NOTE, "%s", text);
}

void syn_shell_notify_clear(void)
{
    SH.nnote = 0;
}

void syn_shell_notifications(int w, int top)
{
    const Fonts *fo = synos_fonts();
    Uint32 now = SDL_GetTicks();
    /* mako's: five seconds, an urgent one until it is clicked away */
    for (int i = 0; i < SH.nnote;) {
        if (!SH.note[i].urgent && now - SH.note[i].at > 5000) {
            memmove(SH.note + i, SH.note + i + 1, sizeof SH.note[0] * (size_t)(SH.nnote - i - 1));
            SH.nnote--;
        } else {
            i++;
        }
    }
    /* below the console while it is down */
    float y = (float)top + 8 + (SH.console || SDL_GetTicks() - SH.console_at < 160 ? SH.console_h : 0);
    int shown = 0;
    for (int i = SH.nnote - 1; i >= 0 && shown < 3; i--, shown++) {
        const char *s = SH.note[i].text;
        float tw = fminf(text_w(fo->mono_small, s), (float)w * 0.5f);
        UIRect r = {(float)w - 8 - tw - 32, y, tw + 32, skin_text_height(fo->mono_small) + 26};
        Uint32 age = now - SH.note[i].at;
        float a = !SH.note[i].urgent && age > 4600 ? 1.0f - (float)(age - 4600) / 400.0f : 1.0f;
        if (age < 120)
            a *= (float)age / 120.0f;
        skin_set_alpha(a);
        SDL_Rect sr = sdl(r);
        ui_layer_begin(ui_id("shellnote", (uint32_t)i), sr, false);
        skin_note(r, SH.note[i].urgent);
        char fit[200];
        ink(fo->mono_small, skin_fit(fo->mono_small, s, tw, fit, sizeof fit), r.x + 16, r.y + 13,
            skin_tone(TONE_TEXT), ALIGN_LEFT);
        bool gone = ui_pressed(sr, 1);
        ui_layer_end();
        skin_set_alpha(1);
        if (gone) {
            memmove(SH.note + i, SH.note + i + 1, sizeof SH.note[0] * (size_t)(SH.nnote - i - 1));
            SH.nnote--;
            return;
        }
        y += r.h + 8;
    }
}

/* ---- the launcher ---- */

void syn_shell_launcher_open(void)
{
    SH.launcher = true;
    SH.query[0] = '\0';
    SH.qcur = 0;
    SH.sel = 0;
    SH.first = 0;
    syn_shell_menu_close();
}

bool syn_shell_launcher_showing(void)
{
    return SH.launcher;
}

static bool contains(const char *hay, const char *needle)
{
    size_t n = strlen(needle);
    for (; *hay; hay++)
        if (!strncasecmp(hay, needle, n))
            return true;
    return n == 0;
}

/* how well c answers the query: lower is better, -1 not at all */
static int score(const ShellCommand *c, const char *query)
{
    char q[64];
    snprintf(q, sizeof q, "%s", query);
    char *tok[8];
    int nt = syn_shell_split(q, tok, 8);
    if (!nt)
        return 0;
    int best = 0;
    for (int i = 0; i < nt; i++) {
        bool in_label = contains(c->label, tok[i]), in_word = c->word && contains(c->word, tok[i]);
        bool in_section = c->section && contains(c->section, tok[i]);
        if (!in_label && !in_word && !in_section)
            return -1;
        int s = 3;
        size_t n = strlen(tok[i]);
        if (!strncasecmp(c->label, tok[i], n) || (c->word && !strcasecmp(c->word, tok[i])))
            s = 0;
        else if (c->word && !strncasecmp(c->word, tok[i], n))
            s = 1;
        else if (in_label)
            for (const char *p = c->label; *p; p++)
                if ((p == c->label || p[-1] == ' ') && !strncasecmp(p, tok[i], n)) {
                    s = 2;
                    break;
                }
        best += s;
    }
    return best;
}

typedef struct {
    int cmd;           /* -1: a heading */
    const char *title; /* the heading's */
    int score;
} Row;

static int by_score(const void *a, const void *b)
{
    const Row *x = a, *y = b;
    return x->score != y->score ? x->score - y->score : x->cmd - y->cmd;
}

int syn_shell_launcher(int w, int h, const ShellCommand *c, int n)
{
    if (!SH.launcher)
        return 0;
    const Fonts *fo = synos_fonts();
    const Theme *t = skin_theme();
    int got = 0;

    /* the rows: under their headings while nothing is typed, best first after */
    static Row rows[400];
    int nr = 0;
    if (!SH.query[0]) {
        const char *last = NULL;
        for (int i = 0; i < n && nr < 398; i++) {
            if (!c[i].label)
                continue;
            if (c[i].section && (!last || strcmp(last, c[i].section))) {
                rows[nr++] = (Row){-1, c[i].section, 0};
                last = c[i].section;
            }
            rows[nr++] = (Row){i, NULL, 0};
        }
    } else {
        for (int i = 0; i < n && nr < 398; i++) {
            int s = c[i].label ? score(&c[i], SH.query) : -1;
            if (s >= 0)
                rows[nr++] = (Row){i, NULL, s};
        }
        qsort(rows, (size_t)nr, sizeof rows[0], by_score);
    }

    float cw = fminf(600, (float)w - 32);
    float row_h = skin_text_height(fo->mono) + 10, head_h = skin_text_height(fo->mono_small_bold) + 12;
    float list_max = (float)h * 0.62f;
    int visible = (int)(list_max / row_h);
    if (visible < 4)
        visible = 4;
    UIRect card = {((float)w - cw) / 2, fmaxf(SHELL_BAR_H + 12, (float)h * 0.12f), cw, 0};

    /* keys: they move the pick over the rows that can be picked */
    uint32_t id = ui_id("shellquery", 0);
    UiLineKey k = ui_line_edit(id, SH.query, sizeof SH.query, &SH.qcur);
    static char last_query[64];
    if (strcmp(last_query, SH.query)) {
        snprintf(last_query, sizeof last_query, "%s", SH.query);
        SH.sel = 0;
        SH.first = 0;
    }
    int pickable[400], np = 0;
    for (int i = 0; i < nr; i++)
        if (rows[i].cmd >= 0 && !c[rows[i].cmd].disabled)
            pickable[np++] = i;
    if (SH.sel >= np)
        SH.sel = np ? np - 1 : 0;
    if (k == UI_LINE_DOWN && np)
        SH.sel = (SH.sel + 1) % np;
    if (k == UI_LINE_UP && np)
        SH.sel = (SH.sel + np - 1) % np;
    if (k == UI_LINE_PAGE_DOWN && np)
        SH.sel = SH.sel + visible / 2 < np ? SH.sel + visible / 2 : np - 1;
    if (k == UI_LINE_PAGE_UP && np)
        SH.sel = SH.sel > visible / 2 ? SH.sel - visible / 2 : 0;
    if (k == UI_LINE_ESCAPE) {
        SH.launcher = false;
        ui_unfocus();
        return 0;
    }
    if (k == UI_LINE_ENTER && np) {
        SH.launcher = false;
        ui_unfocus();
        return c[rows[pickable[SH.sel]].cmd].id;
    }
    int sel_row = np ? pickable[SH.sel] : -1;
    /* the list's window follows the pick */
    float hh = 0;
    for (int i = SH.first; i < nr && i <= sel_row; i++)
        hh += rows[i].cmd < 0 ? head_h : row_h;
    if (sel_row >= 0 && sel_row < SH.first)
        SH.first = sel_row > 0 && rows[sel_row - 1].cmd < 0 ? sel_row - 1 : sel_row;
    while (sel_row >= 0 && hh > list_max && SH.first < sel_row) {
        hh -= rows[SH.first].cmd < 0 ? head_h : row_h;
        SH.first++;
    }

    float input_h = skin_text_height(fo->mono) + 22;
    float lh = 0;
    int last_row = SH.first;
    for (; last_row < nr; last_row++) {
        float rh = rows[last_row].cmd < 0 ? head_h : row_h;
        if (lh + rh > list_max)
            break;
        lh += rh;
    }
    card.h = input_h + (nr ? lh + 12 : row_h + 12) + 34;

    skin_fill(ui_rect(0, 0, (float)w, (float)h), t->bg, 0.45f);
    SDL_Rect all = {0, 0, w, h};
    ui_layer_begin(ui_id("shelllauncher", 0), all, true);
    /* rofi's card: the panel inside a 3px border in the accent */
    skin_fill(card, t->panel, 0.98f);
    skin_note(card, false);
    /* the line typed, over its 2px rule */
    float ix = card.x + 18, iy = card.y + 11;
    skin_glyph(ix + 8, iy + skin_text_height(fo->mono) * 0.5f, 14, fo->icon);
    float qx = ix + 26;
    if (SH.query[0]) {
        ink(fo->mono, SH.query, qx, iy, t->text, ALIGN_LEFT);
    } else {
        ink(fo->mono, "type to find a command", qx, iy, skin_tone(TONE_MUTED), ALIGN_LEFT);
    }
    char before[64];
    snprintf(before, sizeof before, "%.*s", SH.qcur, SH.query);
    if ((SDL_GetTicks() / 530) % 2 == 0)
        skin_fill(ui_rect(qx + text_w(fo->mono, before), iy, 8, skin_text_height(fo->mono)),
                  skin_tone(TONE_ACCENT), 0.85f);
    skin_fill(ui_rect(card.x + 3, card.y + input_h, card.w - 6, 2), t->accent, 1);

    float y = card.y + input_h + 8;
    int mx, my;
    ui_mouse(&mx, &my);
    static int last_mx = -1, last_my = -1;
    bool moved = mx != last_mx || my != last_my;
    last_mx = mx;
    last_my = my;
    float wheel = ui_wheel_fy();
    if (wheel != 0 && np) {
        SH.sel -= (int)wheel;
        SH.sel = SH.sel < 0 ? 0 : SH.sel >= np ? np - 1 : SH.sel;
    }
    if (!nr)
        mid(fo->mono, "nothing called that", card.x + 18, y + row_h * 0.5f, skin_tone(TONE_MUTED),
            ALIGN_LEFT);
    for (int i = SH.first; i < last_row; i++) {
        if (rows[i].cmd < 0) {
            /* the root menu's headings: bold, upper case, in the accent */
            mid(fo->mono_small_bold, rows[i].title, card.x + 18, y + head_h * 0.55f,
                skin_tone(TONE_ACCENT), ALIGN_LEFT);
            y += head_h;
            continue;
        }
        const ShellCommand *cm = &c[rows[i].cmd];
        UIRect r = {card.x + 6, y, card.w - 12, row_h};
        bool h = !cm->disabled && over(r);
        int pi = -1;
        for (int j = 0; j < np; j++)
            if (pickable[j] == i)
                pi = j;
        /* the pointer picks only when it moves, so the keys keep their pick */
        if (h && moved && pi >= 0)
            SH.sel = pi;
        bool picked = i == sel_row;
        Rgb fg = cm->disabled ? skin_tone(TONE_MUTED) : t->text;
        if (picked) {
            /* SYN-OS's menus: the pick filled in the accent */
            skin_fill(r, t->accent, 1);
            fg = skin_tone(TONE_ON_ACCENT);
        }
        float cy = y + row_h * 0.5f;
        if (cm->icon)
            mid(fo->icon, cm->icon, r.x + 22, cy, picked ? fg : skin_tone(TONE_MUTED), ALIGN_CENTER);
        mid(fo->mono, cm->label, r.x + 42, cy, fg, ALIGN_LEFT);
        float kx = r.x + r.w - 10;
        if (cm->key) {
            float kw = skin_keycap_width(fo->mono_small, cm->key);
            skin_keycap(ui_rect(kx - kw, cy - 9, kw, 19), fo->mono_small, cm->key);
            kx -= kw + 10;
        }
        if (cm->word) {
            char wd[40];
            snprintf(wd, sizeof wd, ":%s", cm->word);
            kx -= mid(fo->mono_small, wd, kx, cy, picked ? fg : skin_tone(TONE_MUTED), ALIGN_RIGHT) + 10;
        }
        if (cm->on)
            kx -= mid(fo->icon, ICON_CHECK, kx, cy, picked ? fg : skin_tone(TONE_GOOD), ALIGN_RIGHT) + 10;
        /* found by a search: where it lives */
        if (SH.query[0] && cm->section)
            mid(fo->mono_small, cm->section, kx, cy, picked ? fg : skin_tone(TONE_MUTED), ALIGN_RIGHT);
        if (h && clicked(r)) {
            got = cm->id;
            SH.launcher = false;
            ui_unfocus();
        }
        y += row_h;
    }
    /* the hints, wmenu's way */
    float by = card.y + card.h - 22;
    char foot[120];
    snprintf(foot, sizeof foot, "%d of %d   up/down pick   Enter runs   Esc closes", np, n);
    mid(fo->mono_small, foot, card.x + 18, by, skin_tone(TONE_MUTED), ALIGN_LEFT);
    /* a click off the card closes it */
    if (ui_pressed(all, 1) && !skin_hit(card, mx, my)) {
        SH.launcher = false;
        ui_unfocus();
    }
    ui_layer_end();
    return got;
}

/* ---- the console ---- */

void syn_shell_console_toggle(void)
{
    SH.console = !SH.console;
    SH.console_at = SDL_GetTicks();
    SH.scroll = 0;
    if (!SH.console)
        ui_unfocus();
}

bool syn_shell_console_showing(void)
{
    return SH.console;
}

static void log_line(ShellKind kind, const char *s)
{
    int at;
    if (SH.nlog < LOG_LINES) {
        at = (SH.head + SH.nlog) % LOG_LINES;
        SH.nlog++;
    } else {
        at = SH.head;
        SH.head = (SH.head + 1) % LOG_LINES;
    }
    size_t n = strlen(s);
    if (n > LOG_LEN - 1)
        n = LOG_LEN - 1;
    memcpy(SH.log[at], s, n);
    SH.log[at][n] = '\0';
    SH.kind[at] = (unsigned char)kind;
}

void syn_shell_print(ShellKind kind, const char *fmt, ...)
{
    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    /* a line a row of the log; long ones are wrapped as they are drawn */
    char *p = buf;
    for (;;) {
        char *nl = strchr(p, '\n');
        if (nl)
            *nl = '\0';
        /* longer than a row of the log holds: in pieces, not mid-character */
        while (strlen(p) > LOG_LEN - 1) {
            size_t cut = LOG_LEN - 1;
            while (cut > 0 && ((unsigned char)p[cut] & 0xC0) == 0x80)
                cut--;
            char keep = p[cut];
            p[cut] = '\0';
            log_line(kind, p);
            p[cut] = keep;
            p += cut;
        }
        log_line(kind, p);
        if (!nl)
            break;
        p = nl + 1;
    }
}

void syn_shell_clear(void)
{
    SH.nlog = 0;
    SH.head = 0;
    SH.scroll = 0;
}

int syn_shell_split(char *line, char **argv, int max)
{
    int n = 0;
    char *p = line;
    while (*p && n < max) {
        while (*p == ' ' || *p == '\t')
            p++;
        if (!*p)
            break;
        if (*p == '"') {
            argv[n++] = ++p;
            while (*p && *p != '"')
                p++;
        } else {
            argv[n++] = p;
            while (*p && *p != ' ' && *p != '\t')
                p++;
        }
        if (*p)
            *p++ = '\0';
    }
    return n;
}

const ShellCommand *syn_shell_lookup(const ShellCommand *c, int n, const char *word)
{
    for (int i = 0; i < n; i++)
        if (c[i].word && !strcasecmp(c[i].word, word))
            return &c[i];
    return NULL;
}

void syn_shell_list(const ShellCommand *c, int n)
{
    const char *last = NULL;
    for (int i = 0; i < n; i++) {
        if (!c[i].word)
            continue;
        if (c[i].section && (!last || strcmp(last, c[i].section))) {
            syn_shell_print(SHELL_HEAD, "%s", c[i].section);
            last = c[i].section;
        }
        syn_shell_print(c[i].disabled ? SHELL_NOTE : SHELL_OUT, "  %-14s %s%s%s%s", c[i].word, c[i].label,
                        c[i].key ? "  (" : "", c[i].key ? c[i].key : "", c[i].key ? ")" : "");
    }
}

static Rgb kind_colour(int kind)
{
    switch (kind) {
    case SHELL_ECHO: return skin_tone(TONE_ACCENT);
    case SHELL_ERROR: return skin_tone(TONE_BAD);
    case SHELL_NOTE: return skin_tone(TONE_MUTED);
    case SHELL_HEAD: return skin_tone(TONE_ACCENT);
    case SHELL_GOOD: return skin_tone(TONE_GOOD);
    default: return skin_theme()->text;
    }
}

/* the words starting with what is typed: one is put in, several listed */
static void complete(const char *const *words, int nwords)
{
    if (strchr(SH.line, ' '))
        return;
    const char *only = NULL;
    int count = 0;
    size_t n = strlen(SH.line), common = 0;
    for (int i = 0; i < nwords; i++) {
        if (strncasecmp(words[i], SH.line, n))
            continue;
        if (!only) {
            only = words[i];
            common = strlen(words[i]);
        } else {
            size_t k = 0;
            while (k < common && tolower((unsigned char)only[k]) == tolower((unsigned char)words[i][k]))
                k++;
            common = k;
        }
        count++;
    }
    if (count == 1) {
        snprintf(SH.line, sizeof SH.line, "%s ", only);
    } else if (count > 1) {
        if (common > n) {
            snprintf(SH.line, sizeof SH.line, "%.*s", (int)common, only);
        } else {
            char list[1600] = "";
            for (int i = 0; i < nwords; i++)
                if (!strncasecmp(words[i], SH.line, n) && strlen(list) + strlen(words[i]) + 3 < sizeof list) {
                    strcat(list, words[i]);
                    strcat(list, "  ");
                }
            syn_shell_print(SHELL_NOTE, "%s", list);
        }
    }
    SH.lcur = (int)strlen(SH.line);
}

const char *syn_shell_console(int w, int top, int h, const char *const *words, int nwords)
{
    const Fonts *fo = synos_fonts();
    const Theme *t = skin_theme();
    Uint32 since = SDL_GetTicks() - SH.console_at;
    float slide = since >= 160 ? 1.0f : (float)since / 160.0f;
    if (!SH.console) {
        if (since >= 160)
            return NULL;
        slide = 1.0f - slide; /* going away */
    }
    float full = fmaxf(240, (float)(h - top) * 0.46f);
    float ch = full * (1.0f - (1.0f - slide) * (1.0f - slide));
    SH.console_h = ch;
    UIRect r = {0, (float)top, (float)w, ch};
    ui_layer_begin(ui_id("shellconsole", 0), sdl(r), false);
    /* foot's: the background a little see-through, Terminus, 8x6 padding */
    skin_fill(r, t->bg, 0.9f);
    skin_fill(ui_rect(0, r.y + r.h - 2, (float)w, 2), t->accent, 1);
    const char *ret = NULL;
    TTF_Font *f = fo->mono;
    float lh = skin_text_height(f) + 1, cw = text_w(f, "M");
    if (cw <= 0)
        cw = 8;
    int cols = (int)(((float)w - 16) / cw);
    if (cols < 20)
        cols = 20;

    if (SH.console) {
        uint32_t id = ui_id("shellline", 0);
        UiLineKey k = UI_LINE_NONE;
        if (ui_key_pressed(SDLK_BACKQUOTE)) {
            syn_shell_console_toggle();
        } else {
            k = ui_line_edit(id, SH.line, sizeof SH.line, &SH.lcur);
        }
        int page = (int)(ch / lh) - 3;
        if (k == UI_LINE_ESCAPE) {
            syn_shell_console_toggle();
        } else if (k == UI_LINE_ENTER) {
            snprintf(SH.entered, sizeof SH.entered, "%s", SH.line);
            syn_shell_print(SHELL_ECHO, ": %s", SH.line);
            if (SH.line[0] && (!SH.nhist || strcmp(SH.hist[SH.nhist - 1], SH.line))) {
                if (SH.nhist == HISTORY) {
                    memmove(SH.hist, SH.hist + 1, sizeof SH.hist[0] * (HISTORY - 1));
                    SH.nhist--;
                }
                snprintf(SH.hist[SH.nhist++], sizeof SH.hist[0], "%s", SH.line);
            }
            SH.hpos = SH.nhist;
            SH.line[0] = '\0';
            SH.lcur = 0;
            SH.scroll = 0;
            ret = SH.entered;
        } else if (k == UI_LINE_TAB) {
            complete(words, nwords);
        } else if (k == UI_LINE_UP && SH.hpos > 0) {
            snprintf(SH.line, sizeof SH.line, "%s", SH.hist[--SH.hpos]);
            SH.lcur = (int)strlen(SH.line);
        } else if (k == UI_LINE_DOWN && SH.hpos < SH.nhist) {
            SH.hpos++;
            snprintf(SH.line, sizeof SH.line, "%s", SH.hpos < SH.nhist ? SH.hist[SH.hpos] : "");
            SH.lcur = (int)strlen(SH.line);
        } else if (k == UI_LINE_PAGE_UP) {
            SH.scroll += page;
        } else if (k == UI_LINE_PAGE_DOWN) {
            SH.scroll = SH.scroll > page ? SH.scroll - page : 0;
        }
        if (ui_key_pressed(SDLK_l) && (ui_mods() & KMOD_CTRL))
            syn_shell_clear();
        float wy = ui_wheel_fy();
        if (wy != 0 && over(r)) {
            SH.scroll += (int)lroundf(wy * 3);
            if (SH.scroll < 0)
                SH.scroll = 0;
        }
    }

    /* the line being typed, at the bottom; the log above it, newest last */
    float y = r.y + r.h - 8 - lh;
    float px = 8;
    px += ink(f, ":", px, y, skin_tone(TONE_ACCENT), ALIGN_LEFT) + cw;
    ink(f, SH.line, px, y, t->text, ALIGN_LEFT);
    char before[256];
    snprintf(before, sizeof before, "%.*s", SH.lcur, SH.line);
    if (SH.console && (SDL_GetTicks() / 530) % 2 == 0)
        skin_fill(ui_rect(px + text_w(f, before), y, cw, lh - 1), skin_tone(TONE_ACCENT), 0.8f);
    /* its title bar, as labwc draws a window's */
    float tbh = skin_text_height(fo->mono_small_bold) + 8;
    UIRect tb = {0, r.y, (float)w, tbh};
    if (r.h > tbh) {
        Rgb tc = skin_titlebar(tb);
        float tcy = tb.y + tbh * 0.5f;
        mid(fo->mono_small_bold, "CONSOLE", 10, tcy, tc, ALIGN_LEFT);
        /* its keys, right to left, as the hints along the bottom show them */
        static const char *const keys[] = {"help", "lists", "PgUp", "scrolls", "Up", "history",
                                           "Tab", "completes", "`", "closes"};
        float kx = (float)w - 10;
        for (int i = 8; i >= 0; i -= 2) {
            kx -= mid(fo->mono_small, keys[i + 1], kx, tcy, tc, ALIGN_RIGHT) + 6;
            float kw = skin_keycap_width(fo->mono_small, keys[i]);
            skin_keycap(ui_rect(kx - kw, tcy - 9, kw, 18), fo->mono_small, keys[i]);
            kx -= kw + 16;
            if (kx < 140)
                break;
        }
    }

    /* rows of the log, wrapped to the width, counted from the newest */
    y -= lh + 4;
    int skip = SH.scroll;
    int rows_fit = (int)((y - r.y - tbh - 4) / lh) + 1;
    int drawn = 0;
    for (int i = SH.nlog - 1; i >= 0 && drawn < rows_fit; i--) {
        int at = (SH.head + i) % LOG_LINES;
        const char *s = SH.log[at];
        int len = (int)strlen(s);
        /* rows of this line, by characters (UTF-8 counted as it goes) */
        int starts[64], ns = 0, count = 0;
        starts[ns++] = 0;
        for (int b = 0; b < len; b++) {
            if (((unsigned char)s[b] & 0xC0) == 0x80)
                continue;
            if (count == cols && ns < 64) {
                starts[ns++] = b;
                count = 0;
            }
            count++;
        }
        for (int row = ns - 1; row >= 0 && drawn < rows_fit; row--) {
            if (skip > 0) {
                skip--;
                continue;
            }
            int end = row + 1 < ns ? starts[row + 1] : len;
            char part[LOG_LEN];
            snprintf(part, sizeof part, "%.*s", end - starts[row], s + starts[row]);
            TTF_Font *pf = SH.kind[at] == SHELL_HEAD ? fo->mono_bold : f;
            ink(pf, part, 8, y, kind_colour(SH.kind[at]), ALIGN_LEFT);
            y -= lh;
            drawn++;
        }
    }
    if (SH.scroll > 0 && skip > 0)
        SH.scroll -= skip; /* not that far back */
    ui_layer_end();
    return ret;
}

/* ---- the right-click menu ---- */

void syn_shell_menu_open(int x, int y, const char *title, const ShellCommand *items, int n)
{
    SH.menu = true;
    SH.mx = x;
    SH.my = y;
    snprintf(SH.title, sizeof SH.title, "%s", title ? title : "");
    SH.nitems = n < MENU_ITEMS ? n : MENU_ITEMS;
    memcpy(SH.items, items, sizeof items[0] * (size_t)SH.nitems);
    SH.launcher = false;
}

bool syn_shell_menu_showing(void)
{
    return SH.menu;
}

void syn_shell_menu_close(void)
{
    SH.menu = false;
}

int syn_shell_menu(int w, int h)
{
    if (!SH.menu)
        return 0;
    const Fonts *fo = synos_fonts();
    const Theme *t = skin_theme();
    TTF_Font *f = fo->mono_small;
    float row = skin_text_height(f) + 12, head = skin_text_height(fo->mono_small_bold) + 12;
    float mw = text_w(fo->mono_small_bold, SH.title) + 36;
    float mh = SH.title[0] ? head + 4 : 4;
    for (int i = 0; i < SH.nitems; i++) {
        const ShellCommand *c = &SH.items[i];
        float iw = 42 + text_w(f, c->label) + (c->key ? skin_keycap_width(f, c->key) + 30 : 16);
        mw = fmaxf(mw, iw);
        mh += !strcmp(c->label, "-") ? 9 : c->id == 0 ? head : row;
    }
    mh += 4;
    mw = fmaxf(mw, 200);
    float x = (float)SH.mx, y = (float)SH.my;
    if (x + mw > (float)w - 4)
        x = (float)w - 4 - mw;
    if (y + mh > (float)h - 4)
        y = fmaxf(4, (float)h - 4 - mh);
    UIRect m = {x, y, mw, mh};
    int got = 0;
    SDL_Rect all = {0, 0, w, h};
    ui_layer_begin(ui_id("shellmenu", 0), all, true);
    skin_panel(m, false);
    float cy = y + 2;
    if (SH.title[0]) {
        UIRect tb = {x + 2, cy, mw - 4, head};
        Rgb tc = skin_titlebar(tb);
        mid(fo->mono_small_bold, SH.title, tb.x + 14, tb.y + head * 0.5f, tc, ALIGN_LEFT);
        cy += head + 2;
    }
    for (int i = 0; i < SH.nitems; i++) {
        const ShellCommand *c = &SH.items[i];
        if (!strcmp(c->label, "-")) {
            skin_separator(x + 8, cy + 4, mw - 16);
            cy += 9;
            continue;
        }
        if (c->id == 0) {
            mid(fo->mono_small_bold, c->label, x + 14, cy + head * 0.5f, skin_tone(TONE_ACCENT), ALIGN_LEFT);
            cy += head;
            continue;
        }
        UIRect r = {x + 2, cy, mw - 4, row};
        bool hv = !c->disabled && over(r);
        Rgb fg = c->disabled ? skin_tone(TONE_MUTED) : t->text;
        if (hv) {
            /* labwc's: the item under the pointer filled in the accent */
            skin_fill(r, t->accent, 1);
            fg = skin_tone(TONE_ON_ACCENT);
        }
        float my = cy + row * 0.5f;
        if (c->icon)
            mid(fo->icon, c->icon, r.x + 20, my, hv ? fg : skin_tone(TONE_MUTED), ALIGN_CENTER);
        mid(f, c->label, r.x + 40, my, fg, ALIGN_LEFT);
        if (c->key) {
            float kw = skin_keycap_width(f, c->key);
            skin_keycap(ui_rect(r.x + r.w - 10 - kw, my - 9, kw, 19), f, c->key);
        }
        if (hv && clicked(r))
            got = c->id;
        cy += row;
    }
    int mx, my;
    ui_mouse(&mx, &my);
    if (got || ((ui_pressed(all, 1) || ui_pressed(all, 3)) && !skin_hit(m, mx, my)) ||
        ui_key_pressed(SDLK_ESCAPE))
        SH.menu = false;
    ui_layer_end();
    return got;
}

/* ---- the card ---- */

SDL_Rect syn_shell_card_rect(int w, int h)
{
    /* clear of the minimap at the right, unless the window is too narrow */
    int cw = w - 280 < 800 ? w - 280 : 800;
    if (cw < 520)
        cw = w - 32 < 520 ? w - 32 : 520;
    int ch = 196;
    return (SDL_Rect){16, h - SHELL_HINTS_H - 12 - ch, cw, ch};
}

int syn_shell_card(int w, int h, const ShellCard *c)
{
    const Fonts *fo = synos_fonts();
    SDL_Rect sr = syn_shell_card_rect(w, h);
    UIRect r = {(float)sr.x, (float)sr.y, (float)sr.w, (float)sr.h};
    int got = 0;
    ui_layer_begin(ui_id("shellcard", 0), sr, false);
    skin_panel(r, true);
    /* the title bar, as labwc draws a window's */
    float th = 28;
    UIRect tb = {r.x + 1, r.y + 1, r.w - 2, th};
    Rgb tc = skin_titlebar(tb);
    float tx = tb.x + 12, cy = tb.y + th * 0.5f;
    tx += mid(fo->mono_bold, c->title, tx, cy, tc, ALIGN_LEFT) + 12;
    if (c->state) {
        Rgb sc = skin_tone(c->state_tone);
        /* on a filled title bar the state keeps to the label's colour */
        const Theme *t = skin_theme();
        if (t->style == STYLE_SLAB || t->style == STYLE_BEVEL || t->style == STYLE_WIN95)
            sc = tc;
        mid(fo->mono_small_bold, c->state, tx, cy, sc, ALIGN_LEFT);
    }
    if (c->subtitle[0]) {
        char fit[96];
        mid(fo->mono_small, skin_fit(fo->mono_small, c->subtitle, r.w * 0.45f, fit, sizeof fit),
            tb.x + tb.w - 12, cy, tc, ALIGN_RIGHT);
    }

    /* its body, and the figures beside it in their wells */
    float top = r.y + th + 10;
    SDL_Rect body = {(int)r.x + 10, (int)top, 104, 104};
    if (c->draw_body)
        c->draw_body(c->ud, body);
    float gx = r.x + 10 + 104 + 12, gw = r.x + r.w - 10 - gx;
    int cols = c->ncells > 8 ? 5 : 4;
    float cwid = (gw - (float)(cols - 1) * 6) / (float)cols, chh = 24;
    for (int i = 0; i < c->ncells; i++) {
        UIRect cell = {gx + (float)(i % cols) * (cwid + 6), top + (float)(i / cols) * (chh + 6), cwid, chh};
        skin_well(cell);
        mid(fo->small, c->cells[i].label, cell.x + 7, cell.y + chh * 0.5f, skin_tone(TONE_MUTED), ALIGN_LEFT);
        mid(fo->mono_small_bold, c->cells[i].value, cell.x + cell.w - 7, cell.y + chh * 0.5f,
            skin_tone(c->cells[i].tone), ALIGN_RIGHT);
    }
    /* the bars: SYN-TREK's segments, the missing ones faint */
    float by = top + (float)((c->ncells + cols - 1) / cols) * (chh + 6) + 4;
    for (int b = 0; b < 2; b++) {
        if (!c->bar_label[b])
            continue;
        float bx = gx + (float)b * (gw / 2 + 3), bw = gw / 2 - 3;
        mid(fo->small, c->bar_label[b], bx, by + 7, skin_tone(TONE_MUTED), ALIGN_LEFT);
        float lw = text_w(fo->small, c->bar_label[b]) + 8;
        int cells = 14;
        float sw = (bw - lw - (float)(cells - 1) * 2) / (float)cells;
        double f = c->bar[b] < 0 ? 0 : c->bar[b] > 1 ? 1 : c->bar[b];
        int lit = (int)lround(f * cells);
        Rgb on = skin_tone(c->bar_tone[b]);
        for (int i = 0; i < cells; i++)
            skin_fill(ui_rect(bx + lw + (float)i * (sw + 2), by + 1, sw, 12), on, i < lit ? 1.0f : 0.16f);
    }

    /* what can be done to it, along the bottom: named, closer together if
     * that is what it takes, or as icons alone (named in their tooltips) */
    float ay = r.y + r.h - 10 - 30, ax = r.x + 10, text = 0, room = r.w - 20;
    for (int i = 0; i < c->nactions; i++)
        text += text_w(fo->small_bold, c->actions[i].label);
    int n = c->nactions;
    int tier = text + (float)n * 40 + (float)(n - 1) * 6 <= room   ? 0
               : text + (float)n * 33 + (float)(n - 1) * 4 <= room ? 1
                                                                   : 2;
    float gap = tier ? 4 : 6;
    for (int i = 0; i < c->nactions; i++) {
        const ShellCommand *a = &c->actions[i];
        bool icon_only = tier == 2 && a->icon;
        float bw = icon_only ? 38 : (tier ? 33 : 40) + text_w(fo->small_bold, a->label);
        if (ax + bw > r.x + r.w - 10)
            break;
        UIRect b = {ax, ay, bw, 30};
        bool hv = !a->disabled && over(b);
        Tone tone = skin_button_chrome(b, (WidgetState){.hover = hv, .active = a->on, .disabled = a->disabled}, true);
        Rgb fg = skin_tone(a->disabled ? TONE_MUTED : tone);
        float bcy = b.y + 14;
        if (a->icon)
            mid(fo->icon, a->icon, icon_only ? b.x + b.w / 2 : b.x + (tier ? 14 : 16), bcy, fg, ALIGN_CENTER);
        if (!icon_only)
            mid(fo->small_bold, a->label, b.x + (tier ? 26 : 30), bcy, fg, ALIGN_LEFT);
        if (hv) {
            ui_tooltip(a->tip ? a->tip : a->label);
            if (clicked(b))
                got = a->id;
        }
        ax += bw + gap;
    }
    ui_layer_end();
    return got;
}

bool syn_shell_modal(void)
{
    return SH.launcher || SH.menu || SH.console;
}

bool syn_shell_animating(void)
{
    return SH.nnote > 0 || SH.launcher || SH.menu || SH.console ||
           SDL_GetTicks() - SH.console_at < 200;
}

/* ---- dialogs ---- */

/* wrapped lines of text, drawn or only measured: the height */
static float wrap(TTF_Font *f, const char *text, float x, float y, float w, Rgb c, bool draw)
{
    float lh = skin_text_height(f) + 4, h = 0;
    const char *p = text;
    char line[512];
    while (*p) {
        /* a paragraph, to its \n */
        size_t plen = strcspn(p, "\n");
        size_t n = 0;
        const char *q = p, *end = p + plen;
        while (q < end) {
            const char *word = q;
            while (word < end && *word == ' ')
                word++;
            const char *we = word;
            while (we < end && *we != ' ')
                we++;
            char cand[512];
            snprintf(cand, sizeof cand, "%.*s%s%.*s", (int)n, line, n ? " " : "", (int)(we - word), word);
            if (n && text_w(f, cand) > w) {
                line[n] = '\0';
                if (draw)
                    ink(f, line, x, y + h, c, ALIGN_LEFT);
                h += lh;
                n = 0;
                continue;
            }
            n = strlen(cand) < sizeof line - 1 ? strlen(cand) : sizeof line - 1;
            memcpy(line, cand, n);
            q = we;
        }
        line[n] = '\0';
        if (draw && n)
            ink(f, line, x, y + h, c, ALIGN_LEFT);
        h += lh;
        p = *end ? end + 1 : end;
    }
    return h;
}

int syn_shell_dialog(int w, int h, const char *title, const char *text, const char *const *buttons, int n)
{
    const Fonts *fo = synos_fonts();
    const Theme *t = skin_theme();
    float dw = fminf(560, (float)w - 32), inner = dw - 48;
    float th = skin_text_height(fo->heading) + 10;
    float body = wrap(fo->body, text, 0, 0, inner, t->text, false);
    float dh = 20 + th + body + 24 + 38 + 20;
    UIRect r = {((float)w - dw) / 2, ((float)h - dh) / 2, dw, dh};
    SDL_Rect all = {0, 0, w, h};
    skin_fill(ui_rect(0, 0, (float)w, (float)h), (Rgb){0, 0, 0}, 0.55f);
    ui_layer_begin(ui_id("shelldialog", 0), all, true);
    skin_panel(r, false);
    ink(fo->heading, title, r.x + 24, r.y + 20, t->text, ALIGN_LEFT);
    wrap(fo->body, text, r.x + 24, r.y + 20 + th, inner, skin_tone(TONE_MUTED), true);
    int got = -1;
    float bx = r.x + r.w - 24, by = r.y + r.h - 20 - 38;
    for (int i = 0; i < n; i++) {
        float bw = text_w(fo->bold, buttons[i]) + 36;
        if (bw < 110)
            bw = 110;
        UIRect b = {bx - bw, by, bw, 38};
        bool hv = over(b);
        skin_button(b, fo->bold, buttons[i], (WidgetState){.hover = hv, .active = i == 0}, false);
        if (hv && clicked(b))
            got = i;
        bx -= bw + 8;
    }
    if (ui_key_pressed(SDLK_RETURN) || ui_key_pressed(SDLK_KP_ENTER))
        got = 0;
    if (ui_key_pressed(SDLK_ESCAPE))
        got = n - 1;
    ui_layer_end();
    return got;
}
