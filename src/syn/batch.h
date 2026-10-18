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

/* Batched textured triangles for the SYN edition's looks: strokes with
 * round caps, sprites and rectangles, drawn with one SDL_RenderGeometry
 * call per batch. The textures are generated at start-up (discs, glows,
 * stars, rings and tileable noise), so no image files are needed. */
#ifndef SYN_BATCH_H
#define SYN_BATCH_H

#include <SDL.h>
#include <stdbool.h>

typedef enum {
    TX_WHITE,    /* solid: hard-edged quads, grid lines */
    TX_DISC,     /* antialiased disc: round-capped strokes */
    TX_GLOW,     /* soft falloff: glows, membranes, halos */
    TX_STAR,     /* bright core, halo and four spikes */
    TX_RING,     /* thin ring: selection markers */
    TX_NOISE,    /* tileable smooth noise in alpha */
    TX_GRIME,    /* tileable blotches and speckle in alpha */
    TX_NEBULA,   /* tileable coloured clouds */
    TX_VIGNETTE, /* black, transparent in the middle */
    TX_COUNT
} TexId;

typedef struct {
    SDL_Vertex *v;
    int *idx;
    int nv, ni, cap_v, cap_i;
    TexId tex;
    SDL_BlendMode blend;
} Batch;

bool batch_textures_init(SDL_Renderer *r);
void batch_textures_free(void);

void batch_begin(Batch *b, TexId tex, SDL_BlendMode blend);
/* Draws what was added and empties the batch (keeping its memory). */
void batch_flush(Batch *b, SDL_Renderer *r);
void batch_free(Batch *b);

/* The whole texture on a square of half-size r around (x, y), turned by
 * angle (radians). */
void batch_sprite(Batch *b, float x, float y, float r, float angle, SDL_Color c);
/* Part of the texture (u0,v0)-(u1,v1) on an axis-aligned rectangle. */
void batch_rect(Batch *b, float x, float y, float w, float h, float u0, float v0, float u1,
                float v1, SDL_Color c);
/* A stroke of half-width hw with round caps: the left half, middle column
 * and right half of a round texture (TX_DISC or TX_GLOW). */
void batch_capsule(Batch *b, float x0, float y0, float x1, float y1, float hw, SDL_Color c);
/* A stroke without caps: the middle column of the texture across it. */
void batch_stroke(Batch *b, float x0, float y0, float x1, float y1, float hw, SDL_Color c);

#endif
