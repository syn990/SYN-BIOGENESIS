# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""Statements, functions and files for j2c.py."""

import os
import re

import j2c_spec as SPEC
from j2c import (BOOL, DOUBLE, INT, NULL, RANDOM, STRING, WRITE, E, Local, Scope, Translator,
                 TranslateError, T, build_class, is_num, is_ref, paren, snake, widens)
from jparse import Node, Type, parse_file

C_RESERVED = {'auto', 'char', 'const', 'default', 'double', 'enum', 'extern', 'float', 'goto',
              'inline', 'int', 'long', 'register', 'restrict', 'short', 'signed', 'sizeof',
              'static', 'struct', 'typedef', 'union', 'unsigned', 'void', 'volatile', 'P'}

INDENT = '    '

IMPURE_LIB_NAMES = {'nextInt', 'nextDouble', 'nextBoolean', 'randomSign', 'randomMutation'}


def c_comment(text):
    """A Java comment as C, or None to drop it (commented-out code)."""
    if text.startswith('//'):
        return text
    body = text[2:-2]
    if body.startswith('*'):
        body = body[1:]
    code_like = re.search(r';\s*$|^\s*[{}]\s*$', body, re.M) and ('(' in body)
    if code_like and not text.startswith('/**'):
        return None
    lines = [ln.strip() for ln in body.strip('\n').split('\n')]
    lines = [re.sub(r'^\* ?', '', ln) for ln in lines]
    while lines and not lines[-1]:
        lines.pop()
    while lines and not lines[0]:
        lines.pop(0)
    if not lines:
        return None
    if len(lines) == 1:
        return f'/* {lines[0]} */'
    return '/* ' + '\n * '.join(lines) + '\n */'


def ends_flow(st):
    if st.kind in ('Break', 'Return', 'Continue', 'Throw'):
        return True
    if st.kind == 'Block' and st.stmts:
        return ends_flow(st.stmts[-1])
    return False


