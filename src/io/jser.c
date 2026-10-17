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

/* The Java Object Serialization Stream Protocol: see jser.h. */
#include "io/jser.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    TC_NULL = 0x70,
    TC_REFERENCE = 0x71,
    TC_CLASSDESC = 0x72,
    TC_OBJECT = 0x73,
    TC_STRING = 0x74,
    TC_ARRAY = 0x75,
    TC_BLOCKDATA = 0x77,
    TC_ENDBLOCKDATA = 0x78,
    TC_BLOCKDATALONG = 0x7a,
    TC_LONGSTRING = 0x7c,
    BASE_HANDLE = 0x7e0000,
    MAX_BLOCK = 1024, /* ObjectOutputStream's block data buffer */
};

static void *xcalloc(size_t n, size_t s)
{
    void *p = calloc(n ? n : 1, s ? s : 1);
    if (!p)
        abort();
    return p;
}

static void *xrealloc(void *p, size_t n)
{
    p = realloc(p, n ? n : 1);
    if (!p)
        abort();
    return p;
}

/* ---- the graph ---- */

static JObj *graph_add(JGraph *g, JObj *o)
{
    if (g->nobjs == g->cap) {
        g->cap = g->cap ? g->cap * 2 : 256;
        g->objs = xrealloc(g->objs, (size_t)g->cap * sizeof *g->objs);
    }
    o->handle = -1;
    g->objs[g->nobjs++] = o;
    return o;
}

/* the classes from the top serializable superclass down to c */
static int chain_of(const JClass *c, const JClass *chain[16])
{
    const JClass *rev[16];
    int n = 0;
    for (; c && n < 16; c = c->super)
        rev[n++] = c;
    for (int i = 0; i < n; i++)
        chain[i] = rev[n - 1 - i];
    return n;
}

static int total_fields(const JClass *c)
{
    int n = 0;
    for (; c; c = c->super)
        n += c->nfields;
    return n;
}

void jgraph_free(JGraph *g)
{
    for (int i = 0; i < g->nobjs; i++) {
        JObj *o = g->objs[i];
        if (o->ann) {
            const JClass *chain[16];
            int n = o->kind == JK_OBJECT ? chain_of(o->cls, chain) : 0;
            for (int k = 0; k < n; k++) {
                for (int a = 0; a < o->nann[k]; a++)
                    free(o->ann[k][a].bytes);
                free(o->ann[k]);
            }
            free(o->ann);
            free(o->nann);
        }
        free(o->vals);
        free(o->str);
        free(o);
    }
    free(g->objs);
    for (int i = 0; i < g->nclasses; i++) {
        JClass *c = g->classes[i];
        for (int f = 0; f < c->nfields; f++) {
            free((char *)c->fields[f].name);
            free((char *)c->fields[f].sig);
        }
        free((JField *)c->fields);
        free((char *)c->name);
        free(c);
    }
    free(g->classes);
    memset(g, 0, sizeof *g);
}

static JObj *new_obj(JGraph *g, JKind kind, const JClass *cls, int nvals)
{
    JObj *o = xcalloc(1, sizeof *o);
    o->kind = kind;
    o->cls = cls;
    o->nvals = nvals;
    o->vals = nvals ? xcalloc((size_t)nvals, sizeof *o->vals) : NULL;
    if (kind == JK_OBJECT) {
        const JClass *chain[16];
        int n = chain_of(cls, chain);
        o->ann = xcalloc((size_t)n, sizeof *o->ann);
        o->nann = xcalloc((size_t)n, sizeof *o->nann);
    }
    return graph_add(g, o);
}

JObj *jobj_new(JGraph *g, const JClass *cls)
{
    return new_obj(g, JK_OBJECT, cls, total_fields(cls));
}

JObj *jarray_new(JGraph *g, const JClass *cls, int n)
{
    return new_obj(g, JK_ARRAY, cls, n);
}

