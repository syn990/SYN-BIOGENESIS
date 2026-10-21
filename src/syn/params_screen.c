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

#include "syn/params_screen.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "syn/genesis_screen.h"
#include "syn/shell.h"
#include "synos/synos.h"
#include "ui/app.h"
#include "ui/gfx.h"
#include "ui/ui.h"

#define PAD 24.0f
#define CONFIRM_MS 4000

static struct {
    int tab;
    float scroll;
    char status[200];
    bool status_bad;
    Uint32 status_at;
    Uint32 ask_at; /* DEFAULT VALUES asked once, when */
} S;

/* the part of the screen the fields scroll in: only a box wholly in it
 * takes the mouse and keys */
static UIRect g_view;

static SDL_Rect sdl(UIRect r)
{
    return (SDL_Rect){(int)floorf(r.x), (int)floorf(r.y), (int)ceilf(r.w), (int)ceilf(r.h)};
}

static bool over(UIRect r)
{
    return ui_hover(sdl(r));
}

static bool inside(UIRect r)
{
    return r.y >= g_view.y && r.y + r.h <= g_view.y + g_view.h;
}

/* a field's box: live while it is in view, drawn as it would be otherwise */
static bool field_box(int i, UIRect box, char *text, size_t cap)
{
    if (inside(box))
        return ui_textfield(ui_id("synparam", (uint32_t)i), sdl(box), text, cap);
    skin_input(box, false, false, false);
    gfx_text((int)box.x + 5, (int)(box.y + (box.h - (float)gfx_font_h()) / 2), text, C_TEXT);
    return false;
}

static bool button(UIRect r, const char *label, bool active)
{
    bool h = over(r);
    skin_button(r, synos_fonts()->bold, label, (WidgetState){.hover = h, .active = active}, true);
    return h && ui_pressed(sdl(r), 1);
}

/* text wrapped to w from (x, y), drawn or only measured: its height */
static float para(TTF_Font *f, const char *text, float x, float y, float w, Tone t, bool draw)
{
    char line[400];
    size_t n = 0;
    float lh = skin_text_height(f) + 3, h = 0;
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
            if (draw)
                skin_text(f, line, x, y + h, t, ALIGN_LEFT);
            h += lh;
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
        if (draw)
            skin_text(f, line, x, y + h, t, ALIGN_LEFT);
        h += lh;
    }
    return h;
}

/* the dialog's label, without its colon, in capitals */
static void caption(const char *label, char *out, size_t n)
{
    size_t k = 0;
    for (; label[k] && k + 1 < n; k++)
        out[k] = (char)toupper((unsigned char)label[k]);
    while (k > 0 && (out[k - 1] == ':' || out[k - 1] == ' '))
        k--;
    out[k] = '\0';
}

static void say(bool bad, const char *fmt, const char *a)
{
    snprintf(S.status, sizeof S.status, fmt, a ? a : "");
    S.status_bad = bad;
    S.status_at = SDL_GetTicks();
}

/* a field taken as entered: applied and saved, or refused */
static void take(struct App *a, int i)
{
    ParamsField f;
    params_info(i, &f);
    char name[64];
    caption(f.label, name, sizeof name);
    if (!params_valid(a, i)) {
        say(true, "%s: not a value it takes, so it stays as it was", name);
        return;
    }
    bool changed = params_changed(a, i);
    params_commit(a);
    if (changed)
        say(false, f.next_world ? "%s saved; it applies to the next new world" : "%s saved", name);
}

void syn_params_open(struct App *a)
{
    params_reload(a);
    S.scroll = 0;
    S.status[0] = '\0';
    /* screenshots: SYN_BIOGENESIS_SHOT_TAB = general | world | organisms | ... */
    const char *t = getenv("SYN_BIOGENESIS_SHOT_TAB");
    for (int i = 0; t && i < params_tab_count(); i++)
        if (!strcasecmp(t, params_tab(i)))
            S.tab = i;
}

/* the colours 0.9 names, for their swatches */
static bool swatch(const char *name, Rgb *c)
{
    static const struct {
        const char *name;
        Rgb rgb;
    } k[] = {{"red", {255, 0, 0}},     {"green", {0, 255, 0}},   {"blue", {0, 0, 255}},
             {"cyan", {0, 255, 255}},  {"white", {255, 255, 255}}, {"gray", {128, 128, 128}},
             {"yellow", {255, 255, 0}}};
    for (size_t i = 0; i < sizeof k / sizeof k[0]; i++)
        if (!strcmp(k[i].name, name)) {
            *c = k[i].rgb;
            return true;
        }
    return false;
}

