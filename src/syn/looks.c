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

#include "syn/looks.h"

#include <math.h>
#include <stdio.h>

#include "sim/genetic_code.h"
#include "syn/batch.h"
#include "syn/palette.h"
#include "ui/gfx.h"

/* Every look keeps the colours' meaning (green still photosynthesises),
 * in its own palette: these for 0.9's colours, and for Color Mod's the same
 * where it has them. */
enum { P_GREEN, P_RED, P_CYAN, P_BLUE, P_WHITE, P_GRAY, P_YELLOW, P_BROWN };
static const SDL_Color PAL[LOOK_COUNT][8] = {
    [LOOK_BIOLOGICAL] = {{88, 200, 92, 255},
                         {214, 64, 72, 255},
                         {90, 210, 220, 255},
                         {86, 112, 230, 255},
                         {232, 228, 240, 255},
                         {140, 146, 160, 255},
                         {236, 196, 70, 255},
                         {128, 92, 58, 255}},
    [LOOK_GALAXY] = {{110, 255, 170, 255},
                     {255, 90, 70, 255},
                     {120, 220, 255, 255},
                     {90, 120, 255, 255},
                     {255, 250, 240, 255},
                     {170, 140, 220, 255},
                     {255, 210, 110, 255},
                     {140, 90, 70, 255}},
    /* SYN-OS: its reds, gold and grey, plus a green, cyan and blue for the
     * colours SYN-OS has no use for */
    [LOOK_DIGITAL] = {{72, 255, 96, 255},
                      {255, 32, 32, 255},
                      {32, 232, 255, 255},
                      {74, 124, 255, 255},
                      {240, 240, 240, 255},
                      {170, 170, 170, 255},
                      {224, 160, 32, 255},
                      {138, 20, 20, 255}},
};

enum {
    B_MEMBRANE, /* TX_GLOW, blended */
    B_GLOW,     /* TX_GLOW, added */
    B_BODY,     /* TX_DISC, blended */
    B_LIGHT,    /* TX_DISC, added */
    B_STAR,     /* TX_STAR, added */
    B_TOP,      /* TX_DISC, blended, over the bodies */
    B_RING,     /* TX_RING, blended */
    B_SOLID,    /* TX_WHITE, blended */
    B_SOLID_ADD,
    B_RIM,    /* TX_DISC, blended, into the cells layer: membranes */
    B_PLASMA, /* the same, inside them */
    B_BG,     /* backgrounds and overlays, flushed straight away */
    B_COUNT
};

static Batch B[B_COUNT];
/* Biological draws its cells opaque into this layer, so that each cell's
 * strokes merge into one body with one membrane, then lays it over the
 * water translucent. */
static SDL_Texture *cells;
static int cells_w, cells_h;

#ifdef BIO_COLOUR
static void to_hsv(SDL_Color k, float *h, float *s, float *v)
{
    float r = k.r / 255.0f, g = k.g / 255.0f, b = k.b / 255.0f;
    float mx = fmaxf(r, fmaxf(g, b)), mn = fminf(r, fminf(g, b)), d = mx - mn;
    *v = mx;
    *s = mx > 0 ? d / mx : 0;
    if (d <= 0)
        *h = 0;
    else if (mx == r)
        *h = fmodf((g - b) / d + 6, 6);
    else if (mx == g)
        *h = (b - r) / d + 2;
    else
        *h = (r - g) / d + 4;
}

static SDL_Color from_hsv(float h, float s, float v)
{
    float c = v * s, x = c * (1 - fabsf(fmodf(h, 2) - 1)), m = v - c, r = 0, g = 0, b = 0;
    int hi = (int)h % 6;
    if (hi == 0) { r = c; g = x; }
    else if (hi == 1) { r = x; g = c; }
    else if (hi == 2) { g = c; b = x; }
    else if (hi == 3) { g = x; b = c; }
    else if (hi == 4) { r = x; b = c; }
    else { r = c; b = x; }
    return (SDL_Color){(uint8_t)lroundf((r + m) * 255), (uint8_t)lroundf((g + m) * 255),
                       (uint8_t)lroundf((b + m) * 255), 255};
}

/* Color Mod's colours in a look: the seven 0.9 has as 0.9's look draws
 * them; the rest keep their hue, so all 46 stay apart, with the look's
 * light and saturation; segments out of action greyed. */
static SDL_Color colour_look(Look l, int slot)
{
    static SDL_Color tab[LOOK_COUNT][SYN_NCOL];
    static bool ready;
    if (!ready) {
        static const struct {
            uint8_t r, g, b;
            int bio;
        } shared[] = {{0, 255, 0, P_GREEN},    {255, 0, 0, P_RED},   {0, 255, 255, P_CYAN},
                      {0, 0, 255, P_BLUE},     {255, 255, 255, P_WHITE},
                      {128, 128, 128, P_GRAY}, {255, 255, 0, P_YELLOW}};
        for (int s = 0; s < SYN_NCOL; s++) {
            SDL_Color k = {0, 0, 0, 255};
            syn_col_rgb(s, &k.r, &k.g, &k.b);
            tab[LOOK_CLASSIC][s] = k;
            int same = s == SYN_DEAD ? P_BROWN : -1;
            for (size_t i = 0; i < sizeof shared / sizeof shared[0]; i++)
                if (k.r == shared[i].r && k.g == shared[i].g && k.b == shared[i].b)
                    same = shared[i].bio;
            float h, sat, v;
            to_hsv(k, &h, &sat, &v);
            bool off = syn_col_role(s) == ROLE_INACTIVE;
            for (int L = LOOK_BIOLOGICAL; L < LOOK_COUNT; L++) {
                if (same >= 0) {
                    tab[L][s] = PAL[L][same];
                    continue;
                }
                float s2 = sat, v2 = v;
                if (L == LOOK_BIOLOGICAL) {
                    s2 = sat * 0.68f;
                    v2 = 0.42f + 0.52f * v;
                } else if (L == LOOK_GALAXY) {
                    s2 = sat * 0.62f;
                    v2 = 0.72f + 0.28f * v;
                } else {
                    s2 = sat > 0.2f ? fmaxf(sat, 0.88f) : sat;
                    v2 = sat > 0.2f ? 0.62f + 0.38f * v : 0.55f + 0.45f * v;
                }
                if (off) {
                    s2 *= 0.4f;
                    v2 *= 0.8f;
                }
                SDL_Color out = from_hsv(h, s2, v2);
                if (L == LOOK_GALAXY)
                    out = (SDL_Color){(uint8_t)(out.r + (255 - out.r) * 0.12f),
                                      (uint8_t)(out.g + (255 - out.g) * 0.12f),
                                      (uint8_t)(out.b + (255 - out.b) * 0.12f), 255};
                tab[L][s] = out;
            }
        }
        ready = true;
    }
    if (slot < 0 || slot >= SYN_NCOL)
        slot = SYN_NCOL - 2;
    return tab[l][slot];
}
#endif

