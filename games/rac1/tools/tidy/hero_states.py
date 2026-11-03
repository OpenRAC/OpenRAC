#!/usr/bin/env python3
"""
Use hero state names instead of numbers.

  python3 tools/tidy/hero_states.py            # dry run: show counts
  python3 tools/tidy/hero_states.py --write
  python3 tools/tidy/hero_states.py --list     # show the sites

`g_hero.state.current == 0x32` and `Hero_SetMoveState(0x7A, 1)` take names from include/enums/hero_state.h, as do the
cases of a switch on the current state. A name has the same value, so the bytes do not change. A number with no name
stays as it is. control_mode is left alone: it is not a state ID.
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import moby_classes as mc  # noqa: E402
import prune_externs as pe  # noqa: E402

ROOT = pe.ROOT
HEADER = ROOT / "include/enums/hero_state.h"
INCLUDE = '#include "enums/hero_state.h"'
NUMBER = r"(0[xX][0-9A-Fa-f]+|[0-9]+)"
FIELD = r"(?:\.|->)state\.(?:current|prev2?)\b"
COMPARE = re.compile(r"(" + FIELD + r"\s*(?:==|!=)\s*)" + NUMBER + r"\b")
ASSIGN = re.compile(r"(" + FIELD + r"\s*=\s*)" + NUMBER + r"(?=\s*;)")
CALL = re.compile(r"(\b\w*MoveState\w*\s*\(\s*)" + NUMBER + r"(?=\s*[,)])")
SWITCH = re.compile(r"\bswitch\s*\([^(){};]*" + FIELD + r"\s*\)\s*\{")


def names(path=HEADER):
    """{state number: name}, from the HeroStateId enum only."""
    body = re.search(r"enum HeroStateId \{(.*?)\};", path.read_text(), re.S).group(1)
    return {int(value, 16): name for name, value in re.findall(r"(HERO_STATE_\w+) = (0x[0-9A-Fa-f]+)", body)}


def rewrite(text, table):
    """(new text, number of literals replaced)."""
    count = [0]

    def one(m):
        number = int(m.group(2), 0)
        if number not in table:
            return m.group(0)
        count[0] += 1
        return m.group(1) + table[number]

    def code(part):
        for pattern in (COMPARE, ASSIGN, CALL):
            part = pattern.sub(one, part)
        return part

    text = mc.outside_comments_and_strings(text, code)
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
    table = names()
    total = files = 0
    for rel in pe.sources(["src"]):
        path = ROOT / rel
        text = path.read_text(errors="surrogateescape")
        new, count = rewrite(text, table)
        changed = {}
        for fragment in sorted((ROOT / "src/versions").glob(f"*/{rel[len('src/'):-len('.c')]}/*.c")):
            old = fragment.read_text(errors="surrogateescape")
            after, fragment_count = rewrite(old, table)
            if fragment_count:
                changed[fragment] = after
                count += fragment_count
        if not count:
            continue
        total += count
        files += 1
        if listing:
            for line in new.splitlines():
                if "HERO_STATE_" in line and line not in text:
                    print(f"{rel}: {line.strip()[:110]}")
        if write:
            path.write_text(add_include(new), errors="surrogateescape")
            for fragment, after in changed.items():
                fragment.write_text(after, errors="surrogateescape")
    print(f"{'replaced' if write else 'would replace'} {total} state numbers in {files} files")


if __name__ == "__main__":
    main()
