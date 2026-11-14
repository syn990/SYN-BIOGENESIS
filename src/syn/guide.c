/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Part of the SYN edition of SYN-BIOGENESIS, a C port of Biogenesis
 * (https://biogenesis.sourceforge.net/).
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version. It is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License (LICENSE) for more details.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Written from the simulations' code (0.9's Organism.touchEffects and
 * segment effects; Color Mod 2.01's Organism.java), with Color Mod's own
 * readme for what its modifier genes are for. "A bite" is what an eating
 * segment takes in one touch: 0.9's length x 0.5, Color Mod's
 * 2 x the square root of its length. */
#include "syn/guide.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    const char *name; /* as syn_col_name has it */
    const char *line;
    const char *more;
} Entry;

#ifdef BIO_COLOUR

static const Entry k_entries[] = {
    /* leaves */
    {"green", "Leaf: makes energy from CO2.",
     "The plain leaf. Every frame it turns CO2 from the air into energy, more for a longer segment. "
     "Every leaf earns more on a body with fewer genes and fewer arms, and a body with fewer than 8 arms "
     "often fails to photosynthesise when CO2 is low."},
    {"forest", "Colony leaf: earns more while touching plants.",
     "A leaf that earns extra while the organism touches another plant (a friend counts too): the most "
     "when forest touches forest, less for other leaves, least for other segments."},
    {"spring", "Spinning leaf: turns its organism round.",
     "A leaf that also spins the organism, clockwise or the other way as its Clockwise gene says. "
     "Longer spring segments spin it harder."},
    {"summer", "Seasonal leaf: rests one season in three.",
     "A strong leaf that goes dormant, shown as winter, for one time unit in every three; the Activity "
     "gene picks which. It makes nothing while dormant."},
    {"lime", "Loner's leaf: strong alone, weak when crowded.",
     "Full strength while nothing touches the organism; crowded, only the 'Lime effectivity if not "
     "alone' share of it (half, by default)."},
    {"leaf", "Undisturbed leaf: works only when left alone.",
     "Makes energy only while the organism isn't flashing a colour, so any attack or action that lights "
     "it up stops it, and after an attack it can go dark green for a while."},
    {"c4plant", "Small leaf: earns about the same at any length.",
     "Earns 0.55 x (10 + its length), so a short C4 segment earns nearly as much as a long one while "
     "its upkeep, like every segment's, grows with length. A body whose only leaves are C4, with no "
     "shield, killer, virus or eater, also dodges most attacks for a frame and earns a little more."},
    {"jade", "Antiviral leaf: its body's leaves can't be infected.",
     "A leaf that protects the organism's other leaves from viruses: an infecting touch only knocks "
     "the leaf out for a while (dark jade, dark green) instead of infecting it. Recovers sooner on "
     "bodies with fewer arms."},
    {"grass", "Tough leaf: eaters get half from it.",
     "A leaf that eaters, spike and lilac take only half as much from."},
    {"bark", "Armoured leaf: hardens to old bark when attacked.",
     "A weaker leaf (0.7 of green) that hardens into old bark when something attacks it. Old bark "
     "protects but makes nothing."},
    {"purple", "Methane eater: energy from CH4 every 8 frames.",
     "Makes energy from methane, which dead bodies rot into, instead of CO2, once every 8 frames. Only "
     "0.55 of that if the organism also has leaves. Its upkeep is a hundredth of normal."},
    /* eaters */
    {"red", "Carnivore: eats other eaters.",
     "Bites only organisms that are eaters themselves (cream excepted); it does nothing to plants. With "
     "lavender it resists plague and fallow; with dark gray it also eats cream and some killers."},
    {"fire", "Generalist: full bites on eaters, quarter on the rest.",
     "Bites eaters at full strength and everything else at a quarter. With dark gray it also eats "
     "corpses."},
    {"orange", "Omnivore: eats almost anything.",
     "Bites nearly every kind of segment, leaves included. Shields (blue, ochre, sky) stop it, and so "
     "do red, maroon and cream segments."},
    {"maroon", "Herbivore: eats plants, costs little.",
     "Bites the leaves and other parts of plants, some protected ones too. Its upkeep is a fifth of "
     "normal."},
    {"pink", "Decomposer: eats corpses, the weak and viruses.",
     "Eats corpses, damaged or weakened segments, pink, cream, and viruses (white, plague, coral). Its "
     "digestion releases methane, not CO2. With gray it eats corpses only if Mod Pink is set."},
    {"cream", "Parasite: small bites, hard to infect.",
     "Takes a tenth of a bite but is mostly immune to infection. With dark gray, its Mod Cream gene "
     "(1 to 3) widens what it feeds on."},
    /* killers */
    {"spike", "Spike: its point drains energy; its line is armour.",
     "The point drains five bites' worth of energy from what it touches, and gains nothing. The line is "
     "hard to hurt, except on plants. With dark gray the point eats instead of draining."},
    {"lilac", "Weakener: one big drain, then it recharges.",
     "Drains ten bites' worth of energy in one touch, then recharges (dark lilac) unless the organism "
     "has dark gray. Without the Mod Lilac gene it leaves harmless segments alone."},
    {"gray", "Killer: kills what it touches.",
     "Kills the organism it touches outright, for 0.1 energy. Blue, spike points and corpses stop it; "
     "magenta lets a victim survive once; olive breaks it; bark hardens instead. An organism with gray "
     "can never dodge."},
    {"violet", "Poison: ruins the segment it touches.",
     "Poisons the segment it touches, which withers and stops working until healed. It poisons leaves "
     "only if the poisoner is a plant or an eater. Blue blocks it; olive breaks it."},
    {"olive", "Defence cracker: breaks shields and killers.",
     "Breaks the blue shields of plants, sky and ochre, and the killing parts of others (gray, violet, "
     "lilac, spike). Gray and violet weaken it in turn."},
    {"fallow", "Birth blocker: stops others reproducing.",
     "Stops the organism it touches from reproducing for a while. Its Mod Fallow gene (1 to 4) picks "
     "how: against spores, against fungi, stillbirths, or destroying reproductive segments."},
    /* defence */
    {"sky", "Freezer: stops what it touches from moving.",
     "Freezes the movement segments of what it touches (they turn to ice), and shields itself with "
     "frost, shown as deep sky. With Mod Sky it freezes leaves too."},
    {"blue", "Shield: blocks almost every attack.",
     "Stops eaters, killers, viruses and poison, at no cost with the default parameters. Olive can "
     "crack the blue of plants. With dark gray, whatever attacks it loses energy."},
    {"ochre", "Pusher: shoves others away.",
     "Pushes organisms away, harder with a longer segment, and protects against many attacks. With dark "
     "gray the push costs the pushed organism energy."},
    /* viruses */
    {"white", "Virus: makes others give birth to its kind.",
     "Infects the organism it touches, whose next child is then a copy of the infector. Only plants and "
     "eaters can be infected. Blue, sky, ochre, mint, lavender and spike points resist it; bark hardens."},
    {"plague", "Forcer: makes the infected give birth at once.",
     "Forces an organism it has infected to give birth to the virus straight away. Mod Plague picks "
     "whether it forces eaters or plants."},
    /* helpers */
    {"coral", "Transformer: turns small viruses into its kind.",
     "Kills organisms that are neither plants nor eaters, such as small viruses, and turns them into "
     "its own kind."},
    {"mint", "Immune system: cures infections, breaks viruses.",
     "Cures the organism's infections over time, faster with more mint, and destroys the white, plague, "
     "coral and fallow segments of organisms it touches."},
    {"lavender", "Immunity shield: against parasites and viruses.",
     "Builds a shield against cream, plague, coral, fallow and strong ochre. A white, plague, coral, "
     "cream or fallow attacker that touches lavender loses energy instead."},
    {"magenta", "Healer: repairs damaged segments.",
     "Heals the organism's withered, broken and frozen segments over time, and lets it survive one gray "
     "kill, after which its magenta is spent until healed."},
    {"rose", "Friend: shares energy with family and friends.",
     "Gives energy to related or friendly organisms. With the Social gene, organisms whose rose has the "
     "same length and angle don't attack each other; with Peaceful, peaceful rose organisms leave each "
     "other alone. A rose organism that is neither plant nor eater feeds the plants it touches."},
    {"indigo", "Decoy: starves newborn viruses.",
     "Cuts the energy of a virus child this organism is made to bear; below 3 energy the virus isn't "
     "born. It also wastes the effort of viruses and coral."},
    /* movement */
    {"cyan", "Mover: kicks the organism in random directions.",
     "Each frame there is an 8 in 100 chance that it pushes the organism along its own direction. Free "
     "with the default parameters."},
    {"teal", "Reflex mover: reacts to what it touches.",
     "Moves like cyan, and lets the organism use its reaction genes: stand still, flee or attack, "
     "depending on the colour it touches. Harmless organisms with teal can dodge. With Passive it waits "
     "still until touched."},
    /* reproduction */
    {"yellow", "Fertility: more children per birth.",
     "One more child per yellow gene, and one more for every 23 of total yellow length."},
    {"auburn", "Survivor: still has its own child when infected.",
     "An infected organism with auburn gets one child of its own besides the virus."},
    {"blond", "Early bloomer: reproduces on less energy.",
     "Lowers the energy it needs to reproduce, by its length plus 3 for each blond segment, to no less "
     "than 40 for plants."},
    {"flower", "Late bloomer: waits for more energy, then hurries.",
     "Raises the energy needed to reproduce, then lets the organism reproduce again without the usual "
     "wait."},
    {"spore", "Spores: its children are spores.",
     "Its children are spores of one colour; the Mod Spore gene (1 to 12) picks which: fruits, seeds, "
     "eggs, larvae, sleeping or drifting spores."},
    /* senses */
    {"eye", "Eye: sees without touching.",
     "Doesn't collide; a teal organism (or a cyan one, for the eye alone) reacts to what the eye "
     "crosses. Half as long again as other segments."},
    /* others */
    {"silver", "Experience: more children as it ages; bites by kills.",
     "Gives more children per birth as the organism's children add up, up to 8. With kills, or dark "
     "gray, it bites by its kills against the victim's."},
    {"gold", "Long life: the organism lives longer.",
     "Adds its length, over the Gold divisor, to the organism's life expectancy. Cheap upkeep."},
    {"dark gray", "Enhancer: strengthens other colours.",
     "Changes what many other colours do (the 'enhanced' rules): gray kills more, blue hurts attackers, "
     "some eaters take corpses, lilac needn't recharge."},
    {"black", "Mimic: copies the colour it touches.",
     "Copies the colour and reactions of the segment it touches, leaves excepted. With Mod Black every "
     "black segment changes at once."},
    /* what segments turn for a while */
    {"vision", "An eye that isn't at an arm's end.", "An eye must end its arm; one that doesn't is vision."},
    {"fruit", "A fruit spore, waiting to be eaten.", "A fruit spore (Mod Spore 1 or 2), until it is eaten."},
    {"deep sky", "Sky's frost shield, up after an attack.", "Sky behind its frost shield; it thaws back to sky."},
    {"spike point", "The point of a spike, where it drains.", "The end of a spike segment that touched first."},
    {"withered", "Destroyed (not a plant): out of action.", "Out of action until magenta heals it."},
    {"poisoned", "A poisoned leaf: makes nothing.", "Out of action until magenta heals it."},
    {"broken", "A plant's destroyed segment: out of action.", "Out of action until magenta heals it."},
    {"ice", "Frozen: can't move or photosynthesise.", "Frozen by sky until magenta heals it."},
    {"numb", "Cracked or numbed: out of action.", "A shield cracked by olive, or sky frozen by sky."},
    {"winter", "A summer leaf resting this season.", "Summer's dormant season; it wakes the next."},
    {"dark green", "A leaf knocked out; it recovers.", "Knocked out by an attack on a leaf; it comes back."},
    {"dark jade", "Jade that just stopped a virus; still works.", "It turns jade again."},
    {"spent", "Used up for now; it recovers.", "Magenta after saving it from gray, or fire poisoned."},
    {"recharging", "Lilac recharging after a drain.", "It turns lilac again."},
    {"dark olive", "Weakened olive; it recovers fast.", "Weakened by gray or violet."},
    {"old bark", "Hardened bark: protects, makes nothing.", "Bark after an attack; it stays this way."},
    {"dead bark", "Frozen bark.", "Bark frozen by sky."},
    {"brown", "Dead: corpses rot into methane.", "A dead body, rotting into methane."},
    {"dead", "Dead: corpses rot into methane.", "A dead body, rotting into methane."},
};

