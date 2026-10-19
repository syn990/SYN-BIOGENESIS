#!/usr/bin/env python3
# SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
# Copyright (C) 2026 syn990
# SPDX-License-Identifier: GPL-2.0-or-later (see LICENSE)

"""Makes the Windows icon from the original's 30x30 icon: centred on a clear
32x32 ground, and that halved to 16x16, with nothing but the standard
library.

    tools/win/icon.py assets/icons/biogenesis.png src/win/syn-biogenesis.ico
"""
import struct
import sys
import zlib


def read_png(path):
    """8-bit RGB or RGBA, not interlaced: rows of (r, g, b, a)"""
    data = open(path, "rb").read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        sys.exit(f"{path}: not a PNG")
    pos, idat = 8, b""
    while pos < len(data):
        n, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + n]
        if kind == b"IHDR":
            w, h, depth, ctype, _, _, interlace = struct.unpack(">IIBBBBB", body)
        elif kind == b"IDAT":
            idat += body
        pos += 12 + n
    if depth != 8 or ctype not in (2, 6) or interlace:
        sys.exit(f"{path}: only 8-bit RGB or RGBA, not interlaced")
    bpp = 3 if ctype == 2 else 4
    raw, stride = zlib.decompress(idat), w * bpp
    rows, prev = [], bytes(stride)
    for y in range(h):
        f, line = raw[y * (stride + 1)], bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - bpp] if i >= bpp else 0
            b = prev[i]
            c = prev[i - bpp] if i >= bpp else 0
            if f == 1:
                line[i] = (line[i] + a) & 255
            elif f == 2:
                line[i] = (line[i] + b) & 255
            elif f == 3:
                line[i] = (line[i] + (a + b) // 2) & 255
            elif f == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        prev = bytes(line)
        rows.append([tuple(line[x * bpp:x * bpp + 3]) + ((line[x * bpp + 3],) if bpp == 4 else (255,))
                     for x in range(w)])
    return rows


def centred(rows, size):
    h, w = len(rows), len(rows[0])
    top, left = (size - h) // 2, (size - w) // 2
    out = [[(0, 0, 0, 0)] * size for _ in range(size)]
    for y in range(h):
        for x in range(w):
            out[top + y][left + x] = rows[y][x]
    return out


def halved(rows):
    """each 2x2 block to one pixel, its colour weighted by alpha"""
    out = []
    for y in range(0, len(rows), 2):
        line = []
        for x in range(0, len(rows[0]), 2):
            px = [rows[y + j][x + i] for j in (0, 1) for i in (0, 1)]
            a = sum(p[3] for p in px)
            rgb = tuple((sum(p[k] * p[3] for p in px) + a // 2) // a if a else 0 for k in range(3))
            line.append(rgb + ((a + 2) // 4,))
        out.append(line)
    return out


def dib(rows):
    """a 32-bit icon image: BITMAPINFOHEADER, BGRA rows bottom up, AND mask"""
    size = len(rows)
    head = struct.pack("<IiiHHIIiiII", 40, size, 2 * size, 1, 32, 0, 0, 0, 0, 0, 0)
    pixels = b"".join(bytes((p[2], p[1], p[0], p[3])) for row in reversed(rows) for p in row)
    mask = bytes(((size + 31) // 32) * 4 * size)
    return head + pixels + mask


def main():
    big = centred(read_png(sys.argv[1]), 32)
    images = [(16, dib(halved(big))), (32, dib(big))]
    out = struct.pack("<HHH", 0, 1, len(images))
    offset = 6 + 16 * len(images)
    for size, img in images:
        out += struct.pack("<BBBBHHII", size, size, 0, 0, 1, 32, len(img), offset)
        offset += len(img)
    open(sys.argv[2], "wb").write(out + b"".join(img for _, img in images))


if __name__ == "__main__":
    main()