/* one field's card: its name, what it does, its box, its default; its height */
static float field_card(struct App *a, int i, UIRect r, bool draw)
{
    const Fonts *fo = synos_fonts();
    ParamsField f;
    params_info(i, &f);
    char name[64];
    caption(f.label, name, sizeof name);
    float box_w = 150, tx = r.x + 16, tw = r.w - 32 - box_w - 16;
    float y = r.y + 14;
    float h = skin_text_height(fo->small_bold) + 6;
    if (f.help)
        h += para(fo->small, f.help, tx, y, tw, TONE_MUTED, false) + 2;
    float body_h = fmaxf(h, 36 + 4 + skin_text_height(fo->small));
    float extra = 0;
    char *text = params_text(a, i);
    if (f.genesis)
        extra = (float)syn_genesis_warning(0, 0, text, false);
    float total = 14 + body_h + extra + 14;
    if (!draw)
        return total;

    if (r.h < total)
        r.h = total;
    skin_panel(r, true);
    bool changed = !params_is_default(a, i);
    /* SYN-DEBT's stripe: what is not the default */
    if (changed)
        skin_fill(ui_rect(r.x, r.y, 4, r.h), skin_theme()->accent, 1);
    float nx = tx + skin_text(fo->small_bold, name, tx, y, TONE_TEXT, ALIGN_LEFT) + 8;
    if (f.unit)
        nx += skin_text(fo->small, f.unit, nx, y, TONE_MUTED, ALIGN_LEFT) + 8;
    if (f.next_world)
        skin_text(fo->small_bold, "NEXT WORLD", nx, y, TONE_ACCENT, ALIGN_LEFT);
    if (f.help)
        para(fo->small, f.help, tx, y + skin_text_height(fo->small_bold) + 6, tw, TONE_MUTED, true);

    UIRect box = {r.x + r.w - 16 - box_w, y - 2, box_w, 34};
    char def[32];
    params_default_text(i, def, sizeof def);
    if (f.type == 'c') {
        /* No or Yes, as the dialog's combo */
        UIRect no = {box.x, box.y, box_w / 2 - 3, 34}, yes = {box.x + box_w / 2 + 3, box.y, box_w / 2 - 3, 34};
        bool is_yes = !strcmp(text, "Yes");
        if (button(no, "NO", !is_yes) && is_yes && inside(no)) {
            snprintf(text, params_text_cap(), "No");
            take(a, i);
        }
        if (button(yes, "YES", is_yes) && !is_yes && inside(yes)) {
            snprintf(text, params_text_cap(), "Yes");
            take(a, i);
        }
    } else {
        bool bad = !params_valid(a, i);
        if (field_box(i, box, text, params_text_cap()))
            take(a, i);
        if (bad)
            skin_input(box, false, false, true);
    }
    /* its default, a click away */
    char dl[64];
    snprintf(dl, sizeof dl, changed ? "default %s: click" : "default %s", def);
    UIRect dr = {box.x, box.y + 38, box_w, skin_text_height(fo->small)};
    bool dh = changed && over(dr);
    skin_text(fo->small, dl, box.x + box_w, dr.y, dh ? TONE_ACCENT : TONE_MUTED, ALIGN_RIGHT);
    if (dh && inside(dr) && ui_pressed(sdl(dr), 1)) {
        snprintf(text, params_text_cap(), "%s", def);
        take(a, i);
    }
    if (f.genesis)
        syn_genesis_warning((int)tx, (int)(r.y + 14 + body_h), text, true);
    return total;
}