SDL_Color look_color(Look l, int c)
{
#ifdef BIO_COLOUR
    return colour_look(l, c);
#else
    if (c < 0 || c >= COL_COUNT)
        c = COL_WHITE;
    if (l == LOOK_CLASSIC) {
        SDL_Color k = {0, 0, 0, 255};
        bio_color_rgb(c, &k.r, &k.g, &k.b);
        return k;
    }
    return PAL[l][c];
#endif
}

void look_free(void)
{
    for (int i = 0; i < B_COUNT; i++)
        batch_free(&B[i]);
    if (cells)
        SDL_DestroyTexture(cells);
    cells = NULL;
}

/* ---- helpers ---- */

static SDL_Color rgba(SDL_Color c, float a)
{
    a = a < 0 ? 0 : a > 255 ? 255 : a;
    c.a = (uint8_t)lroundf(a);
    return c;
}

static SDL_Color mix(SDL_Color a, SDL_Color b, float t)
{
    return (SDL_Color){(uint8_t)lroundf(a.r + (b.r - a.r) * t),
                       (uint8_t)lroundf(a.g + (b.g - a.g) * t),
                       (uint8_t)lroundf(a.b + (b.b - a.b) * t), a.a};
}

static const SDL_Color WHITE = {255, 255, 255, 255};

static float hashf(uint32_t a, uint32_t b)
{
    uint32_t h = a * 0x9e3779b1u ^ (b + 0x7f4a7c15u) * 0x85ebca77u;
    h ^= h >> 15;
    h *= 0x2c1b3c6du;
    h ^= h >> 12;
    h *= 0x297a2d39u;
    h ^= h >> 15;
    return (h & 0xffffff) / 16777216.0f;
}

static float sx(const LookCtx *c, double wx)
{
    return (float)((wx - c->cam_x) * c->zoom + c->in.x);
}

static float sy(const LookCtx *c, double wy)
{
    return (float)((wy - c->cam_y) * c->zoom + c->in.y);
}

/* Half-width on screen of a stroke ww world pixels wide, at least minhw.
 * A stroke thinner than that is drawn minhw wide and fainter (*cov), the
 * share of the pixels it would cover. */
static float stroke_hw(const LookCtx *c, float ww, float minhw, float *cov)
{
    float hw = ww * 0.5f * (float)c->zoom;
    if (hw >= minhw) {
        *cov = 1;
        return hw;
    }
    *cov = fmaxf(hw / minhw, 0.35f);
    return minhw;
}

static float org_hash(const DOrg *d)
{
    return hashf((uint32_t)d->o->id, 1);
}

/* how full of energy, 0..1 (1: ready to reproduce) */
static float org_charge(const DOrg *d)
{
    double re = syn_reproduce_energy(d->o);
    float e = (float)(d->o->energy / (re > 0 ? re : 1));
    return e < 0 ? 0 : e > 1 ? 1 : e;
}

/* 1 when a flash has just begun, fading to 0 over its ten drawings */
static float flash(const DOrg *d)
{
    return syn_flash_left(d->o);
}

static SDL_Color seg_col(Look l, const DOrg *d, int i)
{
    SDL_Color k = look_color(l, d->col[i]);
    float f = flash(d);
    if (f > 0)
        k = mix(k, look_color(l, syn_flash(d->o)), f);
    return k;
}

static Role seg_role(const DOrg *d, int i)
{
    return syn_col_role(d->col[i]);
}

/* the world's rectangle on screen */
static SDL_FRect world_rect(const LookCtx *c)
{
    return (SDL_FRect){sx(c, 0), sy(c, 0), (float)(c->w->width * c->zoom),
                       (float)(c->w->height * c->zoom)};
}

static void fill(SDL_FRect r, SDL_Color k)
{
    batch_rect(&B[B_BG], r.x, r.y, r.w, r.h, 0, 0, 1, 1, k);
}

static void flush_bg(const LookCtx *c, TexId t, SDL_BlendMode m)
{
    B[B_BG].tex = t;
    B[B_BG].blend = m;
    batch_flush(&B[B_BG], c->R);
}

/* Tiles of texture t over the part of the world in view: tile world
 * pixels square, shifted by (ox, oy) world pixels. */
static void tile_world(const LookCtx *c, TexId t, SDL_BlendMode m, double tile, double ox,
                       double oy, SDL_Color k)
{
    double ts = tile * c->zoom;
    if (ts < 8)
        return;
    double x0 = fmax(c->cam_x, 0), y0 = fmax(c->cam_y, 0);
    double x1 = fmin(c->cam_x + c->in.w / c->zoom, c->w->width);
    double y1 = fmin(c->cam_y + c->in.h / c->zoom, c->w->height);
    if (x1 <= x0 || y1 <= y0)
        return;
    SDL_FRect wr = world_rect(c);
    SDL_Rect clip = {(int)floorf(wr.x), (int)floorf(wr.y), (int)ceilf(wr.w), (int)ceilf(wr.h)};
    gfx_clip_push(clip);
    batch_begin(&B[B_BG], t, m);
    for (double ty = floor((y0 - oy) / tile) * tile + oy; ty < y1; ty += tile)
        for (double tx = floor((x0 - ox) / tile) * tile + ox; tx < x1; tx += tile)
            batch_rect(&B[B_BG], sx(c, tx), sy(c, ty), (float)ts, (float)ts, 0, 0, 1, 1, k);
    flush_bg(c, t, m);
    gfx_clip_pop();
}

/* everything around the world, darkened */
static void shade_outside(const LookCtx *c, SDL_Color k)
{
    SDL_FRect wr = world_rect(c);
    float L = (float)c->in.x, T = (float)c->in.y, R = L + c->in.w, Bt = T + c->in.h;
    batch_begin(&B[B_BG], TX_WHITE, SDL_BLENDMODE_BLEND);
    if (wr.y > T)
        fill((SDL_FRect){L, T, R - L, wr.y - T}, k);
    if (wr.y + wr.h < Bt)
        fill((SDL_FRect){L, wr.y + wr.h, R - L, Bt - wr.y - wr.h}, k);
    float y0 = fmaxf(wr.y, T), y1 = fminf(wr.y + wr.h, Bt);
    if (y1 > y0 && wr.x > L)
        fill((SDL_FRect){L, y0, wr.x - L, y1 - y0}, k);
    if (y1 > y0 && wr.x + wr.w < R)
        fill((SDL_FRect){wr.x + wr.w, y0, R - wr.x - wr.w, y1 - y0}, k);
    flush_bg(c, TX_WHITE, SDL_BLENDMODE_BLEND);
}

