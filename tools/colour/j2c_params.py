# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""Writes src/colour/sim/params.{h,c} from Color Mod's Utils.java: the
parameters (Utils' static fields, defaulted from its DEF_ constants), the
colours, and the constant tables, so none of 150 names or defaults is
typed by hand."""

import os

from jparse import parse_file
from j2c_emit import HEADER

# Not simulation parameters: the window, the network, the Java2D options.
NOT_SAVED = {'WINDOW_X', 'WINDOW_Y', 'WINDOW_STATE', 'HARDWARE_ACCELERATION', 'LOCAL_PORT',
             'MAX_CONNECTIONS', 'ACCEPT_CONNECTIONS', 'CONNECT_TO_SERVER', 'SERVER_PORT'}

JAVA_COLORS = {
    'BLACK': (0, 0, 0), 'BLUE': (0, 0, 255), 'CYAN': (0, 255, 255), 'DARK_GRAY': (64, 64, 64),
    'GRAY': (128, 128, 128), 'GREEN': (0, 255, 0), 'LIGHT_GRAY': (192, 192, 192),
    'MAGENTA': (255, 0, 255), 'ORANGE': (255, 200, 0), 'PINK': (255, 175, 175),
    'RED': (255, 0, 0), 'WHITE': (255, 255, 255), 'YELLOW': (255, 255, 0),
}


def const_expr(n, defs):
    """A constant initialiser as C."""
    k = n.kind
    if k == 'Lit':
        t = n.text
        if n.type.base == 'double':
            t = t.rstrip('dD')
            if '.' not in t and 'e' not in t.lower():
                t += '.0'
        if n.type.base == 'String':
            return t
        return t
    if k == 'Name':
        return const_expr(defs[n.name], defs)
    if k == 'Paren':
        return '(' + const_expr(n.expr, defs) + ')'
    if k == 'Unary':
        return n.op + const_expr(n.expr, defs)
    if k == 'Binary':
        return f'{const_expr(n.l, defs)} {n.op} {const_expr(n.r, defs)}'
    if k == 'Field' and n.obj.kind == 'Name':
        q = f'{n.obj.name}.{n.name}'
        if q == 'Math.PI':
            return 'M_PI'
        if q == 'JFrame.NORMAL':
            return '0'
    raise ValueError(f'Utils.java:{n.line}: unsupported constant')


def color_rgb(n):
    if n.kind == 'New' and n.type.base == 'Color':
        return tuple(int(a.text) for a in n.args)
    if n.kind == 'Call' and n.name == 'darker' and n.obj.kind == 'Field':
        r, g, b = JAVA_COLORS[n.obj.name]
        return tuple(max(int(c * 0.7), 0) for c in (r, g, b))
    raise ValueError(f'Utils.java:{n.line}: unsupported colour')


def generate(src_dir, out_dir):
    utils = parse_file(os.path.join(src_dir, 'Utils.java'), skip_bodies=True)[0]
    fields = [(m, t, n, i) for m in utils.members if m.kind == 'Field' for t, n, i in m.decls]
    defs = {n: i for m, t, n, i in fields if 'final' in m.mods}
    params, colours, tables = [], [], []
    for m, t, n, init in fields:
        if 'static' not in m.mods:
            continue
        if n.startswith('Color'):
            colours.append((n[5:], color_rgb(init)))
        elif 'final' not in m.mods and n != 'random':
            if t.base == 'String':
                continue
            params.append((n, t.base, const_expr(init, defs)))
        elif n in ('side', 'scale', 'springscale'):
            tables.append((n, t, init))

    ctype = {'int': 'int', 'double': 'double', 'boolean': 'bool'}
    h = [HEADER.format(java='Utils.java'), '''/* The simulation parameters and constants of Color Mod's Utils.java.
 *
 * As in Java they are global: the world, organisms and genetic codes read
 * them directly, and the Parameters dialog edits them in place. Field
 * names are Utils' own in lower case (Utils.RED_PROB is P.red_prob).
 * Persisted as key=value text in $XDG_CONFIG_HOME/syn-biogenesis/
 * colour-params.cfg under Utils' preference keys (Java used Preferences).
 */
#ifndef COLOUR_PARAMS_H
#define COLOUR_PARAMS_H

#include <stdbool.h>

#include "sim/jrt.h"

typedef struct {''']
    for n, t, _ in params:
        h.append(f'    {ctype[t]} {n.lower()};')
    h.append('} Params;')
    h.append('''
extern Params P;
/* Utils.random: the one random stream every simulation decision draws from. */
extern JRandom g_rng;

#define BIO_TOL 0.0000001
#define U_FILE_VERSION 700
#define U_VERSION 900

extern const int k_side[8][2];
extern const double k_scale[16];
extern const double k_springscale[16];
''')
    h.append('/* Color Mod\'s colours (Utils.Color...) */')
    for n, (r, g, b) in colours:
        h.append(f'#define CM_{n} JRGB({r}, {g}, {b})')
    h.append('''
void params_set_defaults(Params *p);
/* Load/save the user's parameters. Missing file or keys keep defaults. */
void params_load(void);
bool params_save(void);
const char *params_config_dir(void);

/* Utils helpers that draw from g_rng */
int bio_random_sign(void);
bool bio_random_mutation(void);

/* Utils.between(int, int, int) */
static inline int bio_between_i(int value, int min, int max)
{
    return j_imax(j_imin(max, value), min);
}

/* Utils.between(double, double, double) */
static inline double bio_between(double value, double min, double max)
{
    return jmax(jmin(max, value), min);
}

#endif''')

    c = [HEADER.format(java='Utils.java'), '''#include "sim/params.h"

#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "compat.h"

Params P;
JRandom g_rng;
''']
    for n, t, init in tables:
        items = init.items
        if n == 'side':
            rows = ', '.join('{' + ', '.join(const_expr(x, defs) for x in row.items) + '}'
                             for row in items)
            c.append(f'const int k_side[8][2] = {{{rows}}};')
        else:
            vals = ', '.join(const_expr(x, defs) for x in items)
            c.append(f'const double k_{n}[16] = {{{vals}}};')
    c.append('\nvoid params_set_defaults(Params *p)\n{')
    for n, t, val in params:
        c.append(f'    p->{n.lower()} = {val};')
    c.append('}')
    c.append('''
int bio_random_sign(void)
{
    return (jr_next_int(&g_rng, 2) << 1) - 1;
}

bool bio_random_mutation(void)
{
    return jr_next_int(&g_rng, 10000) < P.meta_mutation_rate;
}

/* ---- persistence ---- */

typedef struct {
    const char *key;
    char type; /* 'i', 'd' or 'b' */
    size_t offset;
} ParamField;

static const ParamField k_fields[] = {''')
    for n, t, _ in params:
        if n in NOT_SAVED:
            continue
        c.append(f'    {{"{n}", \'{t[0]}\', offsetof(Params, {n.lower()})}},')
    c.append('''};

const char *params_config_dir(void)
{
    static char dir[1024];
    if (dir[0])
        return dir;
    const char *xdg = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");
#ifdef _WIN32
    /* Windows keeps an application's settings in the roaming AppData */
    const char *appdata = getenv("APPDATA");
    if (!(xdg && *xdg) && appdata && *appdata)
        xdg = appdata;
#endif
    if (xdg && *xdg)
        snprintf(dir, sizeof dir, "%s/syn-biogenesis", xdg);
    else if (home && *home)
        snprintf(dir, sizeof dir, "%s/.config/syn-biogenesis", home);
    else
        snprintf(dir, sizeof dir, ".");
    return path_slashes(dir);
}

static void params_path(char *out, size_t n)
{
    snprintf(out, n, "%s/colour-params.cfg", params_config_dir());
}

void params_load(void)
{
    char path[1100];
    params_path(path, sizeof path);
    FILE *f = fopen(path, "r");
    if (!f)
        return;
    char line[256];
    while (fgets(line, sizeof line, f)) {
        char *eq = strchr(line, '=');
        if (!eq || line[0] == '#')
            continue;
        *eq = '\\0';
        const char *val = eq + 1;
        for (size_t i = 0; i < sizeof k_fields / sizeof k_fields[0]; i++) {
            if (strcmp(line, k_fields[i].key) != 0)
                continue;
            char *base = (char *)&P + k_fields[i].offset;
            char *end;
            errno = 0;
            if (k_fields[i].type == 'i') {
                long v = strtol(val, &end, 10);
                if (end != val && !errno)
                    *(int *)base = (int)v;
            } else if (k_fields[i].type == 'b') {
                *(bool *)base = !strncmp(val, "true", 4);
            } else {
                double v = strtod(val, &end);
                if (end != val && !errno && isfinite(v))
                    *(double *)base = v;
            }
            break;
        }
    }
    fclose(f);
}

bool params_save(void)
{
    const char *dir = params_config_dir();
    char parent[1024];
    snprintf(parent, sizeof parent, "%s", dir);
    char *slash = strrchr(parent, '/');
    if (slash) {
        *slash = '\\0';
        mkdir(parent, 0755);
    }
    mkdir(dir, 0755);
    char path[1100];
    params_path(path, sizeof path);
    FILE *f = fopen(path, "w");
    if (!f)
        return false;
    fprintf(f, "# SYN-BIOGENESIS colour build parameters (Color Mod 2.01)\\n");
    for (size_t i = 0; i < sizeof k_fields / sizeof k_fields[0]; i++) {
        const char *base = (const char *)&P + k_fields[i].offset;
        if (k_fields[i].type == 'i')
            fprintf(f, "%s=%d\\n", k_fields[i].key, *(const int *)base);
        else if (k_fields[i].type == 'b')
            fprintf(f, "%s=%s\\n", k_fields[i].key, *(const bool *)base ? "true" : "false");
        else
            fprintf(f, "%s=%.17g\\n", k_fields[i].key, *(const double *)base);
    }
    return fclose(f) == 0;
}''')
    os.makedirs(out_dir, exist_ok=True)
    with open(os.path.join(out_dir, 'params.h'), 'w') as f:
        f.write('\n'.join(h) + '\n')
    with open(os.path.join(out_dir, 'params.c'), 'w') as f:
        f.write('\n'.join(c) + '\n')
    return params, colours
