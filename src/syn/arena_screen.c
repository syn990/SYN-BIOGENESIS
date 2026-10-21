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

#include "syn/arena_screen.h"

#include <SDL_image.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "compat.h"
#include "sim/params.h"
#include "syn/arena.h"
#include "syn/batch.h"
#include "syn/library_screen.h"
#include "syn/palette.h"
#include "synos/synos.h"
#include "ui/app.h"
#include "ui/gfx.h"
#include "ui/ui.h"

#define PAD 24.0f
#define GAP 14.0f
#define TIP_LINES 12

#ifdef BIO_COLOUR
#define SIM_TITLE "Color Mod 2.01"
#define SIM_FILE "colour"
#else
#define SIM_TITLE "Biogenesis 0.9"
#define SIM_FILE "0.9"
#endif

static struct {
    Batch b;
    float scroll;     /* the leaderboard's */
    int view;               /* V_SETUP, V_BATTLES, V_TREE */
    bool view_set;          /* chosen yet */
    char status[400];       /* what PRINT WINNERS did, for a while */
    Uint32 status_at;
    /* a readout drawn over everything, last */
    char tip[TIP_LINES][96];
    SDL_Color tip_col[TIP_LINES];
    int ntip;
    float tip_x, tip_y;
} S;

enum { V_SETUP, V_BATTLES, V_TREE };

static int g_mx, g_my;

static SDL_Rect sdl(UIRect r)
{
    return (SDL_Rect){(int)floorf(r.x), (int)floorf(r.y), (int)ceilf(r.w), (int)ceilf(r.h)};
}

/* the mouse is over r, on this screen (not on a window above it) */
static bool over(UIRect r)
{
    return ui_hover(sdl(r));
}

static bool button(UIRect r, const char *label, bool active, bool enabled)
{
    bool h = enabled && over(r);
    skin_button(r, synos_fonts()->bold, label, (WidgetState){.hover = h, .active = active, .disabled = !enabled},
                true);
    return h && ui_pressed(sdl(r), 1);
}

static SDL_Color tone(Tone t, Uint8 a)
{
    SDL_Color c = skin_tone_color(t);
    c.a = a;
    return c;
}

/* a gene's colour as the simulation keeps it */
static SDL_Color raw(int c, Uint8 a)
{
    uint8_t r, g, b;
    bio_color_rgb(c, &r, &g, &b);
    return (SDL_Color){r, g, b, a};
}

/* The lines' colours: eight hues in a fixed order, a step for dark themes
 * and one for light, the fighters first (seat k, colour k, in every lane). */
static SDL_Color series(int i, Uint8 a)
{
    static const Uint8 dark[8][3] = {{0x39, 0x87, 0xe5}, {0xd9, 0x59, 0x26}, {0x19, 0x9e, 0x70}, {0xc9, 0x85, 0x00},
                                     {0xd5, 0x51, 0x81}, {0x00, 0x83, 0x00}, {0x90, 0x85, 0xe9}, {0xe6, 0x67, 0x67}};
    static const Uint8 light[8][3] = {{0x2a, 0x78, 0xd6}, {0xeb, 0x68, 0x34}, {0x1b, 0xaf, 0x7a}, {0xed, 0xa1, 0x00},
                                      {0xe8, 0x7b, 0xa4}, {0x00, 0x83, 0x00}, {0x62, 0x50, 0xd6}, {0xe3, 0x49, 0x48}};
    if (i < 0 || i >= 8)
        return tone(TONE_MUTED, (Uint8)(a / 2));
    const Uint8 *c = skin_theme()->light ? light[i] : dark[i];
    return (SDL_Color){c[0], c[1], c[2], a};
}

/* each outcome a hue of its own, in a fixed order; no survivors grey */
static SDL_Color outcome(int o, Uint8 a)
{
    return o >= 0 && o < OUT_EXTINCT ? series(o, a) : tone(TONE_MUTED, a);
}

/* the outcomes as this screen names them */
static const char *outcome_label(int o)
{
    static const char *const k[OUT_KINDS] = {"QUALIFIED", "WON", "OFFSPRING WON", "UPSET", "NO SURVIVORS"};
    return o >= 0 && o < OUT_KINDS ? k[o] : "";
}

/* the worlds as this screen names them: "a small world" */
static const char *world_label(int map)
{
    return map == MAP_SMALL ? "small" : map == MAP_MEDIUM ? "medium" : map == MAP_LARGE ? "large" : "own";
}

/* "40 s", "12 min", "2 h 5 min" */
static void span_text(char *out, size_t n, double s)
{
    if (s < 59.5)
        snprintf(out, n, "%d s", s < 1 ? 1 : (int)lround(s));
    else if (s < 3570)
        snprintf(out, n, "%d min", (int)lround(s / 60));
    else
        snprintf(out, n, "%d h %d min", (int)lround(s / 60) / 60, (int)lround(s / 60) % 60);
}

static void tip_line(SDL_Color c, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static void tip_line(SDL_Color c, const char *fmt, ...)
{
    if (S.ntip >= TIP_LINES)
        return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(S.tip[S.ntip], sizeof S.tip[S.ntip], fmt, ap);
    va_end(ap);
    S.tip_col[S.ntip++] = c;
}

static void tip_draw(int w, int h)
{
    if (!S.ntip)
        return;
    const Fonts *fo = synos_fonts();
    float lh = skin_text_height(fo->small) + 3, tw = 0;
    for (int i = 0; i < S.ntip; i++)
        tw = fmaxf(tw, skin_text_width(i ? fo->small : fo->small_bold, S.tip[i]));
    UIRect r = {S.tip_x + 14, S.tip_y + 14, tw + 20, lh * (float)S.ntip + 12};
    if (r.x + r.w > (float)w - 8)
        r.x = S.tip_x - 14 - r.w;
    if (r.y + r.h > (float)h - 8)
        r.y = (float)h - 8 - r.h;
    skin_panel(r, true);
    for (int i = 0; i < S.ntip; i++) {
        float y = r.y + 6 + lh * (float)i;
        if (S.tip_col[i].a) {
            skin_fill(ui_rect(r.x + 8, y + lh / 2 - 4, 8, 8), (Rgb){S.tip_col[i].r, S.tip_col[i].g, S.tip_col[i].b}, 1);
            skin_text(i ? fo->small : fo->small_bold, S.tip[i], r.x + 20, y, TONE_TEXT, ALIGN_LEFT);
        } else {
            skin_text(i ? fo->small : fo->small_bold, S.tip[i], r.x + 8, y, i ? TONE_MUTED : TONE_TEXT, ALIGN_LEFT);
        }
    }
    S.ntip = 0;
}

/* Text wrapped to width w from (x, y), drawn or only measured; returns the
 * y after it. */
static float para_at(TTF_Font *f, const char *text, float x, float y, float w, Tone t, bool draw)
{
    char line[400];
    size_t n = 0;
    float lh = skin_text_height(f) + 4;
    const char *p = text;
    while (*p) {
        /* the next word, with the space before it */
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
        if (draw)
            skin_text(f, line, x, y, t, ALIGN_LEFT);
        y += lh;
    }
    return y;
}

static float para(TTF_Font *f, const char *text, float x, float y, float w, Tone t)
{
    return para_at(f, text, x, y, w, t, true);
}

static float para_h(TTF_Font *f, const char *text, float w)
{
    return para_at(f, text, 0, 0, w, TONE_MUTED, false);
}

/* ---- beings ---- */

/* A code's body in r, on black, as the laboratory draws it. */
static void body(UIRect r, const GeneticCode *gc, Uint8 alpha)
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
    batch_begin(&S.b, TX_DISC, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < m; i++)
        batch_capsule(&S.b, cx + (float)lines[i].x0 * scale, cy + (float)lines[i].y0 * scale,
                      cx + (float)lines[i].x1 * scale, cy + (float)lines[i].y1 * scale, hw,
                      raw(lines[i].color, alpha));
    batch_flush(&S.b, gfx_renderer());
    if (lines != stackbuf)
        free(lines);
}

/* A being that opens in the laboratory on a click; a right click offers
 * copying its code, or keeping it in the being library under its name. */
static void thumb(App *a, UIRect r, GeneticCode *gc, uint32_t id, const char *tip, const char *name)
{
    int pick = ui_popup_take(id);
    if (pick == 0 && gc)
        lab_open(a, gc);
    else if (pick == 1 && gc) {
        app_set_clipboard(a, gc);
        app_set_status(a, "Genetic code copied");
        snprintf(S.status, sizeof S.status, "Genetic code copied: paste it into the world from its menu");
        S.status_at = SDL_GetTicks();
    } else if (pick == 2 && gc) {
        char origin[200];
        snprintf(origin, sizeof origin, "the arena: %.150s", tip ? tip : "");
        if (syn_library_keep(a, gc, "Arena", name && *name ? name : "ARENA BEING", origin))
            snprintf(S.status, sizeof S.status, "Kept in the library's Arena folder: %s",
                     name && *name ? name : "ARENA BEING");
        else
            snprintf(S.status, sizeof S.status, "It couldn't be kept in the library");
        S.status_at = SDL_GetTicks();
    }
    gfx_clip_push(sdl(r));
    body(r, gc, 255);
    gfx_clip_pop();
    if (gc && over(r)) {
        skin_fill(r, skin_theme()->accent, 0.12f);
        if (tip)
            ui_tooltip(tip);
        if (ui_pressed(sdl(r), 1))
            lab_open(a, gc);
        if (ui_pressed(sdl(r), 3)) {
            const char *items[] = {"Open in the laboratory", "Copy genetic code", "Keep in the library"};
            ui_popup_open(id, g_mx, g_my, items, NULL, 3);
        }
    }
}

/* ---- charts ---- */

/* filled triangles between two polylines */
static void band(const float *x, const float *ytop, const float *ybot, int n, SDL_Color c)
{
    if (n < 2)
        return;
    SDL_Vertex *v = malloc((size_t)n * 2 * sizeof *v);
    int *ix = malloc((size_t)(n - 1) * 6 * sizeof *ix);
    if (!v || !ix) {
        free(v);
        free(ix);
        return;
    }
    for (int i = 0; i < n; i++) {
        v[2 * i] = (SDL_Vertex){{x[i], ytop[i]}, c, {0, 0}};
        v[2 * i + 1] = (SDL_Vertex){{x[i], ybot[i]}, c, {0, 0}};
    }
    for (int i = 0; i < n - 1; i++) {
        int *q = ix + 6 * i, k = 2 * i;
        q[0] = k;
        q[1] = k + 1;
        q[2] = k + 2;
        q[3] = k + 2;
        q[4] = k + 1;
        q[5] = k + 3;
    }
    SDL_SetRenderDrawBlendMode(gfx_renderer(), SDL_BLENDMODE_BLEND);
    SDL_RenderGeometry(gfx_renderer(), NULL, v, n * 2, ix, (n - 1) * 6);
    free(v);
    free(ix);
}

/* A lane's bloodlines over its round so far, as shares of the living,
 * stacked: the followed ones from the bottom (the fighters first), the
 * rest above in grey. */
