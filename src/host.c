/***************************************************************
 * host.c: stage 1 on the host, for checking the NICE build.
 *
 *     mandel_host          the picture in characters, by count
 *     mandel_host --words  the 2000 screen words that the NICE build
 *                          writes, one per line, in hex
 *     mandel_host --count  the total of the iterations
 *
 * The counts are those of the NICE build, as mandel.c computes the
 * same products in C.
 *
 * Copyright (c) 2026 Michael Jørgensen. MIT license (see LICENSE).
 ***************************************************************/

#include <stdio.h>
#include <string.h>
#include "mandel.h"

int main(int argc, char *argv[])
{
    const char *mode = argc > 1 ? argv[1] : "";
    unsigned long iterations = 0;

    if (*mode && strcmp(mode, "--words") && strcmp(mode, "--count"))
    {
        fprintf(stderr, "usage: %s [--words | --count]\n", argv[0]);
        return 1;
    }

    for (int row = 0; row < ROWS; ++row)
    {
        int32_t b = mandel_im(row);

        for (int col = 0; col < COLS; ++col)
        {
            int count = mandel_count(mandel_re(col), b);
            uint32_t color = mandel_color(count);

            iterations += (unsigned long) count;
            if (!strcmp(mode, "--words"))
                printf("%08x\n", (unsigned) (color << 20 | color << 8 | ' '));
            else if (!*mode)
                putchar(count >= MAX_ITER ? '#' : " .:-=+*%@"[count % 9]);
        }
        if (!*mode)
            putchar('\n');
    }
    if (!strcmp(mode, "--count"))
        printf("%lu\n", iterations);
    return 0;
}
