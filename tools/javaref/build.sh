#!/bin/sh
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

# Builds the Java reference into tools/javaref/build/: Harness.java,
# BggInterop.java and Stubs.java compiled against the shipped Biogenesis 0.9
# jar. The simulation that runs is the jar's own bytecode, unmodified; the
# stubs only stand in for the Swing classes (VisibleWorld, MainWindow, ...)
# so it runs without a window, and come first on the class path.
#
#   tools/javaref/build.sh BIOGENESIS_JAR JTS_JAR
#
# fetch.sh downloads both (jts-core-1.15.0.jar is nested in the 0.9 jar).
set -eu
here=$(cd "$(dirname "$0")" && pwd)
jar=$1
jts=$2
out="$here/build"
rm -rf "$out"
mkdir -p "$out/classes"
cp "$jar" "$out/biogenesis.jar"
cp "$jts" "$out/jts.jar"
if ! javac -nowarn -cp "$out/biogenesis.jar:$out/jts.jar" -d "$out/classes" \
	"$here/Stubs.java" "$here/Harness.java" "$here/BggInterop.java" 2>"$out/javac.log"; then
	cat "$out/javac.log" >&2
	exit 1
fi
echo "built $out"