static void lane_chart(UIRect r, const ArenaLane *L)
{
    skin_fill(r, (Rgb){0, 0, 0}, 0.28f);
    int n = L->nsamp;
    if (n < 1)
        return;
    /* the round so far, across the width; at most a point a pixel, the
     * newest always */
    float t0 = (float)L->samp[0].t, span = fmaxf(1, (float)L->samp[n - 1].t - t0);
    int step = n > (int)r.w ? (n + (int)r.w - 1) / (int)r.w : 1;
    int m = (n + step - 1) / step + 1;
    float *x = malloc((size_t)m * sizeof *x), *lo = malloc((size_t)m * sizeof *lo),
          *hi = malloc((size_t)m * sizeof *hi);
    if (!x || !lo || !hi) {
        free(x);
        free(lo);
        free(hi);
        return;
    }
    for (int k = 0; k <= ARENA_SERIES; k++) {
        if (k < ARENA_SERIES && k >= L->nseries)
            continue;
        int c = 0;
        for (int i = 0; i < n; i = i + step < n || i == n - 1 ? i + step : n - 1) {
            const ArenaSample *s = &L->samp[i];
            float all = s->pop > 0 ? (float)s->pop : 1, below = 0;
            for (int j = 0; j < k; j++)
                below += s->n[j];
            x[c] = r.x + r.w * ((float)s->t - t0) / span;
            lo[c] = r.y + r.h - r.h * below / all;
            hi[c] = r.y + r.h - r.h * (below + s->n[k]) / all;
            c++;
        }
        /* the series' own colour: a fighter's seat, else its place */
        int col = k == ARENA_SERIES ? -1 : L->series_slot[k] >= 0 ? L->series_slot[k] : k;
        band(x, hi, lo, c, k == ARENA_SERIES ? tone(TONE_MUTED, 70) : series(col, 220));
    }
    free(x);
    free(lo);
    free(hi);
    /* hovered: the time and every bloodline at it */
    if (over(r)) {
        float t = t0 + ((float)g_mx - r.x) / r.w * span;
        int best = 0;
        for (int i = 1; i < n; i++)
            if (fabsf((float)L->samp[i].t - t) < fabsf((float)L->samp[best].t - t))
                best = i;
        const ArenaSample *s = &L->samp[best];
        float gx = r.x + r.w * ((float)s->t - t0) / span, all = s->pop > 0 ? (float)s->pop : 1;
        skin_fill(ui_rect(gx, r.y, 1, r.h), (Rgb){255, 255, 255}, 0.5f);
        S.ntip = 0;
        tip_line((SDL_Color){0, 0, 0, 0}, "time %d: %d alive", s->t, s->pop);
        for (int k = 0; k < L->nseries; k++) {
            int col = L->series_slot[k] >= 0 ? L->series_slot[k] : k;
            tip_line(series(col, 255), "%s%s: %.0f (%.0f%%)", L->series_label[k],
                     L->series_slot[k] >= 0 ? " (fighter)" : "", s->n[k], 100 * s->n[k] / all);
        }
        tip_line(tone(TONE_MUTED, 255), "other bloodlines: %.0f (%.0f%%)", s->n[ARENA_SERIES],
                 100 * s->n[ARENA_SERIES] / all);
        S.tip_x = (float)g_mx;
        S.tip_y = (float)g_my;
    }
}

static void lane_card(App *a, UIRect r, int i, const ArenaLane *L)
{
    const Fonts *fo = synos_fonts();
    skin_fill(r, skin_theme()->bg_alt, 0.55f);
    char s[200], v[32];
    float x = r.x + 10, y = r.y + 8;
    if (!L->up) {
        skin_text(fo->bold, "STOPPED", x, y, TONE_MUTED, ALIGN_LEFT);
        return;
    }
    if (!L->busy && !L->nsamp) {
        snprintf(s, sizeof s, "BATTLE %d", i + 1);
        skin_text(fo->bold, s, x, y, TONE_TEXT, ALIGN_LEFT);
        skin_text(fo->small, "setting up a round", x, y + 22, TONE_MUTED, ALIGN_LEFT);
        return;
    }
    /* what round it is, as much as fits; the time, and what is left of it
     * at the speed so far, as much as fits beside it */
    char title[2][48];
    if (L->level) {
        snprintf(title[0], sizeof title[0], "LEVEL %d · ROUND %d", L->level, L->cycle);
        snprintf(title[1], sizeof title[1], "LEVEL %d", L->level);
    } else {
        snprintf(title[0], sizeof title[0], "QUALIFIER · ROUND %d", L->cycle);
        snprintf(title[1], sizeof title[1], "QUALIFIER");
    }
    int ti = skin_text_width(fo->bold, title[0]) <= (r.w - 20) * 0.55f ? 0 : 1;
    float tw = skin_text(fo->bold, title[ti], x, y, TONE_TEXT, ALIGN_LEFT);
    char opt[3][96], left[32];
    double secs = L->fps > 0 && L->t < L->units ? (double)(L->units - L->t) * 256 / L->fps : 0;
    span_text(left, sizeof left, secs);
    snprintf(opt[0], sizeof opt[0], "time %d of %d · up to %s left", L->t, L->units, left);
    snprintf(opt[1], sizeof opt[1], "time %d of %d", L->t, L->units);
    snprintf(opt[2], sizeof opt[2], "%d/%d", L->t, L->units);
    for (int k = secs > 0 ? 0 : 1; k < 3; k++)
        if (skin_text_width(fo->small, opt[k]) <= r.w - 20 - tw - 12) {
            skin_text(fo->small, opt[k], r.x + r.w - 10, y + 3, TONE_MUTED, ALIGN_RIGHT);
            break;
        }
    if (over(ui_rect(r.x, r.y, r.w, 32))) {
        S.ntip = 0;
        if (L->level)
            tip_line((SDL_Color){0, 0, 0, 0}, "Round %d: %d beings of level %d", L->cycle, L->nslots, L->level);
        else
            tip_line((SDL_Color){0, 0, 0, 0}, "Round %d, a qualifier: newcomers only", L->cycle);
        if (L->level)
            tip_line((SDL_Color){0, 0, 0, 0}, "the winner goes up to level %d, the rest are knocked out", L->level + 1);
        else
            tip_line((SDL_Color){0, 0, 0, 0}, "the winner goes on the ladder at level 1");
        tip_line((SDL_Color){0, 0, 0, 0}, "time in time units (256 frames = 1 time unit)");
        if (secs > 0)
            tip_line((SDL_Color){0, 0, 0, 0}, "%s left at most, at this speed%s", left,
                     syn_arena_settings()->knockout ? " (sudden death can end it sooner)" : "");
        fmt_num(v, sizeof v, L->fps);
        tip_line((SDL_Color){0, 0, 0, 0}, "%s frames a second · seed %lld", v, (long long)L->seed);
        S.tip_x = (float)g_mx;
        S.tip_y = (float)g_my;
    }
    skin_meter(ui_rect(x, y + 24, r.w - 20, 4), L->units ? (double)L->t / L->units : 0, TONE_ACCENT);

    float foot = 52;
    UIRect chart = {x, y + 36, r.w - 20, r.h - foot - 54};
    if (chart.h > 20)
        lane_chart(chart, L);

    /* the leader, and the fighters as they stand */
    float fy = r.y + r.h - foot - 4;
    UIRect th = {x, fy, foot - 4, foot - 4};
    char tip[128];
    snprintf(tip, sizeof tip, "%s leads round %d: click for the laboratory", L->leader_name, L->cycle);
    thumb(a, th, L->leader_gc, ui_id("arena-lead", (uint32_t)i), tip, L->leader_name);
    float tx = th.x + th.w + 10, tr = r.x + r.w - 10;
    char fit[64];
    skin_text(fo->bold, skin_fit(fo->bold, L->leader_name[0] ? L->leader_name : "-", tr - tx, fit, sizeof fit), tx,
              fy, TONE_TEXT, ALIGN_LEFT);
    char p1[24], p2[24];
    fmt_num(p1, sizeof p1, L->leader_alive);
    fmt_num(p2, sizeof p2, L->pop);
    snprintf(s, sizeof s, "leading: %s of %s alive · %d bloodlines", p1, p2, L->lines);
    skin_text(fo->small, skin_fit(fo->small, s, tr - tx, fit, sizeof fit), tx, fy + 19, TONE_MUTED, ALIGN_LEFT);
    float dx = tx;
    for (int k = 0; k < L->nslots; k++) {
        snprintf(s, sizeof s, "%s %d", L->slot_name[k], L->slot_alive[k]);
        float w = skin_text_width(fo->small, s) + 16;
        if (dx + w > tr)
            break;
        SDL_Color c = series(k, 255);
        skin_fill(ui_rect(dx, fy + 39, 8, 8), (Rgb){c.r, c.g, c.b}, L->slot_alive[k] ? 1.0f : 0.3f);
        skin_text(fo->small, s, dx + 11, fy + 35, L->slot_alive[k] ? TONE_TEXT : TONE_MUTED, ALIGN_LEFT);
        dx += w + 6;
    }
}

/* ---- the setup ---- */

/* buttons side by side across w, the chosen one lit: the one clicked, or -1 */
static int choices(float x, float y, float w, const char *const *labels, int n, int sel, bool enabled)
{
    float bw = (w - 6 * (float)(n - 1)) / (float)n;
    int hit = -1;
    for (int i = 0; i < n; i++)
        if (button(ui_rect(x + (bw + 6) * (float)i, y, bw, 32), labels[i], i == sel, enabled) && i != sel)
            hit = i;
    return hit;
}

#define CARD_TOP 40.0f /* a card's title and question */
#define CARD_IN 14.0f  /* its margin */

/* A card of the setup, with its title and the question it answers; returns
 * where its contents start. */
static float card(UIRect r, const char *title, const char *question)
{
    const Fonts *fo = synos_fonts();
    skin_fill(r, skin_theme()->bg_alt, 0.55f);
    float tw = skin_text(fo->bold, title, r.x + CARD_IN, r.y + 10, TONE_TEXT, ALIGN_LEFT);
    char fit[160];
    skin_text(fo->small, skin_fit(fo->small, question, r.w - 2 * CARD_IN - tw - 12, fit, sizeof fit),
              r.x + CARD_IN + tw + 12, r.y + 13, TONE_MUTED, ALIGN_LEFT);
    return r.y + CARD_TOP;
}

/* a card of choices: its buttons (with shorter labels for narrow ones),
 * then what the one chosen means */
typedef struct {
    const char *title, *question;
    const char *labels[4];
    int n, sel;
    char says[400];
    float h;
    const char *shorts[4];
} Choice;

static void choice_measure(Choice *c, float w)
{
    c->h = CARD_TOP + 32 + 8 + para_h(synos_fonts()->small, c->says, w - 2 * CARD_IN) + 6;
}

