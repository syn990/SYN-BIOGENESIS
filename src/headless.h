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

#ifndef HEADLESS_H
#define HEADLESS_H

#include <stdbool.h>

typedef struct {
    long frames;
    const char *dump_path; /* NULL: no dump */
    long dump_every;
    const char *hash_path; /* NULL: no per-frame state hashes */
    const char *load_path; /* NULL: a new world (genesis) */
    const char *save_path; /* NULL: not saved at the end */
    long report_every;
    bool quiet;
} HeadlessOptions;

int headless_run(const HeadlessOptions *opt);

#endif
