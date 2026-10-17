#!/bin/sh
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

# make test: file format checks, java.awt geometry against the JVM, and the
# frame-by-frame comparison with the Java original (skipped without a JDK
# or without tools/javaref/fetch.sh having been run).
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
status=0

tests/sim_check "$tmp" || status=1

if command -v javac >/dev/null 2>&1; then
	javac -d "$tmp" tests/GeomCheck.java
	java -cp "$tmp" GeomCheck 31337 200000 >"$tmp/geom_java.txt"
	tests/geom_check 31337 200000 >"$tmp/geom_c.txt"
	if cmp -s "$tmp/geom_java.txt" "$tmp/geom_c.txt"; then
		echo "java.awt geometry: 200000 random cases identical to the JVM"
	else
		echo "FAIL java.awt geometry differs from the JVM:"
		diff "$tmp/geom_java.txt" "$tmp/geom_c.txt" | head -5
		status=1
	fi
else
	echo "skip: no JDK for the geometry check"
fi

if [ -d tools/javaref/build/classes ]; then
	# .bgg files cross both ways: each side reads what the other wrote
	# (Java validating against its DTD) and both print the same codes
	cp="tools/javaref/build/classes:tools/javaref/build/jts.jar"
	mkdir -p "$tmp/bgg"
	java -cp "$cp" biogenesis.BggInterop write "$tmp/bgg" 2024 50
	tests/bgg_tool write "$tmp/bgg" 2024 50
	java -cp "$cp" biogenesis.BggInterop read "$tmp"/bgg/java_*.bgg >"$tmp/jj.txt"
	tests/bgg_tool read "$tmp"/bgg/java_*.bgg >"$tmp/jc.txt"
	java -cp "$cp" biogenesis.BggInterop read "$tmp"/bgg/c_*.bgg >"$tmp/cj.txt"
	tests/bgg_tool read "$tmp"/bgg/c_*.bgg >"$tmp/cc.txt"
	if cmp -s "$tmp/jj.txt" "$tmp/jc.txt" && cmp -s "$tmp/cj.txt" "$tmp/cc.txt" &&
		cmp -s "$tmp/jj.txt" "$tmp/cc.txt"; then
		echo ".bgg interchange: 50 Java-written and 50 C-written codes read identically by both"
	else
		echo "FAIL .bgg interchange differs"
		status=1
	fi
fi

if [ -d tools/javaref/build-strict/classes ] && [ -x ./syn-biogenesis-strict ]; then
	for seed in 1 42 1234; do
		tools/javaref/compare.sh --strict "$seed" 3000 || status=1
	done
else
	echo "skip: Java reference not built (tools/javaref/fetch.sh, make strict)"
fi
exit $status
