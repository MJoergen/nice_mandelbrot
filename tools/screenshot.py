#!/usr/bin/env python3
"""Draw a screen, as niceemu dumps it, as a PNG of the MEGA65's screen.

    screenshot.py DUMP PNG

DUMP has the words of niceemu -d 0xFFFF0000:16000 -d 0xFFFE0000:8192
-d 0xFFFFFF87:1, the screen, the font RAM, and the mode word
(tools/words.py reads them). In the text, mode 0, each 80x25 cell is 16x32 pixels, its glyph's set pixels in the foreground color
(bits 19..8), and its clear ones in the background color (bits
31..20). The colors are 12 bits, red, green, blue. A row of a glyph
is a word, the leftmost pixel in bit 15 (nice_cpu's SYSTEM.md §1.8).
In the chunky mode, mode 1, each of the 160x100 words is a square of
8x8 pixels in its bits 31..20 (nice_cpu's DEVICES.md §1.10).

Copyright (c) 2026 Michael Jørgensen. MIT license (see LICENSE).
"""

import sys

from PIL import Image

from words import read_dump

COLS, ROWS, CELL_W, CELL_H = 80, 25, 16, 32
SCREEN, FONT_RAM, MODE = 0xFFFF0000, 0xFFFE0000, 0xFFFFFF87
PIXEL_COLS, PIXEL_ROWS, PIXEL = 160, 100, 8


def rgb(color):
    return tuple(((color >> shift) & 0xF) * 17 for shift in (8, 4, 0))


def main():
    if len(sys.argv) != 3:
        sys.exit(f"usage: {sys.argv[0]} DUMP PNG")
    words = read_dump(sys.argv[1])
    img = Image.new("RGB", (COLS * CELL_W, ROWS * CELL_H))
    px = img.load()
    if words.get(MODE, 0) == 1:
        for i in range(PIXEL_COLS * PIXEL_ROWS):
            color = rgb(words[SCREEN + i] >> 20)
            x0, y0 = i % PIXEL_COLS * PIXEL, i // PIXEL_COLS * PIXEL
            for r in range(PIXEL):
                for c in range(PIXEL):
                    px[x0 + c, y0 + r] = color
        img.save(sys.argv[2], optimize=True)
        return
    for i in range(COLS * ROWS):
        word = words[SCREEN + i]
        fg, bg = rgb(word >> 8 & 0xFFF), rgb(word >> 20)
        x0, y0 = i % COLS * CELL_W, i // COLS * CELL_H
        for r in range(CELL_H):
            bits = words[FONT_RAM + 32 * (word & 0xFF) + r]
            for c in range(CELL_W):
                px[x0 + c, y0 + r] = fg if bits >> (15 - c) & 1 else bg
    img.save(sys.argv[2], optimize=True)


if __name__ == "__main__":
    main()
