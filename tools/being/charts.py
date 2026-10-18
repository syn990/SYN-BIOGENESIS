#!/usr/bin/env python3
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""Charts of the being search, light and dark (docs/being/*.svg), drawn
with tools/drift/charts.py:

  progress    the best and the median child of each generation
  finalists   the last round in the chosen world, with the best natural
              champion and design for comparison
  growth      the champion's lineage after one import
  contenders  every run of the leading contenders (training and final)
  battles     head-to-head matrix, standings and free-for-all (battles.py)

    charts.py
"""
import csv
import json
import statistics
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / "drift"))
import charts as base  # noqa: E402  (tools/drift/charts.py)
from search import FINAL_FRAMES, FINAL_SEEDS, OUT, PHASE1_GENS, PHASE2_FRAMES, PHASE2_GENS, PHASE2_SEEDS  # noqa: E402

base.OUT = OUT


def nice_ticks(v, n=4):
    return base.nice_ticks(max(v, 1), n)


def progress(theme, data):
    cands = data["candidates"]
    h = 620
    c = base.Chart(theme, "How the search improved, generation by generation",
                   "Mean living lineage per import. Lines: the best candidate so far; dots: the median "
                   "child of each generation", height=h)
    c.legend([(0, "best so far", False)])
    x = 24 + 24 + 7.1 * len("best so far") + 22
    c.parts.append(f'<circle cx="{x + 9}" cy="74" r="4" fill="{c.t["series"][2]}" '
                   f'stroke="{c.t["surface"]}" stroke-width="2"/>')
    c.text(x + 24, 78, "median child of the generation", 12, "ink2")
    panels = [(1, "train", PHASE1_GENS, "Phase 1: 16 training worlds, 2 imports each, 30,000 frames", 96, 300),
              (2, "world", PHASE2_GENS, f'Phase 2: inside {data.get("world") or "the chosen world"}, '
               f'{len(PHASE2_SEEDS)} imports, {PHASE2_FRAMES:,} frames', 380, h - base.B)]
    for phase, st, gens, title, top, bottom in panels:
        if not any(x.get("phase") == phase and x["kind"] == "bred" for x in cands):
            continue
        start = [x for x in cands if st in x["scores"] and not (x["kind"] == "bred" and x["phase"] == phase)]
        best, med = [], []
        cur = max((x["scores"][st]["mean"] for x in start), default=0)
        best.append((0, cur))
        for gen in range(1, gens + 1):
            kids = [x["scores"][st]["mean"] for x in cands
                    if x["kind"] == "bred" and x["phase"] == phase and x["gen"] == gen and st in x["scores"]]
            if not kids:
                continue
            cur = max(cur, max(kids))
            best.append((gen, cur))
            med.append((gen, statistics.median(kids)))
        yt = nice_ticks(max(v for _, v in best) * 1.05)
        c.plot(0, gens, 0, yt[-1], top + 16, bottom)
        c.text(base.L, top + 6, title, 12, "ink2")
        c.axes(range(0, gens + 1), yt, "generation" if phase == 2 else "", "lineage per import")
        c.line(best, 0, step=True)
        for x, y in med:
            c.dot(x, y, 2)
        c.text(c.sx(best[-1][0]) - 4, c.sy(best[-1][1]) - 8, f"{best[-1][1]:.1f}", 11, "ink", "end")
    c.save(OUT / f"progress-{theme}.svg")


def finalists(theme, data):
    cands = {x["id"]: x for x in data["candidates"]}
    st = "final" if any("final" in x["scores"] for x in cands.values()) else "held"
    rows = sorted((x for x in cands.values() if st in x["scores"]), key=lambda x: -x["scores"][st].get("score", x["scores"][st]["mean"]))
    if not rows:
        return
    h = 136 + 34 * len(rows)
    where = (f'in {data.get("world")}, {len(FINAL_SEEDS)} imports, {FINAL_FRAMES:,} frames' if st == "final"
             else "on 10 held-out worlds, 6 imports each, 50,000 frames")
    c = base.Chart(theme, "The last round", f"Mean living lineage per import {where}", height=h)
    top = max(x["scores"][st]["mean"] for x in rows)
    xt = nice_ticks(top * 1.05)
    c.plot(0, xt[-1] * 1.4, 0, 1, 96, h - base.B)  # the right part for the text
    for v in xt:
        X = c.sx(v)
        c.parts.append(f'<line x1="{X:.1f}" y1="96" x2="{X:.1f}" y2="{h - base.B}" '
                       f'stroke="{c.t["grid"]}" stroke-width="1"/>')
        c.text(X, h - base.B + 16, base.compact(v) if v >= 1 else f"{v:g}", 11, "muted", "middle")
    for i, x in enumerate(rows):
        s = x["scores"][st]
        y = 108 + 34 * i
        name = f'#{x["id"]}'
        kind = {"natural": "natural champion", "world": "world winner", "design": x["origin"],
                "random": "random", "bred": "bred", "yours": f'{x["origin"]} species'}[x["kind"]]
        c.text(base.L - 8, y + 13, name, 12, "ink", "end", weight="600" if x["id"] == data.get("champion") else None)
        w = max(1.0, c.sx(s["mean"]) - base.L)
        c.parts.append(f'<path d="M{base.L},{y + 4} h{w - 4:.1f} a4,4 0 0 1 4,4 v10 a4,4 0 0 1 -4,4 '
                       f'h{-(w - 4):.1f} z" fill="{c.t["series"][0 if x["kind"] == "bred" else 2]}"/>')
        c.text(base.L + w + 6, y + 17, ("champion  ·  " if x["id"] == data.get("champion") else "")
               + f'{s["mean"]:.1f}  ·  {s["success"]:.0%} survived  ·  {kind}',
               11, "ink2")
    c.legend([(0, "bred in the search", False), (2, "for comparison", False)])
    c.save(OUT / f"finalists-{theme}.svg")


def growth(theme, data):
    path = OUT / "growth.csv"
    if not path.exists():
        return
    series = {}
    for r in csv.DictReader(open(path)):
        series.setdefault(int(r["candidate"]), {}).setdefault(int(r["frame"]), []).append(int(r["lineage"]))
    champ = data.get("champion")
    order = ([champ] + [i for i in series if i != champ])[:4]
    cands = {x["id"]: x for x in data["candidates"]}

    def label(i):
        x = cands[i]
        return "the champion" if i == champ else {"natural": "best natural champion", "world": "best world winner",
                                                    "design": f'best design ({x["origin"]})',
                                                    "yours": "best of your world's species"}.get(x["kind"], f"#{i}")

    slots = [0 if i == champ else KIND_SLOT[cands[i]["kind"]] for i in order]

    q = lambda v, p: sorted(v)[min(len(v) - 1, int(p * len(v)))]
    top = max(q(v, 0.75) for s in series.values() for v in s.values())
    c = base.Chart(theme, f'After one import into {data.get("world") or "the world"}',
                   f"Living lineage over {FINAL_FRAMES:,} frames from {len(FINAL_SEEDS)} import spots: "
                   "the median, and the middle half shaded")
    c.legend([(slots[k], label(i), False) for k, i in enumerate(order)])
    yt = nice_ticks(top * 1.05)
    c.plot(0, FINAL_FRAMES, 0, yt[-1], base.T, base.H - base.B)
    c.axes(nice_ticks(FINAL_FRAMES), yt, "frames after the import", "organisms in its lineage")
    for k, i in enumerate(order):
        fr = sorted(series[i])
        lo = [(f, q(series[i][f], 0.25)) for f in fr]
        hi = [(f, q(series[i][f], 0.75)) for f in fr]
        pts = " ".join(f"{c.sx(f):.1f},{c.sy(v):.1f}" for f, v in hi + lo[::-1])
        c.parts.append(f'<polygon points="{pts}" fill="{c.t["series"][slots[k]]}" fill-opacity="0.12"/>')
        med = [(f, statistics.median(series[i][f])) for f in fr]
        c.line(med, slots[k])
        c.text(c.sx(FINAL_FRAMES) - 4, c.sy(med[-1][1]) - 8, f"{med[-1][1]:g}", 11, "ink", "end")
    c.save(OUT / f"growth-{theme}.svg")


KIND_SLOT = {"bred": 0, "yours": 1, "natural": 2, "world": 2, "design": 3, "random": 4}
KIND_NAME = {"bred": "bred in the search", "yours": "your world's species", "natural": "natural champion",
             "world": "natural champion", "design": "design", "random": "random"}


def strip(theme, data, stage, n, title, subtitle, name):
    """every run of the top n candidates of a stage as a dot, the mean as a tick"""
    rows = sorted((x for x in data["candidates"] if stage in x["scores"]),
                  key=lambda x: -x["scores"][stage].get("score", x["scores"][stage]["mean"]))[:n]
    if not rows:
        return
    h = 130 + 30 * len(rows)
    c = base.Chart(theme, title, subtitle, height=h)
    kinds = []
    for x in rows:
        if KIND_SLOT[x["kind"]] not in [k for k, _ in kinds]:
            kinds.append((KIND_SLOT[x["kind"]], KIND_NAME[x["kind"]]))
    for i, (slot, label) in enumerate(kinds):
        X = 24 + i * 190
        c.parts.append(f'<circle cx="{X + 9}" cy="74" r="4" fill="{c.t["series"][slot]}"/>')
        c.text(X + 22, 78, label, 12, "ink2")
    top = max(r[0] for x in rows for r in x["scores"][stage]["runs"])
    xt = nice_ticks(top * 1.05)
    c.plot(0, xt[-1] * 1.3, 0, 1, 96, h - base.B)  # the right part for the text
    for v in xt:
        X = c.sx(v)
        c.parts.append(f'<line x1="{X:.1f}" y1="96" x2="{X:.1f}" y2="{h - base.B}" stroke="{c.t["grid"]}"/>')
        c.text(X, h - base.B + 16, base.compact(v) if v >= 1 else f"{v:g}", 11, "muted", "middle")
    c.text((base.L + c.sx(xt[-1])) / 2, h - base.B + 34, "living lineage when the run ended", 12, "ink2",
           "middle")
    for i, x in enumerate(rows):
        y = 108 + 30 * i
        sc = x["scores"][stage]
        champ = x["id"] == data.get("champion")
        c.text(base.L - 8, y + 4, f'#{x["id"]}', 12, "ink", "end", weight="600" if champ else None)
        col = c.t["series"][KIND_SLOT[x["kind"]]]
        for k, r in enumerate(sc["runs"]):
            jy = y + ((k * 7) % 11 - 5)
            c.parts.append(f'<circle cx="{c.sx(r[0]):.1f}" cy="{jy:.1f}" r="2.6" fill="{col}" fill-opacity="0.75"/>')
        X = c.sx(sc["mean"])
        c.parts.append(f'<line x1="{X:.1f}" y1="{y - 9}" x2="{X:.1f}" y2="{y + 9}" stroke="{c.t["ink"]}" '
                       f'stroke-width="2"/>')
        c.text(base.W - base.R, y + 4, ("champion · " if champ else "") + f'mean {sc["mean"]:.1f} · '
               f'{sc["success"]:.0%} alive', 11, "ink2", "end")
    c.save(OUT / f"{name}-{theme}.svg")


DIVERGE = {"light": ("#e34948", "#f0efec", "#2a78d6"), "dark": ("#e66767", "#383835", "#3987e5")}


def mix(a, b, t):
    ca = [int(a[i:i + 2], 16) for i in (1, 3, 5)]
    cb = [int(b[i:i + 2], 16) for i in (1, 3, 5)]
    return "#" + "".join(f"{round(x + (y - x) * t):02x}" for x, y in zip(ca, cb))


def battles(theme, data):
    path = OUT / "battles.json"
    if not path.exists():
        return
    b = json.loads(path.read_text())
    ids = b["contenders"]
    k = len(ids)
    games = b["head_to_head"]["games"]
    won = {(i, j): 0 for i in ids for j in ids}
    played = {(i, j): 0 for i in ids for j in ids}
    for g in games:
        a, bb = g["a"], g["b"]
        na, nb = g["lineage"]
        played[(a, bb)] += 1
        played[(bb, a)] += 1
        won[(a, bb)] += (na > nb) + 0.5 * (na == nb)
        won[(bb, a)] += (nb > na) + 0.5 * (na == nb)
    per = len(b["head_to_head"]["seeds"]) * 2
    # matrix
    cell = 56
    w = 150 + cell * k + 40
    h = 170 + cell * k + 70
    c = base.Chart(theme, "Head to head", f'Each pair {per} times in {b["world"]} '
                   f'({len(b["head_to_head"]["seeds"])} battle seeds x both orders, '
                   f'{b["head_to_head"]["frames"]:,} frames): wins of the row against the column', height=h)
    lo, mid, hi = DIVERGE[theme]
    x0, y0 = 150, 120
    for j, cj in enumerate(ids):
        c.text(x0 + cell * j + cell / 2, y0 - 10, f"#{cj}", 11, "ink2", "middle")
    for i, ci in enumerate(ids):
        c.text(x0 - 10, y0 + cell * i + cell / 2 + 4, f"#{ci}", 12, "ink", "end")
        for j, cj in enumerate(ids):
            X, Y = x0 + cell * j, y0 + cell * i
            if i == j:
                c.parts.append(f'<rect x="{X + 1}" y="{Y + 1}" width="{cell - 2}" height="{cell - 2}" rx="3" '
                               f'fill="none" stroke="{c.t["grid"]}"/>')
                continue
            share = won[(ci, cj)] / played[(ci, cj)] if played[(ci, cj)] else 0.5
            col = mix(lo, mid, share * 2) if share < 0.5 else mix(mid, hi, (share - 0.5) * 2)
            lum = sum(int(col[q:q + 2], 16) * f for q, f in ((1, .299), (3, .587), (5, .114)))
            ink = "#0b0b0b" if lum > 150 else "#f0efec"
            c.parts.append(f'<rect x="{X + 1}" y="{Y + 1}" width="{cell - 2}" height="{cell - 2}" rx="3" '
                           f'fill="{col}"/>')
            c.parts.append(f'<text x="{X + cell / 2}" y="{Y + cell / 2 + 4}" font-size="12" '
                           f'text-anchor="middle" fill="{ink}">{won[(ci, cj)]:g}–{played[(ci, cj)] - won[(ci, cj)]:g}'
                           f'</text>')
    ly = y0 + cell * k + 30
    for q, (t_, lab) in enumerate(((0, "row lost all"), (0.5, "even"), (1, "row won all"))):
        col = mix(lo, mid, t_ * 2) if t_ < 0.5 else mix(mid, hi, (t_ - 0.5) * 2)
        c.parts.append(f'<rect x="{x0 + q * 130}" y="{ly}" width="14" height="14" rx="3" fill="{col}"/>')
        c.text(x0 + q * 130 + 20, ly + 12, lab, 12, "ink2")
    c.save(OUT / f"battles-matrix-{theme}.svg")

    # standings
    pts = {i: sum(won[(i, j)] for j in ids if j != i) for i in ids}
    tot = {i: sum(played[(i, j)] for j in ids if j != i) for i in ids}
    ffa = b["free_for_all"]["battles"]
    ffa_w = {i: sum(1 for bt in ffa if max(bt["rows"][-1][1:-1]) > 0 and
                    bt["rows"][-1][1 + ids.index(i)] == max(bt["rows"][-1][1:-1])) for i in ids}
    order = sorted(ids, key=lambda i: (-pts[i], -ffa_w[i]))
    h = 120 + 34 * k
    c = base.Chart(theme, "Standings", f"Points from every head-to-head game (a win 1, a draw 1/2), "
                   f"and free-for-all battles won of {len(ffa)}", height=h)
    xt = nice_ticks(max(tot.values()))
    c.plot(0, xt[-1], 0, 1, 80, h - base.B)
    for v in xt:
        X = c.sx(v)
        c.parts.append(f'<line x1="{X:.1f}" y1="80" x2="{X:.1f}" y2="{h - base.B}" stroke="{c.t["grid"]}"/>')
        c.text(X, h - base.B + 16, f"{v:g}", 11, "muted", "middle")
    for r, i in enumerate(order):
        y = 92 + 34 * r
        champ = i == data.get("champion")
        c.text(base.L - 8, y + 13, f"#{i}", 12, "ink", "end", weight="600" if champ else None)
        wd = max(1.0, c.sx(pts[i]) - base.L)
        c.parts.append(f'<path d="M{base.L},{y + 4} h{wd - 4:.1f} a4,4 0 0 1 4,4 v10 a4,4 0 0 1 -4,4 '
                       f'h{-(wd - 4):.1f} z" fill="{c.t["series"][0]}"/>')
        c.text(base.L + wd + 6, y + 17, ("champion  ·  " if champ else "")
               + f"{pts[i]:g} of {tot[i]}  ·  free-for-all {ffa_w[i]} of {len(ffa)}",
               11, "ink2")
    c.save(OUT / f"battles-standings-{theme}.svg")

    # free-for-all: one panel per contender (small multiples on one scale),
    # every battle a thin line, the mean a thick one
    frames = [0] + [r[0] for r in ffa[0]["rows"]]
    runs = {i: [[1] + [r[1 + n] for r in bt["rows"]] for bt in ffa] for n, i in enumerate(ids)}
    top = max(v for i in ids for run in runs[i] for v in run)
    yt = nice_ticks(top * 1.05)
    cols, pw, ph, gx, gy = 2, 340, 130, 64, 62
    rows_n = (k + cols - 1) // cols
    left, top_y = 64, 112
    h = top_y + rows_n * (ph + gy) + 30
    c = base.Chart(theme, f"Free-for-all: all {k} at once",
                   f'{len(ffa)} battles in {b["world"]}, {b["free_for_all"]["frames"]:,} frames. Each battle '
                   f"a thin line, the mean a thick one", height=h)
    col = c.t["series"][0]
    for n, i in enumerate(ids):
        X0 = left + (n % cols) * (pw + gx)
        Y1 = top_y + (n // cols) * (ph + gy) + 20
        Y0 = Y1 + ph
        sx = lambda f: X0 + f / frames[-1] * pw
        sy = lambda v: Y0 - v / yt[-1] * ph
        alive = sum(1 for run in runs[i] if run[-1] > 0)
        champ = i == data.get("champion")
        c.text(X0, Y1 - 10, f"#{i}" + ("  champion" if champ else ""), 12, "ink", weight="600")
        c.text(X0 + pw, Y1 - 10, f"alive at the end in {alive} of {len(ffa)}  ·  won {ffa_w[i]}", 11, "ink2",
               "end")
        for v in yt:
            c.parts.append(f'<line x1="{X0}" y1="{sy(v):.1f}" x2="{X0 + pw}" y2="{sy(v):.1f}" '
                           f'stroke="{c.t["grid"] if v else c.t["axis"]}"/>')
            if n % cols == 0:
                c.text(X0 - 8, sy(v) + 4, base.compact(v), 11, "muted", "end")
        for f in nice_ticks(frames[-1], 2):
            c.text(sx(f), Y0 + 16, base.compact(f), 11, "muted", "middle")
        for run in runs[i]:
            d = "M" + " L".join(f"{sx(f):.1f},{sy(v):.1f}" for f, v in zip(frames, run))
            c.parts.append(f'<path d="{d}" fill="none" stroke="{col}" stroke-opacity="0.35" stroke-width="1"/>')
        mean = [sum(run[q] for run in runs[i]) / len(ffa) for q in range(len(frames))]
        d = "M" + " L".join(f"{sx(f):.1f},{sy(v):.1f}" for f, v in zip(frames, mean))
        c.parts.append(f'<path d="{d}" fill="none" stroke="{col}" stroke-width="2.2" stroke-linejoin="round"/>')
    c.parts.append(f'<text transform="translate(16 {(top_y + h - 30) / 2:.1f}) rotate(-90)" font-size="12" '
                   f'fill="{c.t["ink2"]}" text-anchor="middle">lineage</text>')
    c.save(OUT / f"battles-ffa-{theme}.svg")

def main():
    data = json.loads((OUT / "tournament.json").read_text())
    for theme in base.THEMES:
        progress(theme, data)
        finalists(theme, data)
        growth(theme, data)
        strip(theme, data, "train", 12, "Contenders on the training worlds",
              "The best twelve: every import (16 worlds x 2) as a dot, the mean as a line", "contenders-train")
        strip(theme, data, "final", 8, f'Contenders in {data.get("world") or "the world"}',
              f"The last round: every import ({len(FINAL_SEEDS)} spots, {FINAL_FRAMES:,} frames) as a dot, "
              "the mean as a line", "contenders-final")
        battles(theme, data)


if __name__ == "__main__":
    main()
