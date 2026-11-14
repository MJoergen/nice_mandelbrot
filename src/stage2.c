/***************************************************************
 * stage2.c: the Mandelbrot set at 80x50, two points per character
 * cell, one above the other.
 *
 * The program copies the character ROM into the font RAM, writes the
 * upper half block over code GLYPH_HALF, 0x80, which is blank in the
 * ROM, and shows the font RAM (nice_cpu's DEVICES.md §1.8). Each cell
 * is that glyph, in the top point's color, on the bottom point's
 * color. A point is then 16x16 pixels, square, so the imaginary step
 * is the real one.
 *
 * The picture fills in a row of cells, two rows of points, at a time.
 * Then the program writes its figures to the console's line, as
 * stage 1 does, and waits for a key. On a key it shows the ROM again,
 * clears the screen, and returns. At the end of the console's input,
 * as in niceemu with no input, it returns at once and leaves the
 * screen and the font as they are.
 *
 * This program is for NICE only.
 *
 * Copyright (c) 2026 Michael Jørgensen. MIT license (see LICENSE).
 ***************************************************************/

#include <stdint.h>
#include <nice.h>
#include "mandel.h"
#include "report.h"

#define FONT_RAM  0xFFFE0000u
#define FONT_ROM  0xFFFE2000u
#define FONT_BASE 0xFFFFFF86u

// The ROM's glyphs into the font RAM, the upper half block over
// GLYPH_HALF, and the RAM shown.
static void font_init(void)
{
    for (unsigned i = 0; i < 256 * 32; ++i)
        nice_poke(FONT_RAM + i, nice_peek(FONT_ROM + i));
    for (unsigned r = 0; r < 32; ++r)
        nice_poke(FONT_RAM + 32 * GLYPH_HALF + r, r < 16 ? 0xFFFFu : 0);
    nice_poke(FONT_BASE, FONT_RAM);
}

int main(void)
{
    unsigned long iterations = 0;

    font_init();
    report_start();
    for (int row = 0; row < ROWS2 / 2; ++row)
    {
        int32_t b_top = mandel_im(2 * row, ROWS2, STEP2);
        int32_t b_bottom = mandel_im(2 * row + 1, ROWS2, STEP2);

        for (int col = 0; col < COLS; ++col)
        {
            int32_t a = mandel_re(col);
            int top = mandel_count(a, b_top);
            int bottom = mandel_count(a, b_bottom);

            iterations += (unsigned long) (top + bottom);
            nice_poke(NICE_SCREEN + (unsigned) (COLS * row + col),
                      mandel_cell2(top, bottom));
        }
    }
    report_end("nice_mandelbrot stage 2: 80x50", iterations);

    if (nice_getc() >= 0)
    {
        nice_poke(FONT_BASE, 0);
        nice_screen_clear();
    }
    return 0;
}