/* drawn at (x, y): the choice clicked, or -1 */
static int choice_card(const Choice *c, float x, float y, float w, bool enabled)
{
    float cy = card(ui_rect(x, y, w, c->h), c->title, c->question);
    float bw = (w - 2 * CARD_IN - 6 * (float)(c->n - 1)) / (float)c->n;
    bool narrow = false;
    for (int i = 0; i < c->n; i++)
        narrow |= c->shorts[i] && skin_text_width(synos_fonts()->bold, c->labels[i]) > bw - 16;
    int k = choices(x + CARD_IN, cy, w - 2 * CARD_IN, narrow ? c->shorts : c->labels, c->n, c->sel, enabled);
    para(synos_fonts()->small, c->says, x + CARD_IN, cy + 40, w - 2 * CARD_IN, TONE_MUTED);
    return k;
}

/* how a winners' round's first organisms are shared, as the arena shares
 * them: the newcomers, and the copies of each fighter */
static void field(int organisms, int fighters, int wild_pct, int *newcomers, int *copies)
{
    int per = 0;
    if (fighters > 0) {
        per = (int)lround(organisms * (100.0 - wild_pct) / 100.0) / fighters;
        if (per < 1)
            per = 1;
    }
    *copies = per;
    *newcomers = wild_pct > 0 && organisms - per * fighters > 0 ? organisms - per * fighters : 0;
}

static void leaderboard(App *a, UIRect r);
static void ladder(App *a, UIRect r);
static void feed(App *a, UIRect r);

/* The highest on the ladder, large, and its record in words; the height
 * taken (0: there is none). */
static float champion(App *a, UIRect r)
{
    const Fonts *fo = synos_fonts();
    if (syn_arena_count() == 0)
        return 0;
    const ArenaEntry *e = syn_arena_at(0);
    float side = fminf(r.h - CARD_TOP - CARD_IN, 220);
    if (side < 90)
        return 0;
    /* its kin: the beings of its bloodline */
    int kin = 0;
    for (int i = 1; i < syn_arena_count(); i++)
        kin += !strcmp(syn_arena_at(i)->line, e->line);
    const ArenaEntry *par = syn_arena_find(e->parent);
    char level[48], plan[400], record[400], more[120], s[160], fit[160];
    snprintf(level, sizeof level, "LEVEL %d", e->level);
    snprintf(plan, sizeof plan, "%s bloodline · %s", e->line, e->plan);
    if (e->state == ENTRY_IN) {
        snprintf(record, sizeof record, "%d win%s in a row: it first won in round %d%s%s, and every round since. %s",
                 e->wins, e->wins == 1 ? "" : "s", e->cycle, par ? ", as the offspring of " : "", par ? par->name : "",
                 syn_arena_in_round(e->uid) ? "It is fighting now." : "It is waiting for a challenger of its level.");
    } else {
        char st[120];
        syn_arena_standing(e, st, sizeof st);
        snprintf(record, sizeof record, "%d win%s: it first won in round %d. Now %s.", e->wins,
                 e->wins == 1 ? "" : "s", e->cycle, st);
    }
    snprintf(more, sizeof more, "%d more of its bloodline on the ladder.", kin);
    float tw = r.w - 2 * CARD_IN - side - 18;
    float text_h = 34 + 26 + para_h(fo->small, plan, tw) + 8 + para_h(fo->body, record, tw) + 6 +
                   (kin ? para_h(fo->small, more, tw) : 0);
    float h = CARD_TOP + fmaxf(side, text_h) + CARD_IN;
    float cy = card(ui_rect(r.x, r.y, r.w, h), "REIGNING CHAMPION", "the highest on the ladder");
    snprintf(s, sizeof s, "%s, the reigning champion: click for the laboratory", e->name);
    thumb(a, ui_rect(r.x + CARD_IN, cy, side, side), e->gc, ui_id("arena-champion", e->uid), s, e->name);
    float tx = r.x + CARD_IN + side + 18, ty = cy;
    skin_text(fo->heading, skin_fit(fo->heading, e->name, tw, fit, sizeof fit), tx, ty, TONE_TEXT, ALIGN_LEFT);
    skin_text(fo->bold, level, tx, ty + 34, TONE_ACCENT, ALIGN_LEFT);
    ty = para(fo->small, plan, tx, ty + 60, tw, TONE_MUTED) + 8;
    ty = para(fo->body, record, tx, ty, tw, TONE_TEXT) + 6;
    if (kin)
        para(fo->small, more, tx, ty, tw, TONE_MUTED);
    return h;
}

enum { C_WORLD, C_LENGTH, C_SUDDEN, C_ROUNDS, C_POWER, C_SEATS, C_WILD, C_COUNT };

