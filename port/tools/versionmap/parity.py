#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""What each version of a game runs for each function the port binds, and where versions differ.

    parity.py rac1                 the check: errors, then exit 1 if any
    parity.py rac1 --table         also every function a table binds, version by version
    parity.py --program rac1-pal1  the same for that program's game (its build runs it)

Every row of a program's libraries.tsv (host, wrap, hook, game) is read as a place: the
executable or a level, and an address. The decomp's catalogue of that version
(games/rac1/decomp/config/versions/<v>/functions.tsv, no disc needed) says which function starts
there, by its PAL v2.00 name, so the four rac1 programs can be read side by side. Two errors:

- a row whose address is not the start of a function in its version: it binds the middle of
  another function, or nothing, and the function it was meant for runs its console C;
- a function a version has, bound one way in one version and another way (or not at all) in
  another, with no reason in port/game/<game>/parity.tsv.

parity.tsv: PAL v2.00 name, place (boot or NN), the programs that differ, why; or a table row's
name, "row", the programs whose row is not at a function's start, why. A row there that no
longer applies is an error too, so the file stays true.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
NAME = re.compile(r"^(?:FUN|func)_(?:L(\d\d)_)?([0-9A-Fa-f]{8})(?:_\w*)?$")
# The programs of each game the check reads, and their catalogue in rac1-decomp's versions.
GAMES = {
    "rac1": {"source": "games/rac1/decomp",
             "programs": {"rac1-pal": "pal", "rac1-ntsc": "us", "rac1-pal1": "pal_v1", "rac1-jp": "jp"}},
}


class Catalogue:
    """One version's functions: where each starts (place, address) and its PAL v2.00 name."""

    def __init__(self, path: Path):
        self.at: dict[tuple[str, int], str] = {}
        self.spans: dict[str, list[tuple[int, int, str]]] = defaultdict(list)
        self.places: dict[str, set[str]] = defaultdict(set)
        for line in path.read_text().splitlines():
            cols = line.split("\t")
            if line.startswith("#") or len(cols) < 9:
                continue
            name, size = cols[0], int(cols[2]) if cols[2].isdigit() else 0
            for item in cols[8].split(","):
                place, _, addr = item.partition(":")
                if not addr:
                    continue
                self.at.setdefault((place, int(addr, 16)), name)
                self.spans[place].append((int(addr, 16), size, name))
                self.places[name].add(place)

    def inside(self, place: str, address: int) -> str | None:
        """The function whose bytes hold ADDRESS in PLACE, if one does."""
        for start, size, name in self.spans.get(place, ()):
            if start < address < start + size:
                return name
        return None


def symbols(source: Path) -> dict[str, int]:
    """The named functions of a decomp whose rows use names, not addresses (the NTSC decomp)."""
    out = {}
    for path in (source / "config/us/symbol_addrs.txt", source / "config/us/port_symbols.txt"):
        if path.is_file():
            for m in re.finditer(r"^(\w+) = 0x([0-9A-Fa-f]+);", path.read_text(), re.M):
                out[m.group(1)] = int(m.group(2), 16)
    return out


def place_of(name: str, named: dict[str, int]) -> tuple[str, int] | None:
    m = NAME.match(name)
    if m:
        return (m.group(1) or "boot", int(m.group(2), 16))
    if name in named:
        return ("boot", named[name])
    return None


def rows(path: Path) -> list[list[str]]:
    return [line.split("\t") for line in path.read_text().splitlines()
            if line.strip() and not line.startswith("#") and len(line.split("\t")) >= 4]


def read_reasons(path: Path) -> dict[tuple[str, str], tuple[set[str], str]]:
    out = {}
    if path.is_file():
        for cols in (line.split("\t") for line in path.read_text().splitlines()
                     if line.strip() and not line.startswith("#")):
            out[(cols[0], cols[1])] = (set(cols[2].split(",")), cols[3] if len(cols) > 3 else "")
    return out


