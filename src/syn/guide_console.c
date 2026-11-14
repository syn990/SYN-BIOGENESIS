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

/* The field guide's words in the console, the same in both windows. */
#include "syn/guide_console.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "syn/guide.h"
#include "syn/kind.h"
#include "syn/library_screen.h"
#include "syn/shell.h"
#include "synos/synos.h"

/* text in lines of at most 90 characters, broken between words, indented */
static void print_wrapped(const char *text)
{
    const char *p = text;
    while (*p) {
        size_t n = strlen(p);
        if (n > 90) {
            n = 90;
            while (n > 0 && p[n] != ' ')
                n--;
            if (n == 0)
                n = 90;
        }
        syn_shell_print(SHELL_OUT, "  %.*s", (int)n, p);
        p += n;
        while (*p == ' ')
            p++;
    }
}

static void print_colour(int s)
{
    syn_shell_print(SHELL_HEAD, "%s  (%s)", syn_col_name(s), syn_guide_role(syn_col_role(s)));
    print_wrapped(syn_guide_short(s));
    print_wrapped(syn_guide_long(s));
}

bool syn_guide_console(int argc, char **v, const GeneticCode *selected)
{
    const char *k = v[0];
    if (!strcasecmp(k, "colour") || !strcasecmp(k, "color")) {
        if (argc > 1) {
            /* "dark gray", "spike point": the rest of the line is the name */
            char name[64] = "";
            for (int i = 1; i < argc; i++)
                snprintf(name + strlen(name), sizeof name - strlen(name), "%s%s", i > 1 ? " " : "", v[i]);
            int s = syn_guide_find(name);
            if (s < 0 || !syn_guide_short(s)[0]) {
                syn_shell_print(SHELL_ERROR, "no colour %s (colour alone lists them)", name);
                return true;
            }
            print_colour(s);
            return true;
        }
        int last = -1;
        for (int s = 0; s < SYN_NCOL; s++) {
            if (!syn_guide_is_gene(s))
                continue;
            if ((int)syn_col_role(s) != last) {
                last = (int)syn_col_role(s);
                syn_shell_print(SHELL_HEAD, "%s", syn_guide_role(syn_col_role(s)));
            }
            syn_shell_print(SHELL_OUT, "  %-10s %s", syn_col_name(s), syn_guide_short(s));
        }
        syn_shell_print(SHELL_OUT, "colour NAME for the whole of it; guide for the field guide");
        return true;
    }
    if (!strcasecmp(k, "kind")) {
        if (selected) {
            char label[128], about[600];
            syn_kind_label(selected, label, sizeof label);
            syn_kind_describe(selected, about, sizeof about);
            syn_shell_print(SHELL_HEAD, "%s", label);
            print_wrapped(about);
            return true;
        }
        for (int i = 0; i < KIND_COUNT; i++) {
#ifdef BIO_COLOUR
            if (i == KIND_PREDATOR)
                continue;
#else
            if (i >= KIND_GRAZER && i <= KIND_PARASITE)
                continue;
#endif
            syn_shell_print(SHELL_OUT, "  %-10s %s", syn_kind_name((Kind)i), syn_kind_meaning((Kind)i));
        }
        syn_shell_print(SHELL_OUT, "select an organism, and kind says what it is");
        return true;
    }
    if (!strcasecmp(k, "guide")) {
        syn_library_show_guide();
        syn_library_open_from(synos_screen());
        synos_set_screen(SYNOS_LIBRARY);
        return true;
    }
    return false;
}
