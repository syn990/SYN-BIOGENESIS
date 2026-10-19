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

#include "syn/genesis_screen.h"

#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>

#include "synos/skin.h"
#include "synos/synos.h"
#include "ui/gfx.h"
#include "ui/ui.h"

/* made unseen for this long before the screen shows, so a quick world
 * doesn't flash it */
#define QUIET_S 0.25
/* making between frames: long enough that drawing (and waiting for the
 * screen) takes little of its time, short enough for Esc to answer */
#define SLICE_S 0.1

static void commas(char *out, size_t n, long v)
{
    char d[32];
    int len = snprintf(d, sizeof d, "%ld", v < 0 ? -v : v);
    size_t o = 0;
    if (v < 0 && o + 1 < n)
        out[o++] = '-';
    for (int i = 0; i < len && o + 1 < n; i++) {
        if (i && (len - i) % 3 == 0 && o + 1 < n)
            out[o++] = ',';
        if (o + 1 < n)
            out[o++] = d[i];
    }
    out[o] = '\0';
}

static void save_shot(SDL_Renderer *r, const char *path)
{
    int w, h;
    SDL_GetRendererOutputSize(r, &w, &h);
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (s && SDL_RenderReadPixels(r, NULL, SDL_PIXELFORMAT_ARGB8888, s->pixels, s->pitch) == 0)
        IMG_SavePNG(s, path);
    else
        fprintf(stderr, "screenshot failed: %s\n", SDL_GetError());
    if (s)
        SDL_FreeSurface(s);
}

static void draw(const SynGenesis *g, SDL_Renderer *r, double secs, const char *shot)
{
    int W, H;
    SDL_GetRendererOutputSize(r, &W, &H);
    char made[32], total[32], ww[32], wh[32], line[96], size[160];
    commas(made, sizeof made, g->made);
    commas(total, sizeof total, g->total);
    commas(ww, sizeof ww, g->w->width);
    commas(wh, sizeof wh, g->w->height);
    snprintf(line, sizeof line, "%s of %s organisms", made, total);
    snprintf(size, sizeof size, "In a %s x %s world, %.0f s so far", ww, wh, secs);
    const char *hint = "Esc starts the world with the organisms made so far";
    double frac = g->total > 0 ? (double)g->made / g->total : 1;

    if (synos_active()) {
        const Fonts *fo = synos_fonts();
        float hh = skin_text_height(fo->heading), bh = skin_text_height(fo->body);
        float sh = skin_text_height(fo->small);
        float pw = (float)W - 32 < 580 ? (float)W - 32 : 580;
        float ph = 22 + hh + 14 + bh + 10 + 8 + 14 + sh + 6 + sh + 22;
        UIRect p = ui_rect(((float)W - pw) / 2, ((float)H - ph) / 2, pw, ph);
        skin_background((float)W, (float)H);
        skin_panel(p, false);
        float x = p.x + 24, y = p.y + 22;
        skin_text(fo->heading, "CREATING WORLD", x, y, TONE_ACCENT, ALIGN_LEFT);
        y += hh + 14;
        skin_text(fo->body, line, x, y, TONE_TEXT, ALIGN_LEFT);
        y += bh + 10;
        skin_meter(ui_rect(x, y, p.w - 48, 8), frac, TONE_ACCENT);
        y += 8 + 14;
        skin_text(fo->small, size, x, y, TONE_MUTED, ALIGN_LEFT);
        y += sh + 6;
        skin_text(fo->small, hint, x, y, TONE_MUTED, ALIGN_LEFT);
        synos_end_frame();
    } else {
        int fh = gfx_font_h();
        int pw = W - 32 < 440 ? W - 32 : 440;
        int ph = 14 + fh + 10 + fh + 8 + 12 + 10 + fh + 4 + fh + 14;
        SDL_Rect p = {(W - pw) / 2, (H - ph) / 2, pw, ph};
        gfx_fill((SDL_Rect){0, 0, W, H}, C_BG);
        gfx_fill(p, C_PANEL);
        gfx_frame(p, C_BORDER);
        int x = p.x + 16, y = p.y + 14;
        gfx_text(x, y, "Creating world", C_TEXT);
        y += fh + 10;
        gfx_text(x, y, line, C_TEXT);
        y += fh + 8;
        SDL_Rect bar = {x, y, pw - 32, 12};
        gfx_fill(bar, C_FIELD);
        gfx_frame(bar, C_BORDER);
        gfx_fill((SDL_Rect){bar.x + 2, bar.y + 2, (int)((bar.w - 4) * frac), bar.h - 4}, C_ACCENT);
        y += 12 + 10;
        gfx_text(x, y, size, C_TEXT_DIM);
        y += fh + 4;
        gfx_text(x, y, hint, C_TEXT_DIM);
        gfx_end_frame();
    }
    if (shot)
        save_shot(r, shot);
    SDL_RenderPresent(r);
}

/* Test hooks: SYN_BIOGENESIS_GENESIS_SHOT=out.png shows the screen at once,
 * in screenshot and scripted runs too, and saves its 5th frame;
 * SYN_BIOGENESIS_GENESIS_STOP=N presses Esc at its Nth. */
