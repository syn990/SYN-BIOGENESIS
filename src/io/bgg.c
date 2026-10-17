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

#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Double.toString-compatible enough for Double.parseDouble: the shortest
 * digits that round-trip, with ".0" on integral values like Java prints. */
static void format_double(char *out, size_t n, double v)
{
    if (v != v || v == HUGE_VAL || v == -HUGE_VAL) {
        snprintf(out, n, "%s", v != v ? "NaN" : v > 0 ? "Infinity" : "-Infinity");
        return;
    }
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

/* The DOCTYPE BioXMLParser.writeGeneticCode writes */
static const char k_doctype[] =
    "<!DOCTYPE genetic_code [\n"
    "<!ELEMENT genetic_code (gene+)>\n"
    "<!ATTLIST genetic_code symmetry (1|2|3|4|5|6|7|8) #REQUIRED>\n"
    "<!ATTLIST genetic_code mirror (yes|no) #REQUIRED>\n"
    "<!ATTLIST genetic_code disperse (yes|no) #REQUIRED>\n"
    "<!ELEMENT gene EMPTY>\n"
    "<!ATTLIST gene length CDATA #REQUIRED>\n"
    "<!ATTLIST gene theta CDATA #REQUIRED>\n"
    "<!ATTLIST gene color (red|green|blue|cyan|yellow|gray|white) #REQUIRED>\n"
    "]>\n";

bool bgg_write(const char *path, const GeneticCode *gc)
{
    FILE *f = fopen(path, "w");
    if (!f)
        return false;
    fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n", f);
    fputs(k_doctype, f);
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

/* ---- reading ----
 *
 * 0.9 reads a genetic code with a validating XML parser (Xerces, through
 * DocumentBuilder, against the DTD inside the file), then Integer.parseInt
 * and Double.parseDouble on the attribute values. This reader accepts the
 * same files and reads the same values: XML 1.0 line-end and attribute-value
 * normalisation, character and entity references, the internal DTD's
 * attribute types, defaults and entities, and Java's number grammar. One
 * difference: a file with no DOCTYPE, which 0.9 refuses ("no grammar
 * found"), opens here, read as if it had the DOCTYPE 0.9 writes. */

typedef struct {
    char elem[32], name[32];
    bool listed;   /* an enumeration or a token type: normalised further */
    char *tokens;  /* "|a|b|" for enumerations, NULL otherwise */
    int deflt;     /* 0 #IMPLIED, 1 #REQUIRED, 2 default value, 3 #FIXED */
    char *value;   /* the default or fixed value */
} AttDecl;

typedef struct {
    char name[32];
    char *value; /* replacement text */
} EntDecl;

typedef struct {
    const char *p, *end;
    bool has_dtd;
    char doctype[32];
    AttDecl *att;
    int natt;
    EntDecl *ent;
    int nent;
    bool declared_gc, declared_gene; /* <!ELEMENT> seen */
    char *err;
    size_t errn;
    bool failed;
} Reader;

typedef struct {
    char name[32];
    char *val;
    bool specified;
} Attr;

typedef struct {
    char name[32];
    Attr *attr;
    int nattr;
    bool empty; /* <x/> */
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

static bool fail(Reader *r, const char *msg)
{
    if (!r->failed)
        set_err(r->err, r->errn, "%s", msg);
    r->failed = true;
    return false;
}

static char *xstrndup(const char *s, size_t n)
{
    char *d = malloc(n + 1);
    if (!d)
        abort();
    memcpy(d, s, n);
    d[n] = '\0';
    return d;
}

/* ---- text buffer ---- */

typedef struct {
    char *s;
    size_t n, cap;
} Buf;

static void buf_put(Buf *b, char c)
{
    if (b->n + 2 > b->cap) {
        b->cap = b->cap ? b->cap * 2 : 64;
        b->s = realloc(b->s, b->cap);
        if (!b->s)
            abort();
    }
    b->s[b->n++] = c;
    b->s[b->n] = '\0';
}

static void buf_utf8(Buf *b, unsigned long cp)
{
    if (cp < 0x80) {
        buf_put(b, (char)cp);
    } else if (cp < 0x800) {
        buf_put(b, (char)(0xc0 | cp >> 6));
        buf_put(b, (char)(0x80 | (cp & 0x3f)));
    } else if (cp < 0x10000) {
        buf_put(b, (char)(0xe0 | cp >> 12));
        buf_put(b, (char)(0x80 | ((cp >> 6) & 0x3f)));
        buf_put(b, (char)(0x80 | (cp & 0x3f)));
    } else {
        buf_put(b, (char)(0xf0 | cp >> 18));
        buf_put(b, (char)(0x80 | ((cp >> 12) & 0x3f)));
        buf_put(b, (char)(0x80 | ((cp >> 6) & 0x3f)));
        buf_put(b, (char)(0x80 | (cp & 0x3f)));
    }
}

/* The document as UTF-8 with XML 1.0 line ends (CR LF and CR become LF):
 * UTF-16 with a byte order mark is converted, a UTF-8 one dropped. */
static char *prepare(const char *in, size_t len, size_t *outlen)
{
    Buf b = {0};
    const unsigned char *u = (const unsigned char *)in;
    if (len >= 2 && ((u[0] == 0xff && u[1] == 0xfe) || (u[0] == 0xfe && u[1] == 0xff))) {
        bool le = u[0] == 0xff;
        for (size_t i = 2; i + 1 < len; i += 2) {
            unsigned long c = le ? (unsigned long)(u[i] | u[i + 1] << 8)
                                 : (unsigned long)(u[i] << 8 | u[i + 1]);
            if (c >= 0xd800 && c < 0xdc00 && i + 3 < len) {
                unsigned long d = le ? (unsigned long)(u[i + 2] | u[i + 3] << 8)
                                     : (unsigned long)(u[i + 2] << 8 | u[i + 3]);
                if (d >= 0xdc00 && d < 0xe000) {
                    c = 0x10000 + ((c - 0xd800) << 10) + (d - 0xdc00);
                    i += 2;
                }
            }
            buf_utf8(&b, c);
        }
    } else {
        size_t i = len >= 3 && u[0] == 0xef && u[1] == 0xbb && u[2] == 0xbf ? 3 : 0;
        for (; i < len; i++)
            buf_put(&b, (char)u[i]);
    }
    buf_put(&b, '\0'); /* an empty document still gets a buffer */
    b.n--;
    /* line ends */
    size_t w = 0;
    for (size_t r = 0; r < b.n; r++) {
        if (b.s[r] == '\r') {
            b.s[w++] = '\n';
            if (r + 1 < b.n && b.s[r + 1] == '\n')
                r++;
        } else {
            b.s[w++] = b.s[r];
        }
    }
    b.s[w] = '\0';
    *outlen = w;
    return b.s;
}

/* ---- lexing ---- */

static bool is_ws(char c)
{
    return c == ' ' || c == '\t' || c == '\n';
}

static bool name_start(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || c == ':' ||
           (unsigned char)c >= 0x80;
}

static bool name_char(char c)
{
    return name_start(c) || (c >= '0' && c <= '9') || c == '.' || c == '-';
}

static bool starts(const Reader *r, const char *s)
{
    size_t n = strlen(s);
    return (size_t)(r->end - r->p) >= n && !memcmp(r->p, s, n);
}

static bool skip_ws(Reader *r)
{
    const char *p = r->p;
    while (r->p < r->end && is_ws(*r->p))
        r->p++;
    return r->p > p;
}

static bool read_name(Reader *r, char *out, size_t n)
{
    if (r->p >= r->end || !name_start(*r->p))
        return fail(r, "Malformed XML: a name was expected.");
    size_t k = 0;
    while (r->p < r->end && name_char(*r->p)) {
        if (k + 1 >= n)
            return fail(r, "Malformed XML: a name is too long.");
        out[k++] = *r->p++;
    }
    out[k] = '\0';
    return true;
}

static bool skip_past(Reader *r, const char *s)
{
    size_t n = strlen(s);
    for (; r->p + n <= r->end; r->p++) {
        if (!memcmp(r->p, s, n)) {
            r->p += n;
            return true;
        }
    }
    return fail(r, "Malformed XML: unterminated markup.");
}

/* <!-- ... --> ("--" may not occur inside) or <? ... ?> */
static bool skip_misc(Reader *r, bool *skipped)
{
    *skipped = false;
    if (starts(r, "<!--")) {
        r->p += 4;
        for (; r->p + 1 < r->end; r->p++) {
            if (r->p[0] == '-' && r->p[1] == '-') {
                if (r->p + 2 < r->end && r->p[2] == '>') {
                    r->p += 3;
                    *skipped = true;
                    return true;
                }
                return fail(r, "Malformed XML: \"--\" inside a comment.");
            }
        }
        return fail(r, "Malformed XML: unterminated comment.");
    }
    if (starts(r, "<?")) {
        r->p += 2;
        char target[32];
        if (!read_name(r, target, sizeof target))
            return false;
        if (!strcmp(target, "xml"))
            return fail(r, "Malformed XML: misplaced XML declaration.");
        *skipped = true;
        return skip_past(r, "?>");
    }
    return true;
}

static bool quoted(Reader *r, const char **s, size_t *n)
{
    if (r->p >= r->end || (*r->p != '"' && *r->p != '\''))
        return fail(r, "Malformed XML: a quoted value was expected.");
    char q = *r->p++;
    *s = r->p;
    while (r->p < r->end && *r->p != q)
        r->p++;
    if (r->p >= r->end)
        return fail(r, "Malformed XML: unterminated value.");
    *n = (size_t)(r->p - *s);
    r->p++;
    return true;
}

/* ---- the internal DTD ---- */

static const AttDecl *find_att(const Reader *r, const char *elem, const char *name)
{
    for (int i = 0; i < r->natt; i++)
        if (!strcmp(r->att[i].elem, elem) && !strcmp(r->att[i].name, name))
            return &r->att[i];
    return NULL;
}

static bool expand(Reader *r, Buf *out, const char *s, size_t n, int depth);

/* <!ATTLIST elem (name type default)*> */
static bool dtd_attlist(Reader *r)
{
    char elem[32];
    if (!skip_ws(r) || !read_name(r, elem, sizeof elem))
        return fail(r, "Malformed DTD: ATTLIST.");
    for (;;) {
        skip_ws(r);
        if (r->p < r->end && *r->p == '>') {
            r->p++;
            return true;
        }
        AttDecl d;
        memset(&d, 0, sizeof d);
        snprintf(d.elem, sizeof d.elem, "%s", elem);
        if (!read_name(r, d.name, sizeof d.name) || !skip_ws(r))
            return fail(r, "Malformed DTD: ATTLIST.");
        if (r->p < r->end && *r->p == '(') {
            const char *s = ++r->p;
            while (r->p < r->end && *r->p != ')')
                r->p++;
            if (r->p >= r->end)
                return fail(r, "Malformed DTD: ATTLIST.");
            Buf t = {0};
            buf_put(&t, '|');
            for (const char *q = s; q < r->p; q++)
                if (!is_ws(*q))
                    buf_put(&t, *q);
            buf_put(&t, '|');
            d.tokens = t.s;
            d.listed = true;
            r->p++;
        } else {
            char type[16];
            if (!read_name(r, type, sizeof type))
                return fail(r, "Malformed DTD: ATTLIST.");
            if (!strcmp(type, "NOTATION"))
                return fail(r, "Unsupported DTD: NOTATION attribute.");
            d.listed = strcmp(type, "CDATA") != 0;
        }
        if (!skip_ws(r))
            return fail(r, "Malformed DTD: ATTLIST.");
        if (starts(r, "#REQUIRED")) {
            r->p += 9;
            d.deflt = 1;
        } else if (starts(r, "#IMPLIED")) {
            r->p += 8;
        } else {
            d.deflt = 2;
            if (starts(r, "#FIXED")) {
                r->p += 6;
                d.deflt = 3;
                skip_ws(r);
            }
            const char *s;
            size_t n;
            if (!quoted(r, &s, &n))
                return false;
            Buf v = {0};
            buf_put(&v, '\0');
            v.n = 0;
            if (!expand(r, &v, s, n, 0))
                return false;
            d.value = v.s;
        }
        /* the first declaration of an attribute is the one that counts */
        if (find_att(r, d.elem, d.name)) {
            free(d.tokens);
            free(d.value);
            continue;
        }
        r->att = realloc(r->att, (size_t)(r->natt + 1) * sizeof *r->att);
        if (!r->att)
            abort();
        r->att[r->natt++] = d;
    }
}

/* <!ENTITY name "value"> (general, internal); parameter and external
 * entities are not supported and refuse the file */
static bool dtd_entity(Reader *r)
{
    EntDecl e;
    if (!skip_ws(r))
        return fail(r, "Malformed DTD: ENTITY.");
    if (r->p < r->end && *r->p == '%')
        return fail(r, "Unsupported DTD: parameter entity.");
    if (!read_name(r, e.name, sizeof e.name) || !skip_ws(r))
        return fail(r, "Malformed DTD: ENTITY.");
    const char *s;
    size_t n;
    if (r->p < r->end && *r->p != '"' && *r->p != '\'')
        return fail(r, "Unsupported DTD: external entity.");
    if (!quoted(r, &s, &n))
        return false;
    skip_ws(r);
    if (r->p >= r->end || *r->p != '>')
        return fail(r, "Malformed DTD: ENTITY.");
    r->p++;
    for (int i = 0; i < r->nent; i++)
        if (!strcmp(r->ent[i].name, e.name))
            return true; /* first declaration wins */
    e.value = xstrndup(s, n);
    r->ent = realloc(r->ent, (size_t)(r->nent + 1) * sizeof *r->ent);
    if (!r->ent)
        abort();
    r->ent[r->nent++] = e;
    return true;
}

static bool parse_doctype(Reader *r)
{
    r->p += 9; /* <!DOCTYPE */
    if (!skip_ws(r) || !read_name(r, r->doctype, sizeof r->doctype))
        return fail(r, "Malformed XML: DOCTYPE.");
    r->has_dtd = true;
    skip_ws(r);
    if (starts(r, "SYSTEM") || starts(r, "PUBLIC"))
        return fail(r, "Unsupported DTD: external DTD.");
    if (r->p < r->end && *r->p == '[') {
        r->p++;
        for (;;) {
            skip_ws(r);
            if (r->p >= r->end)
                return fail(r, "Malformed XML: unterminated DOCTYPE.");
            if (*r->p == ']') {
                r->p++;
                break;
            }
            bool skipped;
            if (!skip_misc(r, &skipped))
                return false;
            if (skipped)
                continue;
            if (starts(r, "<!ATTLIST")) {
                r->p += 9;
                if (!dtd_attlist(r))
                    return false;
            } else if (starts(r, "<!ENTITY")) {
                r->p += 8;
                if (!dtd_entity(r))
                    return false;
            } else if (starts(r, "<!ELEMENT")) {
                r->p += 9;
                char name[32];
                if (!skip_ws(r) || !read_name(r, name, sizeof name))
                    return fail(r, "Malformed DTD: ELEMENT.");
                if (!strcmp(name, "genetic_code"))
                    r->declared_gc = true;
                else if (!strcmp(name, "gene"))
                    r->declared_gene = true;
                if (!skip_past(r, ">"))
                    return false;
            } else {
                return fail(r, "Unsupported DTD declaration.");
            }
        }
        skip_ws(r);
    }
    if (r->p >= r->end || *r->p != '>')
        return fail(r, "Malformed XML: DOCTYPE.");
    r->p++;
    return true;
}

/* ---- attribute values ---- */

/* Appends s[0..n) with references replaced and white space made spaces
 * (XML 1.0 3.3.3); a character reference stays literal. */
static bool expand(Reader *r, Buf *out, const char *s, size_t n, int depth)
{
    if (depth > 8)
        return fail(r, "Malformed XML: entities nest too deep.");
    for (size_t i = 0; i < n; i++) {
        char c = s[i];
        if (c == '<')
            return fail(r, "Malformed XML: \"<\" in an attribute value.");
        if (c != '&') {
            buf_put(out, is_ws(c) ? ' ' : c);
            continue;
        }
        size_t j = i + 1;
        while (j < n && s[j] != ';')
            j++;
        if (j >= n)
            return fail(r, "Malformed XML: unterminated reference.");
        const char *ref = s + i + 1;
        size_t rn = j - i - 1;
        if (rn >= 2 && ref[0] == '#') {
            bool hex = ref[1] == 'x';
            unsigned long cp = 0;
            size_t k = hex ? 2 : 1;
            if (k >= rn)
                return fail(r, "Malformed XML: bad character reference.");
            for (; k < rn; k++) {
                char d = ref[k];
                int v = d >= '0' && d <= '9' ? d - '0'
                        : hex && d >= 'a' && d <= 'f' ? d - 'a' + 10
                        : hex && d >= 'A' && d <= 'F' ? d - 'A' + 10
                                                      : -1;
                if (v < 0 || cp > 0x10ffff)
                    return fail(r, "Malformed XML: bad character reference.");
                cp = cp * (hex ? 16 : 10) + (unsigned long)v;
            }
            if (cp == 0 || cp > 0x10ffff || (cp < 0x20 && cp != 9 && cp != 10 && cp != 13))
                return fail(r, "Malformed XML: bad character reference.");
            buf_utf8(out, cp);
        } else {
            static const struct {
                const char *name;
                char ch;
            } pre[] = {{"lt", '<'}, {"gt", '>'}, {"amp", '&'}, {"apos", '\''}, {"quot", '"'}};
            bool found = false;
            for (size_t k = 0; k < sizeof pre / sizeof pre[0]; k++) {
                if (strlen(pre[k].name) == rn && !memcmp(ref, pre[k].name, rn)) {
                    buf_put(out, pre[k].ch);
                    found = true;
                }
            }
            for (int k = 0; !found && k < r->nent; k++) {
                if (strlen(r->ent[k].name) == rn && !memcmp(ref, r->ent[k].name, rn)) {
                    if (!expand(r, out, r->ent[k].value, strlen(r->ent[k].value), depth + 1))
                        return false;
                    found = true;
                }
            }
            if (!found)
                return fail(r, "Malformed XML: undefined entity.");
        }
        i = j;
    }
    return true;
}

/* tokenised attributes: no leading or trailing spaces, single spaces */
static void collapse(char *s)
{
    char *w = s;
    for (char *p = s; *p; p++) {
        if (*p == ' ' && (w == s || w[-1] == ' '))
            continue;
        *w++ = *p;
    }
    if (w > s && w[-1] == ' ')
        w--;
    *w = '\0';
}

/* ---- elements ---- */

static void tag_free(Tag *t)
{
    for (int i = 0; i < t->nattr; i++)
        free(t->attr[i].val);
    free(t->attr);
    memset(t, 0, sizeof *t);
}

/* Reads a start tag at r->p ("<" already checked), its attributes
 * normalised and checked against the DTD, its defaults added. */
static bool read_tag(Reader *r, Tag *t)
{
    memset(t, 0, sizeof *t);
    r->p++;
    if (!read_name(r, t->name, sizeof t->name))
        return false;
    for (;;) {
        bool ws = skip_ws(r);
        if (r->p >= r->end)
            return fail(r, "Malformed XML: unterminated tag.");
        if (*r->p == '>') {
            r->p++;
            break;
        }
        if (starts(r, "/>")) {
            r->p += 2;
            t->empty = true;
            break;
        }
        if (!ws)
            return fail(r, "Malformed XML: attributes must be separated by white space.");
        Attr a = {.specified = true};
        if (!read_name(r, a.name, sizeof a.name))
            return false;
        skip_ws(r);
        if (r->p >= r->end || *r->p != '=')
            return fail(r, "Malformed XML: \"=\" expected.");
        r->p++;
        skip_ws(r);
        const char *s;
        size_t n;
        if (!quoted(r, &s, &n))
            return false;
        for (int i = 0; i < t->nattr; i++)
            if (!strcmp(t->attr[i].name, a.name))
                return fail(r, "Malformed XML: an attribute appears twice.");
        Buf v = {0};
        buf_put(&v, '\0');
        v.n = 0;
        if (!expand(r, &v, s, n, 0)) {
            free(v.s);
            return false;
        }
        a.val = v.s;
        t->attr = realloc(t->attr, (size_t)(t->nattr + 1) * sizeof *t->attr);
        if (!t->attr)
            abort();
        t->attr[t->nattr++] = a;
    }
    if (!r->has_dtd)
        return true;
    /* validity against the internal DTD */
    for (int i = 0; i < t->nattr; i++) {
        const AttDecl *d = find_att(r, t->name, t->attr[i].name);
        if (!d)
            return fail(r, "Invalid: an attribute is not declared in the DTD.");
        if (d->listed)
            collapse(t->attr[i].val);
        if (d->tokens) {
            char *probe = malloc(strlen(t->attr[i].val) + 3);
            if (!probe)
                abort();
            sprintf(probe, "|%s|", t->attr[i].val);
            bool ok = strstr(d->tokens, probe) != NULL;
            free(probe);
            if (!ok)
                return fail(r, "Invalid: an attribute value is not one the DTD allows.");
        }
        if (d->deflt == 3 && strcmp(t->attr[i].val, d->value) != 0)
            return fail(r, "Invalid: a #FIXED attribute has another value.");
    }
    for (int i = 0; i < r->natt; i++) {
        const AttDecl *d = &r->att[i];
        if (strcmp(d->elem, t->name) != 0)
            continue;
        bool present = false;
        for (int k = 0; k < t->nattr; k++)
            present |= !strcmp(t->attr[k].name, d->name);
        if (present)
            continue;
        if (d->deflt == 1)
            return fail(r, "Invalid: a required attribute is missing.");
        if (d->deflt >= 2) {
            Attr a = {.specified = false, .val = xstrndup(d->value, strlen(d->value))};
            snprintf(a.name, sizeof a.name, "%s", d->name);
            if (d->listed)
                collapse(a.val);
            t->attr = realloc(t->attr, (size_t)(t->nattr + 1) * sizeof *t->attr);
            if (!t->attr)
                abort();
            t->attr[t->nattr++] = a;
        }
    }
    return true;
}

static const char *attr(const Tag *t, const char *key)
{
    for (int i = 0; i < t->nattr; i++)
        if (!strcmp(t->attr[i].name, key))
            return t->attr[i].val;
    return "";
}

static bool end_tag(Reader *r, const char *name)
{
    r->p += 2;
    char n[32];
    if (!read_name(r, n, sizeof n))
        return false;
    skip_ws(r);
    if (r->p >= r->end || *r->p != '>' || strcmp(n, name) != 0)
        return fail(r, "Malformed XML: mismatched end tag.");
    r->p++;
    return true;
}

/* ---- Java's number parsing ---- */

static bool digits(const char **p, const char *e, bool hex)
{
    const char *s = *p;
    while (*p < e && ((**p >= '0' && **p <= '9') ||
                      (hex && ((**p >= 'a' && **p <= 'f') || (**p >= 'A' && **p <= 'F')))))
        (*p)++;
    return *p > s;
}

bool bgg_parse_java_double(const char *s, double *out)
{
    /* String.trim, then FloatingDecimal.readJavaFormatString */
    const char *b = s, *e = s + strlen(s);
    while (b < e && (unsigned char)*b <= ' ')
        b++;
    while (e > b && (unsigned char)e[-1] <= ' ')
        e--;
    const char *p = b;
    bool neg = false;
    if (p < e && (*p == '+' || *p == '-'))
        neg = *p++ == '-';
    if (e - p == 3 && !memcmp(p, "NaN", 3)) {
        uint64_t nan = 0x7ff8000000000000ULL; /* Double.NaN, whatever the sign */
        memcpy(out, &nan, sizeof *out);
        return true;
    }
    if (e - p == 8 && !memcmp(p, "Infinity", 8)) {
        *out = neg ? -HUGE_VAL : HUGE_VAL;
        return true;
    }
    bool hex = e - p >= 2 && p[0] == '0' && (p[1] == 'x' || p[1] == 'X');
    const char *q = hex ? p + 2 : p;
    bool lead = digits(&q, e, hex), frac = false;
    if (q < e && *q == '.') {
        q++;
        frac = digits(&q, e, hex);
    }
    if (!lead && !frac)
        return false;
    if (hex || (q < e && (*q == 'e' || *q == 'E'))) {
        if (q >= e || (hex ? (*q != 'p' && *q != 'P') : (*q != 'e' && *q != 'E')))
            return false; /* a hexadecimal number needs its p exponent */
        q++;
        if (q < e && (*q == '+' || *q == '-'))
            q++;
        if (!digits(&q, e, false))
            return false;
    }
    const char *num_end = q;
    if (q < e && (*q == 'f' || *q == 'F' || *q == 'd' || *q == 'D'))
        q++;
    if (q != e)
        return false;
    /* correctly rounded in both; the program never changes LC_NUMERIC */
    char *num = xstrndup(b, (size_t)(num_end - b));
    *out = strtod(num, NULL);
    free(num);
    return true;
}

/* Integer.parseInt on ASCII digits */
static bool java_parse_int(const char *s, long *out)
{
    const char *p = s;
    bool neg = false;
    if (*p == '+' || *p == '-')
        neg = *p++ == '-';
    if (!*p)
        return false;
    long v = 0;
    for (; *p; p++) {
        if (*p < '0' || *p > '9' || v > 100000000)
            return false;
        v = v * 10 + (*p - '0');
    }
    *out = neg ? -v : v;
    return true;
}

static int parse_color(const char *s)
{
    if (!strcmp(s, "green")) return COL_GREEN;
    if (!strcmp(s, "red")) return COL_RED;
    if (!strcmp(s, "blue")) return COL_BLUE;
    if (!strcmp(s, "cyan")) return COL_CYAN;
    if (!strcmp(s, "gray")) return COL_GRAY;
    if (!strcmp(s, "white")) return COL_WHITE;
    if (!strcmp(s, "yellow")) return COL_YELLOW;
    return -1;
}

/* ---- the document ---- */

/* BioXMLParser.parseGeneticCode / parseGene on a read tag */
static bool read_root_attrs(Reader *r, const Tag *t, long *sym, int *mirror, bool *disperse)
{
    if (!java_parse_int(attr(t, "symmetry"), sym) || *sym < 1 || *sym > 8)
        return fail(r, "Symmetry has not an allowed value.");
    const char *s = attr(t, "mirror");
    if (!strcmp(s, "yes"))
        *mirror = 1;
    else if (!strcmp(s, "no"))
        *mirror = 0;
    else
        return fail(r, "Mirror has not an allowed value.");
    s = attr(t, "disperse");
    if (!strcmp(s, "yes"))
        *disperse = true;
    else if (!strcmp(s, "no"))
        *disperse = false;
    else
        return fail(r, "Disperse has not an allowed value.");
    return true;
}

static bool read_gene(Reader *r, const Tag *t, Gene *g)
{
    if (!bgg_parse_java_double(attr(t, "length"), &g->length) ||
        !bgg_parse_java_double(attr(t, "theta"), &g->theta))
        return fail(r, "Attributes length and theta do not exist or have not an allowed value.");
    int col = parse_color(attr(t, "color"));
    if (col < 0)
        return fail(r, "Attribute color does not exist or has not an allowed value.");
    g->color = (uint8_t)col;
    return true;
}

static void reader_free(Reader *r)
{
    for (int i = 0; i < r->natt; i++) {
        free(r->att[i].tokens);
        free(r->att[i].value);
    }
    free(r->att);
    for (int i = 0; i < r->nent; i++)
        free(r->ent[i].value);
    free(r->ent);
}

static GeneticCode *parse_document(Reader *r)
{
    bool skipped;
    /* prolog */
    if (starts(r, "<?xml") && r->end - r->p > 5 && is_ws(r->p[5]))
        if (!skip_past(r, "?>"))
            return NULL;
    for (;;) {
        skip_ws(r);
        if (!skip_misc(r, &skipped))
            return NULL;
        if (skipped)
            continue;
        if (starts(r, "<!DOCTYPE")) {
            if (r->has_dtd)
                return fail(r, "Malformed XML: two DOCTYPEs."), NULL;
            if (!parse_doctype(r))
                return NULL;
            continue;
        }
        break;
    }
    if (r->p >= r->end || *r->p != '<')
        return fail(r, "This file does not contain a genetic_code."), NULL;
    if (!r->has_dtd) {
        /* read as if the file had the DOCTYPE 0.9 writes */
        const char *p = r->p, *end = r->end;
        r->p = k_doctype;
        r->end = k_doctype + sizeof k_doctype - 1;
        bool ok = parse_doctype(r);
        r->p = p;
        r->end = end;
        if (!ok)
            return NULL;
    }
    Tag t;
    if (!read_tag(r, &t)) {
        tag_free(&t);
        return NULL;
    }
    if (r->has_dtd && (strcmp(t.name, r->doctype) != 0 || !r->declared_gc)) {
        tag_free(&t);
        return fail(r, "Invalid: the root element does not match the DTD."), NULL;
    }
    if (strcmp(t.name, "genetic_code") != 0) {
        tag_free(&t);
        return fail(r, "This file does not contain a genetic_code."), NULL;
    }
    long sym;
    int mirror;
    bool disperse;
    bool ok = read_root_attrs(r, &t, &sym, &mirror, &disperse) && !t.empty;
    tag_free(&t);
    if (!ok)
        return fail(r, "A genetic code needs at least one gene."), NULL;

    int cap = 16, n = 0;
    Gene *genes = malloc((size_t)cap * sizeof *genes);
    if (!genes)
        abort();
    /* element content: genes, white space, comments, processing instructions */
    for (;;) {
        skip_ws(r);
        if (!skip_misc(r, &skipped))
            goto fail;
        if (skipped)
            continue;
        if (starts(r, "</")) {
            if (!end_tag(r, "genetic_code"))
                goto fail;
            break;
        }
        if (r->p >= r->end || *r->p != '<' || starts(r, "<!")) {
            fail(r, r->p >= r->end ? "Malformed XML: genetic_code is not closed."
                                   : "Invalid: text inside genetic_code.");
            goto fail;
        }
        if (!read_tag(r, &t)) {
            tag_free(&t);
            goto fail;
        }
        if (strcmp(t.name, "gene") != 0 || (r->has_dtd && !r->declared_gene)) {
            set_err(r->err, r->errn, "Parse error. %s found but gene expected.", t.name);
            r->failed = true;
            tag_free(&t);
            goto fail;
        }
        Gene g;
        ok = read_gene(r, &t, &g);
        bool empty = t.empty;
        tag_free(&t);
        if (!ok)
            goto fail;
        if (!empty && (!starts(r, "</") || !end_tag(r, "gene"))) {
            fail(r, "Invalid: gene must be empty.");
            goto fail;
        }
        if (n == cap) {
            cap *= 2;
            genes = realloc(genes, (size_t)cap * sizeof *genes);
            if (!genes)
                abort();
        }
        genes[n++] = g;
    }
    /* after the root: white space, comments, processing instructions */
    for (;;) {
        skip_ws(r);
        if (r->p >= r->end)
            break;
        if (!skip_misc(r, &skipped))
            goto fail;
        if (!skipped) {
            fail(r, "Malformed XML: content after genetic_code.");
            goto fail;
        }
    }
    if (n == 0) {
        fail(r, "A genetic code needs at least one gene.");
        goto fail;
    }
    GeneticCode *gc = gc_new_from_genes(genes, n, (int)sym, mirror, disperse);
    free(genes);
    return gc;
fail:
    free(genes);
    return NULL;
}

GeneticCode *bgg_parse(const char *xml, size_t len, char *err, size_t errn)
{
    size_t n;
    char *doc = prepare(xml, len, &n);
    Reader r = {.p = doc, .end = doc + n, .err = err, .errn = errn};
    GeneticCode *gc = parse_document(&r);
    reader_free(&r);
    free(doc);
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
