/***************************************************************
 * view.h: the part of the complex plane that stage 2 shows, and the
 * keys that move it.
 *
 * The view is COLS x ROWS2 points, square, step apart. Its left edge
 * is re0, and the imaginary axis of its rows is at im0, in the middle.
 * It starts as the whole set, the real part from -2.0 to 1.0, the same
 * points as stage 2 had with fixed constants.
 *
 *     Left, Right, Up, Down   pan by VIEW_PAN points
 *     + or =                  zoom in, twice, about the middle
 *     - or _                  zoom out, twice
 *     0                       the whole set again
 *     q or Q                  quit
 *
 * The cursor keys are the xterm sequences ESC [ A to ESC [ D, and also
 * ESC O A to ESC O D. Other sequences, such as those of the function
 * keys, are dropped.
 *
 * The same code runs on NICE and on the host, so that the host can
 * check the screen after a series of keys.
 *
 * Copyright (c) 2026 Michael Jørgensen. MIT license (see LICENSE).
 ***************************************************************/

#ifndef VIEW_H
#define VIEW_H

#include <stdint.h>
#include "mandel.h"

// A pan moves the view by this many points.
#define VIEW_PAN 8

// Each zoom in from the start adds this many to the highest count, so
// that the border keeps its detail.
#define VIEW_ITER_PER_LEVEL 64

struct view
{
    int32_t re0;        // The real part of the left edge
    int32_t im0;        // The imaginary part of the middle
    int32_t step;       // The distance between points
    int level;          // Zooms in from the start, less zooms out
    int max_iter;       // The highest count, for this level
    int seq;            // The bytes of an escape sequence so far
};

// What a key did.
#define VIEW_QUIT  (-1)
#define VIEW_SAME  0
#define VIEW_MOVED 1

// The whole set, with no escape sequence begun.
void view_reset(struct view *v);

// Takes one byte of the keys. Returns VIEW_MOVED if the view changed,
// VIEW_QUIT on a quit, and VIEW_SAME otherwise: for a byte of a
// sequence, a key that does nothing, or a move past a limit.
int view_key(struct view *v, int byte);

// The real part of the points of column col, 0 at the left.
static inline int32_t view_re(const struct view *v, int col)
{
    return v->re0 + col * v->step + v->step / 2;
}

// The imaginary part of the points of row row, 0 at the top.
static inline int32_t view_im(const struct view *v, int row)
{
    return v->im0 + (ROWS2 - 1 - 2 * row) * v->step / 2;
}

#endif
