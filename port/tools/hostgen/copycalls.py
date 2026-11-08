#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""The catalogue's folded functions whose copies call different functions.

The catalogue's fingerprint leaves address fields out, so a wrapper of one jal is "the same
function" in every level whatever it calls: Going Commando's func_L00_003E4948 calls
func_L00_003A6C30 in level 0's build and func_L05_0041CFE8 in level 5, and the port, translating
one copy, ran level 0's moby update on level 5's moby. This reads every place's words from the
retail programs and lists each jal that differs between places, for hostgen to call through
openrac_call_in_copy (hostgen.json "call_copies"). One row per caller and callee name:

    caller  callee  home  canon  size  own  exe

home, canon: a level holding the caller and that call, and the caller's place there. size: the
caller's, in bytes. own: the callee's place in home. exe: the jal's target in the executable's copy
of the caller, or 0.

    python3 port/tools/hostgen/copycalls.py --source games/rac2/decomp \\
        --out port/game/rac2-ntsc/call_copies.tsv
"""
from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))


def elf_sections(path: Path) -> list[tuple[int, bytes]]:
    data = path.read_bytes()
    table = struct.unpack_from("<I", data, 28)[0]
    count = struct.unpack_from("<H", data, 44)[0]
    out = []
    for i in range(count):
        kind, offset, vaddr, _, size = struct.unpack_from("<5I", data, table + i * 32)
        if kind == 1:
            out.append((vaddr, data[offset:offset + size]))
    return out


def level_sections(folder: Path) -> list[tuple[int, bytes]]:
    manifest = json.loads((folder / "manifest.json").read_text())
    out = []
    for section in manifest["sections"]:
        path = folder / f"{section['name']}.bin"
        if path.exists():
            out.append((section["address"], path.read_bytes()))
    return out


def words(sections: list[tuple[int, bytes]], address: int, size: int) -> tuple[int, ...] | None:
    for start, data in sections:
        if start <= address and address + size <= start + len(data):
            return struct.unpack_from(f"<{size // 4}I", data, address - start)
    return None


def jal_target(word: int) -> int | None:
    return (word & 0x03FFFFFF) << 2 if word >> 26 == 3 else None


def exe_copy(exe: list[tuple[int, bytes]], address: int, size: int, level: tuple[int, ...]) -> tuple[int, ...] | None:
    """The executable's words of a function the levels copy, lined up with a level's copy: a name can
    start a few words before the code the levels hold (func_002DC900 is entered at 0x002DC908, and
    its level 0 copy starts there), so the words are read from the first of the next 8 places where
    they match the level's, ignoring the fields the linker fills. None when they match nowhere: the
    executable's targets are then not known."""
    want = [masked(w) for w in level]
    for skip in range(8):
        code = words(exe, address + skip * 4, size)
        if code is not None and [masked(w) for w in code] == want:
            return code
    return None


class Layout:
    """Where a release's catalogues and dumps are, and its names (release.py): v1.01's by default."""

    def __init__(self, source: Path, version: str = "us", table: Path | None = None):
        import release
        info = release.RELEASES[version]
        self.levels = source / info["config"] / "levels/functions.tsv"
        self.boot = source / info["config"] / "boot/functions.tsv"
        self.baserom = source / info["baserom"]
        self.exe = self.baserom / {"us": "SCUS_972.68", "pal": "SCES_516.07", "us2": "SCUS_972.68"}[version]
        self.name = release.Release.load(table).name if table else (lambda n: n)
        # v1.01's name of each of the release's names, and v1.01's executable and catalogue, for the
        # hook calls of v1.01's executable functions the release has only in its levels (primary_hooks).
        self.primary = {}
        if table and version != "us":
            us = release.RELEASES["us"]
            for old, new in release.Release.load(table).names.items():
                self.primary.setdefault(new, old)
            self.primary_boot = source / us["config"] / "boot/functions.tsv"
            self.primary_exe = source / us["baserom"] / release.EXECUTABLES["us"]

    def lines(self, path: Path) -> list[str]:
        """A catalogue's lines with its names in the release's names."""
        out = []
        for line in path.read_text().splitlines():
            cols = line.split("\t")
            if not line.startswith("#") and cols:
                cols[0] = self.name(cols[0])
            out.append("\t".join(cols))
        return out


