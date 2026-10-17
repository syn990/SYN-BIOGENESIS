/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 * SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)
 */

/* Checks bgg_parse_java_double against tests/NumCheck.java's results.
 *
 *   num_check STRINGS RESULTS
 */
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "io/bgg.h"

int main(int argc, char **argv)
{
    FILE *in = argc == 3 ? fopen(argv[1], "r") : NULL;
    FILE *ex = argc == 3 ? fopen(argv[2], "r") : NULL;
    if (!in || !ex) {
        fprintf(stderr, "usage: num_check STRINGS RESULTS\n");
        return 2;
    }
    static char line[8192], want[64];
    long n = 0, bad = 0, accepted = 0;
    while (fgets(line, sizeof line, in) && fgets(want, sizeof want, ex)) {
        line[strcspn(line, "\n")] = '\0';
        want[strcspn(want, "\n")] = '\0';
        double v;
        char got[64] = "reject";
        if (bgg_parse_java_double(line, &v)) {
            uint64_t b;
            memcpy(&b, &v, sizeof b);
            snprintf(got, sizeof got, "%016" PRIx64, b);
            accepted++;
        }
        if (strcmp(got, want) != 0 && bad++ < 5)
            printf("  [%s]: Java %s, here %s\n", line, want, got);
        n++;
    }
    fclose(in);
    fclose(ex);
    if (bad) {
        printf("FAIL Double.parseDouble: %ld of %ld strings read differently\n", bad, n);
        return 1;
    }
    printf("Double.parseDouble: %ld random strings (%ld valid) read identically\n", n, accepted);
    return 0;
}