static void setup_view(App *a, UIRect area, bool running)
{
    ArenaSettings *st = syn_arena_settings();
    const Fonts *fo = synos_fonts();
    bool on = !running;
    char n1[24], n2[24];
    Choice c[C_COUNT] = {
        {.title = "THE WORLD",
         .question = "where each battle takes place",
         .labels = {"SMALL", "MEDIUM", "LARGE", "MY WORLD"},
         .n = 4},
        {.title = "ROUND LENGTH",
         .question = "how long each battle lasts",
         .labels = {"5 LIFETIMES", "20 LIFETIMES", "50 LIFETIMES", "100 LIFETIMES"},
         .shorts = {"5 LIVES", "20 LIVES", "50 LIVES", "100 LIVES"},
         .n = 4},
        {.title = "SUDDEN DEATH", .question = "does a round end when one bloodline is left?", .labels = {"ON", "OFF"},
         .n = 2},
        {.title = "ROUNDS",
         .question = "when the arena stops by itself",
         .labels = {"10", "100", "1,000", "NONSTOP"},
         .n = 4},
        {.title = "BATTLES AT ONCE", .question = "how much of your computer to use"},
        {.title = "FIGHTERS PER ROUND",
         .question = "how many winners of a level meet",
         .labels = {"DUELS", "FOUR-WAY", "EIGHT-WAY"},
         .shorts = {"2", "4", "8"},
         .n = 3},
        {.title = "NEWCOMERS",
         .question = "do brand-new beings join the winners' rounds?",
         .labels = {"NONE", "10%", "25%", "50%"},
         .n = 4},
    };
    for (int i = 0; i < C_COUNT; i++)
        c[i].sel = -1;
    static const int maps[] = {MAP_SMALL, MAP_MEDIUM, MAP_LARGE, MAP_PARAMS};
    /* a round's length in the longest a being lives */
    static const int lives[] = {5, 20, 50, 100};
    int life = P.max_age > 0 ? P.max_age : 1;
    static const int target[] = {10, 100, 1000, 0};
    static const int seats[] = {2, 4, 8};
    static const int wild[] = {0, 10, 25, 50};

    /* what each choice means as it stands */
    int ww, wh, orgs;
    syn_arena_world(&ww, &wh, &orgs);
    fmt_num(n1, sizeof n1, orgs);
    for (int i = 0; i < 4; i++)
        if (st->map == maps[i])
            c[C_WORLD].sel = i;
    if (st->map == MAP_PARAMS)
        snprintf(c[C_WORLD].says, sizeof c[C_WORLD].says,
                 "Your own world, as Parameters sets it: %d × %d, with %s beings at the start.%s", ww, wh, n1,
                 orgs > 3000 ? " A world this full makes slow battles: a round can take many minutes, even hours."
                             : "");
    else
        snprintf(c[C_WORLD].says, sizeof c[C_WORLD].says, "A %s world: %d × %d, with %s beings at the start.%s",
                 world_label(st->map), ww, wh, n1,
                 st->map == MAP_SMALL   ? " The battles are quick."
                 : st->map == MAP_LARGE ? " The battles take longer."
                                        : "");
    for (int i = 0; i < 4; i++)
        if (st->units == lives[i] * life)
            c[C_LENGTH].sel = i;
    {
        char many[40];
        double n = (double)st->units / life;
        if (st->units % life == 0)
            snprintf(many, sizeof many, "%d lifetime%s", st->units / life, st->units == life ? "" : "s");
        else if (n < 1)
            snprintf(many, sizeof many, "less than a lifetime");
        else
            snprintf(many, sizeof many, "about %.0f lifetimes", n);
        fmt_num(n1, sizeof n1, st->units);
        char *ls = c[C_LENGTH].says;
        size_t ln = sizeof c[C_LENGTH].says;
        snprintf(ls, ln, "Each round lasts %s: %s time units (256 frames = 1 time unit)%s. ", many, n1,
                 st->knockout ? ", or less if sudden death ends it" : "");
#ifdef BIO_COLOUR
        snprintf(ls + strlen(ls), ln - strlen(ls),
                 "A being here lives %d time units, and 1 more for every %d of its segments.", life,
                 P.age_divisor > 0 ? P.age_divisor : 1);
#else
        snprintf(ls + strlen(ls), ln - strlen(ls),
                 "A being here lives at most %d time units (Life expectancy, in Parameters).", life);
#endif
        double secs = syn_arena_round_seconds();
        if (secs > 0) {
            span_text(n1, sizeof n1, secs);
            snprintf(ls + strlen(ls), ln - strlen(ls), " Rounds like these have taken about %s each here.", n1);
        }
    }
    c[C_SUDDEN].sel = st->knockout ? 0 : 1;
    snprintf(c[C_SUDDEN].says, sizeof c[C_SUDDEN].says, "%s",
             st->knockout ? "A round ends the moment only one bloodline is left alive: the last one standing wins, "
                            "and the next round starts sooner."
                          : "Every round runs its full length; the bloodline with the most beings alive at the end "
                            "wins.");
    for (int i = 0; i < 4; i++)
        if (st->target == target[i])
            c[C_ROUNDS].sel = i;
    fmt_num(n1, sizeof n1, st->target);
    if (st->target)
        snprintf(c[C_ROUNDS].says, sizeof c[C_ROUNDS].says,
                 "The arena stops by itself after %s rounds; STOP ends it sooner. It carries on with this screen "
                 "closed.",
                 n1);
    else
        snprintf(c[C_ROUNDS].says, sizeof c[C_ROUNDS].says,
                 "Rounds go on until you press STOP, even with this screen closed.");
    /* one, half, all but one, all of the cores, as many of those as differ */
    int cores = syn_arena_cores(), power[4];
    const int want[4] = {1, cores / 2, cores - 1, cores};
    static const char *const words[4] = {"ONE", "HALF", "MOST", "ALL"};
    for (int i = 0; i < 4; i++) {
        if (want[i] < 1 || (c[C_POWER].n && want[i] <= power[c[C_POWER].n - 1]))
            continue;
        power[c[C_POWER].n] = want[i];
        c[C_POWER].labels[c[C_POWER].n++] = want[i] == cores ? "ALL" : words[i];
    }
    for (int i = 0; i < c[C_POWER].n; i++)
        if (st->workers == power[i])
            c[C_POWER].sel = i;
    char *ps = c[C_POWER].says;
    size_t pn = sizeof c[C_POWER].says;
    if (st->workers == 1)
        snprintf(ps, pn, "One battle at a time, on 1 of your computer's %d cores.", cores);
    else
        snprintf(ps, pn, "%d battles side by side, one on each of %d of your computer's %d cores.", st->workers,
                 st->workers, cores);
    snprintf(ps + strlen(ps), pn - strlen(ps), " They run at low priority, so the computer stays usable.");
    for (int i = 0; i < 3; i++)
        if (st->seats == seats[i])
            c[C_SEATS].sel = i;
    if (st->seats == 2)
        snprintf(c[C_SEATS].says, sizeof c[C_SEATS].says,
                 "Two beings of the same level share a world. The one whose bloodline wins goes up a level, and "
                 "the other is knocked out: a being on level 5 has won 4 duels in a row, each against another "
                 "winner.");
    else if (st->seats > 2)
        snprintf(c[C_SEATS].says, sizeof c[C_SEATS].says,
                 "%d beings of the same level share a world. The one whose bloodline wins goes up a level, and "
                 "the other %d are knocked out: fewer climb, and each level is harder won.",
                 st->seats, st->seats - 1);
    else
        snprintf(c[C_SEATS].says, sizeof c[C_SEATS].says,
                 "Qualifiers only: every round is newcomers, and its winner waits on level 1.");
    for (int i = 0; i < 4; i++)
        if (st->wild_pct == wild[i])
            c[C_WILD].sel = i;
    {
        int newcomers, copies;
        field(orgs, st->seats >= 2 ? st->seats : 2, st->wild_pct, &newcomers, &copies);
        fmt_num(n1, sizeof n1, newcomers);
        fmt_num(n2, sizeof n2, copies);
        if (st->wild_pct == 0)
            snprintf(c[C_WILD].says, sizeof c[C_WILD].says,
                     "Winners only: each fighter starts as %s cop%s of itself. Newcomers only fight in the "
                     "qualifiers.",
                     n2, copies == 1 ? "y" : "ies");
        else
            snprintf(c[C_WILD].says, sizeof c[C_WILD].says,
                     "Each winners' round starts with %s newcomers beside %s cop%s of each fighter. If a "
                     "newcomer's bloodline wins, it jumps up a level and knocks the fighters out.",
                     n1, n2, copies == 1 ? "y" : "ies");
    }

    const char *intro = running ? "The battles are running: STOP them to change the setup."
                                : "A ladder that winners climb. A qualifier is a world of brand-new beings, and the "
                                  "bloodline that wins it goes on the ladder at level 1. From then on winners "
                                  "only meet winners: beings of the same level share a world, the winner goes "
                                  "up a level and the rest are knocked out. The worlds run far faster than you "
                                  "could watch, several at once.";
    int k[C_COUNT];
    for (int i = 0; i < C_COUNT; i++)
        k[i] = -1;
    if (area.w >= 1500 && area.h >= 640) {
        /* wide: the world and the rounds, then the champion, on the left;
         * the ladder, its choices over it and the newest beings under it,
         * in the middle; the leaderboard on the right */
        float c3 = fmaxf(420, floorf(area.w * 0.3f)), c1 = floorf((area.w - c3 - 2 * GAP) / 2),
              c2 = area.w - c3 - c1 - 2 * GAP;
        float y = para(fo->body, intro, area.x, area.y, area.w, running ? TONE_ACCENT : TONE_MUTED) + 10;
        float lx = area.x, mx = lx + c1 + GAP, rx = mx + c2 + GAP, ly = y, my = y;
        float bottom = area.y + area.h;
        for (int i = C_WORLD; i <= C_POWER; i++) {
            choice_measure(&c[i], c1);
            k[i] = choice_card(&c[i], lx, ly, c1, on);
            ly += c[i].h + GAP;
        }
        if (bottom - ly >= 160)
            champion(a, ui_rect(lx, ly, c1, fminf(bottom - ly, 300)));
        for (int i = C_SEATS; i <= C_WILD; i++) {
            choice_measure(&c[i], c2);
            k[i] = choice_card(&c[i], mx, my, c2, on);
            my += c[i].h + GAP;
        }
        float feed_h = 150, lad_h = bottom - my - (bottom - my >= 360 ? feed_h + GAP : 0);
        if (lad_h >= 120)
            ladder(a, ui_rect(mx, my + 6, c2, lad_h - 6));
        if (bottom - my >= 360)
            feed(a, ui_rect(mx, bottom - feed_h, c2, feed_h));
        leaderboard(a, ui_rect(rx, y, c3, bottom - y));
    } else {
        /* the world and the rounds on the left, the ladder's choices and the
         * computer on the right; the welcome above if there is room, and
         * ROUNDS under the computer if the left is still too tall */
        float cw = floorf((area.w - GAP) / 2), rw = area.w - cw - GAP;
        for (int i = 0; i < C_COUNT; i++)
            choice_measure(&c[i], i >= C_POWER ? rw : cw);
        float intro_h = para_h(fo->body, intro, area.w) + 10;
        float left_h = c[C_WORLD].h + c[C_LENGTH].h + c[C_SUDDEN].h + c[C_ROUNDS].h + 3 * GAP;
        float right_h = c[C_SEATS].h + c[C_WILD].h + c[C_POWER].h + 2 * GAP;
        bool welcome = intro_h + fmaxf(left_h, right_h) <= area.h;
        /* running, the note that the setup waits is kept if moving ROUNDS
         * makes room for it */
        if (running && !welcome)
            welcome = intro_h + fmaxf(left_h - c[C_ROUNDS].h - GAP, right_h + GAP + c[C_ROUNDS].h) <= area.h;
        float avail = area.h - (welcome ? intro_h : 0);
        bool rounds_right = left_h > avail && right_h + GAP + c[C_ROUNDS].h <= avail;
        float y = area.y;
        if (welcome)
            y = para(fo->body, intro, area.x, y, area.w, running ? TONE_ACCENT : TONE_MUTED) + 10;
        float lx = area.x, rx = area.x + cw + GAP, ly = y, ry = y;
        for (int i = C_WORLD; i <= C_ROUNDS; i++) {
            if (i == C_ROUNDS && rounds_right)
                continue;
            k[i] = choice_card(&c[i], lx, ly, cw, on);
            ly += c[i].h + GAP;
        }
        static const int right[] = {C_SEATS, C_WILD, C_POWER, C_ROUNDS};
        for (int j = 0; j < 4; j++) {
            if (right[j] == C_ROUNDS && !rounds_right)
                continue;
            k[right[j]] = choice_card(&c[right[j]], rx, ry, rw, on);
            ry += c[right[j]].h + GAP;
        }
    }

    if (k[C_WORLD] >= 0)
        st->map = maps[k[C_WORLD]];
    if (k[C_LENGTH] >= 0)
        st->units = lives[k[C_LENGTH]] * life;
    if (k[C_SUDDEN] >= 0)
        st->knockout = k[C_SUDDEN] == 0;
    if (k[C_ROUNDS] >= 0)
        st->target = target[k[C_ROUNDS]];
    if (k[C_POWER] >= 0)
        st->workers = power[k[C_POWER]];
    if (k[C_SEATS] >= 0)
        st->seats = seats[k[C_SEATS]];
    if (k[C_WILD] >= 0)
        st->wild_pct = wild[k[C_WILD]];
    bool changed = false;
    for (int i = 0; i < C_COUNT; i++)
        changed |= k[i] >= 0;
    if (changed)
        syn_arena_settings_changed();
}

/* ---- the leaderboard ---- */

static void leaderboard(App *a, UIRect r)
{
    const Fonts *fo = synos_fonts();
    int n = syn_arena_count();
    char s[480], fit[480], st[120];
    skin_text(fo->heading, "LEADERBOARD", r.x, r.y, TONE_TEXT, ALIGN_LEFT);
    int others = syn_arena_others();
    snprintf(s, sizeof s, "%d for this setup%s", n, others ? " · more from other setups" : "");
    skin_text(fo->small, s, r.x + r.w, r.y + 6, TONE_MUTED, ALIGN_RIGHT);
    float hy = r.y + 32;
    /* three columns, as wide as their headings */
    float cw_l = skin_text_width(fo->small_bold, "LEVEL") + 16, cw_w = skin_text_width(fo->small_bold, "WINS") + 16,
          cw_f = skin_text_width(fo->small_bold, "FOUGHT") + 12;
    float c_fought = r.x + r.w - 6, c_wins = c_fought - cw_f, c_level = c_wins - cw_w;
    skin_text(fo->small_bold, "LEVEL", c_level, hy, TONE_MUTED, ALIGN_RIGHT);
    skin_text(fo->small_bold, "WINS", c_wins, hy, TONE_MUTED, ALIGN_RIGHT);
    skin_text(fo->small_bold, "FOUGHT", c_fought, hy, TONE_MUTED, ALIGN_RIGHT);
    if (over(ui_rect(c_level - cw_l, hy, c_fought - c_level + cw_l, 18)))
        ui_tooltip("LEVEL: its rung on the ladder · WINS: rounds its bloodline won · FOUGHT: winners' rounds it "
                   "fought in");
    skin_separator(r.x, hy + 20, r.w);
    UIRect list = {r.x, hy + 24, r.w, r.y + r.h - hy - 24};
    if (n == 0) {
        skin_text(fo->body, "Every winner comes here.", list.x + 4, list.y + 10, TONE_MUTED, ALIGN_LEFT);
        skin_text(fo->small, "The highest on the ladder first.", list.x + 4, list.y + 34, TONE_MUTED, ALIGN_LEFT);
        return;
    }
    float rh = 50;
    float total = rh * (float)n, maxs = fmaxf(0, total - list.h);
    if (over(list))
        S.scroll -= ui_wheel_fy() * rh;
    S.scroll = fminf(fmaxf(S.scroll, 0), maxs);
    gfx_clip_push(sdl(list));
    int first = (int)(S.scroll / rh);
    for (int i = first; i < n; i++) {
        float y = list.y + rh * (float)i - S.scroll;
        if (y > list.y + list.h)
            break;
        const ArenaEntry *e = syn_arena_at(i);
        bool in = e->state == ENTRY_IN, fighting = syn_arena_in_round(e->uid);
        UIRect row = {list.x, y, list.w, rh};
        if (over(row) && over(list))
            skin_row_hover(row);
        snprintf(s, sizeof s, "%d", i + 1);
        skin_text(fo->bold, s, row.x + 26, y + 14, i < 3 && in ? TONE_ACCENT : TONE_MUTED, ALIGN_RIGHT);
        /* in the running: a bar, bright while it fights */
        if (in)
            skin_fill(ui_rect(row.x + 30, y + 8, 3, rh - 16), skin_theme()->accent, fighting ? 1.0f : 0.4f);
        UIRect th = {row.x + 38, y + 5, rh - 10, rh - 10};
        const ArenaEntry *par = syn_arena_find(e->parent);
        syn_arena_standing(e, st, sizeof st);
        snprintf(s, sizeof s, "%s, level %d: %s. First won round %d%s%s", e->name, e->level,
                 fighting ? "fighting now" : st, e->cycle, par ? ", as the offspring of " : "", par ? par->name : "");
        thumb(a, th, e->gc, ui_id("arena-entry", e->uid), s, e->name);
        float tx = th.x + th.w + 10, tw = c_level - cw_l - tx;
        skin_text(fo->bold, skin_fit(fo->bold, e->name, tw, fit, sizeof fit), tx, y + 6, in ? TONE_TEXT : TONE_MUTED,
                  ALIGN_LEFT);
        snprintf(s, sizeof s, "%s · %s bloodline · %s", fighting ? "fighting now" : st, e->line, e->plan);
        skin_text(fo->small, skin_fit(fo->small, s, tw, fit, sizeof fit), tx, y + 27, TONE_MUTED, ALIGN_LEFT);
        snprintf(s, sizeof s, "%d", e->level);
        skin_text(fo->bold, s, c_level, y + 14, in ? TONE_TEXT : TONE_MUTED, ALIGN_RIGHT);
        snprintf(s, sizeof s, "%d", e->wins);
        skin_text(fo->body, s, c_wins, y + 14, TONE_MUTED, ALIGN_RIGHT);
        snprintf(s, sizeof s, "%d", e->entered);
        skin_text(fo->body, s, c_fought, y + 14, TONE_MUTED, ALIGN_RIGHT);
    }
    gfx_clip_pop();
}