/* the world's edge as four strokes */
static void world_edge(const LookCtx *c, Batch *b, float hw, SDL_Color k)
{
    SDL_FRect r = world_rect(c);
    batch_stroke(b, r.x, r.y, r.x + r.w, r.y, hw, k);
    batch_stroke(b, r.x, r.y + r.h, r.x + r.w, r.y + r.h, hw, k);
    batch_stroke(b, r.x, r.y - hw, r.x, r.y + r.h + hw, hw, k);
    batch_stroke(b, r.x + r.w, r.y - hw, r.x + r.w, r.y + r.h + hw, hw, k);
}

static void vignette(const LookCtx *c, uint8_t a)
{
    batch_begin(&B[B_BG], TX_VIGNETTE, SDL_BLENDMODE_BLEND);
    fill((SDL_FRect){(float)c->in.x, (float)c->in.y, (float)c->in.w, (float)c->in.h},
         (SDL_Color){255, 255, 255, a});
    flush_bg(c, TX_VIGNETTE, SDL_BLENDMODE_BLEND);
}

/* ---- Biological: a drop of water under a microscope ---- */

static void bio_begin(const LookCtx *c)
{
    batch_begin(&B[B_BG], TX_WHITE, SDL_BLENDMODE_BLEND);
    fill((SDL_FRect){(float)c->in.x, (float)c->in.y, (float)c->in.w, (float)c->in.h},
         (SDL_Color){6, 12, 13, 255});
    fill(world_rect(c), (SDL_Color){10, 26, 28, 255});
    flush_bg(c, TX_WHITE, SDL_BLENDMODE_BLEND);

    /* the medium drifts, two layers against each other */
    double t = c->t;
    tile_world(c, TX_NOISE, SDL_BLENDMODE_ADD, 600, t * 4.0, t * 2.5,
               (SDL_Color){40, 110, 110, 38});
    tile_world(c, TX_NOISE, SDL_BLENDMODE_ADD, 380, -t * 3.0 + 170, t * 3.5 + 90,
               (SDL_Color){30, 80, 90, 30});

    /* motes floating in it */
    if (c->zoom >= 0.4) {
        const double cell = 160;
        double x0 = fmax(c->cam_x, 0), y0 = fmax(c->cam_y, 0);
        double x1 = fmin(c->cam_x + c->in.w / c->zoom, c->w->width);
        double y1 = fmin(c->cam_y + c->in.h / c->zoom, c->w->height);
        batch_begin(&B[B_BG], TX_GLOW, SDL_BLENDMODE_ADD);
        float r = fmaxf(1.4f * (float)c->zoom, 1.0f);
        for (int gy = (int)floor(y0 / cell); gy * cell < y1; gy++) {
            for (int gx = (int)floor(x0 / cell); gx * cell < x1; gx++) {
                for (uint32_t k = 0; k < 2; k++) {
                    uint32_t key = (uint32_t)gx * 7919u + (uint32_t)gy * 104729u + k;
                    float h1 = hashf(key, 3), h2 = hashf(key, 4), h3 = hashf(key, 5);
                    double wx = (gx + h1) * cell + 12 * sin(t * 0.3 + h3 * 6.3);
                    double wy = (gy + h2) * cell + 12 * cos(t * 0.23 + h1 * 6.3);
                    batch_sprite(&B[B_BG], sx(c, wx), sy(c, wy), r * (0.6f + h3), 0,
                                 (SDL_Color){150, 210, 200, (uint8_t)(30 + 40 * h2)});
                }
            }
        }
        flush_bg(c, TX_GLOW, SDL_BLENDMODE_ADD);
    }

    /* the rim of the dish */
    batch_begin(&B[B_BG], TX_GLOW, SDL_BLENDMODE_ADD);
    world_edge(c, &B[B_BG], 7, (SDL_Color){90, 200, 200, 40});
    flush_bg(c, TX_GLOW, SDL_BLENDMODE_ADD);
    batch_begin(&B[B_BG], TX_DISC, SDL_BLENDMODE_ADD);
    world_edge(c, &B[B_BG], 0.9f, (SDL_Color){120, 220, 220, 110});
    flush_bg(c, TX_DISC, SDL_BLENDMODE_ADD);
}

