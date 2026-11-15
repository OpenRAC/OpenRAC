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

Two more, the same way:

- a function bound in several versions with a different C type in its row (the last column):
  the shared host C is then compiled against another declaration; a reason in parity.tsv says
  how the version's header takes it ("sig" for the place);
- an address of the game's data in a carried release's host/rac1_version.h (RAC1_ADDR,
  RAC1_LADDR) that neither its release_names.json names nor follows the nearest named data before
  and after it (both moved alike, and by as much): it is a guess with nothing behind it ("data",
  with the address as the name: L17:00161240 or boot:0013D618). release_carry.json's addresses
  (versionmap's pairs) count as named; its exact data matches are not recorded there.

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
# The programs of each game the check reads (the primary first), and each one's catalogue: the
# decomp's files that name every function of that version by the primary's name, with its places.
GAMES = {
    "rac1": {"source": "games/rac1/decomp",
             "catalogue": ["config/versions/{v}/functions.tsv"],
             "programs": {"rac1-pal": "pal", "rac1-ntsc": "us", "rac1-pal1": "pal_v1", "rac1-jp": "jp"}},
    # rac2-decomp's own config is NTSC v1.01's; config/<v>/ the releases' (tools/carry.py).
    "rac2": {"source": "games/rac2/decomp",
             "catalogue": ["config/{v}boot/functions.tsv", "config/{v}levels/functions.tsv"],
             "programs": {"rac2-ntsc": "", "rac2-ntsc2": "us2/", "rac2-pal": "pal/"}},
}


def catalogue(spec: dict, source: Path, version: str) -> "Catalogue":
    return Catalogue([source / f.format(v=version) for f in spec["catalogue"]])


class Catalogue:
    """One version's functions: where each starts (place, address) and its PAL v2.00 name."""

    def __init__(self, paths: list[Path]):
        self.at: dict[tuple[str, int], str] = {}
        self.spans: dict[str, list[tuple[int, int, str]]] = defaultdict(list)
        self.places: dict[str, set[str]] = defaultdict(set)
        for path in paths:
            self.read(path)

    def read(self, path: Path) -> None:
        """A table with a "places (level:address)" column, or an executable's (name, address,
        size): its header says which."""
        lines = path.read_text().splitlines()
        heads = [line.lstrip("# ").split("\t") for line in lines if line.startswith("#")]
        where = next((i for head in heads for i, c in enumerate(head) if c.startswith("places")), None)
        if where is None and not any(head[1:2] == ["address"] for head in heads):
            where = 8  # rac1-decomp's config/versions/<v>/functions.tsv
        for line in lines:
            cols = line.split("\t")
            if line.startswith("#") or len(cols) < 3:
                continue
            name, size = cols[0], int(cols[2]) if cols[2].isdigit() else 0
            if where is None:
                items = [f"boot:{cols[1]}"]
            elif len(cols) > where:
                items = cols[where].split(",")
            else:
                continue
            for item in items:
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


def omits(header: Path, game: str = "rac1") -> dict[str, str]:
    """The shared host C a program leaves out on purpose (host/rac1_version.h, RAC1_OMIT_<name> 1),
    by PAL v2.00's name, with the comment that says why."""
    out: dict[str, str] = {}
    if not header.is_file():
        return out
    why = ""
    for line in header.read_text().splitlines():
        m = re.match(rf"\s*#define {game.upper()}_OMIT_(\w+) 1\b(.*)", line)
        if m:
            out[m.group(1)] = (why + " " + m.group(2).strip()).strip()
        elif line.lstrip().startswith(("/*", "*")):
            why = (why + " " if not line.lstrip().startswith("/*") else "") + line.strip(" /*")
        elif line.strip():
            why = ""
    return out


TYPE_WORDS = {"gaddr": "u32", "unsigned": "u32", "uint32_t": "u32", "u32": "u32", "s32": "int",
              "int32_t": "int"}


def signature(text: str) -> str:
    """A row's C type in one spelling: gaddr and unsigned int alike, parameter names left out."""
    m = re.fullmatch(r"\s*(.*?)\s*\((.*)\)\s*", text)
    if not m:
        return text.strip()

    def one(part: str) -> str:
        words = re.sub(r"\bunsigned\s+int\b", "unsigned", part).split()
        if len(words) > 1 and words[-1] not in TYPE_WORDS and words[-1] not in (
                "int", "float", "void", "char", "short", "long") and "*" not in words[-1]:
            words = words[:-1]  # a parameter's name
        return " ".join(TYPE_WORDS.get(w, w) for w in words)

    args = [one(a) for a in m.group(2).split(",")] if m.group(2).strip() else []
    return f"{one(m.group(1))} ({', '.join(args) or 'void'})"


DEFINE = re.compile(r"^#define RAC1_ADDR_(?:L(\d\d)_)?0x([0-9A-Fa-f]{8})u\s+0x([0-9A-Fa-f]{8})u")
SYMBOL = re.compile(r"^(?:D|func)_(?:V2_)?(?:L(\d\d)_)?([0-9A-F]{8})$")


# The evidence release_carry.py writes beside each address of a rac2 release (rac2_version.h).
RAC2_EVIDENCE = {"the release's build", "versionmap: exact", "versionmap: between", "code",
                 "the same instruction of the paired function", "the same microcode at the same place"}
RAC2_DEFINE = re.compile(r"^#define RAC2_(?:L(\d\d)_|ADDR_)(?:0x)?([0-9A-Fa-f]{8})u\s+0x[0-9A-Fa-f]{8}u\s*(?:/\*\s*(.*?)\s*\*/)?")


def rac2_addresses(program_dir: Path) -> list[tuple[str, str]]:
    """A rac2 release's addresses whose evidence is not one release_carry.py stands by (a guess,
    a hand edit with no comment), or that it found nowhere."""
    header = program_dir / "host/rac2_version.h"
    if not (program_dir / "release_names.json").is_file() or not header.is_file():
        return []
    out = []
    for line in header.read_text().splitlines():
        if re.match(r"/\* RAC2_L?ADDR_\S+: not found \*/", line):
            name = line.split()[1].rstrip(":")
            out.append((name, "release_carry.py found it nowhere in this release"))
            continue
        m = RAC2_DEFINE.match(line.replace("RAC2_LADDR_", "RAC2_L").replace("_0x", "_"))
        if m and (m.group(3) or "") not in RAC2_EVIDENCE:
            place = f"L{m.group(1)}" if m.group(1) else "boot"
            out.append((f"{place}:{m.group(2).upper()}", f"its evidence is \"{m.group(3) or 'none'}\""))
    return out


def data_addresses(program_dir: Path) -> list[tuple[str, str]]:
    """The addresses in a carried release's rac1_version.h with nothing behind them: (name, why)."""
    if (program_dir / "host/rac2_version.h").is_file():
        return rac2_addresses(program_dir)
    names_path, header = program_dir / "release_names.json", program_dir / "host/rac1_version.h"
    if not names_path.is_file() or not header.is_file():
        return []
    table = json.loads(names_path.read_text())
    known: dict[str, dict[int, int]] = defaultdict(dict)
    for name in table.get("how", {}):  # every name the release's check or versionmap placed
        m = SYMBOL.match(name)
        if not m:
            continue
        mine = table.get("names", {}).get(name, name)
        moved = SYMBOL.match(mine)
        if moved:
            known[m.group(1) or "boot"][int(m.group(2), 16)] = int(moved.group(2), 16)
    carry = program_dir / "release_carry.json"  # versionmap's pairs a full run recorded
    if carry.is_file():
        for key, hit in json.loads(carry.read_text()).get("addresses", {}).items():
            level, _, address = key.rpartition(":")
            known[level or "boot"][int(address, 16)] = int(hit["as"], 16)
    out = []
    for line in header.read_text().splitlines():
        m = DEFINE.match(line)
        if not m:
            continue
        place, address, theirs = m.group(1) or "boot", int(m.group(2), 16), int(m.group(3), 16)
        name = f"{'boot' if place == 'boot' else 'L' + place}:{address:08X}"
        here = known.get(place, {})
        if address in here:
            if here[address] != theirs:
                out.append((name, f"release_names.json places it at {here[address]:08X}, the header at {theirs:08X}"))
            continue
        below = max((a for a in here if a < address), default=None)
        above = min((a for a in here if a > address), default=None)
        moves = {here[a] - a for a in (below, above) if a is not None}
        if below is None or above is None or moves != {theirs - address}:
            out.append((name, f"no name there, and it does not move as the named data around it "
                              f"({theirs - address:+#x}; " + ", ".join(
                                  f"{a:08X} {here[a] - a:+#x}" for a in (below, above) if a is not None) + ")"))
    return out


def check(game: str, table: bool = False, root: Path = ROOT) -> list[str]:
    spec = GAMES[game]
    source = root / spec["source"]
    errors: list[str] = []
    bound: dict[tuple[str, str], dict[str, str]] = defaultdict(dict)
    typed: dict[str, dict[str, str]] = defaultdict(dict)
    cats: dict[str, Catalogue] = {}
    reasons = read_reasons(root / "port/game" / game / "parity.tsv")
    used = set()
    # The places of the primary's names (RAC1_OMIT names a PAL v2.00 row or function).
    primary = next(iter(spec["programs"]))
    first = catalogue(spec, source, spec["programs"][primary])
    left_out: dict[str, dict[tuple[str, str], str]] = {}
    # Variants split off folded functions (split_places.tsv), each program's own: a carried
    # release's file keeps PAL v2.00's names, with its own places.
    split = {p: split_variants(root / "port/game" / p / "split_places.tsv") for p in spec["programs"]}
    for program in spec["programs"]:
        left_out[program] = {}
        for name, why in omits(root / "port/game" / program / f"host/{game}_version.h", game).items():
            for at in split[primary].get(name, [])[:1]:  # a variant split off a folded function
                if at in first.at:
                    left_out[program][(first.at[at], at[0])] = why
            at = place_of(name, {})
            owner = first.at.get(at) if at else None
            if owner is not None:  # the omit takes the function out in every place it has
                for place in first.places[owner]:
                    left_out[program][(owner, place)] = why
    for program, version in spec["programs"].items():
        cat = cats[program] = catalogue(spec, source, version)
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
            if len(cols) > 5 and cols[5].strip():
                typed[name][program] = signature(cols[5])
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
    for name, by in sorted(typed.items()):
        if len(set(by.values())) <= 1:
            continue
        why = reasons.get((name, "sig"))
        counts = list(by.values())
        common = next(v for v in by.values() if counts.count(v) == max(map(counts.count, counts)))
        odd = {p for p, v in by.items() if v != common}
        if why is not None:
            used.add((name, "sig"))
            if why[0] == odd:
                continue
        errors.append(f"{game}: {name}'s row has another C type in " + ", ".join(
            f"{p} {v}" for p, v in by.items() if p in odd) + f" than in the others ({common}); give the "
            f"reason in port/game/{game}/parity.tsv ({name}, sig, {','.join(sorted(odd))}) or fix the row")
    for program in spec["programs"]:
        for name, what in data_addresses(root / "port/game" / program):
            why = reasons.get((name, "data"))
            if why is not None and program in why[0]:
                used.add((name, "data"))
                continue
            errors.append(f"{program}: host/{game}_version.h's address {name}: {what}; give the reason in "
                          f"port/game/{game}/parity.tsv ({name}, data, {program}) or fix the address")
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
    source = ROOT / GAMES[game]["source"] / GAMES[game]["catalogue"][0].format(v=next(iter(GAMES[game]["programs"].values())))
    if args.program and not source.is_file():
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
