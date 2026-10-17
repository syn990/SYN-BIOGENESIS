#!/bin/sh
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

# Downloads HotSpot's x86_64 Math.sin/cos/pow stubs (OpenJDK jdk-27-ga) and
# regenerates src/sim/jvm_libm_x86_64.S from them.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
dl="$here/download"
mkdir -p "$dl"
base=https://raw.githubusercontent.com/openjdk/jdk/jdk-27-ga/src/hotspot/cpu/x86
for f in sin cos pow constants; do
	curl -fsSL -o "$dl/stubGenerator_x86_64_$f.cpp" "$base/stubGenerator_x86_64_$f.cpp"
done
python3 -I "$here/gen_libm.py" "$dl" >"$root/src/sim/jvm_libm_x86_64.S"
echo "wrote src/sim/jvm_libm_x86_64.S"
