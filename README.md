# SYN-BIOGENESIS

**Biogenesis 0.9 rewritten in C11 and SDL2.**

![A world 30,000 frames after genesis (seed 42)](docs/screenshots/world.png)

[Biogenesis](https://biogenesis.sourceforge.net/) by Joan Queralt Molina
(0.9 by MarcoDBAA) simulates the evolution of unicellular organisms: bodies
made of coloured segments that photosynthesise, eat, infect, kill, shield,
swim and reproduce, mutating as they go. The original is a Java/Swing jar.
This is a native rewrite of its vanilla 0.9 release: the same simulation,
the same parameters, the same genetic-code files, without the JVM.
All credit for the program's design and simulation goes to its authors
(see [Credits](#credits)); this project is not affiliated with them.

| | | |
|---|---|---|
| ![Selected organism at 300%](docs/screenshots/organism.png) | ![Genetic laboratory](docs/screenshots/lab.png) | ![Statistics](docs/screenshots/statistics.png) |
| An organism selected, at 300% | Genetic laboratory | Statistics |

## Build and run

Needs SDL2, SDL2_ttf and SDL2_image (on Arch: `sdl2-compat sdl2_ttf sdl2_image`).

```sh
make
./syn-biogenesis            # random world
./syn-biogenesis 42         # seeded, like `java -jar biogenesis.jar 42`
```

`make headless` builds `syn-biogenesis-headless`, the simulation alone
(no SDL), for benchmarks and long runs:

```sh
./syn-biogenesis-headless --headless --seed 42 --frames 100000 --report 10000
```

Parameters are kept in `~/.config/syn-biogenesis/params.cfg`.

## Using it

Everything from the original is in the same place: **Game** (new, pause,
open, save, quit), **World** (statistics, genetic laboratory, CO2 up/down,
kill all, disperse all, parameters), and the right-click menus on organisms
(track, feed, weaken, force reproduction, rejuvenate, kill, copy, export,
save image), on corpses (revive, disperse) and on empty space (paste,
create randomly, import).

| Input | Does |
| --- | --- |
| Left click | select an organism (info bar at the bottom) |
| Right click | organism / corpse / empty-space menu |
| P or Space | pause / resume |
| S, Ctrl+S | save (Ctrl+Shift+S: save as) |
| Ctrl+O, Ctrl+Q | open, quit |
| Wheel, Shift+wheel, arrows, middle drag | scroll |
| Ctrl+wheel, + / - / 0 | zoom in, out, actual size |
| Esc | deselect |

Segment colours, as in the original: **green** photosynthesis, **red** eats
what it touches, **cyan** cilia (random kicks), **blue** shield, **white**
infects others with its own genetic code, **gray** kills on touch,
**yellow** more children per birth. Dead organisms turn brown and decay.

## How faithful it is

The simulation (`src/sim/`) is a statement-by-statement port of
`Organism.java`, `World.java`, `GeneticCode.java`, `Gene.java`,
`WorldStatistics.java` and the parts of `java.util.Random` and `java.awt`
geometry they depend on, keeping the original's arithmetic order and its
quirks. That is checked, not assumed (`make test`, `tools/javaref/`):

- The original Java classes run headless with the same seed and dump their
  complete state every frame; the C port dumps the same.
- With both sides on the same maths library (fdlibm, which Java's
  `StrictMath` is), the two are **bit-identical**: every organism's
  position, velocity, angle and energy to the last bit, over 30,000 frames
  on each of 10 seeds.
- With the normal maths libraries (JVM `Math` vs glibc) they are identical
  until the first last-bit rounding difference in `sin`/`cos`/`atan2`/`pow`
  (seen at frames 986 and 1,268 on two seeds; a third stayed identical for
  all 2,000 frames tested), after which the worlds drift apart the way the
  Java program does between two different JVMs.
- `java.awt.Rectangle`/`Line2D` semantics: 200,000 random cases identical to
  the JVM.
- Genetic codes (`.bgg`) written by either program are read identically by
  both (the Java side validating against its DTD).

Speed: about **2.2x** the Java original on identical simulations (30,000
frames headless, same seed, same final state). Set "Time per frame" to 1 ms
to run the window's simulation as fast as the machine allows.

Long runs: over 600,000 frames (8 hours at the default 20 frames per
second) the time per frame stays flat in both programs. The Java window,
though, adapts its frame delay: two late frames in a row multiply it by
1.5, it comes back down 1 ms at a time, and past one second it pauses the
game; its FPS display shows that delay rather than a measured rate. This
port keeps to the delay set, runs late frames as fast as it can, and never
slows itself down or pauses.

### Differences from the Java version

- **Collision order.** 0.9 visits collision candidates in JTS STRtree order;
  this visits the same candidates in list order (as 0.8 did). It decides
  which of several simultaneous touches is handled first.
- **World files** are `.sbw` (a plain binary format). Java's `.bgw` files are
  Java object serialisation and can't be read. Genetic codes stay `.bgg`.
- **Not ported:** the Network menu (biological corridors between computers),
  translations (English only), "check last version", and the Java2D OpenGL
  options.
- **Interface:** dark theme; the main toolbar stays visible with the
  organism tools added beside it; the world view zooms; statistics update
  live instead of on "Update"; genetic-code previews scale up as well as
  down; the FPS counter shows the rate actually reached; parameters are
  saved on OK as well as on quit.
- **Kept on purpose** (they shape how worlds evolve): the moment of inertia
  formula that adds `cy*cy` unweighted, the `rbpy * ny` typo in the
  collision response, "Feed" adding CO2 instead of taking it, and the
  negative-determinant "parallel lines" case.

Color Mod and X Mod (in the same SourceForge download) are not included;
they ship as compiled classes only and are much larger than vanilla.

## Source layout

```
src/sim/      simulation: organism, world, genetic code, stats, java.util.Random,
              java.awt geometry semantics, parameters
src/io/       .bgg genetic codes (XML), .sbw worlds
src/ui/       SDL2 front end: immediate-mode widgets, world view, dialogs
src/headless.c, src/main.c
tests/        make test
tools/javaref/  the Java reference harness and fdlibm for the strict build
```

## Tests

```sh
tools/javaref/fetch.sh   # once: downloads Biogenesis 0.9, builds the Java reference
make test
```

`make test` runs the file-format checks (a world saved and reloaded mid-run
continues identically; malformed files are refused), the geometry check
against the JVM, the `.bgg` exchange with the Java parser, and the
frame-by-frame comparison with the original. The Java parts are skipped
without a JDK.

For UI checks without a display, the app has a test hook driven by
environment variables (see `shot_prepare` and `Script` in `src/ui/app.c`):

```sh
SDL_VIDEODRIVER=offscreen SYN_BIOGENESIS_SHOT=out.png SYN_BIOGENESIS_SHOT_SIM=3000 ./syn-biogenesis 42
SDL_VIDEODRIVER=offscreen SYN_BIOGENESIS_SCRIPT="wait 5; click 300 300 r; click 340 344; shot a.png; quit" ./syn-biogenesis
```

## Credits

SYN-BIOGENESIS is a port. The ideas, the simulation and its behaviour are
the original's:

- **Biogenesis**, copyright (c) 2006-2010 Joan Queralt Molina:
  <https://biogenesis.sourceforge.net/>
- **Biogenesis 0.9** by MarcoDBAA (2018-2019), with STRtree collision
  detection (JTS, by Vivid Solutions) implemented by Richard David Williams.
- **Icons** are copied unchanged from Biogenesis, which credits its artwork
  to Ananda Daydream and Florian Haag
  ([Toolbar Icons](https://sourceforge.net/projects/toolbaricons/),
  copyright (c) 2010 Florian Haag, MIT licence). See
  [`assets/icons/LICENSE`](assets/icons/LICENSE).
- **Noto Sans** is under the SIL Open Font License (`assets/fonts/OFL.txt`).
- **fdlibm** (`tools/javaref/fdlibm/`) is Sun Microsystems' freely
  distributable libm, used only by the verification build.

## Licence

GPL version 2 or later, like Biogenesis (`LICENSE`). Each source file
names the original file it was translated from or based on; the changes
from the original are described above under "Differences from the Java
version", and the history is in git.
