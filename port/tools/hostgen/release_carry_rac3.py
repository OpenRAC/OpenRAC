#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Up Your Arsenal's NTSC-U port tables, carried from PAL's (rac3-pal is the primary release).

rac3-pal's port names what it knows by PAL's functions and addresses: the host replacements and
wraps (libraries.tsv), the entry and roots, the functions read from their assembly, the shared
host C (port/game/rac3-pal/host). This writes rac3-ntsc's in NTSC-U's names, as release_carry.py
does for Going Commando's releases, from what rac3-decomp proves:

- config/us/map.tsv: each PAL function, its program (boot_elf, frontbin, i5bootn, levels) and its
  NTSC-U name; `match` (the same code), `differs` (NTSC-U's own code in that place), `none`;
- release/release_names.json (release_rac3.py): every global's NTSC-U address;
- release/release.json (by hand): the release's facts and the pairs and addresses checked by hand,
  each with its evidence.

It writes, in port/game/rac3-ntsc:

- hostgen.json: rac3-pal's, renamed, with release.json's own keys over it;
- libraries.tsv: rac3-pal's rows, the symbol renamed; a row whose function NTSC-U does not have is
  left out (listed in release_carry.json);
- host/rac3_names.tsv: the shared host C's function names (PAL's) and NTSC-U's
  (tools/hostgen/release_host.py writes the shared C with them, cmake/Games.cmake);
