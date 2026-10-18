# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""The SYN-OS theme the pictures are drawn in: the one ~/.config/syn-os/
current-theme names (a flat KEY="#hex" file, see syn-theme-apply), or
SYN-OS-M141's colours where there is none."""

import re
from pathlib import Path

M141 = {"SYN_THEME_NAME": "SYN-OS-M141", "SYN_BG": "#000000", "SYN_BG_ALT": "#0a0a0a",
        "SYN_PANEL": "#1a0000", "SYN_PANEL_HOVER": "#330000", "SYN_ACCENT": "#e00000",
        "SYN_ACCENT_DIM": "#4d0000", "SYN_TEXT": "#ffffff", "SYN_BORDER": "#555555",
        "SYN_URGENT": "#ff2020"}


def theme():
    t = dict(M141)
    d = Path.home() / ".config" / "syn-os"
    try:
        name = (d / "current-theme").read_text().strip()
        for line in (d / "themes" / f"{name}.theme").read_text().splitlines():
            m = re.match(r'(SYN_[A-Z_]+)="(#[0-9a-fA-F]{6})"', line)
            if m:
                t[m.group(1)] = m.group(2)
        t["SYN_THEME_NAME"] = name
    except OSError:
        pass
    return t