/* the colours: probability, its share of all of them, and the cost */
static float genes_table(struct App *a, int tab, UIRect r, bool draw)
{
    const Fonts *fo = synos_fonts();
    int n = params_count();
    double sum = 0;
    for (int i = 0; i < n; i++) {
        ParamsField f;
        params_info(i, &f);
        if (params_tab_genes(f.tab) && f.type == 'i' && params_valid(a, i))
            sum += atof(params_text(a, i));
    }
    float row = 44, y = r.y + 12;
    float c_name = r.x + 16, c_prob = r.x + r.w * 0.30f, c_share = r.x + r.w * 0.52f, c_cost = r.x + r.w * 0.68f;
    int rows = 0;
    for (int i = 0; i < n; i++) {
        ParamsField f;
        params_info(i, &f);
        if (f.tab == tab && f.type == 'i')
            rows++;
    }
    float total = 12 + 28 + (float)rows * row + 12;
    if (!draw)
        return total;
    r.h = total;
    skin_panel(r, true);
    skin_text(fo->small_bold, "COLOUR", c_name, y, TONE_MUTED, ALIGN_LEFT);
    skin_text(fo->small_bold, "PROBABILITY", c_prob, y, TONE_MUTED, ALIGN_LEFT);
    skin_text(fo->small_bold, "SHARE", c_share, y, TONE_MUTED, ALIGN_LEFT);
    skin_text(fo->small_bold, "COST", c_cost, y, TONE_MUTED, ALIGN_LEFT);
    y += 28;
    for (int i = 0; i < n; i++) {
        ParamsField f;
        params_info(i, &f);
        if (f.tab != tab || f.type != 'i')
            continue;
        skin_separator(r.x + 12, y - 4, r.w - 24);
        float cy = y + row * 0.5f - 4;
        Rgb sw;
        float nx = c_name;
        if (swatch(f.label, &sw)) {
            skin_fill(ui_rect(nx, cy - 7, 14, 14), sw, 1);
            nx += 22;
        }
        char name[64];
        caption(f.label, name, sizeof name);
        skin_text(fo->bold, name, nx, cy - skin_text_height(fo->bold) * 0.5f, TONE_TEXT, ALIGN_LEFT);
        UIRect pb = {c_prob, cy - 15, 120, 30};
        char *pt = params_text(a, i);
        if (field_box(i, pb, pt, params_text_cap()))
            take(a, i);
        if (!params_valid(a, i))
            skin_input(pb, false, false, true);
        if (!params_is_default(a, i))
            skin_fill(ui_rect(pb.x - 8, pb.y, 3, pb.h), skin_theme()->accent, 1);
        /* its share of the colours a random gene can be */
        char share[32];
        double v = params_valid(a, i) ? atof(pt) : 0;
        snprintf(share, sizeof share, "%.1f%%", sum > 0 ? 100.0 * v / sum : 0.0);
        skin_meter(ui_rect(c_share, cy + 8, r.w * 0.12f, 4), sum > 0 ? v / sum : 0, TONE_ACCENT);
        skin_text(fo->mono_small_bold, share, c_share, cy - 12, TONE_TEXT, ALIGN_LEFT);
        /* the cost is the field after it */
        if (i + 1 < n) {
            ParamsField g;
            params_info(i + 1, &g);
            if (g.tab == tab && g.type == 'd') {
                UIRect cb = {c_cost, cy - 15, 120, 30};
                char *ct = params_text(a, i + 1);
                if (field_box(i + 1, cb, ct, params_text_cap()))
                    take(a, i + 1);
                if (!params_valid(a, i + 1))
                    skin_input(cb, false, false, true);
                if (!params_is_default(a, i + 1))
                    skin_fill(ui_rect(cb.x - 8, cb.y, 3, cb.h), skin_theme()->accent, 1);
            }
        }
        y += row;
    }
    return total;
}

