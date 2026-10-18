/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Based on VisibleWorld.java
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

#include "ui/worldview.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "ui/gfx.h"
#include "ui/ui.h"
#ifdef BIO_COLOUR
#include "sim/params.h"
#endif

#ifdef BIO_SYN_EDITION
/* the SYN-OS interface pans by dragging and has a minimap instead; the
 * Heritage edition has the scrollbars */
#include "synos/synos.h"
#define SCROLLBAR (synos_active() ? 0 : 12)
#else
#define SCROLLBAR 12
#endif
#define ZOOM_MIN 0.125
#define ZOOM_MAX 16.0

#ifdef BIO_COLOUR
/* the colour build's segment colours are java.awt.Color values already */
static uint32_t argb(JColor c)
{
    return 0xFF000000u | c;
}
#else
static uint32_t argb(int c)
{
    uint8_t r, g, b;
    bio_color_rgb(c, &r, &g, &b);
    return 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}
#endif

void wv_init(WorldView *v)
{
    memset(v, 0, sizeof *v);
    v->zoom = 1.0;
}

void wv_free(WorldView *v)
{
    if (v->tex)
        SDL_DestroyTexture(v->tex);
    free(v->pix);
    memset(v, 0, sizeof *v);
}

/* the part of the panel that shows the world (minus scrollbars) */
static SDL_Rect inner(const WorldView *v)
{
    return (SDL_Rect){v->view.x, v->view.y, v->view.w - SCROLLBAR, v->view.h - SCROLLBAR};
}

void wv_clamp(WorldView *v, const World *w)
{
    SDL_Rect in = inner(v);
    double vw = in.w / v->zoom, vh = in.h / v->zoom;
    /* a world smaller than the panel sits centred; a larger one can't be
     * scrolled past its edges */
    if (vw >= w->width)
        v->cam_x = (w->width - vw) / 2;
    else
        v->cam_x = fmin(fmax(v->cam_x, 0), w->width - vw);
    if (vh >= w->height)
        v->cam_y = (w->height - vh) / 2;
    else
        v->cam_y = fmin(fmax(v->cam_y, 0), w->height - vh);
}

void wv_center_on(WorldView *v, const World *w, double wx, double wy)
{
    SDL_Rect in = inner(v);
    v->cam_x = wx - in.w / v->zoom / 2;
    v->cam_y = wy - in.h / v->zoom / 2;
    wv_clamp(v, w);
}

void wv_screen_to_world(const WorldView *v, int sx, int sy, double *wx, double *wy)
{
    *wx = v->cam_x + (sx - v->view.x) / v->zoom;
    *wy = v->cam_y + (sy - v->view.y) / v->zoom;
}

void wv_zoom_at(WorldView *v, const World *w, double factor, int sx, int sy)
{
    double wx, wy;
    wv_screen_to_world(v, sx, sy, &wx, &wy);
    double z = v->zoom * factor;
    if (z < ZOOM_MIN)
        z = ZOOM_MIN;
    if (z > ZOOM_MAX)
        z = ZOOM_MAX;
    /* snap near 1:1, the original's scale */
    if (fabs(z - 1.0) < 0.06)
        z = 1.0;
    v->zoom = z;
    v->cam_x = wx - (sx - v->view.x) / z;
    v->cam_y = wy - (sy - v->view.y) / z;
    wv_clamp(v, w);
}

static void bars(const WorldView *v, const World *w, SDL_Rect *htrack, SDL_Rect *hthumb,
                 SDL_Rect *vtrack, SDL_Rect *vthumb)
{
    SDL_Rect in = inner(v);
    *htrack = (SDL_Rect){in.x, in.y + in.h, in.w, SCROLLBAR};
    *vtrack = (SDL_Rect){in.x + in.w, in.y, SCROLLBAR, in.h};
    double vw = in.w / v->zoom, vh = in.h / v->zoom;
    double fx = fmin(1.0, vw / w->width), fy = fmin(1.0, vh / w->height);
    int tw = (int)fmax(20, in.w * fx), th = (int)fmax(20, in.h * fy);
    double px = w->width > vw ? v->cam_x / (w->width - vw) : 0;
    double py = w->height > vh ? v->cam_y / (w->height - vh) : 0;
    *hthumb = (SDL_Rect){in.x + (int)((in.w - tw) * px), in.y + in.h + 2, tw, SCROLLBAR - 4};
    *vthumb = (SDL_Rect){in.x + in.w + 2, in.y + (int)((in.h - th) * py), SCROLLBAR - 4, th};
}

