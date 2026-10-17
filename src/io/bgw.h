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

/* Biogenesis 0.9's own world files (.bgw): the World object as
 * ObjectOutputStream.writeObject(world) writes it (MainWindow.saveObject).
 *
 * Loading reads any .bgw 0.9 can, from any Java version, to the last bit:
 * every organism (with the genetic codes and genes they share), the
 * statistics, and the collision tree as it was saved. Saving writes what
 * 0.9 writes on Java 27: the same bytes for a world that started here, or
 * that was loaded and saved again. A world loaded and then run on can differ
 * from 0.9's file in one way that doesn't change what loads: Java then
 * holds two Color objects of the same value (the ones it read and its
 * constants) and writes both, where this writes one. Neither 0.9 nor this
 * program keeps the random number generator in the file, so a loaded world
 * continues on the generator's current stream. */
#ifndef IO_BGW_H
#define IO_BGW_H

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
