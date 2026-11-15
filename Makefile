# nice_mandelbrot: the Mandelbrot set on the NICE CPU.
#
# The NICE toolchain comes from nice_cpu at a pinned release tag, and
# niceemu from that release's files. make deps fetches them into deps/.
# NICE_CPU=DIR and NICEEMU=PATH use local copies instead.
# make help lists the targets.

NICE_CPU_TAG          := V2.1
NICEEMU_SHA256        := 4d40fef5ef9a179f803e7e4aba8e26fadd008760bc7c5c4471d525ddb29a0ece
NICEEMU_WINDOW_SHA256 := ccabcbfec7edee71f64a529364ba11a9693186f3a922be7cdcb5e448d8bb3f25

NICE_CPU       ?= deps/nice_cpu-$(NICE_CPU_TAG)
NICEEMU        ?= deps/niceemu-$(NICE_CPU_TAG)-x86_64-linux/niceemu
NICEEMU_WINDOW ?= deps/niceemu-window-$(NICE_CPU_TAG)-x86_64-linux/niceemu
NICEMON        ?= ../NiceMonMEGA65
PYTHON         ?= python3
CC             ?= cc
STAGE          ?= 2
# The version in the corner of stage 2: the release tag on a tagged
# commit, such as V1.0, and git describe's name of the commit between
# tags. The release workflow sets it to the tag.
VERSION ?= $(shell git describe --tags --match 'V[0-9]*' --always --dirty 2>/dev/null || echo unknown)
export NICEMON

# How to reach the board: ETH=IP or PORT=DEVICE, as NiceMonMEGA65's tools
# take them. Only the board targets need one.
BOARDLINK = $(if $(ETH),--eth $(ETH),$(if $(PORT),--port $(PORT),$(error ETH=IP or PORT=DEVICE names the board)))

NICEBUILD   = $(PYTHON) $(NICE_CPU)/translator/nicebuild.py
RELEASE_URL = https://github.com/MJoergen/nice_cpu/releases/download/$(NICE_CPU_TAG)

# NICE: optimized for speed, without the bounds checks of wasm's memory
# accesses, which this program makes few of.
NICEFLAGS = -O 2 -D NDEBUG --no-bounds-checks --trust-alignment --symbols
# The MEGA65: the same, built for the monitor, which loads it at 0x8000.
MONFLAGS  = $(NICEFLAGS) --monitor
HOSTFLAGS = -O2 -W -Wall -Wextra

STAGES   = 1 2
VERSIONC = build/gen/version.c
CORE     = src/mandel.c src/view.c src/label.c $(VERSIONC)
NICELIB  = src/report.c
HEADERS  = src/mandel.h src/view.h src/label.h src/report.h Makefile

.PHONY: FORCE help all test run explore screenshot board-run board-test release-check dist deps clean distclean
.SECONDARY:
help:
	@echo "nice_mandelbrot: the Mandelbrot set on the NICE CPU"
	@echo ""
	@echo "  make deps    fetch nice_cpu $(NICE_CPU_TAG) and niceemu $(NICE_CPU_TAG) into deps/"
	@echo "  make all     build stages 1 (80x25) and 2 (80x50) for niceemu, for the MEGA65, and for the host"
	@echo "  make test    run both stages in niceemu, and compare their screens with the host's,"
	@echo "               also after a series of keys in stage 2"
	@echo "  make run     run stage STAGE (default 2) in niceemu, and print its screen in color"
	@echo "  make explore run stage 2 in niceemu's window, to pan and zoom with the keys"
	@echo "               (fetches niceemu's window build into deps/)"
	@echo "  make screenshot  draw stage 2's screen in niceemu as docs/stage2.png (needs Pillow)"
	@echo "  make dist    the file of a release, build/dist/nice_mandelbrot_1.0.bin (VERSION=V1.0)"
	@echo "  make clean   remove build/"
	@echo "  make distclean   remove build/ and deps/"
	@echo ""
	@echo "The board (ETH=IP or PORT=DEVICE, and NICEMON=DIR, a checkout of NiceMonMEGA65,"
	@echo "default ../NiceMonMEGA65):"
	@echo "  make board-run      run stage 2 on the MEGA65 under the monitor"
	@echo "  make board-test     the tests of stage 2 on the board, its screens against the host's"
	@echo "  make release-check  board-test, and the stamp that tools/release.py tag needs (docs/RELEASE.md)"
	@echo ""
	@echo "  NICE_CPU=DIR and NICEEMU=PATH use local copies of nice_cpu and niceemu."
	@echo ""
	@echo "The MEGA65 builds are build/mega65/stageN.bin, which NiceMonMEGA65's"
	@echo "monitor loads at 0x8000."

# The NICE builds depend on the toolchain: its directory and commit, in
# a file that changes only when they do, so that a new pin or another
# NICE_CPU=DIR builds them again.
TOOLCHAIN = build/toolchain.txt
$(TOOLCHAIN): FORCE | $(NICE_CPU)
	@mkdir -p $(@D)
	@t="$(abspath $(NICE_CPU)) $$(git -C $(NICE_CPU) rev-parse HEAD 2>/dev/null)"; \
	  [ "$$t" = "$$(cat $@ 2>/dev/null)" ] || echo "$$t" > $@

