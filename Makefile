# nice_mandelbrot: the Mandelbrot set on the NICE CPU.
#
# The NICE toolchain comes from a checkout of nice_cpu, NICE_CPU, with
# niceemu built in it (make -C $(NICE_CPU)/emulator build).
# make help lists the targets.

NICE_CPU ?= ../nice_cpu
NICEEMU  ?= $(NICE_CPU)/emulator/zig-out/bin/niceemu
PYTHON   ?= python3
CC       ?= cc

NICEBUILD = $(PYTHON) $(NICE_CPU)/translator/nicebuild.py

# NICE: optimized for speed, without the bounds checks of wasm's memory
# accesses, which this program makes few of.
NICEFLAGS = -O 2 -D NDEBUG --no-bounds-checks --trust-alignment --symbols
# The MEGA65: the same, built for the monitor, which loads it at 0x8000.
MONFLAGS  = $(NICEFLAGS) --monitor
HOSTFLAGS = -O2 -W -Wall -Wextra

CORE    = src/mandel.c
HEADERS = src/mandel.h Makefile

.PHONY: help all test run window clean
help:
	@echo "nice_mandelbrot: the Mandelbrot set on the NICE CPU"
	@echo ""
	@echo "  make all     build stage 1 for niceemu, for the MEGA65, and for the host"
	@echo "  make test    run stage 1 in niceemu, and compare its screen with the host's"
	@echo "  make run     run stage 1 in niceemu, and print its screen in color"
	@echo "  make clean   remove build/"
	@echo ""
	@echo "  NICE_CPU=DIR the nice_cpu checkout (default ../nice_cpu)"
	@echo ""
	@echo "The MEGA65 build is build/mega65/stage1.bin, which NiceMonMEGA65's"
	@echo "monitor loads at 0x8000."

all: build/nice/stage1.bin build/mega65/stage1.bin build/host/mandel_host

build/nice/stage1.bin: src/stage1.c $(CORE) $(HEADERS)
	@mkdir -p $(@D)
	$(NICEBUILD) $(NICEFLAGS) -o $@ src/stage1.c $(CORE)

build/mega65/stage1.bin: src/stage1.c $(CORE) $(HEADERS)
	@mkdir -p $(@D)
	$(NICEBUILD) $(MONFLAGS) -o $@ src/stage1.c $(CORE)

build/host/mandel_host: src/host.c $(CORE) $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(HOSTFLAGS) -o $@ src/host.c $(CORE)

# nice_cpu's stand-in for a monitor at 0, which runs a program at 0x8000
# as the monitor does, so that the MEGA65 build runs in niceemu too,
# joined after it (a .bin file is address and word pairs).
build/nice/monstub.bin: $(NICE_CPU)/runtime/test/monitor.nasm
	@mkdir -p $(@D)
	$(PYTHON) $(NICE_CPU)/assembler/niceasm.py -o $@ $<

build/test/mon_stage1.bin: build/nice/monstub.bin build/mega65/stage1.bin
	@mkdir -p $(@D)
	cat $^ > $@

# The figures that the program writes to the console, and the screen
# words that niceemu dumps after the run.
build/test/stage1.out: build/nice/stage1.bin
	@mkdir -p $(@D)
	$(NICEEMU) -d 0xFFFF0000:2000 $< < /dev/null > $@

build/test/mon_stage1.out: build/test/mon_stage1.bin
	@mkdir -p $(@D)
	$(NICEEMU) -d 0xFFFF0000:2000 $< < /dev/null > $@

build/test/host.words: build/host/mandel_host
	@mkdir -p $(@D)
	$< --words > $@

# Each build's screen must be the host's, word for word, and its count
# of iterations the host's.
test: build/test/stage1.out build/test/mon_stage1.out build/test/host.words
	@for out in build/test/stage1.out build/test/mon_stage1.out; do \
	  grep '^ffff' $$out | cut -d: -f2 | tr ' ' '\n' | grep . > $$out.words; \
	  cmp $$out.words build/test/host.words || exit 1; \
	  grep -q "^iterations   $$(build/host/mandel_host --count)$$" $$out || \
	    { echo "$$out: the iterations differ from the host's"; exit 1; }; \
	done
	@grep -A4 '^nice_mandelbrot' build/test/stage1.out
	@echo "stage 1: both builds' screens and iterations match the host's"

run: build/nice/stage1.bin
	$(NICEEMU) --color $< < /dev/null

clean:
	rm -rf build
