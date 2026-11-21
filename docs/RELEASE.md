# Releasing nice_mandelbrot

The steps of a release, in order. Each is cheaper than the ones after
it, so a problem is found early. Do not go on while a step fails.
`tools/release.py` scripts the mechanical parts, which each step names.
The procedure is that of nice_chess, whose `release.py` and `board.py`
these are.

## Version names

A release is named `Vmajor.minor`: `V1.0`, `V1.1`, and so on, with a new
major version when a change asks for it. The names are nice_mandelbrot's
own, not those of nice_cpu or NiceMonMEGA65. Each release's notes name
the nice_cpu tag it is built with, and the NiceMonMEGA65 release it was
checked on and needs.

Stages 2 to 4 show the version in the top right corner of the screen,
`nice_mandelbrot V1.0`. A build of a tagged commit shows the tag. Other
builds show what `git describe` says, such as `V1.0-3-g55970b7`, with
`-dirty` for a changed tree. The release workflow sets `VERSION` to the
tag.

## What a release has

- An annotated tag, whose message is the tag's section of
  [RELEASE_NOTES.md](../RELEASE_NOTES.md).
- A GitHub release with that section as its text, and two files for
  NiceMonMEGA65's monitor: `nice_mandelbrot_1.2.bin`, stage 4, which
  needs the chunky mode of NiceMonMEGA65 V2.3 or later on the MEGA65,
  and `nice_mandelbrot_1.2_stage3.bin`, stage 3, for NiceMonMEGA65 V2.1
  or later and the Nexys4DDR. Their names are long names, which the
  monitor's `load` takes from the SD card from NiceMonMEGA65 V2.1 on.
  Up to V1.1, a release had one file, stage 2 in V1.0 and stage 3 in
  V1.1.

`make dist` makes them in `build/dist/`.

## What you need

- What `make test` needs: clang with the wasm32 target, `wasm-ld`,
  `llvm-ar`, a C compiler, Python 3, `curl`, and `make deps`
  ([README.md](../README.md#building-and-running)).
- `gh`, logged in to an account that may push to the repository.
- A MEGA65 under NiceMonMEGA65's monitor, at the release whose notes
  will be named, on its Ethernet port (`ETH=IP`) or its debug UART
  (`PORT=DEVICE`), and a checkout of NiceMonMEGA65 for its tools
  (`NICEMON=DIR`, `../NiceMonMEGA65` by default). Stage 4 needs the
  chunky mode of the screen, which NiceMonMEGA65 has from V2.3 on, so
  `make board-test` needs that monitor or a later one.

## 1. Prepare

Run `python3 tools/release.py preflight`. It checks items 1 and 3, and
lists the open issues for item 2.

1. The branch is `main`, the working tree is clean, and `HEAD` is
   `origin/main`.
2. No open [issue](https://github.com/MJoergen/nice_mandelbrot/issues)
   blocks the release, or the release notes say why it is acceptable.
3. The tools are on the PATH, `make deps` has fetched the pins, and the
   NiceMonMEGA65 checkout is there.

A raise of a pin, such as `NICE_CPU_TAG`, is a commit of its own before
the release, not part of it.

## 2. Documentation

1. [README.md](../README.md) and `make help` say what is true now, and
   agree with the Makefile.
2. The figures quoted (the iterations, the instructions, the times) are
   what the builds of this commit give.
3. The versions in README.md are those of the Makefile.
4. [RELEASE_NOTES.md](../RELEASE_NOTES.md) has a section for the new
   tag: what a user of the file can see changed since the last
   release, what it needs, and the nice_cpu tag. `release.py notes`
   drafts it from the pin, the stamp of step 4, and the commits; the
   text is yours. Before the tag, its heading gets the date of the
   release: `## V1.0 (2026-11-16)`. Documents may be committed after the
   stamp of step 4: `tag` accepts commits since the stamp that change
   only `*.md` files and `docs/`, such as `make screenshot VERSION=V1.0`
   for the picture of the README.

## 3. Quick checks

What CI runs ([test.yml](../.github/workflows/test.yml)), so a green run
on the commit is the same evidence, but run it locally too:

    make test

## 4. The board

    make release-check ETH=192.168.1.65     # or PORT=/dev/ttyUSB3

This runs `release.py check`: on a clean tree, `make board-test`, which
runs stages 2 to 4 on the board under the monitor, with no keys and
after the keys of `make test`, and compares their screens, their glyphs
or the mode, and their iterations with the host's, as `make test` does in niceemu. The logs
are in `build/release/`, and the runs' output and dumps in
`build/board/`. Then it writes `build/release/stamp.json` with the
commit, the link, and the NiceMonMEGA65 checkout's `git describe`.
`tag` needs it, and `make clean` deletes it. The checkout's name is the
tools', and not always the monitor's on the board, which its banner
shows at a cold start; the release notes name the monitor's.

The board runs the commit's build, whose version is `git describe`'s,
not the tag. It differs from the release's only in that string, which
the host's words have too.

Also look at the picture on the board's screen: `make board-run`, then
pan and zoom with the keys, and `make board-run BOARD_STAGE=3`.

## 5. CI

Push, and wait for the job of `test.yml` to pass on that commit:

    python3 tools/release.py ci --wait

## 6. Tag

    python3 tools/release.py tag            # or tag V2.0 for a new major version

It refuses unless `HEAD` is clean and is `origin/main`, the name is
`Vmajor.minor` and comes after the last tag, the stamp of step 4 covers
`HEAD`, RELEASE_NOTES.md has a section for the tag with a dated heading,
and CI passed. It takes the next minor version, or `V1.0` for the first,
unless given a name. It shows the section, asks before it makes the
annotated tag with the section as its message, and asks again before it
pushes. By hand:

    git tag -a --cleanup=whitespace V1.0 -m "<the section>"
    git push origin V1.0

## 7. The release workflow

The push of the tag starts
[release.yml](../.github/workflows/release.yml). With `VERSION` set to
the tag, it runs `make test`, then `make dist`, and creates the GitHub
release with the section as its text, and the files. Wait for it, and
look at the release:

    gh run watch $(gh run list --workflow release.yml --limit 1 --json databaseId -q '.[0].databaseId')
    gh release view V1.2

If it failed on a passing cause, such as the network, rerun it with
`gh run rerun`. Otherwise run `make dist VERSION=V1.2` on a checkout of
the tag, and create the release by hand, as release.yml does:

    python3 tools/release.py section V1.2 > section.md
    tail -n +2 section.md > notes.md
    gh release create V1.2 --verify-tag --title "$(head -n 1 section.md)" \
       --notes-file notes.md build/dist/nice_mandelbrot_1.2.bin \
       build/dist/nice_mandelbrot_1.2_stage3.bin

## 8. The files

Try the files as a user gets them: download them, copy them into the
folder `NICE` of the SD card, and on the MEGA65 type `load
nice_mandelbrot_1.2.bin`, then `r`, and the same for
`nice_mandelbrot_1.2_stage3.bin`. The top right corner must say
`NICE_MANDELBROT V1.2` in stage 4, and `nice_mandelbrot V1.2` in
stage 3.

    gh release download V1.2 --dir build/release/V1.2

Without the card at hand, `tools/load.py` of NiceMonMEGA65 loads them
over the link:

    python3 ../NiceMonMEGA65/tools/load.py --eth 192.168.1.65 --monitor \
        build/release/V1.2/nice_mandelbrot_1.2.bin

A tag is never moved, and a release is not deleted. A fix is the next
release, or the release notes say what is wrong.
