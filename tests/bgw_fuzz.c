/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 * SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)
 */

/* Damaged .bgw files: N copies of a good one with flipped and random bytes,
 * truncations and stray back-references, each loaded and, if it loads,
 * run for 20 frames. Refusing is fine; crashing is not (build with
 * -fsanitize=address,undefined to see more).
 *
 *   bgw_fuzz FILE N
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io/bgw.h"
#include "sim/jrandom.h"
#include "sim/params.h"

int main(int argc, char **argv)
{
    FILE *f = argc == 3 ? fopen(argv[1], "rb") : NULL;
    if (!f) {
        fprintf(stderr, "usage: bgw_fuzz FILE N\n");
        return 2;
    }
    params_set_defaults(&P);
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *src = malloc((size_t)n), *buf = malloc((size_t)n);
    if (!src || !buf || fread(src, 1, (size_t)n, f) != (size_t)n)
        return 2;
    fclose(f);
    char path[1100];
    snprintf(path, sizeof path, "%s.damaged", argv[1]);
    int iters = atoi(argv[2]), loaded = 0;
    uint64_t r = 12345;
    for (int it = 0; it < iters; it++) {
        long len = n;
        memcpy(buf, src, (size_t)n);
        for (int k = 0; k < 1 + it % 6; k++) {
            r = r * 6364136223846793005ULL + 1442695040888963407ULL;
            long at = (long)((r >> 33) % (uint64_t)n);
            switch (it % 4) {
            case 0: buf[at] ^= (unsigned char)(1u << (r % 8)); break;
            case 1: buf[at] = (unsigned char)(r >> 24); break;
            case 2: len = at; break;
            default: /* a TC_REFERENCE to some handle */
                if (at + 4 < n) {
                    buf[at] = 0x71;
                    buf[at + 1] = 0;
                    buf[at + 2] = 0x7e;
                    buf[at + 3] = (unsigned char)(r >> 40);
                    buf[at + 4] = (unsigned char)(r >> 48);
                }
            }
        }
        FILE *o = fopen(path, "wb");
        if (!o || fwrite(buf, 1, (size_t)len, o) != (size_t)len)
            return 2;
        fclose(o);
        World w;
        world_init(&w);
        char err[256];
        if (bgw_load(&w, path, err, sizeof err)) {
            loaded++;
            jr_set_seed(&g_rng, it);
            for (int k = 0; k < 20; k++)
                world_time(&w);
        }
        world_free(&w);
    }
    remove(path);
    free(src);
    free(buf);
    printf(".bgw damage: %d damaged files, %d refused, %d loaded and ran; no crash\n", iters,
           iters - loaded, loaded);
    return 0;
}
