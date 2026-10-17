/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Translated to C in 2026 from BioXMLParser.java
 * of Biogenesis 0.9 (https://biogenesis.sourceforge.net/),
 * Copyright (C) 2006-2010 Joan Queralt Molina.
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

#include "io/bgg.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Double.toString-compatible enough for Double.parseDouble: the shortest
 * digits that round-trip, with ".0" on integral values like Java prints. */
static void format_double(char *out, size_t n, double v)
{
    for (int prec = 1; prec <= 17; prec++) {
        snprintf(out, n, "%.*g", prec, v);
        if (strtod(out, NULL) == v)
            break;
    }
    if (!strpbrk(out, ".eEnN"))
        strncat(out, ".0", n - strlen(out) - 1);
}

static const char *bgg_color_name(int c)
{
    switch (c) {
    case COL_RED: return "red";
    case COL_GREEN: return "green";
    case COL_BLUE: return "blue";
    case COL_CYAN: return "cyan";
    case COL_WHITE: return "white";
    case COL_GRAY: return "gray";
    case COL_YELLOW: return "yellow";
    }
    return "";
}

bool bgg_write(const char *path, const GeneticCode *gc)
{
    FILE *f = fopen(path, "w");
    if (!f)
        return false;
    fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          "<!DOCTYPE genetic_code [\n"
          "<!ELEMENT genetic_code (gene+)>\n"
          "<!ATTLIST genetic_code symmetry (1|2|3|4|5|6|7|8) #REQUIRED>\n"
          "<!ATTLIST genetic_code mirror (yes|no) #REQUIRED>\n"
          "<!ATTLIST genetic_code disperse (yes|no) #REQUIRED>\n"
          "<!ELEMENT gene EMPTY>\n"
          "<!ATTLIST gene length CDATA #REQUIRED>\n"
          "<!ATTLIST gene theta CDATA #REQUIRED>\n"
          "<!ATTLIST gene color (red|green|blue|cyan|yellow|gray|white) #REQUIRED>\n"
          "]>\n",
          f);
    fprintf(f, "<genetic_code symmetry=\"%d\" mirror=\"%s\" disperse=\"%s\">\n", gc->symmetry,
            gc->mirror == 0 ? "no" : "yes", gc->disperse_children ? "yes" : "no");
    for (int i = 0; i < gc->ngenes; i++) {
        char len[40], th[40];
        format_double(len, sizeof len, gc->genes[i].length);
        format_double(th, sizeof th, gc->genes[i].theta);
        fprintf(f, "\t<gene length=\"%s\" theta=\"%s\" color=\"%s\" />\n", len, th,
                bgg_color_name(gc->genes[i].color));
    }
    fputs("</genetic_code>\n", f);
    return fclose(f) == 0;
}

/* ---- a small XML reader: just enough for this format ---- */

typedef struct {
    const char *p, *end;
} Cursor;

typedef struct {
    char name[32];
    struct {
        char key[32];
        char val[64];
    } attr[8];
    int nattr;
    bool closing;
} Tag;

static void set_err(char *err, size_t n, const char *fmt, ...)
{
    if (!err || !n)
        return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(err, n, fmt, ap);
    va_end(ap);
}

static const char *find(const char *p, const char *end, const char *needle)
{
    size_t nl = strlen(needle);
    for (; p + nl <= end; p++)
        if (!memcmp(p, needle, nl))
            return p;
    return NULL;
}

static bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static void decode_entities(char *s)
{
    static const struct {
        const char *ent;
        char ch;
    } ents[] = {{"&amp;", '&'}, {"&lt;", '<'}, {"&gt;", '>'}, {"&quot;", '"'}, {"&apos;", '\''}};
    char *w = s;
    for (char *r = s; *r;) {
        bool done = false;
        if (*r == '&') {
            for (size_t i = 0; i < sizeof ents / sizeof ents[0]; i++) {
                size_t l = strlen(ents[i].ent);
                if (!strncmp(r, ents[i].ent, l)) {
                    *w++ = ents[i].ch;
                    r += l;
                    done = true;
                    break;
                }
            }
        }
        if (!done)
            *w++ = *r++;
    }
    *w = '\0';
}

