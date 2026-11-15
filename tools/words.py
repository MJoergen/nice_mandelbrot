#!/usr/bin/env python3
"""Print words of a dump, as niceemu -d prints them, one per line in hex.

    words.py DUMP ADDR N

DUMP is a file with lines "ADDR: WORD WORD ..." in hex, as niceemu -d
and tools/board.py --dump write them, among other lines, which are
passed over. Prints the N words from ADDR, and fails if one is not in
DUMP. tools/screenshot.py imports read_dump.

Copyright (c) 2026 Michael Jørgensen. MIT license (see LICENSE).
"""

import re
import signal
import sys

LINE = re.compile(r"^([0-9a-f]{8}):((?: [0-9a-f]{8})+)\s*$")


def read_dump(path):
    """The words of the dump lines of path, as {address: word}."""
    words = {}
    with open(path) as f:
        for line in f:
            m = LINE.match(line)
            if m:
                addr = int(m.group(1), 16)
                for i, w in enumerate(m.group(2).split()):
                    words[addr + i] = int(w, 16)
    return words


def main():
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)  # cmp may stop reading early
    if len(sys.argv) != 4:
        sys.exit(f"usage: {sys.argv[0]} DUMP ADDR N")
    words = read_dump(sys.argv[1])
    addr, n = int(sys.argv[2], 0), int(sys.argv[3], 0)
    missing = [a for a in range(addr, addr + n) if a not in words]
    if missing:
        sys.exit(f"{sys.argv[1]}: no word at {missing[0]:08x}")
    print("\n".join(f"{words[a]:08x}" for a in range(addr, addr + n)))


if __name__ == "__main__":
    main()
