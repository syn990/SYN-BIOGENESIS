#!/usr/bin/env python3
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""Translates HotSpot's x86_64 Math.sin/cos/pow stubs into GNU assembler.

On x86_64 the JVM does not compute Math.sin, Math.cos and Math.pow in Java:
it runs Intel LIBM routines that HotSpot writes out instruction by
instruction through its MacroAssembler (stubGenerator_x86_64_{sin,cos,pow}
.cpp, tables in stubGenerator_x86_64_constants.cpp). Their results differ
from fdlibm and from the C library in the last bit for a few percent of
inputs, and Biogenesis calls them every frame, so the port runs the same
instructions: each `__ op(args);` becomes one line of assembly.

    gen_libm.py HOTSPOT_DIR > src/sim/jvm_libm_x86_64.S

Anything not understood stops the translation with an error.
"""

import re
import sys
from pathlib import Path

STUBS = [("sin", "generate_libmSin", "jvm_dsin"),
         ("cos", "generate_libmCos", "jvm_dcos"),
         ("pow", "generate_libmPow", "jvm_dpow")]

GPR64 = ["rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rsp", "rbp"] + [f"r{i}" for i in range(8, 16)]
GPR32 = {**{r: "e" + r[1:] for r in GPR64[:8]}, **{f"r{i}": f"r{i}d" for i in range(8, 16)}}
GPR16 = {**{r: r[1:] for r in GPR64[:8]}, **{f"r{i}": f"r{i}w" for i in range(8, 16)}}

CONDITIONS = {"above": "ja", "aboveEqual": "jae", "below": "jb", "belowEqual": "jbe",
              "equal": "je", "notEqual": "jne", "greater": "jg", "greaterEqual": "jge",
              "less": "jl", "lessEqual": "jle", "negative": "js", "positive": "jns",
              "zero": "je", "notZero": "jne", "carrySet": "jc", "carryClear": "jnc"}

# MacroAssembler name -> (mnemonic, GPR width, memory operand size)
OPS = {
    "movl": ("mov", 32, "dword"), "addl": ("add", 32, "dword"), "subl": ("sub", 32, "dword"),
    "andl": ("and", 32, "dword"), "orl": ("or", 32, "dword"), "xorl": ("xor", 32, "dword"),
    "cmpl": ("cmp", 32, "dword"), "testl": ("test", 32, "dword"), "shll": ("shl", 32, None),
    "shrl": ("shr", 32, None), "sarl": ("sar", 32, None), "negl": ("neg", 32, None),
    "bsrl": ("bsr", 32, None),
    "addq": ("add", 64, "qword"), "subq": ("sub", 64, "qword"), "andq": ("and", 64, "qword"),
    "orq": ("or", 64, "qword"), "cmpq": ("cmp", 64, "qword"), "shlq": ("shl", 64, None),
    "shrq": ("shr", 64, None), "imulq": ("imul", 64, None), "sbbq": ("sbb", 64, None),
    "bsrq": ("bsr", 64, None), "cmp64": ("cmp", 64, "qword"), "lea": ("lea", 64, None),
    "movw": ("mov", 16, "word"),
    "movdq": ("movq", 64, None), "movdl": ("movd", 32, "dword"),
    "cvttsd2sil": ("cvttsd2si", 32, None), "cvttsd2siq": ("cvttsd2si", 64, None),
    "cvtsi2sdl": ("cvtsi2sd", 32, None), "cvtsi2sdq": ("cvtsi2sd", 64, None),
    "pextrw": ("pextrw", 32, None), "pinsrw": ("pinsrw", 32, "word"),
    "movsd": ("movsd", None, "qword"), "addsd": ("addsd", None, "qword"),
    "subsd": ("subsd", None, "qword"), "mulsd": ("mulsd", None, "qword"),
    "divsd": ("divsd", None, "qword"), "sqrtsd": ("sqrtsd", None, "qword"),
    "mulss": ("mulss", None, "dword"),
    "movdqu": ("movdqu", None, "xmmword"), "movdqa": ("movdqa", None, "xmmword"),
    "movapd": ("movapd", None, "xmmword"), "addpd": ("addpd", None, "xmmword"),
    "subpd": ("subpd", None, "xmmword"), "mulpd": ("mulpd", None, "xmmword"),
    "andpd": ("andpd", None, "xmmword"), "xorpd": ("xorpd", None, "xmmword"),
    "orpd": ("orpd", None, "xmmword"), "pand": ("pand", None, "xmmword"),
    "por": ("por", None, "xmmword"), "paddd": ("paddd", None, "xmmword"),
    "paddq": ("paddq", None, "xmmword"), "psrlq": ("psrlq", None, None),
    "psllq": ("psllq", None, None), "psrld": ("psrld", None, None),
    "pshufd": ("pshufd", None, "xmmword"), "unpcklpd": ("unpcklpd", None, "xmmword"),
    "unpckhpd": ("unpckhpd", None, "xmmword"), "movlhps": ("movlhps", None, None),
    "movddup": ("movddup", None, "qword"), "rcpps": ("rcpps", None, "xmmword"),
}


def fail(msg):
    sys.exit(f"gen_libm: {msg}")


def split_args(s):
    out, depth, cur = [], 0, ""
    for ch in s:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


def parse_tables(text):
    tables = {}
    for m in re.finditer(r"ATTRIBUTE_ALIGNED\((\d+)\)\s+static const juint (\w+)\[\]\s*=\s*\{(.*?)\};",
                         text, re.S):
        body = re.sub(r"//[^\n]*", "", m.group(3))
        words = [w.strip().rstrip("UL").rstrip("U") for w in body.split(",") if w.strip()]
        for w in words:
            if not re.fullmatch(r"0x[0-9a-fA-F]{1,8}", w):
                fail(f"table {m.group(2)}: unexpected value {w!r}")
        tables[m.group(2)] = (int(m.group(1)), words)
    return tables


class Stub:
    def __init__(self, short, func, symbol, text, tables):
        self.short, self.symbol, self.tables = short, symbol, tables
        self.aliases = {}
        self.lines = []
        m = re.search(r"address StubGenerator::%s\(\) \{(.*?)\n\}" % func, text, re.S)
        if not m:
            fail(f"{func} not found")
        self.body = m.group(1).split("\n")

    def table(self, name):
        name = self.aliases.get(name, "_" + name)
        if name not in self.tables:
            fail(f"{self.short}: unknown table {name}")
        return name

    def operand(self, a, width, memsize):
        a = a.strip()
        if re.fullmatch(r"xmm\d+", a):
            return a
        if a in GPR64:
            return {64: a, 32: GPR32[a], 16: GPR16[a], None: a}[width]
        if a == "INT_MIN":
            return "-2147483648"
        if re.fullmatch(r"-?(0x[0-9a-fA-F]+|\d+)", a):
            return a
        ptr = f"{memsize} ptr " if memsize else ""
        m = re.fullmatch(r"ExternalAddress\((\w+)\s*(?:\+\s*(\d+))?\)", a)
        if m:
            off = f" + {m.group(2)}" if m.group(2) and m.group(2) != "0" else ""
            return f"{ptr}[rip + {self.table(m.group(1))}{off}]"
        m = re.fullmatch(r"Address\((\w+),\s*(-?\d+)\)", a)
        if m and m.group(1) in GPR64:
            return f"{ptr}[{m.group(1)} + {m.group(2)}]".replace("+ -", "- ")
        m = re.fullmatch(r"Address\((\w+),\s*(\w+),\s*Address::times_(\d),\s*(-?\d+)\)", a)
        if m and m.group(1) in GPR64 and m.group(2) in GPR64:
            return f"{ptr}[{m.group(1)} + {m.group(2)}*{m.group(3)} + {m.group(4)}]".replace("+ -", "- ")
        fail(f"{self.short}: operand {a!r}")

    def label(self, name):
        return f".L{self.short}_{name}"

    def emit(self, s):
        self.lines.append("\t" + s)

    def instruction(self, op, args):
        if op == "bind":
            self.lines.append(self.label(args[0]) + ":")
        elif op in ("jcc", "jccb"):
            cond = args[0].replace("Assembler::", "")
            if cond not in CONDITIONS:
                fail(f"{self.short}: condition {cond}")
            self.emit(f"{CONDITIONS[cond]} {self.label(args[1])}")
        elif op == "jmp":
            self.emit(f"jmp {self.label(args[0])}")
        elif op == "enter":
            self.emit("push rbp")
            self.emit("mov rbp, rsp")
        elif op == "leave":
            self.emit("leave")
        elif op == "ret":
            self.emit("ret")
        elif op in ("push_ppx", "pop_ppx"):
            self.emit(f"{op[:-4]} {args[0]}")
        elif op == "mov64":
            self.emit(f"movabs {args[0]}, {args[1]}")
        elif op == "movq":
            # GPR to GPR is mov; anything with an XMM register is the SSE movq
            if any(re.fullmatch(r"xmm\d+", x) for x in args):
                self.emit("movq " + ", ".join(self.operand(x, 64, "qword") for x in args))
            else:
                self.emit("mov " + ", ".join(self.operand(x, 64, "qword") for x in args))
        elif op in OPS:
            mnem, width, memsize = OPS[op]
            if op in ("shll", "shrl", "sarl", "shlq", "shrq") and len(args) == 1:
                args = args + ["cl"]
            ops = [x if x == "cl" else self.operand(x, width, memsize) for x in args]
            self.emit(f"{mnem} " + ", ".join(ops))
        else:
            fail(f"{self.short}: instruction {op}")

    def translate(self):
        started = skip = False
        branch = None
        for raw in self.body:
            line = re.sub(r"/\*.*?\*/", "", raw)
            line = re.sub(r"//.*", "", line).strip()
            if not started:
                started = line == "start = __ pc();"
                continue
            if not line:
                continue
            if line.startswith("store_archive_data"):
                break
            if line == "#ifdef _WIN64":
                skip = True
                continue
            if line == "#endif":
                skip = False
                continue
            if skip:
                continue
            # The CPU this targets (and any x86_64 with AVX) has SSE3; the
            # two branches compute the same value anyway.
            if line == "if (VM_Version::supports_sse3()) {":
                branch = "then"
                continue
            if line == "} else {" and branch:
                branch = "else"
                continue
            if line == "}" and branch:
                branch = None
                continue
            if branch == "else":
                continue
            if line.startswith("Label ") or re.fullmatch(r"[\w\s,]+[;,]", line):
                continue
            m = re.fullmatch(r"address (\w+)\s*=\s*\(address\)(\w+);", line)
            if m:
                self.aliases[m.group(1)] = m.group(2)
                continue
            m = re.fullmatch(r"__ (\w+)\((.*)\);", line)
            if not m:
                fail(f"{self.short}: cannot translate {raw.strip()!r}")
            op, args = m.group(1), split_args(m.group(2))
            # the scratch register after an ExternalAddress is only for
            # out-of-range addresses; RIP-relative needs none
            ext = [i for i, x in enumerate(args) if x.startswith("ExternalAddress")]
            if ext and op != "lea" and len(args) > ext[0] + 1 and args[-1] in GPR64:
                args = args[:-1]
            self.instruction(op, args)
        if not started:
            fail(f"{self.short}: start not found")


def main():
    src = Path(sys.argv[1])
    constants = (src / "stubGenerator_x86_64_constants.cpp").read_text()
    texts = {s: (src / f"stubGenerator_x86_64_{s}.cpp").read_text() for s, _, _ in STUBS}
    tables = parse_tables(constants)
    for s, t in texts.items():
        for name, v in parse_tables(t).items():
            if name in tables:
                fail(f"table {name} defined twice")
            tables[name] = v

    print("/* Generated by tools/jvmlibm/gen_libm.py from OpenJDK HotSpot (jdk-27-ga):")
    print(" * src/hotspot/cpu/x86/stubGenerator_x86_64_{sin,cos,pow,constants}.cpp.")
    print(" * Do not edit; regenerate. The notices of those files follow.")
    print(" *")
    for f in ["sin", "cos", "pow", "constants"]:
        text = constants if f == "constants" else texts[f]
        head = re.match(r"/\*(.*?)\*/", text, re.S).group(1)
        for l in head.strip("\n").split("\n"):
            l = re.sub(r"^\s?\*\s?", "", l)
            print((" * " + l).rstrip())
        print(" *")
    print(" */")
    print()
    print("#if defined(__x86_64__) && defined(__ELF__)")
    print("\t.intel_syntax noprefix")
    print()
    used = set()
    stubs = []
    for short, func, symbol in STUBS:
        st = Stub(short, func, symbol, texts[short], tables)
        st.translate()
        stubs.append(st)
        for l in st.lines:
            used.update(re.findall(r"\b(_\w+)\b", l))
    print("\t.section .rodata")
    for name in sorted(n for n in tables if n in used):
        align, words = tables[name]
        print(f"\t.balign {align}")
        print(f"{name}:")
        for i in range(0, len(words), 4):
            print("\t.long " + ", ".join(words[i:i + 4]))
    print()
    print("\t.text")
    for st in stubs:
        print()
        print(f"\t.globl {st.symbol}")
        print(f"\t.type {st.symbol}, @function")
        print("\t.balign 32")
        print(f"{st.symbol}:")
        for l in st.lines:
            print(l)
        print(f"\t.size {st.symbol}, . - {st.symbol}")
    print()
    print("\t.section .note.GNU-stack,\"\",@progbits")
    print("#endif")


if __name__ == "__main__":
    main()
