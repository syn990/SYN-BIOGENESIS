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

/* .bgg checks with the original BioXMLParser.
 *
 *   BggInterop write DIR SEED N         N random codes from the Java GeneticCode
 *   BggInterop mutate DIR SEED N GENS   N lineages mutated GENS generations at
 *                                       a 30% mutation rate (gene counts up to 100)
 *   BggInterop evolve DIR SEED FRAMES   every living organism's code after
 *                                       FRAMES frames of a seeded world
 *   BggInterop special DIR              genes with extreme lengths and angles
 *   BggInterop read FILE...             parse (validating DTD) and print each
 *                                       canonically, or "reject"
 */
package biogenesis;

import java.awt.Color;
import java.io.File;
import java.io.PrintStream;

public class BggInterop {
	static String color(Color c) {
		if (c.equals(Color.RED)) return "red";
		if (c.equals(Color.GREEN)) return "green";
		if (c.equals(Color.BLUE)) return "blue";
		if (c.equals(Color.CYAN)) return "cyan";
		if (c.equals(Color.WHITE)) return "white";
		if (c.equals(Color.GRAY)) return "gray";
		if (c.equals(Color.YELLOW)) return "yellow";
		return "?";
	}

	static void write(String dir, String name, GeneticCode g) throws Exception {
		try (PrintStream ps = new PrintStream(new File(dir, name + ".bgg"))) {
			BioXMLParser.writeGeneticCode(ps, g);
		}
	}

	public static void main(String[] args) throws Exception {
		if (args[0].equals("mutate")) {
			Utils.random.setSeed(Long.parseLong(args[2]));
			Utils.MUTATION_RATE = 0.3;
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
			for (Organism o : w._organisms)
				if (o.isAlive())
					write(args[1], "evolved_" + seed + "_" + o._ID, o.getGeneticCode());
			return;
		}
		if (args[0].equals("special")) {
			double[] v = {0.0, -0.0, 4.9E-324, 2.2250738585072014E-308, 1.0E-300, 1.0E-5, 1.0E-3,
					0.1, 1.0 / 3, Math.PI, 1.0E7, 1.0E21, 1.7976931348623157E308, -2.5,
					Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY, Double.NaN};
			Color[] cs = {Color.RED, Color.GREEN, Color.BLUE, Color.CYAN, Color.WHITE, Color.GRAY, Color.YELLOW};
			for (int i = 0; i < v.length; i++) {
				java.util.List<Gene> genes = new java.util.ArrayList<Gene>();
				for (int j = 0; j < v.length; j++)
					genes.add(new Gene(v[i], v[j], cs[(i + j) % cs.length]));
				write(args[1], "special_" + i, new GeneticCode(genes, 1 + i % 8, i % 2, i % 3 == 0));
			}
			// mutation stops at 100 genes; a file can hold any number
			java.util.Random r = new java.util.Random(1);
			for (int n : new int[] {100, 500}) {
				java.util.List<Gene> genes = new java.util.ArrayList<Gene>();
				for (int j = 0; j < n; j++)
					genes.add(new Gene(2 + 16 * r.nextDouble(), 2 * Math.PI * r.nextDouble(), cs[j % cs.length]));
				write(args[1], "special_genes_" + n, new GeneticCode(genes, 1 + n % 8, 1, false));
			}
			return;
		}
		if (args[0].equals("write")) {
			Utils.random.setSeed(Long.parseLong(args[2]));
			int n = Integer.parseInt(args[3]);
			for (int i = 0; i < n; i++) {
				try (PrintStream ps = new PrintStream(new File(args[1], "java_" + i + ".bgg"))) {
					BioXMLParser.writeGeneticCode(ps, new GeneticCode());
				}
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
			StringBuilder sb = new StringBuilder();
			sb.append(g.getSymmetry()).append(' ').append(g.getMirror()).append(' ')
				.append(g.getDisperseChildren() ? 1 : 0).append(' ').append(g.getNGenes());
			for (int i = 0; i < g.getNGenes(); i++) {
				Gene ge = g.getGene(i);
				sb.append(String.format(" %016x %016x %s", Double.doubleToRawLongBits(ge.getLength()),
						Double.doubleToRawLongBits(ge.getTheta()), color(ge.getColor())));
			}
			System.out.println(sb);
		}
	}
}
