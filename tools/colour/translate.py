# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""Writes the colour build's generated C from Biogenesis Color Mod 2.01's
Java source (tools/javaref/fetch.sh downloads it):

  src/colour/sim/organism.{c,h}, genetic_code.{c,h}   (j2c.py, j2c_emit.py)
  src/colour/sim/params.{c,h}                          (j2c_params.py)
  src/colour/ui/params_fields.h                        (j2c_paramdialog.py)
  src/colour/io/bgw_classes.h                          (j2c_bgw.py)

Usage: translate.py [JAVA_SRC_DIR]
"""

import os
import sys

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)

from j2c import TranslateError  # noqa: E402
from j2c_emit import translate_all  # noqa: E402
import j2c_bgw  # noqa: E402
import j2c_paramdialog  # noqa: E402
from jparse import JavaError  # noqa: E402


def main():
    root = os.path.normpath(os.path.join(here, '..', '..'))
    src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        root, 'tools', 'javaref', 'download', 'colormod-src', 'src', 'biogenesis')
    if not os.path.isdir(src):
        print(f'{src}: not found (run tools/javaref/fetch.sh)', file=sys.stderr)
        sys.exit(1)
    try:
        classes = translate_all(src, os.path.join(root, 'src', 'colour', 'sim'))
        j2c_bgw.generate(classes, os.path.join(root, 'src', 'colour', 'io', 'bgw_classes.h'))
        j2c_paramdialog.generate(src, os.path.join(root, 'src', 'colour', 'ui',
                                                   'params_fields.h'))
    except (JavaError, TranslateError) as ex:
        print(f'error: {ex}', file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
