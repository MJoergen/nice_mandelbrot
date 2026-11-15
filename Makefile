# nice_mandelbrot: the Mandelbrot set on the NICE CPU.
#
# The NICE toolchain comes from a checkout of nice_cpu, NICE_CPU, with
# niceemu built in it (make -C $(NICE_CPU)/emulator build).
# make help lists the targets.

NICE_CPU ?= ../nice_cpu
NICEEMU  ?= $(NICE_CPU)/emulator/zig-out/bin/niceemu
PYTHON   ?= python3
CC       ?= cc
STAGE    ?= 2

NICEBUILD = $(PYTHON) $(NICE_CPU)/translator/nicebuild.py

# NICE: optimized for speed, without the bounds checks of wasm's memory
# accesses, which this program makes few of.
NICEFLAGS = -O 2 -D NDEBUG --no-bounds-checks --trust-alignment --symbols
# The MEGA65: the same, built for the monitor, which loads it at 0x8000.
MONFLAGS  = $(NICEFLAGS) --monitor
HOSTFLAGS = -O2 -W -Wall -Wextra

STAGES  = 1 2
CORE    = src/mandel.c
NICELIB = src/report.c
HEADERS = src/mandel.h src/report.h Makefile

.PHONY: help all test run screenshot clean
.SECONDARY:
help:
	@echo "nice_mandelbrot: the Mandelbrot set on the NICE CPU"
	@echo ""
	@echo "  make all     build stages 1 (80x25) and 2 (80x50) for niceemu, for the MEGA65, and for the host"
	@echo "  make test    run both stages in niceemu, and compare their screens with the host's"
	@echo "  make run     run stage STAGE (default 2) in niceemu, and print its screen in color"
	@echo "  make screenshot  draw stage 2's screen in niceemu as docs/stage2.png (needs Pillow)"
	@echo "  make clean   remove build/"
	@echo ""
	@echo "  NICE_CPU=DIR the nice_cpu checkout (default ../nice_cpu)"
	@echo ""
	@echo "The MEGA65 builds are build/mega65/stageN.bin, which NiceMonMEGA65's"
	@echo "monitor loads at 0x8000."

all: $(foreach s,$(STAGES),build/nice/stage$(s).bin build/mega65/stage$(s).bin) \
     build/host/mandel_host

build/nice/stage%.bin: src/stage%.c $(CORE) $(NICELIB) $(HEADERS)
	@mkdir -p $(@D)
	$(NICEBUILD) $(NICEFLAGS) -o $@ $< $(CORE) $(NICELIB)

build/mega65/stage%.bin: src/stage%.c $(CORE) $(NICELIB) $(HEADERS)
	@mkdir -p $(@D)
	$(NICEBUILD) $(MONFLAGS) -o $@ $< $(CORE) $(NICELIB)

build/host/mandel_host: src/host.c $(CORE) $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(HOSTFLAGS) -o $@ src/host.c $(CORE)

# nice_cpu's stand-in for a monitor at 0, which runs a program at 0x8000
# as the monitor does, so that the MEGA65 builds run in niceemu too,
# joined after it (a .bin file is address and word pairs).
build/nice/monstub.bin: $(NICE_CPU)/runtime/test/monitor.nasm
	@mkdir -p $(@D)
	$(PYTHON) $(NICE_CPU)/assembler/niceasm.py -o $@ $<

build/test/mon_stage%.bin: build/nice/monstub.bin build/mega65/stage%.bin
	@mkdir -p $(@D)
	cat $^ > $@

# The figures that the program writes to the console, and the words
# that niceemu dumps after the run: the screen, then the glyph of stage
# 2, code 0x80 of the font RAM, and the font base.
DUMPS = -d 0xFFFF0000:2000 -d 0xFFFE1000:32 -d 0xFFFFFF86:1

build/test/stage%.out: build/nice/stage%.bin
	@mkdir -p $(@D)
	$(NICEEMU) $(DUMPS) $< < /dev/null > $@

build/test/mon_stage%.out: build/test/mon_stage%.bin
	@mkdir -p $(@D)
	$(NICEEMU) $(DUMPS) $< < /dev/null > $@

build/test/host%.words: build/host/mandel_host
	@mkdir -p $(@D)
	$< $* --words > $@

# The glyph of stage 2: rows 0 to 15 set, 16 to 31 clear, and the font
# RAM shown.
build/test/glyph.words:
	@mkdir -p $(@D)
	@for r in $$(seq 16); do echo 0000ffff; done > $@
	@for r in $$(seq 16); do echo 00000000; done >> $@
	@echo fffe0000 >> $@

# Each build's screen must be the host's, word for word, and its count
# of iterations the host's. Stage 2 must also have its glyph shown.
test: $(foreach s,$(STAGES),build/test/stage$(s).out build/test/mon_stage$(s).out \
      build/test/host$(s).words) build/test/glyph.words
	@for s in $(STAGES); do \
	  for out in build/test/stage$$s.out build/test/mon_stage$$s.out; do \
	    grep -E '^ff' $$out | cut -d: -f2 | tr ' ' '\n' | grep . > $$out.words; \
	    head -2000 $$out.words | cmp - build/test/host$$s.words || exit 1; \
	    if [ $$s = 2 ]; then tail -33 $$out.words | cmp - build/test/glyph.words || exit 1; fi; \
	    grep -q "^iterations   $$(build/host/mandel_host $$s --count)$$" $$out || \
	      { echo "$$out: the iterations differ from the host's"; exit 1; }; \
	  done; \
	  grep -A4 '^nice_mandelbrot' build/test/stage$$s.out; \
	done
	@echo "stages 1 and 2: both builds' screens and iterations match the host's"

run: build/nice/stage$(STAGE).bin
	$(NICEEMU) --color $< < /dev/null

# The picture of the README: stage 2's screen in niceemu, 1280x800.
screenshot: docs/stage2.png

docs/stage2.png: build/test/stage2.out tools/screenshot.py
	@mkdir -p $(@D)
	grep -E '^ff' $< | cut -d: -f2 | tr ' ' '\n' | grep . | $(PYTHON) tools/screenshot.py /dev/stdin $@

clean:
	rm -rf build
