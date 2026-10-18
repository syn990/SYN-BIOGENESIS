/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 * SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)
 */

/* The original's world files, for tests/run.sh: a seeded world (or one
 * loaded from a .bgw, as MainWindow's Open does) runs FRAMES frames,
 * dumping its state like Harness, and is then saved as MainWindow's Save
 * does. syn-biogenesis --headless with --load and --save does the same.
 *
 *   java ... biogenesis.BgwRun SEED FRAMES DUMP|- [load FILE] [save FILE]
 */
package biogenesis;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.PrintWriter;

public class BgwRun {
	public static void main(String[] args) throws Exception {
		long seed = Long.parseLong(args[0]);
		long frames = Long.parseLong(args[1]);
		String load = null, save = null;
		for (int i = 3; i + 1 < args.length; i += 2) {
			if (args[i].equals("load"))
				load = args[i + 1];
			else if (args[i].equals("save"))
				save = args[i + 1];
		}
		World w;
		if (load != null) {
			try (ObjectInputStream in = new ObjectInputStream(new FileInputStream(load))) {
				w = (World) in.readObject();
			}
			w.init(new VisibleWorld());
			Utils.random.setSeed(seed);
		} else {
			Utils.random.setSeed(seed);
			w = new World(new VisibleWorld());
			w.genesis();
		}
		PrintWriter pw = args[2].equals("-") ? null : new PrintWriter(new FileWriter(args[2]));
		if (pw != null)
			Harness.dump(pw, w, 0);
		for (long f = 1; f <= frames; f++) {
			w.time();
			if (pw != null)
				Harness.dump(pw, w, f);
		}
		if (pw != null)
			pw.close();
		if (save != null)
			try (ObjectOutputStream out = new ObjectOutputStream(new FileOutputStream(save))) {
				out.writeObject(w);
			}
	}
}