# The version, in a file that changes only when it does.
$(VERSIONC): FORCE
	@mkdir -p $(@D)
	@t='const char nice_mandelbrot_version[] = "$(VERSION)";'; \
	  [ "$$t" = "$$(cat $@ 2>/dev/null)" ] || echo "$$t" > $@

FORCE:

all: $(foreach s,$(STAGES),build/nice/stage$(s).bin build/mega65/stage$(s).bin) \
     build/host/mandel_host

build/nice/stage%.bin: src/stage%.c $(CORE) $(NICELIB) $(HEADERS) $(TOOLCHAIN)
	@mkdir -p $(@D)
	$(NICEBUILD) $(NICEFLAGS) -o $@ $< $(CORE) $(NICELIB)

build/mega65/stage%.bin: src/stage%.c $(CORE) $(NICELIB) $(HEADERS) $(TOOLCHAIN)
	@mkdir -p $(@D)
	$(NICEBUILD) $(MONFLAGS) -o $@ $< $(CORE) $(NICELIB)

build/host/mandel_host: src/host.c $(CORE) $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(HOSTFLAGS) -o $@ src/host.c $(CORE)

# nice_cpu's stand-in for a monitor at 0, which runs a program at 0x8000
# as the monitor does, so that the MEGA65 builds run in niceemu too,
# joined after it (a .bin file is address and word pairs).
build/nice/monstub.bin: $(TOOLCHAIN)
	@mkdir -p $(@D)
	$(PYTHON) $(NICE_CPU)/assembler/niceasm.py -o $@ $(NICE_CPU)/runtime/test/monitor.nasm

build/test/mon_stage%.bin: build/nice/monstub.bin build/mega65/stage%.bin
	@mkdir -p $(@D)
	cat $^ > $@

# The figures that the program writes to the console, and the words
# that niceemu dumps after the run: the screen, the font RAM, and the
# font base. tools/words.py takes words out of them by their address.
DUMPS = -d 0xFFFF0000:2000 -d 0xFFFE0000:8192 -d 0xFFFFFF86:1
WORDS = $(PYTHON) tools/words.py

build/test/stage%.out: build/nice/stage%.bin | $(NICEEMU)
	@mkdir -p $(@D)
	$(NICEEMU) $(DUMPS) $< < /dev/null > $@

build/test/mon_stage%.out: build/test/mon_stage%.bin | $(NICEEMU)
	@mkdir -p $(@D)
	$(NICEEMU) $(DUMPS) $< < /dev/null > $@

build/test/host%.words: build/host/mandel_host
	@mkdir -p $(@D)
	$< $* --words > $@

# The glyph of stage 2, code 0x80 of the font RAM: rows 0 to 15 set,
# 16 to 31 clear. Then the font base, which shows the font RAM.
build/test/glyph.words:
	@mkdir -p $(@D)
	@for r in $$(seq 16); do echo 0000ffff; done > $@
	@for r in $$(seq 16); do echo 00000000; done >> $@
	@echo fffe0000 >> $@

