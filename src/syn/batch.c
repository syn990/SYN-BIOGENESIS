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

#include "syn/batch.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

static SDL_Texture *tex[TX_COUNT];

/* ---- procedural textures ---- */

static uint32_t hash3(uint32_t x, uint32_t y, uint32_t s)
{
    uint32_t h = x * 0x8da6b343u ^ y * 0xd8163841u ^ s * 0xcb1ab31fu;
    h ^= h >> 16;
    h *= 0x7feb352du;
    h ^= h >> 15;
    h *= 0x846ca68bu;
    h ^= h >> 16;
    return h;
}

static float lattice(int x, int y, int period, uint32_t seed)
{
    x = ((x % period) + period) % period;
    y = ((y % period) + period) % period;
    return (hash3((uint32_t)x, (uint32_t)y, seed) & 0xffffff) / 16777215.0f;
}

/* value noise that repeats every `period` lattice cells */
static float vnoise(float x, float y, int period, uint32_t seed)
{
    int ix = (int)floorf(x), iy = (int)floorf(y);
    float fx = x - ix, fy = y - iy;
    fx = fx * fx * (3 - 2 * fx);
    fy = fy * fy * (3 - 2 * fy);
    float a = lattice(ix, iy, period, seed), b = lattice(ix + 1, iy, period, seed);
    float c = lattice(ix, iy + 1, period, seed), d = lattice(ix + 1, iy + 1, period, seed);
    float top = a + (b - a) * fx, bot = c + (d - c) * fx;
    return top + (bot - top) * fy;
}

/* fractal noise over an n x n texture that tiles: octave o has base << o
 * cells across it */
static float fbm(int px, int py, int n, int base, int octaves, uint32_t seed)
{
    float sum = 0, amp = 0.5f, norm = 0;
    for (int o = 0; o < octaves; o++) {
        int period = base << o;
        float k = (float)period / n;
        sum += amp * vnoise(px * k, py * k, period, seed + (uint32_t)o * 977u);
        norm += amp;
        amp *= 0.5f;
    }
    return sum / norm;
}

static float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

static float smooth(float e0, float e1, float x)
{
    float t = clampf((x - e0) / (e1 - e0), 0, 1);
    return t * t * (3 - 2 * t);
}

static uint32_t px_argb(float a, float r, float g, float b)
{
    return (uint32_t)lroundf(clampf(a, 0, 1) * 255) << 24 |
           (uint32_t)lroundf(clampf(r, 0, 1) * 255) << 16 |
           (uint32_t)lroundf(clampf(g, 0, 1) * 255) << 8 | (uint32_t)lroundf(clampf(b, 0, 1) * 255);
}

static SDL_Texture *make(SDL_Renderer *r, int n, const uint32_t *pix)
{
    SDL_Texture *t = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, n, n);
    if (!t)
        return NULL;
    SDL_UpdateTexture(t, NULL, pix, n * 4);
    SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(t, SDL_ScaleModeLinear);
    return t;
}

static void gen(TexId id, int n, uint32_t *p)
{
    float c = (n - 1) / 2.0f;
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            float dx = x - c, dy = y - c, d = sqrtf(dx * dx + dy * dy), R = n / 2.0f;
            float a = 0, cr = 1, cg = 1, cb = 1;
            switch (id) {
            case TX_WHITE:
                a = 1;
                break;
            case TX_DISC:
                /* edge about 3 texels wide: at small sizes the whole disc
                 * is a soft bump, which antialiases thin strokes */
                a = clampf((R - 1.5f - d) / 3.0f + 0.5f, 0, 1);
                break;
            case TX_GLOW: {
                float q = clampf(1 - (d / R) * (d / R), 0, 1);
                a = q * q * q;
                break;
            }
            case TX_STAR: {
                float core = expf(-d * d / (2 * 2.2f * 2.2f));
                float halo = powf(clampf(1 - d / R, 0, 1), 3) * 0.45f;
                float ax = fabsf(dx), ay = fabsf(dy);
                float spikes = (expf(-ay / 0.9f) * powf(clampf(1 - ax / R, 0, 1), 2) +
                                expf(-ax / 0.9f) * powf(clampf(1 - ay / R, 0, 1), 2)) *
                               0.55f;
                a = clampf(core + halo + spikes, 0, 1);
                break;
            }
            case TX_RING:
                a = clampf(1 - fabsf(d - (R - 4)) / 1.8f, 0, 1);
                break;
            case TX_NOISE:
                a = fbm(x, y, n, 4, 5, 11);
                break;
            case TX_GRIME: {
                /* the SYN-OS wallpaper: dark blotches with fine speckle */
                float f = fbm(x, y, n, 6, 5, 23);
                float speck = (hash3((uint32_t)x, (uint32_t)y, 5) & 0xff) / 255.0f;
                a = smooth(0.50f, 0.72f, f) * (0.35f + 0.65f * speck);
                break;
            }
            case TX_NEBULA: {
                float f = fbm(x, y, n, 3, 6, 31), g = fbm(x, y, n, 2, 4, 47);
                float dens = powf(smooth(0.40f, 0.85f, f), 1.6f);
                /* violet, blue and magenta clouds */
                float t = smooth(0.3f, 0.7f, g);
                cr = 0.45f + 0.35f * t;
                cg = 0.25f + 0.12f * (1 - t);
                cb = 0.85f - 0.35f * t;
                a = dens;
                break;
            }
            case TX_VIGNETTE:
                cr = cg = cb = 0;
                a = powf(smooth(0.45f, 1.05f, d / (R * 1.4142f)), 1.3f) * 0.9f;
                break;
            case TX_COUNT:
                break;
            }
            p[y * n + x] = px_argb(a, cr, cg, cb);
        }
    }
}