/* ---- the ladder ---- */

/* One rung: the beings in the running on this level, lit while they
 * fight, with how many wait and fight. */
static void rung(App *a, UIRect row, int level, bool top)
{
    const Fonts *fo = synos_fonts();
    char s[200], label[32];
    if (top)
        skin_fill(row, skin_theme()->accent, 0.08f);
    snprintf(label, sizeof label, "LEVEL %d", level);
    float lw = fmaxf(96, skin_text_width(fo->bold, "LEVEL 88") + 16);
    skin_text(fo->bold, label, row.x + 8, row.y + (row.h - skin_text_height(fo->bold)) / 2,
              top ? TONE_ACCENT : TONE_TEXT, ALIGN_LEFT);
    int n = syn_arena_count(), waiting = 0, fighting = 0;
    for (int i = 0; i < n; i++) {
        const ArenaEntry *e = syn_arena_at(i);
        if (e->state != ENTRY_IN || e->level != level)
            continue;
        if (syn_arena_in_round(e->uid))
            fighting++;
        else
            waiting++;
    }
    char counts[64];
    if (fighting && waiting)
        snprintf(counts, sizeof counts, "%d waiting · %d fighting", waiting, fighting);
    else if (fighting)
        snprintf(counts, sizeof counts, "%d fighting", fighting);
    else if (waiting)
        snprintf(counts, sizeof counts, "%d waiting", waiting);
    else
        snprintf(counts, sizeof counts, "empty: its winners went higher");
    float cw = skin_text_width(fo->small, counts);
    skin_text(fo->small, counts, row.x + row.w - 8, row.y + (row.h - skin_text_height(fo->small)) / 2, TONE_MUTED,
              ALIGN_RIGHT);
    /* the beings: waiting first, then fighting; the champion's name beside
     * it if it is alone at the top */
    float side = row.h - 8, x = row.x + lw, right = row.x + row.w - cw - 20;
    int shown = 0, all = waiting + fighting;
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < n; i++) {
            const ArenaEntry *e = syn_arena_at(i);
            if (e->state != ENTRY_IN || e->level != level)
                continue;
            bool f = syn_arena_in_round(e->uid);
            if (f != (pass == 1))
                continue;
            if (x + side > right - (shown + 1 < all ? 40 : 0))
                continue;
            UIRect th = {x, row.y + 4, side, side};
            snprintf(s, sizeof s, "%s, level %d, %d win%s in a row: %s. Click for the laboratory", e->name, level,
                     e->wins, e->wins == 1 ? "" : "s", f ? "fighting now" : "waiting for a challenger");
            thumb(a, th, e->gc, ui_id("arena-rung", e->uid), s, e->name);
            if (f)
                skin_fill(ui_rect(th.x, th.y + th.h - 3, th.w, 3), skin_theme()->accent, 1);
            x += side + 6;
            shown++;
            if (top && all == 1) {
                char fit[64];
                skin_text(fo->bold, skin_fit(fo->bold, e->name, right - x - 6, fit, sizeof fit), x + 6,
                          row.y + row.h / 2 - 17, TONE_TEXT, ALIGN_LEFT);
                skin_text(fo->small, "CHAMPION", x + 6, row.y + row.h / 2 + 2, TONE_ACCENT, ALIGN_LEFT);
            }
        }
    if (shown < all) {
        snprintf(s, sizeof s, "+%d", all - shown);
        skin_text(fo->small_bold, s, x + 2, row.y + (row.h - skin_text_height(fo->small_bold)) / 2, TONE_MUTED,
                  ALIGN_LEFT);
    }
}

/* The ladder as it stands: a rung for each level up to the highest, those
 * in the running on it, and the qualifiers under them. */
static void ladder(App *a, UIRect r)
{
    const Fonts *fo = synos_fonts();
    int top = syn_arena_top_level(), n = syn_arena_count(), in = 0;
    for (int i = 0; i < n; i++)
        in += syn_arena_at(i)->state == ENTRY_IN;
    float hw = skin_text(fo->heading, "THE LADDER", r.x, r.y, TONE_TEXT, ALIGN_LEFT);
    char s[200], fit[200];
    if (top)
        snprintf(s, sizeof s, "top level %d · %d in the running", top, in);
    else
        snprintf(s, sizeof s, "empty until a qualifier is won");
    skin_text(fo->small, skin_fit(fo->small, s, r.w - hw - 12, fit, sizeof fit), r.x + r.w, r.y + 6, TONE_MUTED,
              ALIGN_RIGHT);
    float y = r.y + 32, bottom = r.y + r.h;
    /* a rung each, the highest first, as tall as there is room for; when
     * even short rungs don't fit, the lowest share one */
    int rows = top + 1;
    float rh = fminf(64, floorf((bottom - y) / (float)(rows > 0 ? rows : 1)));
    int own = top;
    if (rh < 34) {
        rh = 34;
        own = (int)((bottom - y) / rh) - 2;
        if (own < 1)
            own = 1;
        if (own > top)
            own = top;
    }
    for (int lv = top; lv > top - own && lv >= 1; lv--) {
        if (y + rh > bottom)
            break;
        rung(a, ui_rect(r.x, y, r.w, rh - 2), lv, lv == top);
        skin_separator(r.x, y + rh - 1, r.w);
        y += rh;
    }
    if (own < top && y + rh <= bottom) {
        int low = top - own, k = 0;
        for (int i = 0; i < n; i++) {
            const ArenaEntry *e = syn_arena_at(i);
            k += e->state == ENTRY_IN && e->level <= low;
        }
        snprintf(s, sizeof s, low == 1 ? "LEVEL 1" : "LEVELS 1-%d", low);
        skin_text(fo->bold, s, r.x + 8, y + (rh - skin_text_height(fo->bold)) / 2, TONE_TEXT, ALIGN_LEFT);
        snprintf(s, sizeof s, "%d in the running", k);
        skin_text(fo->small, s, r.x + r.w - 8, y + (rh - skin_text_height(fo->small)) / 2, TONE_MUTED, ALIGN_RIGHT);
        skin_separator(r.x, y + rh - 1, r.w);
        y += rh;
    }
    if (y + 30 > bottom)
        return;
    /* the qualifiers: newcomers only */
    int quals = 0;
    for (int i = 0; i < syn_arena_lane_count(); i++) {
        const ArenaLane *L = syn_arena_lane(i);
        quals += L->busy && L->level == 0;
    }
    float qh = fminf(rh, bottom - y);
    skin_text(fo->bold, "QUALIFIERS", r.x + 8, y + (qh - skin_text_height(fo->bold)) / 2, TONE_MUTED, ALIGN_LEFT);
    if (quals)
        snprintf(s, sizeof s, "newcomers only · %d running now · winners start on level 1", quals);
    else
        snprintf(s, sizeof s, "newcomers only · winners start on level 1");
    float lw = fmaxf(96, skin_text_width(fo->bold, "QUALIFIERS") + 16);
    skin_text(fo->small, skin_fit(fo->small, s, r.w - lw - 8, fit, sizeof fit), r.x + r.w - 8,
              y + (qh - skin_text_height(fo->small)) / 2, TONE_MUTED, ALIGN_RIGHT);
}

/* ---- the climb: each round's winner by the level it went up to ---- */

/* the level a round's winner went up to (no survivors: the round's own) */
static int reached(const ArenaResult *res)
{
    return res->outcome == OUT_EXTINCT ? res->level : res->level + 1;
}

