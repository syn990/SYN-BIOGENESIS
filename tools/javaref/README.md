# javaref: checking the port against the Java original

`fetch.sh` downloads Biogenesis 0.9 (source and jar) from SourceForge into
`download/` and builds the reference with `build.sh`: `Harness.java` (runs
a seeded world and dumps every organism's state each frame),
`BggInterop.java` (reads and writes genetic codes with the original
`BioXMLParser`) and `Stubs.java` (no-op stand-ins for the Swing classes),
compiled against the shipped `biogenesis.jar`.

The simulation that runs is the jar's own bytecode, unmodified, with the
JTS 1.15.0 jar it ships with, on the JVM's normal `Math`. Nothing is
recompiled or patched; the stubs replace only window classes the
simulation calls into (repaints, the info bar) and come first on the class
path.

```sh
tools/javaref/compare.sh 42 30000        # every frame
tools/javaref/compare.sh 42 600000 100   # every 100th frame
```

`BgwRun.java` runs a seeded or loaded world and saves it as 0.9's
Save does (`.bgw`), for the world-file checks in `tests/run.sh`; the
headless port's `--load` and `--save` do the same. The classes are
compiled for Java 8 where the compiler allows it, so they also run on the
Java the jar was built for.

`BggInterop.java` writes genetic codes with the jar (random, mutated,
evolved in a world, extreme values) and reads them with its validating
`BioXMLParser`; `tests/run.sh` compares that reading with the port's.
`fetch.sh` also downloads the 15 genetic codes in the Biogenesis gallery
(from the Internet Archive, since the site refuses scripted downloads) into
`download/gallery/`.