void syn_params_frame(struct App *a, int w, int h)
{
    const Fonts *fo = synos_fonts();
    bool keys = !ui_any_modal() && !ui_text_focused() && !ui_popup_showing();
    if (keys && ui_key_pressed(SDLK_ESCAPE)) {
        synos_set_screen(SYNOS_WORLD);
        return;
    }
    int ntabs = params_tab_count();
    if (S.tab >= ntabs)
        S.tab = 0;

    skin_fill(ui_rect(0, 0, (float)w, (float)h), (Rgb){0, 0, 0}, 0.5f);
    float under_bar = SHELL_BAR_H + 10;
    float pw = fminf((float)w - 32, 1500), ph = (float)h - under_bar - 16;
    UIRect panel = {((float)w - pw) / 2, under_bar, pw, ph};
    skin_panel(panel, true);

    /* the tabs; at the right, the defaults and what was last said */
    float x = panel.x + PAD, y = panel.y + 14;
    for (int t = 0; t < ntabs; t++) {
        const char *name = params_tab(t);
        float tw = skin_text_width(fo->bold, name) + 30;
        UIRect r = {x, y, tw, 36};
        bool hv = over(r);
        skin_tab(r, fo->bold, name, (WidgetState){.hover = hv, .active = S.tab == t});
        if (hv && ui_pressed(sdl(r), 1) && S.tab != t) {
            ui_unfocus();
            S.tab = t;
            S.scroll = 0;
        }
        x += tw + 6;
    }
    bool asked = SDL_GetTicks() - S.ask_at < CONFIRM_MS && S.ask_at;
    const char *dlab = asked ? "SURE? EVERY ONE" : "DEFAULT VALUES";
    float dw = skin_text_width(fo->bold, dlab) + 34;
    UIRect dr = {panel.x + panel.w - PAD - dw, y, dw, 36};
    /* a narrow window: the button goes under the tabs */
    if (dr.x < x + 12)
        dr.y += 44;
    if (button(dr, dlab, asked)) {
        if (asked) {
            ui_unfocus();
            params_defaults(a);
            params_commit(a);
            S.ask_at = 0;
            say(false, "Every parameter is its default again%s", "");
        } else {
            S.ask_at = SDL_GetTicks();
        }
    }
    if (over(dr))
        ui_tooltip("Every parameter back to the value Biogenesis starts with (click twice)");

    float top = fmaxf(y + 50, dr.y + 50);
    /* what was said, or how it works */
    const char *line = S.status[0] && SDL_GetTicks() - S.status_at < 6000
                           ? S.status
                           : "Each value is taken as it is entered: Enter, Tab or a click elsewhere. A stripe "
                             "marks the ones that aren't their default.";
    Tone lt = S.status[0] && SDL_GetTicks() - S.status_at < 6000 ? (S.status_bad ? TONE_BAD : TONE_GOOD)
                                                                 : TONE_MUTED;
    skin_text(fo->small, line, panel.x + PAD, top - 10, lt, ALIGN_LEFT);
    top += 12;

    /* the fields, scrolled */
    UIRect view = {panel.x + PAD, top, panel.w - 2 * PAD, panel.y + panel.h - 16 - top};
    g_view = view;
    const char *note = params_tab_note(S.tab);
    float content = 0;
    int cols = view.w >= 980 ? 2 : 1;
    float cw = (view.w - (float)(cols - 1) * 14) / (float)cols;
    /* measure first: the cards of a row are as tall as the tallest */
    float heights[256];
    int idx[256], nf = 0;
    int n = params_count();
    for (int i = 0; i < n && nf < 256; i++) {
        ParamsField f;
        params_info(i, &f);
        if (f.tab != S.tab)
            continue;
        idx[nf] = i;
        heights[nf++] = field_card(a, i, ui_rect(0, 0, cw, 0), false);
    }
    float note_h = note ? para(fo->small, note, 0, 0, view.w, TONE_MUTED, false) + 12 : 0;
    content += note_h;
    if (params_tab_genes(S.tab)) {
        content += genes_table(a, S.tab, ui_rect(view.x, 0, view.w, 0), false);
    } else {
        for (int k = 0; k < nf; k += cols) {
            float rh = heights[k];
            for (int c = 1; c < cols && k + c < nf; c++)
                rh = fmaxf(rh, heights[k + c]);
            content += rh + 12;
        }
    }
    float max_scroll = fmaxf(0, content - view.h);
    if (over(view)) {
        float wy = ui_wheel_fy();
        S.scroll -= wy * 60;
    }
    S.scroll = fminf(fmaxf(S.scroll, 0), max_scroll);

    gfx_clip_push(sdl(view));
    float cy = view.y - S.scroll;
    if (note)
        cy += para(fo->small, note, view.x, cy, view.w, TONE_MUTED, true) + 12;
    if (params_tab_genes(S.tab)) {
        genes_table(a, S.tab, ui_rect(view.x, cy, view.w, 0), true);
    } else {
        for (int k = 0; k < nf; k += cols) {
            float rh = heights[k];
            for (int c = 1; c < cols && k + c < nf; c++)
                rh = fmaxf(rh, heights[k + c]);
            for (int c = 0; c < cols && k + c < nf; c++)
                field_card(a, idx[k + c], ui_rect(view.x + (float)c * (cw + 14), cy, cw, rh), true);
            cy += rh + 12;
        }
    }
    gfx_clip_pop();
    if (max_scroll > 0) {
        float th = view.h * view.h / content;
        float ty = view.y + (view.h - th) * (S.scroll / max_scroll);
        skin_fill(ui_rect(panel.x + panel.w - 10, ty, 4, th), skin_theme()->accent, 0.8f);
    }
}
