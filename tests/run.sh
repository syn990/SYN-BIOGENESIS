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
# a Windows JVM separates class path entries with ';'
case $(uname -s) in *_NT-*) sep=';' ;; *) sep=: ;; esac

tests/sim_check "$tmp" || status=1
tests/telescope_check || status=1
mkdir -p "$tmp/library" && tests/library_check "$tmp/library" || status=1
./syn-biogenesis --headless --defaults --quiet --seed 3 --frames 1500 \
	--save "$tmp/fuzz.bgw" && tests/bgw_fuzz "$tmp/fuzz.bgw" 800 || status=1

# the SYN edition's history only reads the world: the same run with and
# without it hashes and saves the same, and its books add up. The recorded
# run's world is made an organism at a time, as the window makes it.
if tests/history_check 40000 42 plain "$tmp/hist_plain.bgw" >"$tmp/hist_plain.txt" &&
	tests/history_check_small 40000 42 history "$tmp/hist_rec.bgw" >"$tmp/hist_rec.txt" 2>"$tmp/hist.log" &&
	cmp -s "$tmp/hist_plain.txt" "$tmp/hist_rec.txt" && cmp -s "$tmp/hist_plain.bgw" "$tmp/hist_rec.bgw"; then
	echo "SYN history: 40000 frames recorded in a world made step by step, world and save unchanged; $(cat "$tmp/hist.log")"
else
	echo "FAIL SYN history: the recorded run differs, or its books don't add up"
	cat "$tmp/hist.log"
	status=1
fi
# and beside Color Mod's, with its collector running every frame while it
# records (the history's codes must stay alive, the rest not matter)
if tests/history_check_colour 1024 42 plain "$tmp/histc_plain.bgw" >"$tmp/histc_plain.txt" &&
	tests/history_check_colour 1024 42 history "$tmp/histc_rec.bgw" >"$tmp/histc_rec.txt" 2>"$tmp/histc.log" &&
	cmp -s "$tmp/histc_plain.txt" "$tmp/histc_rec.txt" && cmp -s "$tmp/histc_plain.bgw" "$tmp/histc_rec.bgw"; then
	echo "SYN history, Color Mod: 1024 frames recorded in a world made step by step, world and save unchanged; $(cat "$tmp/histc.log")"
else
	echo "FAIL SYN history, Color Mod: the recorded run differs, or its books don't add up"
	cat "$tmp/histc.log"
	status=1
fi

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
	# (on Windows the C library ends the C side's lines with \r\n)
	tests/geom_check 31337 200000 | tr -d '\r' >"$tmp/geom_c.txt"
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
	cp="tools/javaref/build/classes${sep}tools/javaref/build/biogenesis.jar${sep}tools/javaref/build/jts.jar"
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
	# .bgw: a world saved here is the file 0.9 saves; each program loads
	# the other's and continues identically; a loaded file saves unchanged
	cp="tools/javaref/build/classes${sep}tools/javaref/build/biogenesis.jar${sep}tools/javaref/build/jts.jar"
	headless="./syn-biogenesis --headless --defaults --quiet"
	n=0
	bad=0
	for sf in 1:0 42:5 7:2000 2026:6000; do
		seed=${sf%:*}
		frames=${sf#*:}
		java -cp "$cp" biogenesis.BgwRun "$seed" "$frames" - save "$tmp/j.bgw"
		$headless --seed "$seed" --frames "$frames" --save "$tmp/c.bgw"
		$headless --load "$tmp/j.bgw" --frames 0 --save "$tmp/r.bgw"
		java -cp "$cp" biogenesis.BgwRun 5 800 "$tmp/jd.txt" load "$tmp/c.bgw"
		$headless --load "$tmp/j.bgw" --seed 5 --frames 800 --dump "$tmp/cd.txt"
		# loaded, run on and saved by each: 0.9 continues the same from both
		java -cp "$cp" biogenesis.BgwRun 9 300 - load "$tmp/j.bgw" save "$tmp/jf.bgw"
		$headless --load "$tmp/j.bgw" --seed 9 --frames 300 --save "$tmp/cf.bgw"
		java -cp "$cp" biogenesis.BgwRun 6 300 "$tmp/jfd.txt" load "$tmp/jf.bgw"
		java -cp "$cp" biogenesis.BgwRun 6 300 "$tmp/cfd.txt" load "$tmp/cf.bgw"
		n=$((n + 1))
		for pair in j.bgw:c.bgw j.bgw:r.bgw jd.txt:cd.txt jfd.txt:cfd.txt; do
			if ! cmp -s "$tmp/${pair%:*}" "$tmp/${pair#*:}"; then
				echo "  seed $seed, $frames frames: ${pair%:*} and ${pair#*:} differ"
				bad=$((bad + 1))
			fi
		done
	done
	if [ $bad = 0 ]; then
		echo ".bgw: $n worlds saved as 0.9 saves them, loaded both ways, continued identically"
	else
		echo "FAIL .bgw: $bad differences"
		status=1
	fi

	# the dice telescope: the seed of a world the 0.9 jar made and saved,
	# found from the save alone and proved by replaying it
	java -cp "$cp" biogenesis.BgwRun 123456789 3000 - save "$tmp/t.bgw"
	if ./syn-biogenesis --telescope world "$tmp/t.bgw" --defaults >"$tmp/t.txt" &&
		grep -q "^seed 123456789 " "$tmp/t.txt"; then
		echo "telescope: the seed of a world the 0.9 jar saved at tick 3000 found from the save, replayed identical"
	else
		echo "FAIL telescope: the seed of the jar's world not found"
		cat "$tmp/t.txt"
		status=1
	fi
	# and the world goes on from its save as the jar's own did: 1000 more
	# frames from the generator found are the jar's tick 4000, to the byte
	gen=$(sed -n 's/^the generator at the saved tick: //p' "$tmp/t.txt")
	java -cp "$cp" biogenesis.BgwRun 123456789 4000 - save "$tmp/t4000j.bgw"
	if [ -n "$gen" ] &&
		./syn-biogenesis --headless --defaults --quiet --load "$tmp/t.bgw" --generator "$gen" \
			--frames 1000 --save "$tmp/t4000c.bgw" && cmp -s "$tmp/t4000j.bgw" "$tmp/t4000c.bgw"; then
		echo "telescope: that save opened with the generator found and run 1000 frames is the jar's own tick 4000"
	else
		echo "FAIL telescope: the save opened on its thread is not the jar's tick 4000"
		status=1
	fi

	for seed in 1 42 1234; do
		tools/javaref/compare.sh "$seed" 3000 || status=1
	done
else
	echo "skip: Java reference not built (tools/javaref/fetch.sh)"
fi
exit $status
