#!/usr/bin/env python3
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""Battles between the contenders of the being search's last round, in
the world it was judged in, with the deterministic battle protocol of
being.c (`being battle`): every result here replays exactly.

  head-to-head   every pair, battle seeds 1-3, each in both orders
                 (spots and who moves first swap), 20,000 frames
  free-for-all   every contender at once, battle seeds 1-6, 100,000
                 frames, lineages every 2,000 frames

Writes docs/being/battles.json, and every competitor of the tournament as
docs/being/beings/ID.bgg.

    battles.py [--nice N]
"""
import argparse
import concurrent.futures as cf
import itertools
import json
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from search import BUILD, OUT, TOOL, to_bgg  # noqa: E402

H2H_SEEDS, H2H_FRAMES = [1, 2, 3], 20000
FFA_SEEDS, FFA_FRAMES, FFA_EVERY = [1, 2, 3, 4, 5, 6], 100000, 2000


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--nice", type=int, default=19)
    nice = str(ap.parse_args().nice)
    data = json.loads((OUT / "tournament.json").read_text())
    cands = {c["id"]: c for c in data["candidates"]}

    beings = OUT / "beings"
    beings.mkdir(exist_ok=True)
    for c in cands.values():
        (beings / f'{c["id"]}.bgg').write_text(to_bgg(c["code"]))

    st = "final" if any("final" in c["scores"] for c in cands.values()) else "held"
    contenders = sorted((i for i, c in cands.items() if st in c["scores"]),
                        key=lambda i: -cands[i]["scores"][st].get("score", cands[i]["scores"][st]["mean"]))[:8]
    world = BUILD / "worlds" / ("user.sbw" if st == "final" else "w101_250000.sbw")
    files = {i: str(beings / f"{i}.bgg") for i in contenders}

    def battle(seed, frames, every, ids):
        out = subprocess.run(["nice", "-n", nice, TOOL, "battle", str(world), str(seed), str(frames),
                              str(every)] + [files[i] for i in ids], capture_output=True, text=True,
                             check=True).stdout
        rows = []
        for l in out.splitlines():
            p = l.split()
            if p[0] == "T":
                rows.append([int(v) for v in p[1:]])
            elif p[0] == "END":
                rows.append([frames] + [int(v) for v in p[1:]])
        return rows

    h2h_tasks = [(a, b, s, order) for a, b in itertools.combinations(contenders, 2)
                 for s in H2H_SEEDS for order in ("ab", "ba")]

    def h2h(task):
        a, b, s, order = task
        ids = [a, b] if order == "ab" else [b, a]
        end = battle(s, H2H_FRAMES, H2H_FRAMES, ids)[-1]
        n = dict(zip(ids, end[1:3]))
        return {"a": a, "b": b, "seed": s, "order": order, "lineage": [n[a], n[b]], "population": end[3]}

    with cf.ThreadPoolExecutor(os.cpu_count() or 2) as ex:
        games = list(ex.map(h2h, h2h_tasks))
        ffa = list(ex.map(lambda s: {"seed": s, "rows": battle(s, FFA_FRAMES, FFA_EVERY, contenders)},
                          FFA_SEEDS))
    (OUT / "battles.json").write_text(json.dumps({
        "world": data.get("world") or world.name, "contenders": contenders,
        "head_to_head": {"seeds": H2H_SEEDS, "frames": H2H_FRAMES, "games": games},
        "free_for_all": {"seeds": FFA_SEEDS, "frames": FFA_FRAMES, "every": FFA_EVERY, "battles": ffa},
    }, indent=1))
    for i in contenders:
        w = sum((g["lineage"][0] > g["lineage"][1]) if g["a"] == i else (g["lineage"][1] > g["lineage"][0])
                for g in games if i in (g["a"], g["b"]))
        n = sum(1 for g in games if i in (g["a"], g["b"]))
        ffa_wins = sum(1 for b in ffa if max(b["rows"][-1][1:-1]) > 0 and
                       b["rows"][-1][1 + contenders.index(i)] == max(b["rows"][-1][1:-1]))
        print(f"#{i}: head-to-head {w}/{n} won, free-for-all {ffa_wins}/{len(ffa)} won")


if __name__ == "__main__":
    main()