/* Next element tag, skipping text, the prolog, comments and the DOCTYPE.
 * Returns 1 on a tag, 0 at end of input, -1 on malformed input. */
static int next_tag(Cursor *c, Tag *t)
{
    for (;;) {
        while (c->p < c->end && *c->p != '<')
            c->p++;
        if (c->p >= c->end)
            return 0;
        const char *p = c->p;
        if (c->end - p >= 4 && !memcmp(p, "<!--", 4)) {
            const char *q = find(p + 4, c->end, "-->");
            if (!q)
                return -1;
            c->p = q + 3;
            continue;
        }
        if (c->end - p >= 2 && p[1] == '?') {
            const char *q = find(p + 2, c->end, "?>");
            if (!q)
                return -1;
            c->p = q + 2;
            continue;
        }
        if (c->end - p >= 2 && p[1] == '!') {
            /* <!DOCTYPE ... [ internal subset ] > */
            int depth = 0;
            const char *q = p + 2;
            for (; q < c->end; q++) {
                if (*q == '[')
                    depth++;
                else if (*q == ']')
                    depth--;
                else if (*q == '>' && depth <= 0)
                    break;
            }
            if (q >= c->end)
                return -1;
            c->p = q + 1;
            continue;
        }
        break;
    }
    const char *p = c->p + 1;
    memset(t, 0, sizeof *t);
    if (p < c->end && *p == '/') {
        t->closing = true;
        p++;
    }
    size_t n = 0;
    while (p < c->end && !is_space(*p) && *p != '>' && *p != '/') {
        if (n + 1 < sizeof t->name)
            t->name[n++] = *p;
        p++;
    }
    for (;;) {
        while (p < c->end && is_space(*p))
            p++;
        if (p >= c->end)
            return -1;
        if (*p == '>') {
            p++;
            break;
        }
        if (*p == '/') {
            p++;
            continue;
        }
        char key[32] = {0};
        n = 0;
        while (p < c->end && *p != '=' && !is_space(*p) && *p != '>') {
            if (n + 1 < sizeof key)
                key[n++] = *p;
            p++;
        }
        while (p < c->end && is_space(*p))
            p++;
        if (p >= c->end || *p != '=')
            return -1;
        p++;
        while (p < c->end && is_space(*p))
            p++;
        if (p >= c->end || (*p != '"' && *p != '\''))
            return -1;
        char q = *p++;
        const char *vs = p;
        while (p < c->end && *p != q)
            p++;
        if (p >= c->end)
            return -1;
        if (t->nattr < 8) {
            snprintf(t->attr[t->nattr].key, sizeof t->attr[0].key, "%s", key);
            size_t vl = (size_t)(p - vs);
            if (vl >= sizeof t->attr[0].val)
                vl = sizeof t->attr[0].val - 1;
            memcpy(t->attr[t->nattr].val, vs, vl);
            t->attr[t->nattr].val[vl] = '\0';
            decode_entities(t->attr[t->nattr].val);
            t->nattr++;
        }
        p++;
    }
    c->p = p;
    return 1;
}

static const char *attr(const Tag *t, const char *key)
{
    for (int i = 0; i < t->nattr; i++)
        if (!strcmp(t->attr[i].key, key))
            return t->attr[i].val;
    return NULL;
}

static bool parse_java_double(const char *s, double *out)
{
    if (!s)
        return false;
    while (is_space(*s))
        s++;
    char *end;
    errno = 0;
    double v = strtod(s, &end);
    if (end == s)
        return false;
    while (is_space(*end))
        end++;
    /* Java also accepts a trailing d/D/f/F type suffix */
    if (*end == 'd' || *end == 'D' || *end == 'f' || *end == 'F')
        end++;
    if (*end)
        return false;
    *out = v;
    return true;
}

