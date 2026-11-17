/***************************************************************
 * stage4.c: the Mandelbrot set at 160x100 in the chunky mode of the
 * screen, one screen word a point, with the keys of view.h to pan and
 * zoom.
 *
 * The program writes 1 to the screen's mode word and reads it back
 * (nice_cpu's DEVICES.md §1.10). 1 means that the system has the
 * chunky mode: each of the 16000 screen words is then a point, 8x8
 * pixels, square, in its own color, so unlike stage 3 the picture is
 * exact everywhere. 0 means that it has not, as on NiceMonMEGA65
 * before its V2.3 and on the Nexys4DDR: the program then says so on the
 * console and returns 1. The points are those of stage 3. The top
 * right corner has the label of label.h, drawn in the pixels.
 *
 * The picture fills in a row of points at a time, and the keys work as
 * in stages 2 and 3: after each row the program reads the keys that
 * wait, starts a new picture on a key that moves the view, writes its
 * figures after a whole picture, and waits for a key. On q it shows
 * the text again, clears the screen, and returns. At the end of the
 * console's input it returns at once and leaves the screen and the
 * mode as they are.
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

// Draws the picture of v. Returns VIEW_SAME when it is whole, or what
// a key that came before the end did: VIEW_MOVED or VIEW_QUIT.
static int draw(struct view *v)
{
    unsigned long iterations = 0;
    int max = v->max_iter;

    report_start();
    for (int y = 0; y < ROWS4; ++y)
    {
        int32_t b = view_im(v, y);
        int c;

        for (int x = 0; x < COLS4; ++x)
        {
            int count = mandel_count(view_re(v, x), b, max);

            iterations += (unsigned long) count;
            nice_poke(NICE_SCREEN + (unsigned) (COLS4 * y + x),
                      label_pixel(x, y, mandel_pixel4(mandel_color(count, max))));
        }
        while ((c = nice_pollc()) >= 0)
        {
            int r = view_key(v, c);

            if (r != VIEW_SAME)
                return r;
        }
    }
    report_end("nice_mandelbrot stage 4: 160x100 chunky", iterations);
    return VIEW_SAME;
}

int main(void)
{
    struct view v;
    int r;

    nice_poke(NICE_SCREEN_MODE, NICE_MODE_CHUNKY);
    if (nice_peek(NICE_SCREEN_MODE) != NICE_MODE_CHUNKY)
    {
        report_line("nice_mandelbrot stage 4: the screen has no chunky mode, "
                    "which NiceMonMEGA65 has from V2.3 on, on the MEGA65");
        return 1;
    }
    view_init(&v, COLS4, ROWS4);
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

    nice_poke(NICE_SCREEN_MODE, NICE_MODE_TEXT);
    nice_screen_clear();
    return 0;
}
