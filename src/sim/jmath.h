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

/* The transcendental functions the simulation uses, computed as the JVM
 * computes Biogenesis's Math calls, bit for bit.
 *
 * Math.atan2 is StrictMath.atan2, which is fdlibm 5.3 (src/fdlibm/). On
 * x86_64, HotSpot runs Math.sin, Math.cos and Math.pow as Intel LIBM routines
 * of its own (in the interpreter and in compiled code alike), which differ
 * from fdlibm and from the C library in the last bit for a few percent of
 * inputs. src/sim/jvm_libm_x86_64.S is those routines, translated
 * instruction for instruction from HotSpot's source (tools/jvmlibm/).
 * Elsewhere the C library's are used, which can differ in the last bit;
 * defining BIO_LIBC_MATH forces that (make libc, for tools/drift).
 */
#ifndef SIM_JMATH_H
#define SIM_JMATH_H

#include <math.h>

double fd_atan2(double y, double x);
#define bio_atan2 fd_atan2

#if defined(__x86_64__) && defined(__ELF__) && !defined(BIO_LIBC_MATH)
double jvm_dsin(double x);
double jvm_dcos(double x);
double jvm_dpow(double x, double y);
#define bio_sin jvm_dsin
#define bio_cos jvm_dcos
#define bio_pow jvm_dpow
#else
#define bio_sin sin
#define bio_cos cos
#define bio_pow pow
#endif

#endif