class Emitter(Translator):
    def __init__(self, classes):
        super().__init__(classes)
        self.hoist = None
        self.loc_memo = {}
        self.order_problems = []
        self.local_names = set()

    # ---- what methods do ----
    def analyse_effects(self):
        """Each method's effect mask (RANDOM, WRITE), to a fixpoint. Calls
        are matched by name across all classes, which can only add effects."""
        ms = [m for c in self.classes.values() for ml in c.methods.values() for m in ml]
        ms += [m for c in self.classes.values() for m in c.ctors]
        by_name = {}
        for m in ms:
            m.effects = 0
            by_name.setdefault(m.name, []).append(m)
        changed = True
        while changed:
            changed = False
            for m in ms:
                e = self.body_effects(m, by_name)
                if e != m.effects:
                    m.effects = e
                    changed = True

    def body_effects(self, m, by_name):
        locals_ = {p[1] for p in m.params}
        mask = 0

        def walk(n):
            nonlocal mask
            if isinstance(n, (list, tuple)):
                for x in n:
                    walk(x)
                return
            if not isinstance(n, Node):
                return
            k = n.kind
            if k == 'Local':
                for t, name, init in n.decls:
                    locals_.add(name)
            elif k == 'ForEach':
                locals_.add(n.name)
            elif k in ('Assign', 'PreInc', 'PostInc'):
                tgt = n.target if k == 'Assign' else n.expr
                if not (tgt.kind == 'Name' and tgt.name in locals_):
                    mask |= WRITE
            elif k == 'Call' and n.obj is not None and n.obj.kind == 'Super':
                pass  # Object.clone: a new copy, nothing else changes
            elif k == 'Call':
                for mm in by_name.get(n.name, []):
                    mask |= mm.effects
                mask |= SPEC.name_effects(n.name)
            elif k == 'New' and n.type.base in self.classes:
                for c in self.classes[n.type.base].ctors:
                    mask |= c.effects
            for v in n.__dict__.values():
                if isinstance(v, (Node, list, tuple)):
                    walk(v)
        walk(m.node.body)
        return mask

    # ---- names ----
    def local_cname(self, name):
        c = name
        if c in C_RESERVED or c == self.self_name or c in self.reserved_locals:
            c = name + '_'
        return c

    # ---- statements ----
    def comments(self, node, ind, attr='comments'):
        out = []
        for text, own_line in getattr(node, attr, []) or []:
            c = c_comment(text)
            if c is None:
                continue
            for ln in c.split('\n'):
                out.append(ind + ln)
        return out

    def top_expr(self, n):
        """Translate an expression at statement level, collecting evaluation
        order problems."""
        self.hoist = []
        self.top_node = n
        self.chain_ok = set()
        self.loc_memo = {}
        try:
            e = self.ex(n)
        finally:
            probs, self.hoist = self.hoist, None
        return e, probs

    def texpr(self, n):
        """top_expr where evaluation-order problems are errors."""
        e, probs = self.top_expr(n)
        if probs:
            self.err(probs[0][0], 'evaluation order: ' + probs[0][1])
        return e

    def block_lines(self, stmts, ind, scope=True):
        if scope:
            self.scope = Scope(self.scope)
        out = []
        for s in stmts:
            out += self.stmt(s, ind)
        if scope:
            self.scope = self.scope.parent
        return out

    def body(self, s, ind):
        """The body of if/for/while: braces when Java had them."""
        if s.kind == 'Block':
            lines = [' {']
            lines += self.block_lines(s.stmts, ind + INDENT)
            lines += self.comments(s, ind + INDENT, 'end_comments')
            lines.append(ind + '}')
            return lines, True
        inner = self.stmt(s, ind + INDENT)
        return [''] + inner, False

    def stmt(self, s, ind):
        out = self.comments(s, ind)
        if getattr(s, 'else_comments', None):
            out = self.comments(s, ind, 'else_comments') + out
        k = s.kind
        f = getattr(self, 'st_' + k, None)
        if f is None:
            self.err(s, f'unsupported statement {k}')
        return out + f(s, ind)

    def st_Block(self, s, ind):
        out = [ind + '{']
        out += self.block_lines(s.stmts, ind + INDENT)
        out += self.comments(s, ind + INDENT, 'end_comments')
        out.append(ind + '}')
        return out

    def st_Empty(self, s, ind):
        return [ind + ';']

    def declare(self, name, typ):
        cname = self.local_cname(name)
        self.scope.vars[name] = Local(name, typ, cname)
        return cname

    def st_Local(self, s, ind):
        out = []
        for typ, name, init in s.decls:
            if typ == STRING:
                self.err(s, 'String locals are not supported')
            if init is not None and init.kind == 'ArrayInit':
                self.err(s, 'array initialisers are not supported')
            e = None
            if init is not None:
                e, probs = self.top_expr(init)
                if probs:
                    if all(p[0] is init for p in probs) and init.kind in ('New', 'Call'):
                        # Java evaluates arguments left to right; C doesn't
                        # promise to, so give each its own statement
                        pre, init = self.sequence_args(init, ind)
                        out += pre
                        e, probs = self.top_expr(init)
                    if probs:
                        self.err(s, 'evaluation order: ' + probs[0][1])
                if is_num(typ) and is_num(e.type) and widens(e.type, typ) is None:
                    if init.kind == 'Lit' and typ.base in ('char', 'byte', 'short'):
                        pass
                    else:
                        self.err(s, f'narrowing initialisation {e.type} -> {typ}')
            cname = self.declare(name, typ)
            if e is None:
                if typ.base in SPEC.VALUE_TYPES and not typ.dims:
                    val = '{0}'
                elif typ.dims or is_ref(typ):
                    val = 'NULL'
                elif typ.base == 'boolean':
                    val = 'false'
                else:
                    val = '0'
            else:
                val = e.c
            out.append(f'{ind}{SPEC.decl(typ, cname)} = {val};')
        return out

    def sequence_args(self, n, ind):
        """Hoist n's arguments into temporaries, in order. Returns the
        statements and a copy of n that uses them."""
        pre, names = [], []
        for arg in n.args:
            e, probs = self.top_expr(arg)
            if probs:
                self.err(arg, 'evaluation order inside an argument')
            tmp = self.fresh('arg')
            self.scope.vars[tmp] = Local(tmp, e.type, tmp)
            pre.append(f'{ind}{SPEC.decl(e.type, tmp)} = {e.c};')
            names.append(Node('Name', None, name=tmp))
        copy = Node(n.kind, None, **{k: v for k, v in n.__dict__.items()
                                     if k not in ('kind', 'line', 'comments')})
        copy.line = n.line
        copy.args = names
        return pre, copy

    def st_ExprStmt(self, s, ind):
        n = s.expr
        if n.kind == 'Assign':
            tgt_type = self.peek_type(n.target)
            if tgt_type == STRING:
                return self.string_assign(n, ind)
            if tgt_type is not None and tgt_type.dims and n.op == '=':
                tgt = self.texpr(n.target)
                val, probs = self.top_expr(n.value)
                if n.value.kind == 'NewArray' and '->' in tgt.c:
                    return [f'{ind}JARR_SET({tgt.c}, {val.c});']
                if n.value.kind == 'Lit' and n.value.text == 'null':
                    return [f'{ind}JARR_SET({tgt.c}, NULL);']
                if n.target.kind == 'Name' and self.scope.find(n.target.name):
                    return [f'{ind}{tgt.c} = {val.c};']
                self.err(s, 'array fields may only be given new arrays')
        e, probs = self.top_expr(n)
        if probs:
            self.err(s, 'evaluation order: ' + probs[0][1])
        return [f'{ind}{e.c};']

    def peek_type(self, n):
        saved = self.hoist
        self.hoist = None
        try:
            return self.ex(n).type
        except TranslateError:
            return None
        finally:
            self.hoist = saved

    def is_string_concat(self, n):
        """Syntactically a + chain containing a String literal or
        Integer.toString: what Biogenesis builds strings from."""
        if n.kind == 'Paren':
            return self.is_string_concat(n.expr)
        if n.kind == 'Binary' and n.op == '+':
            return self.is_string_concat(n.l) or self.is_string_concat(n.r)
        if n.kind == 'Lit' and n.type == STRING:
            return True
        return n.kind == 'Call' and n.name == 'toString'

    # Strings: Biogenesis keeps two, a clade label and the free birth
    # positions around a parent; Java's immutable String becomes an owned
    # char * that is copied on assignment.
    def string_parts(self, n, parts):
        if n.kind == 'Paren':
            return self.string_parts(n.expr, parts)
        if n.kind == 'Binary' and n.op == '+' and self.is_string_concat(n):
            self.string_parts(n.l, parts)
            self.string_parts(n.r, parts)
            return parts
        if n.kind == 'Lit' and n.type == STRING:
            parts.append(('lit', n.text[1:-1].replace('%', '%%')))
            return parts
        if n.kind == 'Call' and n.name == 'toString' and n.obj is not None and \
                n.obj.kind == 'Name' and n.obj.name == 'Integer':
            e = self.texpr(n.args[0])
            parts.append(('%d', e.c))
            return parts
        e = self.texpr(n)
        if e.type == STRING:
            # Java writes a null String as "null"
            parts.append(('%s', f'JSTR_JAVA({e.c})'))
        elif e.type.base == 'int':
            parts.append(('%d', e.c))
        elif e.type.base == 'char':
            parts.append(('%c', e.c))
        else:
            self.err(n, f'{e.type} in a string')
        return parts

    def is_string(self, n):
        if n.kind == 'Paren':
            return self.is_string(n.expr)
        if n.kind == 'Binary' and n.op == '+':
            return self.is_string(n.l) or self.is_string(n.r)
        return self.peek_type(n) == STRING

    def string_assign(self, n, ind):
        """Strings are shared like Java's (see jrt.h): assigning one shares
        it, a literal is the interned one, and concatenation makes a new
        one."""
        tgt = self.texpr(n.target)
        v = n.value
        if n.op == '=' and v.kind == 'Call' and v.name == 'replace':
            src = self.texpr(v.obj)
            a = self.texpr(v.args[0])
            b = self.texpr(v.args[1])
            return [f'{ind}{tgt.c} = jstr_replace({src.c}, {a.c}, {b.c});']
        if n.op == '=' and v.kind == 'Lit' and v.type == STRING:
            return [f'{ind}{tgt.c} = jstr_lit({v.text});']
        if n.op == '=' and not self.is_string_concat(v):
            e = self.texpr(v)
            if e.type != STRING and e.type != NULL:
                self.err(n, f'{e.type} assigned to a String')
            return [f'{ind}{tgt.c} = {e.c};']
        parts = []
        if n.op == '+=':
            parts.append(('%s', f'JSTR_JAVA({tgt.c})'))
        elif n.op != '=':
            self.err(n, f'{n.op} on a String')
        self.string_parts(v, parts)
        fmt = ''.join(p[1] if p[0] == 'lit' else p[0] for p in parts)
        args = [p[1] for p in parts if p[0] != 'lit']
        return [f'{ind}{tgt.c} = jstr_newf("{fmt}"{"".join(", " + a for a in args)});']

    def st_If(self, s, ind):
        c, probs = self.top_expr(s.cond)
        if probs:
            self.err(s, 'evaluation order: ' + probs[0][1])
        out = []
        lines, braced = self.body(s.then, ind)
        head = f'{ind}if ({c.c})'
        out.append(head + lines[0])
        out += lines[1:]
        if s.els is not None:
            els_comments = self.comments(s.els, ind, 'else_comments')
            if s.els.kind == 'If':
                inner = self.stmt_no_comments(s.els, ind)
                first = inner[0].lstrip()
                if braced:
                    out[-1] += ' else ' + first
                else:
                    out.append(ind + 'else ' + first)
                out += inner[1:]
            else:
                lines, b2 = self.body(s.els, ind)
                if braced:
                    out[-1] += ' else' + lines[0]
                else:
                    out.append(ind + 'else' + lines[0])
                out += lines[1:]
            if els_comments:
                out = out  # comments before else are rare; keep them out of the way
        return out

    def stmt_no_comments(self, s, ind):
        saved = s.comments
        s.comments = []
        try:
            return self.stmt(s, ind)
        finally:
            s.comments = saved

    def st_For(self, s, ind):
        self.scope = Scope(self.scope)
        inits = []
        pre = []
        for i in s.init:
            if i.kind == 'Local':
                parts = []
                for typ, name, init in i.decls:
                    e = self.texpr(init) if init is not None else None
                    cname = self.declare(name, typ)
                    parts.append((typ, cname, e))
                t0 = parts[0][0]
                inits.append(SPEC.decl(t0, parts[0][1]) +
                             (f' = {parts[0][2].c}' if parts[0][2] else '') +
                             ''.join(f', {p[1]}' + (f' = {p[2].c}' if p[2] else '')
                                     for p in parts[1:]))
            else:
                e = self.texpr(i.expr)
                inits.append(e.c)
        cond = self.texpr(s.cond).c if s.cond is not None else ''
        upd = ', '.join(self.texpr(u).c for u in s.update)
        lines, _ = self.body(s.body, ind)
        self.scope = self.scope.parent
        out = pre + [f'{ind}for ({", ".join(inits)}; {cond}; {upd})' + lines[0]]
        return out + lines[1:]

    def st_ForEach(self, s, ind):
        chars = s.iter.kind == 'Call' and s.iter.name == 'toCharArray'
        it = None if chars else self.texpr(s.iter)
        self.scope = Scope(self.scope)
        out = []
        if chars:
            src = self.texpr(s.iter.obj)
            buf = self.fresh('chars')
            p = self.fresh('p')
            cname = self.declare(s.name, s.type)
            out.append(f'{ind}{{')
            out.append(f'{ind}{INDENT}char {buf}[JSTR_LOOP_MAX];')
            out.append(f'{ind}{INDENT}jstr_copy_to({buf}, sizeof {buf}, {src.c});')
            out.append(f'{ind}{INDENT}for (const char *{p} = {buf}; *{p}; {p}++) {{')
            out.append(f'{ind}{INDENT * 2}char {cname} = *{p};')
            body = s.body.stmts if s.body.kind == 'Block' else [s.body]
            out += self.block_lines(body, ind + INDENT * 2)
            out.append(f'{ind}{INDENT}}}')
            out.append(f'{ind}}}')
        elif it.type.dims:
            i = self.fresh('i')
            cname = self.declare(s.name, s.type)
            out.append(f'{ind}for (int {i} = 0; {i} < JLEN({it.c}); {i}++) {{')
            out.append(f'{ind}{INDENT}{SPEC.decl(s.type, cname)} = {it.c}[{i}];')
            body = s.body.stmts if s.body.kind == 'Block' else [s.body]
            out += self.block_lines(body, ind + INDENT)
            out.append(f'{ind}}}')
        else:
            self.err(s, f'for-each over {it.type}')
        self.scope = self.scope.parent
        return out

    def fresh(self, base):
        self.tmp_n += 1
        return f'{base}_{self.tmp_n}'

    def st_While(self, s, ind):
        c = self.texpr(s.cond)
        lines, _ = self.body(s.body, ind)
        return [f'{ind}while ({c.c})' + lines[0]] + lines[1:]

    def st_Switch(self, s, ind):
        e, probs = self.top_expr(s.expr)
        if probs:
            self.err(s, 'evaluation order: ' + probs[0][1])
        out = [f'{ind}switch ({e.c}) {{']
        self.scope = Scope(self.scope)
        for case in s.cases:
            out += self.comments(case, ind)
            for lab in case.labels:
                if lab is None:
                    out.append(f'{ind}default:')
                else:
                    le = self.texpr(lab)
                    out.append(f'{ind}case {le.c}:')
            if case.body and case.body[0].kind == 'Local':
                out[-1] += ';'
            for st in case.body:
                out += self.stmt(st, ind + INDENT)
            if case.body and case is not s.cases[-1] and not ends_flow(case.body[-1]):
                out.append(f'{ind}{INDENT}__attribute__((fallthrough)); /* as in Color Mod */')
        out += self.comments(s, ind + INDENT, 'end_comments')
        self.scope = self.scope.parent
        out.append(f'{ind}}}')
        return out

    def st_Break(self, s, ind):
        if s.label:
            self.err(s, 'labelled break')
        return [ind + 'break;']

    def st_Continue(self, s, ind):
        if s.label:
            self.err(s, 'labelled continue')
        return [ind + 'continue;']

    def st_Return(self, s, ind):
        if s.expr is None:
            return [ind + 'return;']
        if self.method.ret == STRING and s.expr.kind not in ('Name', 'Field'):
            # a getter hands out the field itself, borrowed, like Java's
            # reference; anything else would need an owner
            self.err(s, 'returning a String that is not a field')
        e, probs = self.top_expr(s.expr)
        if probs:
            self.err(s, 'evaluation order: ' + probs[0][1])
        return [f'{ind}return {e.c};']

    def st_Try(self, s, ind):
        for ctypes, cname, blk in s.catches:
            if [str(t) for t in ctypes] != ['CloneNotSupportedException']:
                self.err(s, 'try/catch')
        if s.fin is not None:
            self.err(s, 'finally')
        return self.block_lines(s.body.stmts, ind, scope=True)

    # ---- methods ----
    def proto(self, m):
        params = []
        if not m.static and m.name != '<init>':
            params.append(f'{self.ctype_self(m.cls)}{m.cls.conf["self"]}')
        for t, name in m.params:
            params.append(SPEC.decl(t, self.local_cname_static(name, m)))
        ret = SPEC.ctype(m.ret)
        sep = '' if ret.endswith('*') else ' '
        return f'{ret}{sep}{m.cname}({", ".join(params) or "void"})'

    def local_cname_static(self, name, m):
        c = name
        if c in C_RESERVED or c == m.cls.conf['self']:
            c = name + '_'
        return c

    @staticmethod
    def ctype_self(cls):
        return f'{cls.name} *'

    def function(self, m):
        self.cls = m.cls
        self.method = m
        self.self_name = m.cls.conf['self']
        self.reserved_locals = set()
        self.scope = Scope()
        for t, name in m.params:
            self.declare(name, t)
        out = []
        doc = [c for c in m.node.comments if c[0].startswith('/**')]
        if doc:
            cc = c_comment(doc[-1][0])
            if cc:
                out += cc.split('\n')
        static = 'static ' if 'private' in m.node.mods else ''
        out.append(static + self.proto(m))
        out.append('{')
        if m.name == '<init>':
            out.append(f'{INDENT}{self.ctype_self(m.cls)}{self.self_name} = '
                       f'{m.cls.conf["prefix"]}alloc();')
            body = list(m.node.body.stmts)
            if body and body[0].kind == 'ExprStmt' and body[0].expr.kind == 'Call' and \
                    body[0].expr.name in ('this', 'super'):
                self.err(body[0], 'this()/super() in a constructor')
            out += self.block_lines(body, INDENT, scope=False)
            out.append(f'{INDENT}return {self.self_name};')
        else:
            out += self.block_lines(m.node.body.stmts, INDENT, scope=False)
            out += self.comments(m.node.body, INDENT, 'end_comments')
        out.append('}')
        # parameters the Java ignores
        body = '\n'.join(out[out.index('{'):])
        unused = [self.local_cname_static(n, m) for t, n in m.params
                  if not re.search(r'\b' + re.escape(self.local_cname_static(n, m)) + r'\b', body)]
        if unused:
            i = out.index('{') + 1
            out[i:i] = [f'{INDENT}(void){u};' for u in unused]
        return out


