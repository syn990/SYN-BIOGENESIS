#!/bin/sh
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

# Runs the same seed through the shipped Color Mod 2.01 jar and the colour
# build and reports the first frame where their states differ.
#
#   tools/javaref/colour/compare.sh SEED FRAMES [EVERY]
#
# Needs tools/javaref/colour/build/ (../fetch.sh) and
# ./syn-biogenesis (make), which runs Color Mod with --colour.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
jbuild="$here/build"
cbin="$root/syn-biogenesis"
seed=$1
frames=$2
every=${3:-1}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
java -cp "$jbuild/classes:$jbuild/biogenesis.jar:$jbuild/jts.jar" biogenesis.ColourHarness \
	"$seed" "$frames" "$tmp/java.txt" "$every"
"$cbin" --colour --headless --defaults --quiet --seed "$seed" --frames "$frames" \
	--every "$every" --dump "$tmp/c.txt"
if cmp -s "$tmp/java.txt" "$tmp/c.txt"; then
	echo "seed $seed: identical for $frames frames ($(wc -l <"$tmp/java.txt") state lines)"
	exit 0
fi
line=$(cmp "$tmp/java.txt" "$tmp/c.txt" | sed -n 's/.*line \([0-9]*\).*/\1/p')
frame=$(head -n "$line" "$tmp/java.txt" | grep '^F ' | tail -1 | cut -d' ' -f2)
echo "seed $seed: first difference at frame $frame (line $line)"
echo "  java: $(sed -n "${line}p" "$tmp/java.txt")"
echo "  c:    $(sed -n "${line}p" "$tmp/c.txt")"
exit 1
