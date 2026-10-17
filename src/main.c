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

/* Command line:
 *
 *   syn-biogenesis [seed]                     the simulator window
 *   syn-biogenesis --headless [options]       no window: benchmark / dumps
 *       --frames N      frames to run (default 10000)
 *       --seed S        random seed (Java long)
 *       --dump FILE     write per-frame state (see tools/javaref)
 *       --every K       dump every K frames (default 1)
 *       --report K      print a status line every K frames
 *       --defaults      ignore saved parameters
 *       --quiet
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "headless.h"
#include "sim/params.h"
#ifndef BIO_NO_GUI
#include "ui/app.h"
#endif

static bool parse_long(const char *s, long long *out)
{
    char *end;
    errno = 0;
    long long v = strtoll(s, &end, 10);
    if (errno || end == s || *end)
        return false;
    *out = v;
    return true;
}

static int usage(void)
{
    fprintf(stderr, "usage: syn-biogenesis [seed]\n"
                    "       syn-biogenesis --headless [--frames N] [--seed S] [--dump FILE]\n"
                    "                      [--every K] [--report K] [--defaults] [--quiet]\n");
    return 2;
}

int main(int argc, char **argv)
{
    bool headless = false, defaults = false, seeded = false;
    long long seed = 0;
    HeadlessOptions h = {.frames = 10000, .dump_every = 1, .report_every = 0};

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        long long v;
        if (!strcmp(a, "--headless")) {
            headless = true;
        } else if (!strcmp(a, "--defaults")) {
            defaults = true;
        } else if (!strcmp(a, "--quiet")) {
            h.quiet = true;
        } else if (!strcmp(a, "--frames") && i + 1 < argc && parse_long(argv[i + 1], &v)) {
            h.frames = (long)v;
            i++;
        } else if (!strcmp(a, "--every") && i + 1 < argc && parse_long(argv[i + 1], &v)) {
            h.dump_every = (long)v;
            i++;
        } else if (!strcmp(a, "--report") && i + 1 < argc && parse_long(argv[i + 1], &v)) {
            h.report_every = (long)v;
            i++;
        } else if (!strcmp(a, "--seed") && i + 1 < argc && parse_long(argv[i + 1], &v)) {
            seed = v;
            seeded = true;
            i++;
        } else if (!strcmp(a, "--dump") && i + 1 < argc) {
            h.dump_path = argv[++i];
        } else if (a[0] != '-' && parse_long(a, &v)) {
            /* the original's only argument: java -jar biogenesis.jar [random seed] */
            seed = v;
            seeded = true;
        } else {
            return usage();
        }
    }

    params_set_defaults(&P);
    if (!defaults)
        params_load();
    if (seeded)
        jr_set_seed(&g_rng, (int64_t)seed);
    else
        jr_seed_from_time(&g_rng);

    if (headless)
        return headless_run(&h);
#ifdef BIO_NO_GUI
    fprintf(stderr, "built without the GUI; use --headless\n");
    return 2;
#else
    return app_run();
#endif
}
