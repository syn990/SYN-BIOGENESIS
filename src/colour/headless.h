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

/* Headless runs of the colour build (see src/colour/headless.c). */
#ifndef COLOUR_HEADLESS_H
#define COLOUR_HEADLESS_H

#include <stdbool.h>

typedef struct {
    long frames;
    const char *dump_path; /* NULL: no dump */
    long dump_every;
    const char *load_path; /* NULL: a new world (genesis) */
    const char *save_path; /* NULL: not saved at the end */
    long report_every;
    bool quiet;
} HeadlessOptions;

int headless_run(const HeadlessOptions *opt);

#endif
