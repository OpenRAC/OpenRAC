#!/usr/bin/env python3
"""
Use names for the hero's control modes.

  python3 tools/tidy/control_modes.py            # dry run: show counts
  python3 tools/tidy/control_modes.py --write
  python3 tools/tidy/control_modes.py --list

`g->state.control_mode == 0x14` becomes `g->state.control_mode == HERO_MODE_DYING`, and the global `hero_control_mode` follows. Names are in
include/enums/hero_state.h, and a value with no name stays a number. An enum constant is an int, so no byte changes.
Only a plain compare or store of a number is rewritten: `control_mode - 0x15` stays. A file that cannot see the enum
gets the include. Run tools/format.py and tools/gate_loop.py afterwards.
"""
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import prune_externs as pe  # noqa: E402

ROOT = pe.ROOT
HEADER = "enums/hero_state.h"
MODES = {
    0x00: "IDLE", 0x01: "RUN", 0x02: "FALL", 0x03: "LEDGE", 0x04: "JUMP", 0x05: "LAND", 0x06: "ATTACK", 0x07: "HURT",
    0x08: "ITEM_RELEASE_WAIT", 0x0C: "CROUCH", 0x0D: "GRAPPLE", 0x0E: "GRAPPLE_SWING", 0x0F: "GRIND", 0x11: "SWIM",
    0x12: "WATER_SURFACE", 0x14: "DYING", 0x19: "RESPAWN_WAIT", 0x1A: "FOLLOW_PATH",
}
SITE = re.compile(r"((?:\.(?:prev2?_)?control_mode|\bhero_control_mode(?:\[0\])?)\s*(?:==|!=|=)\s*)(0[xX][0-9A-Fa-f]+|\d+)\b(?=\s*(?:[;),&|?:]|$))", re.M)
INCLUDE = re.compile(r'^#include\s+"([^"]+)"', re.M)


def rewrite_code(code):
    """(new code, number of sites)."""
    count = [0]

    def one(m):
        name = MODES.get(int(m.group(2), 0))
        if not name:
            return m.group(0)
        count[0] += 1
        return f"{m.group(1)}HERO_MODE_{name}"

    return SITE.sub(one, code), count[0]


def rewrite(text):
    """Rewrite code only: comments and strings stay."""
    out, last, total = [], 0, 0
    for m in pe.COMMENT_OR_STRING.finditer(text):
        code, count = rewrite_code(text[last:m.start()])
        out += [code, m.group(0)]
        total += count
        last = m.end()
    code, count = rewrite_code(text[last:])
    return "".join(out) + code, total + count


def sees_header(path, seen=None):
    """Whether the file reaches the enum through its includes."""
    seen = set() if seen is None else seen
    for name in INCLUDE.findall(path.read_text(errors="replace")):
        if name == HEADER:
            return True
        for base in (ROOT / "include", path.parent):
            found = base / name
            if found.exists() and found not in seen:
                seen.add(found)
                if sees_header(found, seen):
                    return True
                break
    return False


def add_include(text):
    last = None
    for last in INCLUDE.finditer(text):
        pass
    line = f'#include "{HEADER}"\n'
    if not last:
        return line + text
    at = text.index("\n", last.end()) + 1
    return text[:at] + line + text[at:]


def main():
    write, listing = "--write" in sys.argv[1:], "--list" in sys.argv[1:]
    out = subprocess.run(["git", "ls-files", "src/*.c"], cwd=ROOT, capture_output=True, text=True).stdout
    total = files = includes = 0
    for rel in out.split():
        path = ROOT / rel
        text = path.read_text(errors="surrogateescape")
        new, count = rewrite(text)
        if not count:
            continue
        total += count
        files += 1
        needs = not rel.startswith("src/versions/") and not sees_header(path)  # a fragment joins its host file
        includes += needs
        if listing:
            print(f"{rel}: {count}{' (adds the include)' if needs else ''}")
        if write:
            path.write_text(add_include(new) if needs else new, errors="surrogateescape")
    print(f"{'changed' if write else 'would change'} {total} sites in {files} files, {includes} new includes")


if __name__ == "__main__":
    main()
