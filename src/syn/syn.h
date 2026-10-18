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

/* The SYN edition's world view (SYN-OS): the same simulation as the
 * faithful build, shown differently.
 *
 * - Navigation like a map: the wheel zooms at the cursor (eased), dragging
 *   with the left or middle button pans and can be flung, Home fits the
 *   world, a minimap shows where the view is.
 * - Looks: Classic (the faithful build's pixels), Biological, Galaxy and
 *   Digital (SYN-OS). The new looks draw organisms as antialiased strokes
 *   at screen resolution, from the genes' exact lengths and the organism's
 *   exact position and angle (the simulation rounds both to whole pixels),
 *   with motion and growth smoothed between simulation frames.
 *
 * Nothing here writes to the world except the flash countdown, which the
 * original's Organism.draw also counts down on each drawing. */
#ifndef SYN_SYN_H
#define SYN_SYN_H

#include <stdbool.h>

#include "sim/world.h"
#include "ui/worldview.h"

typedef enum { LOOK_CLASSIC, LOOK_BIOLOGICAL, LOOK_GALAXY, LOOK_DIGITAL, LOOK_COUNT } Look;

bool syn_init(SDL_Renderer *r);
void syn_shutdown(void);

Look syn_look(void);
void syn_set_look(Look l);
const char *syn_look_name(Look l);
bool syn_minimap(void);
void syn_set_minimap(bool on);

/* The world was replaced (new, opened): forget smoothing state. */
void syn_world_reset(void);
/* Before each simulation frame: remember where every organism was, so
 * drawing can move it smoothly to where it is now. */
void syn_before_step(const World *w);
/* How far the next simulation frame is due, 0..1 (1: draw as it is). */
void syn_set_alpha(double alpha);

/* Mouse and keys over the world panel. Returns true when the left button
 * was clicked (pressed and released without dragging) at screen (*cx, *cy).
 * *dragged is set when the user moved the view by hand (ends tracking). */
bool syn_view_input(WorldView *v, const World *w, bool keys_allowed, int *cx, int *cy,
                    bool *dragged);
/* Instead of syn_view_input while something covers the world (the SYN-OS
 * menus): the view goes on easing and the looks on animating. */
void syn_view_idle(WorldView *v, const World *w);
/* Keep the view on an organism, following its smoothed position. */
void syn_view_follow(WorldView *v, const World *w, const Organism *o);
void syn_view_zoom(WorldView *v, const World *w, double factor);
void syn_view_actual_size(WorldView *v, const World *w);
void syn_view_fit(WorldView *v, const World *w);
void syn_view_draw(WorldView *v, const World *w, const Organism *selected);
/* Something on screen moves even with the simulation paused. */
bool syn_animating(void);

#endif
