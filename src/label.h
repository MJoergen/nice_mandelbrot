/***************************************************************
 * label.h: the label in the top right corner of stage 2, the name
 * and the version, such as "nice_mandelbrot V1.0".
 *
 * The label is drawn in the character ROM's glyphs, which stage 2
 * copies into the font RAM, in white on black, after a blank. It
 * covers the cells at the end of the top row, whose points are still
 * computed, so the iterations are those of the whole picture.
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

#endif
