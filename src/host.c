/***************************************************************
 * host.c: stages 1 and 2 on the host, for checking the NICE builds.
 *
 *     mandel_host STAGE [--keys KEYS] [--words | --count]
 *
 * With no mode it prints the points in characters, by count. --words
 * prints the 2000 screen words that the NICE build writes, one per
 * line, in hex, and --count the total of the iterations. In stage 2
 * both have the label of label.h in the top right corner.
 *
 * STAGE is 1, 80x25, or 2, 80x50. --keys, for stage 2 only, applies
 * the bytes of KEYS to the view first (view.h), as the NICE build does
 * when they are its input: the picture is that of the view after the
 * last key. A q in KEYS is an error, as the NICE build then clears the
 * screen. The counts are those of the NICE builds, as mandel.c computes
 * the same products in C.
 *
 * Copyright (c) 2026 Michael Jørgensen. MIT license (see LICENSE).
 ***************************************************************/

#include <stdio.h>
#include <string.h>
#include "label.h"
#include "mandel.h"
#include "view.h"

static const char shades[] = " .:-=+*%@";

static char shade(int count, int max)
{
    return count >= max ? '#' : shades[count % (sizeof shades - 1)];
}

static int usage(const char *name)
{
    fprintf(stderr, "usage: %s 1|2 [--keys KEYS] [--words | --count]\n",
            name);
    return 1;
}

int main(int argc, char *argv[])
{
    const char *stage = argc > 1 ? argv[1] : "";
    const char *keys = NULL;
    int words = 0, count_only = 0;
    unsigned long iterations = 0;

    if (strcmp(stage, "1") && strcmp(stage, "2"))
        return usage(argv[0]);
    for (int i = 2; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--words") && !count_only)
            words = 1;
        else if (!strcmp(argv[i], "--count") && !words)
            count_only = 1;
        else if (!strcmp(argv[i], "--keys") && i + 1 < argc && !keys
                 && *stage == '2')
            keys = argv[++i];
        else
            return usage(argv[0]);
    }

    if (*stage == '1')
    {
        for (int row = 0; row < ROWS1; ++row)
        {
            int32_t b = mandel_im(row, ROWS1, STEP1);

            for (int col = 0; col < COLS; ++col)
            {
                int count = mandel_count(mandel_re(col), b, MAX_ITER);

                iterations += (unsigned long) count;
                if (words)
                    printf("%08x\n", (unsigned) mandel_cell1(count));
                else if (!count_only)
                    putchar(shade(count, MAX_ITER));
            }
            if (!words && !count_only)
                putchar('\n');
        }
    }
    else
    {
        struct view v;

        view_reset(&v);
        for (const char *k = keys ? keys : ""; *k; ++k)
        {
            if (view_key(&v, (unsigned char) *k) == VIEW_QUIT)
            {
                fprintf(stderr, "%s: the keys quit\n", argv[0]);
                return 1;
            }
        }

        // A row of cells is two rows of points: the characters print
        // them as two lines.
        for (int row = 0; row < ROWS2 / 2; ++row)
        {
            int32_t b_top = view_im(&v, 2 * row);
            int32_t b_bottom = view_im(&v, 2 * row + 1);
            char lines[2][COLS + 1];

            for (int col = 0; col < COLS; ++col)
            {
                int32_t a = view_re(&v, col);
                int top = mandel_count(a, b_top, v.max_iter);
                int bottom = mandel_count(a, b_bottom, v.max_iter);
                char label = label_char(row, col);

                iterations += (unsigned long) (top + bottom);
                if (words)
                    printf("%08x\n",
                           (unsigned) label_cell(row, col,
                                      mandel_cell2(top, bottom, v.max_iter)));
                lines[0][col] = label ? label : shade(top, v.max_iter);
                lines[1][col] = label ? ' ' : shade(bottom, v.max_iter);
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