# ---------------------------------------------------------------------------
# Files


HEADER = """/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Translated to C in 2026 by tools/colour/j2c.py from {java} of
 * Biogenesis Color Mod 2.01 (https://sourceforge.net/projects/biogenesis/),
 * Copyright (C) 2006-2010 Joan Queralt Molina,
 * Color Mod Copyright (C) 2012-2022 MarcoDBAA.
 * Generated: edit the translator or the Java, not this file.
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
"""


GC_TYPES = {'GeneticCode', 'Gene'}

FILES = {
    # output file: (classes, Java files named in its header, description)
    'genetic_code': (['Gene', 'GeneticCode'], 'Gene.java and GeneticCode.java',
                     """Genes and genetic codes (Gene.java, GeneticCode.java) of Color Mod.

 * A gene is one segment: length, angle, colour, the branch it grows from,
 * and how it reacts to each colour it touches. A genetic code is the
 * genes, their symmetry and mirroring, and the code's traits (mutation
 * rate, activity, behaviours, clade). Both are collected objects (see
 * jgc_alloc in jrt.h), shared by reference as in Java."""),
    'organism': (['Organism'], 'Organism.java',
                 """Organism.java of Color Mod: a body of coloured segments that
 * moves, feeds, breathes, grows, collides and reproduces, and the rules of
 * what each colour does when it touches another."""),
}


