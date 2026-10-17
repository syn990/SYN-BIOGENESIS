# SYN-BIOGENESIS

**Biogenesis, the artificial-life simulator, reimplemented in C11 and SDL2.**

![A 3000x2000 world 30,000 frames after genesis (seed 42), zoomed out to 33%](docs/screenshots/world.png)

SYN-BIOGENESIS is a native reimplementation of
[Biogenesis](https://biogenesis.sourceforge.net/) 0.9, made as a tool for
SYN-OS so the simulator runs without Java. It isn't a remake or a game in
the same spirit: given the same seed and parameters, it computes the same
world as the original Java program, bit for bit, on every frame, about 2.2
times as fast.

Biogenesis is the work of **Joan Queralt Molina**, who wrote versions 0.1 to
0.8 (2006 to 2010), and **MarcoDBAA**, who released 0.9 in 2019 with
**Richard David Williams**'s STRtree collision detection. The simulation,
its rules, its parameters and its look are theirs; this project rewrites
them in another language and is not affiliated with them. See
[Credits](#credits).

| | | |
|---|---|---|
| ![Selected organism at 300%](docs/screenshots/organism.png) | ![Genetic laboratory](docs/screenshots/lab.png) | ![Statistics](docs/screenshots/statistics.png) |
| An organism selected, at 300% | Genetic laboratory | Statistics |

**Contents:** [Biogenesis](#biogenesis) ·
[The reimplementation](#the-reimplementation) ·
[Lapsadian determinism and drift](#lapsadian-determinism-and-drift) ·
[Build and run](#build-and-run) · [Using it](#using-it) ·
[Source and tests](#source-and-tests) · [Credits](#credits)

## Biogenesis

Biogenesis is a petri dish. A world starts with fifteen random organisms in
air that holds carbon dioxide and nothing else, and from then on nobody
steers it. Organisms photosynthesise, eat, infect and kill each other,
reproduce with small copying errors, and die of age or hunger. What
survives is whatever happens to work. Leave a world running and the dish
fills with lineages nobody designed: green drifters that live on light, red
grazers that live on them, white viruses that turn other organisms into
factories for their own genetic code, blue-armoured survivors, gray
killers.

The rules are short, and nothing in them describes behaviour. An organism
is a body of coloured line segments. Each colour does one thing, and the
body's shape decides how it moves, what it touches and what it can afford.

### The colours

| Colour | What a segment of it does | Energy per use (default) |
|---|---|---|
| **Green** | Photosynthesis: energy every frame, in proportion to the segment's length and to the CO2 left in the air | 0 |
| **Red** | Eats what it touches, living or dead: up to half the segment's length in energy per frame of contact; a tenth of it is lost as CO2 | 0 |
| **Cyan** | Cilia: an 8% chance each frame of a kick along the segment, with a random spin | 0 |
| **Blue** | Shield: the segment blocks red, white and gray | 0 |
| **White** | Infection: the next child of the organism it touches is born with the infector's genetic code instead of its parent's | 1 |
| **Gray** | Kills what it touches | 1 |
| **Yellow** | More children per birth: one more per yellow segment, up to eight, sharing the same energy between them | 0 |
| **Brown** | Dead. Corpses keep drifting and decay back into CO2 | |

Parents and their children never harm each other. Organisms flash a colour
for a moment when something happens to them (bitten, infected, shielded). A
new segment is green 30% of the time, cyan 20%, and red, blue, white, gray
or yellow 10% each. Every number here is a parameter in **World >
Parameters**.

Yellow is the subtle one. More segments of it don't make an organism any
richer: the energy needed to reproduce stays the same, and it is divided
between the parent and all the children. Three yellow segments give four
children, each starting with a fifth of that energy. It trades a few
well-fed offspring for many weak ones, each carrying its own mutations.

### How a world works

- **The air.** A new world holds 5,000 units of CO2 and no oxygen. The
  first fifteen organisms are each given 50 energy made from it, which
  leaves 4,250 CO2 and 750 O2. Photosynthesis turns CO2 into O2 and
  energy. Every use of energy
  (breathing, kicking, biting, infecting) is respiration and turns O2 back
  into CO2. CO2 and O2 always add up to the same total (unless you add or
  remove CO2 from the menu), so the dish is a closed carbon cycle. When
  plants boom, CO2 runs low and photosynthesis slows with it; when nothing
  photosynthesises, oxygen runs out and nothing can breathe.
- **Bodies.** A genetic code is a list of genes, each one segment: a length
  (2 to 18 pixels in a random gene), an angle and a colour. The body repeats
  that list around its centre 1 to 8 times (its symmetry), optionally
  mirroring every other copy. Random organisms start with 1 to 8 genes;
  mutation can take a code to 100. One more trait decides whether children
  scatter away from their parent or keep its speed.
- **Growing and breathing.** A newborn is drawn at about a fifth of its full
  size and grows in fifteen steps, one every 8 frames while it has energy
  to spare (a tenth of its mass); a starving organism shrinks back. Keeping
  a body costs its mass ÷ 5,000 in energy every frame, breathed with oxygen.
- **Reproduction.** A full-grown organism reproduces when its energy passes
  40 + 3 × genes × symmetry, at most once every 20 frames. Each child is
  placed beside the parent, in the first of the eight directions around it
  (starting from a random one) with room. Each trait of the copied code mutates with a 5% chance: mirroring,
  symmetry, gaining or losing a gene, and each gene's length, angle and
  colour.
- **Age and death.** An organism dies after 31 time units of 256 frames
  (7,936 frames), or as soon as it can't breathe. The "Time" in the status
  bar counts those units.

### Versions and mods

- **0.1 to 0.8**, Joan Queralt Molina, 2006 to 2010: the simulation, the
  genetic laboratory, statistics, network corridors between worlds, and (in
  0.8) the command-line seed this project's verification depends on.
- **0.9**, MarcoDBAA, 2019: STRtree collision detection by Richard David
  Williams "to make the simulation run much faster", fixes for organisms
  vanishing after collisions, German, Portuguese and Russian translations.
- **Color Mod**, MarcoDBAA, 2012 to 2022: over forty colours, methane,
  reaction genes and per-organism mutation rates.
- **X Mod 0.2.2**, Adam Muschielok, 2015: alternative mutation mechanisms and
  stress-based ageing.

SYN-BIOGENESIS reimplements vanilla 0.9. The mods ship in the same
SourceForge download as compiled classes and are not included.

## The reimplementation

SYN-BIOGENESIS was made for SYN-OS, to run Biogenesis there without a Java
runtime. It is the same program as a small native binary:

| | Biogenesis 0.9 (Java) | SYN-BIOGENESIS |
|---|---|---|
| Needs | a Java runtime: 98 MB (Java 8) to 159 MB (Java 21 JRE) | SDL2, SDL2_ttf, SDL2_image |
| Program | 1.3 MB jar | 190 KB binary, plus 780 KB of icons and font |
| Start to first frame, headless | 72 ms | 1 ms |
| Peak memory, 30,000 frames headless | 97 MB | 13 MB |
| Simulation speed, same world | 1× (192.4 s for 600,000 frames) | 2.2× (87.5 s) |
| When frames run late | multiplies its frame delay by 1.5; past one second, pauses the game | keeps the delay you set and catches up |
| Saved worlds | Java object serialisation (`.bgw`) | opens and saves those `.bgw` files, or its own plain binary `.sbw` |
| Without a window | no | `--headless`: benchmarks, state dumps, state hashes |

Timings are on an Intel i5-8500 with OpenJDK 27. Inside the window
everything from the original is where it was. The interface has a dark
theme, and the world view zooms (Ctrl+wheel, + and -). Statistics update
live. Previews of genetic codes scale up as well as down, the FPS counter
shows the rate actually reached, and parameters are saved when you press
OK.

### The same world, bit for bit

A reimplementation usually aims to play the same. This one is held to
computing the same: run the same seed in both programs and every
organism's energy, position, angle and speed agree to the last bit, on
every frame. Getting there meant reproducing the Java program's arithmetic,
not just its rules:

- **A statement-by-statement port** of `Organism.java`, `World.java`,
  `GeneticCode.java`, `Gene.java` and `WorldStatistics.java`, keeping the
  original's order of operations. C compiles it without fused
  multiply-adds (`-ffp-contract=off`) and with wrapping integer overflow
  (`-fwrapv`), as Java computes.
- **Java's library, reimplemented where the simulation uses it:**
  `java.util.Random`'s generator, `Math.round`, and the corner cases of
  `java.awt.Rectangle` and `Line2D` (checked against the JVM on 200,000
  random cases).
- **The JVM's maths.** `Math.atan2` is fdlibm, which the port uses
  (`src/fdlibm/`). `Math.sin`, `Math.cos` and `Math.pow` are not. On x86_64,
  HotSpot replaces them with Intel routines of its own, which round
  differently from fdlibm and from the C library in a few percent of
  cases. `src/sim/jvm_libm_x86_64.S` is those routines, generated
  instruction for instruction from OpenJDK's source by
  `tools/jvmlibm/gen_libm.py`.
- **The same collision order.** When several organisms touch in one frame,
  the first contact found wins, and 0.9 finds contacts through a JTS
  STRtree. `src/sim/world.c` builds the same tree (the same packing, the
  same stable sorts, the same traversal), so contacts are found in the
  same order.
- **The original's bugs, kept.** Fixing any of these would change how every
  world evolves, so they stay:
  - The moment of inertia adds each segment's `cy²` without multiplying
    by its mass, so bodies spin more easily than physics says.
  - In the collision response, the second organism's spin uses `rbpy * ny`
    where `rbpy * nx` was meant.
  - **Feed** (organism menu) gives 10 energy and *adds* 10 CO2, where it
    should take it. The total of CO2 and O2 grows each time you use it.
  - The test for parallel lines in collisions treats every negative
    determinant as parallel. It then places the contact at the midpoint of
    the four ends instead of where the segments cross.

Checked against the shipped jar itself: its own classes, unmodified, run
headless next to the C port and both dump their state every frame (see
[Source and tests](#source-and-tests)).

- **Identical over 30,000 frames on each of 10 seeds**, every frame
  compared.
- **Identical over 600,000 frames on each of 3 seeds**, every 100th frame
  compared.
- **Identical over 20,000 frames on each of 100 seeds**, every frame
  compared by hash (the drift study below).
- **Genetic codes** (`.bgg`) are read as 0.9 reads them: the same files
  accepted, the same values to the last bit, the same files refused. Checked
  on 1,003 files: 909 the 0.9 jar wrote (random, mutated for 200
  generations, evolved in worlds, extreme values, 100 and 500 genes), the
  15 in the [Biogenesis gallery](https://biogenesis.sourceforge.net/gallery.en.php),
  and 79 hand-made variants (other encodings and line ends, references and
  entities, DTD defaults, number forms Java accepts and refuses). Also
  checked on 300,000 random number strings against `Double.parseDouble`.
- **Saved worlds** (`.bgw`) pass both ways:
  - A world started here saves to exactly the bytes 0.9 saves: 30 of 30
    seeds and lengths, from a new world to 7,000 frames.
  - Each program loads the other's files and continues identically.
  - A loaded file saved again is unchanged, 30 of 30.
  - A world loaded and then run on saves with the same content as 0.9's
    file, but not always the same bytes. After loading, 0.9 holds two
    copies of some colours (the ones it read and its own constants) and
    writes both. 0.9 continues identically from either file.
  - Files Java 8 wrote load here, and Java 8 loads these and saves them
    back unchanged.
  - 4,000 damaged files are refused or loaded without a crash, checked
    under the address and undefined-behaviour sanitizers.
- `Math.sin`, `cos` and `pow` identical to the JVM on 12 million inputs,
  including huge, tiny, infinite and NaN ones; `atan2` on 2 million. `make
  test` repeats 2 million of each.

The reference is OpenJDK 27 on x86_64, with the same parameters on both
sides. Each program keeps its own: this one in
`~/.config/syn-biogenesis/params.cfg`, the Java one in Java's preferences.

## Lapsadian determinism and drift

Biogenesis is a lapsadian world, in the sense of Laplace's demon: know the
state of the dish at one frame and the rules, and every later frame is
fixed. Nothing in it is truly random. All of its chance (mutations, cilia
kicks, where a child lands) comes from one pseudo-random stream started by
the seed, so a seed isn't the start of a world, it is the world's entire
history. That is what makes a bit-for-bit reimplementation possible, and
checkable.

The demon's catch is that the rules include the arithmetic. Every
floating-point operation rounds to 53 bits, and `sin`, `cos` and `pow` are
computed by routines that may round the last bit differently in a few
percent of cases. The Java specification allows exactly that: `Math.sin`,
`cos` and `pow` only have to be within one unit in the last place of the
exact result, so implementations may differ; only `StrictMath` must give
the same bits everywhere. Biogenesis uses `Math`.

### How one last bit becomes a different world

What happens to a one-bit difference depends on where it lands. Run seed
42 on the C library's maths instead of the JVM's, and at frame 1,121 one
result rounds the other way. The difference appears in the last bit of
organism 13's spin. A frame later it has spread to the velocity of
organism 22, whose bounding box overlaps 13's. Five frames after that it is
gone. Each of those values receives a change tens to hundreds of times
larger than itself, and the one-bit difference falls below the precision
of the sum and rounds away. From frame 1,127 the two worlds are identical
again.

Differences heal like that when they are absorbed into larger numbers or
clamped at a speed limit. One that isn't absorbed keeps spreading, and
once it reaches a decision it can't be rounded away: an integer bounding
box that moves by a pixel, a contact that happens a frame earlier, a child
that finds room on the other side of its parent. From then on the
difference is in what happened, not in a rounding. Seed 42's two worlds
are never identical again from frame 4,501, and the number of organisms
alive differs by frame 9,480:

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/drift/population-dark.svg">
  <img alt="Population of seed 42 over 20,000 frames: Java 27 and SYN-BIOGENESIS identical; the C library maths version parts from them" src="docs/drift/population-light.svg">
</picture>

After two worlds part, the gap between them grows the way chaos grows. In
the median seed it starts around 10⁻¹⁴ of a pixel and grows about tenfold
every 150 to 200 frames, for well over a thousand frames in which nothing
on screen differs. It crosses one pixel 1,550 frames after the worlds part
with the C library's maths, 1,708 with Java 8 and 2,787 with fdlibm.
Within a few hundred frames more, the same organism is hundreds of pixels
away in the other world:

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/drift/growth-dark.svg">
  <img alt="Largest position gap between the same organism in two worlds, and the share of organisms that differ, by frames since the worlds parted" src="docs/drift/growth-light.svg">
</picture>

### Seeds, Java versions and processors

The drift study (`tools/drift/`) runs 100 seeds for 20,000 frames each on
the shipped jar and on other implementations, and compares each with Java
27 on every frame:

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/drift/divergence-dark.svg">
  <img alt="Share of 100 seeds parted for good from Java 27, by frame, for each implementation" src="docs/drift/divergence-light.svg">
</picture>

| Compared with Java 27 | Same world for all 20,000 frames | Seeds that ever differ | First difference: median frame (range) | Seeds that part for good | Frame they part: median (range) | Differences that healed |
|---|---|---|---|---|---|---|
| SYN-BIOGENESIS | 100 of 100 | 0 | never | 0 | never | 0 |
| Java 21 | 100 of 100 | 0 | never | 0 | never | 0 |
| Java 17 | 100 of 100 | 0 | never | 0 | never | 0 |
| Java 11 | 100 of 100 | 0 | never | 0 | never | 0 |
| Java 8 | 44 of 100 | 56 | 2,477 (716 to 18,337) | 46 | 2,342 (716 to 18,337) | 16 |
| Java 27, fdlibm maths | 0 of 100 | 100 | 80 (1 to 10,250) | 97 | 1,211 (24 to 18,433) | 613 |
| SYN-BIOGENESIS, C library maths | 0 of 100 | 100 | 1,651 (80 to 7,443) | 100 | 2,199 (80 to 18,600) | 88 |

Per-seed results are in [`docs/drift/results.csv`](docs/drift/results.csv).
What each run says:

- **SYN-BIOGENESIS** never differs from Java 27 on any seed.
- **Java 11, 17 and 21** don't either. Their `sin`, `cos` and `pow` give
  exactly Java 27's results here, on a million random inputs.
- **Java 8** computes `sin` and `cos` its own way (about 2.5% of inputs
  round differently from Java 27). The Biogenesis jar was built for it. On
  44 of the 100 seeds no difference ever shows in 20,000 frames; on 46 the
  worlds part for good, at a median of frame 2,342.
- **Java 27 with `-XX:-UseLibmIntrinsic`** turns the Intel routines off and
  computes `Math` with fdlibm, which is what HotSpot does on processors it
  has no routines for. In OpenJDK 27's source, the RISC-V, POWER and s390
  ports install none of their own for these. Every seed differs, the first
  time after a median of only 80 frames, but most of those differences
  heal (613 in all) before the worlds part for good. 97 of the 100 seeds
  part, at a median of frame 1,211.
- **SYN-BIOGENESIS on the C library's maths** (`make libc`) is what a build
  for a processor other than x86_64 runs. Every seed parts for good, at a
  median of frame 2,199.

Not measured here:

- **ARM (aarch64):** HotSpot has its own `sin` and `cos`, described in its
  source as an optimised fdlibm, and computes `pow` with fdlibm.
- **AMD:** Intel's `pow` uses `rcpps`, an approximate reciprocal whose exact
  results differ between processor makers. SYN-BIOGENESIS executes the same
  instruction, so it matches the JVM on whatever x86_64 processor both run
  on.
- **32-bit x86 before Java 17:** the JVM could keep intermediate results in
  80-bit x87 registers, so even `+` and `×` could round differently (JEP
  306 made all floating point strict in Java 17).

So the Java program has never been one world per seed. It is one world per
seed, per JVM family and processor. SYN-BIOGENESIS reproduces the one that
modern HotSpot computes on x86_64. A saved world (`.bgw` or `.sbw`) holds
every organism's exact state but not the random-number stream, which
neither program saves, so a loaded world takes a new path from there.

## Build and run

Needs a C compiler, SDL2, SDL2_ttf and SDL2_image (on Arch:
`sdl2-compat sdl2_ttf sdl2_image`).

```sh
make
./syn-biogenesis            # random world
./syn-biogenesis 42         # seeded, like `java -jar biogenesis.jar 42`
```

`make headless` builds `syn-biogenesis-headless`, the simulation alone (no
SDL), for benchmarks and long runs:

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

### Differences from the Java version

- **World files:** Open reads 0.9's `.bgw` and this program's `.sbw`;
  Save writes `.bgw` unless you name the file `.sbw`. Worlds pass both ways
  between the two programs. Neither format stores the random-number state
  or the parameters, so a loaded world continues differently in each
  program unless both are given the same seed after loading.
- **Genetic codes** stay `.bgg` and pass both ways. The only difference: a
  code with no DTD block, which 0.9 refuses, opens here as if it had 0.9's.
  Codes from 0.4 to 0.6 (genes as x and y, like the six "for version 0.6"
  in the gallery) are refused, as 0.9 refuses them. So are Color Mod codes
  that use its own colours. A Color Mod code that uses only the seven
  original colours opens in both, without its extra traits.
- **Not ported:** the Network menu (biological corridors between computers),
  translations (English only), "check last version", and the Java2D OpenGL
  options.
- **Speed control:** see the table above. The Java window's FPS display
  shows its current frame delay (1000 ÷ delay), not a measured rate. This
  one measures.

## Source and tests

```
src/sim/        simulation: organism, world (with the STRtree), genetic code, stats,
                java.util.Random, java.awt geometry semantics, parameters, and the
                JVM's sin/cos/pow (jvm_libm_x86_64.S, generated)
src/fdlibm/     fdlibm's atan2, which is what Math.atan2 is
src/io/         .bgg genetic codes (XML); .bgw worlds (Java serialization, jser.c);
                .sbw worlds
src/ui/         SDL2 front end: immediate-mode widgets, world view, dialogs
src/headless.c, src/main.c
tests/          make test
tools/javaref/  the Java reference: runs the shipped jar headless
tools/jvmlibm/  translates HotSpot's sin/cos/pow into jvm_libm_x86_64.S
tools/drift/    the drift study and its charts (docs/drift/)
```

```sh
tools/javaref/fetch.sh   # once: downloads Biogenesis 0.9, builds the Java reference
make test
```

`make test` runs these checks:
- the file formats: a world saved and reloaded mid-run, as `.sbw` and as
  `.bgw`, continues identically, and damaged files are refused
- 800 damaged `.bgw` files loaded without a crash
- the genetic-code cases in `tests/bgg_cases/` (what 0.9 reads from each
  is in its `expected.txt`)
- the geometry, maths and number-parsing checks against the JVM
- genetic codes written by the jar (and the gallery's, once fetched), read
  by both programs
- worlds saved by the jar and by the port, compared byte for byte, loaded
  by the other and continued
- the frame-by-frame comparison with the shipped jar The Java parts are
skipped without a JDK. The reference runs the jar's own classes, not
recompiled and not patched. Only the Swing classes they call into are
replaced with no-op stand-ins, so they run without a window.

`tools/drift/drift.py` reruns the drift study (about half an hour on six cores;
see [`tools/drift/README.md`](tools/drift/README.md)).

For UI checks without a display, the app has a test hook driven by
environment variables (see `shot_prepare` and `Script` in `src/ui/app.c`):

```sh
SDL_VIDEODRIVER=offscreen SYN_BIOGENESIS_SHOT=out.png SYN_BIOGENESIS_SHOT_SIM=3000 ./syn-biogenesis 42
SDL_VIDEODRIVER=offscreen SYN_BIOGENESIS_SCRIPT="wait 5; click 300 300 r; click 340 344; shot a.png; quit" ./syn-biogenesis
```

## Credits

SYN-BIOGENESIS is a reimplementation. The ideas, the simulation and its
behaviour are the original's:

- **Biogenesis**, copyright (c) 2006-2010 Joan Queralt Molina:
  <https://biogenesis.sourceforge.net/>
- **Biogenesis 0.9** by MarcoDBAA (2019), with STRtree collision detection
  (JTS, by Vivid Solutions) implemented by Richard David Williams.
- **Icons** are copied unchanged from Biogenesis, which credits its artwork
  to Ananda Daydream and Florian Haag
  ([Toolbar Icons](https://sourceforge.net/projects/toolbaricons/),
  copyright (c) 2010 Florian Haag, MIT licence). See
  [`assets/icons/LICENSE`](assets/icons/LICENSE).
- **JTS** STRtree, translated in `src/sim/world.c`: copyright (c) 2016
  Vivid Solutions, Eclipse Distribution License 1.0
  ([`src/sim/LICENSE.JTS`](src/sim/LICENSE.JTS)).
- **Math.sin, Math.cos, Math.pow** (`src/sim/jvm_libm_x86_64.S`): the Intel
  Math Library routines in OpenJDK's HotSpot, copyright (c) 2016, 2025
  Intel Corporation, (c) 2021 Tencent, (c) 2022, 2025 Oracle, GPL version 2
  only.
- **fdlibm** (`src/fdlibm/`) is Sun Microsystems' freely distributable libm.
- **Noto Sans** is under the SIL Open Font License (`assets/fonts/OFL.txt`).

## Licence

GPL version 2 or later, like Biogenesis (`LICENSE`), except
`src/sim/jvm_libm_x86_64.S`, which comes from OpenJDK under the GPL version
2 only; the program built with it is therefore distributed under version
2. Each source file names the original file it was translated from or
based on; the changes from the original are described above, and the
history is in git.
