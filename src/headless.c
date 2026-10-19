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
 * against the original Java simulation frame by frame, or per-frame hashes
 * of the same state for long comparisons (tools/drift). */
#include "headless.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "io/bgw.h"
#include "io/worldfile.h"
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

/* FNV-1a over the dumped values as little-endian 64-bit words, in dump
 * order; tools/drift/HashRun.java computes the same. */
typedef struct {
    uint64_t h;
} StateHash;

static void sh_word(StateHash *s, uint64_t v)
{
    for (int i = 0; i < 8; i++) {
        s->h ^= (v >> (8 * i)) & 0xff;
        s->h *= 0x100000001b3ULL;
    }
}

static void sh_int(StateHash *s, int v)
{
    sh_word(s, (uint64_t)(int64_t)v);
}

/* One line per frame: frame, population, organisms, state hash. */
static void hash_frame(FILE *f, const World *w, long frame)
{
    StateHash s = {0xcbf29ce484222325ULL};
    sh_int(&s, w->population);
    sh_word(&s, bits(w->o2));
    sh_word(&s, bits(w->co2));
    sh_int(&s, w->next_id);
    sh_int(&s, w->norgs);
    for (int i = 0; i < w->norgs; i++) {
        const Organism *o = w->orgs[i];
        sh_int(&s, o->id);
        sh_int(&s, o->alive ? 1 : 0);
        sh_word(&s, bits(o->energy));
        sh_word(&s, bits(o->dcenter_x));
        sh_word(&s, bits(o->dcenter_y));
        sh_word(&s, bits(o->theta));
        sh_word(&s, bits(o->dx));
        sh_word(&s, bits(o->dy));
        sh_word(&s, bits(o->dtheta));
        sh_int(&s, o->growth_ratio);
        sh_int(&s, o->r.x);
        sh_int(&s, o->r.y);
        sh_int(&s, o->r.width);
        sh_int(&s, o->r.height);
        sh_int(&s, o->age);
        sh_int(&s, o->total_kills);
    }
    fprintf(f, "%ld %d %d %016" PRIx64 "\n", frame, w->population, w->norgs, s.h);
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
    char err[256];
    if (opt->load_path) {
        bool ok = bgw_sniff(opt->load_path) ? bgw_load(&w, opt->load_path, err, sizeof err)
                                             : worldfile_load(&w, opt->load_path, err, sizeof err);
        if (!ok) {
            fprintf(stderr, "%s: %s\n", opt->load_path, err);
            world_free(&w);
            return 1;
        }
    } else {
        world_genesis(&w);
    }
    FILE *dump = NULL;
    if (opt->dump_path) {
        dump = fopen(opt->dump_path, "wb");
        if (!dump) {
            perror(opt->dump_path);
            return 1;
        }
        dump_frame(dump, &w, 0);
    }
    FILE *hash = NULL;
    if (opt->hash_path) {
        hash = fopen(opt->hash_path, "wb");
        if (!hash) {
            perror(opt->hash_path);
            if (dump)
                fclose(dump);
            return 1;
        }
        hash_frame(hash, &w, 0);
    }
    double t0 = now_s();
    for (long f = 1; f <= opt->frames; f++) {
        world_time(&w);
        if (dump && (opt->dump_every <= 1 || f % opt->dump_every == 0))
            dump_frame(dump, &w, f);
        if (hash)
            hash_frame(hash, &w, f);
        if (!opt->quiet && opt->report_every > 0 && f % opt->report_every == 0)
            printf("frame %ld: population %d, organisms %d, O2 %.1f, CO2 %.1f\n", f,
                   w.population, w.norgs, w.o2, w.co2);
    }
    double dt = now_s() - t0;
    if (opt->save_path) {
        size_t n = strlen(opt->save_path);
        bool bgw = n > 4 && !strcmp(opt->save_path + n - 4, "." BGW_FILE_EXT);
        if (!(bgw ? bgw_save(&w, opt->save_path, err, sizeof err)
                  : worldfile_save(&w, opt->save_path, err, sizeof err))) {
            fprintf(stderr, "%s: %s\n", opt->save_path, err);
            world_free(&w);
            return 1;
        }
    }
    if (dump)
        fclose(dump);
    if (hash)
        fclose(hash);
    if (!opt->quiet)
        printf("%ld frames in %.3f s (%.0f frames/s); final population %d, O2 %.1f, CO2 %.1f, "
               "time %" PRId64 "\n",
               opt->frames, dt, dt > 0 ? opt->frames / dt : 0.0, w.population, w.o2, w.co2,
               w.stats.time);
    world_free(&w);
    return 0;
}
