#!/usr/bin/env python3
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""The fade study: what of its seed a world still shows as it goes on.

Runs `syn-biogenesis --telescope fade` on many seeds, for two sets of
parameters: 0.9's defaults, and the ones in docs/fade/params.cfg (the
arena's: the user's own, as saved when the study was run). Every world is
1000 x 1000 with 15 first organisms, the arena's small map. Each sample
reads every living gene's place in the world's stream of draws (see
src/syn/telescope.h), so it says how many founder genes are left, where the
oldest surviving mark is, how many bloodlines are left and what the living
are made of. Writes docs/fade/samples.csv and marks.csv; charts.py draws
them.

    tools/fade/fade.py [--seeds N] [--units U] [--jobs J]
"""

import argparse
import concurrent.futures as cf
import csv
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "docs" / "fade"
PROGRAM = ROOT / "syn-biogenesis"
SETS = {"defaults": ["--defaults"], "arena": []}
EVERY = 10    # time units between samples
MARKS = 2     # seeds of each set whose marks are kept


def kept(tick, marks=False):
    """What is written: every sample to time 1000, then every other; the
    marks every 20 time units (all the charts draw)."""
    t = tick // 256
    return t % 20 == 0 if marks or t > 1000 else True
FIELDS = ["tick", "alive", "draws", "genes", "founder_genes", "oldest_draw", "oldest_tick", "unread",
          "lines", "o2", "co2", "green", "red", "cyan", "blue", "white", "gray", "yellow",
          "genes_per", "symmetry"]


def run(cfg, name, seed, units, marks):
    args = [str(PROGRAM), "--telescope", "fade", str(seed), str(units * 256), str(EVERY * 256),
            "--world", "1000", "1000", "15"] + SETS[name] + (["--marks"] if marks else [])
    env = dict(os.environ, XDG_CONFIG_HOME=cfg)
    out = subprocess.run(args, env=env, capture_output=True, text=True, check=True).stdout
    samples, mk, genesis = [], [], 0
    for line in out.splitlines():
        p = line.split()
        if line.startswith("# seed"):
            genesis = int(line.rsplit("in ", 1)[1].split()[0])
        elif p and p[0] == "S" and kept(int(p[1])):
            samples.append([name, seed, genesis] + p[1:])
        elif p and p[0] == "M" and kept(int(p[1]), True):
            mk.append([name, seed] + p[1:])
    return samples, mk


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seeds", type=int, default=24)
    ap.add_argument("--units", type=int, default=4000)
    ap.add_argument("--jobs", type=int, default=os.cpu_count())
    a = ap.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    params = OUT / "params.cfg"
    if not params.exists():
        shutil.copy(Path.home() / ".config" / "syn-biogenesis" / "params.cfg", params)
    with tempfile.TemporaryDirectory() as tmp:
        (Path(tmp) / "syn-biogenesis").mkdir()
        shutil.copy(params, Path(tmp) / "syn-biogenesis" / "params.cfg")
        jobs = [(name, seed) for name in SETS for seed in range(1, a.seeds + 1)]
        with cf.ProcessPoolExecutor(a.jobs) as ex:
            futs = {ex.submit(run, tmp, name, seed, a.units, seed <= MARKS): (name, seed) for name, seed in jobs}
            done = {}
            for f in cf.as_completed(futs):
                done[futs[f]] = f.result()
                print(f"{futs[f][0]} seed {futs[f][1]}: {len(done[futs[f]][0])} samples", flush=True)
    with open(OUT / "samples.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["set", "seed", "genesis_draws"] + FIELDS)
        for key in jobs:
            w.writerows(done[key][0])
    with open(OUT / "marks.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["set", "seed", "tick", "birth_unit", "genes"])
        for key in jobs:
            w.writerows(done[key][1])


if __name__ == "__main__":
    main()
