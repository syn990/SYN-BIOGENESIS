/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
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

/* The one program (make, src/launcher.c) runs Biogenesis 0.9 or Color Mod
 * 2.01, one after the other, in one window. Each simulation's app_run takes
 * the window over from here (or makes it, the first time), and asks for the
 * other simulation by returning BIO_SWITCH_*; the launcher closes the window
 * at the end. */
#ifndef HOST_H
#define HOST_H

#include <SDL.h>

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
} BioHost;

extern BioHost bio_host;

#define BIO_SWITCH_BIOGENESIS 200
#define BIO_SWITCH_COLOUR 201

#endif