static void climb(UIRect r)
{
    const Fonts *fo = synos_fonts();
    float head = skin_text(fo->heading, "THE CLIMB", r.x, r.y, TONE_TEXT, ALIGN_LEFT);
    /* the legend: what the dots' colours are; under the title if it
     * doesn't fit beside it */
    float lw = 0;
    for (int o = 0; o < OUT_KINDS; o++)
        lw += skin_text_width(fo->small, outcome_label(o)) + 26;
    bool below = head + 16 + lw > r.w;
    float lx = r.x + r.w, ly = below ? r.y + 28 : r.y + 6;
    for (int o = OUT_KINDS - 1; o >= 0; o--) {
        const char *nm = outcome_label(o);
        float w = skin_text_width(fo->small, nm);
        if (lx - w - 13 < r.x)
            break;
        skin_text(fo->small, nm, lx, ly, TONE_MUTED, ALIGN_RIGHT);
        SDL_Color c = outcome(o, 255);
        skin_fill(ui_rect(lx - w - 13, ly + 4, 8, 8), (Rgb){c.r, c.g, c.b}, 1);
        lx -= w + 26;
    }
    float top = (below ? ly + 20 : r.y + 30) + 10;
    UIRect pl = {r.x + 40, top, r.w - 48, r.y + r.h - top - 10};
    if (pl.h < 30)
        return;
    int n = syn_arena_result_count(), most = 2;
    for (int i = 0; i < n; i++)
        most = reached(syn_arena_result(i)) > most ? reached(syn_arena_result(i)) : most;
    /* the levels, every one or every few: 0 at the bottom */
    int every = most <= 6 ? 1 : (most + 5) / 6;
    for (int g = 0; g <= most; g += every) {
        float y = pl.y + pl.h - pl.h * (float)g / (float)most;
        skin_fill(ui_rect(pl.x, y, pl.w, 1), skin_theme()->border, 0.35f);
        char s[16];
        snprintf(s, sizeof s, g ? "%d" : "Q", g);
        skin_text(fo->small, s, pl.x - 6, y - 8, TONE_MUTED, ALIGN_RIGHT);
    }
    if (over(ui_rect(r.x, pl.y, 40, pl.h)))
        ui_tooltip("the level each round's winner went up to: 1 for a qualifier's, one more than the round's "
                   "level for a winners' round");
    if (n == 0) {
        skin_text(fo->small, "each round's winner, by the level it went up to", pl.x + 8, pl.y + 8, TONE_MUTED,
                  ALIGN_LEFT);
        return;
    }
    /* the newest that fit, 5 px apart at most */
    int show = (int)(pl.w / 5);
    if (show > n)
        show = n;
    int from = n - show;
    float dx = show > 1 ? fminf(pl.w / (float)(show - 1), 24) : 0;
    int hover = -1;
    float hd = 1e9f;
    for (int i = from; i < n; i++) {
        float x = pl.x + dx * (float)(i - from);
        float d = fabsf((float)g_mx - x);
        if (over(pl) && d < hd && d < 12) {
            hd = d;
            hover = i;
        }
    }
    /* the highest level reached so far, as a step, under the dots */
    int high = 0;
    for (int i = 0; i < from; i++)
        high = reached(syn_arena_result(i)) > high ? reached(syn_arena_result(i)) : high;
    SDL_Color line = tone(TONE_TEXT, 110);
    batch_begin(&S.b, TX_DISC, SDL_BLENDMODE_BLEND);
    float px = pl.x, py = pl.y + pl.h - pl.h * (float)high / (float)most;
    for (int i = from; i < n; i++) {
        int h = reached(syn_arena_result(i));
        float x = pl.x + dx * (float)(i - from);
        if (h > high) {
            batch_capsule(&S.b, px, py, x, py, 1, line);
            float ny = pl.y + pl.h - pl.h * (float)h / (float)most;
            batch_capsule(&S.b, x, py, x, ny, 1, line);
            px = x;
            py = ny;
            high = h;
        }
    }
    batch_capsule(&S.b, px, py, pl.x + dx * (float)(n - 1 - from), py, 1, line);
    for (int i = from; i < n; i++) {
        const ArenaResult *res = syn_arena_result(i);
        float x = pl.x + dx * (float)(i - from), y = pl.y + pl.h - pl.h * (float)reached(res) / (float)most;
        float rad = i == hover ? 6.0f : 4.0f;
        /* a ring of the surface around each, so overlapping dots stay apart */
        Rgb bg = skin_theme()->bg;
        batch_sprite(&S.b, x, y, rad + 2, 0, (SDL_Color){bg.r, bg.g, bg.b, 255});
        batch_sprite(&S.b, x, y, rad, 0, outcome(res->outcome, 255));
    }
    batch_flush(&S.b, gfx_renderer());
    if (hover >= 0) {
        const ArenaResult *res = syn_arena_result(hover);
        S.ntip = 0;
        if (res->level)
            tip_line((SDL_Color){0, 0, 0, 0}, "round %d, level %d, ended at time %d%s", res->cycle, res->level,
                     res->t_end, res->knockout ? " by sudden death" : "");
        else
            tip_line((SDL_Color){0, 0, 0, 0}, "round %d, a qualifier, ended at time %d%s", res->cycle, res->t_end,
                     res->knockout ? " by sudden death" : "");
        SDL_Color c = outcome(res->outcome, 255);
        if (res->outcome == OUT_EXTINCT)
            tip_line(c, "no one survived%s", res->level ? ": the fighters are all out" : "");
        else if (res->outcome == OUT_QUALIFIED)
            tip_line(c, "%s won: on the ladder at level 1", res->name);
        else if (res->outcome == OUT_ADVANCED)
            tip_line(c, "%s won: up to level %d", res->name, res->level + 1);
        else if (res->outcome == OUT_EVOLVED)
            tip_line(c, "%s, a fighter's offspring, won: up to level %d", res->name, res->level + 1);
        else
            tip_line(c, "%s, a newcomer, beat them all: up to level %d", res->name, res->level + 1);
        if (res->nout)
            tip_line((SDL_Color){0, 0, 0, 0}, "%d knocked out", res->nout);
        if (res->outcome != OUT_EXTINCT)
            tip_line((SDL_Color){0, 0, 0, 0}, "its bloodline held %.0f%% of the living", res->share * 100);
        S.tip_x = (float)g_mx;
        S.tip_y = (float)g_my;
    }
}

/* ---- the beings named most recently ---- */

static void feed(App *a, UIRect r)
{
    const Fonts *fo = synos_fonts();
    float hw = skin_text(fo->heading, "NEW BEINGS", r.x, r.y, TONE_TEXT, ALIGN_LEFT);
    skin_text(fo->small, "species born in the battles, newest first", r.x + hw + 12, r.y + 6, TONE_MUTED,
              ALIGN_LEFT);
    float side = fminf(r.h - 32 - 34, 76), tw = side + 24;
    int fits = (int)((r.w + 10) / (tw + 10));
    int n = syn_arena_feed_count();
    char fit[64], s[128];
    for (int i = 0; i < n && i < fits; i++) {
        const ArenaBeing *b = syn_arena_feed(i);
        float x = r.x + (tw + 10) * (float)i, y = r.y + 30;
        snprintf(s, sizeof s, "%s, named in round %d with %d alive", b->name, b->cycle, b->alive);
        thumb(a, ui_rect(x + 12, y, side, side), b->gc, ui_id("arena-feed", (uint32_t)(b->cycle * 131 + i)), s,
              b->name);
        skin_text(fo->small_bold, skin_fit(fo->small_bold, b->name, tw, fit, sizeof fit), x + tw / 2, y + side + 3,
                  TONE_TEXT, ALIGN_CENTER);
        snprintf(s, sizeof s, "round %d", b->cycle);
        skin_text(fo->small, s, x + tw / 2, y + side + 18, TONE_MUTED, ALIGN_CENTER);
    }
    if (n == 0)
        skin_text(fo->small, "new species show here as the battles run", r.x, r.y + 34, TONE_MUTED, ALIGN_LEFT);
}

/* ---- printing ---- */

/* The ladder's highest as a sheet: their bodies, names and levels. */
static bool sheet_png(const char *path)
{
    SDL_Renderer *R = gfx_renderer();
    int n = syn_arena_count();
    if (n > 24)
        n = 24;
    if (!n || !SDL_RenderTargetSupported(R))
        return false;
    const Fonts *fo = synos_fonts();
    const int cols = 6, cw = 200, chh = 236, rows = (n + cols - 1) / cols;
    const int W = cols * cw + 40, H = 110 + rows * chh + 20;
    SDL_Texture *t = SDL_CreateTexture(R, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, W, H);
    if (!t)
        return false;
    SDL_Texture *old = SDL_GetRenderTarget(R);
    SDL_Rect clip;
    SDL_bool clipped = SDL_RenderIsClipEnabled(R);
    SDL_RenderGetClipRect(R, &clip);
    SDL_SetRenderTarget(R, t);
    SDL_RenderSetClipRect(R, NULL);
    SDL_SetRenderDrawColor(R, 16, 17, 20, 255);
    SDL_RenderClear(R);
    float alpha = skin_alpha();
    skin_set_alpha(1);
    int w, h, orgs;
    syn_arena_world(&w, &h, &orgs);
    const ArenaSettings *st = syn_arena_settings();
    char s[200], fit[64];
    skin_text_rgb(fo->big, "ARENA LADDER", 20, 18, 240, 240, 240, ALIGN_LEFT);
    time_t now = time(NULL);
    char when[40];
    strftime(when, sizeof when, "%Y-%m-%d %H:%M", localtime(&now));
    snprintf(s, sizeof s,
             "%s · %s world, %d × %d, %d beings at the start · rounds of %d time units · %d rounds fought · %s",
             SIM_TITLE, world_label(st->map), w, h, orgs, st->units, syn_arena_cycles(), when);
    skin_text_rgb(fo->body, s, 20, 60, 170, 170, 170, ALIGN_LEFT);
    for (int i = 0; i < n; i++) {
        const ArenaEntry *e = syn_arena_at(i);
        float x = 20 + (float)(i % cols) * cw, y = 100 + (float)(i / cols) * chh;
        snprintf(s, sizeof s, "#%d · LEVEL %d", i + 1, e->level);
        skin_text_rgb(fo->bold, s, x, y, 230, 200, 120, ALIGN_LEFT);
        body(ui_rect(x, y + 24, cw - 20, 132), e->gc, 255);
        skin_text_rgb(fo->bold, skin_fit(fo->bold, e->name, cw - 20, fit, sizeof fit), x, y + 162, 240, 240, 240,
                      ALIGN_LEFT);
        snprintf(s, sizeof s, "%d win%s · fought %d · %s", e->wins, e->wins == 1 ? "" : "s", e->entered,
                 e->state == ENTRY_IN ? "in the running" : "out");
        skin_text_rgb(fo->small, skin_fit(fo->small, s, cw - 20, fit, sizeof fit), x, y + 184, 190, 190, 190,
                      ALIGN_LEFT);
        snprintf(s, sizeof s, "%s bloodline · first won round %d", e->line, e->cycle);
        skin_text_rgb(fo->small, skin_fit(fo->small, s, cw - 20, fit, sizeof fit), x, y + 202, 140, 140, 140,
                      ALIGN_LEFT);
    }
    SDL_Surface *sf = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_ARGB8888);
    bool ok = sf && SDL_RenderReadPixels(R, NULL, SDL_PIXELFORMAT_ARGB8888, sf->pixels, sf->pitch) == 0 &&
              IMG_SavePNG(sf, path) == 0;
    SDL_FreeSurface(sf);
    SDL_SetRenderTarget(R, old);
    SDL_RenderSetClipRect(R, clipped ? &clip : NULL);
    SDL_DestroyTexture(t);
    skin_set_alpha(alpha);
    return ok;
}

static void print_winners(void)
{
    char cwd[900], dir[1100], stamp[32];
    if (!getcwd(cwd, sizeof cwd))
        snprintf(cwd, sizeof cwd, ".");
    path_slashes(cwd);
    time_t now = time(NULL);
    strftime(stamp, sizeof stamp, "%Y%m%d-%H%M%S", localtime(&now));
    snprintf(dir, sizeof dir, "%s/arena-winners-%s-%s", cwd, SIM_FILE, stamp);
    char msg[1300];
    int n = syn_arena_print(dir, msg, sizeof msg);
    if (n) {
        char png[1200];
        snprintf(png, sizeof png, "%s/winners.png", dir);
        if (sheet_png(png))
            snprintf(msg + strlen(msg), sizeof msg - strlen(msg), ", with winners.png");
    }
    snprintf(S.status, sizeof S.status, "%s", msg);
    S.status_at = SDL_GetTicks();
}

/* ---- the screen ---- */

static void duration(char *out, size_t n, double s)
{
    int t = (int)s;
    if (t >= 3600)
        snprintf(out, n, "%d:%02d:%02d", t / 3600, t / 60 % 60, t % 60);
    else
        snprintf(out, n, "%d:%02d", t / 60, t % 60);
}

/* "about 12 min", "about 2 h 5 min" */
static void about(char *out, size_t n, double s)
{
    int m = (int)lround(s / 60);
    if (m < 1)
        snprintf(out, n, "under a minute");
    else if (m < 60)
        snprintf(out, n, "about %d min", m);
    else
        snprintf(out, n, "about %d h %d min", m / 60, m % 60);
}

/* How the battles go, in a line: what PRINT WINNERS did for a while, else
 * the run, else what START would do. */