def rows(source: Path, layout: "Layout | None" = None) -> list[tuple[str, str, int, int, int, int, int]]:
    layout = layout or Layout(source)
    baserom = layout.baserom
    table: list[tuple[str, str, int, list[tuple[int, int]]]] = []
    names: dict[tuple[int, int], str] = {}
    kinds: dict[str, str] = {}
    for line in layout.lines(layout.levels):
        if line.startswith("#"):
            continue
        cols = line.split("\t")
        if len(cols) < 6:
            continue
        places = [(int(p[:2]), int(p[3:], 16)) for p in cols[5].split(",") if p]
        table.append((cols[0], cols[1], int(cols[2]), places))
        kinds[cols[0]] = cols[1]
        for place in places:
            names[place] = cols[0]
    programs = {int(d.name[:2]): level_sections(d) for d in sorted((baserom / "levels").iterdir())
                if d.is_dir() and d.name[:2].isdigit()}
    exe = elf_sections(layout.exe)
    out = []
    for caller, kind, size, places in table:
        if len(places) < 2 or size < 8:
            continue
        size &= ~3
        copies = [(level, address, words(programs[level], address, size))
                  for level, address in places if level in programs]
        copies = [c for c in copies if c[2] is not None]
        own_exe = exe_copy(exe, int(caller[-8:], 16), size, copies[0][2]) if kind == "boot" and copies else None
        found: dict[str, tuple[int, int, int, int]] = {}
        for i in range(size // 4):
            targets = {(level, address): jal_target(w[i]) for level, address, w in copies}
            called = {names.get((level, t), f"?{t:X}") for (level, _), t in targets.items() if t is not None}
            exe_target = jal_target(own_exe[i]) if own_exe else None
            if len(called) == 1 and exe_target is not None:
                # Every level calls one function, but the executable's copy calls another: a level
                # function (func_L00_0030DDB0) whose executable twin the catalogue did not pair
                # (func_0029CCD8). C of the executable's function that names the level's runs the
                # level's code in the executable too; the row gives the executable its own target.
                (only,) = called
                if not (kinds.get(only) == "boot" and int(only[-8:], 16) == exe_target):
                    called = called | {f"exe:{exe_target:X}"}
            if len(called) < 2:
                continue
            for (level, address), t in targets.items():
                name = names.get((level, t)) if t is not None else None
                if name is not None and name not in found:
                    found[name] = (level, address, t, exe_target or 0)
        if own_exe is None and copies:
            for hook, i in primary_hooks(layout, caller, size, copies[0][2]):
                level, address, w = copies[0]
                if hook not in found and (t := jal_target(w[i])) is not None:
                    found[hook] = (level, address, t, 0)
        for name, (level, address, target, exe_target) in sorted(found.items()):
            out.append((caller, name, level, address, size, target, exe_target))
    return out


def primary_hooks(layout: "Layout", caller: str, size: int, level: tuple[int, ...]) -> list[tuple[str, int]]:
    """A release's level function that is v1.01's executable function (USA v2.00's func_L00_00361D08,
    the level start's cutscene, is v1.01's func_002E6D88): where v1.01's executable copy calls a
    level hook (an empty function each level fills), the levels' copies call their own function. The
    C names the hook, which a function of the executable reaches through its level copy's call
    (openrac_hook_in_copy); here there is no executable copy, so the hook's name gets the row of the
    copies' call at that place: (the hook's name in the release, the word's index)."""
    primary = layout.primary.get(caller)
    if primary is None or not hasattr(layout, "primary_exe") or not layout.primary_exe.is_file():
        return []
    if not hasattr(layout, "_primary"):
        import release
        layout._primary = ({n: a for n, a, _ in release.read_boot(layout.primary_boot)},
                           elf_sections(layout.primary_exe))
        boot = release.read_boot(layout.primary_boot)
        layout._primary_stubs = {a for _, a, z in boot if z == 8
                                 and words(layout._primary[1], a, 8) is not None
                                 and words(layout._primary[1], a, 8)[0] == release.JR_RA
                                 and words(layout._primary[1], a, 8)[1] in release.EMPTY_SLOTS}
    entries, exe = layout._primary
    if primary not in entries:
        return []
    code = exe_copy(exe, entries[primary], size, level)
    if code is None:
        return []
    out = []
    for i, w in enumerate(code):
        t = jal_target(w)
        if t is not None and t in layout._primary_stubs:
            out.append((layout.name(f"func_{t:08X}"), i))
    return out


def masked(word: int) -> int:
    """An instruction without the fields the linker fills: a jump's target, an immediate's 16 bits."""
    op = word >> 26
    if op in (2, 3):
        return word & 0xFC000000
    return word if op == 0 else word & 0xFFFF0000


def twins(source: Path, layout: "Layout | None" = None) -> list[tuple[str, int, int, list[tuple[int, int]]]]:
    """The executable's functions whose catalogue row starts away from the address their name says
    (a run of another function's leftover instructions in front: func_0031FC28, the ammo pickup's
    update, is entered at 0x0031FC60), and the level places of the code that is theirs. The level
    catalogue pairs a boot function with its copies by name, and these names do not match their
    address, so their copies are listed under level names (func_L00_003D1258): hostgen calls the
    level's own functions through them (level_hooks). One row per function:

        name  entry  size  places"""
    layout = layout or Layout(source)
    baserom = layout.baserom
    programs = {int(d.name[:2]): level_sections(d) for d in sorted((baserom / "levels").iterdir())
                if d.is_dir() and d.name[:2].isdigit()}
    exe = elf_sections(layout.exe)
    by_size: dict[int, list[tuple[str, list[tuple[int, int]]]]] = {}
    for line in layout.lines(layout.levels):
        cols = line.split("\t")
        if line.startswith("#") or len(cols) < 6:
            continue
        places = [(int(p[:2]), int(p[3:], 16)) for p in cols[5].split(",") if p]
        by_size.setdefault(int(cols[2]), []).append((cols[0], places))
    out = []
    for line in layout.lines(layout.boot):
        cols = line.split("\t")
        if line.startswith("#") or len(cols) < 3 or cols[0][-8:].upper() == cols[1].upper():
            continue
        name, entry, size = cols[0], int(cols[1], 16), int(cols[2])
        code = words(exe, entry, size & ~3)
        if code is None or size < 8 or name in {row[0] for row in by_size.get(size, [])}:
            continue
        import release
        if size == 8 and code[0] == release.JR_RA and code[1] in release.EMPTY_SLOTS:
            # An empty function (a level hook): its catalogue name is another hook's where the
            # hooks were paired by their callers (release.pair_stubs), and every level's empty
            # functions have its words.
            continue
        mine = [masked(w) for w in code]
        places: list[tuple[int, int]] = []
        for _, theirs in by_size.get(size, []):
            level, address = next(((lv, a) for lv, a in theirs if lv in programs), (None, 0))
            copy = words(programs[level], address, size & ~3) if level is not None else None
            if copy is not None and [masked(w) for w in copy] == mine:
                places += theirs
        if places:
            out.append((name, entry, size, sorted(places)))
    return out


COMPLETES = (0x09, 0x0D, 0x31, 0x39, 0x37, 0x3F, 0x1E, 0x1F)


def formed(code: tuple[int, ...], gp: int) -> dict[int, int]:
    """The addresses a function forms, by a lui and its completion or from $gp: index of the
    completing word -> address."""
    upper: dict[int, tuple[int, int]] = {}
    out = {}
    for i, w in enumerate(code):
        op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
        if op == 0x0F:
            upper[rt] = (i, w & 0xFFFF)
        elif rs == 28 and rt != 28 and op != 0x0D and (op in COMPLETES or 0x20 <= op <= 0x2B):
            low = w & 0xFFFF
            out[i] = (gp + (low - 0x10000 if low & 0x8000 else low)) & 0xFFFFFFFF
        elif rs in upper and (op in COMPLETES or 0x20 <= op <= 0x2B):
            low = w & 0xFFFF
            if op == 0x0D:
                out[i] = (upper[rs][1] << 16) | low
            else:
                out[i] = ((upper[rs][1] << 16) + (low - 0x10000 if low & 0x8000 else low)) & 0xFFFFFFFF
    return out


def data_conflicts(source: Path, low: int, high: int, gp: int, layout: "Layout | None" = None) -> list[tuple[str, int]]:
    """Functions and the level globals they form that do not pair with one address of another level:
    what is one field of a record in one level is two in another (level 0's 0x22144C is both
    0x221D34 and 0x221D4C in Feltzin), so pairing all functions gives one of them the wrong one."""
    layout = layout or Layout(source)
    baserom = layout.baserom
    programs = {int(d.name[:2]): level_sections(d) for d in sorted((baserom / "levels").iterdir())
                if d.is_dir() and d.name[:2].isdigit()}
    pairs: dict[tuple[int, int, int], set[int]] = {}
    users: dict[tuple[int, int], set[str]] = {}
    small: set[tuple[str, int]] = set()
    for line in layout.lines(layout.levels):
        cols = line.split("\t")
        if line.startswith("#") or len(cols) < 6:
            continue
        places = [(int(p[:2]), int(p[3:], 16)) for p in cols[5].split(",") if p]
        size = int(cols[2]) & ~3
        if len(places) < 2 or size < 8:
            continue
        if not cols[0].startswith("func_L"):
            continue  # the executable's names: hostgen relocates those from the executable
        home = int(cols[0].split("_")[1][1:])
        codes = {lv: words(programs[lv], a, size) for lv, a in places if lv in programs}
        if codes.get(home) is None:
            continue
        canon = formed(codes[home], gp)
        if size <= 64 and any(low <= address < high for address in canon.values()):
            # An accessor: pairing also reads a base and an offset in other functions, which in
            # another level's record is another field (level 0's 0x22144C pairs as 0x221D4C through
            # its neighbours, where Feltzin's accessor reads 0x221D34).
            small.update((cols[0], a) for a in canon.values() if low <= a < high)
        for level, code in codes.items():
            if level == home or code is None:
                continue
            mine = formed(code, gp)
            for i, address in canon.items():
                if low <= address < high and i in mine:
                    pairs.setdefault((home, level, address), set()).add(mine[i])
                    users.setdefault((home, address), set()).add(cols[0])
    out = small
    for (home, _, address), seen in pairs.items():
        if len(seen) > 1:
            out |= {(user, address) for user in users[(home, address)]}
    return sorted(out)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--data", type=Path, help="also write the functions that take globals from their copy")
    parser.add_argument("--config", type=Path, help="the game's hostgen.json")
    parser.add_argument("--twins", type=Path, help="also write the level copies of the executable's "
                        "functions entered away from their name (hostgen.json \"level_twins\")")
    parser.add_argument("--version", default="us", choices=["us", "pal", "us2"],
                        help="the release (its catalogues and dumps; tools/levels.py's --version)")
    parser.add_argument("--release-table", type=Path,
                        help="the release's table (release.py): the catalogues read in the release's names")
    args = parser.parse_args()
    layout = Layout(args.source, args.version, args.release_table)
    if args.twins:
        found_twins = twins(args.source, layout)
        args.twins.write_text(
            "# Written by port/tools/hostgen/copycalls.py --twins from the retail programs; do not edit.\n"
            "# name\tentry\tsize\tplaces\n"
            + "".join(f"{n}\t{e:08X}\t{z}\t{','.join(f'{lv:02d}:{a:08X}' for lv, a in pl)}\n"
                      for n, e, z, pl in found_twins))
        print(f"{len(found_twins)} executable functions with level copies under other names")
    found = rows(args.source, layout)
    lines = ["# Written by port/tools/hostgen/copycalls.py from the retail programs; do not edit.",
             "# caller\tcallee\thome\tcanon\tsize\town\texe"]
    lines += [f"{a}\t{b}\t{c}\t{d:08X}\t{e}\t{f:08X}\t{g:08X}" for a, b, c, d, e, f, g in found]
    args.out.write_text("\n".join(lines) + "\n")
    print(f"{len(found)} calls of {len({r[0] for r in found})} functions")
    if args.data:
        config = json.loads(args.config.read_text())
        low, high = config["relocate"][:2] if isinstance(config["relocate"], list) else (config["relocate"]["low"], config["relocate"]["high"])
        names = data_conflicts(args.source, int(str(low), 0), int(str(high), 0), int(config["asm"]["gp"], 0), layout)
        args.data.write_text("# Written by port/tools/hostgen/copycalls.py from the retail programs; do not edit.\n"
                             + "".join(f"{n}\t{a:08X}\n" for n, a in names))
        print(f"{len(names)} uses, by {len({n for n, _ in names})} functions, of globals that do not pair")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