static void bio_org(const LookCtx *c, const DOrg *d)
{
    const Look L = LOOK_BIOLOGICAL;
    const Organism *o = d->o;
    float z = (float)c->zoom, h = org_hash(d), cov;
    if (d->rad < 2.5f) {
        SDL_Color k = look_color(L, o->alive ? d->main_col : SYN_DEAD);
        batch_sprite(&B[o->alive ? B_GLOW : B_MEMBRANE], d->cx, d->cy, fmaxf(d->rad * 1.6f, 1.5f),
                     0, rgba(k, o->alive ? 200 : 120));
        return;
    }
    bool detail = z >= 1.5f;
    float hw = stroke_hw(c, 1.7f, 0.6f, &cov);
    if (!o->alive) {
        for (int i = 0; i < d->n; i++) {
            if (detail)
                batch_capsule(&B[B_MEMBRANE], d->x0[i], d->y0[i], d->x1[i], d->y1[i], 2.6f * z,
                              (SDL_Color){90, 70, 50, 18});
            batch_capsule(&B[B_BODY], d->x0[i], d->y0[i], d->x1[i], d->y1[i], hw,
                          rgba(look_color(L, SYN_DEAD), 170 * cov));
        }
        return;
    }

    /* the cell: cytoplasm round the strands inside a membrane, breathing */
    float breath = 1 + 0.07f * sinf((float)c->t * 2.1f + h * 6.283f);
    SDL_Color tint = look_color(L, d->main_col);
    if (z >= 0.8f) {
        float cw = fmaxf(4.6f * z * breath, 3.0f), rim = fmaxf(0.7f * z, 1.2f);
        SDL_Color rimc = rgba(mix(tint, WHITE, 0.45f), 255);
        SDL_Color plasma = rgba(mix(tint, (SDL_Color){10, 26, 28, 255}, 0.5f), 255);
        for (int i = 0; i < d->n; i++) {
            batch_capsule(&B[B_RIM], d->x0[i], d->y0[i], d->x1[i], d->y1[i], cw, rimc);
            batch_capsule(&B[B_PLASMA], d->x0[i], d->y0[i], d->x1[i], d->y1[i], cw - rim, plasma);
        }
    } else {
        batch_sprite(&B[B_MEMBRANE], d->cx, d->cy, (d->rad * 1.25f + 3 * z) * breath, 0,
                     rgba(mix(tint, WHITE, 0.55f), 50));
    }

    for (int i = 0; i < d->n; i++) {
        SDL_Color k = seg_col(L, d, i);
        Role role = seg_role(d, i);
        batch_capsule(&B[B_BODY], d->x0[i], d->y0[i], d->x1[i], d->y1[i], hw, rgba(k, 255 * cov));
        if (detail && hw >= 2 && role != ROLE_INACTIVE)
            batch_capsule(&B[B_LIGHT], d->x0[i], d->y0[i], d->x1[i], d->y1[i], hw * 0.35f,
                          rgba(mix(k, WHITE, 0.5f), 70));
        /* Color Mod's: healers glow; eyes look out from the end of their strand */
        if (detail && role == ROLE_HEAL)
            batch_capsule(&B[B_GLOW], d->x0[i], d->y0[i], d->x1[i], d->y1[i], hw * 2.6f + z,
                          rgba(k, 60));
        if (detail && role == ROLE_SENSE) {
            float r = fmaxf(1.3f * z, 1.5f);
            batch_sprite(&B[B_TOP], d->x1[i], d->y1[i], r * 1.25f, 0, (SDL_Color){235, 240, 225, 230});
            batch_sprite(&B[B_TOP], d->x1[i], d->y1[i], r * 0.7f, 0, (SDL_Color){20, 24, 30, 240});
            batch_sprite(&B[B_TOP], d->x1[i] - r * 0.25f, d->y1[i] - r * 0.25f, r * 0.22f, 0,
                         (SDL_Color){255, 255, 255, 220});
        }
        /* cilia beat along the segments that move it */
        if (z >= 2.5f && role == ROLE_MOVE) {
            float dx = d->x1[i] - d->x0[i], dy = d->y1[i] - d->y0[i];
            float len = sqrtf(dx * dx + dy * dy);
            if (len < 1)
                continue;
            int hairs = (int)fminf(fmaxf(len / (3 * z), 2), 8);
            float nx = -dy / len, ny = dx / len, hl = 2.2f * z, hh = fmaxf(0.22f * z, 0.5f);
            for (int k2 = 0; k2 < hairs; k2++) {
                float p = (k2 + 0.5f) / hairs, side = (k2 & 1) ? -1.0f : 1.0f;
                float wob = 0.6f * sinf((float)c->t * 14 + k2 * 1.3f + h * 10);
                float cs = cosf(wob), sn = sinf(wob);
                float hx = (nx * cs - ny * sn) * side, hy = (nx * sn + ny * cs) * side;
                float bx = d->x0[i] + dx * p, by = d->y0[i] + dy * p;
                batch_capsule(&B[B_BODY], bx, by, bx + hx * hl, by + hy * hl, hh,
                              rgba(look_color(L, d->col[i]), 170));
            }
        }
    }

    /* the nucleus swells with stored energy */
    if (detail) {
        float r = (0.9f + 1.4f * org_charge(d)) * z * (1 + 0.05f * sinf((float)c->t * 3 + h * 9));
        batch_sprite(&B[B_TOP], d->cx, d->cy, r, 0, (SDL_Color){60, 40, 100, 190});
        batch_sprite(&B[B_TOP], d->cx - r * 0.25f, d->cy - r * 0.25f, r * 0.35f, 0,
                     (SDL_Color){200, 170, 255, 90});
    }
    float f = flash(d);
    if (f > 0)
        batch_sprite(&B[B_GLOW], d->cx, d->cy, d->rad * 1.6f + 4, 0,
                     rgba(look_color(L, syn_flash(o)), 90 * f));
    /* a newborn sends out a ripple */
    if (d->fresh > 0)
        batch_sprite(&B[B_RING], d->cx, d->cy, d->rad + 4 + 14 * (1 - d->fresh) * fmaxf(z, 0.5f), 0,
                     (SDL_Color){200, 240, 235, (uint8_t)(140 * d->fresh)});
}

static void flush_cells(const LookCtx *c)
{
    if (B[B_RIM].ni == 0)
        return;
    SDL_Renderer *R = c->R;
    int W, H;
    SDL_GetRendererOutputSize(R, &W, &H);
    if (!cells || cells_w != W || cells_h != H) {
        if (cells)
            SDL_DestroyTexture(cells);
        cells = SDL_CreateTexture(R, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, W, H);
        cells_w = W;
        cells_h = H;
        if (cells)
            SDL_SetTextureBlendMode(cells, SDL_BLENDMODE_BLEND);
    }
    SDL_Texture *prev = SDL_GetRenderTarget(R);
    if (!cells || SDL_SetRenderTarget(R, cells) != 0) {
        /* no render targets: plain translucent strokes */
        for (int i = 0; i < B[B_PLASMA].nv; i++)
            B[B_PLASMA].v[i].color.a = 40;
        B[B_RIM].nv = B[B_RIM].ni = 0;
        batch_flush(&B[B_PLASMA], R);
        return;
    }
    SDL_SetRenderDrawColor(R, 0, 0, 0, 0);
    SDL_RenderClear(R);
    batch_flush(&B[B_RIM], R);
    batch_flush(&B[B_PLASMA], R);
    SDL_SetRenderTarget(R, prev);
    SDL_SetTextureAlphaMod(cells, 120);
    SDL_RenderCopy(R, cells, &c->in, &c->in);
}

static void bio_end(const LookCtx *c, const DOrg *sel)
{
    batch_flush(&B[B_MEMBRANE], c->R);
    flush_cells(c);
    static const int order[] = {B_GLOW, B_BODY, B_LIGHT, B_TOP};
    for (size_t i = 0; i < sizeof order / sizeof order[0]; i++)
        batch_flush(&B[order[i]], c->R);
    if (sel) {
        /* a ring, and a ping going out from it */
        float r = sel->rad + 6, ph = (float)fmod(c->t * 0.8, 1.0);
        batch_sprite(&B[B_RING], sel->cx, sel->cy, r + 2, 0, (SDL_Color){230, 250, 245, 200});
        batch_sprite(&B[B_RING], sel->cx, sel->cy, r + 2 + 14 * ph, 0,
                     (SDL_Color){230, 250, 245, (uint8_t)(120 * (1 - ph))});
    }
    batch_flush(&B[B_RING], c->R); /* with the newborns' ripples */
    vignette(c, 170);
}

/* ---- Galaxy: each organism a galaxy, its segments the arms ---- */

/* Background layers lie on planes behind the world, depth times the
 * camera's height further away, seen by the same camera: they zoom about
 * the same centre, but less, and so move more slowly when panning. */
static double layer_zoom(double zoom, double depth)
{
    return zoom / (1 + depth * zoom);
}

