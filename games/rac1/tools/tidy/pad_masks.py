#!/usr/bin/env python3
"""
Use button names instead of numbers for the pad's masks.

  python3 tools/tidy/pad_masks.py            # dry run: show counts
  python3 tools/tidy/pad_masks.py --write
  python3 tools/tidy/pad_masks.py --list     # show the sites

`g_controller_state.pressed & 0x40` becomes `g_controller_state.pressed & PAD_CROSS`, and `& 0xC0` becomes
`& (PAD_CROSS | PAD_SQUARE)`. A macro has the same value, so the bytes do not change.

Only expressions that are certainly the pad are rewritten. `held` and `pressed` alone are common words, so a local
variable with one of those names stays as it is. Bits above 0xFFFF are not buttons (include/structs/pad.h).
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import moby_classes as mc  # noqa: E402
import prune_externs as pe  # noqa: E402

ROOT = pe.ROOT
INCLUDE = '#include "structs/pad.h"'
BITS = {0x0001: "PAD_L2", 0x0002: "PAD_R2", 0x0004: "PAD_L1", 0x0008: "PAD_R1", 0x0010: "PAD_TRIANGLE",
        0x0020: "PAD_CIRCLE", 0x0040: "PAD_CROSS", 0x0080: "PAD_SQUARE", 0x0100: "PAD_SELECT", 0x0200: "PAD_L3",
        0x0400: "PAD_R3", 0x0800: "PAD_START", 0x1000: "PAD_UP", 0x2000: "PAD_RIGHT", 0x4000: "PAD_DOWN",
        0x8000: "PAD_LEFT"}
# The global, under any of its names, then the field. Also the two fields with their own global name.
PAD = (r"(?:\b(?:g_controller_state\w*|gPad\w*|C940\w*|pad_state\w*)(?:\[[^\]]*\])?(?:\.|->)(?:pressed|held|repeat\w*)"
       r"|\bpad0_(?:pressed|held)\w*|\bpad_held\b)")
MASK = re.compile(r"(" + PAD + r"\s*&\s*)(0[xX][0-9A-Fa-f]+|[0-9]+)\b")
# More than this many buttons is a group, and spelling it out does not help.
MAX_BITS = 4


def spelled(value):
    """The buttons in a mask, or None if it is not a short list of buttons."""
    if not 0 < value <= 0xFFFF:
        return None
    names = [name for bit, name in BITS.items() if value & bit]
    if len(names) > MAX_BITS:
        return None
    return names[0] if len(names) == 1 else "(" + " | ".join(names) + ")"


def rewrite(text):
    """(new text, number of masks replaced)."""
    count = [0]

    def one(m):
        name = spelled(int(m.group(2), 0))
        if name is None:
            return m.group(0)
        count[0] += 1
        return m.group(1) + name

    return mc.outside_comments_and_strings(text, lambda code: MASK.sub(one, code)), count[0]


def add_include(text):
    if INCLUDE in text:
        return text
    first = re.search(r"^#include [^\n]*\n(?:#include [^\n]*\n)*", text, re.M)
    at = first.end() if first else 0
    return text[:at] + INCLUDE + "\n" + text[at:]


def main():
    write, listing = "--write" in sys.argv[1:], "--list" in sys.argv[1:]
    total = files = 0
    for rel in pe.sources(["src"]):
        path = ROOT / rel
        text = path.read_text(errors="surrogateescape")
        new, count = rewrite(text)
        changed = {}
        for fragment in sorted((ROOT / "src/versions").glob(f"*/{rel[len('src/'):-len('.c')]}/*.c")):
            old = fragment.read_text(errors="surrogateescape")
            after, fragment_count = rewrite(old)
            if fragment_count:
                changed[fragment] = after
                count += fragment_count
        if not count:
            continue
        total += count
        files += 1
        if listing:
            for line in new.splitlines():
                if "PAD_" in line and line not in text:
                    print(f"{rel}: {line.strip()[:110]}")
        if write:
            path.write_text(add_include(new), errors="surrogateescape")
            for fragment, after in changed.items():
                fragment.write_text(after, errors="surrogateescape")
    print(f"{'replaced' if write else 'would replace'} {total} masks in {files} files")


if __name__ == "__main__":
    main()
