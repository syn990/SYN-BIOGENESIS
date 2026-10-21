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

/* LIBRARY, the being library's screen (syn/library.h): its folders on the
 * left, with the arena's ladder as one more, read only; the beings of the
 * one chosen on the right, each drawn; and what can be done with the one
 * picked: put it on the arena's ladder, place it in the world, open it in
 * the laboratory, copy it, rename it, move it to another folder, delete
 * it. Beings come in from files (IMPORT), from the world and the
 * laboratory (Keep), and from the arena's ladder (KEEP IN LIBRARY). */
#ifndef SYN_LIBRARY_SCREEN_H
#define SYN_LIBRARY_SCREEN_H

#include <stdbool.h>

#include "sim/genetic_code.h"

struct App;

void syn_library_frame(struct App *a, int w, int h);
/* The screen BACK and Esc return to (a SynosScreen): the main menu unless
 * it was opened from somewhere else. */
void syn_library_open_from(int screen);
/* A code kept in a folder of the library under a name, saying where it
 * came from; the status line says where it went. */
bool syn_library_keep(struct App *a, const GeneticCode *gc, const char *folder, const char *name,
                      const char *origin);
/* What the file dialog's IMPORT chose. */
void syn_library_imported(struct App *a, const char *path);
void syn_library_shot_prepare(void);
void syn_library_screen_free(void);

#endif
