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

/* The world panel (VisibleWorld.java): draws organisms, handles scrolling,
 * zoom, selection and the right-click menus' positions.
 *
 * Organisms are rasterised in software into a pixel buffer at world
 * resolution (1-pixel aliased lines, like Java2D's drawLine) and the buffer
 * is scaled onto the screen, so zooming in shows crisp pixels. */
#ifndef UI_WORLDVIEW_H
#define UI_WORLDVIEW_H

#include <SDL.h>
#include <stdbool.h>

#include "sim/world.h"

typedef struct {
    double cam_x, cam_y; /* world coordinate at the view's top-left */
    double zoom;
    SDL_Rect view; /* screen rectangle of the panel */
    SDL_Texture *tex;
    int tex_w, tex_h;
    uint32_t *pix;
    size_t pix_cap;
    bool panning;
    int pan_mx, pan_my;
    double pan_cx, pan_cy;
    int drag_bar; /* 0 none, 1 horizontal, 2 vertical */
    int drag_off;
} WorldView;

void wv_init(WorldView *v);
void wv_free(WorldView *v);
void wv_clamp(WorldView *v, const World *w);
void wv_center_on(WorldView *v, const World *w, double wx, double wy);
void wv_screen_to_world(const WorldView *v, int sx, int sy, double *wx, double *wy);
void wv_zoom_at(WorldView *v, const World *w, double factor, int sx, int sy);
/* Handles scrolling input over the panel (wheel, middle drag, scrollbars,
 * arrow keys). */
void wv_input(WorldView *v, const World *w, bool keys_allowed);
void wv_draw(WorldView *v, const World *w, const Organism *selected);

/* Organism.getImage: the organism alone on black, for "Save image". */
SDL_Surface *wv_organism_image(const Organism *o);

#endif
