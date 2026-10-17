/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 * SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)
 */

/* Writes N random inputs and what this JVM's Math returns for them, for
 * tests/math_check to compare with the port's maths (src/sim/jmath.h).
 * Records of seven little-endian doubles: t, x, y, sin(t), cos(t),
 * atan2(x, y), pow(x, y). Half the inputs are in the ranges the simulation
 * uses; the rest are arbitrary bit patterns (huge, tiny, NaN, infinite).
 *
 *   java MathCheck N OUT SEED
 */
import java.io.BufferedOutputStream;
import java.io.FileOutputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Random;

public class MathCheck {
	public static void main(String[] args) throws Exception {
		int n = Integer.parseInt(args[0]);
		Random r = new Random(Long.parseLong(args[2]));
		double[] exps = {2, 3, 0.5, -1, 1, 0, 1.5, -0.5};
		ByteBuffer bb = ByteBuffer.allocate(56).order(ByteOrder.LITTLE_ENDIAN);
		try (OutputStream os = new BufferedOutputStream(new FileOutputStream(args[1]), 1 << 20)) {
			for (int i = 0; i < n; i++) {
				double t, x, y;
				if (i % 2 == 0) {
					t = Double.longBitsToDouble(r.nextLong());
					x = Double.longBitsToDouble(r.nextLong());
					y = Double.longBitsToDouble(r.nextLong());
				} else {
					t = (r.nextDouble() * 2 - 1) * (i % 3 == 0 ? 1e6 : 20);
					x = Math.rint((r.nextDouble() * 2 - 1) * 200);
					y = i % 5 == 0 ? (r.nextDouble() * 2 - 1) * 20 : exps[r.nextInt(exps.length)];
				}
				bb.clear();
				bb.putDouble(t).putDouble(x).putDouble(y).putDouble(Math.sin(t)).putDouble(Math.cos(t))
						.putDouble(Math.atan2(x, y)).putDouble(Math.pow(x, y));
				os.write(bb.array());
			}
		}
	}
}
