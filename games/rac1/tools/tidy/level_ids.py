#!/usr/bin/env python3
"""
Use level names instead of numbers.

  python3 tools/tidy/level_ids.py            # dry run: show counts
  python3 tools/tidy/level_ids.py --write
  python3 tools/tidy/level_ids.py --list     # show the sites

`g_current_level_index == 5` becomes `g_current_level_index == LEVEL_RILGAR`, and the cases of a switch on it change
too. An enum has the same value, so the bytes do not change. Only the level index globals change: a local named level
may hold something else.
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import levels  # noqa: E402
import moby_classes as mc  # noqa: E402
import prune_externs as pe  # noqa: E402

ROOT = pe.ROOT
INCLUDE = '#include "enums/level.h"'
INDEX = r"\b(?:g_current_level_index|selected_level_index)\w*"
COMPARE = re.compile(r"(" + INDEX + r"\s*(?:==|!=)\s*)(0[xX][0-9A-Fa-f]+|[0-9]+)\b")
BELOW = re.compile(r"(" + INDEX + r"\s*<\s*)(0[xX]13|19)\b")
SWITCH = re.compile(r"\bswitch\s*\(\s*" + INDEX + r"\s*\)\s*\{")


def names():
    """{level number: enum name}"""
    return {level: f"LEVEL_{name.upper()}" for level, name, _ in levels.LEVELS}


def rewrite(text, table=None):
    """(new text, number of literals replaced)."""
    table = table or names()
    count = [0]

    def compare(m):
        number = int(m.group(2), 0)
        if number not in table:
            return m.group(0)
        count[0] += 1
        return m.group(1) + table[number]

    def below(m):
        count[0] += 1
        return m.group(1) + "LEVEL_COUNT"

    text = mc.outside_comments_and_strings(text, lambda code: BELOW.sub(below, COMPARE.sub(compare, code)))
    text, cases = mc.rewrite_switches(text, table, SWITCH)
    return text, count[0] + cases


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
                if "LEVEL_" in line and line not in text:
                    print(f"{rel}: {line.strip()[:110]}")
        if write:
            path.write_text(add_include(new), errors="surrogateescape")
            for fragment, after in changed.items():
                fragment.write_text(after, errors="surrogateescape")
    print(f"{'replaced' if write else 'would replace'} {total} level numbers in {files} files")


if __name__ == "__main__":
    main()
