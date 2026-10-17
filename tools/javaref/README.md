# javaref: checking the port against the Java original

`fetch.sh` downloads Biogenesis 0.9 (source and jar) from SourceForge into
`download/` and builds two copies of the original simulation classes with
`build.sh`:

- `build/`: as shipped (`Math`), the reference for `syn-biogenesis-headless`.
- `build-strict/`: `Math.sin/cos/atan2/pow` replaced by `StrictMath`, the
  reference for `make strict` (the C simulation on fdlibm, see `fdlibm/`).

Both get `Stubs.java` (no-op stand-ins for the Swing classes the simulation
touches) and `Harness.java` (runs a seeded world and dumps every organism's
state each frame). The only change to the original code: `World.checkHit`
visits collision candidates in list order instead of STRtree order, so a
seed has one well-defined outcome, matching the C port.

```sh
tools/javaref/compare.sh 42 3000            # glibc vs JVM Math
tools/javaref/compare.sh --strict 42 30000  # fdlibm vs StrictMath: identical
```

`BggInterop.java` writes and reads genetic codes with the original
`BioXMLParser` for the `.bgg` exchange test in `tests/run.sh`.
