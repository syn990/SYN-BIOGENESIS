# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""What j2c.py knows besides the translated classes: how their names map
to C, and the C that stands in for everything else they use (World,
Utils, java.lang.Math, java.awt geometry, java.util.Random, Color).

The C on the other side is hand-written: src/colour/sim/world.c,
params.c (generated from Utils.java by j2c_params.py) and jrt.h.
"""

import re

from jparse import Type

# ---------------------------------------------------------------------------
# Translated classes

CLASSES = {
    'Organism': dict(
        prefix='org_', self='o', extends='Rectangle',
        # drawing is the window's business (src/ui)
        skip={'draw', 'getImage'},
        rename={
            # the names the 0.9 port uses, so the window code reads alike
            '_geneticCode': 'gc', '_infectedGeneticCode': 'infected_gc',
            '_savedGeneticCode': 'saved_gc', '_ID': 'id', '_parentID': 'parent_id',
            '_nTotalChildren': 'total_children', '_nTotalKills': 'total_kills',
            '_nTotalInfected': 'total_infected', '_startPointX': 'start_x',
            '_startPointY': 'start_y', '_endPointX': 'end_x', '_endPointY': 'end_y',
            '_dCenterX': 'dcenter_x', '_dCenterY': 'dcenter_y', '_I': 'inertia',
            '_color': 'flash_color', 'hasGrown': 'has_grown', 'hasMoved': 'has_moved',
            'lastFrame': 'last_frame', '_framesColor': 'frames_color',
        },
        method_names={
            '<init>/1': 'org_new', '<init>/2': 'org_new_with_code',
        },
        extra_fields=[
            '/* world bookkeeping, not part of the Java organism */',
            'bool removed;  /* out of energy this frame; freed when the next one starts */',
            'double env[4]; /* frame-start envelope: minx, maxx, miny, maxy */',
        ],
    ),
    'GeneticCode': dict(
        prefix='gc_', self='gc', skip={'draw'},
        method_names={
            '<init>/0': 'gc_new_random', '<init>/1': 'gc_new_mutated',
            '<init>/24': 'gc_new_from_genes',
        },
        collected=True,
    ),
    'Gene': dict(
        prefix='gene_', self='g',
        method_names={'<init>/0': 'gene_new', '<init>/40': 'gene_new_full'},
        collected=True,
    ),
}

# Classes the translated code refers to but that are written by hand (or are
# Java's).
EXTERNAL = {'Utils', 'Math', 'Color', 'Integer', 'World', 'Rectangle', 'Line2D',
            'Line2D.Double', 'Point2D', 'Point2D.Double', 'ExLine2DDouble', 'Vector2D',
            'String', 'Random', 'List', 'Object', 'VisibleWorld', 'Point',
            'CloneNotSupportedException', 'OutCorridor'}

SUPER = {'Organism': 'Rectangle', 'ExLine2DDouble': 'Line2D.Double',
         'Line2D.Double': 'Line2D', 'Point2D.Double': 'Point2D'}

VALUE_TYPES = {'Rectangle': 'JRect', 'Line2D': 'JLine', 'Line2D.Double': 'JLine',
               'ExLine2DDouble': 'JLine', 'Point2D': 'JPoint', 'Point2D.Double': 'JPoint',
               'Vector2D': 'Vector2D', 'Point': 'JIPoint'}

PRIM_C = {'int': 'int', 'long': 'int64_t', 'double': 'double', 'float': 'float',
          'boolean': 'bool', 'char': 'char', 'void': 'void', 'byte': 'int8_t',
          'short': 'int16_t'}

REF_C = {'String': 'char *', 'Color': 'JColor', 'Organism': 'Organism *',
         'GeneticCode': 'GeneticCode *', 'Gene': 'Gene *', 'World': 'World *',
         'VisibleWorld': 'void *', 'Object': 'void *', 'List': 'GeneList *',
         'OutCorridor': 'OutCorridor *'}


def is_class(name):
    return name in EXTERNAL or name in CLASSES


def is_subclass(a, b):
    while a:
        if a == b:
            return True
        a = SUPER.get(a)
    return False


def ctype(t):
    if t.dims:
        return ctype(t.elem()).rstrip() + ' *' if not ctype(t.elem()).endswith('*') \
            else ctype(t.elem()) + '*'
    if t.base in PRIM_C:
        return PRIM_C[t.base]
    if t.base in VALUE_TYPES:
        return VALUE_TYPES[t.base]
    if t.base in REF_C:
        return REF_C[t.base]
    raise ValueError(f'no C type for {t}')


def decl(t, name):
    c = ctype(t)
    return f'{c}{name}' if c.endswith('*') else f'{c} {name}'


# ---------------------------------------------------------------------------
# Fields of the translated classes

RECT_FIELDS = {'x', 'y', 'width', 'height'}


def member(cls, f):
    return f.cname


def const_name(cls, f):
    """A static final constant with a literal value."""
    if cls == 'Organism':
        return 'TY_' + f.name if f.name != 'serialVersionUID' else 'FILE_VERSION'
    return f'{cls.upper()}_{f.name.upper()}'


def static_name(cls, f):
    return f'{CLASSES[cls]["prefix"]}{f.cname}'


# ---------------------------------------------------------------------------
# Utils: the parameters (P.x, mutable), the constants and the colours

UTILS = {}   # name -> (c, type, mutable)


def load_utils(fields):
    """fields: [(name, Type, static, final)] from Utils.java."""
    for name, typ, static, final in fields:
        if not static:
            continue
        if name.startswith('Color'):
            UTILS[name] = (f'CM_{name[5:]}', Type('Color'), False)
        elif name == 'random':
            UTILS[name] = ('g_rng', Type('Random'), True)
        elif final:
            if name in ('tol',):
                UTILS[name] = ('BIO_TOL', typ, False)
            elif name in ('side', 'scale', 'springscale'):
                UTILS[name] = ('k_' + name, typ, False)
            else:
                UTILS[name] = (f'U_{name}', typ, False)
        else:
            UTILS[name] = (f'P.{name.lower()}', typ, True)


JCOLORS = {'BLACK', 'BLUE', 'CYAN', 'DARK_GRAY', 'GRAY', 'GREEN', 'LIGHT_GRAY', 'MAGENTA',
           'ORANGE', 'PINK', 'RED', 'WHITE', 'YELLOW'}


def static_field(cls, name):
    if cls == 'Utils':
        return UTILS.get(name)
    if cls == 'Color' and name in JCOLORS:
        return (f'JC_{name}', Type('Color'), False)
    if cls == 'Math' and name == 'PI':
        return ('M_PI', Type('double'), False)
    if cls == 'Integer' and name in ('MAX_VALUE', 'MIN_VALUE'):
        return (f'INT32_{name[:3]}', Type('int'), False)
    return None


# Fully qualified constants. Double.MIN_VALUE is the smallest positive
# double, not the most negative one (Biogenesis uses it as if it were).
QUALIFIED = {
    'java.lang.Double.MAX_VALUE': ('DBL_MAX', 'double'),
    'java.lang.Double.MIN_VALUE': ('JDOUBLE_MIN_VALUE', 'double'),
    'Double.MAX_VALUE': ('DBL_MAX', 'double'),
    'Double.MIN_VALUE': ('JDOUBLE_MIN_VALUE', 'double'),
}

WORLD_FIELDS = {'_corridorexists': ('corridor_exists', 'boolean'),
                '_visibleWorld': ('visible_world', 'VisibleWorld')}


def rect_of(obj):
    """C address of the java.awt.Rectangle part of obj (an E)."""
    if obj.type.base == 'Organism':
        return f'&{paren16(obj)}->r'
    return f'&{paren16(obj)}' if obj.c.isidentifier() or '->' in obj.c else f'&({obj.c})'


def paren16(obj):
    return obj.c if obj.prec >= 16 else f'({obj.c})'


def instance_field(cls, name, obj):
    if cls == 'Organism' and name in RECT_FIELDS:
        return (f'{paren16(obj)}->r.{name}', 'int')
    if cls == 'Rectangle' and name in RECT_FIELDS:
        return (f'{paren16(obj)}.{name}', 'int')
    if cls in ('Line2D', 'Line2D.Double', 'ExLine2DDouble') and name in ('x1', 'y1', 'x2', 'y2'):
        return (f'{paren16(obj)}.{name}', 'double')
    if cls == 'Point' and name in ('x', 'y'):
        return (f'{paren16(obj)}.{name}', 'int')
    if cls in ('Point2D', 'Point2D.Double') and name in ('x', 'y'):
        return (f'{paren16(obj)}.{name}', 'double')
    if cls == 'World' and name in WORLD_FIELDS:
        c, t = WORLD_FIELDS[name]
        return (f'{paren16(obj)}->{c}', t)
    return None


# ---------------------------------------------------------------------------
# Methods of everything that isn't translated.
# Each: (param types, return type, C template or function, pure)
# A template's {0}.. are the arguments, {o} the object, {po} the object
# parenthesised if needed.


def _rect_m(fn):
    return lambda obj, a, args: fn(rect_of(obj), a)


WORLD_METHODS = {
    # name: ([params], ret, effects) where effects is True (none), 'world'
    # (only the world's own state: atmosphere, population, statistics,
    # ids) or False (anything: checkHit runs collisions)
    'photosynthesis': (['double'], 'double', 'world'),
    'methanotrophy': (['double'], 'double', 'world'),
    'respiration': (['double'], 'double', 'world'),
    'decomposition': (['double'], 'double', 'world'),
    'getCO2': ([], 'double', True),
    'getO2': ([], 'double', True),
    'getCH4': ([], 'double', True),
    'addO2': (['double'], 'void', 'world'),
    'addCO2': (['double'], 'void', 'world'),
    'addCH4': (['double'], 'void', 'world'),
    'decreaseO2': (['double'], 'void', 'world'),
    'decreaseCO2': (['double'], 'void', 'world'),
    'decreaseCH4': (['double'], 'void', 'world'),
    'decreasePopulation': ([], 'void', 'world'),
    'increasePopulation': ([], 'void', 'world'),
    'organismHasDied': (['Organism', 'Organism'], 'void', 'world'),
    'organismHasBeenInfected': (['Organism', 'Organism'], 'void', 'world'),
    'addOrganism': (['Organism', 'Organism'], 'void', 'world'),
    'fastCheckHit': (['Organism'], 'Organism', True),
    'genesisCheckHit': (['Organism'], 'Organism', True),
    'transformCheckHit': (['Organism'], 'Organism', True),
    'checkHit': (['Organism'], 'Organism', False),
    'checkHitCorridor': (['Organism'], 'OutCorridor', True),
    'getNewId': ([], 'int', 'world'),
    'getTime': ([], 'long', True),
    'getFrame': ([], 'long', True),
    'getWidth': ([], 'int', True),
    'getHeight': ([], 'int', True),
}

# World methods that read only the world's own state
WORLD_ONLY_READS = {'getCO2', 'getO2', 'getCH4', 'getTime', 'getFrame', 'getWidth',
                    'getHeight', 'getNewId', 'photosynthesis', 'methanotrophy', 'respiration',
                    'decomposition', 'addO2', 'addCO2', 'addCH4', 'decreaseO2', 'decreaseCO2',
                    'decreaseCH4', 'decreasePopulation', 'increasePopulation'}


def lib_reads(cls, name):
    if cls == 'World':
        return 2 if name in WORLD_ONLY_READS else 3
    return 1

LIB = {
    ('Math', 'sqrt'): [(['double'], 'double', 'sqrt({0})', True)],
    ('Math', 'pow'): [(['double', 'double'], 'double', 'bio_pow({0}, {1})', True)],
    ('Math', 'sin'): [(['double'], 'double', 'bio_sin({0})', True)],
    ('Math', 'cos'): [(['double'], 'double', 'bio_cos({0})', True)],
    ('Math', 'atan2'): [(['double', 'double'], 'double', 'bio_atan2({0}, {1})', True)],
    ('Math', 'abs'): [(['int'], 'int', 'j_iabs({0})', True),
                      (['long'], 'long', 'j_labs({0})', True),
                      (['double'], 'double', 'fabs({0})', True)],
    ('Math', 'min'): [(['int', 'int'], 'int', 'j_imin({0}, {1})', True),
                      (['long', 'long'], 'long', 'j_lmin({0}, {1})', True),
                      (['double', 'double'], 'double', 'jmin({0}, {1})', True)],
    ('Math', 'max'): [(['int', 'int'], 'int', 'j_imax({0}, {1})', True),
                      (['long', 'long'], 'long', 'j_lmax({0}, {1})', True),
                      (['double', 'double'], 'double', 'jmax({0}, {1})', True)],
    ('Math', 'round'): [(['double'], 'long', 'j_lround({0})', True)],
    ('Utils', 'between'): [(['int', 'int', 'int'], 'int', 'bio_between_i({0}, {1}, {2})', True),
                           (['double', 'double', 'double'], 'double',
                            'bio_between({0}, {1}, {2})', True)],
    ('Utils', 'min'): [(['int', 'int', 'int'], 'int', 'j_imin(j_imin({0}, {1}), {2})', True),
                       (['double', 'double', 'double'], 'double',
                        'jmin(jmin({0}, {1}), {2})', True)],
    ('Utils', 'max'): [(['int', 'int', 'int'], 'int', 'j_imax(j_imax({0}, {1}), {2})', True),
                       (['double', 'double', 'double'], 'double',
                        'jmax(jmax({0}, {1}), {2})', True)],
    ('Utils', 'randomSign'): [([], 'int', 'bio_random_sign()', 'random')],
    ('Utils', 'randomMutation'): [([], 'boolean', 'bio_random_mutation()', 'random')],
    ('Random', 'nextInt'): [(['int'], 'int', 'jr_next_int(&{o}, {0})', 'random')],
    ('Random', 'nextDouble'): [([], 'double', 'jr_next_double(&{o})', 'random')],
    ('Random', 'nextBoolean'): [([], 'boolean', 'jr_next_boolean(&{o})', 'random')],
    ('Color', 'equals'): [(['Object'], 'boolean', '{po} == {0}', True)],
    ('Integer', 'toString'): [(['int'], 'String', 'jstr_from_int({0})', True)],
    ('Rectangle', 'intersects'): [(['Rectangle'], 'boolean', lambda o, a, args:
                                   f'jrect_intersects({rect_of(o)}, {rect_of(args[0])})', True)],
    ('Rectangle', 'intersectsLine'): [(['Line2D'], 'boolean', lambda o, a, args:
                                       f'jrect_intersects_line({rect_of(o)}, &{a[0]})', True)],
    ('Rectangle', 'contains'): [(['int', 'int'], 'boolean', lambda o, a, args:
                                 f'jrect_contains({rect_of(o)}, {a[0]}, {a[1]})', True)],
    ('Rectangle', 'setBounds'): [
        (['int', 'int', 'int', 'int'], 'void', lambda o, a, args:
         f'jrect_set({rect_of(o)}, {a[0]}, {a[1]}, {a[2]}, {a[3]})', False),
        (['Rectangle'], 'void', lambda o, a, args:
         f'*{rect_of(o)} = *{rect_of(args[0])}', False)],
    ('Line2D', 'setLine'): [(['double', 'double', 'double', 'double'], 'void',
                             'jline_set(&{o}, {0}, {1}, {2}, {3})', False)],
    ('Line2D', 'intersectsLine'): [(['Line2D'], 'boolean',
                                    'jline_intersects_line(&{o}, &{0})', True)],
    ('Line2D', 'getP1'): [([], 'Point2D.Double', '(JPoint){{{po}.x1, {po}.y1}}', True)],
    ('Line2D', 'getP2'): [([], 'Point2D.Double', '(JPoint){{{po}.x2, {po}.y2}}', True)],
    ('Line2D', 'getX1'): [([], 'double', '{po}.x1', True)],
    ('Line2D', 'getY1'): [([], 'double', '{po}.y1', True)],
    ('Line2D', 'getX2'): [([], 'double', '{po}.x2', True)],
    ('Line2D', 'getY2'): [([], 'double', '{po}.y2', True)],
    ('ExLine2DDouble', 'getIntersection'): [(['Line2D.Double'], 'Point2D.Double',
                                             'jline_get_intersection(&{o}, &{0})', True)],
    ('Point2D', 'distanceSq'): [(['Point2D'], 'double', 'jpoint_distance_sq({o}, {0})', True)],
    ('Vector2D', 'getX'): [([], 'double', 'vec_get_x(&{o})', True)],
    ('Vector2D', 'getY'): [([], 'double', 'vec_get_y(&{o})', True)],
    ('Vector2D', 'getModulus'): [([], 'double', '{po}.modulus', True)],
    ('Vector2D', 'getTheta'): [([], 'double', '{po}.theta', True)],
    ('Vector2D', 'setModulus'): [(['double'], 'void', '{po}.modulus = {0}', False)],
    ('Vector2D', 'setTheta'): [(['double'], 'void', '{po}.theta = {0}', False)],
    ('Vector2D', 'setPolar'): [(['double', 'double'], 'void',
                                'vec_set_polar(&{o}, {0}, {1})', False)],
    ('Vector2D', 'addDegree'): [(['double'], 'void', '{po}.theta += {0}', False)],
    ('Vector2D', 'invertX'): [([], 'void', 'vec_invert_x(&{o})', False)],
    ('Vector2D', 'invertY'): [([], 'void', 'vec_invert_y(&{o})', False)],
    # network corridors are not ported: World.checkHitCorridor finds none
    ('OutCorridor', 'canSendOrganism'): [([], 'boolean', 'out_corridor_can_send({o})', True)],
    ('OutCorridor', 'sendOrganism'): [(['Organism'], 'boolean',
                                       'out_corridor_send({o}, {0})', False)],
    ('List', 'size'): [([], 'int', '{po}->n', True)],
    ('List', 'get'): [(['int'], 'Gene', '{po}->items[{0}]', True)],
    ('List', 'toArray'): [(['Gene[]'], 'Gene[]', 'gene_list_to_array({o}, {0})', False)],
}


RANDOM_NAMES = {'nextInt', 'nextDouble', 'nextBoolean', 'randomSign', 'randomMutation'}


def effect_bits(pure):
    """A LIB/World entry's purity flag as j2c's effect mask: True is none,
    'random' draws from Utils.random only, False changes state."""
    if pure is True:
        return 0
    if pure == 'random':
        return 1
    if pure == 'world':
        return 4
    return 2


def name_effects(name):
    """Union of what any library or World method of this name does."""
    m = 0
    for (c, n), cands in LIB.items():
        if n == name:
            for cand in cands:
                m |= effect_bits(cand[3])
    if name in WORLD_METHODS:
        m |= effect_bits(WORLD_METHODS[name][2])
    return m


def lib_methods(cls, name, static):
    if cls == 'World':
        if name not in WORLD_METHODS:
            return []
        params, ret, pure = WORLD_METHODS[name]
        c = 'world_' + re.sub(r'([a-z0-9])([A-Z])', r'\1_\2', name).lower()
        c = c.replace('c_o2', 'co2').replace('o_2', 'o2').replace('c_h4', 'ch4')

        def tmpl(obj, a, args, c=c):
            return f'{c}({", ".join([obj.c] + a)})'
        return [(params, ret, tmpl, pure)]
    c = cls
    while c:
        if (c, name) in LIB:
            return LIB[(c, name)]
        c = SUPER.get(c)
    return []


def lib_new(base, argtypes, cargs):
    if base in ('Rectangle',) and not cargs:
        return '(JRect){0}'
    if base in ('ExLine2DDouble', 'Line2D.Double') and not cargs:
        return '(JLine){0}'
    if base == 'Point2D.Double' and len(cargs) == 2:
        return f'(JPoint){{{cargs[0]}, {cargs[1]}}}'
    if base == 'Point' and len(cargs) == 2:
        return f'(JIPoint){{{cargs[0]}, {cargs[1]}}}'
    if base == 'Vector2D' and not cargs:
        return '(Vector2D){0}'
    return None