# Stage 2 after a series of keys: pan left and up (both forms of the
# cursor keys), zoom in four times, out twice, and in again (both keys
# for each), and F9 between, which does nothing. The screen is that of the view
# after the last key, as each key that moves the view starts a new
# picture.
KEYS = \033[D==\033[A\033OA+=-\033[20~_=

build/test/keys_stage%.out: build/nice/stage%.bin | $(NICEEMU)
	@mkdir -p $(@D)
	printf '$(KEYS)' | $(NICEEMU) $(DUMPS) $< > $@

build/test/keys_mon_stage%.out: build/test/mon_stage%.bin | $(NICEEMU)
	@mkdir -p $(@D)
	printf '$(KEYS)' | $(NICEEMU) $(DUMPS) $< > $@

build/test/keys_host%.words: build/host/mandel_host
	@mkdir -p $(@D)
	$< $* --keys "$$(printf '$(KEYS)')" --words > $@

# check STAGE OUT DUMP HOST [KEYS]: the screen of the run's DUMP must be
# the host's words HOST, word for word, and the last count of iterations
# in its output OUT the host's, after the keys KEYS. Stage 2 must also
# have its glyph shown.
define check
	$(WORDS) $(3) 0xFFFF0000 2000 | cmp - $(4)
	[ $(1) != 2 ] || { $(WORDS) $(3) 0xFFFE1000 32; $(WORDS) $(3) 0xFFFFFF86 1; } | cmp - build/test/glyph.words
	[ "$$(grep '^iterations' $(2) | tail -1)" = \
	  "iterations   $$(build/host/mandel_host $(1) $(if $(5),--keys "$$(printf '$(5)')") --count)" ] || \
	  { echo "$(2): the iterations differ from the host's"; exit 1; }
endef

test: $(foreach s,$(STAGES),build/test/stage$(s).out build/test/mon_stage$(s).out \
      build/test/host$(s).words) build/test/glyph.words \
      build/test/keys_stage2.out build/test/keys_mon_stage2.out build/test/keys_host2.words
	@$(foreach s,$(STAGES),$(foreach o,stage$(s) mon_stage$(s), \
	  $(call check,$(s),build/test/$(o).out,build/test/$(o).out,build/test/host$(s).words)$(newline)))
	@$(foreach o,keys_stage2 keys_mon_stage2, \
	  $(call check,2,build/test/$(o).out,build/test/$(o).out,build/test/keys_host2.words,$(KEYS))$(newline))
	@for s in $(STAGES); do grep -A4 '^nice_mandelbrot' build/test/stage$$s.out; done
	@echo "stage 2 after keys:"
	@grep -A4 '^nice_mandelbrot' build/test/keys_stage2.out | tail -5
	@echo "stages 1 and 2: both builds' screens and iterations match the host's, also after keys"

define newline


endef

run: build/nice/stage$(STAGE).bin | $(NICEEMU)
	$(NICEEMU) --color $< < /dev/null

# Stage 2 in niceemu's window, with the keys typed into it.
explore: build/nice/stage2.bin | $(NICEEMU_WINDOW)
	$(NICEEMU_WINDOW) --window $<

# The picture of the README: stage 2's screen in niceemu, 1280x800.
screenshot: docs/stage2.png

docs/stage2.png: build/test/stage2.out tools/screenshot.py tools/words.py
	@mkdir -p $(@D)
	$(PYTHON) tools/screenshot.py $< $@

# The board. tools/board.py loads the MEGA65 build under the monitor,
# types r, which runs it, and is its console; it reads the screen and
# the font once the program is quiet, and then ends its input.
BOARD = $(PYTHON) tools/board.py $(BOARDLINK)

board-run: build/mega65/stage2.bin
	$(PYTHON) $(NICEMON)/tools/load.py $(BOARDLINK) --monitor $<

# Stage 2 on the board, as make test runs the MEGA65 build in niceemu:
# with no keys, and after the keys of KEYS.
board-test: build/mega65/stage2.bin build/test/host2.words build/test/keys_host2.words build/test/glyph.words
	@mkdir -p build/board
	$(BOARD) --dump build/board/stage2.dump $< < /dev/null > build/board/stage2.out
	@$(call check,2,build/board/stage2.out,build/board/stage2.dump,build/test/host2.words)
	printf '$(KEYS)' | $(BOARD) --dump build/board/keys_stage2.dump $< > build/board/keys_stage2.out
	@$(call check,2,build/board/keys_stage2.out,build/board/keys_stage2.dump,build/test/keys_host2.words,$(KEYS))
	@grep -A4 '^nice_mandelbrot' build/board/stage2.out
	@echo "stage 2 after keys:"
	@grep -A4 '^nice_mandelbrot' build/board/keys_stage2.out | tail -5
	@echo "stage 2 on the board: its screens and iterations match the host's, also after keys"

# The gate of a release on the board, and its stamp (tools/release.py).
release-check:
	$(PYTHON) tools/release.py check $(BOARDLINK)

# A release (docs/RELEASE.md): stage 2 for the MEGA65, under the
# monitor, as nice_mandelbrot_MAJOR.MINOR.bin, a long name, which the
# monitor's load takes from NiceMonMEGA65 V2.1 on. The release workflow
# runs it with VERSION set to the tag, after the tests.
DIST      = build/dist
DIST_FILE = $(DIST)/nice_mandelbrot_$(patsubst V%,%,$(VERSION)).bin
dist: build/mega65/stage2.bin
	rm -rf $(DIST)
	mkdir -p $(DIST)
	cp $< $(DIST_FILE)
	@echo "$(DIST_FILE)"

deps: $(NICE_CPU) $(NICEEMU)

deps/nice_cpu-$(NICE_CPU_TAG):
	git clone --quiet --depth 1 --branch $(NICE_CPU_TAG) -c advice.detachedHead=false \
	  https://github.com/MJoergen/nice_cpu $@

# fetch NAME SHA256: a tar.gz of the nice_cpu release, checked, into deps/.
define fetch
	@mkdir -p deps
	curl -sSfL -o deps/$(1).tar.gz $(RELEASE_URL)/$(1).tar.gz
	echo "$(2)  deps/$(1).tar.gz" | sha256sum -c --quiet
	tar xzf deps/$(1).tar.gz -C deps
	rm deps/$(1).tar.gz
	@touch $@
endef

deps/niceemu-$(NICE_CPU_TAG)-x86_64-linux/niceemu:
	$(call fetch,niceemu-$(NICE_CPU_TAG)-x86_64-linux,$(NICEEMU_SHA256))

deps/niceemu-window-$(NICE_CPU_TAG)-x86_64-linux/niceemu:
	$(call fetch,niceemu-window-$(NICE_CPU_TAG)-x86_64-linux,$(NICEEMU_WINDOW_SHA256))

clean:
	rm -rf build

distclean: clean
	rm -rf deps
