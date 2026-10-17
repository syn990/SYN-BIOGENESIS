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

/* Prints java.awt geometry results for random inputs drawn from
 * java.util.Random(seed); tests/geom_check.c draws the same inputs with
 * its java.util.Random port and prints its own results for comparison. */
import java.awt.Rectangle;
import java.awt.geom.Line2D;
import java.util.Random;

public class GeomCheck {
	public static void main(String[] args) {
		Random r = new Random(Long.parseLong(args[0]));
		int n = Integer.parseInt(args[1]);
		StringBuilder sb = new StringBuilder();
		for (int i = 0; i < n; i++) {
			Rectangle a = new Rectangle(r.nextInt(41) - 20, r.nextInt(41) - 20, r.nextInt(23) - 2, r.nextInt(23) - 2);
			Rectangle b = new Rectangle(r.nextInt(41) - 20, r.nextInt(41) - 20, r.nextInt(23) - 2, r.nextInt(23) - 2);
			int px = r.nextInt(41) - 20, py = r.nextInt(41) - 20;
			double[] c = new double[8];
			for (int k = 0; k < 8; k++)
				c[k] = r.nextBoolean() ? r.nextInt(61) - 30 : r.nextDouble() * 60 - 30;
			Line2D.Double l1 = new Line2D.Double(c[0], c[1], c[2], c[3]);
			Line2D.Double l2 = new Line2D.Double(c[4], c[5], c[6], c[7]);
			double d = r.nextDouble() * 20 - 10;
			sb.append(a.intersects(b) ? 1 : 0).append(a.contains(px, py) ? 1 : 0)
				.append(a.intersectsLine(l1) ? 1 : 0).append(l1.intersectsLine(l2) ? 1 : 0)
				.append(' ').append((int) Math.round(d)).append(' ')
				.append((int) Math.round(Math.floor(d) + 0.5)).append('\n');
		}
		System.out.print(sb);
	}
}
