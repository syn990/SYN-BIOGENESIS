#!/usr/bin/env python3
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""Prints docs/drift/results.csv as the Markdown table in the README.

    tools/drift/summary.py
"""

import csv
import statistics
from pathlib import Path

OUT = Path(__file__).resolve().parents[2] / "docs" / "drift"
ORDER = ["port", "java21", "java17", "java11", "java8", "java27-fdlibm", "port-libc"]


def main():
    with open(OUT / "variants.csv", newline="") as f:
        labels = {r["variant"]: (r["label"], r["version"]) for r in csv.DictReader(f)}
    with open(OUT / "results.csv", newline="") as f:
        rows = list(csv.DictReader(f))
    frames = int(rows[0]["frames"])
    seeds = len({r["seed"] for r in rows})
    print(f"| Compared with Java 27 | Same world for all {frames:,} frames | Seeds that ever differ "
          "| First difference: median frame (range) | Seeds that part for good "
          "| Frame they part: median (range) | Differences that healed |")
    print("|---|---|---|---|---|---|---|")

    def span(xs):
        if not xs:
            return "never"
        return f"{statistics.median(xs):,.0f} ({min(xs):,} to {max(xs):,})"

    for v in ORDER:
        rs = [r for r in rows if r["variant"] == v]
        if not rs:
            continue
        first = [int(r["first_difference"]) for r in rs if r["first_difference"]]
        part = [int(r["permanent_divergence"]) for r in rs if r["permanent_divergence"]]
        healed = sum(int(r["healed"]) for r in rs)
        same = len(rs) - len(first)
        print(f"| {labels[v][0]} | {same} of {seeds} | {len(first)} | {span(first)} | {len(part)} "
              f"| {span(part)} | {healed:,} |")


if __name__ == "__main__":
    main()
