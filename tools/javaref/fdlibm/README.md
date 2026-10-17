# fdlibm 5.3 (subset)

Sun's freely distributable libm, from https://www.netlib.org/fdlibm/ (licence
notice in each file). Java's `StrictMath` is a port of exactly this library,
so building the C simulation against it (`make strict`) and running the Java
original with `StrictMath` (`tools/javaref/build.sh --strict`) removes the one
legitimate source of difference between the two: the last-bit rounding of
`sin`, `cos`, `atan2` and `pow`, which the JVM's `Math` and glibc each do
their own way.

Used only for that verification build; the program itself uses the system
libm. Public symbols are renamed with `-D` flags in the Makefile.
