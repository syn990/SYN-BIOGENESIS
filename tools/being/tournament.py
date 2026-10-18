#!/usr/bin/env python3
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""The being search as a Graphviz graph, in the SYN-OS theme
(docs/being/tournament.png and .dot): where the candidates came from, the
sixteen that passed screening, every bred being that is an ancestor of a
finalist (with mutation and crossover edges from its parents), the rest
of each generation as a count, the finalists and the champion, whose line
of descent is drawn in the theme's accent colour. Run pictures.py first:
the nodes show its thumbnails.

    tournament.py
"""
import json
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from search import BUILD, OUT, PHASE1_GENS, PHASE2_GENS  # noqa: E402
from theme import theme  # noqa: E402

KINDS = {"natural": "natural champions (3,000,000-frame evolutions)",
         "world": "winners of the training worlds", "design": "designs", "random": "random codes"}
LATE = {"yours": "your world's {n} commonest species", "design": "{n} armoured designs"}


def esc(s):
    return str(s).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace('"', "&quot;")


def best_stage(c):
    for st, name in (("final", "your world"), ("world", "phase 2"), ("held", "held-out"),
                     ("train", "training"), ("screen", "screening")):
        if st in c["scores"]:
            return st, name
    return None, ""


def main():
    t = theme()
    data = json.loads((OUT / "tournament.json").read_text())
    cands = {c["id"]: c for c in data["candidates"]}
    champ = data.get("champion")
    finalists = {i for i, c in cands.items() if "final" in c["scores"] or "held" in c["scores"]}

    def ancestors(ids):
        seen, todo = set(), list(ids)
        while todo:
            i = todo.pop()
            if i in seen:
                continue
            seen.add(i)
            todo += cands[i]["parents"]
        return seen

    line = ancestors([champ]) if champ is not None else set()
    top16 = {i for i, c in cands.items() if c["kind"] != "bred" and "train" in c["scores"]}
    shown = top16 | ancestors(finalists) | finalists

    bg, panel, text, border = t["SYN_BG"], t["SYN_PANEL"], t["SYN_TEXT"], t["SYN_BORDER"]
    accent, dim = t["SYN_ACCENT"], t["SYN_ACCENT_DIM"]
    g = [f'digraph tournament {{',
         f'  graph [rankdir=LR, bgcolor="{bg}", fontcolor="{text}", fontname="sans", fontsize=22, '
         f'nodesep=0.12, ranksep=0.9, pad=0.4, labelloc=t, '
         f'label="The being search  ·  {esc(t["SYN_THEME_NAME"])}\\n\\n"];',
         f'  node [shape=plain, fontname="sans", fontcolor="{text}", fontsize=11];',
         f'  edge [color="{border}", arrowsize=0.5, penwidth=0.8];']

    def node(c, rank_note=""):
        st, stname = best_stage(c)
        s = c["scores"].get(st, {})
        score = f'{s["mean"]:.1f} lineage · {s["success"]:.0%} alive ({stname})' if s else ""
        if st in ("world", "final"):
            score = f'{s["mean"]:.1f} lineage · {s["success"]:.0%} survived ({stname})'
        is_champ = c["id"] == champ
        frame = accent if (is_champ or c["id"] in line) else (dim if c["id"] in finalists else border)
        width = 3 if is_champ else (2 if c["id"] in finalists or c["id"] in line else 1)
        img = BUILD / "png" / f'{c["id"]}.png'
        pic = f'<TR><TD FIXEDSIZE="TRUE" WIDTH="72" HEIGHT="72"><IMG SRC="{img}"/></TD></TR>' if img.exists() else ""
        title = "CHAMPION  " if is_champ else ("finalist  " if c["id"] in finalists else "")
        origin = c["origin"] if c["kind"] != "bred" else c["op"]
        g.append(f'  n{c["id"]} [label=<<TABLE BORDER="{width}" COLOR="{frame}" BGCOLOR="{panel}" '
                 f'CELLBORDER="0" CELLPADDING="3" CELLSPACING="0">{pic}'
                 f'<TR><TD><B>{esc(title)}#{c["id"]}</B></TD></TR>'
                 f'<TR><TD><FONT POINT-SIZE="9">{esc(origin)}</FONT></TD></TR>'
                 f'<TR><TD><FONT POINT-SIZE="9" COLOR="{border}">{esc(score)}</FONT></TD></TR></TABLE>>];')

    def count(name, n, note):
        g.append(f'  {name} [shape=box, style="rounded,dashed", color="{border}", fontcolor="{border}", '
                 f'label="{esc(note)}"];')

    # sources
    g.append('  subgraph cluster_sources { label="candidates"; fontsize=14; color="' + border + '"; fontcolor="' + text + '";')
    for k, name in KINDS.items():
        n = sum(1 for c in cands.values() if c["kind"] == k and c["phase"] == 1)
        g.append(f'    src_{k} [shape=box, style="rounded,filled", fillcolor="{panel}", color="{border}", '
                 f'label="{n} {esc(name)}"];')
    g.append("  }")
    out_n = sum(1 for c in cands.values() if c["kind"] != "bred" and c["phase"] == 1 and "train" not in c["scores"])
    count("screened_out", out_n, f"{out_n} screened out")
    for k in KINDS:
        g.append(f'  src_{k} -> screened_out [style=dashed];')

    # the sixteen
    g.append('  subgraph cluster_top { label="passed screening"; fontsize=14; color="' + border + '"; fontcolor="' + text + '";')
    for i in sorted(top16, key=lambda i: -cands[i]["scores"]["screen"]["mean"]):
        node(cands[i])
    g.append("  }")
    for i in top16:
        g.append(f'  src_{cands[i]["kind"]} -> n{i} [color="{accent if i in line else border}"];')

    # generations, kept in order left to right by an invisible chain
    anchor = "screened_out"
    for phase, gens in ((1, PHASE1_GENS), (2, PHASE2_GENS)):
        late = [c for c in cands.values() if c["kind"] != "bred" and c["phase"] == 2]
        if phase == 2 and late:
            # candidates that join in the chosen world
            g.append('  subgraph cluster_late { label="joining in your world"; fontsize=14; color="' + border
                     + '"; fontcolor="' + text + '";')
            for k, name in LATE.items():
                n = sum(1 for c in late if c["kind"] == k)
                if n:
                    g.append(f'    late_{k} [shape=box, style="rounded,filled", fillcolor="{panel}", '
                             f'color="{border}", label="{esc(name.format(n=n))}"];')
            for c in sorted(late, key=lambda c: c["id"]):
                if c["id"] in shown:
                    node(c)
            g.append("  }")
            first = f'late_{late[0]["kind"]}'
            g.append(f"  {anchor} -> {first} [style=invis, weight=10];")
            anchor = first
            for c in late:
                if c["id"] in shown:
                    g.append(f'  late_{c["kind"]} -> n{c["id"]} [color="{accent if c["id"] in line else border}"];')
        for gen in range(1, gens + 1):
            born = [c for c in cands.values() if c["kind"] == "bred" and c["phase"] == phase and c["gen"] == gen]
            if not born:
                continue
            here = [c for c in born if c["id"] in shown]
            st = "train" if phase == 1 else "world"
            top = max((c["scores"][st]["mean"] for c in born if st in c["scores"]), default=0)
            label = (f"phase {phase}, generation {gen}" + (" (in your world)" if phase == 2 else "")
                     + f"\\nbest child {top:.1f}")
            g.append(f'  subgraph cluster_p{phase}g{gen} {{ label="{label}"; fontsize=14; color="{border}"; fontcolor="{text}";')
            for c in sorted(here, key=lambda c: c["id"]):
                node(c)
            rest = len(born) - len(here)
            first = f"rest_p{phase}g{gen}" if rest else f'n{min(c["id"] for c in here)}'
            if rest:
                count(f"rest_p{phase}g{gen}", rest, f"{rest} more bred")
            g.append("  }")
            g.append(f"  {anchor} -> {first} [style=invis, weight=10];")
            anchor = first
            for c in here:
                for p in c["parents"]:
                    if p in shown:
                        on = c["id"] in line and p in line
                        style = "solid" if c["op"] == "mutation" else "dashed"
                        g.append(f'  n{p} -> n{c["id"]} [style={style}, color="{accent if on else border}", '
                                 f'penwidth={2.2 if on else 0.8}];')
    if champ is not None:
        g.append(f'  champion [shape=doubleoctagon, style=filled, fillcolor="{accent}", color="{accent}", '
                 f'fontcolor="{text}", fontsize=16, label="the champion\\n#{champ}"];')
        g.append(f'  n{champ} -> champion [color="{accent}", penwidth=3];')
    g.append("}")
    dot = "\n".join(g) + "\n"
    (OUT / "tournament.dot").write_text(dot.replace(str(BUILD / "png") + "/", ""))
    subprocess.run(["dot", "-Tpng", "-Gdpi=96", "-o", str(OUT / "tournament.png")], input=dot,
                   text=True, check=True)


if __name__ == "__main__":
    main()
