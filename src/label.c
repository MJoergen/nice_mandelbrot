/***************************************************************
 * label.c: the label in the top right corner of stages 2 to 4.
 *
 * In stage 4 the label is a row of glyphs of 3x5 pixels, each with a
 * clear column after it, inside a frame of one clear pixel: 7 rows,
 * and 4 columns a character and one more. Where the name and the
 * version are wider than the screen, the version is cut at its end.
 *
 * Copyright (c) 2026 Michael Jørgensen. MIT license (see LICENSE).
 ***************************************************************/

#include "label.h"
#include "mandel.h"

#define LABEL_ROW 0
#define LABEL_BG  0x000u
#define LABEL_FG  0xFFFu

static const char name[] = " nice_mandelbrot ";

// Stage 4: the name in capitals, and the glyphs of 3x5 pixels, row 0
// in bits 14..12, the leftmost pixel the highest bit. A lowercase letter
// takes the glyph of its capital, and a character without a glyph that
// of UNKNOWN, a question mark.
static const char name4[] = "NICE_MANDELBROT ";
static const char glyph_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.";
static const uint16_t glyphs[] =
{
    0x2BED, 0x6BAE, 0x3923, 0x6B6E, 0x79A7, 0x79A4, 0x396B, 0x5BED,
    0x7497, 0x126A, 0x5BAD, 0x4927, 0x5FED, 0x6B6D, 0x2B6A, 0x6BA4,
    0x2B73, 0x6BAD, 0x388E, 0x7492, 0x5B6F, 0x5B6A, 0x5BFD, 0x5AAD,
    0x5A92, 0x72A7, 0x7B6F, 0x2C97, 0x62A7, 0x628E, 0x5BC9, 0x798E,
    0x39AA, 0x7292, 0x2AAA, 0x2ACE, 0x0007, 0x01C0, 0x0002,
};
#define UNKNOWN 0x6282
#define PIXEL_COLS 160

static int length(const char *s)
{
    int n = 0;

    while (s[n])
        ++n;
    return n;
}

char label_char(int row, int col)
{
    int name_len = (int) sizeof name - 1;
    int version_len = 0;
    int i;

    if (row != LABEL_ROW)
        return 0;
    version_len = length(nice_mandelbrot_version);
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

static unsigned glyph(char c)
{
    if (c >= 'a' && c <= 'z')
        c = (char) (c - 'a' + 'A');
    if (c == ' ')
        return 0;
    for (int i = 0; glyph_chars[i]; ++i)
        if (glyph_chars[i] == c)
            return glyphs[i];
    return UNKNOWN;
}

int label_bit(int x, int y)
{
    int name_len, version_len, i, gx;
    char c;

    if (y >= LABEL_ROWS4)
        return LABEL_NONE;
    name_len = (int) sizeof name4 - 1;
    version_len = length(nice_mandelbrot_version);
    if (4 * (name_len + version_len) + 1 > PIXEL_COLS)
        version_len = (PIXEL_COLS - 1) / 4 - name_len;
    x -= PIXEL_COLS - (4 * (name_len + version_len) + 1);
    if (x < 0)
        return LABEL_NONE;
    if (x == 0 || y == 0 || y == LABEL_ROWS4 - 1)
        return 0;               // The frame
    i = (x - 1) / 4;
    gx = (x - 1) % 4;
    if (gx == 3)
        return 0;               // The column after a glyph
    c = i < name_len ? name4[i] : nice_mandelbrot_version[i - name_len];
    return (int) (glyph(c) >> (14 - 3 * (y - 1) - gx) & 1);
}

uint32_t label_pixel(int x, int y, uint32_t word)
{
    int bit = label_bit(x, y);

    return bit == LABEL_NONE ? word
         : mandel_pixel4(bit ? LABEL_FG : LABEL_BG);
}
