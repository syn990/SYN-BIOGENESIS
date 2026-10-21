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

/* PARAMETERS, SYN-OS's screen for the parameters dialog: each simulation's
 * own fields (params_info in its ui/app.h), a card each with what it does,
 * its default and whether it waits for a new world, and the colours of the
 * genes as a table with each one's share. A value is taken as it is
 * entered (Enter, Tab or a click elsewhere), parsed and checked as the
 * dialog's OK does it, and saved; a refused one stays red until put right. */
#ifndef SYN_PARAMS_SCREEN_H
#define SYN_PARAMS_SCREEN_H

struct App;

/* the fields from the values in use, as the screen comes up */
void syn_params_open(struct App *a);
void syn_params_frame(struct App *a, int w, int h);

#endif
