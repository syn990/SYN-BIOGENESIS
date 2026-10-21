/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
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

/* The one program (make): Biogenesis 0.9, in the SYN edition's window
 * (Heritage or SYN-OS), and Color Mod 2.01. Each is its own program inside
 * this one: 0.9's main renamed bio_main, Color Mod's colour_main with every
 * other name of it hidden (see the Makefile), so the two simulations'
 * organisms, worlds and windows don't meet.
 *
 *   syn-biogenesis                the simulation used last; the first time,
 *                                 Biogenesis 0.9 as the original looks
 *   syn-biogenesis [0.9's arguments]      Biogenesis 0.9 (a seed, --headless...)
 *   syn-biogenesis --colour [arguments]   Color Mod 2.01, as its own program
 *   syn-biogenesis [--colour] --arena ... the arena without a window
 *
 * The choice is kept in ~/.config/syn-biogenesis/variant.cfg;
 * SYN_BIOGENESIS_VARIANT=biogenesis|colour picks one for a single run. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include "compat.h"
#include "host.h"
#include "sim/params.h"

int bio_main(int argc, char **argv);
int colour_main(int argc, char **argv);
/* the arena (src/syn/arena.h), of each simulation */
int syn_arena_main(int argc, char **argv);
int syn_arena_worker(int argc, char **argv);
int colour_arena_main(int argc, char **argv);
int colour_arena_worker(int argc, char **argv);
/* the dice telescope (src/syn/telescope.h), Biogenesis 0.9's */
int syn_telescope_main(int argc, char **argv);

BioHost bio_host;

typedef enum { V_BIOGENESIS, V_COLOUR } Variant;

static void variant_path(char *out, size_t n)
{
    snprintf(out, n, "%s/variant.cfg", params_config_dir());
}

static Variant variant_load(void)
{
    const char *env = getenv("SYN_BIOGENESIS_VARIANT");
    if (env && *env)
        return strcmp(env, "colour") == 0 ? V_COLOUR : V_BIOGENESIS;
    char path[1100];
    variant_path(path, sizeof path);
    FILE *f = fopen(path, "r");
    Variant v = V_BIOGENESIS;
    if (f) {
        char line[64];
        while (fgets(line, sizeof line, f))
            if (!strncmp(line, "variant=", 8))
                v = strncmp(line + 8, "colour", 6) == 0 ? V_COLOUR : V_BIOGENESIS;
        fclose(f);
    }
    return v;
}

static void variant_save(Variant v)
{
    if (getenv("SYN_BIOGENESIS_VARIANT"))
        return;
    const char *dir = params_config_dir();
    char parent[1024];
    snprintf(parent, sizeof parent, "%s", dir);
    char *slash = strrchr(parent, '/');
    if (slash) {
        *slash = '\0';
        mkdir(parent, 0755);
    }
    mkdir(dir, 0755);
    char path[1100];
    variant_path(path, sizeof path);
    FILE *f = fopen(path, "w");
    if (!f)
        return;
    fprintf(f, "# SYN-BIOGENESIS: the simulation it opens with\nvariant=%s\n",
            v == V_COLOUR ? "colour" : "biogenesis");
    fclose(f);
}

#ifdef _WIN32
/* On Windows this is a window program, which starts without a console: run
 * from one (--help, --headless) with its output not redirected, it writes
 * to that console. */
static void attach_console(void)
{
    bool out = GetFileType(GetStdHandle(STD_OUTPUT_HANDLE)) == FILE_TYPE_UNKNOWN;
    bool err = GetFileType(GetStdHandle(STD_ERROR_HANDLE)) == FILE_TYPE_UNKNOWN;
    if ((out || err) && AttachConsole(ATTACH_PARENT_PROCESS)) {
        if (out)
            freopen("CONOUT$", "w", stdout);
        if (err)
            freopen("CONOUT$", "w", stderr);
    }
}
#endif

