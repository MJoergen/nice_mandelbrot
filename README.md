# nice_mandelbrot

The classic Mandelbrot rendering, on the NICE CPU.

The program runs on [NiceMonMEGA65](https://github.com/MJoergen/NiceMonMEGA65),
the [NICE CPU](https://github.com/MJoergen/nice_cpu) on the MEGA65, and in
`niceemu`. It is written in C first, and later in NICE BASIC, the
interpreter planned for the ROM of NiceMonMEGA65.

## The algorithm

Each point c of the complex plane is iterated by z ↦ z² + c, starting
from z = 0, until |z|² > 4 or a maximum count of iterations is reached.
The count gives the color of the point. Points that reach the maximum
are taken to be in the set, and are drawn black.

With z = x + iy and c = a + ib, one iteration is

    x2 = x·x
    y2 = y·y
    if x2 + y2 > 4 then stop
    y  = 2·x·y + b
    x  = x2 − y2 + a

that is three multiplications, and a few additions and a compare.

In C the numbers are 32-bit fixed point, with a few integer bits and the
rest fraction, and each product is a 64-bit product of the math unit
(MPU, nice_cpu's DEVICES.md §2), shifted back. Fixed point keeps the
inner loop off the floating-point unit, whose `MUL` takes some 20 cycles.

## The resolutions

The screen is 80×25 characters, each 16×32 pixels, so 1280×800 pixels.
There are two stages:

| Stage | Resolution | How a point is drawn | Pixel shape on the screen |
|---|---|---|---|
| 1 | 80×25 | one character cell per point, its background color | 16×32 |
| 2 | 160×50 | a font of 2×2 blocks in the font RAM | 8×16 |

The points are twice as tall as they are wide in both stages, so the
step in the imaginary direction is twice the step in the real one.

### Colors in the 2×2 font

The 2×2 font has 2⁴ = 16 glyphs, one for each pattern of set blocks,
written to the font RAM (nice_cpu's DEVICES.md §1.8). A set block is
drawn in the cell's foreground color, and a clear block in its
background color.

A screen word has only one foreground and one background color, so a
cell shows at most two colors, whatever its glyph. The four points of
a cell can have four different counts. Where they do, the program must
pick two colors for the cell, for example those of the two most frequent
counts, and draw each of the other blocks in the nearer of the two. So
the 160×50 image is exact where neighbouring points have at most two
colors, which is most of the screen, and approximate along the bands
where the colors change quickly, near the border of the set.

The choice of glyph and colors is not unique: a pattern and its inverse
with the colors swapped look the same. Only 8 of the 16 glyphs are
needed if the program always puts, say, the top-left block in the
background color.

## Estimated running times

The estimate counts iterations and the cycles of one iteration on the
pipelined core at 100 MHz.

**Iterations per point.** For the whole set, real part −2.0 to 1.0, with a
maximum of 256 iterations, about 21% of the points are in the set (its
area is about 1.5 of the 7.2 of the view) and take all 256 iterations.
The points outside escape after some 5 to 15 on average. That is about
65 iterations per point on average.

**Cycles per iteration.** Each multiplication through the MPU is two
writes, one or two reads, and a shift: some 6 to 8 cycles. With the
additions, the compare, and the loop, an iteration takes some 40 cycles
when written by hand, and some 60 (40 to 100) when compiled from C by
clang and `nicewasm`, which takes about 1.4 cycles per instruction on
the pipelined core. That is about 0.6 µs per iteration.

| Resolution | Points | Iterations | C (≈ 0.6 µs each) | NICE BASIC (≈ 20–30 µs each) |
|---|---|---|---|---|
| 80×25 | 2000 | 130 000 | ≈ 0.1 s (0.05–0.15) | ≈ 3–4 s |
| 160×50 | 8000 | 520 000 | ≈ 0.3 s (0.2–0.5) | ≈ 10–15 s |

The BASIC column is the roughest. It assumes an interpreter that spends
some 1000 to 3000 cycles on each statement of the loop, with its numbers
on the FPU, so 30 to 50 times slower than C.

Notes on the estimate:

- The time grows linearly with the average count. A zoom near the
  border of the set, with a higher maximum, can take ten times as long.
- Most of the time goes to the points in the set. A test for the main
  cardioid and the period-2 bulb, before the loop, skips most of them
  and about halves the time of the whole view.
- Drawing is not counted: 8000 screen writes, and the choice of two
  colors for each of 2000 cells, take a few milliseconds at most.
- The figures are to be checked against the performance counters
  (DEVICES.md §3), which count cycles and microseconds on the board.

## Building and running

Stage 1 builds with the toolchain of a checkout of
[nice_cpu](https://github.com/MJoergen/nice_cpu), `../nice_cpu` by
default (`NICE_CPU=DIR`), with niceemu built in it
(`make -C ../nice_cpu/emulator build`), and clang with the wasm32 target.

    make all     # build/nice/stage1.bin for niceemu, build/mega65/stage1.bin
                 # for the MEGA65's monitor at 0x8000, build/host/mandel_host
    make test    # both NICE builds in niceemu, their screens compared with the host's
    make run     # stage 1 in niceemu, its screen printed in color

The program draws the picture row by row, then writes the iterations,
cycles, instructions, and microseconds it took to the console's line,
and waits for a key. On a key it clears the screen and returns. At the
end of the console's input, as in niceemu, it returns at once.

The files:

- `src/mandel.c`, `src/mandel.h`: the iteration, in fixed point with 25
  fraction bits, and the colors. On NICE each product is one of the math
  unit, through `nice_poke` and `nice_peek`; on the host it is a 64-bit
  product in C, with the same bits.
- `src/stage1.c`: stage 1, for NICE.
- `src/host.c`: stage 1 on the host, as characters, or as the screen
  words of the NICE build, for `make test`.

## Status

Stage 1 is written and runs in niceemu. In niceemu it takes 152 085
iterations, 76 per point on average, and 6 246 779 instructions, so
about 41 instructions per iteration. At the 54 million instructions a
second that nice_chess measures on the MEGA65, that is about 0.12 s.
It is not yet run on the board.

## License

See [LICENSE](LICENSE).