static int parse_color(const char *s)
{
    if (!s)
        return -1;
    if (!strcmp(s, "green")) return COL_GREEN;
    if (!strcmp(s, "red")) return COL_RED;
    if (!strcmp(s, "blue")) return COL_BLUE;
    if (!strcmp(s, "cyan")) return COL_CYAN;
    if (!strcmp(s, "gray")) return COL_GRAY;
    if (!strcmp(s, "white")) return COL_WHITE;
    if (!strcmp(s, "yellow")) return COL_YELLOW;
    return -1;
}

GeneticCode *bgg_parse(const char *xml, size_t len, char *err, size_t errn)
{
    Cursor c = {xml, xml + len};
    Tag t;
    int r = next_tag(&c, &t);
    if (r <= 0 || t.closing || strcmp(t.name, "genetic_code") != 0) {
        set_err(err, errn, "This file does not contain a genetic_code.");
        return NULL;
    }
    const char *s = attr(&t, "symmetry");
    char *end;
    long sym = s ? strtol(s, &end, 10) : 0;
    if (!s || !*s || *end || sym < 1 || sym > 8) {
        set_err(err, errn, "Symmetry has not an allowed value.");
        return NULL;
    }
    int mirror;
    s = attr(&t, "mirror");
    if (s && !strcmp(s, "yes"))
        mirror = 1;
    else if (s && !strcmp(s, "no"))
        mirror = 0;
    else {
        set_err(err, errn, "Mirror has not an allowed value.");
        return NULL;
    }
    bool disperse;
    s = attr(&t, "disperse");
    if (s && !strcmp(s, "yes"))
        disperse = true;
    else if (s && !strcmp(s, "no"))
        disperse = false;
    else {
        set_err(err, errn, "Disperse has not an allowed value.");
        return NULL;
    }

    int cap = 16, n = 0;
    Gene *genes = malloc((size_t)cap * sizeof *genes);
    if (!genes)
        abort();
    while ((r = next_tag(&c, &t)) > 0) {
        if (t.closing) {
            if (!strcmp(t.name, "genetic_code"))
                break;
            continue;
        }
        if (strcmp(t.name, "gene") != 0) {
            set_err(err, errn, "Parse error. %s found but gene expected.", t.name);
            free(genes);
            return NULL;
        }
        Gene g;
        if (!parse_java_double(attr(&t, "length"), &g.length) ||
            !parse_java_double(attr(&t, "theta"), &g.theta)) {
            set_err(err, errn,
                    "Attributes length and theta do not exist or have not an allowed value.");
            free(genes);
            return NULL;
        }
        int col = parse_color(attr(&t, "color"));
        if (col < 0) {
            set_err(err, errn, "Attribute color does not exist or has not an allowed value.");
            free(genes);
            return NULL;
        }
        g.color = (uint8_t)col;
        if (n == cap) {
            cap *= 2;
            Gene *ng = realloc(genes, (size_t)cap * sizeof *genes);
            if (!ng)
                abort();
            genes = ng;
        }
        genes[n++] = g;
    }
    if (r < 0) {
        set_err(err, errn, "Malformed XML.");
        free(genes);
        return NULL;
    }
    if (n == 0) {
        set_err(err, errn, "A genetic code needs at least one gene.");
        free(genes);
        return NULL;
    }
    GeneticCode *gc = gc_new_from_genes(genes, n, (int)sym, mirror, disperse);
    free(genes);
    return gc;
}

GeneticCode *bgg_read(const char *path, char *err, size_t errn)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        set_err(err, errn, "This file can't be read");
        return NULL;
    }
    size_t cap = 4096, len = 0;
    char *buf = malloc(cap);
    if (!buf)
        abort();
    size_t got;
    while ((got = fread(buf + len, 1, cap - len, f)) > 0) {
        len += got;
        if (len == cap) {
            if (cap >= (16u << 20))
                break; /* no genetic code is 16 MB */
            cap *= 2;
            char *nb = realloc(buf, cap);
            if (!nb)
                abort();
            buf = nb;
        }
    }
    fclose(f);
    GeneticCode *gc = bgg_parse(buf, len, err, errn);
    free(buf);
    return gc;
}
