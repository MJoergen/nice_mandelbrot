/***************************************************************
 * mandel.h: the Mandelbrot iteration, in 32-bit fixed point.
 *
 * The same code builds for NICE, where each product is one of the
 * math unit (nice_cpu's DEVICES.md §2), and for the host, where it is
 * a 64-bit product in C. Both give the same counts.
 *
 * Copyright (c) 2026 Michael Jørgensen. MIT license (see LICENSE).
 ***************************************************************/

#ifndef MANDEL_H
#define MANDEL_H

#include <stdint.h>

// The numbers have FRAC fraction bits, so they run from -64 to 64.
// Before an escape |z| <= 2, so after one more iteration |z| <= 4 + |c|,
// and x*x + y*y stays below 64 for any c of the view.
#define FRAC 25
#define FIX(num, den) ((int32_t) ((int64_t) (num) * (1 << FRAC) / (den)))
#define FOUR FIX(4, 1)

// The highest count. A point that reaches it is in the set.
#define MAX_ITER 256

// The view: 80 points across, the real part from -2.0 to 1.0, each
// point at the center of its pixel. The imaginary axis is in the middle
// of the rows, and its step is chosen by the stage: twice the real one
// in stage 1, where a point is a whole cell, 16x32 pixels, and the same
// in stage 2, where a point is half a cell, 16x16.
#define COLS 80
#define RE_MIN FIX(-2, 1)
#define RE_STEP FIX(3, COLS)

// The number of iterations of z -> z*z + c from z = 0 before |z| > 2,
// or MAX_ITER, for c = a + ib.
int mandel_count(int32_t a, int32_t b);

// The real part of the points of column col.
static inline int32_t mandel_re(int col)
{
    return RE_MIN + col * RE_STEP + RE_STEP / 2;
}

// The imaginary part of the points of row row, of rows rows at steps
// of step, row 0 at the top.
static inline int32_t mandel_im(int row, int rows, int32_t step)
{
    return (rows - 1 - 2 * row) * step / 2;
}

// The color of a count, as 12 bits: red, green, blue.
uint32_t mandel_color(int count);

// A screen word: the background in bits 31..20, the foreground in bits
// 19..8, and the character in bits 7..0 (nice_cpu's DEVICES.md §1.1).
static inline uint32_t mandel_word(uint32_t bg, uint32_t fg, unsigned ch)
{
    return bg << 20 | fg << 8 | ch;
}

// Stage 1: one point per cell, a blank in the point's color.
#define ROWS1 25
#define STEP1 (2 * RE_STEP)
static inline uint32_t mandel_cell1(int count)
{
    uint32_t color = mandel_color(count);
    return mandel_word(color, color, ' ');
}

// Stage 2: two points per cell, the upper half block of GLYPH_HALF in
// the top point's color, on the bottom point's color.
#define ROWS2 50
#define STEP2 RE_STEP
#define GLYPH_HALF 0x80
static inline uint32_t mandel_cell2(int top, int bottom)
{
    return mandel_word(mandel_color(bottom), mandel_color(top), GLYPH_HALF);
}

#endif