static void nebula_layer(const LookCtx *c, double depth, double off, SDL_Color k)
{
    double s = layer_zoom(c->zoom, depth), tile = 900 * (1 + depth);
    double ccx = c->cam_x + c->in.w / c->zoom / 2, ccy = c->cam_y + c->in.h / c->zoom / 2;
    /* layer coordinates at the panel's top left */
    double nx0 = ccx - c->in.w / 2.0 / s + off + c->t * 1.5, ny0 = ccy - c->in.h / 2.0 / s + off * 0.57;
    double nx1 = nx0 + c->in.w / s, ny1 = ny0 + c->in.h / s;
    batch_begin(&B[B_BG], TX_NEBULA, SDL_BLENDMODE_ADD);
    for (double ty = floor(ny0 / tile) * tile; ty < ny1; ty += tile)
        for (double tx = floor(nx0 / tile) * tile; tx < nx1; tx += tile)
            batch_rect(&B[B_BG], (float)(c->in.x + (tx - nx0) * s), (float)(c->in.y + (ty - ny0) * s),
                       (float)(tile * s), (float)(tile * s), 0, 0, 1, 1, k);
    flush_bg(c, TX_NEBULA, SDL_BLENDMODE_ADD);
}

static void starfield(const LookCtx *c)
{
    static const float depth[3] = {24, 10, 4}, chance[3] = {0.75f, 0.55f, 0.35f};
    double ccx = c->cam_x + c->in.w / c->zoom / 2, ccy = c->cam_y + c->in.h / c->zoom / 2;
    for (int L = 0; L < 3; L++) {
        /* cells about 70 pixels across at 100% */
        double s = layer_zoom(c->zoom, depth[L]), cell = 70 * (1 + depth[L]);
        double ox = ccx - c->in.w / 2.0 / s, oy = ccy - c->in.h / 2.0 / s;
        int gx0 = (int)floor(ox / cell), gy0 = (int)floor(oy / cell);
        int gx1 = (int)floor((ox + c->in.w / s) / cell), gy1 = (int)floor((oy + c->in.h / s) / cell);
        if ((double)(gx1 - gx0 + 1) * (gy1 - gy0 + 1) > 20000)
            continue;
        batch_begin(&B[B_BG], TX_GLOW, SDL_BLENDMODE_ADD);
        Batch *flares = &B[B_STAR];
        for (int gy = gy0; gy <= gy1; gy++) {
            for (int gx = gx0; gx <= gx1; gx++) {
                uint32_t key = (uint32_t)gx * 73856093u ^ (uint32_t)gy * 19349663u ^ (uint32_t)L * 83492791u;
                if (hashf(key, 10) > chance[L])
                    continue;
                float h1 = hashf(key, 11), h2 = hashf(key, 12), b = hashf(key, 13);
                b *= b;
                float x = (float)(c->in.x + ((gx + h1) * cell - ox) * s);
                float y = (float)(c->in.y + ((gy + h2) * cell - oy) * s);
                float tw = 0.65f + 0.35f * sinf((float)c->t * (1 + 2 * h1) + h2 * 20);
                SDL_Color k = h2 < 0.3f   ? (SDL_Color){180, 200, 255, 255}
                              : h2 > 0.8f ? (SDL_Color){255, 225, 180, 255}
                                          : (SDL_Color){235, 235, 255, 255};
                float r = (0.7f + 0.6f * L) * (0.6f + b);
                batch_sprite(&B[B_BG], x, y, r + 0.8f, 0, rgba(k, (60 + 195 * b) * tw));
                if (b > 0.86f && L > 0)
                    batch_sprite(flares, x, y, 4.5f + 3 * L * b, 0, rgba(k, 110 * tw));
            }
        }
        flush_bg(c, TX_GLOW, SDL_BLENDMODE_ADD);
    }
    batch_flush(&B[B_STAR], c->R);
}

static void galaxy_begin(const LookCtx *c)
{
    batch_begin(&B[B_BG], TX_WHITE, SDL_BLENDMODE_BLEND);
    fill((SDL_FRect){(float)c->in.x, (float)c->in.y, (float)c->in.w, (float)c->in.h},
         (SDL_Color){2, 3, 10, 255});
    flush_bg(c, TX_WHITE, SDL_BLENDMODE_BLEND);
    nebula_layer(c, 3, 0, (SDL_Color){255, 255, 255, 110});
    nebula_layer(c, 1.2, 1300, (SDL_Color){150, 200, 255, 60});
    starfield(c);
    shade_outside(c, (SDL_Color){0, 0, 0, 150});
    batch_begin(&B[B_BG], TX_GLOW, SDL_BLENDMODE_ADD);
    world_edge(c, &B[B_BG], 5, (SDL_Color){120, 90, 200, 30});
    flush_bg(c, TX_GLOW, SDL_BLENDMODE_ADD);
    batch_begin(&B[B_BG], TX_DISC, SDL_BLENDMODE_ADD);
    world_edge(c, &B[B_BG], 0.7f, (SDL_Color){140, 110, 220, 80});
    flush_bg(c, TX_DISC, SDL_BLENDMODE_ADD);
}

