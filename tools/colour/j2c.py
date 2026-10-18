# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""Translates Biogenesis Color Mod's simulation classes from Java to C.

Color Mod's Organism.java alone is over 19,000 lines, most of it the
colour-against-colour rules of collisions. Ported by hand, a slip in one of
thousands of near-identical branches would go unseen; translated by a
program, every branch gets the same treatment and a mistake shows up
everywhere at once, where tools/javaref's frame-by-frame comparison
catches it.

The translation keeps Java's meaning, not just its look:

- types are resolved, overloads chosen by Java's rules (Utils.between(int..)
  against Utils.between(double..), Math.min on ints or doubles);
- narrowing casts and compound assignments use Java's conversions (j_d2i:
  NaN to 0, saturating), Math.min/max Java's NaN and -0.0 rules, % on
  doubles fmod;
- Java evaluates operands left to right and C doesn't promise to, so any
  expression where the order could matter (two calls with side effects,
  or a call and a read of what it may change) is split into statements
  that run in Java's order, and one that can't be split is an error;
- anything not covered is an error, not a guess.

Usage: j2c.py JAVA_SRC_DIR OUT_DIR
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from jparse import JavaError, Node, Type, parse_file  # noqa: E402

import j2c_spec as SPEC  # noqa: E402


def T(s):
    """Type from a string like 'int', 'Gene[]', 'List<Gene>'."""
    m = re.fullmatch(r'([\w.]+)(?:<(.+)>)?((?:\[\])*)', s.replace(' ', ''))
    if not m:
        raise ValueError(s)
    args = [T(a) for a in m.group(2).split(',')] if m.group(2) else ()
    return Type(m.group(1), args, m.group(3).count('[]'))


INT, LONG, DOUBLE, FLOAT, BOOL, CHAR, VOID = (T(x) for x in
                                              ('int', 'long', 'double', 'float', 'boolean',
                                               'char', 'void'))
STRING, NULL = T('String'), T('null')
NUMERIC_RANK = {'byte': 0, 'short': 1, 'char': 1, 'int': 2, 'long': 3, 'float': 4, 'double': 5}


def is_num(t):
    return t.dims == 0 and t.base in NUMERIC_RANK


def is_integral(t):
    return is_num(t) and t.base not in ('float', 'double')


def is_ref(t):
    return t.dims > 0 or t.base not in NUMERIC_RANK and t.base not in ('boolean', 'void')


def promote(a, b):
    """Binary numeric promotion."""
    ra, rb = NUMERIC_RANK[a.base], NUMERIC_RANK[b.base]
    r = max(ra, rb, 2)
    return {2: INT, 3: LONG, 4: FLOAT, 5: DOUBLE}[r]


def widens(src, dst):
    """Cost of a method-invocation conversion from src to dst, or None."""
    if src == dst:
        return 0
    if src == NULL:
        return 1 if is_ref(dst) else None
    if is_num(src) and is_num(dst):
        if src.base == 'char' and dst.base in ('byte', 'short'):
            return None
        if NUMERIC_RANK[dst.base] > NUMERIC_RANK[src.base] or (src.base in ('byte', 'short')
                                                                 and dst.base == 'int'):
            return 1 + NUMERIC_RANK[dst.base] - NUMERIC_RANK[src.base]
        return None
    if dst.base == 'Object' and dst.dims == 0 and is_ref(src):
        return 10
    if is_ref(src) and is_ref(dst) and src.dims == dst.dims:
        if SPEC.is_subclass(src.base, dst.base):
            return 2
    return None


def snake(name):
    """Java field/method name to C: drop the leading underscore, camelCase to
    snake_case."""
    n = name.lstrip('_')
    n = re.sub(r'([a-z0-9])([A-Z])', r'\1_\2', n)
    n = re.sub(r'([A-Z]+)([A-Z][a-z])', r'\1_\2', n)
    return n.lower()


class TranslateError(Exception):
    pass


# ---------------------------------------------------------------------------
# The program model: the translated classes, plus the hand-written C ones
# (World, Utils, ...) described by j2c_spec.


class Field:
    def __init__(self, cls, name, typ, static, final, init, node):
        self.cls, self.name, self.type = cls, name, typ
        self.static, self.final, self.init, self.node = static, final, init, node
        self.cname = None


