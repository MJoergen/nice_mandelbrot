/***************************************************************
 * stage1.c: the Mandelbrot set on the 80x25 screen, one point per
 * character cell, drawn as the cell's background color.
 *
 * Each cell gets a blank in its color as background and foreground,
 * as soon as its count is known, so the picture fills in row by row.
 * Then the program writes to the console the iterations, the clock
 * cycles, the instructions, and the microseconds that the picture
 * took, and waits for a key. On a key it clears the screen and
 * returns. At the end of the console's input, as in niceemu with no
 * input, it returns at once and leaves the screen as it is.
 *
 * The figures go straight to the console's register, not through
 * nice_putc, so that under the monitor on the MEGA65 they go to the
 * line and not over the picture.
 *
 * This program is for NICE only.
 *
 * Copyright (c) 2026 Michael Jørgensen. MIT license (see LICENSE).
 ***************************************************************/

#include <stdint.h>
#include <nice.h>
#include "mandel.h"

// A screen word: the background in bits 31..20, the foreground in
// bits 19..8, and the character in bits 7..0 (DEVICES.md §1.1).
#define CELL(color) ((uint32_t) (color) << 20 | (uint32_t) (color) << 8 | ' ')

static void put_str(const char *s)
{
    while (*s)
        nice_poke(NICE_CONSOLE, (unsigned char) *s++);
}

static void put_dec(unsigned long long v)
{
    char buf[21];
    char *p = buf + sizeof buf;

    *--p = 0;
    do
    {
        *--p = (char) ('0' + v % 10);
        v /= 10;
    } while (v);
    put_str(p);
}

static void put_figure(const char *name, unsigned long long v)
{
    put_str(name);
    put_dec(v);
    put_str("\n");
}

int main(void)
{
    unsigned long long cycles = nice_cycles();
    unsigned long long instructions = nice_instructions();
    unsigned long long micros = nice_micros();
    unsigned long iterations = 0;

    for (int row = 0; row < ROWS; ++row)
    {
        int32_t b = mandel_im(row);

        for (int col = 0; col < COLS; ++col)
        {
            int count = mandel_count(mandel_re(col), b);

            iterations += (unsigned long) count;
            nice_poke(NICE_SCREEN + (unsigned) (COLS * row + col),
                      CELL(mandel_color(count)));
        }
    }

    cycles = nice_cycles() - cycles;
    instructions = nice_instructions() - instructions;
    micros = nice_micros() - micros;

    put_str("nice_mandelbrot stage 1: 80x25\n");
    put_figure("iterations   ", iterations);
    put_figure("cycles       ", cycles);
    put_figure("instructions ", instructions);
    put_figure("microseconds ", micros);

    if (nice_getc() >= 0)
        nice_screen_clear();
    return 0;
}
