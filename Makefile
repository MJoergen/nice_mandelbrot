# nice_mandelbrot: the Mandelbrot set on the NICE CPU.
#
# The NICE toolchain comes from nice_cpu at a pinned release tag, and
# niceemu from that release's files. make deps fetches them into deps/.
# NICE_CPU=DIR and NICEEMU=PATH use local copies instead.
# make help lists the targets.

NICE_CPU_TAG          := V2.3
NICEEMU_SHA256        := f1c5b0e68ba9841ccd5b0b9e5cfcbff6946fa02e06316751dc5db24283b41404
NICEEMU_WINDOW_SHA256 := 688387dd6ea9484f526a53bbba22d4f6a829c7d3b8d3d68f2cb4ce5c4f312a1c

NICE_CPU       ?= deps/nice_cpu-$(NICE_CPU_TAG)
NICEEMU        ?= deps/niceemu-$(NICE_CPU_TAG)-x86_64-linux/niceemu
NICEEMU_WINDOW ?= deps/niceemu-window-$(NICE_CPU_TAG)-x86_64-linux/niceemu
NICEMON        ?= ../NiceMonMEGA65
PYTHON         ?= python3
CC             ?= cc
STAGE          ?= 2
# The version in the corner of stages 2 to 4: the release tag on a tagged
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

STAGES   = 1 2 3 4
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
	@echo "  make all     build stages 1 (80x25), 2 (80x50), 3 (160x100 in octants), and"
	@echo "               4 (160x100 in the chunky mode) for niceemu, for the MEGA65, and for the host"
	@echo "  make test    run the stages in niceemu, and compare their screens with the host's,"
	@echo "               also after a series of keys in stages 2 to 4"
	@echo "  make run     run stage STAGE (default 2) in niceemu, and print its screen in color"
	@echo "  make explore run stage STAGE (2 to 4) in niceemu's window, to pan and zoom with the keys"
	@echo "               (fetches niceemu's window build into deps/)"
	@echo "  make screenshot  draw the screens of stages 2 to 4 in niceemu as docs/stageN.png"
	@echo "               (needs Pillow)"
	@echo "  make dist    the files of a release in build/dist/: nice_mandelbrot_1.2.bin, stage 4,"
	@echo "               and nice_mandelbrot_1.2_stage3.bin (VERSION=V1.2)"
	@echo "  make clean   remove build/"
	@echo "  make distclean   remove build/ and deps/"
	@echo ""
	@echo "The board (ETH=IP or PORT=DEVICE, and NICEMON=DIR, a checkout of NiceMonMEGA65,"
	@echo "default ../NiceMonMEGA65):"
	@echo "  make board-run      run stage BOARD_STAGE (default 4) on the MEGA65 under the monitor"
	@echo "  make board-test     the tests of stages 2 to 4 on the board, their screens against the host's"
	@echo "                      (stage 4 needs NiceMonMEGA65 V2.3 on the MEGA65)"
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
# that niceemu dumps after the run: the screen, 16000 words for the
# chunky mode of stage 4, the font RAM, the font base, and the mode
# word. tools/words.py takes words out of them by their address.
DUMPS = -d 0xFFFF0000:16000 -d 0xFFFE0000:8192 -d 0xFFFFFF86:1 -d 0xFFFFFF87:1
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

# The other words that stages 2 to 4 write for their screens, as
# ADDR:N, which the host's --setup prints: in stages 2 and 3 the glyphs
# in the font RAM from code 0x80, then the font base, which shows the
# font RAM, the upper half block of stage 2 and the 128 octants of stage
# 3, and in stage 4 the mode word, the chunky mode. The screen is 2000
# words, and 16000 in the chunky mode.
build/test/setup%.words: build/host/mandel_host
	@mkdir -p $(@D)
	$< $* --setup > $@
SETUP_2 = 0xFFFE1000:32 0xFFFFFF86:1
SETUP_3 = 0xFFFE1000:4096 0xFFFFFF86:1
SETUP_4 = 0xFFFFFF87:1
SCREEN_WORDS_1 = 2000
SCREEN_WORDS_2 = 2000
SCREEN_WORDS_3 = 2000
SCREEN_WORDS_4 = 16000

