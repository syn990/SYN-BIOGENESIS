/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Part of the SYN-OS interface SYN-BIOGENESIS shares with SYN-TREK and
 * SYN-DEBT; this file follows SYN-DEBT's src/ui/skin.c, drawn with
 * SDL_Renderer instead of OpenGL (the shapes and colours are the same).
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
#include "synos/skin.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SDL_Renderer *R;
static float g_alpha = 1.0f;
static Theme g_theme;
static bool g_theme_set = false;
static Rgb g_tones[TONE_COUNT];

/* ---- colour ---- */

static double channel_lum(unsigned char c) {
    double s = c / 255.0;
    return s <= 0.03928 ? s / 12.92 : pow((s + 0.055) / 1.055, 2.4);
}

static double luminance(Rgb c) {
    return 0.2126 * channel_lum(c.r) + 0.7152 * channel_lum(c.g) + 0.0722 * channel_lum(c.b);
}

static double contrast(Rgb a, Rgb b) {
    double la = luminance(a), lb = luminance(b);
    return (fmax(la, lb) + 0.05) / (fmin(la, lb) + 0.05);
}

static Rgb mix(Rgb a, Rgb b, double t) {
    Rgb o = {
        (unsigned char)lround(a.r + (b.r - a.r) * t),
        (unsigned char)lround(a.g + (b.g - a.g) * t),
        (unsigned char)lround(a.b + (b.b - a.b) * t),
    };
    return o;
}

/* Pull a colour toward the text colour until it reads on the background. A
 * dark maroon accent (SYN-OS-RED) is fine as a bar but not as words on black. */
static Rgb readable(Rgb c, Rgb bg, Rgb text, double min_contrast) {
    for (int i = 0; i <= 10 && contrast(c, bg) < min_contrast; i++) c = mix(c, text, 0.12);
    return c;
}

void skin_set_theme(const Theme *t) {
    g_theme = *t;
    g_theme_set = true;
    const Theme *th = &g_theme;
    Rgb good = th->light ? (Rgb){0x15, 0x80, 0x3d} : (Rgb){0x4a, 0xde, 0x80};
    Rgb warn = th->light ? (Rgb){0xa1, 0x62, 0x07} : (Rgb){0xfb, 0xbf, 0x24};
    g_tones[TONE_TEXT] = th->text;
    g_tones[TONE_MUTED] = mix(th->text, th->bg, 0.42);
    g_tones[TONE_ACCENT] = readable(th->accent, th->bg, th->text, 3.2);
    g_tones[TONE_GOOD] = readable(good, th->bg, th->text, 3.0);
    g_tones[TONE_BAD] = readable(th->urgent, th->bg, th->text, 3.0);
    g_tones[TONE_WARN] = readable(warn, th->bg, th->text, 3.0);
    g_tones[TONE_ON_ACCENT] = contrast(th->text, th->accent) >= contrast(th->bg, th->accent) ? th->text : th->bg;
}

const Theme *skin_theme(void) {
    if (!g_theme_set) {
        Theme t;
        theme_fallback(&t);
        skin_set_theme(&t);
    }
    return &g_theme;
}

SDL_Color skin_color(Rgb c, float alpha) {
    float a = alpha * g_alpha;
    if (a < 0) a = 0;
    if (a > 1) a = 1;
    SDL_Color o = {c.r, c.g, c.b, (Uint8)lroundf(a * 255.0f)};
    return o;
}

SDL_Color skin_tone_color(Tone t) {
    skin_theme();
    return skin_color(g_tones[t], 1.0f);
}

void skin_set_alpha(float a) {
    g_alpha = a;
}

void skin_tone_rgb(Tone t, Uint8 *r, Uint8 *g, Uint8 *b) {
    skin_theme();
    *r = g_tones[t].r;
    *g = g_tones[t].g;
    *b = g_tones[t].b;
}

/* ---- text ----
 * A cache of white text textures, one per font and string, tinted per draw
 * (like src/ui/gfx.c's). */

typedef struct TextEntry {
    struct TextEntry *next;
    TTF_Font *font;
    uint32_t hash;
    SDL_Texture *tex;
    int w, h;
    unsigned last_used;
    char s[];
} TextEntry;

#define TEXT_BUCKETS 1024
static TextEntry *g_buckets[TEXT_BUCKETS];
static unsigned g_frame;

