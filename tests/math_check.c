/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 * SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)
 */

/* Reads tests/MathCheck.java's records and checks that bio_sin, bio_cos,
 * bio_atan2 and bio_pow return exactly what the JVM's Math did.
 *
 *   math_check FILE
 */
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "sim/jmath.h"

static int same(double a, double b)
{
    uint64_t x, y;
    memcpy(&x, &a, 8);
    memcpy(&y, &b, 8);
    /* Java writes NaN results as its one canonical NaN */
    return x == y || (a != a && b != b);
}

int main(int argc, char **argv)
{
    FILE *f = argc == 2 ? fopen(argv[1], "rb") : NULL;
    if (!f) {
        fprintf(stderr, "usage: math_check FILE\n");
        return 2;
    }
    static const char *names[] = {"sin", "cos", "atan2", "pow"};
    long n = 0, bad[4] = {0};
    double r[7];
    while (fread(r, sizeof r, 1, f) == 1) {
        double c[4] = {bio_sin(r[0]), bio_cos(r[0]), bio_atan2(r[1], r[2]), bio_pow(r[1], r[2])};
        for (int k = 0; k < 4; k++) {
            if (!same(c[k], r[3 + k]) && bad[k]++ == 0)
                printf("  %s(%a, %a): %a, the JVM %a\n", names[k], k < 2 ? r[0] : r[1],
                       k < 2 ? 0.0 : r[2], c[k], r[3 + k]);
        }
        n++;
    }
    fclose(f);
    if (!bad[0] && !bad[1] && !bad[2] && !bad[3]) {
        printf("Math.sin/cos/atan2/pow: %ld random inputs identical to the JVM\n", n);
        return 0;
    }
    printf("FAIL maths differs from the JVM: sin %ld, cos %ld, atan2 %ld, pow %ld of %ld\n", bad[0],
           bad[1], bad[2], bad[3], n);
    return 1;
}