def field_comment(f):
    docs = [c for c in f.node.comments if c[0].startswith('/**')]
    if not docs:
        return None
    text = c_comment(docs[-1][0])
    if not text:
        return None
    first = text.split('\n')[0].strip()
    first = first[3:] if first.startswith('/* ') else first[2:]
    first = first.rstrip(' */').strip()
    return first or None


def emit_struct(cls):
    out = [f'typedef struct {cls.name} {{']
    if cls.conf.get('collected'):
        out.append('    JGcHead gch; /* the collector\'s, not Java\'s */')
    if cls.super == 'Rectangle':
        out.append('    JRect r; /* the java.awt.Rectangle it extends: the bounding box */')
    for f in cls.fields.values():
        if f.static:
            continue
        com = field_comment(f)
        line = f'    {SPEC.decl(f.type, f.cname)};'
        if com:
            if len(line) + len(com) + 7 <= 100:
                line += f' /* {com} */'
            else:
                out.append(f'    /* {com} */')
        out.append(line)
    for line in cls.conf.get('extra_fields', []):
        out.append('    ' + line)
    out.append(f'}} {cls.name};')
    return out


def constants(cls):
    out = []
    for f in cls.fields.values():
        if f.static and f.final and f.init is not None and f.init.kind in ('Lit', 'Unary') \
                and f.name != 'serialVersionUID':
            if f.type.base != 'int':
                raise TranslateError(f'{cls.name}.{f.name}: non-int constant')
            v = f.init.text if f.init.kind == 'Lit' else f.init.op + f.init.expr.text
            out.append((SPEC.const_name(cls.name, f), v))
    return out


