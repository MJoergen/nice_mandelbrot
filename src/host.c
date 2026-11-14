/***************************************************************
 * host.c: stages 1 and 2 on the host, for checking the NICE builds.
 *
 *     mandel_host STAGE          the points in characters, by count
 *     mandel_host STAGE --words  the 2000 screen words that the NICE
 *                                build writes, one per line, in hex
 *     mandel_host STAGE --count  the total of the iterations
 *
 * STAGE is 1, 80x25, or 2, 80x50. The counts are those of the NICE
 * builds, as mandel.c computes the same products in C.
 *
 * Copyright (c) 2026 Michael Jørgensen. MIT license (see LICENSE).
 ***************************************************************/

#include <stdio.h>
#include <string.h>
#include "mandel.h"

static const char shades[] = " .:-=+*%@";

static char shade(int count)
{
    return count >= MAX_ITER ? '#' : shades[count % (sizeof shades - 1)];
}

int main(int argc, char *argv[])
{
    const char *stage = argc > 1 ? argv[1] : "";
    const char *mode = argc > 2 ? argv[2] : "";
    int words = !strcmp(mode, "--words");
    int count_only = !strcmp(mode, "--count");
    unsigned long iterations = 0;

    if ((strcmp(stage, "1") && strcmp(stage, "2")) || argc > 3
        || (*mode && !words && !count_only))
    {
        fprintf(stderr, "usage: %s 1|2 [--words | --count]\n", argv[0]);
        return 1;
    }

    if (*stage == '1')
    {
        for (int row = 0; row < ROWS1; ++row)
        {
            int32_t b = mandel_im(row, ROWS1, STEP1);

            for (int col = 0; col < COLS; ++col)
            {
                int count = mandel_count(mandel_re(col), b);

                iterations += (unsigned long) count;
                if (words)
                    printf("%08x\n", (unsigned) mandel_cell1(count));
                else if (!count_only)
                    putchar(shade(count));
            }
            if (!words && !count_only)
                putchar('\n');
        }
    }
    else
    {
        // A row of cells is two rows of points: the characters print
        // them as two lines.
        for (int row = 0; row < ROWS2 / 2; ++row)
        {
            int32_t b_top = mandel_im(2 * row, ROWS2, STEP2);
            int32_t b_bottom = mandel_im(2 * row + 1, ROWS2, STEP2);
            char lines[2][COLS + 1];

            for (int col = 0; col < COLS; ++col)
            {
                int32_t a = mandel_re(col);
                int top = mandel_count(a, b_top);
                int bottom = mandel_count(a, b_bottom);

                iterations += (unsigned long) (top + bottom);
                if (words)
                    printf("%08x\n", (unsigned) mandel_cell2(top, bottom));
                lines[0][col] = shade(top);
                lines[1][col] = shade(bottom);
            }
            lines[0][COLS] = lines[1][COLS] = 0;
            if (!words && !count_only)
                printf("%s\n%s\n", lines[0], lines[1]);
        }
    }
    if (count_only)
        printf("%lu\n", iterations);
    return 0;
}