/* index of field `name` in o->vals, or -1 */
static int field_index(const JObj *o, const char *name, char type)
{
    const JClass *chain[16];
    int n = chain_of(o->cls, chain), base = 0;
    for (int k = 0; k < n; k++) {
        for (int f = 0; f < chain[k]->nfields; f++) {
            const JField *fd = &chain[k]->fields[f];
            if (!strcmp(fd->name, name)) {
                bool obj = fd->type == 'L' || fd->type == '[';
                bool want_obj = type == 'L' || type == '[';
                return (type == 0 || fd->type == type || (obj && want_obj)) ? base + f : -1;
            }
        }
        base += chain[k]->nfields;
    }
    return -1;
}

JVal *jobj_field(const JObj *o, const char *name, char type)
{
    if (!o || o->kind != JK_OBJECT)
        return NULL;
    int i = field_index(o, name, type);
    return i < 0 ? NULL : &o->vals[i];
}

bool jobj_is(const JObj *o, const char *class_name)
{
    return o && o->kind != JK_STRING && !strcmp(o->cls->name, class_name);
}

const JAnn *jobj_ann(const JObj *o, const char *class_name, int *n)
{
    *n = 0;
    if (!o || o->kind != JK_OBJECT)
        return NULL;
    const JClass *chain[16];
    int k = chain_of(o->cls, chain);
    for (int i = 0; i < k; i++) {
        if (!strcmp(chain[i]->name, class_name)) {
            *n = o->nann[i];
            return o->ann[i];
        }
    }
    return NULL;
}

void jobj_set(JObj *o, const char *name, JVal v)
{
    int i = field_index(o, name, 0);
    if (i < 0) {
        fprintf(stderr, "jser: %s has no field %s\n", o->cls->name, name);
        abort();
    }
    o->vals[i] = v;
}

static JAnn *ann_slot(JObj *o, const JClass *cls)
{
    const JClass *chain[16];
    int n = chain_of(o->cls, chain);
    for (int k = 0; k < n; k++) {
        if (chain[k] == cls) {
            o->ann[k] = xrealloc(o->ann[k], (size_t)(o->nann[k] + 1) * sizeof *o->ann[k]);
            JAnn *a = &o->ann[k][o->nann[k]++];
            memset(a, 0, sizeof *a);
            return a;
        }
    }
    abort();
}

void jobj_add_bytes(JObj *o, const JClass *cls, const void *bytes, size_t n)
{
    JAnn *a = ann_slot(o, cls);
    a->bytes = xcalloc(n, 1);
    memcpy(a->bytes, bytes, n);
    a->n = n;
}

void jobj_add_object(JObj *o, const JClass *cls, JObj *obj)
{
    JAnn *a = ann_slot(o, cls);
    a->is_object = true;
    a->obj = obj;
}

/* ---- reading ---- */

typedef struct {
    const uint8_t *p, *end;
    JGraph *g;
    void **h; /* handle -> JClass* or JObj* */
    bool *h_is_class;
    int nh, hcap;
    char *err;
    size_t errn;
    bool failed;
    int depth;
} Rd;

static bool rfail(Rd *r, const char *fmt, ...)
{
    if (!r->failed && r->err && r->errn) {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(r->err, r->errn, fmt, ap);
        va_end(ap);
    }
    r->failed = true;
    r->p = r->end;
    return false;
}

static bool need(Rd *r, size_t n)
{
    if ((size_t)(r->end - r->p) < n)
        return rfail(r, "The file is truncated.");
    return true;
}

static uint64_t rbe(Rd *r, int n)
{
    if (!need(r, (size_t)n))
        return 0;
    uint64_t v = 0;
    for (int i = 0; i < n; i++)
        v = v << 8 | *r->p++;
    return v;
}

static int add_handle(Rd *r, void *x, bool is_class)
{
    if (r->nh == r->hcap) {
        r->hcap = r->hcap ? r->hcap * 2 : 1024;
        r->h = xrealloc(r->h, (size_t)r->hcap * sizeof *r->h);
        r->h_is_class = xrealloc(r->h_is_class, (size_t)r->hcap * sizeof *r->h_is_class);
    }
    r->h[r->nh] = x;
    r->h_is_class[r->nh] = is_class;
    return r->nh++;
}

