#!/usr/bin/env python3
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""The drift study: how long one seed's world stays the same world.

Runs the shipped Biogenesis 0.9 jar on OpenJDK 27 (the reference) and a set
of other implementations on the same seeds, compares their state every frame
(by hash, see HashRun.java and `syn-biogenesis --headless --hash`) and writes
the results and charts to docs/drift/:

  results.csv      per seed and variant: the first frame that differs, the
                   number of differences that healed, and the frame from
                   which the worlds never agree again
  population.csv   population over time for one seed, every variant
  growth.csv       how far apart two worlds drift after they part, from
                   full state dumps (median over seeds)
  *.svg            the charts in the README, light and dark

    tools/drift/drift.py [--seeds N] [--frames N] [--jobs N] [--growth-only]

--growth-only reuses results.csv and redoes only growth.csv and the charts.

Needs tools/javaref/fetch.sh to have run, `make headless libc`, and for
Java 11/17/21 tools/drift/fetch-jdks.sh. Variants whose JVM is missing are
skipped.
"""

import argparse
import concurrent.futures as cf
import csv
import os
import statistics
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HERE = ROOT / "tools" / "drift"
BUILD = HERE / "build"
REF = ROOT / "tools" / "javaref" / "build"
OUT = ROOT / "docs" / "drift"
JAVA8 = Path("/usr/lib/jvm/java-8-openjdk/jre/bin/java")

POPULATION_SEED = 42
GROWTH_SEEDS = 10
GROWTH_FRAMES = 3000

# name, label, how it runs: ("java", [jvm and flags]) or ("c", binary)
VARIANTS = [
    ("java27", "Java 27 (reference)", ("java", ["java"])),
    ("port", "SYN-BIOGENESIS", ("c", ROOT / "syn-biogenesis-headless")),
    ("port-libc", "SYN-BIOGENESIS, C library maths", ("c", ROOT / "syn-biogenesis-headless-libc")),
    ("java8", "Java 8", ("java", [str(JAVA8)])),
    ("java27-fdlibm", "Java 27, fdlibm maths",
     ("java", ["java", "-XX:+UnlockDiagnosticVMOptions", "-XX:-UseLibmIntrinsic"])),
    ("java11", "Java 11", ("java", [str(HERE / "jdk" / "11" / "bin" / "java")])),
    ("java17", "Java 17", ("java", [str(HERE / "jdk" / "17" / "bin" / "java")])),
    ("java21", "Java 21", ("java", [str(HERE / "jdk" / "21" / "bin" / "java")])),
]
GROWTH_VARIANTS = ["port-libc", "java8", "java27-fdlibm"]


def classpath():
    return f"{BUILD / 'classes'}:{REF / 'biogenesis.jar'}:{REF / 'jts.jar'}"


def available(v):
    kind, how = v[2]
    exe = how[0] if kind == "java" else how
    if kind == "java" and exe == "java":
        return True
    return Path(exe).exists()


def version(v):
    kind, how = v[2]
    if kind == "c":
        return "this repository"
    r = subprocess.run(how[:1] + ["-version"], capture_output=True, text=True)
    return r.stderr.splitlines()[0] if r.stderr else "?"


def build():
    if not (REF / "biogenesis.jar").exists():
        sys.exit("drift: run tools/javaref/fetch.sh first")
    (BUILD / "classes").mkdir(parents=True, exist_ok=True)
    # --release 8: the same classes run on every JVM in the study
    subprocess.run(["javac", "--release", "8", "-nowarn", "-Xlint:-options", "-cp",
                    f"{REF / 'biogenesis.jar'}:{REF / 'jts.jar'}", "-d", str(BUILD / "classes"),
                    str(ROOT / "tools" / "javaref" / "Stubs.java"),
                    str(ROOT / "tools" / "javaref" / "Harness.java"),
                    str(HERE / "HashRun.java")], check=True)
    subprocess.run(["make", "-C", str(ROOT), "-s", "headless", "libc"], check=True)


def run(v, seed, frames, out, dump=False):
    kind, how = v[2]
    if kind == "java":
        cls = ["biogenesis.Harness", str(seed), str(frames), str(out)] if dump else \
              ["biogenesis.HashRun", str(seed), str(frames), str(out)]
        cmd = how + ["-cp", classpath()] + cls
    else:
        cmd = [str(how), "--headless", "--defaults", "--quiet", "--seed", str(seed),
               "--frames", str(frames), "--dump" if dump else "--hash", str(out)]
    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL)


def read_hashes(path):
    rows = []
    with open(path) as f:
        for line in f:
            frame, pop, norgs, h = line.split()
            rows.append((int(pop), int(norgs), h))
    return rows


def compare(ref, other):
    """First differing frame, healed episodes, and the frame from which the
    two never agree again (None when they agree at the last frame)."""
    first = None
    episodes = 0
    start = None
    for f, (a, b) in enumerate(zip(ref, other)):
        if a != b:
            if start is None:
                start = f
                episodes += 1
            if first is None:
                first = f
        else:
            start = None
    permanent = start
    healed = episodes - (1 if permanent is not None else 0)
    return first, healed, permanent


def seed_job(seed, frames, variants):
    rows, pops = [], {}
    with tempfile.TemporaryDirectory() as tmp:
        ref_path = Path(tmp) / "ref.txt"
        run(variants[0], seed, frames, ref_path)
        ref = read_hashes(ref_path)
        if seed == POPULATION_SEED:
            pops[variants[0][0]] = [r[0] for r in ref]
        for v in variants[1:]:
            p = Path(tmp) / f"{v[0]}.txt"
            run(v, seed, frames, p)
            other = read_hashes(p)
            first, healed, permanent = compare(ref, other)
            rows.append({"seed": seed, "variant": v[0], "frames": frames,
                         "first_difference": "" if first is None else first,
                         "healed": healed,
                         "permanent_divergence": "" if permanent is None else permanent})
            if seed == POPULATION_SEED:
                pops[v[0]] = [r[0] for r in other]
            p.unlink()
    return rows, pops


def dump_frames(path, start, end):
    """Yields (frame, {id: (cx, cy)}, set of organism lines) for start..end."""
    cur, orgs, lines = None, {}, set()
    with open(path) as f:
        for line in f:
            if line.startswith("F "):
                if cur is not None and start <= cur <= end:
                    yield cur, orgs, lines
                cur = int(line.split()[1])
                orgs, lines = {}, set()
                if cur > end:
                    return
            elif cur is not None and cur >= start:
                parts = line.split()
                c = parts[4][2:].split(",")
                orgs[int(parts[1])] = (struct.unpack(">d", bytes.fromhex(c[0]))[0],
                                       struct.unpack(">d", bytes.fromhex(c[1]))[0])
                lines.add(line)
    if cur is not None and start <= cur <= end:
        yield cur, orgs, lines


def growth_job(v, seed, onset, ref_v):
    """Per frame from the onset of permanent divergence: the largest position
    difference between organisms present in both worlds, and the share of
    organisms whose state differs or exists in one world only."""
    end = onset + GROWTH_FRAMES
    with tempfile.TemporaryDirectory() as tmp:
        a, b = Path(tmp) / "a.txt", Path(tmp) / "b.txt"
        run(ref_v, seed, end, a, dump=True)
        run(v, seed, end, b, dump=True)
        out = []
        for (fa, oa, la), (fb, ob, lb) in zip(dump_frames(a, onset, end), dump_frames(b, onset, end)):
            assert fa == fb
            common = oa.keys() & ob.keys()
            maxdiff = max((max(abs(oa[i][0] - ob[i][0]), abs(oa[i][1] - ob[i][1])) for i in common),
                          default=0.0)
            share = 1 - len(la & lb) / max(1, len(la), len(lb))
            out.append((fa - onset, maxdiff, share))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seeds", type=int, default=100)
    ap.add_argument("--frames", type=int, default=20000)
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) - 1))
    ap.add_argument("--growth-only", action="store_true")
    args = ap.parse_args()

    build()
    variants = [v for v in VARIANTS if available(v)]
    OUT.mkdir(parents=True, exist_ok=True)
    with open(OUT / "variants.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["variant", "label", "version"])
        for v in variants:
            w.writerow([v[0], v[1], version(v)])
    for v in VARIANTS:
        if v not in variants:
            print(f"drift: skipping {v[0]} (not installed)", file=sys.stderr)

    rows, pops = [], {}
    if args.growth_only:
        with open(OUT / "results.csv", newline="") as f:
            rows = list(csv.DictReader(f))
        args.frames = int(rows[0]["frames"])
    with cf.ThreadPoolExecutor(args.jobs) as ex:
        seeds = [] if args.growth_only else range(1, args.seeds + 1)
        futs = [ex.submit(seed_job, s, args.frames, variants) for s in seeds]
        for n, fut in enumerate(cf.as_completed(futs), 1):
            r, p = fut.result()
            rows += r
            pops.update(p)
            print(f"\rseeds {n}/{args.seeds}", end="", file=sys.stderr, flush=True)
    print(file=sys.stderr)
    if not args.growth_only:
        rows.sort(key=lambda r: (r["seed"], r["variant"]))
        with open(OUT / "results.csv", "w", newline="") as f:
            w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
            w.writeheader()
            w.writerows(rows)

    if pops:
        names = [v[0] for v in variants if v[0] in pops]
        with open(OUT / "population.csv", "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["frame"] + names)
            for fr in range(0, args.frames + 1, 20):
                w.writerow([fr] + [pops[n][fr] for n in names])

    # growth: the first seeds (in order) whose worlds part for good early
    # enough to follow them for GROWTH_FRAMES frames
    ref_v = variants[0]
    jobs = []
    for name in GROWTH_VARIANTS:
        v = next((x for x in variants if x[0] == name), None)
        if not v:
            continue
        cands = [r for r in rows if r["variant"] == name and r["permanent_divergence"] != ""
                 and int(r["permanent_divergence"]) + GROWTH_FRAMES <= args.frames]
        cands.sort(key=lambda r: int(r["seed"]))
        for r in cands[:GROWTH_SEEDS]:
            jobs.append((v, r["seed"], int(r["permanent_divergence"])))
    curves = {}
    with cf.ThreadPoolExecutor(max(1, args.jobs // 2)) as ex:
        futs = {ex.submit(growth_job, v, s, onset, ref_v): v[0] for v, s, onset in jobs}
        for n, fut in enumerate(cf.as_completed(futs), 1):
            curves.setdefault(futs[fut], []).append(fut.result())
            print(f"\rgrowth {n}/{len(jobs)}", end="", file=sys.stderr, flush=True)
    print(file=sys.stderr)
    with open(OUT / "growth.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["variant", "frames_after_parting", "seeds", "median_max_position_difference",
                    "median_share_of_organisms_differing"])
        for name, cs in curves.items():
            for k in range(GROWTH_FRAMES + 1):
                pts = [c[k] for c in cs if k < len(c)]
                w.writerow([name, k, len(pts), repr(statistics.median(p[1] for p in pts)),
                            f"{statistics.median(p[2] for p in pts):.4f}"])

    if (HERE / "charts.py").exists():
        subprocess.run([sys.executable, "-I", str(HERE / "charts.py")], check=True)


if __name__ == "__main__":
    main()