void wv_input(WorldView *v, const World *w, bool keys_allowed)
{
    int mx, my;
    ui_mouse(&mx, &my);
    SDL_Rect in = inner(v);
    SDL_Rect ht, hb, vt, vb;
    bars(v, w, &ht, &hb, &vt, &vb);
    uint32_t cap = ui_id("worldview", 0);

    /* scrollbars */
    if (ui_pressed(hb, 1)) {
        v->drag_bar = 1;
        v->drag_off = mx - hb.x;
        ui_capture(cap);
    } else if (ui_pressed(vb, 1)) {
        v->drag_bar = 2;
        v->drag_off = my - vb.y;
        ui_capture(cap);
    } else if (ui_pressed(ht, 1)) {
        v->cam_x += (mx < hb.x ? -1 : 1) * in.w / v->zoom * 0.9;
    } else if (ui_pressed(vt, 1)) {
        v->cam_y += (my < vb.y ? -1 : 1) * in.h / v->zoom * 0.9;
    }
    if (v->drag_bar && ui_mouse_down(1)) {
        double vw = in.w / v->zoom, vh = in.h / v->zoom;
        if (v->drag_bar == 1 && in.w > hb.w)
            v->cam_x = (double)(mx - v->drag_off - in.x) / (in.w - hb.w) * (w->width - vw);
        if (v->drag_bar == 2 && in.h > vb.h)
            v->cam_y = (double)(my - v->drag_off - in.y) / (in.h - vb.h) * (w->height - vh);
    } else {
        v->drag_bar = 0;
    }

    /* middle-button drag pans */
    if (ui_pressed(in, 2)) {
        v->panning = true;
        v->pan_mx = mx;
        v->pan_my = my;
        v->pan_cx = v->cam_x;
        v->pan_cy = v->cam_y;
        ui_capture(cap);
    }
    if (v->panning) {
        if (ui_mouse_down(2)) {
            v->cam_x = v->pan_cx - (mx - v->pan_mx) / v->zoom;
            v->cam_y = v->pan_cy - (my - v->pan_my) / v->zoom;
        } else {
            v->panning = false;
        }
    }

    /* wheel: scroll like the original's scroll pane; Ctrl zooms */
    if (ui_hover(v->view)) {
        int wy = ui_wheel_y(), wx = ui_wheel_x();
        if (wy && (ui_mods() & KMOD_CTRL))
            wv_zoom_at(v, w, wy > 0 ? 1.25 : 0.8, mx, my);
        else if (wy && (ui_mods() & KMOD_SHIFT))
            v->cam_x -= wy * 60 / v->zoom;
        else if (wy)
            v->cam_y -= wy * 60 / v->zoom;
        if (wx)
            v->cam_x += wx * 60 / v->zoom;
    }

    if (keys_allowed) {
        double step = 40 / v->zoom;
        if (ui_key_pressed(SDLK_LEFT))
            v->cam_x -= step;
        if (ui_key_pressed(SDLK_RIGHT))
            v->cam_x += step;
        if (ui_key_pressed(SDLK_UP))
            v->cam_y -= step;
        if (ui_key_pressed(SDLK_DOWN))
            v->cam_y += step;
        int cx = in.x + in.w / 2, cy = in.y + in.h / 2;
        if (ui_key_pressed(SDLK_PLUS) || ui_key_pressed(SDLK_EQUALS) ||
            ui_key_pressed(SDLK_KP_PLUS))
            wv_zoom_at(v, w, 1.25, cx, cy);
        if (ui_key_pressed(SDLK_MINUS) || ui_key_pressed(SDLK_KP_MINUS))
            wv_zoom_at(v, w, 0.8, cx, cy);
        if (ui_key_pressed(SDLK_0) || ui_key_pressed(SDLK_KP_0))
            wv_zoom_at(v, w, 1.0 / v->zoom, cx, cy);
    }
    wv_clamp(v, w);
}

/* ---- software rasteriser ---- */

typedef struct {
    uint32_t *pix;
    int w, h;
} Canvas;

