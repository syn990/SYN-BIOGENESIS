/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Part of a C port of Biogenesis (https://biogenesis.sourceforge.net/).
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

/* Saved worlds (.sbw).
 *
 * The Java version wrote worlds with Java object serialization (.bgw),
 * which only a JVM can read. This is a plain little-endian binary dump of
 * the same state: every organism field, the statistics, and the genetic
 * codes stored once each so shared codes stay shared after loading (white
 * segments and pasted clones depend on code identity).
 */
#ifndef IO_WORLDFILE_H
#define IO_WORLDFILE_H

#include <stdbool.h>
#include <stddef.h>

#include "sim/world.h"

#define WORLD_FILE_EXT "sbw"

bool worldfile_save(const World *w, const char *path, char *err, size_t errn);
/* Loads into *w (which must be initialised; its old contents are freed
 * only on success). */
bool worldfile_load(World *w, const char *path, char *err, size_t errn);

#endif
