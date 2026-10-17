#!/bin/sh
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

# Builds the Java reference: the original Biogenesis 0.9 simulation classes
# plus Stubs.java and Harness.java, compiled into tools/javaref/build/.
#
#   tools/javaref/build.sh [--strict] SRC_DIR JTS_JAR
#
# --strict swaps Math.sin/cos/atan2/pow for StrictMath (fdlibm), into
# build-strict/, to compare against `make strict`.
#
# SRC_DIR is the biogenesis/ source directory from biogenesis_src_0_9.zip;
# JTS_JAR is jts-core-1.15.0.jar (nested inside the 0.9 biogenesis.jar).
# fetch.sh downloads both.
#
# One change is made to the original: World.checkHit visits the STRtree's
# candidates in list order rather than tree order (the C port's order, and
# 0.8's), so a seed gives one well-defined result. Organism gets a field to
# carry that order.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
strict=0
if [ "${1:-}" = --strict ]; then
	strict=1
	shift
fi
src=$1
jts=$2
out="$here/build"
[ $strict = 1 ] && out="$here/build-strict"
rm -rf "$out"
mkdir -p "$out/src/biogenesis" "$out/classes"
for f in Organism World GeneticCode Gene Utils Vector2D ExLine2DDouble WorldStatistics BioXMLParser; do
	cp "$src/$f.java" "$out/src/biogenesis/"
done
python3 -I - "$out/src/biogenesis" $strict <<'EOF'
import re, sys, pathlib
d = pathlib.Path(sys.argv[1])
if sys.argv[2] == '1':
    for p in d.glob('*.java'):
        s = p.read_text(encoding='latin-1')
        s = re.sub(r'(?<!Strict)Math\.(sin|cos|atan2|pow)\(', r'StrictMath.\1(', s)
        p.write_text(s, encoding='latin-1')
def patch(name, old, new):
    p = d / name
    s = p.read_text(encoding='latin-1').replace('\r\n', '\n')
    if old not in s:
        sys.exit(f"patch target not found in {name}: {old!r}")
    p.write_text(s.replace(old, new, 1), encoding='latin-1')
patch('Organism.java', 'protected boolean alive = true;',
      'protected boolean alive = true;\n\tint _treeIdx;')
patch('World.java',
      'for (Organism o: _organisms) {\n',
      'int treeIdx = 0;\n\t\t\tfor (Organism o: _organisms) {\n\t\t\t\to._treeIdx = treeIdx++;\n')
patch('World.java',
      'for (Organism collidingOrganism : collidingOrgs) {',
      'collidingOrgs.sort((p, q) -> Integer.compare(p._treeIdx, q._treeIdx));\n'
      '\t\tfor (Organism collidingOrganism : collidingOrgs) {')
EOF
cp "$here/Stubs.java" "$here/Harness.java" "$here/BggInterop.java" "$out/src/biogenesis/"
if ! javac -nowarn -encoding ISO-8859-1 -cp "$jts" -d "$out/classes" "$out"/src/biogenesis/*.java \
	2>"$out/javac.log"; then
	cat "$out/javac.log" >&2
	exit 1
fi
cp "$jts" "$out/jts.jar"
echo "built $out/classes"
