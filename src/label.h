/***************************************************************
 * label.h: the label in the top right corner of stages 2 to 4, the
 * name and the version, such as "nice_mandelbrot V1.0".
 *
 * In stages 2 and 3 the label is drawn in the character ROM's glyphs,
 * which they copy into the font RAM, in white on black, after a blank.
 * Stage 4 shows no font, so there it is drawn in its pixels, in
 * capitals of 3x5 pixels, white on black: NICE_MANDELBROT V1.0. It
 * covers the cells or the pixels at the end of the top row, whose
 * points are still computed, so the iterations are those of the whole
 * picture.
 *
 * The same code runs on NICE and on the host, so that the host's
 * screen words have the label too.
 *
 * Copyright (c) 2026 Michael Jørgensen. MIT license (see LICENSE).
 ***************************************************************/

#ifndef LABEL_H
#define LABEL_H

#include <stdint.h>

// The version, from git describe or the release tag: build/gen/version.c,
// which the Makefile writes.
extern const char nice_mandelbrot_version[];

// The character of the label at cell (row, col) of the 80x25 screen, or
// 0 where the label is not.
char label_char(int row, int col);

// The screen word of cell (row, col): the label's where it is, and word
// elsewhere.
uint32_t label_cell(int row, int col, uint32_t word);

// Stage 4: whether the pixel (x, y) of the 160x100 of the chunky mode
// is in the label, and if so, whether it is set, white, rather than
// clear, black. LABEL_NONE where the label is not.
#define LABEL_NONE  (-1)
#define LABEL_ROWS4 7
int label_bit(int x, int y);

// The pixel word of (x, y): the label's where it is, and word elsewhere.
uint32_t label_pixel(int x, int y, uint32_t word);

#endif
