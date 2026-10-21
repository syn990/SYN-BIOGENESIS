#!/bin/sh
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

# make test, the arena (src/syn/arena.c), in both simulations: a round is
# the simulation unchanged (a qualifier ends as the world --headless makes
# with its seed, byte for byte); a run on one worker comes out the same
# every time, ladder and all, and keeps the ladder's rule; the ladder reads
# back as it was written, and a damaged one is set aside, never
# overwritten; the other simulation's is left alone.
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
status=0
# a configuration folder of its own (on Windows too, where it would be in
# %APPDATA%): the default parameters, and no leaderboard but these
export HOME="$tmp/home"
export XDG_CONFIG_HOME="$tmp/config"
mkdir -p "$HOME" "$XDG_CONFIG_HOME/syn-biogenesis"
# Color Mod's default world takes minutes a cycle: a small one, for both
# the arena (its map PARAMETERS) and --headless
printf 'INITIAL_ORGANISMS=41\nWORLD_WIDTH=400\nWORLD_HEIGHT=400\n' >"$XDG_CONFIG_HOME/syn-biogenesis/colour-params.cfg"

# worlds SIM CYCLES UNITS SEED: the cycles' worlds against --headless's
worlds() {
	sim=$1 n=$2 units=$3 seed=$4
	dir="$tmp/worlds$sim"
	mkdir -p "$dir"
	SYN_BIOGENESIS_ARENA_WORLDS="$dir" ./syn-biogenesis $sim --arena --map params --cycles "$n" --units "$units" \
		--seed "$seed" --seats 0 --no-knockout --no-board --quiet >/dev/null
	c=1
	while [ $c -le "$n" ]; do
		./syn-biogenesis $sim --headless --quiet --seed $((seed + c)) --frames $((units * 256)) \
			--save "$dir/plain-$c.bgw"
		cmp -s "$dir/cycle-$c.bgw" "$dir/plain-$c.bgw" || return 1
		c=$((c + 1))
	done
}
if worlds "" 4 100 500; then
	echo "arena: 4 qualifiers of 100 units, each world the one --headless makes with its seed"
else
	echo "FAIL arena: a round's world differs from --headless's"
	status=1
fi
if worlds --colour 2 10 600; then
	echo "arena, Color Mod: 2 qualifiers of 10 units, each world the one --headless makes with its seed"
else
	echo "FAIL arena, Color Mod: a round's world differs from --headless's"
	status=1
fi

# the same run twice on one worker: the same winners, the same ladder
# (what the program prints compared without the \r Windows ends lines with)
run() {
	./syn-biogenesis --arena --rounds 16 --units 50 --seed 700 --workers 1 --board "$tmp/$1.board" |
		tr -d '\r' | grep -v "frames a second" >"$tmp/$1.txt"
}
if run a && run b && cmp -s "$tmp/a.txt" "$tmp/b.txt" && cmp -s "$tmp/a.board" "$tmp/b.board"; then
	echo "arena: 16 rounds of the ladder on one worker, twice: the same results and ladder ($(grep -cE "^round +[0-9]+ +level " "$tmp/a.txt" || true) winners' rounds)"
else
	echo "FAIL arena: a run on one worker came out differently"
	diff "$tmp/a.txt" "$tmp/b.txt" || true
	status=1
fi

# the ladder's rule: a qualifier's winner goes on at level 1, a level N
# round's at N + 1; every being knocked out was beaten by one a level up
ok=yes
rounds=$(grep -c "^round " "$tmp/a.txt" || true)
bad=$(awk '/^round /{ lv = ($3 == "qualifying") ? 0 : $4; n = split($0, w, "level "); got = w[n] + 0;
	if ($0 ~ /no survivors/) next; if (got != lv + 1) print }' "$tmp/a.txt")
[ -z "$bad" ] || ok=no
./syn-biogenesis --arena --list --board "$tmp/a.board" | tr -d '\r' >"$tmp/ladder.txt"
outs=$(grep -c "knocked out by" "$tmp/ladder.txt" || true)
top=$(awk '/^ *1 +level /{ print $3; exit }' "$tmp/ladder.txt")
[ "$outs" -ge 1 ] && [ "${top:-0}" -ge 2 ] || ok=no
if [ $ok = yes ]; then
	echo "arena: the ladder's rule kept over $rounds rounds: top level $top, $outs knocked out"
else
	echo "FAIL arena: a round's winner went to the wrong level, or no one was knocked out"
	[ -n "$bad" ] && echo "$bad"
	status=1
fi

# the ladder as saved is the one the run ended with, and prints
./syn-biogenesis --arena --list --board "$tmp/a.board" --out "$tmp/printed" | tr -d '\r' >"$tmp/list.txt"
sed -n '/^ladder/,$p' "$tmp/a.txt" | head -11 >"$tmp/ended.txt"
n=$(grep -c "^ *[0-9]" "$tmp/ended.txt" || true)
head -$((n + 1)) "$tmp/list.txt" >"$tmp/listed.txt"
if cmp -s "$tmp/listed.txt" "$tmp/ended.txt" && [ -f "$tmp/printed/winners.txt" ] &&
	[ "$(ls "$tmp/printed"/*.bgg | wc -l)" -ge "$n" ]; then
	echo "arena: the ladder read back as saved, and printed ($(ls "$tmp/printed"/*.bgg | wc -l) codes)"
else
	echo "FAIL arena: the ladder didn't read back as saved, or didn't print"
	diff "$tmp/ended.txt" "$tmp/listed.txt" || true
	ls "$tmp/printed" || true
	status=1
fi

# a damaged ladder is moved aside, and the arena starts without it
head -c 200 "$tmp/a.board" >"$tmp/bad.board"
./syn-biogenesis --arena --list --board "$tmp/bad.board" >"$tmp/bad.txt" 2>"$tmp/bad.log"
if [ -f "$tmp/bad.board.bad" ] && [ ! -f "$tmp/bad.board" ] && grep -q "^ladder (0 " "$tmp/bad.txt"; then
	echo "arena: a damaged ladder set aside, not read"
else
	echo "FAIL arena: a damaged ladder wasn't set aside"
	status=1
fi
# a being from a .bgg put on the ladder at level 1, named after its file
if ./syn-biogenesis --arena --board "$tmp/enter.board" --enter docs/being/ultimate.bgg --list \
	>"$tmp/enter.txt" 2>&1 && grep -q "level 1 *ultimate " "$tmp/enter.txt" &&
	./syn-biogenesis --arena --board "$tmp/enter.board" --list | grep -q "level 1 *ultimate "; then
	echo "arena: a being from a .bgg put on the ladder at level 1, and kept there"
else
	echo "FAIL arena: a being from a .bgg wasn't put on the ladder"
	cat "$tmp/enter.txt"
	status=1
fi
# the other simulation's ladder is refused and left exactly as it was
cp "$tmp/a.board" "$tmp/other.board"
if ./syn-biogenesis --colour --arena --list --board "$tmp/other.board" >/dev/null 2>"$tmp/other.log"; then
	refused=no
else
	refused=yes
fi
if [ $refused = yes ] && cmp -s "$tmp/a.board" "$tmp/other.board" && [ ! -f "$tmp/other.board.bad" ]; then
	echo "arena: Biogenesis 0.9's ladder refused by Color Mod's arena, and left as it was"
else
	echo "FAIL arena: the other simulation's ladder wasn't left alone"
	status=1
fi
exit $status
