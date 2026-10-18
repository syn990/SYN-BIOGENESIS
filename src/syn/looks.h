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

/* The vector looks (Biological, Galaxy, Digital). view.c culls and
 * smooths; these draw one frame: background, then each organism, then
 * the selection and overlays. */
#ifndef SYN_LOOKS_H
#define SYN_LOOKS_H

#include <SDL.h>
#include <stdint.h>

#include "sim/world.h"
#include "syn/syn.h"

typedef struct {
    SDL_Renderer *R;
    SDL_Rect in; /* the panel, in screen pixels */
    double cam_x, cam_y, zoom;
    double t; /* seconds, for animation */
    const World *w;
} LookCtx;

/* One organism ready to draw, in screen coordinates. */
typedef struct {
    const Organism *o;
    float cx, cy; /* centre */
    float rad;    /* radius of the body */
    int n;
    const float *x0, *y0, *x1, *y1;
    const uint8_t *col;
    uint8_t main_col; /* the commonest segment colour */
    float fresh;      /* 1 when just born, to 0 over a moment of display time */
} DOrg;

void look_begin(Look l, const LookCtx *c);
void look_org(Look l, const LookCtx *c, const DOrg *d);
/* sel may be NULL */
void look_end(Look l, const LookCtx *c, const DOrg *sel);
void look_free(void);

/* The look's colour for a BioColor, for the minimap. */
SDL_Color look_color(Look l, int bio_color);

#endif
