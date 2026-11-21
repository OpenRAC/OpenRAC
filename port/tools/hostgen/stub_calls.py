#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Finds C that calls a function with no C (an empty stub in stubs.c).

hostgen writes an empty body for every function it has no C for. A call to one from translated
C does nothing, silently: the game logic behind it is lost. PAL Going Commando's
ResetGornStageCounters called the v1.01 name of table_reset_working_values, which PAL has under
another name, so the stub ran and the ship's Nuke ammo was never refilled. The runtime only logs
such a call when it happens; this finds them all from the generated C, with no disc and no run.

    python3 port/tools/hostgen/stub_calls.py --gen port/build/release/games/rac2-pal/gen \\
        --allow port/game/rac2-pal/checks/stub_calls.tsv

Exit 1 when a call is found that the allow table does not list (name, caller, why), or when a
row of the table no longer matches a call. Exit 77 (skipped) when the generated C is missing.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

# An empty body: its definition line, then its reach site marked "no-c".
STUB = re.compile(r'^[^\s#][^\n]*?\b(\w+)\([^;{]*\)\s*\{\n\s*static openrac_reach_site site_ = \{"[^"]*", "no-c"',
                  re.M)
# A function definition in generated C: a line that starts at column 0 and opens a body.
DEFINITION = re.compile(r"^[A-Za-z_][\w \*]*?\b(\w+)\([^;]*\)\s*$|^[A-Za-z_][\w \*]*?\b(\w+)\([^;]*\)\s*\{", re.M)


def stubs(gen: Path) -> set[str]:
    path = gen / "stubs.c"
    return set(STUB.findall(path.read_text(errors="replace"))) if path.is_file() else set()


def calls(gen: Path, names: set[str]) -> dict[tuple[str, str], str]:
    """(stub, caller) -> the file of the first call."""
    if not names:
        return {}
    word = re.compile(r"(?<![\w.>])([A-Za-z_]\w*)\s*\(")
    out: dict[tuple[str, str], str] = {}
    for path in sorted(gen.rglob("*.c")):
        rel = path.relative_to(gen).as_posix()
        if rel.startswith("prep/") or rel in ("stubs.c", "functions.c"):
            continue
        text = path.read_text(errors="replace")
        hits = [m for m in word.finditer(text) if m.group(1) in names]
        if not hits:
            continue
        starts = [(m.start(), m.group(1) or m.group(2)) for m in DEFINITION.finditer(text)]
        for m in hits:
            line_start = text.rfind("\n", 0, m.start()) + 1
            if text[line_start] not in " \t":
                continue  # a definition or declaration at column 0, not a call
            caller = next((name for at, name in reversed(starts) if at < m.start()), "?")
            out.setdefault((m.group(1), caller), rel)
    return out


def read_allow(path: Path | None) -> dict[tuple[str, str], str]:
    out: dict[tuple[str, str], str] = {}
    if path is None or not path.is_file():
        return out
    for line in path.read_text().splitlines():
        if not line.strip() or line.startswith("#"):
            continue
        parts = line.split("\t")
        if len(parts) < 3 or not parts[2].strip():
            raise SystemExit(f"{path}: a row needs name, caller and why: {line!r}")
        out[(parts[0].strip(), parts[1].strip())] = parts[2].strip()
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--gen", type=Path, required=True, help="hostgen's output folder")
    ap.add_argument("--allow", type=Path, help="calls known and allowed: name, caller, why")
    args = ap.parse_args()
    if not (args.gen / "stubs.c").is_file():
        print(f"{args.gen}: no generated C (skipped)")
        return 77
    found = calls(args.gen, stubs(args.gen))
    allow = read_allow(args.allow)
    bad = sorted(k for k in found if k not in allow)
    stale = sorted(k for k in allow if k not in found)
    for name, caller in bad:
        print(f"{caller} calls {name}, which has no C ({found[(name, caller)]})")
    for name, caller in stale:
        print(f"{args.allow}: {name} from {caller} is no longer called: remove the row")
    if bad or stale:
        print(f"{len(bad)} call(s) to functions with no C, {len(stale)} stale allow row(s)")
        return 1
    print(f"{len(found)} call(s) to functions with no C, all allowed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
