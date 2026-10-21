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

/* TELESCOPE, the dice telescope's screen (syn/telescope.h), Biogenesis
 * 0.9's: the seed a saved world was made from, found from its genes and
 * proved by replaying it, as `syn-biogenesis --telescope world` finds it.
 *
 * The search runs in this program started again, never in the window's
 * own simulation: it makes and replays worlds with the one random
 * generator and parameters the world on screen runs on. The screen shows
 * what it says as it goes; once the seed is found, the saved world can be
 * opened with the generator where it was when the world was saved, so it
 * goes on as the world it was. */
#ifndef SYN_TELESCOPE_SCREEN_H
#define SYN_TELESCOPE_SCREEN_H

struct App;

void syn_telescope_frame(struct App *a, int w, int h);
/* What the file dialog's A SAVED WORLD chose. */
void syn_telescope_chosen(struct App *a, const char *path);
void syn_telescope_shot_prepare(struct App *a);
/* Ends a search still going. */
void syn_telescope_screen_free(void);
/* The being to look for (WHERE A BEING LIVES), the search begun at once;
 * the screen opens on it. */
struct GeneticCode;
void syn_telescope_find(const struct GeneticCode *gc, const char *name);

#endif
