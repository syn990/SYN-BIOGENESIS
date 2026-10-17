#!/bin/sh
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

# make test: file format checks, java.awt geometry and Math against the JVM,
# and the frame-by-frame comparison with the shipped Java original (skipped
# without a JDK or without tools/javaref/fetch.sh having been run).
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
status=0

tests/sim_check "$tmp" || status=1

# .bgg reader cases (tests/bgg_cases): what 0.9 reads from each, or refuses
n=0
bad=0
for f in tests/bgg_cases/*.bgg; do
	name=$(basename "$f" .bgg)
	want=$(grep "^$name " tests/bgg_cases/expected.txt | cut -d' ' -f2-)
	got=$(tests/bgg_tool read "$f")
	n=$((n + 1))
	if [ "$got" != "$want" ]; then
		echo "  $name: expected ${want%% *}..., read ${got%% *}..."
		bad=$((bad + 1))
	fi
done
if [ $bad = 0 ]; then
	echo ".bgg cases: $n files read as Biogenesis 0.9 reads them"
else
	echo "FAIL .bgg cases: $bad of $n read differently from 0.9"
	status=1
fi

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
	javac -d "$tmp" tests/MathCheck.java
	java -cp "$tmp" MathCheck 2000000 "$tmp/math.bin" 2718
	tests/math_check "$tmp/math.bin" || status=1
	javac -d "$tmp" tests/NumCheck.java
	java -cp "$tmp" NumCheck 300000 "$tmp/num_in.txt" "$tmp/num_out.txt"
	tests/num_check "$tmp/num_in.txt" "$tmp/num_out.txt" || status=1
else
	echo "skip: no JDK for the geometry and maths checks"
fi

if [ -f tools/javaref/build/biogenesis.jar ]; then
	# .bgg files cross both ways: each side reads what the other wrote
	# (Java validating against its DTD) and both print the same codes
	cp="tools/javaref/build/classes:tools/javaref/build/biogenesis.jar:tools/javaref/build/jts.jar"
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
	# codes the 0.9 jar writes: random, mutated for 200 generations, from
	# evolved worlds, with extreme values; and the Biogenesis gallery's
	mkdir -p "$tmp/corpus"
	java -cp "$cp" biogenesis.BggInterop mutate "$tmp/corpus" 11 100 200
	java -cp "$cp" biogenesis.BggInterop special "$tmp/corpus"
	java -cp "$cp" biogenesis.BggInterop evolve "$tmp/corpus" 3 10000
	cp tools/javaref/download/gallery/*.bgg "$tmp/corpus/" 2>/dev/null || true
	java -cp "$cp" biogenesis.BggInterop read "$tmp"/corpus/*.bgg >"$tmp/kj.txt"
	tests/bgg_tool read "$tmp"/corpus/*.bgg >"$tmp/kc.txt"
	if cmp -s "$tmp/kj.txt" "$tmp/kc.txt"; then
		echo ".bgg corpus: $(wc -l <"$tmp/kj.txt") codes from the 0.9 jar and the gallery read as 0.9 reads them"
	else
		echo "FAIL .bgg corpus read differently from 0.9"
		status=1
	fi
fi

if [ -f tools/javaref/build/biogenesis.jar ]; then
	for seed in 1 42 1234; do
		tools/javaref/compare.sh "$seed" 3000 || status=1
	done
else
	echo "skip: Java reference not built (tools/javaref/fetch.sh)"
fi
exit $status
