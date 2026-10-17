#!/bin/sh
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

# Runs the same seed through the Java original and the C port and reports
# the first frame where their states differ.
#
#   tools/javaref/compare.sh [--strict] SEED FRAMES [EVERY]
#
# Needs tools/javaref/build/ (fetch.sh) and ./syn-biogenesis-headless, or
# with --strict build-strict/ and ./syn-biogenesis-strict (make strict).
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
jbuild="$here/build"
cbin="$root/syn-biogenesis-headless"
if [ "${1:-}" = --strict ]; then
	jbuild="$here/build-strict"
	cbin="$root/syn-biogenesis-strict"
	shift
fi
seed=$1
frames=$2
every=${3:-1}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
java -cp "$jbuild/classes:$jbuild/jts.jar" biogenesis.Harness "$seed" "$frames" "$tmp/java.txt" "$every"
"$cbin" --headless --defaults --quiet --seed "$seed" --frames "$frames" \
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
