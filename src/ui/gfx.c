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

#include "ui/gfx.h"

#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

const Col C_BG = {24, 24, 26, 255};
const Col C_PANEL = {40, 41, 44, 255};
const Col C_PANEL2 = {52, 53, 57, 255};
const Col C_BORDER = {78, 80, 86, 255};
const Col C_TEXT = {226, 226, 228, 255};
const Col C_TEXT_DIM = {150, 152, 158, 255};
const Col C_ACCENT = {90, 150, 230, 255};
const Col C_HOVER = {66, 68, 74, 255};
const Col C_PRESS = {86, 90, 98, 255};
const Col C_FIELD = {28, 29, 32, 255};
const Col C_DISABLED = {96, 98, 104, 255};
const Col C_ORANGE = {255, 200, 0, 255}; /* java.awt.Color.ORANGE: the selection box */

static SDL_Renderer *R;
static TTF_Font *font;
static int font_h;

/* ---- text cache: string -> white texture, tinted per draw ---- */

typedef struct TextEntry {
    struct TextEntry *next;
    uint32_t hash;
    SDL_Texture *tex;
    int w, h;
    unsigned last_used;
    char s[];
} TextEntry;

#define TEXT_BUCKETS 1024
static TextEntry *buckets[TEXT_BUCKETS];
static unsigned frame_no;
static int n_entries;

#define CLIP_DEPTH 16
static SDL_Rect clip_stack[CLIP_DEPTH];
static int clip_n;

#define MAX_ICONS 32
static struct {
    char name[48];
    SDL_Texture *tex;
} icons[MAX_ICONS];
static int n_icons;

static char asset_root[1024];

static bool is_dir(const char *p)
{
    struct stat st;
    return stat(p, &st) == 0 && S_ISDIR(st.st_mode);
}

static void find_assets(void)
{
    const char *env = getenv("SYN_BIOGENESIS_DATA");
    char cand[1100];
    if (env && *env) {
        snprintf(cand, sizeof cand, "%s/fonts", env);
        if (is_dir(cand)) {
            snprintf(asset_root, sizeof asset_root, "%s", env);
            return;
        }
    }
    char *base = SDL_GetBasePath();
    const char *rels[] = {"assets", "../assets", "../share/syn-biogenesis/assets"};
    for (size_t i = 0; base && i < sizeof rels / sizeof rels[0]; i++) {
        snprintf(cand, sizeof cand, "%s%s/fonts", base, rels[i]);
        if (is_dir(cand)) {
            snprintf(asset_root, sizeof asset_root, "%s%s", base, rels[i]);
            SDL_free(base);
            return;
        }
    }
    SDL_free(base);
    if (is_dir("assets/fonts")) {
        snprintf(asset_root, sizeof asset_root, "assets");
        return;
    }
    if (is_dir("/usr/share/syn-biogenesis/assets/fonts"))
        snprintf(asset_root, sizeof asset_root, "/usr/share/syn-biogenesis/assets");
}

const char *gfx_asset(const char *rel)
{
    static char path[1200];
    if (!asset_root[0])
        return NULL;
    snprintf(path, sizeof path, "%s/%s", asset_root, rel);
    struct stat st;
    return stat(path, &st) == 0 ? path : NULL;
}

bool gfx_init(SDL_Renderer *r)
{
    R = r;
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
        return false;
    }
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
    find_assets();
    const char *fp = gfx_asset("fonts/NotoSans-Regular.ttf");
    const char *fallbacks[] = {fp, "/usr/share/fonts/noto/NotoSans-Regular.ttf",
                               "/usr/share/fonts/TTF/DejaVuSans.ttf",
                               "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"};
    for (size_t i = 0; i < sizeof fallbacks / sizeof fallbacks[0] && !font; i++)
        if (fallbacks[i])
            font = TTF_OpenFont(fallbacks[i], 14);
    if (!font) {
        fprintf(stderr, "no usable font found (set SYN_BIOGENESIS_DATA to the assets dir)\n");
        return false;
    }
    TTF_SetFontHinting(font, TTF_HINTING_LIGHT);
    font_h = TTF_FontHeight(font);
    return true;
}

static void cache_clear(bool all)
{
    for (int b = 0; b < TEXT_BUCKETS; b++) {
        TextEntry **pp = &buckets[b];
        while (*pp) {
            TextEntry *e = *pp;
            if (all || frame_no - e->last_used > 120) {
                *pp = e->next;
                SDL_DestroyTexture(e->tex);
                free(e);
                n_entries--;
            } else {
                pp = &e->next;
            }
        }
    }
}

void gfx_shutdown(void)
{
    cache_clear(true);
    for (int i = 0; i < n_icons; i++)
        if (icons[i].tex)
            SDL_DestroyTexture(icons[i].tex);
    n_icons = 0;
    if (font)
        TTF_CloseFont(font);
    font = NULL;
    IMG_Quit();
    TTF_Quit();
}

