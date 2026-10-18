#!/usr/bin/env python3
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""The being search: which genetic code does best when one copy is imported
into an evolved world (the window's right-click Import)?

A code's score in a run is its lineage when the run ends: every living
organism whose code descends from it, through its children and through the
children of organisms it infected (tools/being/being.c). Every candidate
in a stage is scored on the same worlds and import spots.

  1. Candidates: the commonest codes of six worlds evolved for 3,000,000
     frames, the winners of the 16 training worlds, twelve designs, and
     twenty random codes.
  2. Screening: 8 training worlds, one import each, 15,000 frames.
  3. The best 16 on all 16 training worlds, two imports each, 30,000 frames.
  4. Five generations of breeding (mutation and crossover of the best ten),
     24 children a generation, scored as in 3.
  5. The best six on 10 held-out worlds, six imports each, 50,000 frames.
  6. With --world: eight more generations bred and scored inside that
     world (12 imports, 12,000 frames), then the best six there on 24
     held-out import spots for 20,000 frames. The best of those is the
     champion.
     In a crowded world most imports die, so phase 2 ranks by a score that
     still separates them: a run's lineage at the end plus 1 if its line
     survived, or, if it died out, the fraction of the run it lasted. Phase
     2 also adds the world's own ten commonest codes and three armoured
     designs to the candidates.

    search.py natural                make the long evolutions (build/natural)
    search.py worlds                 make the training and held-out worlds
    search.py run [--world FILE]     the search; results to docs/being/
    search.py ... --nice N           priority of the simulations (default 19)
    search.py growth                 the champion's lineage over time

Uses the saved parameters (~/.config/syn-biogenesis/params.cfg), so the
search is for the worlds you play. Scores already in docs/being/
tournament.json are reused: the tournament replays the same way (it is
deterministic) and only computes what it hasn't.
"""
import argparse
import concurrent.futures as cf
import glob
import hashlib
import json
import math
import os
import random
import re
import shutil
import statistics
import subprocess
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
TOOL = str(HERE / "being")
BUILD = HERE / "build"
OUT = ROOT / "docs" / "being"
JOBS = max(1, os.cpu_count() or 2)
COLORS = ["green", "red", "cyan", "blue", "white", "gray", "yellow"]

NATURAL_SEEDS = range(11, 17)
NATURAL_FRAMES, NATURAL_EVERY = 3_000_000, 250_000
PHASE1_GENS, PHASE2_GENS = 5, 8
PHASE2_SEEDS, PHASE2_FRAMES = list(range(101, 113)), 12000
FINAL_SEEDS, FINAL_FRAMES = list(range(201, 225)), 20000
TRAIN = [(s, a) for s in range(1, 9) for a in (30000, 120000)]
NICE = 19
CACHE = {}  # (code key, stage) -> scores, from an earlier run
HELD = [(s, a) for s in range(101, 106) for a in (60000, 250000)]


def world_path(w):
    return w if isinstance(w, Path) else BUILD / "worlds" / f"w{w[0]}_{w[1]}.sbw"


def sh(args):
    return subprocess.run(["nice", "-n", str(NICE)] + args, capture_output=True, text=True).stdout


def parallel(fn, items):
    with cf.ThreadPoolExecutor(JOBS) as ex:
        return list(ex.map(fn, items))


def make_natural():
    d = BUILD / "natural"
    d.mkdir(parents=True, exist_ok=True)
    parallel(lambda s: sh([TOOL, "evolve", str(s), str(NATURAL_FRAMES), str(NATURAL_EVERY), str(d)]),
             NATURAL_SEEDS)


def make_worlds():
    (BUILD / "worlds").mkdir(parents=True, exist_ok=True)
    todo = [w for w in TRAIN + HELD if not world_path(w).exists()]
    for out in parallel(lambda w: sh([TOOL, "base", str(w[0]), str(w[1]), str(world_path(w))]), todo):
        print(out.strip(), flush=True)


# ---- genetic codes ----

def parse_bgg(path):
    t = Path(path).read_text()
    root = re.search(r"<genetic_code ([^>]*)>", t).group(1)
    genes = [(float(L), float(th), c) for L, th, c in
             re.findall(r'<gene length="([^"]+)" theta="([^"]+)" color="([a-z]+)"', t)]
    return {"sym": int(re.search(r'symmetry="(\d)"', root).group(1)),
            "mirror": 'mirror="yes"' in root, "disperse": 'disperse="yes"' in root, "genes": genes}


DTD = """<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE genetic_code [
<!ELEMENT genetic_code (gene+)>
<!ATTLIST genetic_code symmetry (1|2|3|4|5|6|7|8) #REQUIRED>
<!ATTLIST genetic_code mirror (yes|no) #REQUIRED>
<!ATTLIST genetic_code disperse (yes|no) #REQUIRED>
<!ELEMENT gene EMPTY>
<!ATTLIST gene length CDATA #REQUIRED>
<!ATTLIST gene theta CDATA #REQUIRED>
<!ATTLIST gene color (red|green|blue|cyan|yellow|gray|white) #REQUIRED>
]>
"""


def to_bgg(c):
    yn = lambda b: "yes" if b else "no"
    out = [DTD + f'<genetic_code symmetry="{c["sym"]}" mirror="{yn(c["mirror"])}" '
           f'disperse="{yn(c["disperse"])}">']
    out += [f'\t<gene length="{L!r}" theta="{th!r}" color="{col}" />' for L, th, col in c["genes"]]
    return "\n".join(out + ["</genetic_code>\n"])


def code_key(c):
    return json.dumps([c["sym"], c["mirror"], c["disperse"],
                       [(round(L, 9), round(t, 9), k) for L, t, k in c["genes"]]])


def describe(c):
    cols = {}
    for _, _, k in c["genes"]:
        cols[k] = cols.get(k, 0) + 1
    parts = ", ".join(f"{n} {k}" for k, n in sorted(cols.items(), key=lambda x: (-x[1], x[0])))
    return (f'symmetry {c["sym"]}, {len(c["genes"])} gene{"s" if len(c["genes"]) > 1 else ""} '
            f'({parts}), mirror {"yes" if c["mirror"] else "no"}, '
            f'disperse {"yes" if c["disperse"] else "no"}')


# ---- the tournament record ----

class Tournament:
    def __init__(self):
        self.cands = []  # dicts: id, code, origin, kind, parents, op, phase, gen, scores
        self.by_key = {}
        self.meta = {}

    def add(self, code, kind, origin, parents=(), op=None, phase=1, gen=0):
        k = code_key(code)
        if k in self.by_key:
            return None
        c = {"id": len(self.cands), "code": code, "kind": kind, "origin": origin,
             "parents": list(parents), "op": op, "phase": phase, "gen": gen, "scores": {}}
        self.cands.append(c)
        self.by_key[k] = c
        return c

    def save(self, extra=None):
        OUT.mkdir(parents=True, exist_ok=True)
        data = {"candidates": self.cands}
        data.update(self.meta)
        data.update(extra or {})
        (OUT / "tournament.json").write_text(json.dumps(data, indent=1))


def run_one(task):
    bgg, world, rseed, frames = task
    out = sh([TOOL, "eval", bgg, str(world), str(rseed), str(frames)])
    end = [l for l in out.splitlines() if l.startswith("END")]
    p = end[-1].split() if end else ["END", "0", "0", "-1", "0"]
    return [int(p[1]), int(p[2]), int(p[3]), int(p[4])]


def evaluate(t, cands, stage, worlds, seeds, frames):
    """Stores scores[stage] = {mean, success, share, runs} on every candidate."""
    tmp = Path(tempfile.mkdtemp(dir=BUILD))
    tasks, owner = [], []
    want = len(worlds) * len(seeds)
    cached = [c for c in cands if (code_key(c["code"]), stage) in CACHE
              and len(CACHE[(code_key(c["code"]), stage)]["runs"]) == want
              and CACHE[(code_key(c["code"]), stage)].get("frames", frames) == frames]
    for c in cached:
        c["scores"][stage] = CACHE[(code_key(c["code"]), stage)]
    cands = [c for c in cands if c not in cached]
    for c in cands:
        f = tmp / f'{c["id"]}.bgg'
        f.write_text(to_bgg(c["code"]))
        for w in worlds:
            for r in seeds:
                tasks.append((str(f), world_path(w), r, frames))
                owner.append(c)
    runs = {c["id"]: [] for c in cands}
    for c, r in zip(owner, parallel(run_one, tasks)):
        runs[c["id"]].append(r)
    shutil.rmtree(tmp)
    for c in cands:
        rs = runs[c["id"]]
        lin = [r[0] for r in rs]
        c["scores"][stage] = {"mean": statistics.mean(lin), "success": sum(x > 0 for x in lin) / len(lin),
                              "share": statistics.mean(r[0] / r[1] if r[1] else 0 for r in rs),
                              "runs": rs}
    for c in cands + cached:
        sc = c["scores"][stage]
        sc["frames"] = frames
        sc["score"] = statistics.mean(r[0] + 1 if r[2] == 0 else r[2] / frames for r in sc["runs"])


def log(msg):
    print(msg, flush=True)
    with open(OUT / "log.txt", "a") as f:
        f.write(msg + "\n")


def line(c, stage):
    s = c["scores"][stage]
    lasted = statistics.mean(r[2] if r[2] else s["frames"] for r in s["runs"])
    return (f'  #{c["id"]:<4} {s["mean"]:7.1f} alive {s["success"]:4.0%} share {s["share"]:4.0%}'
            f' score {s["score"]:5.2f} lasted {lasted:6.0f}  {c["origin"]}: {describe(c["code"])}')


# ---- variation ----

def rand_gene(rng):
    return (2 + 16 * rng.random(), 2 * math.pi * rng.random(), rng.choice(COLORS))


def mutate(code, rng):
    c = {"sym": code["sym"], "mirror": code["mirror"], "disperse": code["disperse"],
         "genes": list(code["genes"])}
    for _ in range(rng.choice([1, 1, 2, 3])):
        g = c["genes"]
        op = rng.randrange(10)
        i = rng.randrange(len(g))
        L, th, col = g[i]
        if op == 0:
            g[i] = (L, th, rng.choice(COLORS))
        elif op == 1:
            g[i] = (min(30.0, max(1.0, L + rng.gauss(0, 2.5))), th, col)
        elif op == 2:
            g[i] = (L, (th + rng.gauss(0, 0.5)) % (2 * math.pi), col)
        elif op == 3 and len(g) < 12:
            g.insert(rng.randrange(len(g) + 1), rand_gene(rng))
        elif op == 4 and len(g) > 1:
            del g[i]
        elif op == 5 and len(g) < 12:
            g.insert(i, g[i])
        elif op == 6:
            c["sym"] = min(8, max(1, c["sym"] + rng.choice([-1, 1])))
        elif op == 7:
            c["mirror"] = not c["mirror"]
        elif op == 8:
            c["disperse"] = not c["disperse"]
        elif op == 9 and len(g) > 1:
            j = rng.randrange(len(g))
            g[i], g[j] = g[j], g[i]
    return c


def crossover(a, b, rng):
    genes = a["genes"][: rng.randint(1, len(a["genes"]))] + b["genes"][rng.randint(0, len(b["genes"]) - 1):]
    p = a if rng.random() < 0.5 else b
    return {"sym": p["sym"], "mirror": p["mirror"], "disperse": p["disperse"], "genes": genes[:12]}


def designs():
    D = lambda name, sym, m, d, genes: (name, {"sym": sym, "mirror": m, "disperse": d,
                                              "genes": [(float(L), float(t), k) for L, t, k in genes]})
    return [
        D("green star", 8, False, True, [(18, 0, "green")]),
        D("double green star", 4, False, True, [(18, 0, "green"), (10, 1.0, "green")]),
        D("armoured plant", 6, False, True, [(14, 0, "green"), (6, 0.5, "blue")]),
        D("virus plant", 5, False, True, [(12, 0, "green"), (8, 0.3, "white")]),
        D("killer plant", 4, True, True, [(14, 0, "green"), (10, 0.6, "gray"), (5, 0.2, "blue")]),
        D("hunter", 3, False, False, [(10, 0, "red"), (12, 1.0, "cyan"), (8, 2, "green")]),
        D("breeder", 4, False, True, [(14, 0, "green"), (4, 0.4, "yellow")]),
        D("swimming plant", 3, False, True, [(16, 0, "green"), (10, 0.8, "cyan")]),
        D("shielded virus", 6, True, True, [(16, 0, "green"), (8, 1.2, "white"), (6, 0.4, "blue")]),
        D("long breeder", 2, False, True, [(18, 0, "green"), (18, 3.14, "green"), (6, 1.57, "yellow")]),
        D("small armoured star", 8, False, True, [(10, 0, "green"), (4, 0.2, "blue")]),
        D("virus killer", 5, True, True, [(12, 0, "green"), (9, 0.7, "white"), (9, -0.7, "gray")]),
    ]


def armoured():
    """phase 2: designs for a world of gray hunters"""
    D = lambda name, sym, m, d, genes: (name, {"sym": sym, "mirror": m, "disperse": d,
                                              "genes": [(float(L), float(t), k) for L, t, k in genes]})
    return [
        D("blue fortress", 8, False, True, [(10, 0, "blue"), (14, 0.3, "green")]),
        D("armoured killer", 6, False, True, [(12, 0, "green"), (8, 0.4, "blue"), (6, 0.8, "gray")]),
        D("shielded swimmer", 3, True, True, [(14, 0, "green"), (9, 0.5, "blue"), (10, 1.2, "cyan")]),
    ]


def breed(t, rng, parents, stage, n, phase, gen, metric="mean"):
    kids = []
    tries = 0
    while len(kids) < n and tries < 1000:
        tries += 1
        pick = lambda: max(rng.sample(parents, 3), key=lambda c: c["scores"][stage][metric])
        if rng.random() < 0.3:
            a, b = pick(), pick()
            code, op, ps = crossover(a["code"], b["code"], rng), "crossover", [a["id"], b["id"]]
            if rng.random() < 0.5:
                code, op = mutate(code, rng), "crossover+mutation"
        else:
            a = pick()
            code, op, ps = mutate(a["code"], rng), "mutation", [a["id"]]
        c = t.add(code, "bred", f"phase {phase} generation {gen}", ps, op, phase, gen)
        if c:
            kids.append(c)
    return kids


def run(args):
    rng = random.Random(2026)
    OUT.mkdir(parents=True, exist_ok=True)
    sha = hashlib.sha256(Path(args.world).read_bytes()).hexdigest() if args.world else None
    if (OUT / "tournament.json").exists():
        old = json.loads((OUT / "tournament.json").read_text())
        same_world = old.get("world_sha") in (None, sha)  # results in the world are only good for that world
        for c in old["candidates"]:
            for st, sc in c["scores"].items():
                if same_world or st not in ("world", "final"):
                    CACHE[(code_key(c["code"]), st)] = sc
    (OUT / "log.txt").write_text("")
    BUILD.mkdir(parents=True, exist_ok=True)
    t = Tournament()
    t.meta["world_sha"] = sha

    for p in sorted(glob.glob(str(BUILD / "natural" / "*.bgg"))):
        s, f, r = re.match(r"e(\d+)_(\d+)_(\d+)", Path(p).stem).groups()
        t.add(parse_bgg(p), "natural", f"evolution {s}, frame {int(f):,}, rank {int(r) + 1}")
    for w in TRAIN:
        d = BUILD / "harvest" / f"w{w[0]}_{w[1]}"
        d.mkdir(parents=True, exist_ok=True)
        sh([TOOL, "harvest", str(world_path(w)), str(d), "3"])
        for i, p in enumerate(sorted(glob.glob(str(d / "*.bgg")))):
            t.add(parse_bgg(p), "world", f"world {w[0]} at {w[1]:,} frames, rank {i + 1}")
    for name, code in designs():
        t.add(code, "design", name)
    for i in range(20):
        t.add({"sym": rng.randint(1, 8), "mirror": rng.random() < 0.5, "disperse": rng.random() < 0.5,
               "genes": [rand_gene(rng) for _ in range(rng.randint(1, 6))]}, "random", f"random {i + 1}")
    pool = list(t.cands)

    log(f"screening {len(pool)} candidates: 8 training worlds x 1 import, 15,000 frames")
    evaluate(t, pool, "screen", TRAIN[::2], [1], 15000)
    pool.sort(key=lambda c: -c["scores"]["screen"]["mean"])
    for c in pool[:16]:
        log(line(c, "screen"))
    t.save()

    log("the best 16: 16 training worlds x 2 imports, 30,000 frames")
    alive = pool[:16]
    evaluate(t, alive, "train", TRAIN, [1, 2], 30000)
    for gen in range(1, PHASE1_GENS + 1):
        alive.sort(key=lambda c: -c["scores"]["train"]["mean"])
        kids = breed(t, rng, alive[:10], "train", 24, 1, gen)
        evaluate(t, kids, "train", TRAIN, [1, 2], 30000)
        alive += kids
        alive.sort(key=lambda c: -c["scores"]["train"]["mean"])
        log(f"generation {gen}:" + line(alive[0], "train"))
        t.save()

    finalists = alive[:6]
    log("phase 1 finalists on 10 held-out worlds x 6 imports, 50,000 frames")
    evaluate(t, finalists, "held", HELD, [11, 12, 13, 14, 15, 16], 50000)
    for c in sorted(finalists, key=lambda c: -c["scores"]["held"]["mean"]):
        log(line(c, "held"))
    champion = max(finalists, key=lambda c: c["scores"]["held"]["mean"])
    t.save({"champion": champion["id"], "world": None})

    if args.world:
        user = BUILD / "worlds" / "user.sbw"
        shutil.copyfile(args.world, user)
        seeds = PHASE2_SEEDS
        log(f"phase 2 in {Path(args.world).name}: {len(seeds)} imports, {PHASE2_FRAMES:,} frames")
        alive2 = sorted(alive, key=lambda c: -c["scores"]["train"]["mean"])[:10]
        d = BUILD / "harvest" / "user"
        d.mkdir(parents=True, exist_ok=True)
        sh([TOOL, "harvest", str(user), str(d), "10"])
        for p in sorted(d.glob("h*.bgg"), key=lambda p: int(p.stem[1:])):
            c = t.add(parse_bgg(p), "yours", f"{Path(args.world).name}, rank {int(p.stem[1:]) + 1}", phase=2)
            if c:
                alive2.append(c)
        for name, code in armoured():
            c = t.add(code, "design", name, phase=2)
            if c:
                alive2.append(c)
        evaluate(t, alive2, "world", [user], seeds, PHASE2_FRAMES)
        for gen in range(1, PHASE2_GENS + 1):
            alive2.sort(key=lambda c: -c["scores"]["world"]["score"])
            kids = breed(t, rng, alive2[:10], "world", 18, 2, gen, "score")
            evaluate(t, kids, "world", [user], seeds, PHASE2_FRAMES)
            alive2 += kids
            alive2.sort(key=lambda c: -c["scores"]["world"]["score"])
            log(f"phase 2 generation {gen}:" + line(alive2[0], "world"))
            t.save()
        finalists2 = alive2[:6]
        # the best natural champion and design, for comparison
        best_nat = max((c for c in t.cands if c["kind"] in ("natural", "world")),
                       key=lambda c: c["scores"]["screen"]["mean"])
        best_yours = max((c for c in t.cands if c["kind"] == "yours"),
                         key=lambda c: c["scores"]["world"]["score"], default=None)
        best_design = max((c for c in t.cands if c["kind"] == "design" and "screen" in c["scores"]),
                          key=lambda c: c["scores"]["screen"]["mean"])
        extra = [c for c in (best_nat, best_design, best_yours) if c and c not in finalists2]
        log(f"phase 2 finalists in the world: {len(FINAL_SEEDS)} held-out imports, {FINAL_FRAMES:,} frames")
        evaluate(t, finalists2 + extra, "final", [user], FINAL_SEEDS, FINAL_FRAMES)
        for c in sorted(finalists2 + extra, key=lambda c: -c["scores"]["final"]["score"]):
            log(line(c, "final"))
        champion = max(finalists2, key=lambda c: c["scores"]["final"]["score"])
        t.save({"champion": champion["id"], "world": Path(args.world).name,
                "compared": [c["id"] for c in extra]})

    (OUT / "ultimate.bgg").write_text(to_bgg(champion["code"]))
    log(f'champion #{champion["id"]}: {describe(champion["code"])}')


def growth(args):
    """The champion and the best natural champion and design imported into
    the world: lineage every 500 frames from the final round's spots."""
    data = json.loads((OUT / "tournament.json").read_text())
    cands = {c["id"]: c for c in data["candidates"]}
    ids = [data["champion"]] + data.get("compared", [])
    world = BUILD / "worlds" / "user.sbw"
    rows = []
    for cid in ids:
        f = BUILD / f"g{cid}.bgg"
        f.write_text(to_bgg(cands[cid]["code"]))
        outs = parallel(lambda r: sh([TOOL, "eval", str(f), str(world), str(r), str(FINAL_FRAMES), "500"]),
                        FINAL_SEEDS)
        for r, out in zip(FINAL_SEEDS, outs):
            series = {0: 1}
            for l in out.splitlines():
                p = l.split()
                if p[0] == "T":
                    series[int(p[1])] = int(p[2])
            for fr in range(0, FINAL_FRAMES + 1, 500):
                rows.append((cid, r, fr, series.get(fr, 0)))
    with open(OUT / "growth.csv", "w") as fh:
        fh.write("candidate,import_seed,frame,lineage\n")
        for row in rows:
            fh.write(",".join(map(str, row)) + "\n")


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("what", choices=["natural", "worlds", "run", "growth"])
    ap.add_argument("--world", help="a saved world (.sbw or .bgw) to breed and judge in")
    ap.add_argument("--nice", type=int, default=19)
    a = ap.parse_args()
    NICE = a.nice
    {"natural": lambda: make_natural(), "worlds": lambda: make_worlds(),
     "run": lambda: run(a), "growth": lambda: growth(a)}[a.what]()