static char *read_utf(Rd *r, uint64_t n)
{
    if (!need(r, (size_t)n))
        return NULL;
    char *s = xcalloc((size_t)n + 1, 1);
    memcpy(s, r->p, (size_t)n);
    r->p += n;
    return s;
}

static JObj *read_content(Rd *r);
static const JClass *read_classdesc(Rd *r);

static void *read_reference(Rd *r, bool want_class)
{
    int64_t h = (int32_t)rbe(r, 4) - BASE_HANDLE;
    if (r->failed)
        return NULL;
    if (h < 0 || h >= r->nh || r->h_is_class[h] != want_class)
        return rfail(r, "The file refers to an object that isn't there."), NULL;
    return r->h[h];
}

/* skips a class annotation (contents up to TC_ENDBLOCKDATA) */
static void skip_annotation(Rd *r)
{
    while (!r->failed) {
        if (!need(r, 1))
            return;
        uint8_t tc = *r->p;
        if (tc == TC_ENDBLOCKDATA) {
            r->p++;
            return;
        }
        if (tc == TC_BLOCKDATA || tc == TC_BLOCKDATALONG) {
            r->p++;
            uint64_t n = rbe(r, tc == TC_BLOCKDATA ? 1 : 4);
            if (need(r, (size_t)n))
                r->p += n;
            continue;
        }
        read_content(r);
    }
}

static const JClass *read_classdesc_body(Rd *r)
{
    uint64_t n = rbe(r, 2);
    char *name = read_utf(r, n);
    if (!name)
        return NULL;
    JClass *c = xcalloc(1, sizeof *c);
    c->name = name;
    JGraph *g = r->g;
    if (g->nclasses == g->ccap) {
        g->ccap = g->ccap ? g->ccap * 2 : 32;
        g->classes = xrealloc(g->classes, (size_t)g->ccap * sizeof *g->classes);
    }
    g->classes[g->nclasses++] = c;
    c->uid = (int64_t)rbe(r, 8);
    add_handle(r, c, true);
    c->flags = (uint8_t)rbe(r, 1);
    int nfields = (int)rbe(r, 2);
    if (r->failed)
        return NULL;
    if (c->flags & 0x04)
        return rfail(r, "Externalizable class %s is not supported.", name), NULL;
    /* the count only with the (zeroed) fields, so freeing never sees one
     * without the other */
    JField *fs = xcalloc((size_t)nfields, sizeof *fs);
    c->fields = fs;
    c->nfields = nfields;
    for (int i = 0; i < c->nfields && !r->failed; i++) {
        fs[i].type = (char)rbe(r, 1);
        fs[i].name = read_utf(r, rbe(r, 2));
        if (!strchr("BCDFIJSZL[", fs[i].type) || !fs[i].type)
            return rfail(r, "Bad field type in class %s.", name), NULL;
        if (fs[i].type == 'L' || fs[i].type == '[') {
            JObj *s = read_content(r);
            if (r->failed)
                return NULL;
            if (!s || s->kind != JK_STRING)
                return rfail(r, "Bad field signature in class %s.", name), NULL;
            fs[i].sig = strdup(s->str);
        }
    }
    skip_annotation(r);
    c->super = read_classdesc(r);
    return r->failed ? NULL : c;
}

static const JClass *read_classdesc(Rd *r)
{
    uint8_t tc = (uint8_t)rbe(r, 1);
    if (r->failed)
        return NULL;
    if (tc == TC_NULL)
        return NULL;
    if (tc == TC_REFERENCE)
        return read_reference(r, true);
    if (tc == TC_CLASSDESC)
        return read_classdesc_body(r);
    return rfail(r, "Unsupported class descriptor (0x%02x).", tc), NULL;
}

