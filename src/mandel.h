/***************************************************************
 * mandel.h: the Mandelbrot iteration, in 32-bit fixed point.
 *
 * The same code builds for NICE, where each product is one of the
 * math unit (nice_cpu's SYSTEM.md §2), and for the host, where it is
 * a 64-bit product in C. Both give the same counts.
 *
 * Copyright (c) 2026 Michael Jørgensen. MIT license (see LICENSE).
 ***************************************************************/

#ifndef MANDEL_H
#define MANDEL_H

#include <stdint.h>

// The numbers have FRAC fraction bits, so they run from -64 to 64.
// Before an escape |z| <= 2, so after one more iteration |z| <= 4 + |c|.
// For |c| <= 2 that is 6, and x*x + y*y stays below 64. A c with
// 2 < |c| < 8 escapes at the first test, where z = c and
// x*x + y*y = |c|^2 < 64. So any c with |c| < 8 is safe; the view of
// stage 2 (view.c) keeps |re c| and |im c| at most 4.
#define FRAC 25
#define FIX(num, den) ((int32_t) ((int64_t) (num) * (1 << FRAC) / (den)))
#define FOUR FIX(4, 1)

// The highest count of stage 1, and of stage 2 at the start. A point
// that reaches the highest count is in the set.
#define MAX_ITER 256

// The view: 80 points across, the real part from -2.0 to 1.0, each
// point at the center of its pixel. The imaginary axis is in the middle
// of the rows. In stage 1, where a point is a whole cell, 16x32 pixels,
// the imaginary step is twice the real one. Stage 2 starts with this
// view, with square points (view.h).
#define COLS 80
#define RE_MIN FIX(-2, 1)
#define RE_STEP FIX(3, COLS)

// The number of iterations of z -> z*z + c from z = 0 before |z| > 2,
// or max, for c = a + ib.
int mandel_count(int32_t a, int32_t b, int max);

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

// The color of a count, of a highest count max, as 12 bits: red,
// green, blue.
uint32_t mandel_color(int count, int max);

// A screen word: the background in bits 31..20, the foreground in bits
// 19..8, and the character in bits 7..0 (nice_cpu's SYSTEM.md §1.1).
static inline uint32_t mandel_word(uint32_t bg, uint32_t fg, unsigned ch)
{
    return bg << 20 | fg << 8 | ch;
}

// Stage 1: one point per cell, a blank in the point's color.
#define ROWS1 25
#define STEP1 (2 * RE_STEP)
static inline uint32_t mandel_cell1(int count)
{
    uint32_t color = mandel_color(count, MAX_ITER);
    return mandel_word(color, color, ' ');
}

// Stage 2: two points per cell, the upper half block of GLYPH_HALF in
// the top point's color, on the bottom point's color. Its points are
// square, and view.h places them.
#define ROWS2 50
#define GLYPH_HALF 0x80
static inline uint32_t mandel_cell2(int top, int bottom, int max)
{
    return mandel_word(mandel_color(bottom, max), mandel_color(top, max),
                       GLYPH_HALF);
}

// Stage 3: eight points per cell, two across and four down, each
// 8x8 pixels, square, with the glyphs of the octants at codes
// GLYPH_OCTANT to 0xFF. view.h places the points.
//
// Point i of a cell, i = 2 * row + column, row-major from the top
// left, is bit 7 - i of the cell's pattern, and a set bit is drawn in
// the foreground color. A cell has only two colors: the most frequent
// color of its points, and the most frequent of the others, each the
// first in the order of the points when two are as frequent. Each
// point of a third color is drawn in the one of the two that is
// nearer in RGB, the first of the two when they are as near. The
// pattern and its inverse, with the colors swapped, look the same,
// so the top left point is always in the foreground: bit 7 is set,
// and the code is the pattern, 0x80 to 0xFF. Codes 0x00 to 0x7F keep
// the character ROM's ASCII, for the label.
#define COLS3 160
#define ROWS3 100
#define GLYPH_OCTANT 0x80

// The screen word of a cell whose points have the counts count[0..7],
// of a highest count max.
uint32_t mandel_cell3(const int count[8], int max);

// Row r, 0 to 31, of the glyph of code, GLYPH_OCTANT to 0xFF: its two
// halves, 8 pixels each, set where the code's points in that row are.
static inline uint32_t mandel_glyph3(unsigned code, int r)
{
    unsigned bits = code >> (6 - 2 * (r / 8)) & 3;

    return (bits & 2 ? 0xFF00u : 0) | (bits & 1 ? 0x00FFu : 0);
}

// Stage 4: the chunky mode of the screen (nice_cpu's DEVICES.md
// §1.10), one screen word a point, 160x100, each point 8x8 pixels,
// square, in its own color, so the picture is exact everywhere. The
// color is the word's bits 31..20, as the background of a cell, and
// the other bits are ignored; they are 0 here. view.h places the points,
// as in stage 3.
#define COLS4 160
#define ROWS4 100
static inline uint32_t mandel_pixel4(uint32_t color)
{
    return color << 20;
}

#endif
