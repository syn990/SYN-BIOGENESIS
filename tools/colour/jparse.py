# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""A lexer and parser for the Java that Biogenesis Color Mod is written in.

It covers what the simulation classes use (classes, fields, methods,
constructors, the usual statements and expressions) and stops with an
error at anything else, so nothing is translated by guesswork. Comments
are kept: each token carries the comments that came before it, and the
translator writes them out with the statement they belong to.
"""

import re

KEYWORDS = {
    'abstract', 'boolean', 'break', 'byte', 'case', 'catch', 'char', 'class', 'continue',
    'default', 'do', 'double', 'else', 'extends', 'final', 'finally', 'float', 'for', 'if',
    'implements', 'import', 'instanceof', 'int', 'interface', 'long', 'new', 'package',
    'private', 'protected', 'public', 'return', 'short', 'static', 'super', 'switch',
    'synchronized', 'this', 'throw', 'throws', 'transient', 'try', 'void', 'volatile', 'while',
    'true', 'false', 'null',
}
PRIMITIVES = {'boolean', 'byte', 'char', 'short', 'int', 'long', 'float', 'double', 'void'}
MODIFIERS = {'public', 'protected', 'private', 'static', 'final', 'abstract', 'transient',
             'volatile', 'synchronized'}

OPERATORS = sorted("""
>>>= <<= >>= >>> ... -> :: ++ -- && || == != <= >= += -= *= /= %= &= |= ^= << >>
( ) { } [ ] ; , . @ = > < ! ~ ? : + - * / & | ^ %
""".split(), key=len, reverse=True)


class JavaError(Exception):
    pass


class Tok:
    __slots__ = ('kind', 'text', 'line', 'comments', 'nl_before')

    def __init__(self, kind, text, line, comments, nl_before):
        self.kind = kind          # 'id', 'kw', 'num', 'str', 'chr', 'op', 'eof'
        self.text = text
        self.line = line
        self.comments = comments  # [(text, own_line)] before this token
        self.nl_before = nl_before

    def __repr__(self):
        return f'{self.kind}:{self.text}@{self.line}'


NUM_RE = re.compile(r'0[xX][0-9a-fA-F]+[lL]?|(\d+\.\d*|\.\d+|\d+)([eE][+-]?\d+)?[dDfFlL]?')
ID_RE = re.compile(r'[A-Za-z_$][A-Za-z0-9_$]*')


def lex(src, name='?'):
    toks = []
    i, n, line = 0, len(src), 1
    comments = []
    nl = True
    while True:
        # whitespace and comments
        while i < n:
            c = src[i]
            if c == '\n':
                line += 1
                nl = True
                i += 1
            elif c in ' \t\r\f﻿':
                i += 1
            elif src.startswith('//', i):
                j = src.find('\n', i)
                j = n if j < 0 else j
                comments.append((src[i:j], nl))
                i = j
            elif src.startswith('/*', i):
                j = src.find('*/', i + 2)
                if j < 0:
                    raise JavaError(f'{name}:{line}: unterminated comment')
                text = src[i:j + 2]
                comments.append((text, nl))
                line += text.count('\n')
                i = j + 2
            else:
                break
        if i >= n:
            toks.append(Tok('eof', '', line, comments, nl))
            return toks
        c = src[i]
        start_line = line
        if c == '"' or c == "'":
            j = i + 1
            while src[j] != c:
                if src[j] == '\\':
                    j += 1
                if src[j] == '\n':
                    raise JavaError(f'{name}:{line}: newline in literal')
                j += 1
            kind = 'str' if c == '"' else 'chr'
            toks.append(Tok(kind, src[i:j + 1], start_line, comments, nl))
            i = j + 1
        elif c.isdigit() or (c == '.' and i + 1 < n and src[i + 1].isdigit()):
            m = NUM_RE.match(src, i)
            toks.append(Tok('num', m.group(0), start_line, comments, nl))
            i = m.end()
        elif c.isalpha() or c in '_$':
            m = ID_RE.match(src, i)
            word = m.group(0)
            toks.append(Tok('kw' if word in KEYWORDS else 'id', word, start_line, comments, nl))
            i = m.end()
        else:
            for op in OPERATORS:
                if src.startswith(op, i):
                    toks.append(Tok('op', op, start_line, comments, nl))
                    i += len(op)
                    break
            else:
                raise JavaError(f'{name}:{line}: unexpected character {c!r}')
        comments = []
        nl = False


class Node:
    """An AST node: a kind and named attributes."""

    def __init__(self, kind, tok=None, **kw):
        self.kind = kind
        self.line = tok.line if tok is not None else 0
        self.comments = tok.comments if tok is not None else []
        self.__dict__.update(kw)

    def __repr__(self):
        attrs = ', '.join(f'{k}={v!r}' for k, v in self.__dict__.items()
                          if k not in ('kind', 'line', 'comments'))
        return f'{self.kind}({attrs})'


class Type:
    """A Java type: a base name, generic arguments and array dimensions."""

    def __init__(self, base, args=(), dims=0):
        self.base = base
        self.args = tuple(args)
        self.dims = dims

    def elem(self):
        return Type(self.base, self.args, self.dims - 1)

    def array(self, more=1):
        return Type(self.base, self.args, self.dims + more)

    def __eq__(self, o):
        return isinstance(o, Type) and (self.base, self.args, self.dims) == (o.base, o.args, o.dims)

    def __hash__(self):
        return hash((self.base, self.args, self.dims))

    def __str__(self):
        s = self.base
        if self.args:
            s += '<' + ','.join(str(a) for a in self.args) + '>'
        return s + '[]' * self.dims

    __repr__ = __str__


class Parser:
    def __init__(self, toks, name, skip_bodies=False):
        self.t = toks
        self.i = 0
        self.name = name
        self.class_names = set()
        self.skip_bodies = skip_bodies

    # ---- token helpers ----
    def peek(self, k=0):
        return self.t[min(self.i + k, len(self.t) - 1)]

    def at(self, text, k=0):
        tok = self.peek(k)
        return tok.text == text and tok.kind in ('op', 'kw')

    def next(self):
        tok = self.t[self.i]
        self.i += 1
        return tok

    def error(self, msg, tok=None):
        tok = tok or self.peek()
        raise JavaError(f'{self.name}:{tok.line}: {msg} (at {tok.text!r})')

    def expect(self, text):
        tok = self.next()
        if tok.text != text or tok.kind not in ('op', 'kw'):
            self.error(f'expected {text!r}', tok)
        return tok

    def accept(self, text):
        if self.at(text):
            return self.next()
        return None

    def ident(self):
        tok = self.next()
        if tok.kind != 'id':
            self.error('expected an identifier', tok)
        return tok.text

    # ---- compilation unit ----
    def unit(self):
        if self.accept('package'):
            self.qualified()
            self.expect(';')
        while self.accept('import'):
            self.accept('static')
            while not self.at(';'):
                self.next()
            self.expect(';')
        classes = []
        while self.peek().kind != 'eof':
            classes.append(self.class_decl())
        return classes

    def qualified(self):
        parts = [self.ident()]
        while self.at('.') and self.peek(1).kind == 'id':
            self.next()
            parts.append(self.ident())
        return '.'.join(parts)

    def modifiers(self):
        mods = set()
        while True:
            tok = self.peek()
            if tok.kind == 'kw' and tok.text in MODIFIERS:
                mods.add(self.next().text)
            elif self.at('@'):
                self.next()
                self.qualified()
                if self.at('('):
                    depth = 0
                    while True:
                        t = self.next()
                        depth += t.text == '('
                        depth -= t.text == ')'
                        if depth == 0:
                            break
            else:
                return mods

    def class_decl(self):
        start = self.peek()
        mods = self.modifiers()
        if not self.accept('class'):
            self.error('expected a class')
        name = self.ident()
        self.class_names.add(name)
        ext = None
        impl = []
        if self.accept('extends'):
            ext = self.type()
        if self.accept('implements'):
            impl.append(self.type())
            while self.accept(','):
                impl.append(self.type())
        self.expect('{')
        members = []
        while not self.accept('}'):
            m = self.member(name)
            if m is not None:
                members.append(m)
        return Node('Class', start, name=name, mods=mods, extends=ext, implements=impl,
                    members=members)

    def member(self, cname):
        start = self.peek()
        if self.accept(';'):
            return None
        mods = self.modifiers()
        if self.at('{'):
            return Node('Init', start, mods=mods, body=self.block())
        if self.at('class'):
            self.i -= 0
            c = self.class_decl()
            c.mods |= mods
            return c
        # constructor
        if self.peek().kind == 'id' and self.peek().text == cname and self.at('(', 1):
            self.next()
            params = self.params()
            throws = self.throws()
            body = self.skip_block() if self.skip_bodies else self.block()
            return Node('Ctor', start, mods=mods, name=cname, params=params, throws=throws,
                        body=body)
        typ = self.type()
        name_tok = self.peek()
        name = self.ident()
        if self.at('('):
            params = self.params()
            dims = 0
            while self.accept('['):
                self.expect(']')
                dims += 1
            if dims:
                typ = typ.array(dims)
            throws = self.throws()
            if self.accept(';'):
                body = None
            else:
                body = self.skip_block() if self.skip_bodies else self.block()
            return Node('Method', start, mods=mods, ret=typ, name=name, params=params,
                        throws=throws, body=body)
        decls = self.declarators(typ, name)
        self.expect(';')
        return Node('Field', start, mods=mods, decls=decls)

    def throws(self):
        names = []
        if self.accept('throws'):
            names.append(self.qualified())
            while self.accept(','):
                names.append(self.qualified())
        return names

    def params(self):
        self.expect('(')
        ps = []
        if not self.at(')'):
            while True:
                self.modifiers()
                typ = self.type()
                if self.accept('...'):
                    typ = typ.array()
                pname = self.ident()
                while self.accept('['):
                    self.expect(']')
                    typ = typ.array()
                ps.append((typ, pname))
                if not self.accept(','):
                    break
        self.expect(')')
        return ps

    def declarators(self, typ, first_name):
        """`name [= init] {, name [= init]}`, with C-style `name[]` dimensions."""
        decls = []
        name = first_name
        while True:
            t = typ
            while self.accept('['):
                self.expect(']')
                t = t.array()
            init = None
            if self.accept('='):
                init = self.array_init(t) if self.at('{') else self.expr()
            decls.append((t, name, init))
            if not self.accept(','):
                return decls
            name = self.ident()

    def array_init(self, typ):
        tok = self.expect('{')
        items = []
        while not self.at('}'):
            items.append(self.array_init(typ.elem()) if self.at('{') else self.expr())
            if not self.accept(','):
                break
        self.expect('}')
        return Node('ArrayInit', tok, type=typ, items=items)

    # ---- types ----
    def type(self):
        tok = self.peek()
        if tok.kind == 'kw' and tok.text in PRIMITIVES:
            base = self.next().text
            args = ()
        else:
            base = self.qualified()
            args = self.type_args() if self.at('<') else ()
        dims = 0
        while self.at('[') and self.at(']', 1):
            self.next()
            self.next()
            dims += 1
        return Type(base, args, dims)

    def type_args(self):
        self.expect('<')
        args = [self.type()]
        while self.accept(','):
            args.append(self.type())
        tok = self.peek()
        if tok.text == '>':
            self.next()
        elif tok.text in ('>>', '>>>'):
            # split the token: one '>' closes this list
            tok.text = tok.text[1:]
        else:
            self.error("expected '>'")
        return args

    def looks_like_type(self):
        """Whether a type starts here (for local variable declarations)."""
        save = self.i
        try:
            tok = self.peek()
            if tok.kind == 'kw' and tok.text in PRIMITIVES:
                self.type()
            elif tok.kind == 'id':
                self.type()
            else:
                return False
            return self.peek().kind == 'id' and (self.at('=', 1) or self.at(';', 1) or
                                                 self.at(',', 1) or self.at('[', 1) or
                                                 self.at(':', 1))
        except JavaError:
            return False
        finally:
            self.i = save

    def skip_block(self):
        tok = self.expect('{')
        depth = 1
        while depth:
            t = self.next()
            if t.kind == 'eof':
                self.error('unterminated block', tok)
            if t.kind == 'op':
                depth += (t.text == '{') - (t.text == '}')
        return Node('Block', tok, stmts=[], end_comments=[], skipped=True)

    # ---- statements ----
    def block(self):
        tok = self.expect('{')
        stmts = []
        while not self.at('}'):
            stmts.append(self.stmt())
        end = self.expect('}')
        return Node('Block', tok, stmts=stmts, end_comments=end.comments)

    def stmt(self):
        tok = self.peek()
        if self.at('{'):
            return self.block()
        if self.accept(';'):
            return Node('Empty', tok)
        if tok.kind == 'kw':
            k = tok.text
            if k == 'if':
                self.next()
                self.expect('(')
                cond = self.expr()
                self.expect(')')
                then = self.stmt()
                els = None
                if self.at('else'):
                    els_tok = self.next()
                    els = self.stmt()
                    els.else_comments = els_tok.comments
                return Node('If', tok, cond=cond, then=then, els=els)
            if k == 'for':
                return self.for_stmt()
            if k == 'while':
                self.next()
                self.expect('(')
                cond = self.expr()
                self.expect(')')
                return Node('While', tok, cond=cond, body=self.stmt())
            if k == 'do':
                self.next()
                body = self.stmt()
                self.expect('while')
                self.expect('(')
                cond = self.expr()
                self.expect(')')
                self.expect(';')
                return Node('DoWhile', tok, cond=cond, body=body)
            if k == 'switch':
                return self.switch_stmt()
            if k == 'break':
                self.next()
                label = self.ident() if self.peek().kind == 'id' else None
                self.expect(';')
                return Node('Break', tok, label=label)
            if k == 'continue':
                self.next()
                label = self.ident() if self.peek().kind == 'id' else None
                self.expect(';')
                return Node('Continue', tok, label=label)
            if k == 'return':
                self.next()
                e = None if self.at(';') else self.expr()
                self.expect(';')
                return Node('Return', tok, expr=e)
            if k == 'throw':
                self.next()
                e = self.expr()
                self.expect(';')
                return Node('Throw', tok, expr=e)
            if k == 'try':
                self.next()
                body = self.block()
                catches = []
                while self.accept('catch'):
                    self.expect('(')
                    self.modifiers()
                    ctypes = [self.type()]
                    while self.accept('|'):
                        ctypes.append(self.type())
                    cname = self.ident()
                    self.expect(')')
                    catches.append((ctypes, cname, self.block()))
                fin = self.block() if self.accept('finally') else None
                return Node('Try', tok, body=body, catches=catches, fin=fin)
            if k == 'final':
                self.next()
                return self.local_var(tok)
        if tok.kind == 'id' and self.at(':', 1):
            label = self.ident()
            self.next()
            return Node('Labeled', tok, label=label, body=self.stmt())
        if self.looks_like_type():
            return self.local_var(tok)
        e = self.expr()
        self.expect(';')
        return Node('ExprStmt', tok, expr=e)

    def local_var(self, tok):
        typ = self.type()
        name = self.ident()
        decls = self.declarators(typ, name)
        self.expect(';')
        return Node('Local', tok, decls=decls)

    def for_stmt(self):
        tok = self.expect('for')
        self.expect('(')
        if self.looks_like_type():
            save = self.i
            self.modifiers()
            typ = self.type()
            name = self.ident()
            if self.accept(':'):
                it = self.expr()
                self.expect(')')
                return Node('ForEach', tok, type=typ, name=name, iter=it, body=self.stmt())
            self.i = save
        init = []
        if not self.at(';'):
            if self.looks_like_type():
                init_tok = self.peek()
                typ = self.type()
                name = self.ident()
                init.append(Node('Local', init_tok, decls=self.declarators(typ, name)))
            else:
                init.append(Node('ExprStmt', self.peek(), expr=self.expr()))
                while self.accept(','):
                    init.append(Node('ExprStmt', self.peek(), expr=self.expr()))
        self.expect(';')
        cond = None if self.at(';') else self.expr()
        self.expect(';')
        update = []
        if not self.at(')'):
            update.append(self.expr())
            while self.accept(','):
                update.append(self.expr())
        self.expect(')')
        return Node('For', tok, init=init, cond=cond, update=update, body=self.stmt())

    def switch_stmt(self):
        tok = self.expect('switch')
        self.expect('(')
        e = self.expr()
        self.expect(')')
        self.expect('{')
        cases = []
        while not self.at('}'):
            ctok = self.peek()
            labels = []
            while self.at('case') or self.at('default'):
                if self.accept('default'):
                    labels.append(None)
                else:
                    self.next()
                    labels.append(self.expr())
                self.expect(':')
            if not labels:
                self.error('expected case')
            body = []
            while not (self.at('case') or self.at('default') or self.at('}')):
                body.append(self.stmt())
            cases.append(Node('Case', ctok, labels=labels, body=body))
        end = self.expect('}')
        return Node('Switch', tok, expr=e, cases=cases, end_comments=end.comments)

    # ---- expressions ----
    ASSIGN_OPS = {'=', '+=', '-=', '*=', '/=', '%=', '&=', '|=', '^=', '<<=', '>>=', '>>>='}
    BINARY = [
        ['||'], ['&&'], ['|'], ['^'], ['&'], ['==', '!='], ['<', '>', '<=', '>=', 'instanceof'],
        ['<<', '>>', '>>>'], ['+', '-'], ['*', '/', '%'],
    ]

    def expr(self):
        lhs = self.ternary()
        tok = self.peek()
        if tok.kind == 'op' and tok.text in self.ASSIGN_OPS:
            self.next()
            rhs = self.expr()
            return Node('Assign', tok, op=tok.text, target=lhs, value=rhs)
        return lhs

    def ternary(self):
        c = self.binary(0)
        if self.at('?'):
            tok = self.next()
            a = self.expr()
            self.expect(':')
            b = self.ternary()
            return Node('Cond', tok, cond=c, a=a, b=b)
        return c

    def binary(self, level):
        if level == len(self.BINARY):
            return self.unary()
        lhs = self.binary(level + 1)
        while True:
            tok = self.peek()
            if tok.kind in ('op', 'kw') and tok.text in self.BINARY[level]:
                self.next()
                if tok.text == 'instanceof':
                    lhs = Node('InstanceOf', tok, expr=lhs, type=self.type())
                else:
                    rhs = self.binary(level + 1)
                    lhs = Node('Binary', tok, op=tok.text, l=lhs, r=rhs)
            else:
                return lhs

    def cast_ahead(self):
        """At '(': is this a cast? Returns the type or None."""
        save = self.i
        try:
            self.next()
            tok = self.peek()
            prim = tok.kind == 'kw' and tok.text in PRIMITIVES
            if not prim and tok.kind != 'id':
                return None
            typ = self.type()
            if not self.at(')'):
                return None
            self.next()
            nxt = self.peek()
            if prim:
                ok = not (nxt.kind == 'op' and nxt.text in (')', ';', ',', '.', '='))
            else:
                ok = (nxt.kind in ('id', 'num', 'str', 'chr') or
                      (nxt.kind == 'kw' and nxt.text in ('this', 'new', 'true', 'false', 'null',
                                                          'super')) or
                      (nxt.kind == 'op' and nxt.text in ('(', '!', '~')))
                ok = ok and (typ.base[0].isupper() or typ.dims)
            return typ if ok else None
        except JavaError:
            return None
        finally:
            if 'typ' in locals() and locals().get('ok'):
                pass
            self.i = save

    def unary(self):
        tok = self.peek()
        if tok.kind == 'op':
            if tok.text in ('+', '-', '!', '~'):
                self.next()
                return Node('Unary', tok, op=tok.text, expr=self.unary())
            if tok.text in ('++', '--'):
                self.next()
                return Node('PreInc', tok, op=tok.text, expr=self.unary())
            if tok.text == '(':
                typ = self.cast_ahead()
                if typ is not None:
                    self.next()
                    self.type()
                    self.expect(')')
                    return Node('Cast', tok, type=typ, expr=self.unary())
        return self.postfix(self.primary())

    def postfix(self, e):
        while True:
            tok = self.peek()
            if self.at('.'):
                self.next()
                name_tok = self.peek()
                if name_tok.kind == 'kw' and name_tok.text in ('this', 'class', 'new'):
                    self.error('unsupported qualified expression')
                name = self.ident()
                if self.at('('):
                    e = Node('Call', name_tok, obj=e, name=name, args=self.args())
                else:
                    e = Node('Field', name_tok, obj=e, name=name)
            elif self.at('['):
                self.next()
                idx = self.expr()
                self.expect(']')
                e = Node('Index', tok, arr=e, idx=idx)
            elif tok.kind == 'op' and tok.text in ('++', '--'):
                self.next()
                e = Node('PostInc', tok, op=tok.text, expr=e)
            else:
                return e

    def args(self):
        self.expect('(')
        a = []
        if not self.at(')'):
            a.append(self.expr())
            while self.accept(','):
                a.append(self.expr())
        self.expect(')')
        return a

    def primary(self):
        tok = self.next()
        k, t = tok.kind, tok.text
        if k == 'num':
            return Node('Lit', tok, type=num_type(t), text=t)
        if k == 'str':
            return Node('Lit', tok, type=Type('String'), text=t)
        if k == 'chr':
            return Node('Lit', tok, type=Type('char'), text=t)
        if k == 'kw':
            if t in ('true', 'false'):
                return Node('Lit', tok, type=Type('boolean'), text=t)
            if t == 'null':
                return Node('Lit', tok, type=Type('null'), text=t)
            if t == 'this':
                if self.at('('):
                    return Node('Call', tok, obj=None, name='this', args=self.args())
                return Node('This', tok)
            if t == 'super':
                if self.at('('):
                    return Node('Call', tok, obj=None, name='super', args=self.args())
                self.expect('.')
                name = self.ident()
                return Node('Call', tok, obj=Node('Super', tok), name=name, args=self.args())
            if t == 'new':
                typ_tok = self.peek()
                if typ_tok.kind == 'kw' and typ_tok.text in PRIMITIVES:
                    base = self.next().text
                    targs = ()
                else:
                    base = self.qualified()
                    targs = self.type_args() if self.at('<') else ()
                if self.at('['):
                    dims = []
                    extra = 0
                    while self.accept('['):
                        if self.accept(']'):
                            extra += 1
                        else:
                            dims.append(self.expr())
                            self.expect(']')
                    typ = Type(base, targs, len(dims) + extra)
                    init = self.array_init(typ) if self.at('{') else None
                    return Node('NewArray', tok, type=typ, dims=dims, init=init)
                args = self.args()
                if self.at('{'):
                    self.error('anonymous classes are not supported')
                return Node('New', tok, type=Type(base, targs), args=args)
            if t in PRIMITIVES and self.at('.'):
                self.error('class literals are not supported')
        if k == 'op' and t == '(':
            e = self.expr()
            self.expect(')')
            return Node('Paren', tok, expr=e)
        if k == 'id':
            if self.at('('):
                return Node('Call', tok, obj=None, name=t, args=self.args())
            return Node('Name', tok, name=t)
        self.error('unexpected token', tok)


def num_type(text):
    t = text.lower()
    if t.startswith('0x'):
        return Type('long') if t.endswith('l') else Type('int')
    if t.endswith('l'):
        return Type('long')
    if t.endswith('f'):
        return Type('float')
    if t.endswith('d') or '.' in t or 'e' in t:
        return Type('double')
    return Type('int')


def parse_file(path, skip_bodies=False):
    with open(path, encoding='utf-8', errors='replace') as f:
        src = f.read()
    toks = lex(src, path)
    return Parser(toks, path, skip_bodies).unit()


if __name__ == '__main__':
    import sys
    for p in sys.argv[1:]:
        classes = parse_file(p)
        for c in classes:
            n_m = sum(1 for m in c.members if m.kind in ('Method', 'Ctor'))
            n_f = sum(len(m.decls) for m in c.members if m.kind == 'Field')
            print(f'{p}: class {c.name}: {n_f} fields, {n_m} methods/constructors')