static void read_value(Rd *r, char type, JVal *v)
{
    switch (type) {
    case 'B': v->i = (int8_t)rbe(r, 1); break;
    case 'C': v->i = (uint16_t)rbe(r, 2); break;
    case 'S': v->i = (int16_t)rbe(r, 2); break;
    case 'I': v->i = (int32_t)rbe(r, 4); break;
    case 'J': v->i = (int64_t)rbe(r, 8); break;
    case 'Z': v->i = rbe(r, 1) != 0; break;
    case 'F': {
        uint32_t b = (uint32_t)rbe(r, 4);
        memcpy(&v->f, &b, 4);
        break;
    }
    case 'D': {
        uint64_t b = rbe(r, 8);
        memcpy(&v->d, &b, 8);
        break;
    }
    default:
        v->o = read_content(r);
    }
}

static JObj *read_object(Rd *r)
{
    const JClass *cls = read_classdesc(r);
    if (r->failed)
        return NULL;
    if (!cls || !(cls->flags & JSC_SERIALIZABLE))
        return rfail(r, "An object of a class that isn't serializable."), NULL;
    JObj *o = jobj_new(r->g, cls);
    add_handle(r, o, false);
    const JClass *chain[16];
    int n = chain_of(cls, chain), base = 0;
    for (int k = 0; k < n && !r->failed; k++) {
        for (int f = 0; f < chain[k]->nfields; f++)
            read_value(r, chain[k]->fields[f].type, &o->vals[base + f]);
        base += chain[k]->nfields;
        if (!(chain[k]->flags & JSC_WRITE_METHOD))
            continue;
        /* custom data up to TC_ENDBLOCKDATA */
        while (!r->failed && need(r, 1)) {
            uint8_t tc = *r->p;
            if (tc == TC_ENDBLOCKDATA) {
                r->p++;
                break;
            }
            if (tc == TC_BLOCKDATA || tc == TC_BLOCKDATALONG) {
                r->p++;
                uint64_t len = rbe(r, tc == TC_BLOCKDATA ? 1 : 4);
                if (!need(r, (size_t)len))
                    break;
                jobj_add_bytes(o, chain[k], r->p, (size_t)len);
                r->p += len;
            } else {
                JObj *x = read_content(r);
                jobj_add_object(o, chain[k], x);
            }
        }
    }
    return o;
}

static JObj *read_array(Rd *r)
{
    const JClass *cls = read_classdesc(r);
    if (r->failed)
        return NULL;
    if (!cls || cls->name[0] != '[' || !cls->name[1])
        return rfail(r, "Bad array class."), NULL;
    int32_t n = (int32_t)rbe(r, 4);
    if (r->failed || n < 0 || n > (r->end - r->p))
        return rfail(r, "Bad array length."), NULL;
    JObj *a = jarray_new(r->g, cls, n);
    add_handle(r, a, false);
    char t = cls->name[1];
    if (!strchr("BCDFIJSZL[", t))
        return rfail(r, "Bad array type %s.", cls->name), NULL;
    for (int i = 0; i < n && !r->failed; i++)
        read_value(r, t, &a->vals[i]);
    return a;
}

static JObj *read_content(Rd *r)
{
    if (++r->depth > 2000) {
        r->depth--;
        return rfail(r, "The file nests objects too deeply."), NULL;
    }
    JObj *o = NULL;
    uint8_t tc = (uint8_t)rbe(r, 1);
    if (!r->failed) {
        switch (tc) {
        case TC_NULL:
            break;
        case TC_REFERENCE:
            o = read_reference(r, false);
            break;
        case TC_OBJECT:
            o = read_object(r);
            break;
        case TC_ARRAY:
            o = read_array(r);
            break;
        case TC_STRING:
        case TC_LONGSTRING: {
            uint64_t n = rbe(r, tc == TC_STRING ? 2 : 8);
            char *s = read_utf(r, n);
            if (!s)
                break;
            o = new_obj(r->g, JK_STRING, NULL, 0);
            o->str = s;
            add_handle(r, o, false);
            break;
        }
        default:
            rfail(r, "Unsupported content in the stream (0x%02x).", tc);
        }
    }
    r->depth--;
    return o;
}

