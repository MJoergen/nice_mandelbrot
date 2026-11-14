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

int mandel_count(int32_t a, int32_t b)
{
    int32_t x = 0, y = 0;
    int n;

    for (n = 0; n < MAX_ITER; ++n)
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

uint32_t mandel_color(int count)
{
    if (count >= MAX_ITER)
        return 0x000;           // In the set: black
    return palette[count % 16];
}