static uint32_t text_hash(TTF_Font *f, const char *s) {
    uint32_t h = 2166136261u ^ (uint32_t)(uintptr_t)f;
    for (; *s; s++) h = (h ^ (unsigned char)*s) * 16777619u;
    return h;
}

static TextEntry *text_get(TTF_Font *f, const char *s) {
    uint32_t h = text_hash(f, s);
    TextEntry **b = &g_buckets[h % TEXT_BUCKETS];
    for (TextEntry *e = *b; e; e = e->next) {
        if (e->hash == h && e->font == f && strcmp(e->s, s) == 0) {
            e->last_used = g_frame;
            return e;
        }
    }
    SDL_Surface *surf = TTF_RenderUTF8_Blended(f, s, (SDL_Color){255, 255, 255, 255});
    if (!surf) return NULL;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(R, surf);
    int w = surf->w, hh = surf->h;
    SDL_FreeSurface(surf);
    if (!tex) return NULL;
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    size_t n = strlen(s) + 1;
    TextEntry *e = malloc(sizeof *e + n);
    if (!e) {
        SDL_DestroyTexture(tex);
        return NULL;
    }
    e->font = f;
    e->hash = h;
    e->tex = tex;
    e->w = w;
    e->h = hh;
    e->last_used = g_frame;
    memcpy(e->s, s, n);
    e->next = *b;
    *b = e;
    return e;
}

void skin_end_frame(void) {
    g_frame++;
    if (g_frame % 120) return;
    /* drop what hasn't been drawn for a while */
    for (int i = 0; i < TEXT_BUCKETS; i++) {
        TextEntry **p = &g_buckets[i];
        while (*p) {
            TextEntry *e = *p;
            if (g_frame - e->last_used > 240) {
                *p = e->next;
                SDL_DestroyTexture(e->tex);
                free(e);
            } else {
                p = &e->next;
            }
        }
    }
}

static TTF_Font *open_font(const char *path, int size, bool bold) {
    TTF_Font *f = path ? TTF_OpenFont(path, size) : NULL;
    if (!f) return NULL;
    TTF_SetFontHinting(f, TTF_HINTING_LIGHT);
    if (bold) TTF_SetFontStyle(f, TTF_STYLE_BOLD);
    return f;
}

bool skin_init(SDL_Renderer *r, const char *font_path, Fonts *out) {
    R = r;
    out->title = open_font(font_path, 52, false);
    out->button = open_font(font_path, 24, false);
    out->heading = open_font(font_path, 20, true);
    out->big = open_font(font_path, 28, true);
    out->body = open_font(font_path, 16, false);
    out->bold = open_font(font_path, 16, true);
    out->small = open_font(font_path, 13, false);
    out->small_bold = open_font(font_path, 13, true);
    out->ui = open_font(font_path, 14, false);
    return out->title && out->button && out->heading && out->big && out->body && out->bold &&
           out->small && out->small_bold && out->ui;
}

void skin_shutdown(Fonts *f) {
    for (int i = 0; i < TEXT_BUCKETS; i++) {
        while (g_buckets[i]) {
            TextEntry *e = g_buckets[i];
            g_buckets[i] = e->next;
            SDL_DestroyTexture(e->tex);
            free(e);
        }
    }
    TTF_Font **all[] = {&f->title, &f->button, &f->heading, &f->big, &f->body, &f->bold,
                        &f->small, &f->small_bold, &f->ui};
    for (size_t i = 0; i < sizeof all / sizeof all[0]; i++) {
        if (*all[i]) TTF_CloseFont(*all[i]);
        *all[i] = NULL;
    }
}

float skin_text_width(TTF_Font *f, const char *s) {
    int w = 0, h = 0;
    if (!f || !s || !s[0] || TTF_SizeUTF8(f, s, &w, &h) != 0) return 0;
    return (float)w;
}

float skin_text_height(TTF_Font *f) {
    return f ? (float)TTF_FontHeight(f) : 0;
}

float skin_text_rgb(TTF_Font *f, const char *s, float x, float y, Uint8 r, Uint8 g, Uint8 b, Align align) {
    if (!f || !s || !s[0]) return 0;
    TextEntry *e = text_get(f, s);
    if (!e) return 0;
    float w = (float)e->w;
    if (align == ALIGN_CENTER) x -= w * 0.5f;
    else if (align == ALIGN_RIGHT) x -= w;
    SDL_SetTextureColorMod(e->tex, r, g, b);
    SDL_SetTextureAlphaMod(e->tex, (Uint8)lroundf(g_alpha * 255.0f));
    SDL_FRect d = {floorf(x), floorf(y), (float)e->w, (float)e->h};
    SDL_RenderCopyF(R, e->tex, NULL, &d);
    return w;
}

