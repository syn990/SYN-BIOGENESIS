/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Part of the colour build, a C port of Biogenesis Color Mod 2.01
 * (https://sourceforge.net/projects/biogenesis/), Copyright (C) 2006-2010
 * Joan Queralt Molina, Color Mod Copyright (C) 2012-2022 MarcoDBAA.
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

#include "sim/jrt.h"

#include <stdarg.h>
#include <stdio.h>

static void *xmalloc(size_t n)
{
    void *p = malloc(n ? n : 1);
    if (!p)
        abort();
    return p;
}

JColor jcolor_darker(JColor c)
{
    /* Color.darker: Math.max((int)(r * FACTOR), 0) with FACTOR 0.7 */
    int r = j_d2i(JCOLOR_R(c) * 0.7), g = j_d2i(JCOLOR_G(c) * 0.7), b = j_d2i(JCOLOR_B(c) * 0.7);
    return (c & 0xff000000u) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

int64_t j_lround(double a)
{
    /* java.lang.Math.round(double): shift is (SIGNIFICAND_WIDTH - 2 +
     * EXP_BIAS) - exponent, SIGNIFICAND_WIDTH being 53 */
    uint64_t bits;
    memcpy(&bits, &a, sizeof bits);
    int64_t biased_exp = (int64_t)((bits & 0x7ff0000000000000ull) >> 52);
    int64_t shift = (53 - 2 + 1023) - biased_exp;
    if ((shift & -64) == 0) {
        int64_t r = (int64_t)((bits & 0x000fffffffffffffull) | 0x0010000000000000ull);
        if ((int64_t)bits < 0)
            r = -r;
        return ((r >> shift) + 1) >> 1;
    }
    return j_d2l(a);
}

/* ---- arrays ---- */

#define JARR_HEAD 16 /* keeps doubles aligned */

void *jarr_new(int n, size_t elem)
{
    if (n < 0)
        abort(); /* NegativeArraySizeException */
    char *p = xmalloc(JARR_HEAD + (size_t)n * elem);
    memset(p, 0, JARR_HEAD + (size_t)n * elem);
    ((int *)p)[0] = n;
    ((int *)p)[1] = (int)elem;
    return p + JARR_HEAD;
}

void *jarr_dup(const void *a)
{
    if (!a)
        return NULL;
    const char *h = (const char *)a - JARR_HEAD;
    size_t n = (size_t)((const int *)h)[0] * (size_t)((const int *)h)[1];
    char *p = xmalloc(JARR_HEAD + n);
    memcpy(p, h, JARR_HEAD + n);
    return p + JARR_HEAD;
}

void jarr_free(void *a)
{
    if (a)
        free((char *)a - JARR_HEAD);
}

/* ---- strings ---- */

static const JGcKind k_string_kind = {"String", NULL, NULL};

static char *str_alloc(size_t len)
{
    char *h = jgc_alloc(sizeof(JGcHead) + len + 1, &k_string_kind);
    return h + sizeof(JGcHead);
}

char *jstr_new(const char *s)
{
    if (!s)
        return NULL;
    size_t n = strlen(s);
    char *d = str_alloc(n);
    memcpy(d, s, n + 1);
    return d;
}

char *jstr_newf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0)
        abort();
    char *s = str_alloc((size_t)n);
    va_start(ap, fmt);
    vsnprintf(s, (size_t)n + 1, fmt, ap);
    va_end(ap);
    return s;
}

/* Java interns every literal: one String per text, never collected (a
 * root: the header, which is what a root slot holds) */
static struct {
    char *s;
    void *head;
} g_interned[16];
static int g_ninterned;

char *jstr_lit(const char *text)
{
    for (int i = 0; i < g_ninterned; i++)
        if (!strcmp(g_interned[i].s, text))
            return g_interned[i].s;
    if (g_ninterned == (int)(sizeof g_interned / sizeof g_interned[0]))
        abort();
    char *s = jstr_new(text);
    g_interned[g_ninterned].s = s;
    g_interned[g_ninterned].head = s - sizeof(JGcHead);
    jgc_add_root(&g_interned[g_ninterned].head);
    g_ninterned++;
    return s;
}

char *jstr_replace(char *s, char a, char b)
{
    if (!s || a == b || !strchr(s, a))
        return s;
    char *d = jstr_new(s);
    for (char *p = d; *p; p++)
        if (*p == a)
            *p = b;
    return d;
}

void jstr_mark(const char *s)
{
    if (s)
        jgc_mark((char *)s - sizeof(JGcHead));
}

void jstr_copy_to(char *buf, size_t n, const char *s)
{
    size_t len = s ? strlen(s) : 0;
    if (len + 1 > n)
        abort();
    memcpy(buf, s ? s : "", len + 1);
}

/* ---- the collector ---- */

size_t jgc_min_fresh = 65536;

static JGcHead **g_objs;
static size_t g_nobjs, g_cap;
static size_t g_since_sweep;
static uint32_t g_epoch = 1;
static void ***g_roots;
static size_t g_nroots, g_roots_cap;

void *jgc_alloc(size_t size, const JGcKind *kind)
{
    JGcHead *h = xmalloc(size);
    memset(h, 0, size);
    if (g_nobjs == g_cap) {
        g_cap = g_cap ? g_cap * 2 : 1024;
        g_objs = realloc(g_objs, g_cap * sizeof *g_objs);
        if (!g_objs)
            abort();
    }
    h->index = (uint32_t)g_nobjs;
    h->kind = kind;
    g_objs[g_nobjs++] = h;
    g_since_sweep++;
    return h;
}

void jgc_mark(void *obj)
{
    JGcHead *h = obj;
    if (!h || h->mark == g_epoch)
        return;
    h->mark = g_epoch;
    if (h->kind->mark)
        h->kind->mark(obj);
}

void jgc_add_root(void **slot)
{
    if (g_nroots == g_roots_cap) {
        g_roots_cap = g_roots_cap ? g_roots_cap * 2 : 16;
        g_roots = realloc(g_roots, g_roots_cap * sizeof *g_roots);
        if (!g_roots)
            abort();
    }
    g_roots[g_nroots++] = slot;
}

void jgc_remove_root(void **slot)
{
    for (size_t i = 0; i < g_nroots; i++) {
        if (g_roots[i] == slot) {
            g_roots[i] = g_roots[--g_nroots];
            return;
        }
    }
}

static void (*g_markers[8])(void);
static size_t g_nmarkers;

void jgc_add_marker(void (*fn)(void))
{
    for (size_t i = 0; i < g_nmarkers; i++)
        if (g_markers[i] == fn)
            return;
    if (g_nmarkers < sizeof g_markers / sizeof g_markers[0])
        g_markers[g_nmarkers++] = fn;
}

void jgc_sweep(void)
{
    for (size_t i = 0; i < g_nroots; i++)
        jgc_mark(*g_roots[i]);
    for (size_t i = 0; i < g_nmarkers; i++)
        g_markers[i]();
    size_t k = 0;
    for (size_t i = 0; i < g_nobjs; i++) {
        JGcHead *h = g_objs[i];
        if (h->mark == g_epoch) {
            h->index = (uint32_t)k;
            g_objs[k++] = h;
        } else {
            if (h->kind->finalize)
                h->kind->finalize(h);
            free(h);
        }
    }
    g_nobjs = k;
    g_since_sweep = 0;
    /* a new epoch makes every object unmarked again without touching them */
    if (++g_epoch == 0)
        g_epoch = 1;
}

size_t jgc_count(void)
{
    return g_nobjs;
}

size_t jgc_allocated_since_sweep(void)
{
    return g_since_sweep;
}