bool batch_textures_init(SDL_Renderer *r)
{
    static const int size[TX_COUNT] = {4, 64, 64, 64, 64, 256, 256, 512, 128};
    uint32_t *pix = malloc(512 * 512 * sizeof *pix);
    if (!pix)
        return false;
    bool ok = true;
    for (int i = 0; i < TX_COUNT; i++) {
        gen((TexId)i, size[i], pix);
        tex[i] = make(r, size[i], pix);
        ok = ok && tex[i];
    }
    free(pix);
    return ok;
}

void batch_textures_free(void)
{
    for (int i = 0; i < TX_COUNT; i++) {
        if (tex[i])
            SDL_DestroyTexture(tex[i]);
        tex[i] = NULL;
    }
}

/* ---- batches ---- */

void batch_begin(Batch *b, TexId t, SDL_BlendMode blend)
{
    b->nv = b->ni = 0;
    b->tex = t;
    b->blend = blend;
}

void batch_flush(Batch *b, SDL_Renderer *r)
{
    if (b->ni > 0 && tex[b->tex]) {
        SDL_SetTextureBlendMode(tex[b->tex], b->blend);
        SDL_RenderGeometry(r, tex[b->tex], b->v, b->nv, b->idx, b->ni);
    }
    b->nv = b->ni = 0;
}

void batch_free(Batch *b)
{
    free(b->v);
    free(b->idx);
    b->v = NULL;
    b->idx = NULL;
    b->nv = b->ni = b->cap_v = b->cap_i = 0;
}

static void reserve(Batch *b, int nv, int ni)
{
    if (b->nv + nv > b->cap_v) {
        int cap = b->cap_v ? b->cap_v * 2 : 4096;
        while (cap < b->nv + nv)
            cap *= 2;
        SDL_Vertex *v = realloc(b->v, (size_t)cap * sizeof *v);
        if (!v)
            abort();
        b->v = v;
        b->cap_v = cap;
    }
    if (b->ni + ni > b->cap_i) {
        int cap = b->cap_i ? b->cap_i * 2 : 6144;
        while (cap < b->ni + ni)
            cap *= 2;
        int *idx = realloc(b->idx, (size_t)cap * sizeof *idx);
        if (!idx)
            abort();
        b->idx = idx;
        b->cap_i = cap;
    }
}

static inline void vert(Batch *b, float x, float y, SDL_Color c, float u, float v)
{
    b->v[b->nv++] = (SDL_Vertex){{x, y}, c, {u, v}};
}

/* two triangles over vertices a, a+1 (one edge) and a+2, a+3 (the next) */
static inline void strip_quad(Batch *b, int a)
{
    int *i = b->idx + b->ni;
    i[0] = a;
    i[1] = a + 1;
    i[2] = a + 2;
    i[3] = a + 1;
    i[4] = a + 3;
    i[5] = a + 2;
    b->ni += 6;
}

void batch_sprite(Batch *b, float x, float y, float r, float angle, SDL_Color c)
{
    reserve(b, 4, 6);
    int a = b->nv;
    float cs = r, sn = 0;
    if (angle != 0) {
        cs = r * cosf(angle);
        sn = r * sinf(angle);
    }
    /* corners (-1,-1) (-1,1) (1,-1) (1,1), turned */
    vert(b, x - cs + sn, y - sn - cs, c, 0, 0);
    vert(b, x - cs - sn, y - sn + cs, c, 0, 1);
    vert(b, x + cs + sn, y + sn - cs, c, 1, 0);
    vert(b, x + cs - sn, y + sn + cs, c, 1, 1);
    strip_quad(b, a);
}

void batch_rect(Batch *b, float x, float y, float w, float h, float u0, float v0, float u1,
                float v1, SDL_Color c)
{
    reserve(b, 4, 6);
    int a = b->nv;
    vert(b, x, y, c, u0, v0);
    vert(b, x, y + h, c, u0, v1);
    vert(b, x + w, y, c, u1, v0);
    vert(b, x + w, y + h, c, u1, v1);
    strip_quad(b, a);
}

void batch_capsule(Batch *b, float x0, float y0, float x1, float y1, float hw, SDL_Color c)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1e-3f) {
        batch_sprite(b, x0, y0, hw, 0, c);
        return;
    }
    float ux = dx / len * hw, uy = dy / len * hw; /* along, hw long */
    float nx = -uy, ny = ux;                      /* across */
    float px[4] = {x0 - ux, x0, x1, x1 + ux}, py[4] = {y0 - uy, y0, y1, y1 + uy};
    static const float uu[4] = {0, 0.5f, 0.5f, 1};
    reserve(b, 8, 18);
    int a = b->nv;
    for (int i = 0; i < 4; i++) {
        vert(b, px[i] + nx, py[i] + ny, c, uu[i], 0);
        vert(b, px[i] - nx, py[i] - ny, c, uu[i], 1);
    }
    for (int i = 0; i < 3; i++)
        strip_quad(b, a + 2 * i);
}

void batch_stroke(Batch *b, float x0, float y0, float x1, float y1, float hw, SDL_Color c)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1e-3f)
        return;
    float nx = -dy / len * hw, ny = dx / len * hw;
    reserve(b, 4, 6);
    int a = b->nv;
    vert(b, x0 + nx, y0 + ny, c, 0.5f, 0);
    vert(b, x0 - nx, y0 - ny, c, 0.5f, 1);
    vert(b, x1 + nx, y1 + ny, c, 0.5f, 0);
    vert(b, x1 - nx, y1 - ny, c, 0.5f, 1);
    strip_quad(b, a);
}
