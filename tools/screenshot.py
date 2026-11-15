#!/usr/bin/env python3
"""Draw a stage 2 screen, as niceemu dumps it, as a PNG of the MEGA65's screen.

    screenshot.py WORDS PNG

WORDS has the screen words, one per line in hex, the first 2000 of
which are the 80x25 cells. Each cell is 16x32 pixels: the upper half
block of stage 2, its top half in the foreground color (bits 19..8),
its bottom half in the background color (bits 31..20). The colors are
12 bits, red, green, blue.

Copyright (c) 2026 Michael Jørgensen. MIT license (see LICENSE).
"""

import sys

from PIL import Image

COLS, ROWS, CELL_W, CELL_H = 80, 25, 16, 32


def rgb(color):
    return tuple(((color >> shift) & 0xF) * 17 for shift in (8, 4, 0))


def main():
    if len(sys.argv) != 3:
        sys.exit(f"usage: {sys.argv[0]} WORDS PNG")
    with open(sys.argv[1]) as f:
        words = [int(w, 16) for w in f.read().split()[:COLS * ROWS]]
    img = Image.new("RGB", (COLS * CELL_W, ROWS * CELL_H))
    for i, word in enumerate(words):
        x, y = i % COLS * CELL_W, i // COLS * CELL_H
        img.paste(rgb(word >> 8 & 0xFFF), (x, y, x + CELL_W, y + CELL_H // 2))
        img.paste(rgb(word >> 20), (x, y + CELL_H // 2, x + CELL_W, y + CELL_H))
    img.save(sys.argv[2], optimize=True)


if __name__ == "__main__":
    main()
