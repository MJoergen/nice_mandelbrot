/***************************************************************
 * stage1.c: the Mandelbrot set on the 80x25 screen, one point per
 * character cell, drawn as the cell's background color.
 *
 * Each cell gets a blank in its color as background and foreground,
 * as soon as its count is known, so the picture fills in row by row.
 * Then the program writes to the console's line the iterations, the
 * clock cycles, the instructions, and the microseconds that the
 * picture took (report.c), and waits for a key. On a key it clears
 * the screen and returns. At the end of the console's input, as in
 * niceemu with no input, it returns at once and leaves the screen as
 * it is.
 *
 * This program is for NICE only.
 *
 * Copyright (c) 2026 Michael Jørgensen. MIT license (see LICENSE).
 ***************************************************************/

#include <stdint.h>
#include <nice.h>
#include "mandel.h"
#include "report.h"

int main(void)
{
    unsigned long iterations = 0;

    report_start();
    for (int row = 0; row < ROWS1; ++row)
    {
        int32_t b = mandel_im(row, ROWS1, STEP1);

        for (int col = 0; col < COLS; ++col)
        {
            int count = mandel_count(mandel_re(col), b, MAX_ITER);

            iterations += (unsigned long) count;
            nice_poke(NICE_SCREEN + (unsigned) (COLS * row + col),
                      mandel_cell1(count));
        }
    }
    report_end("nice_mandelbrot stage 1: 80x25", iterations);

    if (nice_getc() >= 0)
        nice_screen_clear();
    return 0;
}