#else /* Biogenesis 0.9 */

static const Entry k_entries[] = {
    {"green", "Leaf: makes energy from CO2.",
     "Every frame it turns CO2 from the air into energy, as much as its length allows."},
    {"red", "Eater: bites whatever it touches.",
     "Takes half its length in energy from any segment it touches; only blue stops it."},
    {"cyan", "Mover: kicks the organism in random directions.",
     "Each frame there is an 8 in 100 chance that it pushes the organism along its own direction."},
    {"blue", "Shield: stops red, white and gray.",
     "Blocks bites, infections and kills, at no cost with the default parameters."},
    {"white", "Virus: makes others give birth to its kind.",
     "For 1 energy it infects the organism it touches, whose next child is then a copy of the infector. "
     "Blue stops it."},
    {"gray", "Killer: kills what it touches.",
     "For 1 energy it kills the organism it touches outright. Blue stops it."},
    {"yellow", "Fertility: one more child per segment.",
     "Every yellow segment is one more child at each birth."},
    {"brown", "Dead.", "A dead body. Parents and children never attack each other while alive."},
};

#endif

static const Entry *entry(int slot)
{
    const char *name = syn_col_name(slot);
    for (size_t i = 0; i < sizeof k_entries / sizeof k_entries[0]; i++)
        if (!strcmp(k_entries[i].name, name))
            return &k_entries[i];
    return NULL;
}