int main(int argc, char **argv)
{
    Variant v;
#ifdef _WIN32
    attach_console();
#endif
    if (argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))) {
        printf("usage: syn-biogenesis                    the simulation used last (Biogenesis 0.9 at first)\n"
               "       syn-biogenesis [seed]             Biogenesis 0.9\n"
               "       syn-biogenesis --headless ...     Biogenesis 0.9 without a window\n"
               "       syn-biogenesis --colour [seed]    Color Mod 2.01\n"
               "       syn-biogenesis --colour --headless ...   Color Mod 2.01 without a window\n"
               "       syn-biogenesis [--colour] --arena ...    the arena without a window: rounds\n"
               "                                                of its ladder on every core\n"
               "       syn-biogenesis --telescope ...    Biogenesis 0.9's random generator as algebra:\n"
               "                                         jumps, gene lengths, a saved world's seed\n"
               "syn-biogenesis --headless --help and --colour --headless --help list their options,\n"
               "--arena --help the arena's and --telescope --help the telescope's.\n");
        return 0;
    }
    /* the arena's workers (started by the arena itself), and the arena
     * without a window */
    if (argc > 2 && !strcmp(argv[1], "--arena-worker"))
        return !strcmp(argv[2], "colour") ? colour_arena_worker(argc - 2, argv + 2)
                                          : syn_arena_worker(argc - 2, argv + 2);
    if (argc > 1 && !strcmp(argv[1], "--arena"))
        return syn_arena_main(argc, argv);
    if (argc > 1 && !strcmp(argv[1], "--telescope"))
        return syn_telescope_main(argc - 1, argv + 1);
    if (argc > 2 && !strcmp(argv[1], "--colour") && !strcmp(argv[2], "--arena"))
        return colour_arena_main(argc - 1, argv + 1);
    if (argc > 1 && !strcmp(argv[1], "--colour")) {
        v = V_COLOUR;
        /* swapped, not overwritten: on Windows SDL's main frees each of
         * the strings it passed when this returns */
        char *colour = argv[1];
        argv[1] = argv[0];
        argv[0] = colour;
        argc--;
        argv++;
    } else if (argc > 1) {
        v = V_BIOGENESIS;
    } else {
        v = variant_load();
    }
    int r;
    for (;;) {
        r = v == V_COLOUR ? colour_main(argc, argv) : bio_main(argc, argv);
        if (r != BIO_SWITCH_BIOGENESIS && r != BIO_SWITCH_COLOUR)
            break;
        v = r == BIO_SWITCH_COLOUR ? V_COLOUR : V_BIOGENESIS;
        variant_save(v);
        argc = 1; /* the next one starts without the arguments */
        /* test hook: the scripted input (SYN_BIOGENESIS_SCRIPT, src/ui/app.c)
         * for the simulation switched to, SYN_BIOGENESIS_SCRIPT_NEXT; then
         * SYN_BIOGENESIS_SCRIPT_NEXT2 for the one after */
        const char *next = getenv("SYN_BIOGENESIS_SCRIPT");
        if (next) {
            const char *n1 = getenv("SYN_BIOGENESIS_SCRIPT_NEXT");
            const char *n2 = getenv("SYN_BIOGENESIS_SCRIPT_NEXT2");
            static char buf1[8192], buf2[8192];
            snprintf(buf1, sizeof buf1, "%s", n1 ? n1 : "");
            snprintf(buf2, sizeof buf2, "%s", n2 ? n2 : "");
            if (n1)
                setenv("SYN_BIOGENESIS_SCRIPT", buf1, 1);
            else
                unsetenv("SYN_BIOGENESIS_SCRIPT");
            if (n2)
                setenv("SYN_BIOGENESIS_SCRIPT_NEXT", buf2, 1);
            else
                unsetenv("SYN_BIOGENESIS_SCRIPT_NEXT");
            unsetenv("SYN_BIOGENESIS_SCRIPT_NEXT2");
        }
    }
    /* closes the window (and its renderer) the simulations shared */
    if (bio_host.window)
        SDL_Quit();
    return r;
}
