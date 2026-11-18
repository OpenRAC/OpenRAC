#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""A game's shared host C in one release's function names, for that release's build.

The shared host C (port/game/rac2/host) is written with the primary release's names
(`func_0027F538`); another release's table (`host/<game>_names.tsv`: rac2's written by release_carry.py, rac1's by
release_rac1.py)
gives each its name there. A macro per name would chain: v1.01's `func_0028E458` is the release's
`func_0028ECD8`, which is itself one of v1.01's names the host C uses. So the build compiles copies
written in one pass, every name once (cmake/Games.cmake), and hostgen reads the host C so too.

    release_host.py --names port/game/rac2-pal/host/rac2_names.tsv --out <dir> port/game/rac2/host/*.c
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

TOKEN = re.compile(r"\bfunc_\w+\b")


def read_names(path: Path) -> dict[str, str]:
    """v1.01's name -> the release's, from the table (two columns; # for comments)."""
    out = {}
    for line in path.read_text().splitlines():
        cols = line.split("\t")
        if line.startswith("#") or len(cols) < 2:
            continue
        out[cols[0]] = cols[1]
    return out


# A function the version's table leaves out (rac1.h, `#if !RAC1_OMIT(func_...)`): the table names it
# as the shared C does, so the name inside stays.
OMITTED = re.compile(r"\bRAC1_OMIT\(\s*\w+\s*\)")


def rename(text: str, names: dict[str, str]) -> str:
    """TEXT with each name of NAMES written as the release's, in one pass (no name is renamed twice)."""
    if not names:
        return text
    parts = OMITTED.split(text)
    kept = OMITTED.findall(text)
    out = [TOKEN.sub(lambda m: names.get(m.group(0), m.group(0)), p) for p in parts]
    return "".join(x for pair in zip(out, kept + [""]) for x in pair)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--names", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("files", type=Path, nargs="+")
    args = ap.parse_args(argv)
    names = read_names(args.names)
    args.out.mkdir(parents=True, exist_ok=True)
    for f in args.files:
        text = rename(f.read_text(encoding="utf-8"), names)
        # The copy is elsewhere: an include beside the shared file is found from the top of port/.
        text = text.replace('#include "../../common/', '#include "game/common/')
        # A line directive keeps the compiler's messages on the shared file.
        text = f'#line 1 "{f.resolve().as_posix()}"\n' + text
        out = args.out / f.name
        if not out.exists() or out.read_text(encoding="utf-8") != text:
            out.write_text(text, encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
