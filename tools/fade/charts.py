#!/usr/bin/env python3
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""Draws the fade study's charts from docs/fade/*.csv as SVG, a light and a
dark version of each, in the drift study's style (tools/drift/charts.py).

    tools/fade/charts.py
"""

import csv
import itertools
import math
import statistics
import sys
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "drift"))
import charts as base  # noqa: E402  (tools/drift/charts.py)

OUT = HERE.parents[1] / "docs" / "fade"
SETS = [("defaults", "0.9's defaults", 0), ("arena", "the arena's parameters", 1)]
COLOURS = ["green", "red", "cyan", "blue", "white", "gray", "yellow"]


class Chart(base.Chart):
    def band(self, lo, hi, slot, alpha=0.18):
        pts = [(self.sx(x), self.sy(y)) for x, y in lo] + [(self.sx(x), self.sy(y)) for x, y in reversed(hi)]
        d = "M" + "L".join(f"{x:.1f},{y:.1f}" for x, y in pts) + "Z"
        self.parts.append(f'<path d="{d}" fill="{self.t["series"][slot]}" fill-opacity="{alpha}" stroke="none"/>')

    def thin(self, pts, slot, alpha=0.22):
        d = "M" + "L".join(f"{self.sx(x):.1f},{self.sy(y):.1f}" for x, y in pts)
        self.parts.append(f'<path d="{d}" fill="none" stroke="{self.t["series"][slot]}" stroke-width="1" '
                          f'stroke-opacity="{alpha}" stroke-linejoin="round"/>')

    def spot(self, x, y, r, slot, alpha=0.55):
        self.parts.append(f'<circle cx="{self.sx(x):.1f}" cy="{self.sy(y):.1f}" r="{r:.1f}" '
                          f'fill="{self.t["series"][slot]}" fill-opacity="{alpha}"/>')

    def note(self, x, y, s, anchor="start"):
        self.text(self.sx(x), self.sy(y), s, 11, "ink2", anchor)


def runs():
    by = defaultdict(list)
    with open(OUT / "samples.csv", newline="") as f:
        for r in csv.DictReader(f):
            by[(r["set"], int(r["seed"]))].append(r)
    return by


def per_time(by, name, value, tmax):
    """{time unit: [the value in each run that got there]}"""
    at = defaultdict(list)
    for (s, _), rs in by.items():
        if s != name:
            continue
        for r in rs:
            t = int(r["tick"]) / 256
            if t <= tmax:
                v = value(r)
                if v is not None:
                    at[t].append(v)
    return dict(sorted(at.items()))


def quartiles(at):
    lo, mid, hi = [], [], []
    for t, v in at.items():
        if len(v) < 4:
            continue
        q = statistics.quantiles(v, n=4)
        lo.append((t, q[0]))
        mid.append((t, statistics.median(v)))
        hi.append((t, q[2]))
    return lo, mid, hi


def median_first(by, name, test):
    """The median time a run first passes `test`, over the runs that do."""
    ts = []
    for (s, _), rs in by.items():
        if s == name:
            t = next((int(r["tick"]) / 256 for r in rs if test(r)), None)
            if t is not None:
                ts.append(t)
    return statistics.median(ts) if ts else None


def bits(r):
    d = int(r["oldest_draw"])
    return None if d < 0 else 48 - math.log2(d + 1)


def chart_bits(theme, by):
    c = Chart(theme, "How much of its seed a world still shows",
              "Bits of the 48-bit seed that its oldest living gene still pins down: 48 - log2(draws from the seed to it)")
    c.legend([(slot, label, False) for _, label, slot in SETS])
    c.plot(0, 4000, 0, 48, base.T, base.H - base.B)
    c.axes([0, 1000, 2000, 3000, 4000], [0, 12, 24, 36, 48], "time units", "bits of the seed readable",
           yfmt=lambda v: f"{v:g}")
    for name, _, slot in SETS:
        for (s, _), rs in by.items():
            if s == name:
                c.thin([(int(r["tick"]) / 256, bits(r)) for r in rs if bits(r) is not None], slot, 0.16)
        _, mid, _ = quartiles(per_time(by, name, bits, 4000))
        c.line(mid, slot)
    c.hline(48, "the whole seed: 2^48 worlds", left=True)
    t = median_first(by, "arena", lambda r: int(r["founder_genes"]) == 0)
    if t:
        c.vline(t, f"no founder gene left (median, {t:.0f})")
    return c


def chart_founders(theme, by):
    c = Chart(theme, "The seed's fingerprint: founder genes still alive",
              "Of the living's genes, those drawn at tick 0; median and middle half of 24 seeds")
    c.legend([(slot, label, False) for _, label, slot in SETS])
    c.plot(0, 2000, 0, 1, base.T, base.H - base.B)
    c.axes([0, 500, 1000, 1500, 2000], [0, 0.25, 0.5, 0.75, 1], "time units", "share of the living's genes",
           yfmt=lambda v: f"{v * 100:.0f}%")
    for name, _, slot in SETS:
        at = per_time(by, name, lambda r: int(r["founder_genes"]) / int(r["genes"]) if int(r["genes"]) else None,
                      2000)
        lo, mid, hi = quartiles(at)
        c.band(lo, hi, slot)
        c.line(mid, slot)
    return c


def chart_lines(theme, by):
    c = Chart(theme, "Every world becomes one bloodline",
              "Bloodlines alive (descendants of each of the 15 founders); median and middle half of 24 seeds")
    c.legend([(slot, label, False) for _, label, slot in SETS])
    c.plot(0, 400, 0, 15, base.T, base.H - base.B)
    c.axes([0, 100, 200, 300, 400], [1, 5, 10, 15], "time units", "bloodlines alive", yfmt=lambda v: f"{v:g}")
    for name, _, slot in SETS:
        lo, mid, hi = quartiles(per_time(by, name, lambda r: int(r["lines"]), 400))
        c.band(lo, hi, slot)
        c.line(mid, slot, step=True)
        t = median_first(by, name, lambda r: int(r["lines"]) == 1)
        if t:
            c.dot(t, 1, slot)
    c.hline(1, "one bloodline: the rest of the run is its", left=False)
    return c


def composition(r):
    return [float(r[k]) for k in COLOURS]


def chart_converge(theme, by):
    c = Chart(theme, "How alike different seeds' worlds become",
              "Mean distance between the seeds' colour make-up of the living (those still alive), over 100 time units")
    c.legend([(slot, label, False) for _, label, slot in SETS])
    c.plot(0, 4000, 0, 0.5, base.T, base.H - base.B)
    c.axes([0, 1000, 2000, 3000, 4000], [0, 0.1, 0.2, 0.3, 0.4, 0.5], "time units",
           "mean distance between seeds", yfmt=lambda v: f"{v:g}")
    for name, _, slot in SETS:
        at = per_time(by, name, composition, 4000)
        pts = []
        for t, vs in at.items():
            if len(vs) < 4:
                continue
            ds = [math.dist(a, b) for a, b in itertools.combinations(vs, 2)]
            pts.append((t, statistics.mean(ds)))
        # the mean over the 100 time units up to each
        smooth = [(t, statistics.mean(v for u, v in pts if t - 100 < u <= t)) for t, _ in pts]
        c.line(smooth, slot)
    return c


def chart_green(theme, by):
    c = Chart(theme, "What they become: the green share",
              "Of the living's segments (by length), the green ones that feed on light; median and middle half")
    c.legend([(slot, label, False) for _, label, slot in SETS])
    c.plot(0, 4000, 0, 1, base.T, base.H - base.B)
    c.axes([0, 1000, 2000, 3000, 4000], [0, 0.25, 0.5, 0.75, 1], "time units", "green share",
           yfmt=lambda v: f"{v * 100:.0f}%")
    for name, _, slot in SETS:
        lo, mid, hi = quartiles(per_time(by, name, lambda r: float(r["green"]), 4000))
        c.band(lo, hi, slot)
        c.line(mid, slot)
    return c


def chart_marks(theme, name, seed, slot):
    pts = defaultdict(list)
    with open(OUT / "marks.csv", newline="") as f:
        for r in csv.DictReader(f):
            if r["set"] == name and int(r["seed"]) == seed:
                pts[int(r["tick"]) / 256].append((int(r["birth_unit"]), int(r["genes"])))
    label = dict((n, lab) for n, lab, _ in SETS)[name]
    c = Chart(theme, f"Every living gene is a mark on the seed's stream (seed {seed})",
              f"{label}: each dot, the living genes drawn in one time unit; at 0, the founders' genes")
    tmax = max(pts) if pts else 4000
    c.plot(0, tmax, 0, tmax, base.T, base.H - base.B)
    ticks = [0, 1000, 2000, 3000, 4000]
    c.axes(ticks, ticks, "time units", "drawn in time unit")
    floor = []
    for t, ms in sorted(pts.items()):
        if int(t) % 20:
            continue
        for b, n in ms:
            c.spot(t, b, 1.2 + math.sqrt(n) * 0.9, 3 if b == 0 else slot, 0.5)
        floor.append((t, min(b for b, _ in ms)))
    c.line(floor, 4, step=True)
    c.legend([(slot, "genes drawn after tick 0", False), (3, "founders' genes", False),
              (4, "the oldest alive: how far back the seed is", False)])
    return c


def main():
    by = runs()
    for theme in ("light", "dark"):
        chart_bits(theme, by).save(OUT / f"bits-{theme}.svg")
        chart_founders(theme, by).save(OUT / f"founders-{theme}.svg")
        chart_lines(theme, by).save(OUT / f"lines-{theme}.svg")
        chart_converge(theme, by).save(OUT / f"converge-{theme}.svg")
        chart_green(theme, by).save(OUT / f"green-{theme}.svg")
        chart_marks(theme, "arena", 1, 1).save(OUT / f"marks-{theme}.svg")


if __name__ == "__main__":
    main()
