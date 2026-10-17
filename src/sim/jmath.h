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

/* The transcendental functions the simulation uses.
 *
 * Normally the C library's. The strict verification build (make strict)
 * defines BIO_FDLIBM and links fdlibm 5.3 instead, the library Java's
 * StrictMath is ported from, so the C and Java simulations can be compared
 * for any number of frames without last-bit rounding differences.
 */
#ifndef SIM_JMATH_H
#define SIM_JMATH_H

#include <math.h>

#ifdef BIO_FDLIBM
double fd_sin(double x);
double fd_cos(double x);
double fd_atan2(double y, double x);
double fd_pow(double x, double y);
#define bio_sin fd_sin
#define bio_cos fd_cos
#define bio_atan2 fd_atan2
#define bio_pow fd_pow
#else
#define bio_sin sin
#define bio_cos cos
#define bio_atan2 atan2
#define bio_pow pow
#endif

#endif
