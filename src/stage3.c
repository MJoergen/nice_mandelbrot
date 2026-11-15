/***************************************************************
 * stage3.c: the Mandelbrot set at 160x100, eight points per character
 * cell, two across and four down, with the keys of view.h to pan and
 * zoom.
 *
 * The program copies the character ROM into the font RAM, writes the
 * 128 glyphs of the octants over codes 0x80 to 0xFF (mandel.h), and
 * shows the font RAM (nice_cpu's SYSTEM.md §1.8). A point is then 8x8
 * pixels, square. Each cell has two colors, which mandel_cell3 picks
 * from its eight points, so the picture is exact where a cell's points
 * have at most two colors, and approximate where the bands meet. The
 * top right corner has the label of label.h, in the ROM's letters,
 * which stay at codes below 0x80.
 *
 * The picture fills in a row of cells, four rows of points, at a time,
 * and the keys work as in stage 2: after each row the program reads
 * the keys that wait, starts a new picture on a key that moves the
 * view, writes its figures after a whole picture, and waits for a key.
 * On q it shows the ROM again, clears the screen, and returns. At the
 * end of the console's input it returns at once and leaves the screen
 * and the font as they are.
 *
 * This program is for NICE only.
 *
 * Copyright (c) 2026 Michael Jørgensen. MIT license (see LICENSE).
 ***************************************************************/

#include <stdint.h>
#include <nice.h>
#include "mandel.h"
#include "label.h"
#include "report.h"
#include "view.h"

#define FONT_RAM  0xFFFE0000u
#define FONT_ROM  0xFFFE2000u
#define FONT_BASE 0xFFFFFF86u

// The ROM's glyphs into the font RAM, the octants over GLYPH_OCTANT
// to 0xFF, and the RAM shown.
static void font_init(void)
{
    for (unsigned i = 0; i < GLYPH_OCTANT * 32; ++i)
        nice_poke(FONT_RAM + i, nice_peek(FONT_ROM + i));
    for (unsigned code = GLYPH_OCTANT; code < 256; ++code)
        for (int r = 0; r < 32; ++r)
            nice_poke(FONT_RAM + 32 * code + (unsigned) r, mandel_glyph3(code, r));
    nice_poke(FONT_BASE, FONT_RAM);
}

// Draws the picture of v. Returns VIEW_SAME when it is whole, or what
// a key that came before the end did: VIEW_MOVED or VIEW_QUIT.
static int draw(struct view *v)
{
    unsigned long iterations = 0;
    int max = v->max_iter;

    report_start();
    for (int row = 0; row < ROWS3 / 4; ++row)
    {
        int32_t b[4];
        int c;

        for (int r = 0; r < 4; ++r)
            b[r] = view_im(v, 4 * row + r);
        for (int col = 0; col < COLS; ++col)
        {
            int count[8];

            for (int i = 0; i < 8; ++i)
            {
                count[i] = mandel_count(view_re(v, 2 * col + i % 2), b[i / 2], max);
                iterations += (unsigned long) count[i];
            }
            nice_poke(NICE_SCREEN + (unsigned) (COLS * row + col),
                      label_cell(row, col, mandel_cell3(count, max)));
        }
        while ((c = nice_pollc()) >= 0)
        {
            int r = view_key(v, c);

            if (r != VIEW_SAME)
                return r;
        }
    }
    report_end("nice_mandelbrot stage 3: 160x100", iterations);
    return VIEW_SAME;
}

int main(void)
{
    struct view v;
    int r;

    view_init(&v, COLS3, ROWS3);
    font_init();
    do
    {
        r = draw(&v);
        while (r == VIEW_SAME)
        {
            int c = nice_getc();

            if (c < 0)
                return 0;
            r = view_key(&v, c);
        }
    } while (r == VIEW_MOVED);

    nice_poke(FONT_BASE, 0);
    nice_screen_clear();
    return 0;
}