JObj *jser_read(JGraph *g, const uint8_t *data, size_t len, char *err, size_t errn)
{
    Rd r = {.p = data, .end = data + len, .g = g, .err = err, .errn = errn};
    if (len < 4 || data[0] != 0xac || data[1] != 0xed || data[2] != 0x00 || data[3] != 0x05) {
        rfail(&r, "This is not a Java serialization stream.");
        return NULL;
    }
    r.p += 4;
    JObj *root = read_content(&r);
    free(r.h);
    free(r.h_is_class);
    if (r.failed)
        return NULL;
    if (!root) {
        snprintf(err, errn, "The file holds no object.");
        return NULL;
    }
    return root;
}

/* ---- writing ---- */

typedef struct {
    uint8_t *b;
    size_t n, cap;
    int next;
    const JClass **cls; /* class -> handle */
    int *cls_h;
    int ncls, cls_cap;
    const char **str;   /* signature string -> handle (they are interned in Java) */
    int *str_h;
    int nstr, str_cap;
} Wr;

static void put(Wr *w, const void *p, size_t n)
{
    if (w->n + n > w->cap) {
        while (w->n + n > w->cap)
            w->cap = w->cap ? w->cap * 2 : 65536;
        w->b = xrealloc(w->b, w->cap);
    }
    memcpy(w->b + w->n, p, n);
    w->n += n;
}

static void put_be(Wr *w, uint64_t v, int n)
{
    uint8_t b[8];
    for (int i = n - 1; i >= 0; i--) {
        b[i] = (uint8_t)v;
        v >>= 8;
    }
    put(w, b, (size_t)n);
}

static void put_utf(Wr *w, const char *s)
{
    size_t n = strlen(s);
    put_be(w, n, 2);
    put(w, s, n);
}

static void put_value(Wr *w, char type, const JVal *v);
static void put_object(Wr *w, JObj *o);

static void put_typestring(Wr *w, const char *s)
{
    for (int i = 0; i < w->nstr; i++) {
        if (!strcmp(w->str[i], s)) {
            put_be(w, TC_REFERENCE, 1);
            put_be(w, (uint64_t)(BASE_HANDLE + w->str_h[i]), 4);
            return;
        }
    }
    if (w->nstr == w->str_cap) {
        w->str_cap = w->str_cap ? w->str_cap * 2 : 64;
        w->str = xrealloc(w->str, (size_t)w->str_cap * sizeof *w->str);
        w->str_h = xrealloc(w->str_h, (size_t)w->str_cap * sizeof *w->str_h);
    }
    w->str[w->nstr] = s;
    w->str_h[w->nstr++] = w->next++;
    put_be(w, TC_STRING, 1);
    put_utf(w, s);
}

static void put_classdesc(Wr *w, const JClass *c)
{
    if (!c) {
        put_be(w, TC_NULL, 1);
        return;
    }
    for (int i = 0; i < w->ncls; i++) {
        if (w->cls[i] == c) {
            put_be(w, TC_REFERENCE, 1);
            put_be(w, (uint64_t)(BASE_HANDLE + w->cls_h[i]), 4);
            return;
        }
    }
    if (w->ncls == w->cls_cap) {
        w->cls_cap = w->cls_cap ? w->cls_cap * 2 : 32;
        w->cls = xrealloc(w->cls, (size_t)w->cls_cap * sizeof *w->cls);
        w->cls_h = xrealloc(w->cls_h, (size_t)w->cls_cap * sizeof *w->cls_h);
    }
    w->cls[w->ncls] = c;
    w->cls_h[w->ncls++] = w->next++;
    put_be(w, TC_CLASSDESC, 1);
    put_utf(w, c->name);
    put_be(w, (uint64_t)c->uid, 8);
    put_be(w, c->flags, 1);
    put_be(w, (uint64_t)c->nfields, 2);
    for (int f = 0; f < c->nfields; f++) {
        put_be(w, (uint8_t)c->fields[f].type, 1);
        put_utf(w, c->fields[f].name);
        if (c->fields[f].type == 'L' || c->fields[f].type == '[')
            put_typestring(w, c->fields[f].sig);
    }
    put_be(w, TC_ENDBLOCKDATA, 1);
    put_classdesc(w, c->super);
}

