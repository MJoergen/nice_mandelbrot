/***************************************************************
 * view.c: the view of stage 2, and the keys that move it.
 *
 * The view stays inside the square of -4 to 4 on both axes. There
 * |c| < 8, so |c|^2 and x*x + y*y stay below 64, the range of the
 * fixed point (mandel.h). A zoom in halves the step, down to
 * MIN_STEP, where the points are 2^-24 apart and the picture shows
 * the steps of the fixed point. A zoom out doubles it, as long as the
 * view still fits the square.
 *
 * Copyright (c) 2026 Michael Jørgensen. MIT license (see LICENSE).
 ***************************************************************/

#include "view.h"

#define LIMIT    FIX(4, 1)
#define MIN_STEP 2
// The widest view, COLS points, fits the square.
#define MAX_STEP (2 * LIMIT / COLS)

static int32_t clamp(int32_t x, int32_t lo, int32_t hi)
{
    return x < lo ? lo : x > hi ? hi : x;
}

// Keeps the view in the square, and sets the highest count.
static void fit(struct view *v)
{
    int32_t half_height = ROWS2 * v->step / 2;

    v->re0 = clamp(v->re0, -LIMIT, LIMIT - COLS * v->step);
    v->im0 = clamp(v->im0, -LIMIT + half_height, LIMIT - half_height);
    v->max_iter = MAX_ITER
                + (v->level > 0 ? v->level * VIEW_ITER_PER_LEVEL : 0);
}

void view_reset(struct view *v)
{
    v->re0 = RE_MIN;
    v->im0 = 0;
    v->step = RE_STEP;
    v->level = 0;
    v->seq = 0;
    fit(v);
}

// Pans by dx points to the right and dy points up.
static void pan(struct view *v, int dx, int dy)
{
    v->re0 += dx * v->step;
    v->im0 += dy * v->step;
}

// The middle of the columns, re0 + COLS / 2 * step, stays where it is.
// The step is rounded down when it is halved.
static void zoom(struct view *v, int32_t step)
{
    v->re0 += COLS / 2 * (v->step - step);
    v->step = step;
}

static int command(struct view *v, int byte)
{
    switch (byte)
    {
    case 'A': pan(v, 0, VIEW_PAN); break;
    case 'B': pan(v, 0, -VIEW_PAN); break;
    case 'C': pan(v, VIEW_PAN, 0); break;
    case 'D': pan(v, -VIEW_PAN, 0); break;
    case '+':
    case '=':
        if (v->step / 2 < MIN_STEP)
            return VIEW_SAME;
        zoom(v, v->step / 2);
        ++v->level;
        break;
    case '-':
    case '_':
        if (v->step > MAX_STEP / 2)
            return VIEW_SAME;
        zoom(v, v->step * 2);
        --v->level;
        break;
    case '0': view_reset(v); return VIEW_MOVED;
    case 'q':
    case 'Q': return VIEW_QUIT;
    default: return VIEW_SAME;
    }
    fit(v);
    return VIEW_MOVED;
}

int view_key(struct view *v, int byte)
{
    struct view old = *v;
    int seq = v->seq;
    int r;

    v->seq = 0;
    if (byte == 0x1B)
    {
        v->seq = 1;                     // ESC
        return VIEW_SAME;
    }
    if (seq == 1 && (byte == '[' || byte == 'O'))
    {
        v->seq = 2;                     // ESC [ or ESC O
        return VIEW_SAME;
    }
    if (seq == 2)
    {
        // The parameters of ESC [ 20 ~ and the like, then the end.
        if ((byte >= '0' && byte <= '9') || byte == ';')
        {
            v->seq = 2;
            return VIEW_SAME;
        }
        if (byte < 'A' || byte > 'D')
            return VIEW_SAME;
        r = command(v, byte);
    }
    else if (byte >= 'A' && byte <= 'D')
        return VIEW_SAME;               // Letters, not cursor keys
    else
        r = command(v, byte);

    // A move against a limit, or a reset of the whole set, may change
    // nothing.
    if (r == VIEW_MOVED && v->re0 == old.re0 && v->im0 == old.im0
        && v->step == old.step)
        return VIEW_SAME;
    return r;
}