def emit_alloc(em, cls):
    """{prefix}alloc: a zeroed object with Java's field initialisers run."""
    p = cls.conf['prefix']
    self_ = cls.conf['self']
    em.cls = cls
    em.self_name = self_
    em.reserved_locals = set()
    em.scope = Scope()
    em.method = None
    out = [f'static {cls.name} *{p}alloc(void)', '{']
    if cls.conf.get('collected'):
        out.append(f'    {cls.name} *{self_} = jgc_alloc(sizeof *{self_}, &{p}kind);')
    else:
        out.append(f'    {cls.name} *{self_} = calloc(1, sizeof *{self_});')
        out.append(f'    if (!{self_})')
        out.append('        abort();')
    for f in cls.fields.values():
        if f.static or f.init is None:
            continue
        e = em.texpr(f.init)
        if e.c in ('0', '0.0', 'false', 'NULL', '(JRect){0}'):
            continue  # calloc
        if f.type == STRING:
            raise TranslateError(f'{cls.name}.{f.name}: String initialiser')
        out.append(f'    {self_}->{f.cname} = {e.c};')
    out.append(f'    return {self_};')
    out.append('}')
    return out


def owned(cls):
    """Arrays and strings an object owns."""
    arrs = [f for f in cls.fields.values() if not f.static and f.type.dims]
    strs = [f for f in cls.fields.values() if not f.static and f.type == STRING]
    return arrs, strs


