#!/bin/sh
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

# Downloads Eclipse Temurin JREs (HotSpot, Linux x64) into tools/drift/jdk/
# so drift.py can run the original on more Java versions than the system's.
#
#   tools/drift/fetch-jdks.sh [VERSION...]     default: 11 17 21
set -eu
here=$(cd "$(dirname "$0")" && pwd)
[ $# -gt 0 ] || set -- 11 17 21
for v in "$@"; do
	dir="$here/jdk/$v"
	[ -x "$dir/bin/java" ] && continue
	mkdir -p "$dir"
	curl -fsSL "https://api.adoptium.net/v3/binary/latest/$v/ga/linux/x64/jre/hotspot/normal/eclipse" |
		tar -xz -C "$dir" --strip-components=1
	"$dir/bin/java" -version 2>&1 | head -1
done