const char *syn_guide_short(int slot)
{
    const Entry *e = entry(slot);
    return e ? e->line : "";
}

const char *syn_guide_long(int slot)
{
    const Entry *e = entry(slot);
    return e ? e->more : "";
}

bool syn_guide_is_gene(int slot)
{
#ifdef BIO_COLOUR
    return slot >= 0 && slot < 46;
#else
    return slot >= 0 && slot < GENE_COLORS;
#endif
}

/* letters only, lower case: "Dark Gray" and "darkgray" are one */
static void squash(const char *s, char *out, size_t n)
{
    size_t k = 0;
    for (; *s && k + 1 < n; s++)
        if (isalnum((unsigned char)*s))
            out[k++] = (char)tolower((unsigned char)*s);
    out[k] = '\0';
}

int syn_guide_find(const char *name)
{
    char want[64], have[64];
    squash(name, want, sizeof want);
    if (!strcmp(want, "c4"))
        snprintf(want, sizeof want, "c4plant");
    for (int s = 0; s < SYN_NCOL; s++) {
        squash(syn_col_name(s), have, sizeof have);
        if (!strcmp(want, have))
            return s;
    }
    return -1;
}

const char *syn_guide_role(Role r)
{
    static const char *const t[ROLE_COUNT] = {
        "LEAVES", "EATERS", "KILLERS", "DEFENCE", "VIRUSES", "HELPERS", "MOVERS", "BREEDING", "SENSES",
        "OTHERS", "OUT OF ACTION", "DEAD",
    };
    return r >= 0 && r < ROLE_COUNT ? t[r] : "";
}
