#!/usr/bin/env python3
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""Every competitor in the being search (docs/being/tournament.json), drawn
as the genetic laboratory draws it (being layout), in the SYN-OS theme:

  docs/being/competitors.svg   all of them, grouped by how far they got
  docs/being/beings/ID.svg     the finalists and the champion, larger
  build/png/ID.png             thumbnails for tournament.py

    pictures.py
"""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from search import BUILD, OUT, TOOL, describe, to_bgg  # noqa: E402
from theme import theme  # noqa: E402

FONT = "system-ui, -apple-system, 'Segoe UI', sans-serif"
STAGES = [("final", "in your world"), ("world", "phase 2"), ("held", "held-out worlds"),
          ("train", "training worlds"), ("screen", "screening")]


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def layout(code):
    with tempfile.NamedTemporaryFile("w", suffix=".bgg", dir=BUILD, delete=False) as f:
        f.write(to_bgg(code))
    out = subprocess.run([TOOL, "layout", f.name], capture_output=True, text=True).stdout
    Path(f.name).unlink()
    return [(int(a), int(b), int(c), int(d), col) for a, b, c, d, col in
            (l.split() for l in out.splitlines())]


def drawing(lines, x, y, size, width=1.6):
    """the being centred in a size x size square at (x, y)"""
    if not lines:
        return ""
    xs = [v for l in lines for v in (l[0], l[2])]
    ys = [v for l in lines for v in (l[1], l[3])]
    span = max(max(xs) - min(xs), max(ys) - min(ys), 1)
    k = (size - 8) / span
    cx, cy = (max(xs) + min(xs)) / 2, (max(ys) + min(ys)) / 2
    p = lambda vx, vy: (x + size / 2 + (vx - cx) * k, y + size / 2 + (vy - cy) * k)
    out = []
    for x0, y0, x1, y1, col in lines:
        a, b = p(x0, y0)
        c, d = p(x1, y1)
        out.append(f'<line x1="{a:.1f}" y1="{b:.1f}" x2="{c:.1f}" y2="{d:.1f}" stroke="{col}" '
                   f'stroke-width="{width}" stroke-linecap="round"/>')
    return "".join(out)


def reached(c):
    for i, (st, _) in enumerate(STAGES):
        if st in c["scores"]:
            return i, st
    return len(STAGES), None


def group(c, data):
    if c["id"] == data.get("champion"):
        return 0, "The champion"
    i, st = reached(c)
    names = ["Finalists in your world", "Bred in your world (phase 2)", "Finalists of phase 1",
             "Bred and scored on the training worlds", "Screened out"]
    return i + 1, names[i] if i < len(names) else "Not scored"


def main():
    t = theme()
    data = json.loads((OUT / "tournament.json").read_text())
    cands = data["candidates"]
    groups = {}
    for c in cands:
        groups.setdefault(group(c, data), []).append(c)

    cols, cw, ch, pad, top = 12, 118, 150, 24, 96
    rows = []
    y = top
    body = []
    for (gi, name), cs in sorted(groups.items()):
        st = reached(cs[0])[1] or "screen"
        cs.sort(key=lambda c: -c["scores"].get(st, {"mean": 0})["mean"])
        body.append(f'<text x="{pad}" y="{y + 18}" font-size="16" font-weight="600" fill="{t["SYN_TEXT"]}">'
                    f'{esc(name)} ({len(cs)})</text>')
        y += 30
        for i, c in enumerate(cs):
            x = pad + (i % cols) * cw
            yy = y + (i // cols) * ch
            border = t["SYN_ACCENT"] if gi <= 1 else t["SYN_BORDER"]
            body.append(f'<rect x="{x + .5}" y="{yy + .5}" width="{cw - 9}" height="{ch - 9}" rx="6" '
                        f'fill="{t["SYN_PANEL"]}" stroke="{border}" stroke-width="{2 if gi == 0 else 1}"/>')
            body.append(drawing(layout(c["code"]), x + 9, yy + 6, cw - 27))
            s = c["scores"].get(st)
            score = f'{s["mean"]:.1f} · {s["success"]:.0%}' if s else ""
            body.append(f'<text x="{x + 8}" y="{yy + ch - 34}" font-size="11" fill="{t["SYN_TEXT"]}">'
                        f'#{c["id"]} {esc(c["kind"])}</text>')
            body.append(f'<text x="{x + 8}" y="{yy + ch - 19}" font-size="11" fill="{t["SYN_BORDER"]}">'
                        f'{esc(score)}</text>')
        y += ((len(cs) + cols - 1) // cols) * ch + 18
    width = pad * 2 + cols * cw - 9
    head = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{y + 10}" '
            f'viewBox="0 0 {width} {y + 10}" font-family="{FONT}">'
            f'<title>Every competitor in the being search</title>'
            f'<rect width="100%" height="100%" fill="{t["SYN_BG"]}"/>'
            f'<text x="{pad}" y="40" font-size="22" font-weight="600" fill="{t["SYN_TEXT"]}">'
            f'Every competitor, as the genetic laboratory draws it</text>'
            f'<text x="{pad}" y="64" font-size="13" fill="{t["SYN_BORDER"]}">'
            f'{len(cands)} genetic codes. Under each: its number, where it came from, and at the furthest '
            f'stage it reached the mean living lineage per import and the share of imports that '
            f'survived.</text>')
    (OUT / "competitors.svg").write_text(head + "".join(body) + "</svg>\n")

    # the finalists and the champion, larger, and thumbnails for the graph
    (OUT / "beings").mkdir(exist_ok=True)
    (BUILD / "png").mkdir(parents=True, exist_ok=True)
    keep = {c["id"] for c in cands if any(st in c["scores"] for st in ("final", "held"))}
    if data.get("champion") is not None:
        keep.add(data["champion"])
    for c in cands:
        if c["id"] not in keep and not c["scores"].get("train"):
            continue
        lines = layout(c["code"])
        if c["id"] in keep:
            svg = (f'<svg xmlns="http://www.w3.org/2000/svg" width="260" height="290" viewBox="0 0 260 290" '
                   f'font-family="{FONT}"><title>#{c["id"]}: {esc(describe(c["code"]))}</title>'
                   f'<rect width="260" height="290" rx="8" fill="{t["SYN_BG"]}"/>'
                   f'{drawing(lines, 10, 10, 240, 2.2)}'
                   f'<text x="130" y="276" font-size="14" text-anchor="middle" fill="{t["SYN_TEXT"]}">'
                   f'#{c["id"]}</text></svg>\n')
            (OUT / "beings" / f'{c["id"]}.svg').write_text(svg)
        thumb = (f'<svg xmlns="http://www.w3.org/2000/svg" width="72" height="72" viewBox="0 0 72 72">'
                 f'<rect width="72" height="72" fill="{t["SYN_BG"]}"/>{drawing(lines, 0, 0, 72, 1.4)}</svg>')
        svg_path = BUILD / "png" / f'{c["id"]}.svg'
        svg_path.write_text(thumb)
        subprocess.run(["rsvg-convert", "-o", str(BUILD / "png" / f'{c["id"]}.png'), str(svg_path)],
                       check=True)


if __name__ == "__main__":
    main()
