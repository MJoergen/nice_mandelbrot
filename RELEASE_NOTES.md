# Release notes

The releases of nice_mandelbrot, newest first. Each section lists the
changes since the one before that a user of the file can see.
[docs/RELEASE.md](https://github.com/MJoergen/nice_mandelbrot/blob/main/docs/RELEASE.md)
says how a release is made.

A release is named `Vmajor.minor`, and its file is
`nice_mandelbrot_major.minor.bin` ([README.md](README.md#download)).
Each section names the nice_cpu tag the release is built with, and the
NiceMonMEGA65 release it was checked on.

## V1.1 (2026-11-15)

The Mandelbrot set at 160×100, twice as fine in each direction as V1.0,
with the same keys.

**What it needs**

- The monitor of NiceMonMEGA65 V2.1 or later, as V1.0. Copy
  `nice_mandelbrot_1.1.bin` into the folder `NICE` of the SD card, and
  type `load nice_mandelbrot_1.1.bin`, then `r`.

**What changed**

- The file is stage 3: 160×100 points, each 8×8 pixels, square, eight
  to a character cell, drawn with a font of the 128 octants in the font
  RAM ([#1](https://github.com/MJoergen/nice_mandelbrot/issues/1)).
  The whole set takes some 0.16 s on the MEGA65 (10.6 million
  instructions).
- A cell has only two colors. Where its eight points have more, near
  the border of the set, the others are drawn in the nearer of the two,
  so the picture is approximate there.
- A pan moves 16 points, still a tenth of the width. Each zoom shows the
  same part of the plane as in V1.0, and the deepest is 18 zooms in.

Built with nice_cpu V2.1. Checked on the MEGA65 under NiceMonMEGA65
V2.1.

## V1.0 (2026-11-15)

The first release: the Mandelbrot set at 80×50 on the MEGA65, with pan
and zoom.

**What it needs**

- The monitor of NiceMonMEGA65 V2.1 or later, whose `load` takes the
  file's long name. Copy `nice_mandelbrot_1.0.bin` into the folder
  `NICE` of the SD card, and type `load nice_mandelbrot_1.0.bin`, then
  `r`.

**What it does**

- The whole set, 80×50 points, each 16×16 pixels, drawn in some 0.05 s
  (2.4 million instructions in niceemu).
- The cursor keys pan by a tenth of the width; `+` and `-` zoom in and
  out, twice, about the middle, down to 19 zooms in; `0` shows the whole
  set again, and `q` returns to the monitor.
- After each picture, the iterations, cycles, instructions, and
  microseconds it took go to the monitor's line.
- The top right corner shows the name and the version.

Stages 3 and 4, at 160×50 and 160×100
([#1](https://github.com/MJoergen/nice_mandelbrot/issues/1)), are for
a later release.

Built with nice_cpu V2.1. Checked on the MEGA65 under NiceMonMEGA65
V2.1.

After the release, it also ran on the Nexys4DDR under NiceMonMEGA65
V2.1, from the SD card.
