/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Part of a C port of Biogenesis (https://biogenesis.sourceforge.net/).
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

/* .bgg checks with Color Mod 2.01's own BioXMLParser.
 *
 *   ColourBggInterop write DIR SEED N         N random codes from its GeneticCode
 *   ColourBggInterop mutate DIR SEED N GENS   N lineages mutated GENS generations,
 *                                             at the highest mutation rates
 *   ColourBggInterop evolve DIR SEED FRAMES   every 25th living organism's code
 *                                             after FRAMES frames of a seeded world
 *   ColourBggInterop special DIR              extreme lengths, angles, branches
 *                                             and reactions
 *   ColourBggInterop read FILE...             parse (validating DTD) and print
 *                                             each canonically, or "reject"
 *
 * tests/colour_bgg_tool prints the same for the colour build's reader.
 */
package biogenesis;

import java.awt.Color;
import java.io.File;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.List;

public class ColourBggInterop {
	static void write(String dir, String name, GeneticCode g) throws Exception {
		try (PrintStream ps = new PrintStream(new File(dir, name + ".bgg"))) {
			BioXMLParser.writeGeneticCode(ps, g);
		}
	}

	static int b(boolean v) {
		return v ? 1 : 0;
	}

	static String canonical(GeneticCode g) {
		StringBuilder sb = new StringBuilder();
		sb.append(g.getSymmetry()).append(' ').append(g.getMirror()).append(' ')
			.append(g.getMutationrate()).append(' ').append(g.getActivity()).append(' ')
			.append(g.getModifiescream()).append(' ').append(g.getModifiesfallow()).append(' ')
			.append(g.getModifiesspore()).append(' ').append(g.getAdaptspore()).append(' ')
			.append(b(g.getPlague())).append(b(g.getDisperseChildren()))
			.append(b(g.getGenerationBattle())).append(b(g.getSiblingBattle()))
			.append(b(g.getAltruist())).append(b(g.getFamilial())).append(b(g.getSocial()))
			.append(b(g.getPeaceful())).append(b(g.getPassive())).append(b(g.getClockwise()))
			.append(b(g.getMimicAll())).append(b(g.getModifiespink()))
			.append(b(g.getModifieslilac())).append(b(g.getModifiessky()))
			.append(b(g.getSelfish())).append(' ').append(g.getNGenes());
		for (int i = 0; i < g.getNGenes(); i++) {
			Gene e = g.getGene(i);
			int[] r = {e.getredReaction(), e.getgreenReaction(), e.getblueReaction(),
					e.getplagueReaction(), e.getscourgeReaction(), e.getwhiteReaction(),
					e.getgrayReaction(), e.getsilverReaction(), e.getdefaultReaction(),
					e.getconsumerReaction(), e.getplantReaction(), e.getmagentaReaction(),
					e.getpinkReaction(), e.getcoralReaction(), e.getorangeReaction(),
					e.getbarkReaction(), e.getvioletReaction(), e.getvirusReaction(),
					e.getmaroonReaction(), e.getoliveReaction(), e.getmintReaction(),
					e.getcreamReaction(), e.getspikeReaction(), e.getfallowReaction(),
					e.getlightblueReaction(), e.getochreReaction(), e.getskyReaction(),
					e.getlilacReaction(), e.getfireReaction(), e.getlightbrownReaction(),
					e.getgreenbrownReaction(), e.getbrownReaction(), e.geticeReaction(),
					e.getbrokenReaction(), e.getsickReaction(), e.getfriendReaction()};
			sb.append(String.format(" %016x %016x %08x %d", Double.doubleToRawLongBits(e.getLength()),
					Double.doubleToRawLongBits(e.getTheta()), e.getColor().getRGB(), e.getBranch()));
			for (int k : r)
				sb.append(',').append(k);
		}
		return sb.toString();
	}

	static Gene gene(double len, double theta, Color c, int branch, int r) {
		return new Gene(len, theta, c, branch, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r,
				r, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r, r);
	}

	public static void main(String[] args) throws Exception {
		if (args[0].equals("write")) {
			Utils.random.setSeed(Long.parseLong(args[2]));
			int n = Integer.parseInt(args[3]);
			for (int i = 0; i < n; i++)
				write(args[1], "java_" + i, new GeneticCode());
			return;
		}
		if (args[0].equals("mutate")) {
			Utils.random.setSeed(Long.parseLong(args[2]));
			Utils.META_MUTATION_RATE = 10000;
			Utils.MIN_MUTATION_RATE = 5000;
			Utils.MAX_MUTATION_RATE = 10000;
			int n = Integer.parseInt(args[3]), gens = Integer.parseInt(args[4]);
			for (int i = 0; i < n; i++) {
				GeneticCode g = new GeneticCode();
				for (int k = 0; k < gens; k++)
					g = new GeneticCode(g);
				write(args[1], "mutated_" + i, g);
			}
			return;
		}
		if (args[0].equals("evolve")) {
			long seed = Long.parseLong(args[2]);
			Utils.random.setSeed(seed);
			World w = new World(new VisibleWorld());
			w.genesis();
			for (long f = Long.parseLong(args[3]); f > 0; f--)
				w.time();
			int k = 0;
			for (Organism o : w._organisms)
				if (o.isAlive() && k++ % 25 == 0)
					write(args[1], "evolved_" + seed + "_" + o._ID, o.getGeneticCode());
			return;
		}
		if (args[0].equals("special")) {
			double[] v = {0.0, -0.0, 4.9E-324, 2.2250738585072014E-308, 1.0E-300, 1.0E-5, 1.0E-3,
					0.1, 1.0 / 3, Math.PI, 1.0E7, 1.0E21, 1.7976931348623157E308, -2.5,
					Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY, Double.NaN};
			Color[] cs = {Color.GREEN, Utils.ColorFOREST, Color.RED, Utils.ColorEYE, Utils.ColorDARK,
					Color.LIGHT_GRAY, Utils.ColorSPIKE, Color.DARK_GRAY, Utils.ColorPLAGUE};
			int[] branches = {-1, 0, 1, 999, -7, 2147483647, -2147483648};
			for (int i = 0; i < v.length; i++) {
				List<Gene> genes = new ArrayList<Gene>();
				for (int j = 0; j < v.length; j++)
					genes.add(gene(v[i], v[j], cs[(i + j) % cs.length], branches[j % branches.length],
							(i * j) % 6 - (j == 3 ? 9 : 0)));
				write(args[1], "special_" + i, new GeneticCode(genes, 1 + i % 8, i % 2, i * 600 % 10001,
						i % 3, 1 + i % 3, 1 + i % 4, 1 + i % 12, 1 + i % 70, i % 2 == 0, i % 3 == 0,
						i % 4 == 0, i % 5 == 0, i % 6 == 0, i % 7 == 0, i % 2 == 1, i % 3 == 1, i % 4 == 1,
						i % 5 == 1, i % 6 == 1, i % 7 == 1, i % 3 == 2, i % 4 == 2, i % 5 == 2));
			}
			return;
		}
		BioXMLParser parser = new BioXMLParser();
		for (int k = 1; k < args.length; k++) {
			GeneticCode g;
			try {
				g = parser.parseGeneticCode(new File(args[k]));
			} catch (Exception e) {
				System.out.println("reject");
				continue;
			}
			System.out.println(canonical(g));
		}
	}
}
