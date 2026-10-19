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

/* The SYN-OS interface's statistics screen (World > Statistics), over the
 * running world, from the history in syn/history.h:
 *
 * - OVERVIEW: the world now, its colours, the champions alive and the
 *   records the original's statistics window shows.
 * - CHARTS: a dozen measures over the whole run, hovered together.
 * - EPOCHS: the run in epochs, each with its best species and champions.
 * - FAMILY: the family tree of its species (syn/family_view.h).
 * - TIMELINE: the lines' river over the run and the events, filtered.
 *
 * Organisms shown open in the laboratory on a click; a right click copies
 * their code. Esc or BACK returns to the world. */
#ifndef SYN_STATS_SCREEN_H
#define SYN_STATS_SCREEN_H

#include <stddef.h>

struct App;
struct Organism;

void syn_stats_frame(struct App *a, int w, int h);
/* The FAMILY tab, focused on this organism's species. */
void syn_stats_family(const struct Organism *o);
/* Its species' name in the family tree ("VELORAX-3", "#123"); "" if none. */
void syn_stats_species(const struct Organism *o, char *out, size_t n);
/* The length of the EPOCHS tab's epochs, in time units (its numbering is
 * the FAMILY tab's too). */
struct History;
int syn_stats_epoch_span(const struct History *hi);
void syn_stats_free(void);

#endif