def emit_lifetime(cls):
    p = cls.conf['prefix']
    s = cls.conf['self']
    arrs, strs = owned(cls)
    out = []
    # what it owns
    fin = f'{p}finalize'
    out += [f'static void {fin}(void *obj)', '{', f'    {cls.name} *{s} = obj;']
    for f in arrs:
        out.append(f'    jarr_free({s}->{f.cname});')
    if not arrs:
        out.append(f'    (void){s};')
    out += ['}', '']
    # what it refers to
    mark_name = f'{p}mark_refs'
    out += [f'{"static " if cls.conf.get("collected") else ""}void {mark_name}(void *obj)', '{',
            f'    {cls.name} *{s} = obj;']
    any_ref = False
    for f in cls.fields.values():
        if f.static:
            continue
        if f.type.dims == 0 and f.type.base in GC_TYPES:
            out.append(f'    jgc_mark({s}->{f.cname});')
            any_ref = True
        elif f.type.dims == 1 and f.type.base in GC_TYPES:
            out.append(f'    if ({s}->{f.cname})')
            out.append(f'        for (int i = 0; i < JLEN({s}->{f.cname}); i++)')
            out.append(f'            jgc_mark({s}->{f.cname}[i]);')
            any_ref = True
        elif f.type == STRING:
            out.append(f'    jstr_mark({s}->{f.cname});')
            any_ref = True
    if not any_ref:
        out.append(f'    (void){s};')
    out += ['}', '']
    if cls.conf.get('collected'):
        out = [f'static void {fin}(void *obj);', f'static void {mark_name}(void *obj);',
               f'static const JGcKind {p}kind = {{"{cls.name}", {mark_name}, {fin}}};', ''] + out
        # Object.clone
        out += [f'/* Object.clone: a field-by-field copy. Arrays are copied too, since',
                ' * each belongs to one field; strings are shared, as in Java. */',
                f'static {cls.name} *{p}shallow_clone({cls.name} *{s})', '{',
                f'    {cls.name} *c = jgc_alloc(sizeof *c, &{p}kind);',
                '    JGcHead h = c->gch;',
                f'    *c = *{s};',
                '    c->gch = h;']
        for f in arrs:
            out.append(f'    c->{f.cname} = jarr_dup({s}->{f.cname});')
        out += ['    return c;', '}', '']
    else:
        out += [f'void {p}free({cls.name} *{s})', '{', f'    if (!{s})', '        return;',
                f'    {fin}({s});', f'    free({s});', '}', '']
    return out