static void galaxy_org(const LookCtx *c, const DOrg *d)
{
    const Look L = LOOK_GALAXY;
    const Organism *o = d->o;
    float z = (float)c->zoom, h = org_hash(d), cov;
    if (d->rad < 2.5f) {
        if (!o->alive) {
            batch_sprite(&B[B_GLOW], d->cx, d->cy, fmaxf(d->rad * 1.8f, 1.5f), 0,
                         rgba(look_color(L, SYN_DEAD), 90));
            return;
        }
        SDL_Color k = look_color(L, d->main_col);
        batch_sprite(&B[B_GLOW], d->cx, d->cy, fmaxf(d->rad * 2.2f, 2.0f), 0, rgba(k, 220));
        if (d->rad > 1.2f)
            batch_sprite(&B[B_GLOW], d->cx, d->cy, 1.2f, 0, (SDL_Color){255, 255, 255, 200});
        if (org_charge(d) > 0.6f)
            batch_sprite(&B[B_STAR], d->cx, d->cy, d->rad * 5 + 3, 0, rgba(mix(k, WHITE, 0.5f), 90));
        return;
    }
    bool detail = z >= 1.5f;
    if (!o->alive) {
        float hw = stroke_hw(c, 0.6f, 0.5f, &cov);
        for (int i = 0; i < d->n; i++) {
            batch_capsule(&B[B_GLOW], d->x0[i], d->y0[i], d->x1[i], d->y1[i], fmaxf(2 * z, 1),
                          (SDL_Color){110, 70, 55, 35});
            batch_capsule(&B[B_LIGHT], d->x0[i], d->y0[i], d->x1[i], d->y1[i], hw,
                          (SDL_Color){150, 100, 80, (uint8_t)(110 * cov)});
        }
        return;
    }

    float hw = stroke_hw(c, 0.8f, 0.55f, &cov);
    /* light adds up: many arms over each other would burn out to white */
    float dense = fminf(fmaxf(sqrtf(8.0f / d->n), 0.3f), 1);
    int every = d->n > 16 ? 3 : 1;
    if (!detail)
        batch_sprite(&B[B_GLOW], d->cx, d->cy, d->rad * 1.5f, 0,
                     rgba(look_color(L, d->main_col), 90 * dense));
    for (int i = 0; i < d->n; i++) {
        SDL_Color k = seg_col(L, d, i);
        Role role = seg_role(d, i);
        /* Color Mod's segments out of action are dark lanes in the arms */
        float lit = role == ROLE_INACTIVE ? 0.45f : 1;
        if (detail)
            batch_capsule(&B[B_GLOW], d->x0[i], d->y0[i], d->x1[i], d->y1[i],
                          fmaxf((role == ROLE_HEAL ? 5.0f : 3.2f) * z, 1.5f), rgba(k, 95 * dense * lit));
        batch_capsule(&B[B_LIGHT], d->x0[i], d->y0[i], d->x1[i], d->y1[i], hw,
                      rgba(mix(k, WHITE, 0.2f), 230 * cov * fmaxf(dense, 0.6f) * lit));
        /* eyes: a bright star that winks */
        if (role == ROLE_SENSE && z >= 1) {
            float wink = 0.55f + 0.45f * sinf((float)c->t * 5 + hashf(o->id, i) * 30);
            batch_sprite(&B[B_STAR], d->x1[i], d->y1[i], 3.5f * z, 0, rgba(mix(k, WHITE, 0.7f), 220 * wink));
        }
        /* stars along the arms */
        if (z >= 2 && i % every == 0 && role != ROLE_INACTIVE) {
            float tw = 0.6f + 0.4f * sinf((float)c->t * (2 + 3 * hashf(o->id, i)) + i);
            batch_sprite(&B[B_STAR], d->x1[i], d->y1[i], 2.2f * z, 0,
                         rgba(mix(k, WHITE, 0.5f), 130 * tw * dense));
        }
    }
    /* the core, brighter the more energy it holds, in proportion to the body */
    float e = org_charge(d), pulse = 0.85f + 0.15f * sinf((float)c->t * 2.5f + h * 9);
    SDL_Color core = mix(look_color(L, d->main_col), WHITE, 0.45f);
    batch_sprite(&B[B_GLOW], d->cx, d->cy, fminf((7 + 7 * e) * z, d->rad * 1.3f), 0, rgba(core, 70));
    batch_sprite(&B[B_STAR], d->cx, d->cy, fmaxf(fminf((3 + 4 * e) * z, d->rad * (0.3f + 0.25f * e)), 2.5f),
                 0, rgba(core, 230 * pulse));
    if (d->fresh > 0)
        batch_sprite(&B[B_STAR], d->cx, d->cy, d->rad + (4 + 26 * (1 - d->fresh)) * fmaxf(z, 0.5f), 0,
                     rgba(core, 150 * d->fresh));
    float f = flash(d);
    if (f > 0)
        batch_sprite(&B[B_GLOW], d->cx, d->cy, d->rad * 2 + 6, 0,
                     rgba(look_color(L, syn_flash(o)), 140 * f));
}

static void galaxy_end(const LookCtx *c, const DOrg *sel)
{
    batch_flush(&B[B_GLOW], c->R);
    batch_flush(&B[B_LIGHT], c->R);
    batch_flush(&B[B_STAR], c->R);
    if (sel) {
        float r = sel->rad + 8;
        batch_sprite(&B[B_RING], sel->cx, sel->cy, r, 0, (SDL_Color){200, 180, 255, 170});
        batch_flush(&B[B_RING], c->R);
        for (int k = 0; k < 4; k++) {
            float a = (float)c->t * 0.6f + k * (float)M_PI / 2, cs = cosf(a), sn = sinf(a);
            batch_stroke(&B[B_SOLID_ADD], sel->cx + cs * (r + 3), sel->cy + sn * (r + 3),
                         sel->cx + cs * (r + 8), sel->cy + sn * (r + 8), 0.8f,
                         (SDL_Color){200, 180, 255, 200});
        }
        batch_flush(&B[B_SOLID_ADD], c->R);
    }
    vignette(c, 120);
}

/* ---- Digital: viruses on a SYN-OS screen ---- */

static void grid(const LookCtx *c, double step, float hw, SDL_Color k)
{
    if (step * c->zoom < 14)
        return;
    SDL_FRect wr = world_rect(c);
    float L = fmaxf(wr.x, (float)c->in.x), R = fminf(wr.x + wr.w, (float)(c->in.x + c->in.w));
    float T = fmaxf(wr.y, (float)c->in.y), Bt = fminf(wr.y + wr.h, (float)(c->in.y + c->in.h));
    if (R <= L || Bt <= T)
        return;
    double x0 = fmax(c->cam_x, 0), y0 = fmax(c->cam_y, 0);
    double x1 = fmin(c->cam_x + c->in.w / c->zoom, c->w->width);
    double y1 = fmin(c->cam_y + c->in.h / c->zoom, c->w->height);
    for (double x = ceil(x0 / step) * step; x <= x1; x += step)
        batch_stroke(&B[B_BG], sx(c, x), T, sx(c, x), Bt, hw, k);
    for (double y = ceil(y0 / step) * step; y <= y1; y += step)
        batch_stroke(&B[B_BG], L, sy(c, y), R, sy(c, y), hw, k);
}

