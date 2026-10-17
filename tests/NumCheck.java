/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 * SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)
 */

/* Random number strings, valid and not, with what Double.parseDouble makes
 * of each (the raw bits, or "reject"), for tests/num_check: long digit
 * strings, hexadecimal floats, exponents, signs, suffixes, spaces, junk.
 *
 *   java NumCheck N STRINGS RESULTS
 */
import java.io.*;
import java.util.Random;
public class NumCheck {
	static Random r = new Random(2026);
	static String pick(String... a) { return a[r.nextInt(a.length)]; }
	static String digits(int max, boolean hex) {
		StringBuilder b = new StringBuilder(); int n = r.nextInt(max + 1);
		for (int i = 0; i < n; i++) b.append((hex ? "0123456789abcdefABCDEF" : "0123456789").charAt(r.nextInt(hex ? 22 : 10)));
		return b.toString();
	}
	static String gen() {
		StringBuilder b = new StringBuilder();
		b.append(pick("", "", "", " ", "\t", "  "));
		b.append(pick("", "", "", "+", "-"));
		int k = r.nextInt(20);
		if (k == 0) b.append(pick("NaN", "Infinity", "nan", "inf", "INF", "Inf", "NaNd", "Infinityf", "-NaN", "infinity"));
		else if (k < 4) {
			b.append(pick("0x", "0X", "0x", "x", "00x"));
			b.append(digits(r.nextInt(3) == 0 ? 30 : 6, true));
			if (r.nextBoolean()) { b.append('.'); b.append(digits(r.nextInt(3) == 0 ? 30 : 6, true)); }
			if (r.nextInt(5) > 0) { b.append(pick("p", "P", "e")); b.append(pick("", "+", "-")); b.append(digits(4, false)); }
		} else {
			int big = r.nextInt(4);
			b.append(digits(big == 0 ? 400 : big == 1 ? 25 : 6, false));
			if (r.nextInt(3) > 0) { b.append('.'); b.append(digits(big == 0 ? 400 : big == 1 ? 25 : 6, false)); }
			if (r.nextInt(3) == 0) { b.append(pick("e", "E")); b.append(pick("", "+", "-")); b.append(r.nextInt(8) == 0 ? digits(0, false) : String.valueOf(r.nextInt(r.nextBoolean() ? 30 : 400))); }
		}
		if (r.nextInt(6) == 0) b.append(pick("d", "D", "f", "F", "x", "dd", " d", "_"));
		b.append(pick("", "", "", " ", "\t"));
		if (r.nextInt(40) == 0) b.insert(r.nextInt(b.length() + 1), pick("_", ".", "e", "-", " ", "1"));
		return b.toString();
	}
	public static void main(String[] a) throws Exception {
		int n = Integer.parseInt(a[0]);
		try (PrintWriter in = new PrintWriter(new FileWriter(a[1])); PrintWriter out = new PrintWriter(new FileWriter(a[2]))) {
			for (int i = 0; i < n; i++) {
				String s = gen();
				in.println(s);
				try { out.println(String.format("%016x", Double.doubleToRawLongBits(Double.parseDouble(s)))); }
				catch (NumberFormatException e) { out.println("reject"); }
			}
		}
	}
}
