# fdlibm 5.3 (atan2)

Sun's freely distributable libm, from https://www.netlib.org/fdlibm/ (licence
notice in each file): `atan2` and what it calls. Java's `Math.atan2` is
`StrictMath.atan2`, a port of exactly this code, so the simulation uses it
for `atan2` (see `src/sim/jmath.h`). Public symbols are renamed with `-D`
flags in the Makefile.
