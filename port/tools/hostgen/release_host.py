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
    release_host.py --check port/game/rac2-pal

--check needs no disc: the release's names table and <game>_version.h name exactly what the shared
C uses now (a row or an address for each, none left over), so a change to the shared C that its
release tables were not written again for fails (port/tools/versionmap/check_versions.py).
"""
from __future__ import annotations

import argparse
import json
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


# The shared C as release_carry.py reads it: names, addresses and constants outside comments and strings.
FUNC = re.compile(r"\bfunc_(?:L\d\d_)?[0-9A-F]{8}((?:_\w+)?)\b")
SKIP = re.compile(r'"(?:\\.|[^"\\])*"|/\*.*?\*/|//[^\n]*', re.S)


def check(program: Path) -> list[str]:
    """What PROGRAM's names table and <game>_version.h lack or have left over against the shared C."""
    cfg = json.loads((program / "hostgen.json").read_text())
    game = cfg["game"]
    big = game.upper()
    code = [SKIP.sub(" ", f.read_text(encoding="utf-8")) for d in cfg.get("host_shared", [])
            for f in sorted((program / d).glob("*.[ch]"))]
    used = {m.group(0) for t in code for m in FUNC.finditer(t)}
    keys = set()
    for t in code:
        keys |= {f"{big}_ADDR_0x{a}u" for a in re.findall(rf"\b{big}_ADDR\(\s*0x([0-9A-F]{{8}})u\s*\)", t)}
        keys |= {f"{big}_LADDR_{n}_0x{a}u"
                 for n, a in re.findall(rf"\b{big}_LADDR\(\s*(\d\d)\s*,\s*0x([0-9A-F]{{8}})u\s*\)", t)}
        keys |= {f"{big}_K_{k}" for k in re.findall(rf"\b{big}_K\(\s*(\w+)\s*,", t)}
    out = []
    header = program / "host" / f"{game}_version.h"
    text = header.read_text(encoding="utf-8")
    # An entry the tool could not pair is a comment ("not found"): still an answer for that use.
    have = set(re.findall(rf"^#define ({big}_(?:ADDR_0x|LADDR_|K_)\w+)", text, re.M))
    have |= set(re.findall(rf"^/\* ({big}_(?:ADDR_0x|LADDR_|K_)\w+): not found \*/", text, re.M))
    out += [f"{header.name}: no entry for {k}, which the shared C uses" for k in sorted(keys - have)]
    out += [f"{header.name}: {k} is not used by the shared C" for k in sorted(have - keys)]
    path = program / "host" / f"{game}_names.tsv"
    names = read_names(path)
    gone = set(re.findall(r"^# (func_\w+): not in this release$", path.read_text(), re.M))
    out += [f"{path.name}: {n} is not used by the shared C" for n in sorted((set(names) | gone) - used)]
    # A function the table renames: each spelling the shared C uses (__game, __hooked) is renamed too.
    renamed = {n[:len(n) - len(FUNC.fullmatch(n).group(1))] for n in names if FUNC.fullmatch(n)}
    for n in sorted(used - set(names)):
        base = n[:len(n) - len(FUNC.fullmatch(n).group(1))]
        if base in renamed:
            out.append(f"{path.name}: no row for {n}, though {base} is renamed")
    return out


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--names", type=Path)
    ap.add_argument("--out", type=Path)
    ap.add_argument("--check", type=Path, metavar="PROGRAM", help="port/game/<id>: check its tables, write nothing")
    ap.add_argument("files", type=Path, nargs="*")
    args = ap.parse_args(argv)
    if args.check:
        problems = check(args.check)
        for p in problems:
            print(f"{args.check.name}: {p}", file=sys.stderr)
        if problems:
            print(f"{args.check.name}: write its tables again with release_carry.py", file=sys.stderr)
        return 1 if problems else 0
    if not (args.names and args.out and args.files):
        ap.error("--names, --out and files are needed without --check")
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