static void digital_begin(const LookCtx *c)
{
    batch_begin(&B[B_BG], TX_WHITE, SDL_BLENDMODE_BLEND);
    fill((SDL_FRect){(float)c->in.x, (float)c->in.y, (float)c->in.w, (float)c->in.h},
         (SDL_Color){0, 0, 0, 255});
    fill(world_rect(c), (SDL_Color){8, 2, 2, 255});
    flush_bg(c, TX_WHITE, SDL_BLENDMODE_BLEND);
    tile_world(c, TX_GRIME, SDL_BLENDMODE_ADD, 512, 0, 0, (SDL_Color){70, 10, 10, 255});

    batch_begin(&B[B_BG], TX_WHITE, SDL_BLENDMODE_BLEND);
    grid(c, 8, 0.5f, (SDL_Color){51, 0, 0, 50});
    grid(c, 64, 0.5f, (SDL_Color){77, 0, 0, 80});
    grid(c, 512, 0.6f, (SDL_Color){138, 20, 20, 120});
    flush_bg(c, TX_WHITE, SDL_BLENDMODE_BLEND);

    batch_begin(&B[B_BG], TX_GLOW, SDL_BLENDMODE_ADD);
    world_edge(c, &B[B_BG], 6, (SDL_Color){224, 0, 0, 60});
    flush_bg(c, TX_GLOW, SDL_BLENDMODE_ADD);
    batch_begin(&B[B_BG], TX_WHITE, SDL_BLENDMODE_ADD);
    world_edge(c, &B[B_BG], 0.75f, (SDL_Color){224, 0, 0, 255});
    /* gold brackets at the corners */
    SDL_FRect r = world_rect(c);
    SDL_Color gold = {224, 200, 150, 220};
    for (int k = 0; k < 4; k++) {
        float x = (k & 1) ? r.x + r.w : r.x, y = (k & 2) ? r.y + r.h : r.y;
        float dx = (k & 1) ? -24 : 24, dy = (k & 2) ? -24 : 24;
        batch_stroke(&B[B_BG], x, y, x + dx, y, 1.5f, gold);
        batch_stroke(&B[B_BG], x, y, x, y + dy, 1.5f, gold);
    }
    flush_bg(c, TX_WHITE, SDL_BLENDMODE_ADD);
}

static void digital_org(const LookCtx *c, const DOrg *d)
{
    const Look L = LOOK_DIGITAL;
    const Organism *o = d->o;
    float z = (float)c->zoom, cov;
    if (d->rad < 2.5f) {
        float s = fmaxf(d->rad * 2, 2);
        SDL_Color k = look_color(L, o->alive ? d->main_col : SYN_DEAD);
        batch_rect(&B[B_SOLID_ADD], d->cx - s / 2, d->cy - s / 2, s, s, 0, 0, 1, 1,
                   rgba(k, o->alive ? 230 : 150));
        return;
    }
    bool detail = z >= 1.5f;
    float hw = stroke_hw(c, 0.9f, 0.55f, &cov);
    if (!o->alive) {
        /* corrupted: broken into flickering dashes */
        float fl = hashf((uint32_t)(c->t * 12), (uint32_t)o->id) < 0.2f ? 0.55f : 1;
        SDL_Color k = rgba(look_color(L, SYN_DEAD), 170 * cov * fl);
        for (int i = 0; i < d->n; i++) {
            float dx = d->x1[i] - d->x0[i], dy = d->y1[i] - d->y0[i];
            int dashes = z >= 2 ? (int)fmaxf(sqrtf(dx * dx + dy * dy) / (2.5f * z), 1) : 1;
            for (int k2 = 0; k2 < dashes; k2++) {
                float a = (float)k2 / dashes, b = (k2 + (dashes > 1 ? 0.6f : 1)) / dashes;
                batch_capsule(&B[B_LIGHT], d->x0[i] + dx * a, d->y0[i] + dy * a, d->x0[i] + dx * b,
                              d->y0[i] + dy * b, hw, k);
            }
        }
        return;
    }

    if (!detail)
        batch_sprite(&B[B_GLOW], d->cx, d->cy, d->rad * 1.4f, 0, rgba(look_color(L, d->main_col), 70));
    float f = flash(d);
    float jx = 0, jy = 0;
    if (f > 0) {
        /* a hit glitches it: shaken, with its red and cyan split apart */
        uint32_t slot = (uint32_t)(c->t * 30);
        jx = (hashf(slot, (uint32_t)o->id) - 0.5f) * 3 * f;
        jy = (hashf(slot, (uint32_t)o->id + 7) - 0.5f) * 3 * f;
    }
    for (int i = 0; i < d->n; i++) {
        SDL_Color k = seg_col(L, d, i);
        Role role = seg_role(d, i);
        float x0 = d->x0[i] + jx, y0 = d->y0[i] + jy, x1 = d->x1[i] + jx, y1 = d->y1[i] + jy;
        if (role == ROLE_INACTIVE) {
            /* Color Mod's segments out of action: offline, a dotted trace */
            float dx = x1 - x0, dy = y1 - y0;
            int dots = (int)fmaxf(sqrtf(dx * dx + dy * dy) / fmaxf(2 * z, 2), 1);
            for (int k2 = 0; k2 < dots; k2++) {
                float a = (k2 + 0.25f) / dots, b = (k2 + 0.6f) / dots;
                batch_capsule(&B[B_LIGHT], x0 + dx * a, y0 + dy * a, x0 + dx * b, y0 + dy * b, hw,
                              rgba(k, 150 * cov));
            }
            continue;
        }
        if (detail)
            batch_capsule(&B[B_GLOW], x0, y0, x1, y1, fmaxf(1.8f * z, 1.5f), rgba(k, 70));
        batch_capsule(&B[B_LIGHT], x0, y0, x1, y1, hw, rgba(k, 255 * cov));
        /* eyes: a target reticle; healers: a plus at the middle */
        if (role == ROLE_SENSE && z >= 1.5f) {
            float r = 2.2f * z;
            batch_sprite(&B[B_RING], x1, y1, r, 0, rgba(mix(k, WHITE, 0.4f), 220));
            batch_stroke(&B[B_SOLID_ADD], x1 - r * 1.5f, y1, x1 - r * 0.6f, y1, 0.5f, rgba(k, 200));
            batch_stroke(&B[B_SOLID_ADD], x1 + r * 0.6f, y1, x1 + r * 1.5f, y1, 0.5f, rgba(k, 200));
        }
        if (role == ROLE_HEAL && z >= 2.5f) {
            float mxp = (x0 + x1) / 2, myp = (y0 + y1) / 2, a = 1.1f * z;
            batch_stroke(&B[B_SOLID_ADD], mxp - a, myp, mxp + a, myp, 0.6f, rgba(mix(k, WHITE, 0.5f), 230));
            batch_stroke(&B[B_SOLID_ADD], mxp, myp - a, mxp, myp + a, 0.6f, rgba(mix(k, WHITE, 0.5f), 230));
        }
        if (f > 0) {
            float off = 1.5f + 0.3f * z;
            batch_capsule(&B[B_LIGHT], x0 + off, y0, x1 + off, y1, hw,
                          (SDL_Color){255, 0, 40, (uint8_t)(120 * f)});
            batch_capsule(&B[B_LIGHT], x0 - off, y0, x1 - off, y1, hw,
                          (SDL_Color){0, 220, 255, (uint8_t)(120 * f)});
        }
        if (z >= 2.5f) {
            float s = 1.1f * z;
            batch_rect(&B[B_SOLID_ADD], x1 - s / 2, y1 - s / 2, s, s, 0, 0, 1, 1,
                       rgba(mix(k, WHITE, 0.4f), 200));
        }
    }
    if (detail) {
        float r = (0.9f + 0.9f * org_charge(d)) * z;
        batch_sprite(&B[B_SOLID_ADD], d->cx + jx, d->cy + jy, r, (float)(M_PI / 4 + o->theta),
                     (SDL_Color){255, 40, 40, 230});
    }
    /* newborns: a square opening out from the centre */
    if (d->fresh > 0) {
        float p = 1 - d->fresh, s = d->rad + p * 14 * fmaxf(z, 1);
        SDL_Color k = {224, 200, 150, (uint8_t)(200 * d->fresh)};
        float x0 = d->cx - s, y0 = d->cy - s, x1 = d->cx + s, y1 = d->cy + s;
        batch_stroke(&B[B_SOLID_ADD], x0, y0, x1, y0, 0.6f, k);
        batch_stroke(&B[B_SOLID_ADD], x0, y1, x1, y1, 0.6f, k);
        batch_stroke(&B[B_SOLID_ADD], x0, y0, x0, y1, 0.6f, k);
        batch_stroke(&B[B_SOLID_ADD], x1, y0, x1, y1, 0.6f, k);
    }
}

