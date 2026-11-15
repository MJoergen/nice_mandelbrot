# nice_mandelbrot

The classic Mandelbrot rendering, on the NICE CPU.

The program runs on [NiceMonMEGA65](https://github.com/MJoergen/NiceMonMEGA65),
the [NICE CPU](https://github.com/MJoergen/nice_cpu) on the MEGA65, and in
`niceemu`. It is written in C first, and later in NICE BASIC, the
interpreter planned for the ROM of NiceMonMEGA65.

![Stage 2, the Mandelbrot set at 80×50](docs/stage2.png)

Stage 2 at 80×50, 1280×800 pixels, drawn from the screen that niceemu
dumps after the run.

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
There are three stages:

| Stage | Resolution | How a point is drawn | Pixel shape on the screen |
|---|---|---|---|
| 1 | 80×25 | one character cell per point, its background color | 16×32 |
| 2 | 80×50 | a glyph of 2×1 blocks, the upper half block, in the font RAM | 16×16 |
| 3 | 160×50 | a font of 2×2 blocks in the font RAM | 8×16 |

In stages 1 and 3 the points are twice as tall as they are wide, so the
step in the imaginary direction is twice the step in the real one. In
stage 2 they are square, and the two steps are the same.

### Colors in the 2×1 glyph

Stage 2 needs one glyph: the upper half block, its top 16 rows set and
its bottom 16 clear, which it writes over code `0x80` of the font RAM
(nice_cpu's DEVICES.md §1.8), a blank in the character ROM. Each cell
is that glyph, in the top point's color as foreground, on the bottom
point's color as background. A cell has two points and two colors, so
the 80×50 image is exact everywhere.

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
the 160×50 image of stage 3 is exact where neighbouring points have at most two
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
| 80×50 | 4000 | 260 000 | ≈ 0.15 s (0.1–0.25) | ≈ 5–8 s |
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

Stages 1 and 2 build with the toolchain of a checkout of
[nice_cpu](https://github.com/MJoergen/nice_cpu), `../nice_cpu` by
default (`NICE_CPU=DIR`), with niceemu built in it
(`make -C ../nice_cpu/emulator build`), and clang with the wasm32 target.

    make all     # build/nice/stageN.bin for niceemu, build/mega65/stageN.bin
                 # for the MEGA65's monitor at 0x8000, build/host/mandel_host
    make test    # both NICE builds of each stage in niceemu, their screens
                 # compared with the host's
    make run     # stage 2 in niceemu, its screen printed in color (STAGE=1 for stage 1)
    make screenshot  # docs/stage2.png, stage 2's screen in niceemu (needs Pillow)

Each program draws the picture a row of cells at a time, then writes
the iterations, cycles, instructions, and microseconds it took to the
console's line, and waits for a key. On a key it clears the screen,
shows the character ROM again, and returns. At the end of the console's
input, as in niceemu, it returns at once.

The files:

- `src/mandel.c`, `src/mandel.h`: the iteration, in fixed point with 25
  fraction bits, and the colors. On NICE each product is one of the math
  unit, through `nice_poke` and `nice_peek`; on the host it is a 64-bit
  product in C, with the same bits.
- `src/stage1.c`, `src/stage2.c`: stages 1 and 2, for NICE.
- `src/report.c`, `src/report.h`: the figures of a run, written to the
  console's register, so that under the monitor they go to the line and
  not over the picture.
- `src/host.c`: stages 1 and 2 on the host, as characters, or as the
  screen words of the NICE builds, for `make test`.
- `tools/screenshot.py`: stage 2's screen words as a PNG, for
  `make screenshot`.

## Status

Stages 1 and 2 are written and run in niceemu. They are not yet run on
the board. In niceemu:

| Stage | Iterations | Per point | Instructions | At 54 million a second |
|---|---|---|---|---|
| 1, 80×25 | 152 085 | 76 | 6 212 940 | ≈ 0.12 s |
| 2, 80×50 | 297 888 | 74 | 12 157 703 | ≈ 0.23 s |

That is about 41 instructions per iteration. The rate of 54 million
instructions a second on the MEGA65 is the one nice_chess measures.
Stage 2 also copies the character ROM into the font RAM before it
starts the clock, 8192 words.

## License

See [LICENSE](LICENSE).