float skin_text(TTF_Font *f, const char *s, float x, float y, Tone tone, Align align) {
    skin_theme();
    Rgb c = g_tones[tone];
    return skin_text_rgb(f, s, x, y, c.r, c.g, c.b, align);
}

const char *skin_fit(TTF_Font *f, const char *s, float max_w, char *buf, size_t len) {
    snprintf(buf, len, "%s", s ? s : "");
    if (skin_text_width(f, buf) <= max_w) return buf;
    size_t n = strlen(buf);
    while (n > 0) {
        /* Step back over a whole UTF-8 character. */
        do { n--; } while (n > 0 && ((unsigned char)buf[n] & 0xC0) == 0x80);
        if (n + 4 > len) continue;
        memcpy(buf + n, "...", 4);
        if (skin_text_width(f, buf) <= max_w) return buf;
    }
    buf[0] = '\0';
    return buf;
}

/* ---- primitives ---- */

static void rect(float x, float y, float w, float h, Rgb c, float a) {
    if (w <= 0 || h <= 0) return;
    SDL_Color col = skin_color(c, a);
    if (!col.a) return;
    SDL_SetRenderDrawBlendMode(R, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(R, col.r, col.g, col.b, col.a);
    SDL_FRect fr = {x, y, w, h};
    SDL_RenderFillRectF(R, &fr);
}

void skin_fill(UIRect r, Rgb c, float alpha) {
    rect(r.x, r.y, r.w, r.h, c, alpha);
}

static void outline(UIRect r, float t, Rgb c, float a) {
    rect(r.x, r.y, r.w, t, c, a);
    rect(r.x, r.y + r.h - t, r.w, t, c, a);
    rect(r.x, r.y + t, t, r.h - 2 * t, c, a);
    rect(r.x + r.w - t, r.y + t, t, r.h - 2 * t, c, a);
}

static void gradient(UIRect r, Rgb top, Rgb bottom, float a) {
    int strips = (int)(r.h / 3.0f);
    if (strips < 1) strips = 1;
    if (strips > 24) strips = 24;
    float sh = r.h / (float)strips;
    for (int i = 0; i < strips; i++) {
        double t = strips == 1 ? 0.5 : (double)i / (double)(strips - 1);
        rect(r.x, r.y + sh * (float)i, r.w, sh + 0.5f, mix(top, bottom, t), a);
    }
}

static void glow(UIRect r, Rgb c, float strength) {
    for (int i = 1; i <= 3; i++) {
        float g = (float)i * 1.5f;
        UIRect o = { r.x - g, r.y - g, r.w + 2 * g, r.h + 2 * g };
        outline(o, 1.5f, c, strength * (0.34f - 0.1f * (float)i));
    }
}

static void win95_bevel(UIRect r, bool sunken) {
    Rgb white = {0xff, 0xff, 0xff}, light = {0xdf, 0xdf, 0xdf}, dark = {0x40, 0x40, 0x40};
    Rgb shade = g_theme.border;
    Rgb tl_out = sunken ? shade : white, tl_in = sunken ? dark : light;
    Rgb br_out = sunken ? white : dark, br_in = sunken ? light : shade;
    rect(r.x, r.y, r.w, 1, tl_out, 1);
    rect(r.x, r.y, 1, r.h, tl_out, 1);
    rect(r.x + 1, r.y + 1, r.w - 2, 1, tl_in, 1);
    rect(r.x + 1, r.y + 1, 1, r.h - 2, tl_in, 1);
    rect(r.x, r.y + r.h - 1, r.w, 1, br_out, 1);
    rect(r.x + r.w - 1, r.y, 1, r.h, br_out, 1);
    rect(r.x + 1, r.y + r.h - 2, r.w - 2, 1, br_in, 1);
    rect(r.x + r.w - 2, r.y + 1, 1, r.h - 2, br_in, 1);
}

/* ---- chrome ---- */

void skin_background(float w, float h) {
    const Theme *t = skin_theme();
    rect(0, 0, w, h, t->bg, 1);
}

void skin_bar(UIRect r) {
    const Theme *t = skin_theme();
    switch (t->style) {
    case STYLE_BEVEL:
        gradient(r, t->bg, t->bg_alt, 1);
        rect(r.x, r.y + r.h - 2, r.w, 2, t->panel_hover, 1);
        rect(r.x, r.y + r.h, r.w, 3, (Rgb){0, 0, 0}, 0.35f);
        break;
    case STYLE_FLATLINE:
        rect(r.x, r.y, r.w, r.h, t->bg, 1);
        break;
    case STYLE_HALO:
        rect(r.x, r.y, r.w, r.h, t->bg, 1);
        rect(r.x, r.y + r.h - 2, r.w, 2, t->accent_dim, 1);
        break;
    case STYLE_SLAB:
        rect(r.x, r.y, r.w, r.h, t->bg, 1);
        rect(r.x, r.y + r.h - 6, r.w, 6, t->bg_alt, 1);
        break;
    case STYLE_WIN95:
        rect(r.x, r.y, r.w, r.h, t->panel, 1);
        win95_bevel(r, false);
        break;
    case STYLE_MATRIX:
    case STYLE_VANILLA:
    default:
        rect(r.x, r.y, r.w, r.h, t->bg, 1);
        rect(r.x, r.y + r.h - 3, r.w, 3, t->bg_alt, 1);
        break;
    }
}

void skin_panel(UIRect r, bool over_scene) {
    const Theme *t = skin_theme();
    float a = over_scene ? 0.94f : 1.0f;
    switch (t->style) {
    case STYLE_BEVEL:
        rect(r.x + 2, r.y + r.h, r.w, 3, (Rgb){0, 0, 0}, 0.35f);
        gradient(r, t->bg_alt, t->bg, a);
        outline(r, 1, t->panel_hover, 1);
        rect(r.x + 1, r.y + 1, r.w - 2, 1, (Rgb){0xff, 0xff, 0xff}, 0.08f);
        break;
    case STYLE_FLATLINE:
        if (over_scene) rect(r.x, r.y, r.w, r.h, t->bg, 0.9f);
        break;
    case STYLE_HALO:
        rect(r.x, r.y, r.w, r.h, t->bg, over_scene ? 0.9f : 1.0f);
        outline(r, 1, t->accent_dim, 1);
        break;
    case STYLE_SLAB:
        rect(r.x, r.y, r.w, r.h, t->panel, a);
        outline(r, 2, t->bg_alt, 1);
        break;
    case STYLE_WIN95:
        rect(r.x, r.y, r.w, r.h, t->panel, 1);
        win95_bevel(r, false);
        break;
    case STYLE_MATRIX:
        rect(r.x, r.y, r.w, r.h, t->bg_alt, a);
        outline(r, 1, t->border, 1);
        break;
    case STYLE_VANILLA:
    default:
        rect(r.x, r.y, r.w, r.h, t->panel, a);
        outline(r, 1, t->bg_alt, 1);
        break;
    }
}

static void label_in(UIRect r, TTF_Font *f, const char *label, Tone tone, float alpha_dim) {
    if (!label || !label[0]) return;
    Uint8 cr, cg, cb;
    skin_tone_rgb(tone, &cr, &cg, &cb);
    if (alpha_dim < 1.0f) {
        Rgb c = mix((Rgb){cr, cg, cb}, g_theme.bg, 1.0 - alpha_dim);
        cr = c.r; cg = c.g; cb = c.b;
    }
    float h = skin_text_height(f);
    skin_text_rgb(f, label, r.x + r.w * 0.5f, r.y + (r.h - h) * 0.5f, cr, cg, cb, ALIGN_CENTER);
}

void skin_button(UIRect r, TTF_Font *f, const char *label, WidgetState st, bool over_scene) {
    skin_button_tone(r, f, label, st, over_scene, TONE_COUNT);
}

void skin_input(UIRect r, bool focused, bool hover, bool invalid) {
    const Theme *t = skin_theme();
    Rgb edge = invalid ? g_tones[TONE_BAD] : focused ? g_tones[TONE_ACCENT] : hover ? t->accent_dim : t->border;
    float edge_a = focused || invalid ? 1.0f : hover ? 0.9f : 0.7f;
    switch (t->style) {
    case STYLE_FLATLINE:
        if (focused) rect(r.x, r.y, r.w, r.h, t->bg_alt, 0.6f);
        rect(r.x, r.y + r.h - (focused ? 2.0f : 1.0f), r.w, focused ? 2.0f : 1.0f, edge, edge_a);
        break;
    case STYLE_HALO:
        rect(r.x, r.y, r.w, r.h, t->bg, 1);
        outline(r, 1, focused || invalid ? edge : (hover ? t->accent : t->accent_dim), 1);
        if (focused) glow(r, invalid ? g_tones[TONE_BAD] : t->accent, 0.6f);
        break;
    case STYLE_SLAB:
        rect(r.x, r.y, r.w, r.h, t->bg, 1);
        outline(r, 2, focused || invalid ? edge : hover ? t->accent_dim : t->bg_alt, 1);
        break;
    case STYLE_WIN95:
        rect(r.x, r.y, r.w, r.h, (Rgb){0xff, 0xff, 0xff}, 1);
        win95_bevel(r, true);
        if (focused || invalid) outline((UIRect){ r.x + 2, r.y + 2, r.w - 4, r.h - 4 }, 1, edge, 1);
        break;
    case STYLE_BEVEL:
        rect(r.x, r.y, r.w, r.h, t->bg, 1);
        rect(r.x + 1, r.y + 1, r.w - 2, 2, (Rgb){0, 0, 0}, 0.35f);
        outline(r, 1, edge, edge_a);
        break;
    case STYLE_MATRIX:
    case STYLE_VANILLA:
    default:
        rect(r.x, r.y, r.w, r.h, t->light ? (Rgb){0xff, 0xff, 0xff} : t->bg, 1);
        outline(r, 1, edge, edge_a);
        if (focused) rect(r.x, r.y + r.h - 2, r.w, 2, edge, 1);
        break;
    }
}

void skin_button_tone(UIRect r, TTF_Font *f, const char *label, WidgetState st, bool over_scene, Tone label_tone) {
    const Theme *t = skin_theme();
    float a = over_scene ? 0.88f : 1.0f;
    Tone tone = TONE_TEXT;
    bool lit = st.hover || st.active;

    switch (t->style) {
    case STYLE_BEVEL:
        rect(r.x + 1, r.y + r.h, r.w, 2, (Rgb){0, 0, 0}, 0.4f);
        if (st.active || st.pressed) {
            gradient(r, t->accent, t->accent_dim, a);
            outline(r, 1, t->accent, 1);
            tone = TONE_ON_ACCENT;
        } else {
            gradient(r, lit ? t->panel_hover : t->panel, lit ? t->panel : t->bg_alt, a);
            outline(r, 1, lit ? t->accent_dim : t->bg_alt, 1);
        }
        rect(r.x + 1, r.y + 1, r.w - 2, 1, (Rgb){0xff, 0xff, 0xff}, 0.15f);
        break;
    case STYLE_FLATLINE:
        if (over_scene) rect(r.x, r.y, r.w, r.h, t->bg, 0.55f);
        tone = lit ? TONE_ACCENT : TONE_TEXT;
        break;
    case STYLE_HALO:
        rect(r.x, r.y, r.w, r.h, t->bg, over_scene ? 0.6f : 1.0f);
        outline(r, 1, lit ? t->accent : t->accent_dim, 1);
        if (st.active || st.hover) glow(r, t->accent, st.active ? 1.0f : 0.7f);
        tone = lit ? TONE_ACCENT : TONE_TEXT;
        break;
    case STYLE_SLAB:
        rect(r.x, r.y, r.w, r.h, lit ? t->panel_hover : t->panel, a);
        outline(r, 2, st.active ? t->accent : st.hover ? t->accent_dim : t->bg_alt, 1);
        break;
    case STYLE_WIN95:
        rect(r.x, r.y, r.w, r.h, t->panel, 1);
        win95_bevel(r, st.pressed || st.active);
        if (st.hover) outline((UIRect){ r.x + 4, r.y + 4, r.w - 8, r.h - 8 }, 1, t->border, 0.8f);
        break;
    case STYLE_MATRIX:
        rect(r.x, r.y, r.w, r.h, lit ? t->panel_hover : t->panel, a);
        rect(r.x, r.y, r.w, 3, st.active ? t->accent : st.hover ? t->accent_dim : t->bg_alt, 1);
        break;
    case STYLE_VANILLA:
    default:
        rect(r.x, r.y, r.w, r.h, lit ? t->panel_hover : t->panel, a);
        outline(r, 1, t->bg_alt, 1);
        rect(r.x, r.y + r.h - 3, r.w, 3, st.active ? t->accent : st.hover ? t->accent_dim : t->bg_alt, 1);
        break;
    }
    if (label_tone != TONE_COUNT && tone != TONE_ON_ACCENT) tone = label_tone;
    label_in(r, f, label, tone, st.disabled ? 0.45f : 1.0f);
}

void skin_tab(UIRect r, TTF_Font *f, const char *label, WidgetState st) {
    const Theme *t = skin_theme();
    Tone tone = st.active ? TONE_TEXT : TONE_MUTED;
    switch (t->style) {
    case STYLE_BEVEL:
        if (st.active) {
            gradient(r, t->accent, t->accent_dim, 1);
            outline(r, 1, t->accent, 1);
            rect(r.x + 1, r.y + 1, r.w - 2, 1, (Rgb){0xff, 0xff, 0xff}, 0.15f);
            tone = TONE_ON_ACCENT;
        } else {
            if (st.hover) gradient(r, t->panel_hover, t->panel, 1);
            outline(r, 1, st.hover ? t->accent_dim : t->bg_alt, 1);
            if (st.hover) tone = TONE_TEXT;
        }
        break;
    case STYLE_FLATLINE:
        tone = st.active || st.hover ? TONE_ACCENT : TONE_MUTED;
        break;
    case STYLE_HALO:
        outline(r, 1, st.active ? t->accent : st.hover ? t->accent_dim : t->bg_alt, 1);
        if (st.active) glow(r, t->accent, 0.8f);
        tone = st.active ? TONE_ACCENT : st.hover ? TONE_TEXT : TONE_MUTED;
        break;
    case STYLE_SLAB:
        rect(r.x, r.y, r.w, r.h, st.hover && !st.active ? t->panel_hover : t->panel, 1);
        outline(r, 2, st.active ? t->accent : st.hover ? t->accent_dim : t->bg_alt, 1);
        if (st.hover) tone = TONE_TEXT;
        break;
    case STYLE_WIN95:
        rect(r.x, r.y, r.w, r.h, st.active ? t->bg_alt : t->panel, 1);
        win95_bevel(r, st.active);
        tone = TONE_TEXT;
        break;
    case STYLE_MATRIX:
        if (st.active) rect(r.x, r.y, r.w, r.h, t->bg_alt, 1);
        if (st.active || st.hover) rect(r.x, r.y, r.w, 3, st.active ? t->accent : t->accent_dim, 1);
        if (st.hover) tone = TONE_TEXT;
        break;
    case STYLE_VANILLA:
    default:
        if (st.active) rect(r.x, r.y, r.w, r.h, t->bg_alt, 1);
        if (st.active || st.hover) rect(r.x, r.y + r.h - 3, r.w, 3, st.active ? t->accent : t->accent_dim, 1);
        if (st.hover) tone = TONE_TEXT;
        break;
    }
    label_in(r, f, label, tone, 1.0f);
}

void skin_row_hover(UIRect r) {
    const Theme *t = skin_theme();
    if (t->style == STYLE_FLATLINE) {
        rect(r.x, r.y, 3, r.h, t->accent, 1);
        return;
    }
    rect(r.x, r.y, r.w, r.h, t->panel_hover, t->style == STYLE_WIN95 ? 1.0f : 0.55f);
}

void skin_separator(float x, float y, float w) {
    const Theme *t = skin_theme();
    Rgb c = t->style == STYLE_HALO ? t->accent_dim : t->style == STYLE_SLAB ? t->bg_alt : t->border;
    rect(x, y, w, 1, c, t->style == STYLE_FLATLINE ? 0.35f : 0.6f);
}

void skin_meter(UIRect r, double fraction, Tone tone) {
    const Theme *t = skin_theme();
    if (fraction < 0) fraction = 0;
    if (fraction > 1) fraction = 1;
    rect(r.x, r.y, r.w, r.h, t->light ? t->bg_alt : t->bg, 1);
    if (t->style == STYLE_HALO || t->style == STYLE_FLATLINE) outline(r, 1, t->border, 0.6f);
    skin_theme();
    rect(r.x, r.y, r.w * (float)fraction, r.h, g_tones[tone], 1);
}

void skin_swatches(UIRect r, const Theme *th) {
    Rgb cols[5] = { th->bg, th->panel, th->accent_dim, th->accent, th->text };
    float w = r.w / 5.0f;
    for (int i = 0; i < 5; i++) rect(r.x + w * (float)i, r.y, w + 0.5f, r.h, cols[i], 1);
    outline(r, 1, th->border, 1);
}