- host/rac3_addresses.h: each RAC3_ADDR(0x...u) the shared host C and profile write, at NTSC-U's
  address (the global of that address renamed, else release.json's `addresses`);
- release/release_carry.json: what rests on less than the same code: the host functions and wraps
  whose NTSC-U twin is other code (`differs`), the rows left out, the addresses not found.

    python3 port/tools/hostgen/release_carry_rac3.py --source games/rac3/decomp
"""
from __future__ import annotations

import argparse
import copy
import json
import re
import sys
from pathlib import Path

PORT = Path(__file__).resolve().parents[2]
PRIMARY = PORT / "game/rac3-pal"
RELEASE = PORT / "game/rac3-ntsc"

# A function or global name of the decompilation: func_0038EBC8, func_L01_00436F60,
# func_003A8F90__frontbin (frontbin's own), func_0038EBC8__game (the game's body of a wrap), D_001D89B8.
NAME = re.compile(r"\b(func|D)_(?:(L\d\d)_)?([0-9A-F]{8})(?:__(\w+))?(?![0-9A-Za-z_])")
ADDR = re.compile(r"\bRAC3_ADDR\(\s*0x([0-9A-Fa-f]{1,8})u?\s*\)")
SHARED_SUFFIXES = ("game",)  # a wrap's own body: the same program as the plain name


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


class Names:
    """PAL's function and global names -> NTSC-U's (map.tsv, release_names.json, release.json)."""

    def __init__(self, source: Path, table: dict, hand: dict):
        self.boot: dict[str, str] = {}
        self.front: dict[str, str] = {}
        self.levels: dict[str, str] = {}
        self.status: dict[tuple[str, str], str] = {}
        for line in (source / "config/us/map.tsv").read_text().splitlines():
            cols = line.split("\t")
            if line.startswith("#") or len(cols) < 7:
                continue
            pal, program, us, _place, _size, _state, status = cols[:7]
            which = {"frontbin": self.front, "levels": self.levels}.get(program, self.boot)
            kind = "frontbin" if program == "frontbin" else "levels" if program == "levels" else "boot"
            if pal in which and (us == "-" or not which[pal].startswith("func_PAL_")):
                continue  # a second row of one name: the first with a twin is its place
            which[pal] = us if us != "-" else pal.replace("func_", "func_PAL_", 1)
            self.status[(kind, pal)] = status
        self.data = {k: v for k, v in table["names"].items() if k.startswith("D_")}
        self.pairs = {k: v["is"] for k, v in hand.get("pairs", {}).items() if not k.startswith("about")}

    def function(self, name: str) -> tuple[str, str | None]:
        """NAME's NTSC-U spelling and its status (None: not a function of map.tsv)."""
        m = NAME.fullmatch(name)
        if not m:
            return name, None
        kind, level, address, suffix = m.groups()
        base = f"{kind}_{level + '_' if level else ''}{address}"
        if kind == "D":
            return (self.pairs.get(name) or self.data.get(base, base)) + (f"__{suffix}" if suffix else ""), "data"
        if name in self.pairs:
            return self.pairs[name], "hand"
        if level:
            table, which = self.levels, "levels"
        elif suffix == "frontbin":
            table, which = self.front, "frontbin"
        else:
            table, which = self.boot, "boot"
        if base not in table:
            return name, None
        new = table[base]
        return new + (f"__{suffix}" if suffix else ""), self.status.get((which, base))

    def text(self, text: str) -> str:
        return NAME.sub(lambda m: self.function(m.group(0))[0], text)


def rename_json(value, names: Names):
    """VALUE with every key and string that is one name renamed (prose left as it is)."""
    if isinstance(value, dict):
        return {(names.function(k)[0] if NAME.fullmatch(k) else k): rename_json(v, names) for k, v in value.items()}
    if isinstance(value, list):
        return [rename_json(v, names) for v in value]
    if isinstance(value, str) and NAME.fullmatch(value):
        return names.function(value)[0]
    return value


def merge(base: dict, over: dict) -> dict:
    out = dict(base)
    for k, v in over.items():
        if k.startswith("about") or k.endswith("_why"):
            continue
        if v is None:
            out.pop(k, None)
        elif isinstance(v, dict) and isinstance(out.get(k), dict) and not v.get("replace"):
            out[k] = merge(out[k], v)
        else:
            out[k] = {kk: vv for kk, vv in v.items() if kk != "replace"} if isinstance(v, dict) else v
    return out


def host_files() -> list[Path]:
    return sorted([*(PRIMARY / "host").glob("*.c"), *(PRIMARY / "host").glob("*.h"), PRIMARY / "profile.cpp"])


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--source", type=Path, required=True, help="rac3-decomp (games/rac3/decomp)")
    args = ap.parse_args(argv)
    source = args.source.resolve()
    hand = json.loads((RELEASE / "release/release.json").read_text())
    table = json.loads((RELEASE / "release/release_names.json").read_text())
    names = Names(source, table, hand)
    report: dict = {"about": "What rac3-ntsc's carried tables rest on besides the same code, written by "
                             "port/tools/hostgen/release_carry_rac3.py; do not edit.",
                    "differs": {}, "left_out": {}, "addresses_not_found": [], "hand_addresses": {}}

    # hostgen.json: PAL's, renamed, then the release's own keys.
    primary = json.loads((PRIMARY / "hostgen.json").read_text())
    carried = rename_json(copy.deepcopy(primary), names)
    out = {"about": "Written by port/tools/hostgen/release_carry_rac3.py from port/game/rac3-pal/hostgen.json, "
                    "config/us/map.tsv and release/release.json; do not edit by hand (release_carry.json "
                    "says what rests on less than the same code)."}
    out.update(merge(carried, hand.get("hostgen", {})))
    (RELEASE / "hostgen.json").write_text(json.dumps(out, indent=2) + "\n")

    # libraries.tsv: the rows renamed.
    lines = (PRIMARY / "libraries.tsv").read_text().splitlines()
    rows = ["# Written by port/tools/hostgen/release_carry_rac3.py from port/game/rac3-pal/libraries.tsv in",
            "# NTSC-U's names (config/us/map.tsv); do not edit. The notes are PAL's, its addresses as PAL has them.",
            "# Rows of a function NTSC-U does not have are left out (release/release_carry.json)."]
    drop = set(hand.get("libraries_left_out", {}))
    for line in lines:
        if line.startswith("#") or not line.strip():
            if line.startswith("# symbol"):
                rows.append(line)
            continue
        cols = line.split("\t")
        new, status = names.function(cols[0])
        if cols[0] in drop:
            report["left_out"][cols[0]] = hand["libraries_left_out"][cols[0]]
            continue
        if new.startswith("func_PAL_") or status == "none":
            report["left_out"][cols[0]] = f"NTSC-U has no such function ({status})"
            continue
        if status not in ("match", "asm", None, "data", "hand"):
            report["differs"][cols[0]] = {"is": new, "status": status, "port": cols[3] if len(cols) > 3 else ""}
        cols[0] = new
        rows.append("\t".join(cols))
    (RELEASE / "libraries.tsv").write_text("\n".join(rows) + "\n")

    # host/rac3_names.tsv: the shared host C's function names.
    used: dict[str, str] = {}
    for path in host_files():
        if path.suffix == ".cpp":
            continue
        for m in NAME.finditer(strip_comments(path.read_text())):
            if m.group(1) != "func":
                continue
            new, status = names.function(m.group(0))
            if new != m.group(0):
                used[m.group(0)] = new
                if status not in ("match", "asm", "hand"):
                    report["differs"].setdefault(m.group(0), {"is": new, "status": status, "port": "host C"})
    (RELEASE / "host").mkdir(exist_ok=True)
    (RELEASE / "host/rac3_names.tsv").write_text(
        "# The shared host C's function names (PAL's, port/game/rac3-pal/host) and NTSC-U's. Written by\n"
        "# port/tools/hostgen/release_carry_rac3.py from config/us/map.tsv; do not edit.\n"
        + "".join(f"{k}\t{v}\n" for k, v in sorted(used.items())))

    # host/rac3_addresses.h: every RAC3_ADDR the shared C and the profile write.
    addresses = {}
    hand_addresses = {k: v for k, v in hand.get("addresses", {}).items() if not k.startswith("about")}
    for path in host_files():
        for m in ADDR.finditer(strip_comments(path.read_text())):
            pal = int(m.group(1), 16)
            key = f"0x{pal:08X}u"
            if key in addresses:
                continue
            hand_row = hand_addresses.get(f"0x{pal:08X}")
            if hand_row:
                addresses[key] = (int(hand_row["is"], 16), "release.json: " + hand_row["why"])
                report["hand_addresses"][f"0x{pal:08X}"] = hand_row
                continue
            found = None
            for kind in ("D", "func"):
                new, status = names.function(f"{kind}_{pal:08X}")
                if new != f"{kind}_{pal:08X}" or (status is not None and status != "data"):
                    if "PAL" not in new:
                        found = (int(new.split("_")[-1], 16), f"{kind}_{pal:08X} ({'map.tsv ' + status if kind == 'func' else 'names.tsv'})")
                        break
                elif kind == "D" and f"D_{pal:08X}" in names.data:
                    found = (pal, f"D_{pal:08X} (names.tsv, the same address)")
                    break
                elif kind == "func" and status == "match":
                    found = (pal, f"func_{pal:08X} (map.tsv, the same place)")
                    break
            if found is None:
                report["addresses_not_found"].append(f"0x{pal:08X}")
                addresses[key] = (None, "not known on NTSC-U")
            else:
                addresses[key] = found
    head = ("/* Written by port/tools/hostgen/release_carry_rac3.py; do not edit. */\n"
            "/* SPDX-License-Identifier: GPL-3.0-or-later\n"
            " * Copyright (c) 2026 the OpenRAC contributors\n"
            " *\n"
            " * NTSC-U's address for each PAL address the shared host C writes as RAC3_ADDR(0x...u) (rac3_version.h):\n"
            " * the global or function of that address renamed (config/us/names.tsv, config/us/map.tsv), else\n"
            " * release/release.json's `addresses`. One not known is 0: what reads it reads nothing of the game's. */\n"
            "#ifndef OPENRAC_RAC3_ADDRESSES_H\n#define OPENRAC_RAC3_ADDRESSES_H\n\n")
    body = "".join(f"#define RAC3_ADDR_{k} {('0x%08Xu' % v[0]) if v[0] is not None else '0u'} /* {v[1]} */\n"
                   for k, v in sorted(addresses.items()))
    (RELEASE / "host/rac3_addresses.h").write_text(head + body + "\n#endif /* OPENRAC_RAC3_ADDRESSES_H */\n")

    report["differs"] = dict(sorted(report["differs"].items()))
    (RELEASE / "release/release_carry.json").write_text(json.dumps(report, indent=1) + "\n")
    print(f"rac3-ntsc: {len(rows) - 4} library rows ({len(report['left_out'])} left out), {len(used)} host names, "
          f"{len(addresses)} addresses ({len(report['addresses_not_found'])} not found), "
          f"{len(report['differs'])} on other code")
    return 0


if __name__ == "__main__":
    sys.exit(main())
