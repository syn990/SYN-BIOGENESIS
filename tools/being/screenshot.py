#!/usr/bin/env python3
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""The champion in the world it was judged in, as the game shows it
(docs/being/in-your-world.png): its most typical surviving run of the
final round (of the import spots where its line lived to the end, the one
whose lineage is nearest their median) is replayed with the world saved at
the end, and the window, offscreen, opens that world centred where the
champion's lineage is densest.

    screenshot.py [ZOOM]       default zoom 1.5
"""
import json
import os
import statistics
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from search import BUILD, FINAL_FRAMES, FINAL_SEEDS, OUT, ROOT, TOOL, to_bgg  # noqa: E402


def main():
    zoom = sys.argv[1] if len(sys.argv) > 1 else "1.5"
    data = json.loads((OUT / "tournament.json").read_text())
    champ = next(c for c in data["candidates"] if c["id"] == data["champion"])
    runs = [(s, r) for s, r in zip(FINAL_SEEDS, champ["scores"]["final"]["runs"]) if r[0] > 0]
    if not runs:
        sys.exit("the champion's line died out at every import spot of the final round")
    med = statistics.median(r[0] for _, r in runs)
    seed, run = min(runs, key=lambda sr: abs(sr[1][0] - med))
    code = BUILD / "champion.bgg"
    code.write_text(to_bgg(champ["code"]))
    end = BUILD / "champion-world.sbw"
    out = subprocess.run([TOOL, "eval", str(code), str(BUILD / "worlds" / "user.sbw"), str(seed),
                          str(FINAL_FRAMES), str(FINAL_FRAMES), str(end)],
                         capture_output=True, text=True, check=True).stdout
    spot = next(l.split() for l in out.splitlines() if l.startswith("SPOT"))
    env = dict(os.environ, SDL_VIDEODRIVER="offscreen",
               SYN_BIOGENESIS_SHOT=str(OUT / "in-your-world.png"),
               SYN_BIOGENESIS_SHOT_LOAD=str(end),
               SYN_BIOGENESIS_SHOT_VIEW=f"{spot[1]},{spot[2]},{zoom}")
    subprocess.run([str(ROOT / "syn-biogenesis")], env=env, check=True, timeout=300)
    print(f"import spot {seed} (lineage {run[0]}, median of the {len(runs)} surviving runs {med:g}); "
          f"{spot[3]} of the {spot[4]} living organisms within 250 pixels of {spot[1]},{spot[2]} are its lineage")


if __name__ == "__main__":
    main()