class Method:
    def __init__(self, cls, name, params, ret, static, node):
        self.cls, self.name, self.params, self.ret = cls, name, params, ret
        self.static, self.node = static, node
        self.cname = None
        self.effects = RANDOM | WRITE  # what calling it does, from j2c_emit's analysis


class Class:
    def __init__(self, name, node, conf):
        self.name, self.node, self.conf = name, node, conf
        self.fields = {}
        self.methods = {}
        self.ctors = []
        self.super = conf.get('extends')


def build_class(node, conf):
    c = Class(node.name, node, conf)
    skip = set(conf.get('skip', ()))
    for m in node.members:
        if m.kind == 'Field':
            for typ, name, init in m.decls:
                f = Field(c, name, typ, 'static' in m.mods, 'final' in m.mods, init, m)
                f.cname = conf.get('rename', {}).get(name) or snake(name)
                c.fields[name] = f
        elif m.kind == 'Method':
            if m.name in skip:
                continue
            meth = Method(c, m.name, m.params, m.ret, 'static' in m.mods, m)
            c.methods.setdefault(m.name, []).append(meth)
        elif m.kind == 'Ctor':
            meth = Method(c, '<init>', m.params, T(c.name), False, m)
            c.ctors.append(meth)
        elif m.kind == 'Init':
            raise TranslateError(f'{c.name}: initializer blocks are not supported')
        elif m.kind == 'Class':
            raise TranslateError(f'{c.name}: nested classes are not supported')
    # C names for methods: overloads get the parameter count, or the names
    # given in the configuration
    prefix = conf['prefix']
    names = conf.get('method_names', {})
    for name, ms in c.methods.items():
        for m in ms:
            key = f'{name}/{len(m.params)}'
            if key in names:
                m.cname = names[key]
            elif name in names:
                m.cname = names[name]
            elif len(ms) > 1:
                # overloads: named by their parameter types, the one
                # without parameters by its plain name
                same_arity = [o for o in ms if len(o.params) == len(m.params)]
                if not m.params:
                    m.cname = prefix + snake(name)
                elif len(same_arity) > 1 or any(not o.params for o in ms):
                    m.cname = prefix + snake(name) + ''.join(
                        '_' + snake(t.base.split('.')[-1]) + ('s' if t.dims else '')
                        for t, _ in m.params)
                else:
                    m.cname = f'{prefix}{snake(name)}{len(m.params)}'
            else:
                m.cname = prefix + snake(name)
    for i, m in enumerate(c.ctors):
        key = f'<init>/{len(m.params)}'
        m.cname = names.get(key) or f'{prefix}new{len(m.params)}'
    return c


# ---------------------------------------------------------------------------
# Expressions


RANDOM, WRITE, WORLD = 1, 2, 4
R_OBJ, R_WORLD = 1, 2


class E:
    """A translated expression: C text, Java type, C precedence, and what it
    does: `effects`, a mask of RANDOM (it draws from Utils.random), WORLD
    (it changes the world's own state: atmosphere, population, statistics,
    ids) and WRITE (it assigns, or calls something that changes any state),
    and `reads`, a mask of R_OBJ (fields, array elements, globals) and
    R_WORLD (the world's state). Locals read and written are found from the
    syntax tree (Translator.locals_of)."""

    __slots__ = ('c', 'type', 'prec', 'effects', 'reads', 'lvalue', 'node')

    def __init__(self, c, typ, prec=16, effects=0, reads=0, lvalue=False):
        self.c, self.type, self.prec = c, typ, prec
        self.effects, self.reads, self.lvalue = int(effects), int(reads), lvalue
        self.node = None


# C precedence levels (higher binds tighter); Java's agree for these operators
PREC = {'||': 4, '&&': 5, '|': 6, '^': 7, '&': 8, '==': 9, '!=': 9, '<': 10, '>': 10, '<=': 10,
        '>=': 10, '<<': 11, '>>': 11, '+': 12, '-': 12, '*': 13, '/': 13, '%': 13}


def readsof(es):
    m = 0
    for e in es:
        if e is not None:
            m |= e.reads
    return m


def union(es):
    m = 0
    for e in es:
        if e is not None:
            m |= e.effects
    return m


def paren(e, prec):
    return f'({e.c})' if e.prec < prec else e.c


class Scope:
    def __init__(self, parent=None):
        self.vars = {}
        self.parent = parent

    def find(self, name):
        s = self
        while s:
            if name in s.vars:
                return s.vars[name]
            s = s.parent
        return None


