/***************************************************************
 * host.c: stages 1 to 4 on the host, for checking the NICE builds.
 *
 *     mandel_host STAGE [--keys KEYS] [--words | --count | --setup]
 *
 * With no mode it prints the points in characters, by count. --words
 * prints the screen words that the NICE build writes, one per line, in
 * hex, 2000 in stages 1 to 3 and 16000 in stage 4, and --count the
 * total of the iterations. In stages 2 to 4 both have the label of
 * label.h in the top right corner. --setup, for stages 2 to 4, prints
 * the other words that the NICE build writes for its screen: in stages
 * 2 and 3 its glyphs in the font RAM from code 0x80, and then its font
 * base, and in stage 4 the screen's mode word.
 *
 * STAGE is 1, 80x25, 2, 80x50, 3, 160x100 in octants, or 4, 160x100
 * in the chunky mode. --keys, for stages 2 to 4, applies the bytes of KEYS to the view first (view.h), as the
 * NICE build does when they are its input: the picture is that of the
 * view after the last key. A q in KEYS is an error, as the NICE build then clears the
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

// What main prints.
enum mode { CHARS, WORDS, COUNT };

// Stage 2: a row of cells is two rows of points, which the characters
// print as two lines. Returns the total of the counts.
static unsigned long draw2(const struct view *v, enum mode mode)
{
    unsigned long iterations = 0;

    for (int row = 0; row < ROWS2 / 2; ++row)
    {
        int32_t b_top = view_im(v, 2 * row);
        int32_t b_bottom = view_im(v, 2 * row + 1);
        char lines[2][COLS + 1];

        for (int col = 0; col < COLS; ++col)
        {
            int32_t a = view_re(v, col);
            int top = mandel_count(a, b_top, v->max_iter);
            int bottom = mandel_count(a, b_bottom, v->max_iter);
            char label = label_char(row, col);

            iterations += (unsigned long) (top + bottom);
            if (mode == WORDS)
                printf("%08x\n",
                       (unsigned) label_cell(row, col,
                                  mandel_cell2(top, bottom, v->max_iter)));
            lines[0][col] = label ? label : shade(top, v->max_iter);
            lines[1][col] = label ? ' ' : shade(bottom, v->max_iter);
        }
        lines[0][COLS] = lines[1][COLS] = 0;
        if (mode == CHARS)
            printf("%s\n%s\n", lines[0], lines[1]);
    }
    return iterations;
}

// Stage 3: a row of cells is four rows of points, which the characters
// print as four lines, each of COLS3 points. A cell of the label is its
// letter at the top left. Returns the total of the counts.
static unsigned long draw3(const struct view *v, enum mode mode)
{
    unsigned long iterations = 0;

    for (int row = 0; row < ROWS3 / 4; ++row)
    {
        int32_t b[4];
        char lines[4][COLS3 + 1];

        for (int r = 0; r < 4; ++r)
            b[r] = view_im(v, 4 * row + r);
        for (int col = 0; col < COLS; ++col)
        {
            int count[8];
            char label = label_char(row, col);

            for (int i = 0; i < 8; ++i)
            {
                count[i] = mandel_count(view_re(v, 2 * col + i % 2), b[i / 2],
                                        v->max_iter);
                iterations += (unsigned long) count[i];
                lines[i / 2][2 * col + i % 2] =
                    !label ? shade(count[i], v->max_iter) : i ? ' ' : label;
            }
            if (mode == WORDS)
                printf("%08x\n",
                       (unsigned) label_cell(row, col,
                                  mandel_cell3(count, v->max_iter)));
        }
        for (int r = 0; r < 4; ++r)
        {
            lines[r][COLS3] = 0;
            if (mode == CHARS)
                printf("%s\n", lines[r]);
        }
    }
    return iterations;
}

// Stage 4: one point a screen word, printed as a line of COLS4
// characters for each row. A pixel of the label is '#' where it is set,
// and ' ' where it is clear. Returns the total of the counts.
static unsigned long draw4(const struct view *v, enum mode mode)
{
    unsigned long iterations = 0;

    for (int y = 0; y < ROWS4; ++y)
    {
        int32_t b = view_im(v, y);
        char line[COLS4 + 1];

        for (int x = 0; x < COLS4; ++x)
        {
            int count = mandel_count(view_re(v, x), b, v->max_iter);
            int bit = label_bit(x, y);

            iterations += (unsigned long) count;
            if (mode == WORDS)
                printf("%08x\n",
                       (unsigned) label_pixel(x, y,
                                  mandel_pixel4(mandel_color(count, v->max_iter))));
            line[x] = bit == LABEL_NONE ? shade(count, v->max_iter)
                    : bit ? '#' : ' ';
        }
        line[COLS4] = 0;
        if (mode == CHARS)
            printf("%s\n", line);
    }
    return iterations;
}

// The other words that stage 2, 3, or 4 writes for its screen: in
// stages 2 and 3 its glyphs in the font RAM from code 0x80,
// GLYPH_HALF or GLYPH_OCTANT, then the font base, which shows the RAM,
// and in stage 4 the mode word, the chunky mode.
static void setup(int stage)
{
    if (stage == 4)
    {
        printf("%08x\n", 1u);
        return;
    }
    for (unsigned code = 0x80; code < (stage == 2 ? 0x81u : 0x100u); ++code)
        for (int r = 0; r < 32; ++r)
            printf("%08x\n", stage == 2 ? (r < 16 ? 0xFFFFu : 0)
                                        : (unsigned) mandel_glyph3(code, r));
    printf("%08x\n", 0xFFFE0000u);
}

static int usage(const char *name)
{
    fprintf(stderr, "usage: %s 1|2|3|4 [--keys KEYS] [--words | --count | --setup]\n",
            name);
    return 1;
}

int main(int argc, char *argv[])
{
    const char *stage = argc > 1 ? argv[1] : "";
    const char *keys = NULL;
    enum mode mode = CHARS;
    int setup_only = 0;
    unsigned long iterations = 0;

    if (strcmp(stage, "1") && strcmp(stage, "2") && strcmp(stage, "3")
        && strcmp(stage, "4"))
        return usage(argv[0]);
    for (int i = 2; i < argc; ++i)
    {
        int no_mode = mode == CHARS && !setup_only;

        if (!strcmp(argv[i], "--words") && no_mode)
            mode = WORDS;
        else if (!strcmp(argv[i], "--count") && no_mode)
            mode = COUNT;
        else if (!strcmp(argv[i], "--setup") && no_mode && *stage != '1')
            setup_only = 1;
        else if (!strcmp(argv[i], "--keys") && i + 1 < argc && !keys
                 && *stage != '1')
            keys = argv[++i];
        else
            return usage(argv[0]);
    }
    if (setup_only)
    {
        setup(*stage - '0');
        return 0;
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
                if (mode == WORDS)
                    printf("%08x\n", (unsigned) mandel_cell1(count));
                else if (mode == CHARS)
                    putchar(shade(count, MAX_ITER));
            }
            if (mode == CHARS)
                putchar('\n');
        }
    }
    else
    {
        struct view v;

        if (*stage == '2')
            view_init(&v, COLS, ROWS2);
        else if (*stage == '3')
            view_init(&v, COLS3, ROWS3);
        else
            view_init(&v, COLS4, ROWS4);
        for (const char *k = keys ? keys : ""; *k; ++k)
        {
            if (view_key(&v, (unsigned char) *k) == VIEW_QUIT)
            {
                fprintf(stderr, "%s: the keys quit\n", argv[0]);
                return 1;
            }
        }
        iterations = *stage == '2' ? draw2(&v, mode)
                   : *stage == '3' ? draw3(&v, mode) : draw4(&v, mode);
    }
    if (mode == COUNT)
        printf("%lu\n", iterations);
    return 0;
}
