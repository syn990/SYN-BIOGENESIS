# The seed-cracking engine: a being's worlds from its genes alone

Given only a being, a `.bgg` file with no world and no save, the engine in
[`src/syn/crack.c`](../../src/syn/crack.c) reads back:

- its genesis ancestor, the founder it comes from, gene for gene;
- every seed whose world begins with that founder, and how likely each is
  to end with the founder's bloodline the only one left;
- the being's mutations, in the order they were drawn.

Replays then say which of those worlds is the being's, the tick it is
born, and the tick it reigns: its ancestor's bloodline alone and the
being alive in it. Run the other way, the same weighing forges winners
straight from the dice.

Everything here is Biogenesis 0.9 with its default parameters (1,000 x
1,000, 15 first organisms), the world the arena fights in, unless it says
otherwise.

## One gene gives the founder

Every random decision in a world is a draw from one `java.util.Random`
(see [the telescope](../../README.md#the-dice-telescope-a-worlds-seed-read-back-from-its-save)).
A founder is a fixed run of those draws:

| Draws | What |
|---|---|
| 2 | `org_new`'s angle |
| 1 | mirrored or not |
| 1 | arms (1 to 8) |
| 1 | how many genes (1 to 8) |
| 5 per gene | its length (2), angle (2) and colour (1) |
| 1 | dispersal |
| 2 per try | a place (up to 12 tries) |

A gene length is one `nextDouble`, so cracking it gives the state that
drew it (almost always just one state). Gene *j*'s length is drawn 5 + 5*j*
draws after the founder was begun. Stepping back that far gives the state
the founder began at, and drawing again from there, as `gc_new_random`
does, gives the whole founder: every gene, the arms, the mirroring, the
dispersal.

Tried on all 15 founders of 200 worlds, every one of their 13,506 genes
gave back its founder exactly, and the state it was begun at. `make test`
checks the same for the 180 founders of 12 worlds.

A being has usually been mutated since. Any one gene it still has from
the founder is enough, and the founder's other genes, wherever they still
are in the being, prove the find: each length and angle is 53 bits, so a
second one matching is not chance.

### The same genes, read a gene along

A founder's genes are blocks of five draws, and so is what comes before
them (angle, mirroring, arms, number of genes). So the same run of genes
also reads as founders begun one gene earlier or later. Each of those
readings has its own arms, mirroring and number of genes taken from the
draws before it, and each is the founder of some real seed: every state
is where founder 1 of exactly one seed begins, the seed whose first state
it is. The being's genes are in all of these readings.

The engine puts first the reading the being is the fewest mutations from.
The genes are aligned in order, since `gc_new_mutated` keeps their order.
A gene kept costs nothing, and a gene dropped or added costs one. So does
each length, angle or colour drawn again, and each change of arms,
mirroring or dispersal. Between two readings that cost the same, the
engine takes the one holding more of the being's draws. On a forged
winner with nine mutations, three readings tied at nine. The true one
held nine of the being's draws, the others four and one.

## Every seed that makes it: the Kruskal count

Genesis makes the founders one after another from one stream. Start a few
draws earlier and the first founder is different. But as soon as one
founder happens to begin at a state the other start's founders also begin
at, every founder after it is the same, and almost always in the same
place. Two starts merge onto one chain of founders, the effect behind the
card trick called the Kruskal count.

So the seeds that make a given founder are all within a few hundred draws
per founder slot before it. The engine tries every one of them
(`crack_basin`), in about 0.05 s for 15 founders. Over 1,000 random states
(`--telescope basins 1000 --defaults`):

| Seeds that make a founder at the state | States |
|---|---|
| 1 (only the seed whose first state it is) | 357 |
| 2 | 130 |
| 3 to 9 | 249 |
| 10 to 63 | 198 |
| 64 or more | 66 |

The mean is 14 seeds and the most is 201.

## Its later draws, in order

A gene the being has and its ancestor hasn't is a later draw: a length
drawn again, a gene added whole (its angle drawn straight after its
length), or an angle drawn again. Each is cracked to its state and placed
by counting the draws to it from the ancestor's start. The counts give
the order of the mutations and how far apart they were.

A winner forged from seed 589 (founder 14, eight genes, seven arms):

| Gene | Drawn again | Draws after its ancestor began |
|---|---|---|
| 6 | length | 30,377 |
| 5 | length | 78,763 |
| 4 | angle | 273,014 |
| 2 | length | 3,057,809 |
| 8 | length | 3,728,228 |

With no ancestor left, the stream's start is unknown, but the draws still
have an order. #496, the champion bred for `syn1.sbw`
([`docs/being/`](../being/README.md)), has eight genes. All eight are
lengths drawn within 159 million draws of each other on one stream. Genes
1 and 6 are 44 draws apart, so they were drawn in the same birth.
`ultimate-syn1.bgg` shares five of those genes, and the gaps between them
are the same to the draw (gene 4 to gene 8: 10,961,859 in both).

## Which bloodline is left

The [fade study](../fade/README.md#the-lone-bloodline) found every world
coming down to one bloodline by about time unit 100. Which one is not all
drift. In 0.9 every energy flow is a segment's length times a constant, so
a plant's rate of making children can be read from its code alone:

- light: the green segments' length / 500, times CO2 / 5,000 (at most 1);
- breathing: the whole mass / 5,000;
- reproduction: at reproduce energy (40 + 3 x genes x arms) it splits
  into its litter (1 child, plus one for each yellow segment) and itself,
  each with reproduce energy / (litter + 1);
- time: what it needs to gain at light minus breathing, at least the 20
  frames between litters, plus 120 frames to grow; nothing if that is
  longer than its life.

Its fitness is log(litter + 1) / time. The chance that a founder's
bloodline is the one left is its fitness to the power 2.5, over the sum of
all the founders' (a founder that can't feed itself as a plant counts as
0.0004). The power and the floor were fitted by maximum likelihood on 240
worlds run until one bloodline was left.

Run through the engine itself (`--telescope forge --top 240`, which runs
every seed weighed), on the seeds it was fitted on and on 240 it never
saw:

| | Seeds 1 to 240 (fitted on) | Seeds 100,001 to 100,240 (unseen) |
|---|---|---|
| One bloodline left | 239 | 240 |
| The fittest founder's | 107 (45%) | 111 (46%) |
| Predicted, on average | 43% | 43% |
| Picking a founder at random | 7% | 7% |
| Time unit it happened, median (middle half) | 116 (70 to 213) | 125 (71 to 243) |

By how sure the prediction was (both sets together):

| The fittest founder's chance, predicted | Seeds | Predicted, on average | Its bloodline left |
|---|---|---|---|
| under 20% | 8 | 19% | 0 |
| 20% to 40% | 227 | 31% | 83 (37%) |
| 40% to 60% | 175 | 49% | 90 (51%) |
| 60% to 80% | 57 | 69% | 36 (63%) |
| 80% and over | 12 | 84% | 9 (75%) |

The founder the formula likes best was the one left about six times as
often as a founder picked at random, and about as often as predicted.
Drift decides the rest: in a population of 25 to 67 (the fade study's), a
fitter line still loses often.

## Where it lives, and where it reigns

`--telescope find FILE.bgg` (TELESCOPE > WHERE A BEING LIVES, or Where on
an organism's card) puts it together:

1. the being's ancestor, and its later draws;
2. the seeds that make the ancestor, the likeliest to end with its
   bloodline alone first;
3. each seed run. As the stream passes each of the being's later draws,
   the gene drawn there has to be in a child just made. If it isn't, this
   is not the being's world, and the run stops there. A being with later
   draws lives in at most one of the seeds. A being without (its ancestor
   unchanged, or changed only in ways no draw records: a colour, the arms,
   a gene dropped) is looked for in each.

What a run shows is a list of scenes, each a tick to JUMP to:

- the ancestor at tick 0, and each later draw where it is made;
- the being alive, and the most of it;
- its ancestor's bloodline alone, and the being alive while it is: it
  reigns, the winning state;
- or the bloodline dying out.

| Being | Seeds that make its ancestor | Found | Time |
|---|---|---|---|
| seed 589's winner (5 later draws) | 62 | seed 589: alive from tick 10,240, reigns at time 66 | 4.0 s |
| seed 1228's winner (2 later draws) | 46 | seed 1228: alive from tick 1,024, reigns at time 42 | 2.3 s |
| seed 2595's winner (a gene dropped, no later draw) | 7 | its ancestor's bloodline alone in all 7; it reigns in 3 (seed 2595 at time 41) | 21 s |

The wrong seeds were given up at ticks 267 to 460, the first time their
stream passed the being's earliest later draw (all but one, whose
ancestor's bloodline died out at time 1). The seeds that make a
founder share most of their founders after the point where their chains
merged, so the ancestor's chances are much alike across them: 85% to 94%
for these three.

## Forge a winner

`--telescope forge` (TELESCOPE > FORGE A WINNER) runs the weighing
forwards. Each seed's founders are made with no world run, about 13,000
seeds a second on one core. Each seed is weighed by its likeliest
founder's chance, and the likeliest seeds are run until one bloodline is
left. The being it is left as, the commonest code alive then, is the
winner. Weighing seeds 1 to 20,000 took 1.5 s, and running the 4
likeliest 18 s more. The expected founder's bloodline was the one left in
all 4, at time units 32 to 506. In seed 4361 it was left as the founder
itself, unchanged.

## Where it stops

- **A being with no founder gene left.** Its genes are put in order on
  their stream, but the stream's start, its seed, is gone (the fade).
- **Parameters.** Founders drawn again take their colours from the colour
  odds of the parameters. Which seeds make a founder depends on the
  world's size and number of founders. The arena's world and the saved
  parameters are both searched.
- **Hunters and infectors.** The fitness is a plant's. Red, white and gray
  segments cost and gain energy through other organisms, which no code
  alone says. In the fitting run, 3 of the 239 winners could not feed
  themselves as plants at all.
- **Changes made from the menus** (an import, CO2 added) take a world off
  the thread its seed began; no seed replays to it.
- **Color Mod 2.01** draws its founders differently. The engine is 0.9's
  only.

## Running it again

```sh
make
# an ancestor, its seeds and its later draws, in a fraction of a second
syn-biogenesis --telescope crack BEING.bgg --defaults
# the same, with each seed run for the being and its bloodline
syn-biogenesis --telescope find BEING.bgg --defaults --no-record
# how many seeds make a founder at a state
syn-biogenesis --telescope basins 1000 --defaults
# the study: every seed weighed and run to its lone bloodline
syn-biogenesis --telescope forge --defaults --from 1 --scan 240 --top 240
```
