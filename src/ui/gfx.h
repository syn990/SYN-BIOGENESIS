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

/* Drawing helpers over the SDL renderer: text (cached textures), boxes,
 * lines, icons, clipping, and finding the assets directory. */
#ifndef UI_GFX_H
#define UI_GFX_H

#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t r, g, b, a;
} Col;

#define COL(r, g, b) ((Col){(r), (g), (b), 255})

/* The palette (dark, so the black world blends in). */
extern const Col C_BG, C_PANEL, C_PANEL2, C_BORDER, C_TEXT, C_TEXT_DIM, C_ACCENT, C_HOVER,
    C_PRESS, C_FIELD, C_DISABLED, C_ORANGE;

bool gfx_init(SDL_Renderer *r);
void gfx_shutdown(void);
/* Once per frame: lets the text cache drop strings no longer drawn. */
void gfx_end_frame(void);
SDL_Renderer *gfx_renderer(void);

/* Path of a file under assets/, or NULL if it can't be found. */
const char *gfx_asset(const char *rel);

int gfx_font_h(void);
int gfx_text_w(const char *s);
void gfx_text(int x, int y, const char *s, Col c);
/* Text vertically centred in r, left aligned with padding, or centred. */
void gfx_text_in(SDL_Rect r, const char *s, Col c, bool centre);

void gfx_fill(SDL_Rect r, Col c);
void gfx_frame(SDL_Rect r, Col c);
void gfx_line(int x0, int y0, int x1, int y1, Col c);
void gfx_set_color(Col c);

/* Nested clip rectangles. */
void gfx_clip_push(SDL_Rect r);
void gfx_clip_pop(void);

/* Icons from assets/icons/<name>.png, loaded once. NULL if missing. */
SDL_Texture *gfx_icon(const char *name);

bool rect_has(SDL_Rect r, int x, int y);

#endif
