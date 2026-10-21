/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Part of the SYN-OS interface SYN-BIOGENESIS shares with SYN-TREK and
 * SYN-DEBT.
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

/* The interface of Biogenesis 0.9 in the one program: Heritage edition
 * (Biogenesis' own window, as the faithful build has it, and what the
 * first run shows) or SYN-OS, the interface
 * SYN-TREK and SYN-DEBT share: a main menu over the living world, an
 * options screen, and every panel, bar, button, tab and field drawn in a
 * SYN-OS theme (synos/skin.h). Switched in OPTIONS (or the View menu in
 * Heritage), live, and remembered with the theme in
 * ~/.config/syn-biogenesis/synos.cfg. SYN_BIOGENESIS_INTERFACE=heritage|synos
 * and SYN_BIOGENESIS_THEME=<name> set them for one run. */
#ifndef SYNOS_SYNOS_H
#define SYNOS_SYNOS_H

#include <SDL.h>
#include <stdbool.h>

#include "synos/skin.h"
#include "synos/theme.h"

typedef enum {
    SYNOS_MENU,
    SYNOS_OPTIONS,
    SYNOS_WORLD,
    SYNOS_STATS,
    SYNOS_ARENA,
    SYNOS_LIBRARY,   /* the being library (syn/library_screen.h) */
    SYNOS_TELESCOPE, /* a saved world's seed (syn/telescope_screen.h), 0.9's */
} SynosScreen;

typedef enum {
    SYNOS_NONE,
    SYNOS_CONTINUE,   /* back to the world */
    SYNOS_NEW_WORLD,
    SYNOS_LOAD_WORLD,
    SYNOS_OPEN_OPTIONS,
    SYNOS_QUIT,
    SYNOS_SWITCH_SIM, /* to the other simulation (options) */
    SYNOS_OPEN_ARENA, /* the arena (syn/arena_screen.h) */
    SYNOS_OPEN_LIBRARY,
    SYNOS_OPEN_TELESCOPE,
} SynosAction;

/* title: "BIOGENESIS"; subtitle under it */
bool synos_init(SDL_Renderer *r, SDL_Window *w, const char *title, const char *subtitle);
void synos_shutdown(void);
/* once per frame, after drawing */
void synos_end_frame(void);

/* the SYN-OS interface is on (false: Heritage edition) */
bool synos_active(void);
void synos_set_active(bool on);

const Fonts *synos_fonts(void);
int synos_theme_count(void);
const Theme *synos_theme_at(int i);
int synos_theme_index(void);
void synos_set_theme(int i);
bool synos_fullscreen(void);
void synos_set_fullscreen(bool on);

SynosScreen synos_screen(void);
void synos_set_screen(SynosScreen s);

/* The main menu over whatever the caller drew behind it. can_continue: a
 * world is under way. */
SynosAction synos_menu_frame(int w, int h, bool can_continue);
/* The options screen: interface, simulation, look, minimap, fullscreen,
 * theme. Returns SYNOS_SWITCH_SIM when the other simulation is chosen. */
SynosAction synos_options_frame(int w, int h);

/* The world's speed, for the top bar: SPEED over the frames a second asked
 * for, and a bar from 1 a second to as fast as it goes, set by the wheel
 * over it or a click or drag on the bar; it is Parameters' Time per frame
 * (P.delay). Its right edge at right, its top at y; the width it takes,
 * only measured unless draw. */
float synos_speed(float right, float y, bool draw);

/* SDL_Rect to the skin's rectangles */
static inline UIRect synos_rect(SDL_Rect r)
{
    return ui_rect((float)r.x, (float)r.y, (float)r.w, (float)r.h);
}

#endif
