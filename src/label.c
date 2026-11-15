/***************************************************************
 * label.c: the label in the top right corner of stage 2.
 *
 * Copyright (c) 2026 Michael Jørgensen. MIT license (see LICENSE).
 ***************************************************************/

#include "label.h"
#include "mandel.h"

#define LABEL_ROW 0
#define LABEL_BG  0x000u
#define LABEL_FG  0xFFFu

static const char name[] = " nice_mandelbrot ";

char label_char(int row, int col)
{
    int name_len = (int) sizeof name - 1;
    int version_len = 0;
    int i;

    if (row != LABEL_ROW)
        return 0;
    while (nice_mandelbrot_version[version_len])
        ++version_len;
    if (name_len + version_len > COLS)
        version_len = COLS - name_len;
    i = col - (COLS - name_len - version_len);
    if (i < 0)
        return 0;
    return i < name_len ? name[i] : nice_mandelbrot_version[i - name_len];
}

uint32_t label_cell(int row, int col, uint32_t word)
{
    char c = label_char(row, col);

    return c ? mandel_word(LABEL_BG, LABEL_FG, (unsigned char) c) : word;
}
