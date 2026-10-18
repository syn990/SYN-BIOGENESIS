#!/bin/sh
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

# Downloads the Biogenesis 0.9 sources and jar from SourceForge into
# tools/javaref/download/ and builds the reference (see build.sh). The
# source is for reading; the reference runs the jar. Also fetches Color Mod
# 2.01's source, which tools/colour/translate.py turns into src/colour/sim/,
# and builds its reference from the Color Mod jar of the same download
# (colour/build.sh).
set -eu
here=$(cd "$(dirname "$0")" && pwd)
dl="$here/download"
mkdir -p "$dl"
base=https://downloads.sourceforge.net/project/biogenesis/biogenesis/0.9
[ -f "$dl/src.zip" ] || curl -fsSL -o "$dl/src.zip" "$base/biogenesis_src_0_9.zip"
[ -f "$dl/jar.zip" ] || curl -fsSL -o "$dl/jar.zip" "$base/biogenesis_jar_0_9%20Mods_v20.zip"
rm -rf "$dl/src" "$dl/jar"
mkdir -p "$dl/src" "$dl/jar"
unzip -q -o "$dl/src.zip" -d "$dl/src"
unzip -q -o "$dl/jar.zip" -d "$dl/jar"
unzip -q -o "$dl/jar/0_9 (vanilla)/biogenesis.jar" jts-core-1.15.0.jar -d "$dl"
# The genetic codes in the Biogenesis gallery (https://biogenesis.sourceforge.net/
# gallery.en.php), from the Internet Archive: that site refuses scripted downloads
mkdir -p "$dl/gallery"
for f in month/month gallery/1 gallery/2 gallery/3 gallery/4 gallery/5 gallery/6 gallery/7 \
	gallery/8 gallery/1.old gallery/2.old gallery/3.old gallery/4.old gallery/5.old gallery/6.old; do
	out="$dl/gallery/$(echo "$f" | tr / _).bgg"
	[ -f "$out" ] || curl -fsSL -o "$out" \
		"https://web.archive.org/web/2024id_/https://biogenesis.sourceforge.net/$f.bgg" ||
		echo "could not fetch $f.bgg" >&2
done
"$here/build.sh" "$dl/jar/0_9 (vanilla)/biogenesis.jar" "$dl/jts-core-1.15.0.jar"
# Color Mod 2.01 with its source. The jar in this zip is the one in the
# "Mods" download above, byte for byte; the reference runs that one.
cm=https://downloads.sourceforge.net/project/biogenesis/Color%20Mod/2.0
[ -f "$dl/colormod-src.zip" ] || curl -fsSL -o "$dl/colormod-src.zip" \
	"$cm/ColorMod%202_01%20and%20sourcecode.zip"
rm -rf "$dl/colormod-src"
mkdir -p "$dl/colormod-src"
unzip -q -o "$dl/colormod-src.zip" -d "$dl/colormod-src"
cmp "$dl/colormod-src/ColorMod_2_01/biogenesis.jar" "$dl/jar/ColorMod_2_01/biogenesis.jar"
"$here/colour/build.sh" "$dl/jar/ColorMod_2_01/biogenesis.jar" "$dl/jts-core-1.15.0.jar"