def translate_all(src_dir, out_dir):
    classes = {}
    for name, conf in SPEC.CLASSES.items():
        units = parse_file(os.path.join(src_dir, name + '.java'))
        node = [c for c in units if c.name == name][0]
        classes[name] = build_class(node, conf)
    utils = parse_file(os.path.join(src_dir, 'Utils.java'), skip_bodies=True)[0]
    SPEC.load_utils([(n, t, 'static' in m.mods, 'final' in m.mods)
                     for m in utils.members if m.kind == 'Field' for t, n, _ in m.decls])
    em = Emitter(classes)
    em.analyse_effects()
    os.makedirs(out_dir, exist_ok=True)
    for fname, (cnames, java, desc) in FILES.items():
        hdr = [HEADER.format(java=java)]
        guard = f'COLOUR_{fname.upper()}_H'
        hdr.append(f'/* {desc}\n */')
        hdr += [f'#ifndef {guard}', f'#define {guard}', '', '#include <stdbool.h>',
                '#include <stdint.h>', '']
        if fname == 'organism':
            hdr += ['#include "sim/genetic_code.h"']
        hdr += ['#include "sim/jrt.h"', '']
        if fname == 'organism':
            hdr += ['typedef struct World World;', '']
        else:
            hdr += ['typedef struct Gene Gene;', '',
                    '/* java.util.List<Gene>, for the constructor the file readers use */',
                    'typedef struct {', '    Gene **items;', '    int n;', '} GeneList;',
                    '', 'Gene **gene_list_to_array(const GeneList *l, Gene **a);', '']
        src = [HEADER.format(java=java), f'#include "sim/{fname}.h"', '',
               '#include <math.h>', '#include <stdio.h>', '#include <stdlib.h>',
               '#include <string.h>', '', '#include "sim/params.h"']
        if fname == 'organism':
            src.append('#include "sim/world.h"')
        src.append('')
        protos_pub, protos_priv, bodies = [], [], []
        for cname in cnames:
            cls = classes[cname]
            consts = constants(cls)
            if consts:
                block = ['enum {'] + [f'    {n} = {v},' for n, v in consts] + ['};', '']
                if cname == 'Organism':
                    hdr += ['/* What a colour does (Organism.getTypeColor) */'] + block
                else:
                    hdr += block
            hdr += emit_struct(cls) + ['']
            for f in cls.fields.values():
                if f.static and not (f.final and f.init is not None and
                                     f.init.kind in ('Lit', 'Unary')) and \
                        f.name != 'serialVersionUID':
                    init = '' if (f.init is None or f.init.kind == 'New') else None
                    if init is None:
                        raise TranslateError(f'{cname}.{f.name}: static initialiser')
                    src.append(f'static {SPEC.decl(f.type, SPEC.static_name(cname, f))};')
            p = cls.conf['prefix']
            bodies.append('\n'.join(emit_lifetime(cls)))
            protos_pub.append('/* A zeroed object for the world reader: deserialization runs no')
            protos_pub.append(' * constructor and no field initialiser. */')
            protos_pub.append(f'{cname} *{p}alloc_blank(void);')
            bodies.append(f'{cname} *{p}alloc_blank(void)\n{{\n' +
                          (f'    return jgc_alloc(sizeof({cname}), &{p}kind);\n}}'
                           if cls.conf.get('collected') else
                           f'    {cname} *x = calloc(1, sizeof *x);\n    if (!x)\n'
                           f'        abort();\n    return x;\n}}'))
            if not cls.conf.get('collected'):
                protos_pub.append(f'void {p}free({cname} *{cls.conf["self"]});')
                protos_pub.append(f'/* Marks the collected objects it refers to (jgc_mark). */')
                protos_pub.append(f'void {p}mark_refs(void *obj);')
            bodies.append('\n'.join(emit_alloc(em, cls)))
            for m in [m for ms in cls.methods.values() for m in ms] + cls.ctors:
                if m.node.body is None:
                    continue
                proto = em.proto(m) + ';'
                if 'private' in m.node.mods:
                    protos_priv.append('static ' + proto)
                else:
                    protos_pub.append(proto)
                bodies.append('\n'.join(em.function(m)))
        if fname == 'genetic_code':
            bodies.append("""Gene **gene_list_to_array(const GeneList *l, Gene **a)
{
    /* List.toArray(T[] a), for an array of the list's size */
    for (int i = 0; i < l->n && i < JLEN(a); i++)
        a[i] = l->items[i];
    return a;
}""")
        hdr += protos_pub + ['', '#endif']
        src += [x for x in protos_priv] + ['']
        src += [b + '\n' for b in bodies]
        with open(os.path.join(out_dir, fname + '.h'), 'w') as f:
            f.write('\n'.join(hdr) + '\n')
        with open(os.path.join(out_dir, fname + '.c'), 'w') as f:
            f.write('\n'.join(src))
    import j2c_params
    j2c_params.generate(src_dir, out_dir)
    return classes