void gfx_end_frame(void)
{
    frame_no++;
    if (frame_no % 60 == 0 && n_entries > 256)
        cache_clear(false);
}

SDL_Renderer *gfx_renderer(void)
{
    return R;
}

static uint32_t fnv(const char *s)
{
    uint32_t h = 2166136261u;
    for (; *s; s++)
        h = (h ^ (uint8_t)*s) * 16777619u;
    return h;
}

static TextEntry *text_get(const char *s)
{
    if (!s || !*s)
        return NULL;
    uint32_t h = fnv(s);
    TextEntry *e;
    for (e = buckets[h % TEXT_BUCKETS]; e; e = e->next)
        if (e->hash == h && !strcmp(e->s, s))
            break;
    if (!e) {
        SDL_Surface *surf = TTF_RenderUTF8_Blended(font, s, (SDL_Color){255, 255, 255, 255});
        if (!surf)
            return NULL;
        size_t len = strlen(s);
        e = malloc(sizeof *e + len + 1);
        if (!e)
            abort();
        memcpy(e->s, s, len + 1);
        e->hash = h;
        e->tex = SDL_CreateTextureFromSurface(R, surf);
        e->w = surf->w;
        e->h = surf->h;
        SDL_FreeSurface(surf);
        e->next = buckets[h % TEXT_BUCKETS];
        buckets[h % TEXT_BUCKETS] = e;
        n_entries++;
    }
    e->last_used = frame_no;
    return e;
}

int gfx_font_h(void)
{
    return font_h;
}

int gfx_text_w(const char *s)
{
    TextEntry *e = text_get(s);
    return e ? e->w : 0;
}

void gfx_text(int x, int y, const char *s, Col c)
{
    TextEntry *e = text_get(s);
    if (!e || !e->tex)
        return;
    SDL_SetTextureColorMod(e->tex, c.r, c.g, c.b);
    SDL_SetTextureAlphaMod(e->tex, c.a);
    SDL_Rect d = {x, y, e->w, e->h};
    SDL_RenderCopy(R, e->tex, NULL, &d);
}

void gfx_text_in(SDL_Rect r, const char *s, Col c, bool centre)
{
    int w = gfx_text_w(s);
    int x = centre ? r.x + (r.w - w) / 2 : r.x + 6;
    gfx_text(x, r.y + (r.h - font_h) / 2, s, c);
}

void gfx_set_color(Col c)
{
    SDL_SetRenderDrawColor(R, c.r, c.g, c.b, c.a);
}

void gfx_fill(SDL_Rect r, Col c)
{
    gfx_set_color(c);
    SDL_RenderFillRect(R, &r);
}

void gfx_frame(SDL_Rect r, Col c)
{
    gfx_set_color(c);
    SDL_RenderDrawRect(R, &r);
}

void gfx_line(int x0, int y0, int x1, int y1, Col c)
{
    gfx_set_color(c);
    SDL_RenderDrawLine(R, x0, y0, x1, y1);
}

void gfx_clip_push(SDL_Rect r)
{
    if (clip_n > 0) {
        SDL_Rect out;
        if (!SDL_IntersectRect(&clip_stack[clip_n - 1], &r, &out))
            out = (SDL_Rect){r.x, r.y, 0, 0};
        r = out;
    }
    if (clip_n < CLIP_DEPTH)
        clip_stack[clip_n++] = r;
    /* SDL treats an empty clip rect as "no clipping"; use a 1px offscreen
     * stand-in so a fully clipped region draws nothing */
    SDL_Rect use = r;
    if (use.w <= 0 || use.h <= 0)
        use = (SDL_Rect){-10, -10, 1, 1};
    SDL_RenderSetClipRect(R, &use);
}

void gfx_clip_pop(void)
{
    if (clip_n > 0)
        clip_n--;
    if (clip_n > 0) {
        SDL_Rect use = clip_stack[clip_n - 1];
        if (use.w <= 0 || use.h <= 0)
            use = (SDL_Rect){-10, -10, 1, 1};
        SDL_RenderSetClipRect(R, &use);
    } else {
        SDL_RenderSetClipRect(R, NULL);
    }
}

SDL_Texture *gfx_icon(const char *name)
{
    for (int i = 0; i < n_icons; i++)
        if (!strcmp(icons[i].name, name))
            return icons[i].tex;
    if (n_icons == MAX_ICONS)
        return NULL;
    char rel[96];
    snprintf(rel, sizeof rel, "icons/%s.png", name);
    const char *p = gfx_asset(rel);
    SDL_Texture *t = p ? IMG_LoadTexture(R, p) : NULL;
    if (t)
        SDL_SetTextureScaleMode(t, SDL_ScaleModeLinear);
    snprintf(icons[n_icons].name, sizeof icons[0].name, "%s", name);
    icons[n_icons].tex = t;
    n_icons++;
    return t;
}

bool rect_has(SDL_Rect r, int x, int y)
{
    return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}
