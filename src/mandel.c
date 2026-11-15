/***************************************************************
 * mandel.c: the Mandelbrot iteration, in 32-bit fixed point.
 *
 * One iteration, with z = x + iy and c = a + ib, is
 *
 *     x2 = x*x, y2 = y*y
 *     if x2 + y2 > 4 then stop
 *     y  = 2*x*y + b
 *     x  = x2 - y2 + a
 *
 * On NICE each of the three products goes through the math unit: write
 * the operands A and B, read SMUL_HI and SMUL_LO, the signed 64-bit
 * product, and shift it back by FRAC bits. The order x*x, x*y, y*y
 * changes one operand at a time, so the three products take four
 * writes, not six. 2*x*y is shifted by one bit less, which keeps its
 * last bit. On the host the same products are computed in C.
 *
 * Before the loop, a point in the main cardioid or in the period-2
 * bulb, the disc left of it, is given max at once. Those points never
 * escape, and they are most of the points that reach max, so the test
 * saves most of the time of a view of the whole set. With q =
 * (a - 1/4)^2 + b^2 the cardioid is q (q + a - 1/4) <= b^2 / 4, and the
 * bulb is (a + 1)^2 + b^2 <= 1/16. Each is tested only inside a box
 * around it, where the products cannot overflow.
 *
 * Near the cusp of the cardioid, at a = 1/4, all of q, a - 1/4, and b
 * are tiny, and their products round to 0, so a point just outside
 * would pass a test of <=. The cardioid's test therefore has a margin
 * of 4 in the last bit: such points are iterated, as are the points
 * just inside, and each point that passes never reaches |z| > 2 in the
 * fixed point either (checked on the host over dense grids at the
 * cusp, at the pinch at a = -3/4, at the edges of the bulb, and at
 * random points of both boxes, with 1472 iterations).
 *
 * Copyright (c) 2026 Michael Jørgensen. MIT license (see LICENSE).
 ***************************************************************/

#include "mandel.h"

#ifdef __wasm__

#include <nice.h>

#define MPU_A       0xFFFFFF20u
#define MPU_B       0xFFFFFF21u
#define MPU_SMUL_LO 0xFFFFFF22u
#define MPU_SMUL_HI 0xFFFFFF23u

#define SET_A(v) nice_poke(MPU_A, (unsigned) (v))
#define SET_B(v) nice_poke(MPU_B, (unsigned) (v))
// A * B, shifted right by s bits, 0 < s < 32. A read of the math unit
// has no side effect, so the order of the two reads does not matter.
#define PROD(s)  ((int32_t) (nice_peek(MPU_SMUL_HI) << (32 - (s)) | \
                             nice_peek(MPU_SMUL_LO) >> (s)))

#else

static int32_t mpu_a, mpu_b;
#define SET_A(v) (mpu_a = (v))
#define SET_B(v) (mpu_b = (v))
// The same bits as on NICE: the arithmetic shift of the 64-bit product,
// truncated to 32 bits.
#define PROD(s)  ((int32_t) (uint32_t) ((int64_t) mpu_a * mpu_b >> (s)))

#endif

// The boxes around the cardioid, a from -3/4 to 3/8 and |b| <= 2/3,
// and around the bulb, a from -5/4 to -3/4 and |b| <= 1/4.
#define CARD_A_MIN FIX(-3, 4)
#define CARD_A_MAX FIX(3, 8)
#define CARD_B_MAX FIX(2, 3)
#define BULB_A_MIN FIX(-5, 4)
#define BULB_B_MAX FIX(1, 4)
#define CARD_MARGIN 4

// Whether c = a + ib is in the main cardioid or the period-2 bulb.
static int in_main(int32_t a, int32_t b)
{
    int32_t abs_b = b < 0 ? -b : b;

    if (a >= CARD_A_MIN && a <= CARD_A_MAX && abs_b <= CARD_B_MAX)
    {
        int32_t x = a - FIX(1, 4);

        SET_A(x);
        SET_B(x);
        int32_t x2 = PROD(FRAC);
        SET_A(b);
        SET_B(b);
        int32_t b2 = PROD(FRAC);
        int32_t q = x2 + b2;

        SET_A(q);
        SET_B(q + x);
        return PROD(FRAC) < (b2 >> 2) - CARD_MARGIN;
    }
    if (a >= BULB_A_MIN && a < CARD_A_MIN && abs_b <= BULB_B_MAX)
    {
        int32_t x = a + FIX(1, 1);

        SET_A(x);
        SET_B(x);
        int32_t x2 = PROD(FRAC);
        SET_A(b);
        SET_B(b);
        return x2 + PROD(FRAC) <= FIX(1, 16);
    }
    return 0;
}

int mandel_count(int32_t a, int32_t b, int max)
{
    int32_t x = 0, y = 0;
    int n;

    if (in_main(a, b))
        return max;

    for (n = 0; n < max; ++n)
    {
        SET_A(x);
        SET_B(x);
        int32_t x2 = PROD(FRAC);
        SET_A(y);
        int32_t xy2 = PROD(FRAC - 1);
        SET_B(y);
        int32_t y2 = PROD(FRAC);

        if (x2 + y2 > FOUR)
            break;
        y = xy2 + b;
        x = x2 - y2 + a;
    }
    return n;
}

// The colors of the counts outside the set, which repeat every 16:
// from dark blue through white to orange and dark red.
static const uint16_t palette[16] =
{
    0x117, 0x22A, 0x33C, 0x44E, 0x66F, 0x88F, 0xAAF, 0xCCF,
    0xFFE, 0xFEA, 0xFC6, 0xFA3, 0xE82, 0xC51, 0x931, 0x624
};

uint32_t mandel_color(int count, int max)
{
    if (count >= max)
        return 0x000;           // In the set: black
    return palette[count % 16];
}