SynGenesisEnd syn_genesis_run(SynGenesis *g, World *w, SDL_Renderer *r)
{
    syn_genesis_begin(g, w);
    const char *shot = getenv("SYN_BIOGENESIS_GENESIS_SHOT");
    const char *stop_env = getenv("SYN_BIOGENESIS_GENESIS_STOP");
    int stop_at = stop_env ? atoi(stop_env) : 0;
    bool hooked = shot || stop_at > 0;
    if (!hooked && (getenv("SYN_BIOGENESIS_SHOT") || getenv("SYN_BIOGENESIS_SCRIPT"))) {
        while (syn_genesis_next(g))
            ;
        return SYN_GENESIS_DONE;
    }
    double freq = (double)SDL_GetPerformanceFrequency();
    Uint64 t0 = SDL_GetPerformanceCounter();
    Uint64 quiet = t0 + (Uint64)(hooked ? 0 : QUIET_S * freq);
    while (SDL_GetPerformanceCounter() < quiet)
        if (!syn_genesis_next(g))
            return SYN_GENESIS_DONE;

    /* called mid-frame (from a menu or a key): draw on the window itself,
     * unclipped, and put things back after */
    SDL_Texture *target = SDL_GetRenderTarget(r);
    SDL_Rect clip;
    SDL_bool clipped = SDL_RenderIsClipEnabled(r);
    SDL_RenderGetClipRect(r, &clip);
    float alpha = skin_alpha();
    SDL_SetRenderTarget(r, NULL);
    SDL_RenderSetClipRect(r, NULL);
    skin_set_alpha(1);

    SynGenesisEnd end = SYN_GENESIS_DONE;
    bool more = true;
    for (int frame = 1; more; frame++) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                end = SYN_GENESIS_QUIT;
            else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE &&
                     end == SYN_GENESIS_DONE)
                end = SYN_GENESIS_STOPPED;
        }
        if (frame == stop_at)
            end = SYN_GENESIS_STOPPED;
        if (end != SYN_GENESIS_DONE)
            break;
        draw(g, r, (double)(SDL_GetPerformanceCounter() - t0) / freq, shot && frame == 5 ? shot : NULL);
        Uint64 until = SDL_GetPerformanceCounter() + (Uint64)(SLICE_S * freq);
        while ((more = syn_genesis_next(g)) && SDL_GetPerformanceCounter() < until)
            ;
    }

    SDL_SetRenderTarget(r, target);
    SDL_RenderSetClipRect(r, clipped ? &clip : NULL);
    skin_set_alpha(alpha);
    /* the events went here: the buttons held and the pointer as they are */
    ui_input_resync();
    return end;
}

/* a warning from this many seconds */
#define WARN_S 10.0
/* "starts quickly" */
#define QUICK_S 5.0

static void duration(char *out, size_t n, double s)
{
    if (s < 90)
        snprintf(out, n, "%.0f seconds", s);
    else if (s < 90 * 60)
        snprintf(out, n, "%.0f minutes", s / 60);
    else if (s < 48 * 3600)
        snprintf(out, n, "%.0f hours", s / 3600);
    else
        snprintf(out, n, "%.0f days", s / 86400);
}

int syn_genesis_warning(int x, int y, const char *initial_organisms, bool draw)
{
    char *end;
    long n = strtol(initial_organisms, &end, 10);
    if (end == initial_organisms || *end || n <= 0 || syn_genesis_seconds((double)n) < WARN_S)
        return 0;
    int fh = gfx_font_h();
    if (!draw)
        return 2 * (fh + 4) + 4;
    /* the most that start within QUICK_S, to two figures */
    long lo = 1, hi = n;
    while (lo < hi) {
        long mid = lo + (hi - lo + 1) / 2;
        if (syn_genesis_seconds((double)mid) <= QUICK_S)
            lo = mid;
        else
            hi = mid - 1;
    }
    long step = 1;
    while (lo / step >= 100)
        step *= 10;
    lo = lo / step * step;
    char num[32], quick[32], t[48], l1[160], l2[160];
    commas(num, sizeof num, n);
    commas(quick, sizeof quick, lo);
    duration(t, sizeof t, syn_genesis_seconds((double)n));
    snprintf(l1, sizeof l1, "A new world of %s organisms takes about %s to make.", num, t);
    snprintf(l2, sizeof l2, "%s or fewer take under %.0f seconds; Esc stops it part way.", quick,
             QUICK_S);
    Col c = C_ORANGE;
    if (synos_active()) {
        Uint8 r, g, b;
        skin_tone_rgb(TONE_WARN, &r, &g, &b);
        c = COL(r, g, b);
    }
    gfx_text(x, y, l1, c);
    gfx_text(x, y + fh + 4, l2, C_TEXT_DIM);
    return 2 * (fh + 4) + 4;
}

void syn_genesis_stopped_text(const SynGenesis *g, char *out, size_t n)
{
    char pop[32], total[32];
    commas(pop, sizeof pop, g->w->population);
    commas(total, sizeof total, g->total);
    snprintf(out, n, "Stopped part way: %s of the %s organisms", pop, total);
}
