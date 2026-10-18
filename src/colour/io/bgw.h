/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Part of the colour build, a C port of Biogenesis Color Mod 2.01
 * (https://sourceforge.net/projects/biogenesis/), Copyright (C) 2006-2010
 * Joan Queralt Molina, Color Mod Copyright (C) 2012-2022 MarcoDBAA.
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

/* Color Mod's world files (.bgw): its World object as Java serialization,
 * what ObjectOutputStream.writeObject(world) writes (MainWindow.saveObject). */
#ifndef COLOUR_IO_BGW_H
#define COLOUR_IO_BGW_H

#include <stdbool.h>
#include <stddef.h>

#include "sim/world.h"

#define BGW_FILE_EXT "bgw"

bool bgw_save(const World *w, const char *path, char *err, size_t errn);
/* Loads into *w (which must be initialised; its old contents are freed
 * only on success). */
bool bgw_load(World *w, const char *path, char *err, size_t errn);
/* true if the file starts like a Java serialization stream */
bool bgw_sniff(const char *path);

#endif