static void digital_end(const LookCtx *c, const DOrg *sel)
{
    batch_flush(&B[B_GLOW], c->R);
    batch_flush(&B[B_LIGHT], c->R);
    batch_flush(&B[B_SOLID_ADD], c->R);
    batch_flush(&B[B_RING], c->R); /* Color Mod's eyes */
    if (sel) {
        float r = sel->rad + 6, a = 7;
        SDL_Color red = {255, 48, 48, 255};
        for (int k = 0; k < 4; k++) {
            float x = sel->cx + ((k & 1) ? r : -r), y = sel->cy + ((k & 2) ? r : -r);
            float dx = (k & 1) ? -a : a, dy = (k & 2) ? -a : a;
            batch_stroke(&B[B_SOLID], x, y, x + dx, y, 1, red);
            batch_stroke(&B[B_SOLID], x, y, x, y + dy, 1, red);
        }
        batch_flush(&B[B_SOLID], c->R);
        char label[96];
        snprintf(label, sizeof label, "ORG %d  E %.1f  GEN %d", sel->o->id, sel->o->energy,
                 syn_generation(sel->o));
        gfx_text((int)(sel->cx + r + 4), (int)(sel->cy - r - gfx_font_h() / 2), label,
                 (Col){224, 200, 150, 255});
    }

    /* scanlines, a slow sweep, and now and then a glitch */
    batch_begin(&B[B_BG], TX_WHITE, SDL_BLENDMODE_BLEND);
    for (int y = c->in.y; y < c->in.y + c->in.h; y += 3)
        fill((SDL_FRect){(float)c->in.x, (float)y, (float)c->in.w, 1}, (SDL_Color){0, 0, 0, 46});
    flush_bg(c, TX_WHITE, SDL_BLENDMODE_BLEND);
    float band = (float)fmod(c->t * 60, c->in.h + 200) - 100;
    batch_begin(&B[B_BG], TX_GLOW, SDL_BLENDMODE_ADD);
    fill((SDL_FRect){c->in.x - c->in.w * 0.5f, c->in.y + band - 50, c->in.w * 2.0f, 100},
         (SDL_Color){255, 32, 32, 14});
    flush_bg(c, TX_GLOW, SDL_BLENDMODE_ADD);
    uint32_t slot = (uint32_t)(c->t * 8);
    if (hashf(slot, 99) < 0.12f) {
        batch_begin(&B[B_BG], TX_WHITE, SDL_BLENDMODE_ADD);
        int bars = 1 + (int)(hashf(slot, 98) * 3);
        for (int k = 0; k < bars; k++) {
            float y = c->in.y + hashf(slot, 100 + k) * c->in.h, hgt = 1 + hashf(slot, 110 + k) * 2;
            float x = c->in.x + hashf(slot, 120 + k) * c->in.w * 0.7f;
            float w = c->in.w * (0.1f + 0.3f * hashf(slot, 130 + k));
            fill((SDL_FRect){x, y, w, hgt}, (SDL_Color){255, 32, 32, 60});
            fill((SDL_FRect){x + 3, y + hgt, w * 0.6f, 1}, (SDL_Color){0, 200, 255, 40});
        }
        flush_bg(c, TX_WHITE, SDL_BLENDMODE_ADD);
    }
    vignette(c, 140);
}

/* ---- dispatch ---- */

void look_begin(Look l, const LookCtx *c)
{
    static const struct {
        int b;
        TexId t;
        SDL_BlendMode m;
    } kinds[] = {
        {B_MEMBRANE, TX_GLOW, SDL_BLENDMODE_BLEND}, {B_GLOW, TX_GLOW, SDL_BLENDMODE_ADD},
        {B_BODY, TX_DISC, SDL_BLENDMODE_BLEND},     {B_LIGHT, TX_DISC, SDL_BLENDMODE_ADD},
        {B_STAR, TX_STAR, SDL_BLENDMODE_ADD},       {B_TOP, TX_DISC, SDL_BLENDMODE_BLEND},
        {B_RING, TX_RING, SDL_BLENDMODE_BLEND},     {B_SOLID, TX_WHITE, SDL_BLENDMODE_BLEND},
        {B_SOLID_ADD, TX_WHITE, SDL_BLENDMODE_ADD}, {B_RIM, TX_DISC, SDL_BLENDMODE_BLEND},
        {B_PLASMA, TX_DISC, SDL_BLENDMODE_BLEND},
    };
    for (size_t i = 0; i < sizeof kinds / sizeof kinds[0]; i++)
        batch_begin(&B[kinds[i].b], kinds[i].t, kinds[i].m);
    switch (l) {
    case LOOK_BIOLOGICAL:
        bio_begin(c);
        break;
    case LOOK_GALAXY:
        galaxy_begin(c);
        break;
    case LOOK_DIGITAL:
        digital_begin(c);
        break;
    default:
        break;
    }
}

void look_org(Look l, const LookCtx *c, const DOrg *d)
{
    switch (l) {
    case LOOK_BIOLOGICAL:
        bio_org(c, d);
        break;
    case LOOK_GALAXY:
        galaxy_org(c, d);
        break;
    case LOOK_DIGITAL:
        digital_org(c, d);
        break;
    default:
        break;
    }
}

void look_end(Look l, const LookCtx *c, const DOrg *sel)
{
    switch (l) {
    case LOOK_BIOLOGICAL:
        bio_end(c, sel);
        break;
    case LOOK_GALAXY:
        galaxy_end(c, sel);
        break;
    case LOOK_DIGITAL:
        digital_end(c, sel);
        break;
    default:
        break;
    }
}