static void status_line(UIRect r, bool running)
{
    const Fonts *fo = synos_fonts();
    const ArenaSettings *st = syn_arena_settings();
    char s[3][400], d[24], left[48];
    int cycles;
    double secs, frames;
    syn_arena_run_stats(&cycles, &secs, &frames);
    duration(d, sizeof d, secs);
    bool note = S.status[0] && SDL_GetTicks() - S.status_at < 8000;
    Tone t = TONE_MUTED;
    if (note) {
        snprintf(s[0], sizeof s[0], "%s", S.status);
        snprintf(s[1], sizeof s[1], "%s", S.status);
        t = TONE_ACCENT;
    } else if (running) {
        left[0] = '\0';
        if (st->target > 0 && cycles > 0 && cycles < st->target) {
            char a[32];
            about(a, sizeof a, (st->target - cycles) * secs / cycles);
            snprintf(left, sizeof left, " · %s left", a);
        }
        char lead[96] = "", per[48] = "";
        int top = syn_arena_top_level();
        if (top)
            snprintf(lead, sizeof lead, " · top level %d", top);
        if (syn_arena_round_seconds() > 0) {
            char a[24];
            span_text(a, sizeof a, syn_arena_round_seconds());
            snprintf(per, sizeof per, " · about %s a round", a);
        }
        if (st->target > 0)
            snprintf(s[0], sizeof s[0], "BATTLES RUNNING · %d of %d rounds fought%s · %s%s%s", cycles, st->target,
                     lead, d, per, left);
        else
            snprintf(s[0], sizeof s[0], "BATTLES RUNNING · %d round%s fought%s · %s%s · until you press STOP", cycles,
                     cycles == 1 ? "" : "s", lead, d, per);
        snprintf(s[1], sizeof s[1], "RUNNING · %d round%s%s · %s%s", cycles, cycles == 1 ? "" : "s", lead, d, left);
        t = TONE_ACCENT;
    } else if (cycles) {
        snprintf(s[0], sizeof s[0], "Stopped after %d round%s in %s", cycles, cycles == 1 ? "" : "s", d);
        snprintf(s[1], sizeof s[1], "%d round%s in %s", cycles, cycles == 1 ? "" : "s", d);
    } else {
        int n = syn_arena_cycles();
        snprintf(s[0], sizeof s[0], "%d round%s fought here so far · set up the battles, then START", n,
                 n == 1 ? "" : "s");
        snprintf(s[1], sizeof s[1], "%d round%s fought so far", n, n == 1 ? "" : "s");
    }
    snprintf(s[2], sizeof s[2], "%s", s[1]);
    char fit[400];
    for (int k = 0; k < 3; k++)
        if (k == 2 || skin_text_width(fo->body, s[k]) <= r.w) {
            skin_text(fo->body, k == 2 ? skin_fit(fo->body, s[k], r.w, fit, sizeof fit) : s[k], r.x,
                      r.y + (r.h - skin_text_height(fo->body)) / 2, t, ALIGN_LEFT);
            break;
        }
}

/* The battles as they run on the left, the newest beings under them; the
 * ladder and the climb on the right. */
static void battles_view(App *a, UIRect area, bool running)
{
    const Fonts *fo = synos_fonts();
    float rw = fmaxf(400, floorf(area.w * 0.36f)), lw = area.w - rw - 2 * GAP, rx = area.x + area.w - rw;
    float feed_h = 150, bottom = area.y + area.h;
    UIRect lanes = {area.x, area.y, lw, area.h - feed_h - GAP};
    int nl = syn_arena_lane_count();
    if (!running) {
        int cycles;
        double secs, frames;
        syn_arena_run_stats(&cycles, &secs, &frames);
        float ty = lanes.y + 20;
        skin_text(fo->heading, cycles ? "THE BATTLES ARE OVER" : "NO BATTLES RUNNING", lanes.x, ty, TONE_TEXT,
                  ALIGN_LEFT);
        para(fo->body,
             "START BATTLES runs them, each in a card here: a qualifier or a level, the bloodlines' shares of the "
             "living as the round goes (the fighters in their colours, at the bottom), and the being ahead. "
             "Hover a chart to read it; click a being to open it in the laboratory, or right click to copy its "
             "code.",
             lanes.x, ty + 36, lanes.w - 20, TONE_MUTED);
    } else {
        int cols = nl <= 1 ? 1 : nl <= 4 ? 2 : nl <= 9 ? 3 : 4;
        int rows = (nl + cols - 1) / cols;
        for (int i = 0; i < nl; i++) {
            UIRect r = ui_grid_cell(lanes, cols, rows, i % cols, i / cols, 10, 10);
            lane_card(a, r, i, syn_arena_lane(i));
        }
    }
    feed(a, ui_rect(lanes.x, bottom - feed_h, lw, feed_h));
    float lad_h = floorf(area.h * 0.58f);
    ladder(a, ui_rect(rx, area.y, rw, lad_h));
    climb(ui_rect(rx, area.y + lad_h + GAP, rw, area.h - lad_h - GAP));
}

/* ---- the tournament tree ---- */

/* a being in the tree: the one that knocked it out (or went up in its
 * place) is its parent; its children are those it beat */
typedef struct {
    const ArenaEntry *e;
    int parent;       /* -1: a root, in the running or out with no one above */
    int kid0, nkids;  /* its children, in Tree.kids */
    int col;
} TNode;

static struct {
    TNode *t;
    int *kids, *byuid;
    int cap;
} Tree;

static const TNode *g_tnodes;

static int cmp_uid(const void *a, const void *b)
{
    uint32_t x = g_tnodes[*(const int *)a].e->uid, y = g_tnodes[*(const int *)b].e->uid;
    return (x > y) - (x < y);
}

/* children: those beaten at the highest level first, so that each one's
 * branch passes over the lower ones beside the winner */
static int cmp_kids(const void *a, const void *b)
{
    const TNode *x = &g_tnodes[*(const int *)a], *y = &g_tnodes[*(const int *)b];
    if (x->parent != y->parent)
        return x->parent - y->parent;
    if (x->e->level != y->e->level)
        return y->e->level - x->e->level;
    return x->e->last - y->e->last;
}

/* columns from col, its children's first: the next free one */
static int tree_lay(int x, int col)
{
    TNode *t = Tree.t;
    for (int k = 0; k < t[x].nkids; k++)
        col = tree_lay(Tree.kids[t[x].kid0 + k], col);
    t[x].col = col;
    return col + 1;
}

static int tree_cols(int x)
{
    int n = 1;
    for (int k = 0; k < Tree.t[x].nkids; k++)
        n += tree_cols(Tree.kids[Tree.t[x].kid0 + k]);
    return n;
}

/* Every being on the ladder with these settings, joined to the one that
 * knocked it out: the levels up the side, each being a line from the level
 * it came on at to where it went out, the champion's bracket first. */