def check(game: str, table: bool = False, root: Path = ROOT) -> list[str]:
    spec = GAMES[game]
    source = root / spec["source"]
    errors: list[str] = []
    bound: dict[tuple[str, str], dict[str, str]] = defaultdict(dict)
    cats: dict[str, Catalogue] = {}
    reasons = read_reasons(root / "port/game" / game / "parity.tsv")
    used = set()
    for program, version in spec["programs"].items():
        cat = cats[program] = Catalogue(source / "config/versions" / version / "functions.tsv")
        cfg = json.loads((root / "port/game" / program / "hostgen.json").read_text())
        named = symbols(root / cfg["source"])
        kinds: dict[tuple[str, str], set[str]] = defaultdict(set)
        for cols in rows(root / "port/game" / program / "libraries.tsv"):
            at = place_of(cols[0], named)
            if at is None:
                continue  # a library function by its own name (memcpy): no address to read
            name = cat.at.get(at)
            if name is None:
                why = reasons.get((cols[0], "row"))
                if why is not None and program in why[0]:
                    used.add((cols[0], "row"))
                    continue
                held = cat.inside(*at)
                where = f"inside {held}" if held else "at no function"
                errors.append(f"{program}: libraries.tsv {cols[0]} ({cols[2]}, {cols[3]}) binds "
                              f"{at[0]}:{at[1]:08X}, {where} in this version: the row is carried to the "
                              f"wrong address, and the function it is for runs its console C")
                continue
            kinds[(name, at[0])].add(cols[3])
        for key, k in kinds.items():
            bound[key][program] = ",".join(sorted(k))
    lines = []
    for (name, place), by in sorted(bound.items()):
        # A version without the function, or without it in that place, has nothing to bind.
        seen = {p: by.get(p, "-") for p, cat in cats.items() if place in cat.places.get(name, ())}
        lines.append(f"{name}\t{place}\t" + "\t".join(f"{p}={seen.get(p, 'absent')}" for p in cats))
        if len(set(seen.values())) <= 1:
            continue
        # What most versions do (in a tie, the first program's: the primary's) is the rule.
        counts = list(seen.values())
        common = next(v for v in seen.values() if counts.count(v) == max(map(counts.count, counts)))
        odd = {p for p, v in seen.items() if v != common}
        why = reasons.get((name, place))
        if why is not None:
            used.add((name, place))
            if why[0] == odd:
                continue
        errors.append(f"{game}: {name} ({place}) runs differently: "
                      + ", ".join(f"{p} {v}" for p, v in seen.items())
                      + f"; give the reason in port/game/{game}/parity.tsv ({','.join(sorted(odd))}) or fix the tables")
    for key in sorted(set(reasons) - used):
        errors.append(f"{game}: parity.tsv lists {key[0]} ({key[1]}), whose versions no longer differ: remove the row")
    if table:
        print("\n".join(lines))
    return errors


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("game", nargs="?", choices=sorted(GAMES))
    ap.add_argument("--program", help="the build of this program (port/cmake/Games.cmake): its game's check, "
                                      "or nothing for a game the check does not read yet")
    ap.add_argument("--table", action="store_true", help="print every bound function, version by version")
    args = ap.parse_args(argv)
    game = args.game or next((g for g, spec in GAMES.items() if args.program in spec["programs"]), None)
    if game is None:
        if args.program is None:
            ap.error("give a game or --program")
        return 0
    source = ROOT / GAMES[game]["source"] / "config/versions"
    if args.program and not source.is_dir():
        # A build from a checkout without rac1-decomp (the NTSC decomp alone): CI checks it.
        print(f"parity: {game}: no {source.relative_to(ROOT)} here, not checked", file=sys.stderr)
        return 0
    errors = check(game, args.table)
    for e in errors:
        print(f"parity: {e}", file=sys.stderr)
    print(f"parity: {game}: {len(errors)} problems", file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
