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

/* .bgg interchange check with the original BioXMLParser.
 *
 *   BggInterop write DIR SEED N   N random codes from the Java GeneticCode
 *   BggInterop read FILE...       parse (validating DTD), print canonically
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

	public static void main(String[] args) throws Exception {
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
			GeneticCode g = parser.parseGeneticCode(new File(args[k]));
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