static void tree_view(App *a, UIRect area)
{
    const Fonts *fo = synos_fonts();
    int n = syn_arena_count();
    char s[240], fit[160];
    float hw = skin_text(fo->heading, "TOURNAMENT TREE", area.x, area.y, TONE_TEXT, ALIGN_LEFT);
    if (n == 0) {
        para(fo->body,
             "The tree grows as rounds are won. Each being is a line from the level it came on at; where it "
             "was knocked out, its line joins the line of the one that beat it, so every round's winners "
             "gather into the champion at the top.",
             area.x, area.y + 40, fminf(area.w, 900), TONE_MUTED);
        return;
    }
    if (n > Tree.cap) {
        Tree.cap = n * 2;
        Tree.t = realloc(Tree.t, (size_t)Tree.cap * sizeof *Tree.t);
        Tree.kids = realloc(Tree.kids, (size_t)Tree.cap * sizeof *Tree.kids);
        Tree.byuid = realloc(Tree.byuid, (size_t)Tree.cap * sizeof *Tree.byuid);
        if (!Tree.t || !Tree.kids || !Tree.byuid)
            abort();
    }
    TNode *t = Tree.t;
    g_tnodes = t;
    for (int i = 0; i < n; i++) {
        t[i] = (TNode){.e = syn_arena_at(i), .parent = -1, .col = -1};
        Tree.byuid[i] = i;
    }
    qsort(Tree.byuid, (size_t)n, sizeof *Tree.byuid, cmp_uid);
    int nk = 0;
    for (int i = 0; i < n; i++) {
        uint32_t by = t[i].e->state == ENTRY_IN ? 0 : t[i].e->by;
        int lo = 0, hi = n - 1;
        while (by && lo <= hi) {
            int mid = (lo + hi) / 2;
            uint32_t u = t[Tree.byuid[mid]].e->uid;
            if (u == by) {
                t[i].parent = Tree.byuid[mid];
                break;
            }
            if (u < by)
                lo = mid + 1;
            else
                hi = mid - 1;
        }
        if (t[i].parent >= 0)
            Tree.kids[nk++] = i;
    }
    qsort(Tree.kids, (size_t)nk, sizeof *Tree.kids, cmp_kids);
    for (int k = 0; k < nk; k++) {
        TNode *p = &t[t[Tree.kids[k]].parent];
        if (!p->nkids)
            p->kid0 = k;
        p->nkids++;
    }

    /* the roots in the leaderboard's order, as many brackets as fit */
    UIRect pl = {area.x + 70, area.y + 50, area.w - 80, area.h - 60};
    int cols = 0, roots = 0, left_out = 0, maxl = 1;
    for (int i = 0; i < n; i++) {
        if (t[i].parent >= 0)
            continue;
        int c = tree_cols(i);
        if (roots && (float)(cols + c) * 3 > pl.w) {
            left_out += c;
            continue;
        }
        cols = tree_lay(i, cols);
        roots++;
    }
    /* the levels the brackets shown reach */
    for (int i = 0; i < n; i++)
        if (t[i].col >= 0 && t[i].e->level > maxl)
            maxl = t[i].e->level;
    float cw = pl.w / (float)(cols > 0 ? cols : 1);
    if (cw > 90)
        cw = 90;
    /* the portraits of the brackets' tops over them: room above the top */
    float top_room = pl.h >= 500 && cw * (float)cols >= 200 ? 104 : 30;
    UIRect plot = {pl.x, pl.y + top_room, pl.w, pl.h - top_room - 10};
    float rh = plot.h / ((float)maxl + 0.5f);
#define TY(l) (plot.y + plot.h - (float)(l) * rh)

    snprintf(s, sizeof s, "%d being%s · %d bracket%s%s", n, n == 1 ? "" : "s", roots, roots == 1 ? "" : "s",
             left_out ? " shown, the smallest left out for room" : "");
    skin_text(fo->small, skin_fit(fo->small, s, area.w - hw - 12, fit, sizeof fit), area.x + area.w, area.y + 6,
              TONE_MUTED, ALIGN_RIGHT);
    para(fo->small,
         "Each being is a line from the level it came on at. Where it was knocked out, its line joins the line of "
         "the one that beat it, so the winners gather into the champion at the top. Lit lines are still in the "
         "running.",
         area.x, area.y + 28, area.w, TONE_MUTED);

    /* the levels, up the side */
    int every = rh >= 22 ? 1 : (int)ceilf(22 / rh);
    for (int l = 1; l <= maxl; l += every) {
        float y = TY(l);
        skin_fill(ui_rect(plot.x, y, plot.w, 1), skin_theme()->border, 0.25f);
        snprintf(s, sizeof s, "LEVEL %d", l);
        skin_text(fo->small, s, plot.x - 10, y - 8, TONE_MUTED, ALIGN_RIGHT);
    }

    /* the lines: each from where it came on to where it went out, then
     * across to the one that beat it; those in the running lit, the
     * champion's brightest */
    const ArenaEntry *champ = syn_arena_at(0);
    float lw = cw >= 8 ? 1.2f : 0.8f;
    batch_begin(&S.b, TX_DISC, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < n; i++) {
        const ArenaEntry *e = t[i].e;
        if (t[i].col < 0)
            continue;
        float x = plot.x + cw * ((float)t[i].col + 0.5f);
        bool in = e->state == ENTRY_IN;
        SDL_Color c = e == champ && in ? tone(TONE_ACCENT, 255) : in ? tone(TONE_TEXT, 200) : tone(TONE_MUTED, 150);
        float y0 = TY((float)e->start - 0.5f), y1 = TY(e->level);
        if (t[i].parent >= 0) {
            float yj = TY((float)e->level + 0.5f), xp = plot.x + cw * ((float)t[t[i].parent].col + 0.5f);
            batch_capsule(&S.b, x, y0, x, yj, lw, c);
            batch_capsule(&S.b, x, yj, xp, yj, lw, c);
        } else {
            batch_capsule(&S.b, x, y0, x, y1, e == champ ? lw * 1.8f : lw, c);
        }
    }
    /* a dot where each went out, or stands */
    float dr = fminf(5, fmaxf(1.5f, cw * 0.3f));
    int hover = -1;
    float hd = 1e9f;
    for (int i = 0; i < n; i++) {
        const ArenaEntry *e = t[i].e;
        if (t[i].col < 0)
            continue;
        float x = plot.x + cw * ((float)t[i].col + 0.5f), y = TY(e->level);
        float d = hypotf((float)g_mx - x, (float)g_my - y);
        if (d < hd && d < fmaxf(8, cw * 0.6f)) {
            hd = d;
            hover = i;
        }
        bool in = e->state == ENTRY_IN;
        Rgb bg = skin_theme()->bg;
        batch_sprite(&S.b, x, y, dr + 1.5f, 0, (SDL_Color){bg.r, bg.g, bg.b, 255});
        batch_sprite(&S.b, x, y, dr, 0,
                     in ? (syn_arena_in_round(e->uid) ? tone(TONE_ACCENT, 255) : tone(TONE_TEXT, 255))
                        : tone(TONE_MUTED, 200));
    }
    batch_flush(&S.b, gfx_renderer());
#undef TY

    /* the brackets' tops, over the right of each bracket, where its top
     * stands: their portraits and names */
    for (int i = 0; i < n; i++) {
        const ArenaEntry *e = t[i].e;
        if (t[i].parent >= 0 || t[i].col < 0)
            continue;
        float span = cw * (float)tree_cols(i), side = fminf(72, fminf(span - 6, top_room - 30));
        float right = plot.x + cw * (float)(t[i].col + 1), y = plot.y + plot.h - (float)e->level * rh;
        if (side >= 24) {
            snprintf(s, sizeof s, "%s, level %d, %s: click for the laboratory", e->name, e->level,
                     e->state == ENTRY_IN ? "in the running" : "out");
            thumb(a, ui_rect(right - side - 2, y - side - 26, side, side), e->gc, ui_id("arena-tree", e->uid), s,
                  e->name);
        }
        if (span >= 70)
            skin_text(fo->small_bold, skin_fit(fo->small_bold, e->name, span - 4, fit, sizeof fit), right - 2,
                      y - 22, e == champ ? TONE_ACCENT : TONE_TEXT, ALIGN_RIGHT);
    }

    /* the one under the mouse, and a click for the laboratory */
    if (hover >= 0 && over(plot)) {
        const ArenaEntry *e = t[hover].e;
        char st[120];
        syn_arena_standing(e, st, sizeof st);
        int beat = t[hover].nkids;
        S.ntip = 0;
        tip_line((SDL_Color){0, 0, 0, 0}, "%s, level %d", e->name, e->level);
        tip_line((SDL_Color){0, 0, 0, 0}, "%s", syn_arena_in_round(e->uid) ? "fighting now" : st);
        tip_line((SDL_Color){0, 0, 0, 0}, "came on at level %d in round %d · %d win%s · beat %d", e->start,
                 e->cycle, e->wins, e->wins == 1 ? "" : "s", beat);
        tip_line((SDL_Color){0, 0, 0, 0}, "%s bloodline · click for the laboratory", e->line);
        S.tip_x = (float)g_mx;
        S.tip_y = (float)g_my;
        float x = plot.x + cw * ((float)t[hover].col + 0.5f), y = plot.y + plot.h - (float)e->level * rh;
        UIRect hit = {x - 10, y - 10, 20, 20};
        if (e->gc && ui_pressed(sdl(hit), 1))
            lab_open(a, e->gc);
    }
}

void syn_arena_frame(App *a, int w, int h)
{
    const Fonts *fo = synos_fonts();
    ui_mouse(&g_mx, &g_my);
    S.ntip = 0;
    bool keys = !ui_any_modal() && !ui_text_focused() && !ui_popup_showing();
    if (keys && ui_key_pressed(SDLK_ESCAPE)) {
        synos_set_screen(SYNOS_MENU);
        return;
    }
    bool running = syn_arena_running();
    /* the battles if they are on, else the setup */
    if (!S.view_set) {
        S.view = running ? V_BATTLES : V_SETUP;
        S.view_set = true;
    }

    skin_fill(ui_rect(0, 0, (float)w, (float)h), (Rgb){0, 0, 0}, 0.5f);
    float pw = fminf((float)w - 48, 2400), ph = (float)h - 48;
    UIRect panel = {((float)w - pw) / 2, 24, pw, ph};
    skin_panel(panel, true);

    /* the title; START or STOP, PRINT WINNERS, BACK */
    float x = panel.x + PAD, y = panel.y + 14, right = panel.x + panel.w - PAD;
    x += skin_text(fo->big, "ARENA", x, y, TONE_TEXT, ALIGN_LEFT) + 14;
    skin_text(fo->body, SIM_TITLE, x, y + 9, TONE_MUTED, ALIGN_LEFT);
    UIRect back = {right - 110, y, 110, 36};
    UIRect lib = {back.x - 8 - 120, y, 120, 36};
    UIRect print = {lib.x - 8 - 180, y, 180, 36};
    const char *go_label = running ? "STOP" : "START BATTLES";
    float gw = fmaxf(120, skin_text_width(fo->bold, go_label) + 40);
    UIRect go = {print.x - 8 - gw, y, gw, 36};
    if (button(go, go_label, running, true)) {
        if (running) {
            syn_arena_stop();
        } else {
            char err[256];
            if (syn_arena_start(err, sizeof err)) {
                S.view = V_BATTLES;
            } else {
                snprintf(S.status, sizeof S.status, "The battles couldn't start: %s", err);
                S.status_at = SDL_GetTicks();
            }
        }
        running = syn_arena_running();
    }
    if (button(print, "PRINT WINNERS", false, syn_arena_count() > 0))
        print_winners();
    if (over(print))
        ui_tooltip("The ladder into a folder here: each being's .bgg, a list and a picture of the highest");
    if (button(lib, "LIBRARY", false, true)) {
        syn_library_open_from(SYNOS_ARENA);
        synos_set_screen(SYNOS_LIBRARY);
        return;
    }
    if (over(lib))
        ui_tooltip("The being library: beings kept from earlier, to put on the ladder");
    if (button(back, "BACK", false, true)) {
        synos_set_screen(SYNOS_MENU);
        return;
    }

    /* SETUP or BATTLES, and how it goes */
    y += 50;
    UIRect t1 = {panel.x + PAD, y, 130, 34}, t2 = {t1.x + t1.w + 6, y, 130, 34}, t3 = {t2.x + t2.w + 6, y, 130, 34};
    if (button(t1, "SETUP", S.view == V_SETUP, true))
        S.view = V_SETUP;
    if (button(t2, "BATTLES", S.view == V_BATTLES, true))
        S.view = V_BATTLES;
    if (button(t3, "TREE", S.view == V_TREE, true))
        S.view = V_TREE;
    if (over(t3))
        ui_tooltip("The tournament tree: every being joined to the one that knocked it out");
    status_line(ui_rect(t3.x + t3.w + 18, y, right - t3.x - t3.w - 18, 34), running);
    y += 34 + 16;

    UIRect area = {panel.x + PAD, y, right - panel.x - PAD, panel.y + panel.h - PAD - y};
    if (S.view == V_BATTLES)
        battles_view(a, area, running);
    else if (S.view == V_TREE)
        tree_view(a, area);
    else
        setup_view(a, area, running);

    tip_draw(w, h);
}

void syn_arena_shot_prepare(void)
{
    /* SYN_BIOGENESIS_SHOT_ARENA_VIEW = setup | battles | tree */
    const char *v = getenv("SYN_BIOGENESIS_SHOT_ARENA_VIEW");
    if (v) {
        S.view = !strcmp(v, "tree") ? V_TREE : !strcmp(v, "setup") ? V_SETUP : V_BATTLES;
        S.view_set = true;
    }
    const char *s = getenv("SYN_BIOGENESIS_SHOT_ARENA");
    if (!s)
        return;
    int cycles = 6, units = 60;
    long long seed = 1000;
    sscanf(s, "%d,%d,%lld", &cycles, &units, &seed);
    syn_arena_set_board(NULL);
    ArenaSettings *st = syn_arena_settings();
    st->units = units;
    st->seed = seed;
    st->target = 0;
    char err[256];
    if (!syn_arena_start(err, sizeof err)) {
        fprintf(stderr, "arena: %s\n", err);
        return;
    }
    /* the cycles, then the lanes on average half way (or ten seconds
     * more), so their charts have something in them */
    Uint32 t0 = SDL_GetTicks(), done = 0;
    while (SDL_GetTicks() - t0 < 300000) {
        syn_arena_poll();
        double sum = 0;
        int nl = syn_arena_lane_count();
        for (int i = 0; i < nl; i++) {
            const ArenaLane *L = syn_arena_lane(i);
            sum += L->busy && L->units ? (double)L->t / L->units : 0;
        }
        if (!done && syn_arena_result_count() >= cycles)
            done = SDL_GetTicks();
        if (done && (sum >= 0.5 * nl || SDL_GetTicks() - done > 10000))
            break;
        SDL_Delay(10);
    }
}

void syn_arena_screen_free(void)
{
    free(Tree.t);
    free(Tree.kids);
    free(Tree.byuid);
    memset(&Tree, 0, sizeof Tree);
    batch_free(&S.b);
    memset(&S, 0, sizeof S);
}