# Stages 2 to 4 after a series of keys: pan left and up (both forms of the
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
# in its output OUT the host's, after the keys KEYS. Stages 2 to 4
# must also have the words of their setup, the glyphs shown or the mode.
define check
	$(WORDS) $(3) 0xFFFF0000 $(SCREEN_WORDS_$(1)) | cmp - $(4)
	[ $(1) = 1 ] || for r in $(SETUP_$(1)); do $(WORDS) $(3) $${r%:*} $${r#*:} || exit 1; done | \
	  cmp - build/test/setup$(1).words
	[ "$$(grep '^iterations' $(2) | tail -1)" = \
	  "iterations   $$(build/host/mandel_host $(1) $(if $(5),--keys "$$(printf '$(5)')") --count)" ] || \
	  { echo "$(2): the iterations differ from the host's"; exit 1; }
endef

KEY_STAGES = 2 3 4

test: $(foreach s,$(STAGES),build/test/stage$(s).out build/test/mon_stage$(s).out \
      build/test/host$(s).words) \
      $(foreach s,$(KEY_STAGES),build/test/setup$(s).words build/test/keys_stage$(s).out \
      build/test/keys_mon_stage$(s).out build/test/keys_host$(s).words)
	@$(foreach s,$(STAGES),$(foreach o,stage$(s) mon_stage$(s), \
	  $(call check,$(s),build/test/$(o).out,build/test/$(o).out,build/test/host$(s).words)$(newline)))
	@$(foreach s,$(KEY_STAGES),$(foreach o,keys_stage$(s) keys_mon_stage$(s), \
	  $(call check,$(s),build/test/$(o).out,build/test/$(o).out,build/test/keys_host$(s).words,$(KEYS))$(newline)))
	@for s in $(STAGES); do grep -A4 '^nice_mandelbrot' build/test/stage$$s.out; done
	@for s in $(KEY_STAGES); do echo "stage $$s after keys:"; \
	  grep -A4 '^nice_mandelbrot' build/test/keys_stage$$s.out | tail -5; done
	@echo "stages 1 to 4: both builds' screens and iterations match the host's, also after keys"

define newline


endef

run: build/nice/stage$(STAGE).bin | $(NICEEMU)
	$(NICEEMU) --color $< < /dev/null

# Stage 2, 3, or 4 in niceemu's window, with the keys typed into it.
explore: build/nice/stage$(STAGE).bin | $(NICEEMU_WINDOW)
	$(NICEEMU_WINDOW) --window $<

# The pictures of the README: the screens of stages 2 to 4 in
# niceemu, 1280x800.
screenshot: docs/stage2.png docs/stage3.png docs/stage4.png

docs/stage%.png: build/test/stage%.out tools/screenshot.py tools/words.py
	@mkdir -p $(@D)
	$(PYTHON) tools/screenshot.py $< $@

# The board. tools/board.py loads the MEGA65 build under the monitor,
# types r, which runs it, and is its console; it reads the screen and
# the font once the program is quiet, and then ends its input.
BOARD = $(PYTHON) tools/board.py $(BOARDLINK)

BOARD_STAGE ?= 4
board-run: build/mega65/stage$(BOARD_STAGE).bin
	$(PYTHON) $(NICEMON)/tools/load.py $(BOARDLINK) --monitor $<

# Stages 2 to 4 on the board, as make test runs the MEGA65 builds in
# niceemu: with no keys, and after the keys of KEYS. Stage 4 needs the
# chunky mode, which NiceMonMEGA65 has from V2.3 on, on the MEGA65.
BOARD_STAGES = 2 3 4

# board_check STAGE: the two runs of stage STAGE on the board, checked.
define board_check
	$(BOARD) --dump build/board/stage$(1).dump build/mega65/stage$(1).bin < /dev/null > build/board/stage$(1).out
	@$(call check,$(1),build/board/stage$(1).out,build/board/stage$(1).dump,build/test/host$(1).words)
	printf '$(KEYS)' | $(BOARD) --dump build/board/keys_stage$(1).dump build/mega65/stage$(1).bin > build/board/keys_stage$(1).out
	@$(call check,$(1),build/board/keys_stage$(1).out,build/board/keys_stage$(1).dump,build/test/keys_host$(1).words,$(KEYS))
	@grep -A4 '^nice_mandelbrot' build/board/stage$(1).out
	@echo "stage $(1) after keys:"
	@grep -A4 '^nice_mandelbrot' build/board/keys_stage$(1).out | tail -5
endef

board-test: $(foreach s,$(BOARD_STAGES),build/mega65/stage$(s).bin build/test/host$(s).words \
            build/test/keys_host$(s).words build/test/setup$(s).words)
	@mkdir -p build/board
	$(foreach s,$(BOARD_STAGES),$(call board_check,$(s))$(newline))
	@echo "stages 2 to 4 on the board: their screens and iterations match the host's, also after keys"

# The gate of a release on the board, and its stamp (tools/release.py).
release-check:
	$(PYTHON) tools/release.py check $(BOARDLINK)

# A release (docs/RELEASE.md): two files for the MEGA65, under the
# monitor, with long names, which the monitor's load takes from
# NiceMonMEGA65 V2.1 on. nice_mandelbrot_MAJOR.MINOR.bin is stage 4, which
# needs the chunky mode of NiceMonMEGA65 V2.3 on the MEGA65, and
# nice_mandelbrot_MAJOR.MINOR_stage3.bin is stage 3, for older monitors
# and the Nexys4DDR. The release workflow runs it with VERSION set to the
# tag, after the tests.
DIST        = build/dist
DIST_NAME   = $(DIST)/nice_mandelbrot_$(patsubst V%,%,$(VERSION))
DIST_FILES  = $(DIST_NAME).bin $(DIST_NAME)_stage3.bin
dist: build/mega65/stage4.bin build/mega65/stage3.bin
	rm -rf $(DIST)
	mkdir -p $(DIST)
	cp build/mega65/stage4.bin $(DIST_NAME).bin
	cp build/mega65/stage3.bin $(DIST_NAME)_stage3.bin
	@echo "$(DIST_FILES)"

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
