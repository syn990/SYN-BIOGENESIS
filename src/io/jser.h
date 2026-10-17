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

/* Java Object Serialization (the stream ObjectOutputStream writes and
 * ObjectInputStream reads), as an object graph: enough of the protocol for
 * Biogenesis's .bgw worlds and the JDK classes inside them. Reading accepts
 * any stream of serializable classes (no proxies, enums or externalizable
 * data); writing reproduces ObjectOutputStream's bytes for the graph given:
 * the same handles, back-references, field order and block data. */
#ifndef IO_JSER_H
#define IO_JSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    JSC_WRITE_METHOD = 0x01, /* the class has writeObject: custom data follows its fields */
    JSC_SERIALIZABLE = 0x02,
};

typedef struct {
    char type;        /* B C D F I J S Z, L (object) or [ (array) */
    const char *name;
    const char *sig;  /* L and [ only: "Ljava/util/List;", "[I", ... */
} JField;

typedef struct JClass {
    const char *name; /* "biogenesis.World", "[I", ... */
    int64_t uid;
    uint8_t flags;
    int nfields;      /* in stream order: primitives, then objects, each by name */
    const JField *fields;
    const struct JClass *super; /* serializable superclass, or NULL */
} JClass;

typedef struct JObj JObj;

typedef union {
    int64_t i; /* B C I J S Z */
    float f;
    double d;
    JObj *o;   /* L [ */
} JVal;

/* Data a writeObject method writes after the default fields: raw bytes
 * (written as block data) and objects, in order. */
typedef struct {
    bool is_object;
    JObj *obj;
    uint8_t *bytes;
    size_t n;
} JAnn;

typedef enum { JK_OBJECT, JK_ARRAY, JK_STRING } JKind;

struct JObj {
    JKind kind;
    const JClass *cls;  /* object and array */
    int nvals;
    JVal *vals;         /* object: every field, top superclass first; array: elements */
    JAnn **ann;         /* object: per class in the hierarchy (top first), custom data */
    int *nann;
    char *str;          /* string */
    int handle;         /* writing */
};

/* All objects and classes of one graph, freed together. */
typedef struct {
    JObj **objs;
    int nobjs, cap;
    JClass **classes; /* read from a stream (owned) */
    int nclasses, ccap;
} JGraph;

void jgraph_free(JGraph *g);

/* ---- reading ---- */

/* The stream's first object, or NULL with a reason in err. */
JObj *jser_read(JGraph *g, const uint8_t *data, size_t len, char *err, size_t errn);

/* The field `name` of an object, searching its class and superclasses, or
 * NULL if it has no such field (or the field has another type). */
JVal *jobj_field(const JObj *o, const char *name, char type);
bool jobj_is(const JObj *o, const char *class_name);
/* Custom data of the class `class_name` in o's hierarchy. */
const JAnn *jobj_ann(const JObj *o, const char *class_name, int *n);

/* ---- writing ---- */

JObj *jobj_new(JGraph *g, const JClass *cls);           /* fields zeroed */
JObj *jarray_new(JGraph *g, const JClass *cls, int n);  /* elements zeroed */
/* Sets a field by name; aborts if the class has none (a programming error). */
void jobj_set(JObj *o, const char *name, JVal v);
void jobj_add_bytes(JObj *o, const JClass *cls, const void *bytes, size_t n);
void jobj_add_object(JObj *o, const JClass *cls, JObj *obj);

/* Serializes root as ObjectOutputStream.writeObject would; malloc'd bytes. */
uint8_t *jser_write(JObj *root, size_t *len);

#endif
