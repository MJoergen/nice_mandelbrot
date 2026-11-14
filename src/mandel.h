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

// The view of stage 1: 80x25 points, the real part from -2.0 to 1.0,
// and the imaginary step twice the real one, as the cells are twice as
// tall as they are wide. Each point is at the center of its cell, and
// row 12 is the real axis.
#define COLS 80
#define ROWS 25
#define RE_MIN FIX(-2, 1)
#define RE_STEP FIX(3, COLS)
#define IM_STEP (2 * RE_STEP)

// The number of iterations of z -> z*z + c from z = 0 before |z| > 2,
// or MAX_ITER, for c = a + ib.
int mandel_count(int32_t a, int32_t b);

// The point of the cell at column col and row row.
static inline int32_t mandel_re(int col)
{
    return RE_MIN + col * RE_STEP + RE_STEP / 2;
}

static inline int32_t mandel_im(int row)
{
    return (ROWS / 2 - row) * IM_STEP;
}

// The color of a count, as 12 bits: red, green, blue.
uint32_t mandel_color(int count);

#endif