static void plot_line(Canvas *c, int x0, int y0, int x1, int y1, uint32_t col)
{
    /* Bresenham, clipped per pixel; organism segments are short */
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    if ((x0 < 0 && x1 < 0) || (y0 < 0 && y1 < 0) || (x0 >= c->w && x1 >= c->w) ||
        (y0 >= c->h && y1 >= c->h))
        return;
    for (;;) {
        if ((unsigned)x0 < (unsigned)c->w && (unsigned)y0 < (unsigned)c->h)
            c->pix[y0 * c->w + x0] = col;
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

#ifdef BIO_COLOUR
/* Color Mod's Organism.draw: the flash colour, except on eyes; else the
 * segment colours; brown once dead, unless it has eyes. */
static uint32_t segment_argb(const Organism *o, int i)
{
    if (o->frames_color > 0)
        return o->haseyes && o->seg_color[i] == CM_EYE ? argb(o->seg_color[i])
                                                       : argb(o->flash_color);
    return o->alive || o->haseyes ? argb(o->seg_color[i]) : argb(CM_BROWN);
}

static void draw_organism(Canvas *c, const Organism *o, double ox, double oy, double k)
{
    for (int i = 0; i < o->segments; i++) {
        uint32_t col = segment_argb(o, i);
#else
/* Organism.draw: flash colour, else segment colours (brown once dead). */
static void draw_organism(Canvas *c, const Organism *o, double ox, double oy, double k)
{
    uint32_t flash =
        o->frames_color > 0 && o->flash_color != FLASH_NONE ? argb(o->flash_color) : 0;
    uint32_t brown = argb(COL_BROWN);
    for (int i = 0; i < o->segments; i++) {
        uint32_t col = flash ? flash : (o->alive ? argb(o->seg_color[i]) : brown);
#endif
        int x0 = o->x1[i] + o->center_x, y0 = o->y1[i] + o->center_y;
        int x1 = o->x2[i] + o->center_x, y1 = o->y2[i] + o->center_y;
        if (k == 1.0) {
            plot_line(c, x0 - (int)ox, y0 - (int)oy, x1 - (int)ox, y1 - (int)oy, col);
        } else {
            plot_line(c, (int)floor((x0 - ox) * k), (int)floor((y0 - oy) * k),
                      (int)floor((x1 - ox) * k), (int)floor((y1 - oy) * k), col);
        }
    }
}

static void plot_rect(Canvas *c, int x, int y, int w, int h, uint32_t col)
{
    plot_line(c, x, y, x + w, y, col);
    plot_line(c, x, y + h, x + w, y + h, col);
    plot_line(c, x, y, x, y + h, col);
    plot_line(c, x + w, y, x + w, y + h, col);
}

void wv_draw(WorldView *v, const World *w, const Organism *selected)
{
    SDL_Renderer *R = gfx_renderer();
    SDL_Rect in = inner(v);
    gfx_fill(v->view, COL(48, 48, 52));
    if (in.w <= 0 || in.h <= 0)
        return;

    /* buffer: world pixels when zoomed in (scaled up on blit), screen
     * pixels when zoomed out */
    double k = v->zoom >= 1.0 ? 1.0 : v->zoom;
    double ox = floor(v->cam_x), oy = floor(v->cam_y);
    int bw = (int)ceil(in.w / v->zoom * k) + 2, bh = (int)ceil(in.h / v->zoom * k) + 2;
    size_t need = (size_t)bw * bh;
    if (need > v->pix_cap) {
        free(v->pix);
        v->pix = malloc(need * sizeof *v->pix);
        if (!v->pix)
            abort();
        v->pix_cap = need;
    }
    if (!v->tex || v->tex_w < bw || v->tex_h < bh) {
        if (v->tex)
            SDL_DestroyTexture(v->tex);
        v->tex_w = bw > v->tex_w ? bw + 64 : v->tex_w;
        v->tex_h = bh > v->tex_h ? bh + 64 : v->tex_h;
        v->tex = SDL_CreateTexture(R, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                   v->tex_w, v->tex_h);
        SDL_SetTextureScaleMode(v->tex, SDL_ScaleModeNearest);
    }
    Canvas c = {v->pix, bw, bh};

    /* outside the world: grey, like the scroll pane's background */
    uint32_t outside = 0xFF303034u, black = 0xFF000000u;
#ifdef BIO_SYN_EDITION
    /* SYN-OS: beyond the world is the theme's */
    if (synos_active())
        outside = 0xFF000000u | (uint32_t)C_PANEL.r << 16 | (uint32_t)C_PANEL.g << 8 | C_PANEL.b;
#endif
    for (int y = 0; y < bh; y++) {
        double wy = oy + y / k;
        uint32_t *row = c.pix + (size_t)y * bw;
        if (wy < 0 || wy >= w->height) {
            for (int x = 0; x < bw; x++)
                row[x] = outside;
            continue;
        }
        int x0 = (int)ceil((0 - ox) * k), x1 = (int)ceil((w->width - ox) * k);
        if (x0 < 0)
            x0 = 0;
        if (x1 > bw)
            x1 = bw;
        for (int x = 0; x < bw; x++)
            row[x] = (x >= x0 && x < x1) ? black : outside;
    }

    double wx0 = ox, wy0 = oy, wx1 = ox + bw / k, wy1 = oy + bh / k;
    for (int i = 0; i < w->norgs; i++) {
        Organism *o = w->orgs[i];
        if (o->r.x > wx1 || o->r.y > wy1 || o->r.x + o->r.width < wx0 ||
            o->r.y + o->r.height < wy0)
            continue;
        draw_organism(&c, o, ox, oy, k);
#ifndef BIO_COLOUR
        /* as Organism.draw: the flash lasts ten drawings (Color Mod counts
         * them in Organism.move instead) */
        if (o->frames_color > 0)
            o->frames_color--;
#endif
    }
    if (selected) {
        const JRect *r = &selected->r;
#ifdef BIO_COLOUR
        /* Color Mod outlines an infected organism in white */
        plot_rect(&c, (int)floor((r->x - ox) * k), (int)floor((r->y - oy) * k),
                  (int)floor((r->width - 1) * k), (int)floor((r->height - 1) * k),
                  selected->infected_gc ? 0xFFFFFFFFu : 0xFFFFC800u);
#else
        plot_rect(&c, (int)floor((r->x - ox) * k), (int)floor((r->y - oy) * k),
                  (int)floor((r->width - 1) * k), (int)floor((r->height - 1) * k), 0xFFFFC800u);
#endif
    }

    SDL_Rect src = {0, 0, bw, bh};
    SDL_UpdateTexture(v->tex, &src, v->pix, bw * (int)sizeof(uint32_t));
    double s = v->zoom / k;
    SDL_Rect dst = {in.x - (int)lround((v->cam_x - ox) * v->zoom), in.y - (int)lround((v->cam_y - oy) * v->zoom),
                    (int)lround(bw * s), (int)lround(bh * s)};
    gfx_clip_push(in);
    SDL_RenderCopy(R, v->tex, &src, &dst);
    gfx_clip_pop();

#ifdef BIO_SYN_EDITION
    if (SCROLLBAR) {
#endif
    /* scrollbars */
    SDL_Rect ht, hb, vt, vb;
    bars(v, w, &ht, &hb, &vt, &vb);
    gfx_fill(ht, C_PANEL);
    gfx_fill(vt, C_PANEL);
    gfx_fill(hb, v->drag_bar == 1 ? C_PRESS : C_HOVER);
    gfx_fill(vb, v->drag_bar == 2 ? C_PRESS : C_HOVER);
    gfx_fill((SDL_Rect){in.x + in.w, in.y + in.h, SCROLLBAR, SCROLLBAR}, C_PANEL);
#ifdef BIO_SYN_EDITION
    }
#endif
}

SDL_Surface *wv_organism_image(const Organism *o)
{
    int w = o->r.width > 0 ? o->r.width : 1, h = o->r.height > 0 ? o->r.height : 1;
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!s)
        return NULL;
    SDL_FillRect(s, NULL, 0xFF000000u);
    Canvas c = {s->pixels, w, h};
    if (s->pitch != w * 4) {
        SDL_FreeSurface(s);
        return NULL;
    }
    for (int i = o->segments - 1; i >= 0; i--) {
        uint32_t col = argb(o->seg_color[i]);
        plot_line(&c, o->x1[i] - o->r.x + o->center_x, o->y1[i] - o->r.y + o->center_y,
                  o->x2[i] - o->r.x + o->center_x, o->y2[i] - o->r.y + o->center_y, col);
    }
    return s;
}
