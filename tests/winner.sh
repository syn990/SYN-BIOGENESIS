#!/bin/sh
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

# make test, Create a winner (src/syn/winner.c), in both simulations: a
# world saved and opened again goes on exactly as the one left running,
# with a code pasted into each (what lets the search work on a copy); and
# what the search says its winner does, a replay with the simulation alone
# (tests/winner_check) finds: the same bloodline among the same living,
# the world hashed the same.
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
status=0
# a configuration folder of its own, with no library: the default
# parameters, Color Mod's in a small world (its default one takes minutes)
export HOME="$tmp/home"
export XDG_CONFIG_HOME="$tmp/config"
mkdir -p "$HOME" "$XDG_CONFIG_HOME/syn-biogenesis"
printf 'INITIAL_ORGANISMS=41\nWORLD_WIDTH=400\nWORLD_HEIGHT=400\n' >"$XDG_CONFIG_HOME/syn-biogenesis/colour-params.cfg"

# check SIM NAME CHECKER FRAMES: a world of FRAMES frames, then both checks
check() {
	sim=$1 name=$2 checker=$3 frames=$4
	dir="$tmp/$name"
	mkdir -p "$dir"
	./syn-biogenesis $sim --headless --quiet --seed 5 --frames "$frames" --save "$dir/world.bgw"
	if same=$($checker same "$dir/world.bgw" 700 3000 "$dir/snap.bgw"); then
		echo "$name winner: a world saved and opened again goes on as the one left running; $same"
	else
		echo "FAIL $name winner: a world saved and opened again goes another way; $same"
		status=1
	fi
	./syn-biogenesis $sim --winner "$dir/world.bgw" --generator 2b6a1f03c5d9 --units 16 --seconds 120 \
		--workers 2 --out "$dir/out" >"$dir/log.txt"
	line=$(grep '^@winner ' "$dir/log.txt" || true)
	if [ -z "$line" ]; then
		echo "FAIL $name winner: the search named no winner"
		cat "$dir/log.txt"
		status=1
		return
	fi
	# @winner KIND X Y T MINE ALIVE HASH END_T END_N PATH<tab>LABEL
	set -- $(printf '%s\n' "$line" | cut -f1)
	kind=$2 x=$3 y=$4 t=$5 mine=$6 alive=$7 hash=$8 end_t=$9
	shift 9
	end_n=$1 code=$2
	got=$($checker replay "$dir/world.bgw" 2b6a1f03c5d9 "$code" "$x" "$y" "$t")
	end=$($checker replay "$dir/world.bgw" 2b6a1f03c5d9 "$code" "$x" "$y" "$end_t")
	if [ "$got" = "$t $mine $alive $hash" ] && [ "$(echo "$end" | cut -d' ' -f1-2)" = "$end_t $end_n" ] &&
		{ [ "$kind" != owns ] || [ "$mine" = "$alive" ]; }; then
		echo "$name winner: the search's winner ($kind) replayed: $mine of the $alive living at time $t, $end_n at time $end_t, the world as it said"
	else
		echo "FAIL $name winner: the search said '$t $mine $alive $hash' and $end_n at $end_t; the replay '$got', '$end'"
		status=1
	fi
}

check "" 0.9 tests/winner_check 6000
check --colour colour tests/winner_check_colour 3000
exit $status
