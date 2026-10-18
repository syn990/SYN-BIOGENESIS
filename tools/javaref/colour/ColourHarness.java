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

/* Runs the shipped Color Mod 2.01 world without a window and dumps its
 * state each frame in the same format as
 * `syn-biogenesis --colour --headless --dump`.
 *
 *   java -cp <classes>:biogenesis.jar:jts.jar biogenesis.ColourHarness SEED FRAMES OUT [EVERY]
 *
 * Uses Utils' defaults, as a first start of Color Mod does (and as its
 * README asks).
 */
package biogenesis;

import java.io.FileWriter;
import java.io.PrintWriter;

public class ColourHarness {
	static String b(double d) {
		return String.format("%016x", Double.doubleToRawLongBits(d));
	}

	static void dump(PrintWriter pw, World w, long frame) {
		pw.printf("F %d pop=%d o2=%s co2=%s ch4=%s next=%d n=%d\n", frame, w._population,
				b(w._O2), b(w._CO2), b(w._CH4), w.NEXT_ID, w._organisms.size());
		for (Organism o : w._organisms) {
			pw.printf("O %d %d e=%s c=%s,%s t=%s v=%s,%s,%s g=%d r=%d,%d,%d,%d a=%d k=%d\n",
					o._ID, o.alive ? 1 : 0, b(o._energy), b(o._dCenterX), b(o._dCenterY),
					b(o._theta), b(o.dx), b(o.dy), b(o.dtheta), o._growthRatio,
					o.x, o.y, o.width, o.height, o._age, o._nTotalKills);
		}
	}

	public static void main(String[] args) throws Exception {
		long seed = Long.parseLong(args[0]);
		long frames = Long.parseLong(args[1]);
		long every = args.length > 3 ? Long.parseLong(args[3]) : 1;
		Utils.random.setSeed(seed);
		World w = new World(new VisibleWorld());
		w.genesis();
		try (PrintWriter pw = new PrintWriter(new FileWriter(args[2]))) {
			dump(pw, w, 0);
			for (long f = 1; f <= frames; f++) {
				w.time();
				if (every <= 1 || f % every == 0)
					dump(pw, w, f);
			}
		}
	}
}
