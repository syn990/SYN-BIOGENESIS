/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Translated to C in 2026 from BioXMLParser.java
 * of Biogenesis 0.9 (https://biogenesis.sourceforge.net/),
 * Copyright (C) 2006-2010 Joan Queralt Molina.
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

/* Genetic code files (.bgg): the XML format of BioXMLParser.java, so codes
 * exported here open in the Java Biogenesis and vice versa. */
#ifndef IO_BGG_H
#define IO_BGG_H

#include <stdbool.h>
#include <stddef.h>

#include "sim/genetic_code.h"

bool bgg_write(const char *path, const GeneticCode *gc);
/* NULL on failure, with a reason in err. */
GeneticCode *bgg_read(const char *path, char *err, size_t errn);
/* Parse from memory (used by tests). */
GeneticCode *bgg_parse(const char *xml, size_t len, char *err, size_t errn);

#endif
