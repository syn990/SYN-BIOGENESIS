#!/usr/bin/env python3
# SYN-BIOGENESIS: the SYN-OS interface's fonts, made from the system's.
#
#   python3 tools/fonts/make.py [TERMINESS_DIR] [FONT_AWESOME_OTF]
#
# SYN Terminal (assets/fonts/SYNTerminal-*.ttf) is Terminess Nerd Font Mono,
# Terminus as SYN-OS's desktop uses it (labwc, mako, foot, wmenu), cut down
# to the characters the interface draws and renamed, as the SIL Open Font
# License asks of a modified font: Terminus' reserved names are "Terminus
# Font" and "Terminus (TTF)". Font Awesome, the icons waybar draws on SYN-OS,
# is copied as it is (assets/fonts/fa-solid-900.otf). Arch: terminus-font's
# ttf-terminus-nerd and otf-font-awesome. Needs fontTools.
import os
import shutil
import sys

from fontTools import subset
from fontTools.ttLib import TTFont

here = os.path.dirname(os.path.abspath(__file__))
out = os.path.join(here, "..", "..", "assets", "fonts")
src = sys.argv[1] if len(sys.argv) > 1 else "/usr/share/fonts/TTF"
fa = sys.argv[2] if len(sys.argv) > 2 else "/usr/share/fonts/OTF/Font Awesome 7 Free-Solid-900.otf"

# what the interface writes: Latin-1, Greek, punctuation, arrows, the
# mathematics the telescope prints, box drawing, blocks and shapes
ranges = [(0x20, 0x7E), (0xA0, 0xFF), (0x391, 0x3C9), (0x2010, 0x2027), (0x2030, 0x203A),
          (0x2070, 0x209C), (0x2190, 0x21FF), (0x2200, 0x22FF), (0x2300, 0x23FF),
          (0x2500, 0x25FF), (0x2713, 0x2718)]
unicodes = [c for a, b in ranges for c in range(a, b + 1)]

for style in ("Regular", "Bold"):
    f = TTFont(os.path.join(src, "TerminessNerdFontMono-%s.ttf" % style))
    opts = subset.Options()
    opts.name_IDs = ["*"]
    opts.name_languages = ["*"]
    opts.notdef_outline = True
    opts.glyph_names = False
    opts.layout_features = ["*"]
    s = subset.Subsetter(opts)
    s.populate(unicodes=unicodes)
    s.subset(f)
    names = {1: "SYN Terminal", 2: style, 3: "SYN Terminal %s" % style,
             4: "SYN Terminal %s" % style, 6: "SYNTerminal-%s" % style,
             16: "SYN Terminal", 17: style}
    for rec in list(f["name"].names):
        if rec.nameID in names:
            rec.string = names[rec.nameID]
        elif rec.nameID in (18, 21, 22):
            f["name"].names.remove(rec)
    f.save(os.path.join(out, "SYNTerminal-%s.ttf" % style))

shutil.copyfile(fa, os.path.join(out, "fa-solid-900.otf"))
