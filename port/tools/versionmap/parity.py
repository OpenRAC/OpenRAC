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
  another, with no reason in port/game/<game>/parity.tsv. A version whose host/rac1_version.h
  leaves the shared host C out (RAC1_OMIT, with its reason) is not compared there.

A row naming a variant of the program's split_places.tsv binds it by name (a carried release
keeps PAL v2.00's name there, with its own places): it is read at the variant's first place.

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


def split_variants(path: Path) -> dict[str, list[tuple[str, int]]]:
    """The variants a program's split_places.tsv splits off folded functions: name, places."""
    out: dict[str, list[tuple[str, int]]] = {}
    if path.is_file():
        for cols in (line.split("\t") for line in path.read_text().splitlines()):
            if cols[0].startswith("#") or len(cols) < 3:
                continue
            for item in cols[2].split(","):
                level, _, addr = item.strip().partition(":")
                if addr:
                    out.setdefault(cols[1], []).append((f"{int(level):02d}", int(addr, 16)))
    return out


def omits(header: Path) -> dict[str, str]:
    """The shared host C a program leaves out on purpose (host/rac1_version.h, RAC1_OMIT_<name> 1),
    by PAL v2.00's name, with the comment that says why."""
    out: dict[str, str] = {}
    if not header.is_file():
        return out
    why = ""
    for line in header.read_text().splitlines():
        m = re.match(r"\s*#define RAC1_OMIT_(\w+) 1\b(.*)", line)
        if m:
            out[m.group(1)] = (why + " " + m.group(2).strip()).strip()
        elif line.lstrip().startswith(("/*", "*")):
            why = (why + " " if not line.lstrip().startswith("/*") else "") + line.strip(" /*")
        elif line.strip():
            why = ""
    return out


def check(game: str, table: bool = False, root: Path = ROOT) -> list[str]:
    spec = GAMES[game]
    source = root / spec["source"]
    errors: list[str] = []
    bound: dict[tuple[str, str], dict[str, str]] = defaultdict(dict)
    cats: dict[str, Catalogue] = {}
    reasons = read_reasons(root / "port/game" / game / "parity.tsv")
    used = set()
    # The places of the primary's names (RAC1_OMIT names a PAL v2.00 row or function).
    primary = next(iter(spec["programs"]))
    first = Catalogue(source / "config/versions" / spec["programs"][primary] / "functions.tsv")
    left_out: dict[str, dict[tuple[str, str], str]] = {}
    # Variants split off folded functions (split_places.tsv), each program's own: a carried
    # release's file keeps PAL v2.00's names, with its own places.
    split = {p: split_variants(root / "port/game" / p / "split_places.tsv") for p in spec["programs"]}
    for program in spec["programs"]:
        left_out[program] = {}
        for name, why in omits(root / "port/game" / program / "host/rac1_version.h").items():
            for at in split[primary].get(name, [])[:1]:  # a variant split off a folded function
                if at in first.at:
                    left_out[program][(first.at[at], at[0])] = why
            at = place_of(name, {})
            owner = first.at.get(at) if at else None
            if owner is not None:  # the omit takes the function out in every place it has
                for place in first.places[owner]:
                    left_out[program][(owner, place)] = why
    for program, version in spec["programs"].items():
        cat = cats[program] = Catalogue(source / "config/versions" / version / "functions.tsv")
        cfg = json.loads((root / "port/game" / program / "hostgen.json").read_text())
        named = symbols(root / cfg["source"])
        kinds: dict[tuple[str, str], set[str]] = defaultdict(set)
        for cols in rows(root / "port/game" / program / "libraries.tsv"):
            if cols[0] in split[program]:
                # A variant split off a folded function: bound by its name, in the places its
                # program's split_places.tsv gives it (a release keeps PAL v2.00's name there).
                # Read at its first place, as a function's own name is.
                at = split[program][cols[0]][0]
                if at in cat.at:
                    kinds[(cat.at[at], at[0])].add(cols[3])
                continue
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
        # A version that leaves the shared host C out on purpose (RAC1_OMIT, with its reason) runs
        # the game's own code there: not a difference.
        seen = {p: by.get(p, "-") for p, cat in cats.items()
                if place in cat.places.get(name, ()) and (name, place) not in left_out[p]}
        lines.append(f"{name}\t{place}\t" + "\t".join(
            f"{p}={'omitted' if (name, place) in left_out[p] else seen.get(p, 'absent')}" for p in cats))
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
