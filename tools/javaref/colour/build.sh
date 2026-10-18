#!/bin/sh
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

# Builds the Color Mod 2.01 reference into tools/javaref/colour/build/:
# ColourHarness.java and the window stubs (../Stubs.java) compiled against
# the shipped Color Mod jar. As for 0.9, the simulation that runs is the
# jar's own bytecode, unmodified.
#
#   tools/javaref/colour/build.sh COLORMOD_JAR JTS_JAR
set -eu
here=$(cd "$(dirname "$0")" && pwd)
jar=$1
jts=$2
out="$here/build"
rm -rf "$out"
mkdir -p "$out/classes"
cp "$jar" "$out/biogenesis.jar"
cp "$jts" "$out/jts.jar"
src="$here/../Stubs.java $here/ColourHarness.java $here/ColourBgwRun.java $here/ColourBggInterop.java"
if ! javac -nowarn -Xlint:-options --release 8 -cp "$out/biogenesis.jar:$out/jts.jar" \
	-d "$out/classes" $src 2>"$out/javac.log" &&
	! javac -nowarn -cp "$out/biogenesis.jar:$out/jts.jar" -d "$out/classes" $src \
		2>"$out/javac.log"; then
	cat "$out/javac.log" >&2
	exit 1
fi
echo "built $out"
