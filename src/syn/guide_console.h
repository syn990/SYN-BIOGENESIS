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

/* The console's words for the field guide (syn/guide.h, syn/kind.h):
 *   colour [NAME]   every colour in a line, or one in full
 *   kind            what the selected organism is (or every kind)
 *   guide           the library's field guide */
#ifndef SYN_GUIDE_CONSOLE_H
#define SYN_GUIDE_CONSOLE_H

#include <stdbool.h>

#include "sim/genetic_code.h"

#define SYN_GUIDE_WORDS "colour", "color", "kind", "guide"

/* v[0] is one of them: done (true), else false and nothing done */
bool syn_guide_console(int argc, char **v, const GeneticCode *selected);

#endif
