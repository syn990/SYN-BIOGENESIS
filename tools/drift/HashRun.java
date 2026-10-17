/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 * SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)
 */

/* Runs the shipped Biogenesis 0.9 world without a window and writes, every
 * frame, the frame number, population, organism count and a hash of the
 * state tools/javaref/Harness.java dumps. `syn-biogenesis --headless --hash`
 * writes the same, so two runs can be compared over many frames and seeds
 * without full dumps.
 *
 *   java -cp <classes>:biogenesis.jar:jts.jar biogenesis.HashRun SEED FRAMES OUT
 */
package biogenesis;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.PrintWriter;

public class HashRun {
	private long h;

	/* FNV-1a over a little-endian 64-bit word */
	private void word(long v) {
		for (int i = 0; i < 8; i++) {
			h ^= (v >>> (8 * i)) & 0xff;
			h *= 0x100000001b3L;
		}
	}

	private void bits(double d) {
		word(Double.doubleToRawLongBits(d));
	}

	private String frame(World w, long f) {
		h = 0xcbf29ce484222325L;
		word(w._population);
		bits(w._O2);
		bits(w._CO2);
		word(w.NEXT_ID);
		word(w._organisms.size());
		for (Organism o : w._organisms) {
			word(o._ID);
			word(o.alive ? 1 : 0);
			bits(o._energy);
			bits(o._dCenterX);
			bits(o._dCenterY);
			bits(o._theta);
			bits(o.dx);
			bits(o.dy);
			bits(o.dtheta);
			word(o._growthRatio);
			word(o.x);
			word(o.y);
			word(o.width);
			word(o.height);
			word(o._age);
			word(o._nTotalKills);
		}
		return f + " " + w._population + " " + w._organisms.size() + " " + String.format("%016x", h);
	}

	public static void main(String[] args) throws Exception {
		long seed = Long.parseLong(args[0]);
		long frames = Long.parseLong(args[1]);
		Utils.random.setSeed(seed);
		World w = new World(new VisibleWorld());
		w.genesis();
		HashRun r = new HashRun();
		try (PrintWriter pw = new PrintWriter(new BufferedWriter(new FileWriter(args[2])))) {
			pw.println(r.frame(w, 0));
			for (long f = 1; f <= frames; f++) {
				w.time();
				pw.println(r.frame(w, f));
			}
		}
	}
}
