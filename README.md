# nice_mandelbrot

The classic Mandelbrot rendering, on the NICE CPU.

The program runs on [NiceMonMEGA65](https://github.com/MJoergen/NiceMonMEGA65),
the [NICE CPU](https://github.com/MJoergen/nice_cpu) on the MEGA65, and in
`niceemu`. It is written in C first, and later in NICE BASIC, the
interpreter planned for the ROM of NiceMonMEGA65.

![Stage 2, the Mandelbrot set at 80×50](docs/stage2.png)

Stage 2 at 80×50, 1280×800 pixels, drawn from the screen that niceemu
dumps after the run, with the name and the version in the top right
corner.

## Download

Each [release](https://github.com/MJoergen/nice_mandelbrot/releases)
has one file, stage 2 for the MEGA65, such as
`nice_mandelbrot_1.0.bin` of V1.0. It needs the monitor of
NiceMonMEGA65 V2.1 or later. Copy it into the folder `NICE` of the SD
card, and at the monitor's prompt type

    load nice_mandelbrot_1.0.bin
    r

[RELEASE_NOTES.md](RELEASE_NOTES.md) lists what each release changed,
and [docs/RELEASE.md](docs/RELEASE.md) says how a release is made.

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
(MPU, nice_cpu's SYSTEM.md §2), shifted back. Fixed point keeps the
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
(nice_cpu's SYSTEM.md §1.8), a blank in the character ROM. Each cell
is that glyph, in the top point's color as foreground, on the bottom
point's color as background. A cell has two points and two colors, so
the 80×50 image is exact everywhere.

### Colors in the 2×2 font

The 2×2 font has 2⁴ = 16 glyphs, one for each pattern of set blocks,
written to the font RAM (nice_cpu's SYSTEM.md §1.8). A set block is
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
  cardioid and the period-2 bulb, before the loop, skips most of them.
  That turned out to be more than the half guessed here: the C programs
  have the test, and run the whole view some 5 times faster (Status,
  below).
- Drawing is not counted: 8000 screen writes, and the choice of two
  colors for each of 2000 cells, take a few milliseconds at most.
- The figures are to be checked against the performance counters
  (SYSTEM.md §3), which count cycles and microseconds on the board.

## Building and running

Stages 1 and 2 build with the toolchain of
[nice_cpu](https://github.com/MJoergen/nice_cpu) V2.1, which `make deps`
fetches into `deps/`, with niceemu from that release's files, and need
clang with the wasm32 target, `wasm-ld`, and `llvm-ar`. `NICE_CPU=DIR`
and `NICEEMU=PATH` use a local checkout and niceemu instead.

    make deps    # nice_cpu V2.1 and niceemu V2.1 into deps/
    make all     # build/nice/stageN.bin for niceemu, build/mega65/stageN.bin
                 # for the MEGA65's monitor at 0x8000, build/host/mandel_host
    make test    # both NICE builds of each stage in niceemu, their screens
                 # compared with the host's, also after a series of keys
    make run     # stage 2 in niceemu, its screen printed in color (STAGE=1 for stage 1)
    make explore # stage 2 in niceemu's window, to pan and zoom (fetches niceemu's window build)
    make screenshot  # docs/stage2.png, stage 2's screen in niceemu (needs Pillow)
    make dist    # build/dist/nice_mandelbrot_1.0.bin, the file of a release (VERSION=V1.0)

On the MEGA65 under NiceMonMEGA65's monitor, with a checkout of
NiceMonMEGA65 for its tools (`NICEMON=DIR`, `../NiceMonMEGA65` by
default), and the board on its Ethernet port (`ETH=IP`) or its debug
UART (`PORT=DEVICE`):

    make board-run       # stage 2 on the board
    make board-test      # stage 2 on the board, its screens compared with the host's
    make release-check   # board-test, and the stamp of a release (docs/RELEASE.md)

Each program draws the picture a row of cells at a time, then writes
the iterations, cycles, instructions, and microseconds it took to the
console's line, and waits for a key. In stage 1, any key clears the
screen and returns. At the end of the console's input, as in niceemu,
it returns at once.

### Pan and zoom in stage 2

Stage 2 takes these keys:

| Key | Action |
|---|---|
| ← → ↑ ↓ | Pan by 8 points, a tenth of the width |
| `+` or `=` | Zoom in, twice, about the middle |
| `-` or `_` | Zoom out, twice |
| `0` | The whole set again |
| `q` | Show the character ROM again, clear the screen, and return |

The cursor keys are the xterm sequences `ESC [ A` to `ESC [ D`, which
niceemu's window and the MEGA65's keyboard send, and also `ESC O A` to
`ESC O D`. Other sequences, such as those of the function keys, do
nothing. After each row of cells the program reads the keys that wait,
and a key that moves the view starts the new picture at once. So a key
held down does not queue up pictures, and only whole pictures write
their figures.

Each zoom in halves the step between points, from 3/80 at the start
down to 2⁻²⁴, after 19 zooms, where the points are a step of the fixed
point apart. Each also adds 64 to the highest count, 256 at the start,
so that the border of the set keeps its detail: 1472 at the deepest. One
zoom out from the start is allowed, to a width of 6, and the view stays
inside −4 to 4 on both axes, where the fixed point cannot overflow
(`src/mandel.h`).

Deep views take longer. The points of the main cardioid and the
period-2 bulb cost nothing (Status, below), but a view all inside
another part of the set, such as a small copy of it, at the deepest zoom
is 5.9 million iterations, some 4 s on the board. A key still takes
effect after the row of cells being drawn, some 0.2 s.

The files:

- `src/mandel.c`, `src/mandel.h`: the iteration, in fixed point with 25
  fraction bits, and the colors. On NICE each product is one of the math
  unit, through `nice_poke` and `nice_peek`; on the host it is a 64-bit
  product in C, with the same bits.
- `src/view.c`, `src/view.h`: the view of stage 2, and the keys that
  pan and zoom it, for NICE and for the host.
- `src/label.c`, `src/label.h`: the name and the version in the top
  right corner of stage 2, for NICE and for the host. The version is
  `git describe`'s, in `build/gen/version.c`, or the tag's in a
  release.
- `src/stage1.c`, `src/stage2.c`: stages 1 and 2, for NICE.
- `src/report.c`, `src/report.h`: the figures of a run, written to the
  console's register, so that under the monitor they go to the line and
  not over the picture.
- `src/host.c`: stages 1 and 2 on the host, as characters, or as the
  screen words of the NICE builds, for `make test`. `--keys` applies a
  series of keys to the view of stage 2 first.
- `tools/screenshot.py`: stage 2's screen as a PNG, its cells drawn
  from the font RAM, for `make screenshot`.
- `tools/words.py`: the words of a dump of niceemu or the board, by
  address, for the tests.
- `tools/board.py`: a program under the monitor on the board, as its
  console, with the screen dumped as niceemu dumps it, for
  `make board-test` (from nice_chess).
- `tools/release.py`: the scripted steps of a release (from
  nice_chess), [docs/RELEASE.md](docs/RELEASE.md).

## Status

Stages 1 and 2 are written and run in niceemu, stage 2 with pan and
zoom. Stage 2 is the file of a release; `make board-test` runs it on
the board. Stage 3 and the NICE BASIC version are not written yet. In
niceemu, stage 2 with the label of V1.0:

| Stage | Count | Per point | Instructions | At 54 million a second | Without the test |
|---|---|---|---|---|---|
| 1, 80×25 | 152 085 | 76 | 1 252 824 | ≈ 0.023 s | 6 077 411 |
| 2, 80×50 | 297 888 | 74 | 2 366 125 | ≈ 0.044 s | 11 937 238 |

The count is the total of the points' counts, as the programs report it
under `iterations`: a point in the main cardioid or the period-2 bulb
counts as 256, though the test before the loop
(`src/mandel.c`) gives it at once. Without that test the programs run
about 40 instructions per iteration, and the test makes them some 5
times faster, with the same picture, word for word. In the view of
`make test` after the keys, near the border at zoom level 3, it is 15
times faster: 3 616 517 instructions, from 55 509 527. The rate of 54
million instructions a second on the MEGA65 is the one nice_chess
measures.
Stage 2 also copies the character ROM into the font RAM before it
starts the clock, 8192 words. Its label in the top right corner costs
some 20 000 instructions a picture.

## License

See [LICENSE](LICENSE).