static void put_value(Wr *w, char type, const JVal *v)
{
    switch (type) {
    case 'B':
    case 'Z': put_be(w, (uint64_t)v->i, 1); break;
    case 'C':
    case 'S': put_be(w, (uint64_t)v->i, 2); break;
    case 'I': put_be(w, (uint64_t)v->i, 4); break;
    case 'J': put_be(w, (uint64_t)v->i, 8); break;
    case 'F': {
        uint32_t b;
        memcpy(&b, &v->f, 4);
        put_be(w, b, 4);
        break;
    }
    case 'D': {
        uint64_t b;
        memcpy(&b, &v->d, 8);
        put_be(w, b, 8);
        break;
    }
    default:
        put_object(w, v->o);
    }
}

/* block data as ObjectOutputStream buffers it: at most MAX_BLOCK bytes a block */
static void put_block(Wr *w, const uint8_t *p, size_t n)
{
    while (n > 0) {
        size_t k = n < MAX_BLOCK ? n : MAX_BLOCK;
        if (k <= 0xff) {
            put_be(w, TC_BLOCKDATA, 1);
            put_be(w, k, 1);
        } else {
            put_be(w, TC_BLOCKDATALONG, 1);
            put_be(w, k, 4);
        }
        put(w, p, k);
        p += k;
        n -= k;
    }
}

static void put_object(Wr *w, JObj *o)
{
    if (!o) {
        put_be(w, TC_NULL, 1);
        return;
    }
    if (o->handle >= 0) {
        put_be(w, TC_REFERENCE, 1);
        put_be(w, (uint64_t)(BASE_HANDLE + o->handle), 4);
        return;
    }
    if (o->kind == JK_STRING) {
        put_be(w, TC_STRING, 1);
        o->handle = w->next++;
        put_utf(w, o->str);
        return;
    }
    if (o->kind == JK_ARRAY) {
        put_be(w, TC_ARRAY, 1);
        put_classdesc(w, o->cls);
        o->handle = w->next++;
        put_be(w, (uint64_t)o->nvals, 4);
        for (int i = 0; i < o->nvals; i++)
            put_value(w, o->cls->name[1], &o->vals[i]);
        return;
    }
    put_be(w, TC_OBJECT, 1);
    put_classdesc(w, o->cls);
    o->handle = w->next++;
    const JClass *chain[16];
    int n = chain_of(o->cls, chain), base = 0;
    for (int k = 0; k < n; k++) {
        for (int f = 0; f < chain[k]->nfields; f++)
            put_value(w, chain[k]->fields[f].type, &o->vals[base + f]);
        base += chain[k]->nfields;
        if (!(chain[k]->flags & JSC_WRITE_METHOD))
            continue;
        /* consecutive raw writes share a block until an object is written */
        uint8_t *pend = NULL;
        size_t np = 0;
        for (int a = 0; a < o->nann[k]; a++) {
            const JAnn *x = &o->ann[k][a];
            if (!x->is_object) {
                pend = xrealloc(pend, np + x->n);
                memcpy(pend + np, x->bytes, x->n);
                np += x->n;
                continue;
            }
            put_block(w, pend, np);
            np = 0;
            put_object(w, x->obj);
        }
        put_block(w, pend, np);
        free(pend);
        put_be(w, TC_ENDBLOCKDATA, 1);
    }
}

uint8_t *jser_write(JObj *root, size_t *len)
{
    Wr w = {0};
    put_be(&w, 0xaced, 2);
    put_be(&w, 5, 2);
    put_object(&w, root);
    free(w.cls);
    free(w.cls_h);
    free(w.str);
    free(w.str_h);
    *len = w.n;
    return w.b;
}
