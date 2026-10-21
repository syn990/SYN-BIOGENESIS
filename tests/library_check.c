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

/* The being library (src/syn/library.c) in a folder of its own:
 *
 * 1. A new library has its first folders; codes put in come back exactly,
 *    with where they came from; a taken name gets a number.
 * 2. Beings move, are renamed, go to the Bin and from it; folders are made
 *    and removed (only empty ones, never the Bin).
 * 3. A .bgg from elsewhere is copied in byte for byte, and one the
 *    simulation can't read is refused.
 * 4. Names that would leave the library ("../x", "a/b") stay inside it.
 *
 *   tests/library_check DIR   (DIR: an empty folder to use)
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io/bgg.h"
#include "sim/genetic_code.h"
#include "sim/jrandom.h"
#include "sim/params.h"
#include "syn/library.h"

static int failures;

#define CHECK(cond, ...)                                                                           \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            printf("FAIL library: " __VA_ARGS__);                                                  \
            printf("\n");                                                                          \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

static bool same(const GeneticCode *a, const GeneticCode *b)
{
    if (!a || !b || a->ngenes != b->ngenes || a->symmetry != b->symmetry || a->mirror != b->mirror ||
        a->disperse_children != b->disperse_children)
        return false;
    for (int i = 0; i < a->ngenes; i++)
        if (memcmp(&a->genes[i].length, &b->genes[i].length, sizeof(double)) ||
            memcmp(&a->genes[i].theta, &b->genes[i].theta, sizeof(double)) ||
            a->genes[i].color != b->genes[i].color)
            return false;
    return true;
}

static bool has_folder(const char *want)
{
    static char f[64][LIB_PATH];
    int n = syn_library_folders(f, 64);
    for (int i = 0; i < n; i++)
        if (!strcmp(f[i], want))
            return true;
    return false;
}

static bool file_bytes(const char *path, char *buf, size_t cap, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    *len = fread(buf, 1, cap, f);
    fclose(f);
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: tests/library_check DIR\n");
        return 2;
    }
    params_set_defaults(&P);
    jr_set_seed(&g_rng, 77);
    char root[1024], err[300], got[LIB_NAME];
    snprintf(root, sizeof root, "%s/beings", argv[1]);
    syn_library_set_root(root);

    /* 1 */
    CHECK(syn_library_ready(err, sizeof err), "not made: %s", err);
    CHECK(has_folder("") && has_folder("Saved") && has_folder("Imported") && has_folder("Arena") &&
              has_folder(LIB_BIN),
          "the first folders");
    GeneticCode *code[5];
    for (int i = 0; i < 5; i++) {
        code[i] = gc_new_random();
        CHECK(syn_library_put("Saved", "VELORAX-3", code[i], "test", got, sizeof got, err, sizeof err),
              "put %d: %s", i, err);
    }
    CHECK(!strcmp(got, "VELORAX-3 (5)"), "the fifth name is %s", got);
    LibEntry e[16];
    int n = syn_library_list("Saved", e, 16);
    CHECK(n == 5, "%d in Saved", n);
    CHECK(n > 0 && !strcmp(e[0].name, "VELORAX-3") && !strcmp(e[0].origin, "test") && e[0].saved[0],
          "the first: %s, %s, %s", n ? e[0].name : "", n ? e[0].origin : "", n ? e[0].saved : "");
    GeneticCode *back = syn_library_read("Saved", "VELORAX-3 (2)", err, sizeof err);
    CHECK(same(back, code[1]), "the second code read back");
    gc_unref(back);

    /* 2 */
    CHECK(syn_library_new_folder("Arena", "Level 4", got, sizeof got, err, sizeof err) &&
              !strcmp(got, "Arena/Level 4"),
          "new folder: %s %s", got, err);
    CHECK(syn_library_move("Saved", "VELORAX-3", "Arena/Level 4", err, sizeof err), "move: %s", err);
    CHECK(syn_library_count("Saved") == 4 && syn_library_count("Arena/Level 4") == 1, "after the move");
    back = syn_library_read("Arena/Level 4", "VELORAX-3", err, sizeof err);
    CHECK(same(back, code[0]), "the moved code");
    gc_unref(back);
    n = syn_library_list("Arena/Level 4", e, 16);
    CHECK(n == 1 && !strcmp(e[0].origin, "test"), "the moved being kept its origin");
    CHECK(syn_library_rename("Arena/Level 4", "VELORAX-3", "CHAMPION", got, sizeof got, err,
                             sizeof err) &&
              !strcmp(got, "CHAMPION"),
          "rename: %s", err);
    CHECK(!syn_library_remove_folder("Arena/Level 4", err, sizeof err), "a full folder removed");
    CHECK(syn_library_delete("Arena/Level 4", "CHAMPION", err, sizeof err), "to the bin: %s", err);
    CHECK(syn_library_count(LIB_BIN) == 1 && syn_library_count("Arena/Level 4") == 0, "in the bin");
    CHECK(syn_library_remove_folder("Arena/Level 4", err, sizeof err) && !has_folder("Arena/Level 4"),
          "an empty folder removed: %s", err);
    CHECK(!syn_library_remove_folder(LIB_BIN, err, sizeof err), "the bin removed");
    CHECK(syn_library_delete(LIB_BIN, "CHAMPION", err, sizeof err) && syn_library_count(LIB_BIN) == 0,
          "gone from the bin: %s", err);
    char moved[LIB_PATH];
    CHECK(syn_library_rename_folder("Saved", "Kept", moved, sizeof moved, err, sizeof err) &&
              !strcmp(moved, "Kept") && syn_library_count("Kept") == 4 && !has_folder("Saved"),
          "a folder renamed: %s %s", moved, err);
    CHECK(!syn_library_rename_folder(LIB_BIN, "Other", moved, sizeof moved, err, sizeof err),
          "the bin renamed");
    CHECK(syn_library_delete("Kept", "VELORAX-3 (2)", err, sizeof err) &&
              syn_library_delete("Kept", "VELORAX-3 (3)", err, sizeof err) &&
              syn_library_empty_bin() == 2 && syn_library_count(LIB_BIN) == 0,
          "the bin emptied");

    /* 3 */
    char src[1200], dst[1400], a[65536], b[65536];
    size_t la = 0, lb = 0;
    snprintf(src, sizeof src, "%s/outside.bgg", argv[1]);
    CHECK(bgg_write(src, code[2]), "a .bgg outside");
    CHECK(syn_library_import(src, "Imported", got, sizeof got, err, sizeof err) &&
              !strcmp(got, "outside"),
          "import: %s", err);
    snprintf(dst, sizeof dst, "%s/Imported/outside.bgg", root);
    CHECK(file_bytes(src, a, sizeof a, &la) && file_bytes(dst, b, sizeof b, &lb) && la == lb &&
              !memcmp(a, b, la),
          "imported byte for byte");
    snprintf(src, sizeof src, "%s/broken.bgg", argv[1]);
    FILE *f = fopen(src, "w");
    if (f) {
        fputs("<genetic_code symmetry=\"9\">", f);
        fclose(f);
    }
    CHECK(!syn_library_import(src, "Imported", got, sizeof got, err, sizeof err),
          "a broken file imported");
    CHECK(syn_library_count("Imported") == 1, "only the good one in Imported");

    /* 4 */
    CHECK(syn_library_put("../escape", "../../x/y", code[3], "", got, sizeof got, err, sizeof err),
          "put with a strange name: %s", err);
    CHECK(has_folder("escape") && syn_library_count("escape") == 1, "kept inside the library");
    n = syn_library_list("escape", e, 16);
    CHECK(n == 1 && !strchr(e[0].name, '/') && e[0].name[0] != '.', "the name kept: %s",
          n ? e[0].name : "");

    for (int i = 0; i < 5; i++)
        gc_unref(code[i]);
    if (!failures)
        printf("library: folders made, moved and removed, beings kept exactly and moved, renamed, "
               "binned, imported byte for byte; strange names kept inside\n");
    return failures != 0;
}
