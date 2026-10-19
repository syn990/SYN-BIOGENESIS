#!/bin/sh
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

# make test, the colour build: its .bgg and .bgw files against the shipped
# Color Mod 2.01 jar, and its simulation frame by frame (skipped without the
# Color Mod reference: tools/javaref/fetch.sh).
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"
if [ ! -f tools/javaref/colour/build/biogenesis.jar ] || ! command -v java >/dev/null 2>&1; then
	echo "skip: Color Mod reference not built (tools/javaref/fetch.sh)"
	exit 0
fi
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
status=0
b=tools/javaref/colour/build
# a Windows JVM separates class path entries with ';'
case $(uname -s) in *_NT-*) sep=';' ;; *) sep=: ;; esac
cp="$b/classes$sep$b/biogenesis.jar$sep$b/jts.jar"
bgg() { java -cp "$cp" biogenesis.ColourBggInterop "$@"; }
headless="./syn-biogenesis --colour --headless --defaults --quiet"

# .bgg: codes written by each read the same by both, and refused alike
mkdir -p "$tmp/bgg" "$tmp/corpus" "$tmp/bad"
bgg write "$tmp/bgg" 2024 50
tests/colour_bgg_tool write "$tmp/bgg" 2024 50
bgg read "$tmp"/bgg/java_*.bgg >"$tmp/jj.txt"
tests/colour_bgg_tool read "$tmp"/bgg/java_*.bgg >"$tmp/jc.txt"
bgg read "$tmp"/bgg/c_*.bgg >"$tmp/cj.txt"
tests/colour_bgg_tool read "$tmp"/bgg/c_*.bgg >"$tmp/cc.txt"
if cmp -s "$tmp/jj.txt" "$tmp/jc.txt" && cmp -s "$tmp/cj.txt" "$tmp/cc.txt" &&
	cmp -s "$tmp/jj.txt" "$tmp/cc.txt"; then
	echo "colour .bgg interchange: 50 codes written by Color Mod and the same 50 written here, read identically by both"
else
	echo "FAIL colour .bgg interchange differs"
	status=1
fi
bgg mutate "$tmp/corpus" 11 100 200
bgg special "$tmp/corpus"
bgg evolve "$tmp/corpus" 3 600
# damaged copies of one code: values out of range, unknown names, gaps
f=$(ls "$tmp"/corpus/mutated_*.bgg | head -1)
i=0
for s in 's/mutationrate="[0-9]*"/mutationrate="10001"/' 's/activity="[0-9]"/activity="3"/' \
	's/modifiescream="[0-9]"/modifiescream="0"/' 's/adaptspore="[0-9]*"/adaptspore="71"/' \
	's/plague="[a-z]*"/plague="maybe"/' 's/color="[a-z]*"/color="purpleish"/' \
	's/branch="[-0-9]*"/branch="x"/' 's/redreaction="[0-9]*"/redreaction=""/' \
	's/symmetry="[0-9]"/symmetry="9"/' 's/<!DOCTYPE.*//' '/<gene /d' \
	's/theta="[^"]*"/theta="1e400"/' 's/length="[^"]*"/length="0x1p3"/' \
	's/friendreaction="[0-9]*"/friendreaction="2147483648"/' 's/mirror="[a-z]*"/mirror="Yes"/'; do
	sed "$s" "$f" >"$tmp/bad/bad_$i.bgg"
	i=$((i + 1))
done
# 200 at a time: a command line on Windows holds 32,767 characters
printf '%s\n' "$tmp"/corpus/*.bgg "$tmp"/bad/*.bgg >"$tmp/list"
xargs -n 200 java -cp "$cp" biogenesis.ColourBggInterop read <"$tmp/list" >"$tmp/kj.txt"
xargs -n 200 tests/colour_bgg_tool read <"$tmp/list" >"$tmp/kc.txt"
if cmp -s "$tmp/kj.txt" "$tmp/kc.txt"; then
	echo "colour .bgg corpus: $(wc -l <"$tmp/kj.txt") codes (mutated, evolved, extreme, damaged) read as Color Mod reads them"
else
	echo "FAIL colour .bgg corpus read differently from Color Mod"
	status=1
fi

# .bgw: a world saved here is the file Color Mod saves; each program loads
# the other's and continues identically; a loaded file saves unchanged
n=0
bad=0
for sf in 1:0 42:5 7:300 2026:600; do
	seed=${sf%:*}
	frames=${sf#*:}
	java -cp "$cp" biogenesis.ColourBgwRun "$seed" "$frames" - save "$tmp/j.bgw"
	$headless --seed "$seed" --frames "$frames" --save "$tmp/c.bgw"
	$headless --load "$tmp/j.bgw" --frames 0 --save "$tmp/r.bgw"
	java -cp "$cp" biogenesis.ColourBgwRun 5 50 "$tmp/jd.txt" load "$tmp/c.bgw"
	$headless --load "$tmp/j.bgw" --seed 5 --frames 50 --dump "$tmp/cd.txt"
	# loaded, run on and saved by each: Color Mod continues the same from both
	java -cp "$cp" biogenesis.ColourBgwRun 9 30 - load "$tmp/j.bgw" save "$tmp/jf.bgw"
	$headless --load "$tmp/j.bgw" --seed 9 --frames 30 --save "$tmp/cf.bgw"
	java -cp "$cp" biogenesis.ColourBgwRun 6 30 "$tmp/jfd.txt" load "$tmp/jf.bgw"
	java -cp "$cp" biogenesis.ColourBgwRun 6 30 "$tmp/cfd.txt" load "$tmp/cf.bgw"
	n=$((n + 1))
	for pair in j.bgw:c.bgw j.bgw:r.bgw jd.txt:cd.txt jfd.txt:cfd.txt; do
		if ! cmp -s "$tmp/${pair%:*}" "$tmp/${pair#*:}"; then
			echo "  seed $seed, $frames frames: ${pair%:*} and ${pair#*:} differ"
			bad=$((bad + 1))
		fi
	done
	rm -f "$tmp"/*.bgw "$tmp"/*d.txt
done
if [ $bad = 0 ]; then
	echo "colour .bgw: $n worlds saved as Color Mod saves them, loaded both ways, continued identically"
else
	echo "FAIL colour .bgw: $bad differences"
	status=1
fi

# the simulation: Color Mod's default world (12,500 organisms), every 10th frame
for seed in 1 42 1234; do
	tools/javaref/colour/compare.sh "$seed" 1000 10 || status=1
done
exit $status