class Local:
    def __init__(self, name, typ, cname):
        self.name, self.type, self.cname = name, typ, cname


class Translator:
    def __init__(self, classes):
        self.classes = classes  # name -> Class
        self.tmp_n = 0
        self.problems = []

    # ---- lookups ----
    def cls_of(self, t):
        if t.dims:
            return None
        return self.classes.get(t.base)

    def field(self, cname, fname):
        """A field of a translated class or of its superclasses."""
        c = self.classes.get(cname)
        while c:
            if fname in c.fields:
                return c.fields[fname]
            c = self.classes.get(c.super) if c.super else None
        return None

    def methods(self, cname, mname):
        out = []
        c = self.classes.get(cname)
        while c:
            out += c.methods.get(mname, [])
            c = self.classes.get(c.super) if c.super else None
        return out

    def err(self, node, msg):
        raise TranslateError(f'{self.cls.name}.java:{node.line}: {msg}')

    # ---- method resolution ----
    def pick(self, node, cands, args, what):
        """Java's most-specific-method choice among candidates whose
        parameter types the arguments convert to."""
        best = None
        for cand in cands:
            ptypes = cand[0]
            if len(ptypes) != len(args):
                continue
            costs = [widens(a.type, p) for a, p in zip(args, ptypes)]
            if any(c is None for c in costs):
                continue
            score = sum(costs)
            if best is None or score < best[0]:
                best = (score, cand)
            elif score == best[0]:
                self.err(node, f'ambiguous call to {what}')
        if best is None:
            sig = ', '.join(str(a.type) for a in args)
            self.err(node, f'no {what}({sig})')
        return best[1]

    # ---- expressions ----
    def ex(self, n):
        f = getattr(self, 'ex_' + n.kind, None)
        if f is None:
            self.err(n, f'unsupported expression {n.kind}')
        e = f(n)
        if e.node is None:
            e.node = n
        return e

    def locals_of(self, n):
        """(locals read, locals written) by expression n."""
        key = id(n)
        if key in self.loc_memo:
            return self.loc_memo[key]
        rd, wr = set(), set()

        def walk(x):
            if isinstance(x, (list, tuple)):
                for y in x:
                    walk(y)
                return
            if not isinstance(x, Node):
                return
            if x.kind == 'Name' and self.scope.find(x.name):
                rd.add(x.name)
            if x.kind in ('Assign', 'PreInc', 'PostInc'):
                t = x.target if x.kind == 'Assign' else x.expr
                if t.kind == 'Name' and self.scope.find(t.name):
                    wr.add(t.name)
            for v in x.__dict__.values():
                if isinstance(v, (Node, list, tuple)):
                    walk(v)
        walk(n)
        self.loc_memo[key] = (rd, wr)
        return rd, wr

    def ex_Paren(self, n):
        e = self.ex(n.expr)
        return E(f'({e.c})', e.type, 16, e.effects, e.reads)

    def ex_Lit(self, n):
        t, s = n.type, n.text
        if t.base == 'boolean':
            return E(s, BOOL)
        if t.base == 'null':
            return E('NULL', NULL)
        if t.base == 'char':
            body = s[1:-1]
            if body.startswith('\\u'):
                return E(str(int(body[2:], 16)), CHAR)
            return E(s, CHAR)
        if t.base == 'String':
            return E(s, STRING)
        low = s.lower()
        if t.base == 'int':
            return E(s, INT)
        if t.base == 'long':
            return E(s[:-1] + 'LL', LONG)
        if t.base == 'float':
            return E(s, FLOAT)
        # double: drop d/D, make sure C reads it as a double
        v = s[:-1] if low.endswith('d') else s
        if not re.search(r'[.eE]', v):
            v += '.0'
        if v.startswith('.'):
            v = '0' + v
        if v.endswith('.'):
            v += '0'
        return E(v, DOUBLE)

    def ex_This(self, n):
        return E(self.self_name, T(self.cls.name))

    def ex_Name(self, n):
        loc = self.scope.find(n.name)
        if loc:
            return E(loc.cname, loc.type, lvalue=True)
        f = self.field(self.cls.name, n.name)
        if f:
            return self.field_access(n, None, f)
        if self.cls.super:
            r = SPEC.instance_field(self.cls.name, n.name, E(self.self_name, T(self.cls.name)))
            if r is not None:
                return E(r[0], T(r[1]), reads=True, lvalue=True)
        if SPEC.is_class(n.name) or n.name in self.classes:
            return E(None, Type('#class:' + n.name))
        self.err(n, f'unknown name {n.name}')

    def field_access(self, n, obj, f):
        """obj is an E (or None for this); f a Field of a translated class."""
        if f.static:
            if f.final and f.init is not None and (f.init.kind == 'Lit' or (
                    f.init.kind == 'Unary' and f.init.expr.kind == 'Lit')):
                return E(SPEC.const_name(f.cls.name, f), f.type)
            return E(SPEC.static_name(f.cls.name, f), f.type, reads=not f.final, lvalue=True)
        base = self.self_name if obj is None else paren(obj, 16)
        acc = SPEC.member(f.cls.name, f)
        return E(f'{base}->{acc}', f.type, reads=True, lvalue=True,
                 effects=obj.effects if obj else 0)

    def ex_Field(self, n):
        dotted = self.dotted(n)
        if dotted and dotted in SPEC.QUALIFIED:
            c, t = SPEC.QUALIFIED[dotted]
            return E(c, T(t))
        if n.obj.kind == 'Name' and not self.scope.find(n.obj.name) and \
                not self.field(self.cls.name, n.obj.name):
            cname = n.obj.name
            if cname in self.classes:
                f = self.field(cname, n.name)
                if not f or not f.static:
                    self.err(n, f'no static field {cname}.{n.name}')
                return self.field_access(n, None, f)
            r = SPEC.static_field(cname, n.name)
            if r is None:
                self.err(n, f'unknown {cname}.{n.name}')
            c, t, mutable = r
            return E(c, T(t) if isinstance(t, str) else t, reads=mutable, lvalue=mutable)
        obj = self.ex(n.obj)
        if obj.type.dims and n.name == 'length':
            return E(f'JLEN({obj.c})', INT, reads=True, effects=obj.effects)
        if obj.type.base.startswith('#class:'):
            cname = obj.type.base[7:]
            r = SPEC.static_field(cname, n.name)
            if r is None:
                self.err(n, f'unknown {cname}.{n.name}')
            c, t, mutable = r
            return E(c, T(t) if isinstance(t, str) else t, reads=mutable, lvalue=mutable)
        f = self.field(obj.type.base, n.name) if obj.type.dims == 0 else None
        if f:
            return self.field_access(n, obj, f)
        r = SPEC.instance_field(obj.type.base, n.name, obj)
        if r is None:
            self.err(n, f'unknown field {obj.type}.{n.name}')
        c, t = r
        return E(c, T(t) if isinstance(t, str) else t, reads=True, lvalue=True,
                 effects=obj.effects)

    @staticmethod
    def dotted(n):
        parts = []
        while n.kind == 'Field':
            parts.append(n.name)
            n = n.obj
        if n.kind != 'Name':
            return None
        parts.append(n.name)
        return '.'.join(reversed(parts))

    def ex_Index(self, n):
        a = self.ex(n.arr)
        i = self.ex(n.idx)
        if not a.type.dims:
            self.err(n, 'indexing a non-array')
        return E(f'{paren(a, 16)}[{i.c}]', a.type.elem(), 16, a.effects | i.effects,
                 True, lvalue=True)

    def ex_Call(self, n):
        args = [self.ex(a) for a in n.args]
        # this.m(...) / m(...)
        if n.obj is None or n.obj.kind == 'This':
            ms = self.methods(self.cls.name, n.name)
            if not ms and self.cls.super:
                # a method of the Java superclass (Organism extends Rectangle)
                me = E(self.self_name, T(self.cls.name))
                return self.lib_call(n, self.cls.super, me, args, static=False)
            if not ms:
                self.err(n, f'unknown method {n.name}')
            m = self.pick(n, [([p[0] for p in mm.params], mm) for mm in ms], args, n.name)[1]
            return self.call_method(n, m, None, args)
        # Class.m(...)
        if n.obj.kind == 'Name' and not self.scope.find(n.obj.name) and \
                not self.field(self.cls.name, n.obj.name):
            cname = n.obj.name
            if cname in self.classes:
                ms = [mm for mm in self.methods(cname, n.name) if mm.static]
                if not ms:
                    self.err(n, f'no static method {cname}.{n.name}')
                m = self.pick(n, [([p[0] for p in mm.params], mm) for mm in ms], args,
                              n.name)[1]
                return self.call_method(n, m, None, args)
            return self.lib_call(n, cname, None, args, static=True)
        if n.obj.kind == 'Super':
            if n.name == 'clone' and not args:
                # Object.clone: a field-by-field copy (see shallow_clone in
                # the generated file)
                return E(f'{self.cls.conf["prefix"]}shallow_clone({self.self_name})',
                         T('Object'), 16, 0, R_OBJ)
            self.err(n, 'super calls are not supported')
        obj = self.ex(n.obj)
        if obj.type.base.startswith('#class:'):
            return self.lib_call(n, obj.type.base[7:], None, args, static=True)
        if obj.type.dims == 0 and obj.type.base in self.classes:
            ms = self.methods(obj.type.base, n.name)
            if ms:
                m = self.pick(n, [([p[0] for p in mm.params], mm) for mm in ms], args,
                              n.name)[1]
                return self.call_method(n, m, obj, args)
        return self.lib_call(n, obj.type.base if obj.type.dims == 0 else str(obj.type), obj,
                             args, static=False)

    def call_method(self, n, m, obj, args):
        cargs = []
        if not m.static:
            cargs.append(self.self_name if obj is None else obj.c)
        for a, (pt, _) in zip(args, m.params):
            cargs.append(self.convert(a, pt).c)
        effects = m.effects | union(args) | (obj.effects if obj is not None else 0)
        e = E(f'{m.cname}({", ".join(cargs)})', m.ret, 16, effects, R_OBJ | R_WORLD)
        if self.hoist is not None:
            self.call_args_order(n, ([obj] if obj else []) + args)
        return e

    def lib_call(self, n, cname, obj, args, static):
        cands = SPEC.lib_methods(cname, n.name, static)
        if not cands:
            self.err(n, f'unknown method {cname}.{n.name}')
        ptypes, ret, tmpl, pure = self.pick(n, [([T(p) for p in c[0]],) + tuple(c[1:])
                                                for c in cands], args, f'{cname}.{n.name}')
        cargs = [self.convert(a, p).c for a, p in zip(args, ptypes)]
        if callable(tmpl):
            c = tmpl(obj, cargs, args)
        else:
            c = tmpl.format(*cargs, o=obj.c if obj else None,
                            po=(paren(obj, 16) if obj else None))
        effects = SPEC.effect_bits(pure) | union(args) | (obj.effects if obj is not None
                                                          else 0)
        reads = SPEC.lib_reads(cname, n.name) | readsof(args) | (obj.reads if obj is not None
                                                                 else 0)
        e = E(c, T(ret) if isinstance(ret, str) else ret, 16, effects, reads)
        if self.hoist is not None:
            self.call_args_order(n, ([obj] if obj else []) + args)
        return e

    def convert(self, e, t):
        """An argument or assigned value converted to type t (widening only,
        which C does implicitly; this only checks)."""
        if widens(e.type, t) is None and not (e.type == t):
            raise TranslateError(f'{self.cls.name}: cannot pass {e.type} as {t}')
        return e

    def ex_New(self, n):
        t = n.type
        args = [self.ex(a) for a in n.args]
        if t.base in self.classes:
            ctors = self.classes[t.base].ctors
            m = self.pick(n, [([p[0] for p in c.params], c) for c in ctors], args,
                          f'new {t.base}')[1]
            cargs = [self.convert(a, p[0]).c for a, p in zip(args, m.params)]
            if self.hoist is not None:
                self.call_args_order(n, args)
            return E(f'{m.cname}({", ".join(cargs)})', t, 16, m.effects | union(args),
                     R_OBJ | R_WORLD)
        r = SPEC.lib_new(t.base, [a.type for a in args], [a.c for a in args])
        if r is None:
            self.err(n, f'unsupported new {t}')
        if self.hoist is not None:
            self.call_args_order(n, args)
        return E(r, t, 16, union(args), readsof(args))

    def ex_NewArray(self, n):
        if n.init is not None or len(n.dims) != 1 or n.type.dims != 1:
            self.err(n, 'only one-dimensional arrays without initialisers are supported')
        d = self.ex(n.dims[0])
        et = n.type.elem()
        return E(f'JARR_NEW({SPEC.ctype(et)}, {d.c})', n.type, 16, d.effects, d.reads)

    def ex_Unary(self, n):
        e = self.ex(n.expr)
        if n.op == '!':
            return E(f'!{paren(e, 15)}', BOOL, 15, e.effects, e.reads)
        t = promote(e.type, INT) if is_num(e.type) else e.type
        c = f'{n.op}{paren(e, 15)}'
        if n.op == '-' and c.startswith('--'):
            c = f'-({e.c})'
        return E(c, t, 15, e.effects, e.reads)

    def ex_PreInc(self, n):
        e = self.ex(n.expr)
        self.nested_write(n)
        return E(f'{n.op}{paren(e, 15)}', e.type, 15, WRITE | e.effects, True)

    def ex_PostInc(self, n):
        e = self.ex(n.expr)
        self.nested_write(n)
        return E(f'{paren(e, 16)}{n.op}', e.type, 16, WRITE | e.effects, True)

    def nested_write(self, n):
        """An assignment or ++/-- inside a larger expression is checked
        against its neighbours like any other effect (see conflict)."""

    def ex_Cast(self, n):
        e = self.ex(n.expr)
        dst, src = n.type, e.type
        if dst == src:
            return e
        if is_num(dst) and is_num(src):
            fs = src.base in ('float', 'double')
            if dst.base == 'int' and fs:
                return E(f'j_d2i({e.c})', INT, 16, e.effects, e.reads)
            if dst.base == 'long' and fs:
                return E(f'j_d2l({e.c})', LONG, 16, e.effects, e.reads)
            if dst.base == 'char' and fs:
                return E(f'(char)j_d2i({e.c})', CHAR, 16, e.effects, e.reads)
            return E(f'({SPEC.ctype(dst)}){paren(e, 14)}', dst, 14, e.effects, e.reads)
        if is_ref(dst) and is_ref(src):
            return E(e.c, dst, e.prec, e.effects, e.reads)
        self.err(n, f'unsupported cast {src} -> {dst}')

    def ex_Binary(self, n):
        op = n.op
        l, r = self.ex(n.l), self.ex(n.r)
        if op == '+' and (l.type == STRING or r.type == STRING):
            self.err(n, 'string concatenation outside an assignment to a String')
        p = PREC[op]
        eff, rd = l.effects | r.effects, l.reads | r.reads
        if op in ('&&', '||'):
            return E(f'{paren(l, p)} {op} {paren(r, p + 1)}', BOOL, p, eff, rd)
        if op in ('==', '!='):
            if is_num(l.type) and is_num(r.type):
                pass
            elif l.type.base == 'boolean' or is_ref(l.type) or l.type == NULL:
                pass
            else:
                self.err(n, f'comparison of {l.type} and {r.type}')
            if self.hoist is not None:
                self.binary_order(n, l, r)
            return E(f'{paren(l, p)} {op} {paren(r, p + 1)}', BOOL, p, eff, rd)
        if op in ('<', '>', '<=', '>='):
            if self.hoist is not None:
                self.binary_order(n, l, r)
            return E(f'{paren(l, p)} {op} {paren(r, p + 1)}', BOOL, p, eff, rd)
        if op in ('&', '|', '^') and l.type.base == 'boolean':
            if self.hoist is not None:
                self.binary_order(n, l, r)
            return E(f'{paren(l, p)} {op} {paren(r, p + 1)}', BOOL, p, eff, rd)
        if not (is_num(l.type) and is_num(r.type)):
            self.err(n, f'{op} on {l.type} and {r.type}')
        if self.hoist is not None:
            self.binary_order(n, l, r)
        if op in ('<<', '>>', '>>>'):
            t = promote(l.type, INT)
            if not re.fullmatch(r'\d+', r.c):
                self.err(n, 'shift by a non-constant')
            if op == '>>>':
                self.err(n, '>>> is not supported')
            return E(f'{paren(l, p)} {op} {r.c}', t, p, eff, rd)
        t = promote(l.type, r.type)
        if op == '%' and t.base in ('float', 'double'):
            return E(f'fmod({l.c}, {r.c})', t, 16, eff, rd)
        if op in ('/', '%') and is_integral(t) and not re.fullmatch(r'[1-9]\d*', r.c):
            # Java throws on a zero divisor; C's is undefined. Biogenesis never
            # divides by a variable that can be zero, but say so in the code.
            pass
        return E(f'{paren(l, p)} {op} {paren(r, p + 1)}', t, p, eff, rd)

    def ex_Assign(self, n):
        tgt = self.ex(n.target)
        if not tgt.lvalue:
            self.err(n, 'assignment to a non-lvalue')
        if tgt.type == STRING:
            self.err(n, 'String assignment inside an expression')
        self.nested_write(n)
        if n.value.kind == 'Assign':
            self.chain_ok.add(n.value)
        val = self.ex(n.value)
        op = n.op
        if self.hoist is not None:
            if op == '=':
                # Java works out a[i] before the value; it matters if the
                # value changes the array or the index
                if '[' in tgt.c and self.unordered(tgt, val):
                    self.order_problem(n, 'assignment target and value')
            elif self.unordered(tgt, val):
                # x += f(): Java reads x before calling f
                self.order_problem(n, f"'{op}' target and value")
        if op == '=':
            if is_num(tgt.type) and is_num(val.type):
                if widens(val.type, tgt.type) is None:
                    self.err(n, f'narrowing assignment {val.type} -> {tgt.type}')
            return E(f'{tgt.c} = {val.c}', tgt.type, 2, WRITE | val.effects, True)
        bop = op[:-1]
        eff = WRITE | val.effects | tgt.effects
        if tgt.type.base == 'boolean' and bop in ('&', '|', '^'):
            return E(f'{tgt.c} {op} {paren(val, 3)}', tgt.type, 2, eff, True)
        if not (is_num(tgt.type) and is_num(val.type)):
            self.err(n, f'{op} on {tgt.type} and {val.type}')
        t = promote(tgt.type, val.type)
        if bop == '%' and t.base in ('float', 'double'):
            res = f'fmod({tgt.c}, {val.c})'
        else:
            res = None
        if t == tgt.type or (tgt.type.base in ('float', 'double') and t.base != 'double'):
            if res:
                return E(f'{tgt.c} = {res}', tgt.type, 2, eff, True)
            return E(f'{tgt.c} {op} {paren(val, 3)}', tgt.type, 2, eff, True)
        # implicit narrowing: x op= y is x = (T)(x op y)
        if '[' in tgt.c and tgt.effects & WRITE:
            self.err(n, 'narrowing compound assignment to an element with side effects')
        inner = res or f'{tgt.c} {bop} {paren(val, PREC[bop] + 1)}'
        if is_integral(tgt.type) and t.base in ('float', 'double'):
            conv = 'j_d2i' if tgt.type.base == 'int' else ('j_d2l' if tgt.type.base == 'long'
                                                           else None)
            if conv is None:
                self.err(n, f'compound narrowing to {tgt.type}')
            return E(f'{tgt.c} = {conv}({inner})', tgt.type, 2, eff, True)
        return E(f'{tgt.c} = ({SPEC.ctype(tgt.type)})({inner})', tgt.type, 2, eff, True)

    def ex_Cond(self, n):
        c = self.ex(n.cond)
        a = self.ex(n.a)
        b = self.ex(n.b)
        t = a.type if a.type == b.type else (promote(a.type, b.type)
                                             if is_num(a.type) and is_num(b.type) else a.type)
        return E(f'{paren(c, 4)} ? {paren(a, 4)} : {paren(b, 3)}', t, 3,
                 c.effects | a.effects | b.effects, c.reads | a.reads | b.reads)

    # ---- evaluation order ----
    def binary_order(self, n, l, r):
        if self.unordered(l, r):
            self.order_problem(n, f"operands of '{n.op}'")

    def call_args_order(self, n, items):
        items = [i for i in items if i is not None]
        for i in range(len(items)):
            for j in range(i + 1, len(items)):
                if self.unordered(items[i], items[j]):
                    self.order_problem(n, f'arguments of {getattr(n, "name", "new")}')
                    return

    def unordered(self, a, b):
        """Could C's choice of order between a and b change the result? Two
        draws from the random stream must come in Java's order; a write must
        not move past a read or write of what it may change. A draw alone
        changes nothing but the stream."""
        return self.conflict(a, b) or self.conflict(b, a)

    def conflict(self, a, b):
        if a.effects & RANDOM and b.effects & RANDOM:
            return True
        if a.effects & WRITE and (b.reads or b.effects & (WRITE | WORLD)):
            return True
        if a.effects & WORLD and (b.reads & R_WORLD or b.effects & (WRITE | WORLD)):
            return True
        if a.node is not None and b.node is not None:
            ar, aw = self.locals_of(a.node)
            br, bw = self.locals_of(b.node)
            if aw & (br | bw):
                return True
        return False

    def order_problem(self, n, what):
        self.hoist.append((n, what))

    def check_order(self, n, e):
        pass
