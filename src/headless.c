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

/* Headless runs: benchmarking, and state dumps that tools/javaref compares
 * against the original Java simulation frame by frame. */
#include "headless.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "sim/params.h"
#include "sim/world.h"

static uint64_t bits(double d)
{
    uint64_t u;
    memcpy(&u, &d, sizeof u);
    return u;
}

/* One line per world and per organism; the Java harness prints the same. */
static void dump_frame(FILE *f, const World *w, long frame)
{
    fprintf(f, "F %ld pop=%d o2=%016" PRIx64 " co2=%016" PRIx64 " next=%d n=%d\n", frame,
            w->population, bits(w->o2), bits(w->co2), w->next_id, w->norgs);
    for (int i = 0; i < w->norgs; i++) {
        const Organism *o = w->orgs[i];
        fprintf(f,
                "O %d %d e=%016" PRIx64 " c=%016" PRIx64 ",%016" PRIx64 " t=%016" PRIx64
                " v=%016" PRIx64 ",%016" PRIx64 ",%016" PRIx64 " g=%d r=%d,%d,%d,%d a=%d k=%d\n",
                o->id, o->alive ? 1 : 0, bits(o->energy), bits(o->dcenter_x), bits(o->dcenter_y),
                bits(o->theta), bits(o->dx), bits(o->dy), bits(o->dtheta), o->growth_ratio,
                o->r.x, o->r.y, o->r.width, o->r.height, o->age, o->total_kills);
    }
}

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int headless_run(const HeadlessOptions *opt)
{
    World w;
    world_init(&w);
    world_genesis(&w);
    FILE *dump = NULL;
    if (opt->dump_path) {
        dump = fopen(opt->dump_path, "w");
        if (!dump) {
            perror(opt->dump_path);
            return 1;
        }
        dump_frame(dump, &w, 0);
    }
    double t0 = now_s();
    for (long f = 1; f <= opt->frames; f++) {
        world_time(&w);
        if (dump && (opt->dump_every <= 1 || f % opt->dump_every == 0))
            dump_frame(dump, &w, f);
        if (!opt->quiet && opt->report_every > 0 && f % opt->report_every == 0)
            printf("frame %ld: population %d, organisms %d, O2 %.1f, CO2 %.1f\n", f,
                   w.population, w.norgs, w.o2, w.co2);
    }
    double dt = now_s() - t0;
    if (dump)
        fclose(dump);
    if (!opt->quiet)
        printf("%ld frames in %.3f s (%.0f frames/s); final population %d, O2 %.1f, CO2 %.1f, "
               "time %" PRId64 "\n",
               opt->frames, dt, dt > 0 ? opt->frames / dt : 0.0, w.population, w.o2, w.co2,
               w.stats.time);
    world_free(&w);
    return 0;
}
