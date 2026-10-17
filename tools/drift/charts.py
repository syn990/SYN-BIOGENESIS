#!/usr/bin/env python3
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""Draws the drift study's charts from docs/drift/*.csv as SVG, a light and
a dark version of each (the README picks one with <picture>). Standard
library only.

    tools/drift/charts.py
"""

import csv
import math
from pathlib import Path

OUT = Path(__file__).resolve().parents[2] / "docs" / "drift"

THEMES = {
    "light": {"surface": "#fcfcfb", "ink": "#0b0b0b", "ink2": "#52514e", "muted": "#898781",
              "grid": "#e1e0d9", "axis": "#c3c2b7", "border": "rgba(11,11,11,0.10)",
              "series": ["#2a78d6", "#eb6834", "#1baf7a", "#eda100", "#e87ba4"]},
    "dark": {"surface": "#1a1a19", "ink": "#f0efec", "ink2": "#c3c2b7", "muted": "#898781",
             "grid": "#2c2c2a", "axis": "#383835", "border": "rgba(255,255,255,0.10)",
             "series": ["#3987e5", "#d95926", "#199e70", "#c98500", "#d55181"]},
}
# colour follows the implementation in every chart
SLOT = {"java27": 0, "port": 1, "port-libc": 2, "java8": 3, "java27-fdlibm": 4}
NAME = {"java27": "Java 27", "port": "SYN-BIOGENESIS", "port-libc": "SYN-BIOGENESIS, C library maths",
        "java8": "Java 8", "java27-fdlibm": "Java 27, fdlibm maths"}
FONT = "system-ui, -apple-system, 'Segoe UI', sans-serif"

W, H = 820, 440
L, R, T, B = 64, 24, 96, 48  # plot margins


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def compact(v):
    if v >= 1000 and v % 1000 == 0:
        return f"{v // 1000}K"
    return f"{v:g}"


class Chart:
    def __init__(self, theme, title, subtitle, height=H):
        self.t = THEMES[theme]
        self.h = height
        self.parts = [
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{height}" '
            f'viewBox="0 0 {W} {height}" font-family="{FONT}">',
            f'<title>{esc(title)}</title>',
            f'<rect x="0.5" y="0.5" width="{W - 1}" height="{height - 1}" rx="8" '
            f'fill="{self.t["surface"]}" stroke="{self.t["border"]}"/>',
            f'<text x="24" y="34" font-size="16" font-weight="600" fill="{self.t["ink"]}">{esc(title)}</text>',
            f'<text x="24" y="56" font-size="13" fill="{self.t["ink2"]}">{esc(subtitle)}</text>',
        ]

    def text(self, x, y, s, size=12, color="muted", anchor="start", weight=None):
        w = f' font-weight="{weight}"' if weight else ""
        self.parts.append(f'<text x="{x:.1f}" y="{y:.1f}" font-size="{size}" fill="{self.t[color]}" '
                          f'text-anchor="{anchor}"{w}>{esc(s)}</text>')

    def legend(self, keys, y=78):
        x = 24
        for slot, label, dash in keys:
            c = self.t["series"][slot]
            d = ' stroke-dasharray="5 4"' if dash else ""
            self.parts.append(f'<line x1="{x}" y1="{y - 4}" x2="{x + 18}" y2="{y - 4}" stroke="{c}" '
                              f'stroke-width="2" stroke-linecap="round"{d}/>')
            self.text(x + 24, y, label, 12, "ink2")
            x += 24 + 7.1 * len(label) + 22

    def plot(self, x0, x1, y0, y1, top, bottom, ylog=False):
        self.x0, self.x1, self.y0, self.y1, self.ylog = x0, x1, y0, y1, ylog
        self.pt, self.pb = top, bottom

    def sx(self, x):
        return L + (x - self.x0) / (self.x1 - self.x0) * (W - L - R)

    def sy(self, y):
        if self.ylog:
            y = math.log10(y)
            a, b = math.log10(self.y0), math.log10(self.y1)
        else:
            a, b = self.y0, self.y1
        return self.pb - (y - a) / (b - a) * (self.pb - self.pt)

    def axes(self, xticks, yticks, xlabel, ylabel, yfmt=compact):
        for v in yticks:
            y = self.sy(v)
            self.parts.append(f'<line x1="{L}" y1="{y:.1f}" x2="{W - R}" y2="{y:.1f}" '
                              f'stroke="{self.t["grid"]}" stroke-width="1"/>')
            self.text(L - 8, y + 4, yfmt(v), 11, "muted", "end")
        self.parts.append(f'<line x1="{L}" y1="{self.pb:.1f}" x2="{W - R}" y2="{self.pb:.1f}" '
                          f'stroke="{self.t["axis"]}" stroke-width="1"/>')
        for v in xticks:
            self.text(self.sx(v), self.pb + 18, compact(v), 11, "muted", "middle")
        self.text((L + W - R) / 2, self.pb + 36, xlabel, 12, "ink2", "middle")
        self.parts.append(f'<text transform="translate(16 {(self.pt + self.pb) / 2:.1f}) rotate(-90)" '
                          f'font-size="12" fill="{self.t["ink2"]}" text-anchor="middle">{esc(ylabel)}</text>')

    def line(self, pts, slot, dash=False, step=False):
        c = self.t["series"][slot]
        d = []
        prev = None
        for x, y in pts:
            if y is None:
                prev = None
                continue
            X, Y = self.sx(x), self.sy(y)
            if prev is None:
                d.append(f"M{X:.1f},{Y:.1f}")
            elif step:
                d.append(f"H{X:.1f}V{Y:.1f}")
            else:
                d.append(f"L{X:.1f},{Y:.1f}")
            prev = (X, Y)
        da = ' stroke-dasharray="5 4"' if dash else ""
        self.parts.append(f'<path d="{"".join(d)}" fill="none" stroke="{c}" stroke-width="2" '
                          f'stroke-linejoin="round" stroke-linecap="round"{da}/>')

    def dot(self, x, y, slot):
        self.parts.append(f'<circle cx="{self.sx(x):.1f}" cy="{self.sy(y):.1f}" r="4" '
                          f'fill="{self.t["series"][slot]}" stroke="{self.t["surface"]}" stroke-width="2"/>')

    def vline(self, x, label, anchor="start"):
        X = self.sx(x)
        self.parts.append(f'<line x1="{X:.1f}" y1="{self.pt}" x2="{X:.1f}" y2="{self.pb}" '
                          f'stroke="{self.t["axis"]}" stroke-width="1"/>')
        self.text(X + (6 if anchor == "start" else -6), self.pt + 12, label, 11, "ink2", anchor)

    def hline(self, y, label, left=False):
        Y = self.sy(y)
        self.parts.append(f'<line x1="{L}" y1="{Y:.1f}" x2="{W - R}" y2="{Y:.1f}" '
                          f'stroke="{self.t["axis"]}" stroke-width="1"/>')
        if left:
            self.text(L + 4, Y - 6, label, 11, "ink2")
        else:
            self.text(W - R - 4, Y - 6, label, 11, "ink2", "end")

    def save(self, path):
        path.write_text("\n".join(self.parts + ["</svg>"]) + "\n")


def read(name):
    with open(OUT / name, newline="") as f:
        return list(csv.DictReader(f))


def nice_ticks(v, n=4):
    """0 and up to n more round ticks covering v."""
    k = 10 ** math.floor(math.log10(v / n))
    for m in (1, 2, 2.5, 5, 10):
        if m * k * n >= v:
            step = m * k
            return [step * i for i in range(math.ceil(v / step) + 1)]


def power(v):
    sup = str.maketrans("-0123456789", "\u207b\u2070\u00b9\u00b2\u00b3\u2074\u2075\u2076\u2077\u2078\u2079")
    e = int(round(math.log10(v)))
    return {0: "1", 3: "1000"}.get(e, "10" + str(e).translate(sup))


def population(theme, seed, results):
    rows = read("population.csv")
    frames = [int(r["frame"]) for r in rows]
    names = [n for n in ("java27", "port", "port-libc") if n in rows[0]]
    part = next((int(r["permanent_divergence"]) for r in results
                 if r["seed"] == str(seed) and r["variant"] == "port-libc" and r["permanent_divergence"]), None)
    yt = nice_ticks(max(int(r[n]) for r in rows for n in names) * 1.05)
    c = Chart(theme, f"Population of seed {seed}, frame by frame",
              "SYN-BIOGENESIS (dashed) runs exactly on top of the Java original; "
              "with the C library's maths it parts from it")
    c.legend([(SLOT["java27"], "Java 27 (the shipped jar)", False),
              (SLOT["port"], "SYN-BIOGENESIS", True),
              (SLOT["port-libc"], "SYN-BIOGENESIS, C library maths", False)])
    c.plot(0, frames[-1], 0, yt[-1], T, H - B)
    c.axes(nice_ticks(frames[-1]), yt, "frame", "organisms alive")
    if part is not None:
        c.vline(part, f"parts at frame {part:,}")
    c.line([(f, int(r["port-libc"])) for f, r in zip(frames, rows)], SLOT["port-libc"])
    c.line([(f, int(r["java27"])) for f, r in zip(frames, rows)], SLOT["java27"])
    c.line([(f, int(r["port"])) for f, r in zip(frames, rows)], SLOT["port"], dash=True)
    c.save(OUT / f"population-{theme}.svg")


def divergence(theme, results, frames, seeds):
    c = Chart(theme, f"When does a seed's world part from the Java original's for good?",
              f"Share of {seeds} seeds (1 to {seeds}) whose state never matches Java 27's again, "
              f"by frame, over {frames:,} frames")
    order = ["port-libc", "java27-fdlibm", "java8"]
    present = [n for n in order if any(r["variant"] == n for r in results)]
    c.legend([(SLOT[n], NAME[n], False) for n in present] + [(SLOT["port"], "SYN-BIOGENESIS", False)])
    c.plot(0, frames, 0, 100, T, H - B)
    c.axes(nice_ticks(frames), [0, 25, 50, 75, 100], "frame", "seeds parted (%)",
           yfmt=lambda v: f"{v:g}%")
    ends = []
    for n in present:
        parts = sorted(int(r["permanent_divergence"]) for r in results
                       if r["variant"] == n and r["permanent_divergence"])
        pts = [(0, 0.0)]
        for i, f in enumerate(parts, 1):
            pts.append((f, 100.0 * i / seeds))
        pts.append((frames, 100.0 * len(parts) / seeds))
        c.line(pts, SLOT[n], step=True)
        ends.append((c.sy(pts[-1][1]), len(parts)))
    # end labels above their lines; one too close to the label above goes below
    ends.sort()
    prev = None
    for y, k in ends:
        ty = y - 8 if prev is None or y - 8 - prev > 13 else y + 16
        c.text(c.sx(frames) - 4, ty, f"{k} of {seeds}", 11, "ink", "end")
        prev = ty
    same = [n for n in ("port", "java11", "java17", "java21")
            if any(r["variant"] == n for r in results)
            and not any(r["variant"] == n and r["first_difference"] for r in results)]
    c.line([(0, 0.0), (frames, 0.0)], SLOT["port"])
    labels = ["SYN-BIOGENESIS" if n == "port" else "Java " + n[4:] for n in same]
    c.text(c.sx(frames) - 4, c.sy(0) - 8, ", ".join(labels) + f": 0 of {seeds}, never differ", 11, "ink", "end")
    c.save(OUT / f"divergence-{theme}.svg")


def growth(theme):
    rows = read("growth.csv")
    names = [n for n in ("port-libc", "java8", "java27-fdlibm") if any(r["variant"] == n for r in rows)]
    span = max(int(r["frames_after_parting"]) for r in rows)
    h2 = 640
    c = Chart(theme, "Lapsadian determinism: how one last bit becomes a different world",
              "From the frame two worlds part for good (median over the seeds followed): "
              "largest gap between the same organism in each", height=h2)
    c.legend([(SLOT[n], NAME[n], False) for n in names])
    # top panel: position gap, log scale; bottom panel: organisms differing
    c.plot(0, span, 1e-14, 1e3, T, 380, ylog=True)
    c.axes(nice_ticks(span), [1e-12, 1e-9, 1e-6, 1e-3, 1, 1e3],
           "", "largest position gap (pixels)", yfmt=power)
    c.hline(1, "1 pixel: visible on screen", left=True)
    for n in names:
        pts = []
        for r in rows:
            if r["variant"] != n:
                continue
            v = float(r["median_max_position_difference"])
            pts.append((int(r["frames_after_parting"]), v if v > 0 else None))
        c.line(pts, SLOT[n])
    c.plot(0, span, 0, 100, 430, h2 - B)
    c.text(L, 418, "Organisms whose state differs, or that exist in one world only", 12, "ink2")
    c.axes(nice_ticks(span), [0, 50, 100], "frames since the worlds parted", "",
           yfmt=lambda v: f"{v:g}%")
    for n in names:
        c.line([(int(r["frames_after_parting"]), 100 * float(r["median_share_of_organisms_differing"]))
                for r in rows if r["variant"] == n], SLOT[n])
    c.save(OUT / f"growth-{theme}.svg")


def main():
    results = read("results.csv")
    frames = int(results[0]["frames"])
    seeds = len({r["seed"] for r in results})
    for theme in THEMES:
        population(theme, 42, results)
        divergence(theme, results, frames, seeds)
        growth(theme)


if __name__ == "__main__":
    main()
