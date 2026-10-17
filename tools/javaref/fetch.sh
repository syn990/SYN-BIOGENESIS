#!/bin/sh
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

# Downloads the Biogenesis 0.9 sources and jar from SourceForge into
# tools/javaref/download/ and builds the reference (see build.sh).
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
"$here/build.sh" "$dl/src/src/biogenesis" "$dl/jts-core-1.15.0.jar"
"$here/build.sh" --strict "$dl/src/src/biogenesis" "$dl/jts-core-1.15.0.jar"
