# jvmlibm: the JVM's Math.sin, Math.cos and Math.pow

On x86_64, HotSpot does not run `Math.sin`, `Math.cos` and `Math.pow` as
Java code: it generates Intel LIBM routines instruction by instruction
through its MacroAssembler and calls them from the interpreter and from
compiled code. They round differently from fdlibm (`StrictMath`) and from
the C library in a few percent of cases, and Biogenesis calls them every
frame, so `src/sim/jvm_libm_x86_64.S` is those routines.

`gen_libm.py` translates HotSpot's source
(`src/hotspot/cpu/x86/stubGenerator_x86_64_{sin,cos,pow,constants}.cpp`,
OpenJDK jdk-27-ga) one `__ op(args);` call to one instruction, tables
included, and stops on anything it does not recognise. `fetch.sh`
downloads those files and regenerates the `.S`. `make test` checks the
result against the installed JVM on two million inputs.

The routines are written for the System V convention. On Windows each is
entered through a frame that keeps what the Microsoft x64 convention has
the called function keep and System V does not: `rsi` and `rdi` (which
HotSpot's own Windows build saves, in the `_WIN64` lines the translation
leaves out) and `xmm6` and `xmm7`. The routines themselves are the same.

`pow` uses `rcpps`, whose result is not fully specified and may differ
between CPU vendors; the port executes the same instruction, so it
matches the JVM on the same machine.

The generated file keeps the OpenJDK notices: it is under the GPL version
2 only.
