# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""The retail program's instructions, read from the player's own executable and level programs.

asmc.py writes C for the functions with no C from their instructions. The decompilations keep
those as assembly files their setup writes (asm.py reads them), which only a machine that ran the
decompilation's setup has. This reads the same instructions from the program itself, wherever the
port is built from the player's disc:

- the executable (an ELF: its code sections, and its data for jump tables), and
- each level's program (the records ParseBin loads over the executable from `relocate`'s low end
  up: lit, data, the class tables, text), from level dumps (a folder per level with its
  manifest.json and records: rac1-decomp's level_NN, rac2-decomp's NN_name) or from the disc image itself (the
  level's overlay section, through the editor's own reader, editor/disc.py and formats.py).

What the assembly's text says beyond the words, this works out the way spimdisasm does:

- where functions start: every function hostgen knows, by its places;
- which immediates are halves of an address: a lui and the addiu, ori, load or store that
  completes it on the same register before that register is written again. An address in the
  program's code is the function there (and an offset into it); one in game RAM is a global,
  named as the decompilations name them (D_<addr>, D_Lnn_<addr> for a level's own data from
  `relocate`'s low end up), so hostgen relocates it as it does the C's;
- jump tables: a jr through a register loaded from such an address after an addu with an index,
  with the words there that are addresses in the same function.

Nothing is written or kept: the bytes are read when the port is built, and only C comes out.
"""
from __future__ import annotations

import bisect
import json
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

sys.path.insert(1, str(next(p for p in Path(__file__).resolve().parents if (p / "tools/openrac_common").is_dir()) / "tools"))
from openrac_common import paths  # noqa: E402

import asm  # noqa: E402

RAM = (0x00100000, 0x02000000)
# A Ratchet & Clank level program's records in their order, as the level dumps name them.
RECORD_NAMES = ("lit", "bss", "data", "vtbl", "camvtbl", "sndvtbl", "text")
LOADS_STORES = {"lb", "lbu", "lh", "lhu", "lw", "lwu", "ld", "lq", "lwl", "lwr", "ldl", "ldr", "sb", "sh", "sw",
                "sd", "sq", "swl", "swr", "sdl", "sdr", "lwc1", "swc1", "lqc2", "sqc2"}


@dataclass
class Part:
    name: str
    base: int
    data: bytes
    code: bool

    def holds(self, addr: int, size: int = 4) -> bool:
        return self.base <= addr and addr + size <= self.base + len(self.data)

    def word(self, addr: int) -> int:
        return struct.unpack_from("<I", self.data, addr - self.base)[0]


class Retail:
    """The executable's sections and each level's records."""

    def __init__(self, exe: bytes | None, levels: dict[int, list[Part]], low: int, label: str = "retail",
                 named: dict[str, list[Part]] | None = None):
        # A loader that carries the game packed (Up Your Arsenal's) is read as the game it unpacks.
        packed = packed_program(exe) if exe else None
        self.exe: list[Part] = packed[0] if packed else elf_parts(exe) if exe else []
        self.entry = packed[1] if packed else struct.unpack_from("<I", exe, 0x18)[0] if exe else None
        self.levels = levels
        self.low = low
        # The disc's other programs by name (Up Your Arsenal's frontbin), which hostgen.json's
        # "asm" "retail" "programs" places in an overlay of their own (place_named).
        self.named = named or {}
        self.label = label

    @classmethod
    def from_files(cls, exe: Path, dump: Path | None, low: int) -> "Retail":
        return cls(exe.read_bytes(), read_dump(dump) if dump else {}, low, exe.name)

    @classmethod
    def from_image(cls, image: Path, low: int, executable: str | None = None) -> "Retail":
        exe, levels = read_image(image)
        if executable is not None:
            # Another executable of the disc (hostgen.json "asm" "retail" "executable"), by its
            # name in the disc's root: I5BOOTN.ELF.
            from openrac_common import iso9660
            exe = iso9660.read(image, executable)
        return cls(exe, levels, low, f"{image.name}:{executable}" if executable else image.name,
                   read_named_programs(image))

    def pointed(self, levels: set[int], known: set[tuple[int, int]] = frozenset()) -> set[tuple[int, int]]:
        """(program, address) for each word of a program's data (the executable's, and each of
        `levels`) that is an address in code where a function starts: after the end of another (a
        jr ra or a j and its delay slot, then any zeros), or where a frame is set up before any
        branch. The words of the jump tables the code indexes are left out: their targets are
        inside a function, and some follow a return. Nor is an address inside a function that starts
        before it (`known`, the entries its calls found): one whose code branches past it, or code
        after it that branches back before it (a switch's case, reached through a table the scan
        did not see)."""
        out: set[tuple[int, int]] = set()
        starts: dict[int, list[int]] = {}
        for h, a in known:
            starts.setdefault(h, []).append(a)
        for h in starts:
            starts[h].sort()

        def branch_target(w, at):
            op = w >> 26
            if op in (4, 5, 6, 7, 0x14, 0x15, 0x16, 0x17) or (op == 1 and ((w >> 16) & 31) in (0, 1, 2, 3)):
                off = w & 0xFFFF
                return at + 4 + 4 * (off - 0x10000 if off & 0x8000 else off)
            if op == 2:
                return (at & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
            return None

        def inside(parts, h, a):
            st = starts.get(h, [])
            i = bisect.bisect_left(st, a)
            before = st[i - 1] if i > 0 else None
            after = st[i] if i < len(st) and st[i] != a else (st[i + 1] if i + 1 < len(st) else None)
            if before is None or a - before > 0x8000:
                return False
            for at in range(before, a, 4):
                w = word_in(parts, at)
                t = branch_target(w, at) if w is not None else None
                if t is not None and t > a and (after is None or t < after):
                    return True
            end = min(after if after is not None else a + 0x8000, a + 0x8000)
            for at in range(a, end, 4):
                w = word_in(parts, at)
                t = branch_target(w, at) if w is not None else None
                if t is not None and before <= t < a:
                    return True
            return False

        homes = [-1] + sorted(h for h in levels if h in self.levels)
        code = {h: self.code_parts(h) for h in homes}
        exe_code = code[-1]

        def word_in(parts, a):
            for q in parts:
                if q.holds(a):
                    return q.word(a)
            return None

        def starts_here(parts, a):
            k, steps = a - 4, 0
            while steps < 4 and word_in(parts, k) == 0:
                k -= 4
                steps += 1
            for at in (k, k - 4):
                w = word_in(parts, at)
                if w is not None and (w == 0x03E00008 or w >> 26 == 2):
                    return True
            # Or a leaf that only returns (jr ra, its result in the delay slot: a callback answering
            # a constant), or a frame set up in its first instructions (addiu $sp, $sp, -N before any branch):
            # behind the function before it, a return path's last instructions, not its jr ra.
            if word_in(parts, a) == 0x03E00008:
                return True
            for i in range(4):
                w = word_in(parts, a + 4 * i)
                if w is None or (w >> 26) in (1, 2, 3, 4, 5, 6, 7, 0x14, 0x15, 0x16, 0x17) \
                        or (w >> 26 == 0 and (w & 0x3E) == 0x08):
                    return False
                if w >> 16 == 0x27BD and w & 0x8000:
                    return True
            return False

        for home in homes:
            tables: set[int] = set()
            for p in code[home]:
                n = len(p.data) // 4
                for (_jr, table) in RetailCode.table_scan(p.base, list(struct.unpack_from(f"<{n}I", p.data))):
                    tables.add(table)
            own = self.exe if home < 0 else self.levels.get(home, [])
            data_parts = [p for p in own if not p.code]
            table_words: set[int] = set()
            for t in tables:
                k = 0
                while k < 1024:
                    w = None
                    for q in data_parts + code[home]:
                        if q.holds(t + 4 * k):
                            w = q.word(t + 4 * k)
                            break
                    if w is None or not any(c.holds(w) for c in code[home]):
                        break
                    table_words.add(t + 4 * k)
                    k += 1
            for p in data_parts:
                n = len(p.data) // 4
                for k, w in enumerate(struct.unpack_from(f"<{n}I", p.data)):
                    if w & 3 or w < RAM[0] or w >= RAM[1] or p.base + 4 * k in table_words:
                        continue
                    for parts, h in ((code[home], home), (exe_code, -1)):
                        if any(c.holds(w) for c in parts):
                            owner = h if h < 0 or w >= self.low else -1
                            if starts_here(parts, w) and not inside(parts, owner, w):
                                out.add((owner, w))
                            break
        return out

    def place_named(self, programs: dict[str, int]) -> None:
        """The named programs as the overlays hostgen gives them ({"frontbin": 100})."""
        for name, overlay in programs.items():
            if name in self.named:
                self.levels[int(overlay)] = self.named[name]

    def program(self, level: int) -> list[Part]:
        """Everything in game memory while program `level` (-1 the executable) is loaded."""
        if level < 0:
            return self.exe
        out = [Part(p.name, p.base, p.data[:max(0, self.low - p.base)], p.code) for p in self.exe
               if p.base < self.low]
        return out + self.levels.get(level, [])

    def code_parts(self, home: int) -> list[Part]:
        """The code of program `home` itself (a level's own text, not the executable's under it)."""
        return [p for p in (self.exe if home < 0 else self.levels.get(home, [])) if p.code]

    def called(self, *, include_jumps: bool = True) -> set[tuple[int, int]]:
        """(program, address) every jal and j of each program's own code goes to, and every code
        address it forms with a lui and an addiu or ori (a callback it hands on)."""
        out = set()
        for home in [-1] + sorted(self.levels):
            parts = self.code_parts(home)
            in_code = lambda t: any(q.holds(t) for q in parts) or any(q.holds(t) for q in self.code_parts(-1))
            for p in parts:
                n = len(p.data) // 4
                hi: dict[int, tuple[int, int]] = {}
                for k, w in enumerate(struct.unpack_from(f"<{n}I", p.data)):
                    op = w >> 26
                    t = None
                    if op == 3 or (op == 2 and include_jumps):
                        t = ((p.base + 4 * k) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
                    elif op == 0x0F:
                        hi[(w >> 16) & 31] = ((w & 0xFFFF) << 16, k)
                    elif op in (0x09, 0x0D) and ((w >> 21) & 31) in hi and k - hi[(w >> 21) & 31][1] <= 8:
                        value = hi[(w >> 21) & 31][0]
                        imm = w & 0xFFFF
                        t = (value | imm) if op == 0x0D else (value + (imm - 0x10000 if imm & 0x8000 else imm)) & 0xFFFFFFFF
                        if not in_code(t):
                            t = None
                    if t is not None:
                        # Below `low` is the executable's code whatever program is loaded.
                        out.add((home if t >= self.low else -1, t))
        return out

    def word(self, level: int, addr: int) -> int | None:
        for p in self.program(level):
            if p.holds(addr):
                return p.word(addr)
        return None


def elf_parts(b: bytes) -> list[Part]:
    """An EE executable's loaded sections (its code, and its data), or its segments without them."""
    out = []
    shoff, = struct.unpack_from("<I", b, 0x20)
    shnum, shstrndx = struct.unpack_from("<HH", b, 0x30)
    if shnum and shstrndx < shnum and shoff + 40 * shnum <= len(b):
        stro = struct.unpack_from("<I", b, shoff + 40 * shstrndx + 16)[0]
        for i in range(shnum):
            nm, ty, fl, ad, of, sz = struct.unpack_from("<6I", b, shoff + 40 * i)
            name = b[stro + nm:b.index(b"\0", stro + nm)].decode(errors="replace")
            # Loaded (SHF_ALLOC), with contents (not NOBITS); the VU microcode is not EE code.
            if ad and sz and ty != 8 and fl & 2 and name != ".vutext":
                out.append(Part(name, ad, b[of:of + sz], bool(fl & 4)))
    if not out:
        phoff, = struct.unpack_from("<I", b, 0x1C)
        for i in range(struct.unpack_from("<H", b, 0x2C)[0]):
            kind, off, va, _pa, size = struct.unpack_from("<5I", b, phoff + 32 * i)
            if kind == 1:
                out.append(Part("load", va, b[off:off + size], True))
    return out


# The sections of Up Your Arsenal's packed game, in the payload's order: name and ELF flags, as
# rac3-decomp's tools/levels.py (BOOT_SECTIONS) gives them for the boot_elf.elf it writes from it.
PACKED_SECTIONS = (
    (".reginfo", 0x0), (".vutext", 0x6), ("core.text", 0x6), ("core.data", 0x3), ("core.rdata", 0x2),
    ("core.bss", 0xF0000003), ("core.lit", 0xF0000003), (".lit", 0xF0000003), (".bss", 0xF0000003),
    (".data", 0x3), ("lvl.vtbl", 0x2), ("lvl.camvtbl", 0x2), ("lvl.sndvtbl", 0x2), (".text", 0x6),
    ("patch.data", 0x6), ("legal.data", 0x3), ("mc1.data", 0x3), ("mc1.data", 0x3))


def editor_modules() -> None:
    """The editor's readers (editor/disc.py, formats.py, level.py, wads.py) on the import path."""
    editor = paths.repo_root(__file__) / "editor"
    if str(editor) not in sys.path:
        sys.path.insert(0, str(editor))


def packed_program(b: bytes) -> tuple[list[Part], int] | None:
    """The game a loader executable carries WAD-compressed in its data, unpacked, with its entry:
    Up Your Arsenal's SCES_524.56 and SCUS_973.53 (i5bootn's loader at 0x00800000) hold the whole
    game as the records level programs are made of (address, size, type, entry; then the bytes),
    which the loader copies to their addresses before it jumps to the entry. The parts are the
    sections elf_parts gives for the ELF rac3-decomp's setup writes from it (boot_elf.elf): loaded,
    with contents, the VU microcode left out. None for any other executable."""
    editor_modules()
    from formats import FormatError, overlay_sections, wad  # noqa: E402
    at = b.find(b"WAD")
    while at >= 0:
        size, = struct.unpack_from("<I", b, at + 3) if at + 16 <= len(b) else (0,)
        if 16 < size and at + size <= len(b) and b[at + 7:at + 16] == bytes(9):
            try:
                payload = wad(b[at:at + size])
                sections = overlay_sections(payload)
            except FormatError:
                sections = None
            if sections is not None:
                return packed_parts(payload, sections), sections["entry_point"]
        at = b.find(b"WAD", at + 1)
    return None


def packed_parts(payload: bytes, sections: dict) -> list[Part]:
    recs = sections["sections"]
    named = len(recs) == len(PACKED_SECTIONS)
    out = []
    for k, rec in enumerate(recs):
        name, flags = PACKED_SECTIONS[k] if named else (f"record{k}", 0x6 if rec["address"] <= sections[
            "entry_point"] < rec["address"] + rec["bytes"] else 0x3)
        if rec["address"] and rec["bytes"] and rec["type"] != 8 and flags & 2 and name != ".vutext":
            out.append(Part(name, rec["address"], payload[rec["offset"]:rec["offset"] + rec["bytes"]],
                            bool(flags & 4)))
    return out


def manifest_records(manifest: dict) -> list[dict]:
    """A level manifest's records: "records" in rac1-decomp's dumps, "sections" in rac2-decomp's."""
    return manifest.get("records") or manifest["sections"]


def level_dump_dir(dump: Path, level: int) -> Path | None:
    """The folder of LEVEL's dump: level_NN, else the one whose manifest names the level."""
    named = dump / f"level_{level:02d}"
    if named.is_dir():
        return named
    for d in sorted(dump.iterdir()):
        man = d / "manifest.json"
        if man.is_file() and json.loads(man.read_text()).get("level") == level:
            return d
    return None


def read_dump(dump: Path) -> dict[int, list[Part]]:
    levels = {}
    for d in sorted(dump.iterdir()) if dump.is_dir() else ():
        man = d / "manifest.json"
        if not man.is_file():
            continue
        m = json.loads(man.read_text())
        parts = []
        for r in manifest_records(m):
            f = d / f"{r['name']}.bin"
            if r.get("type") == 1 and f.is_file():
                parts.append(Part(r["name"], r["address"], f.read_bytes(), r["name"] == "text"))
        levels[int(m["level"])] = parts
    return levels


def read_named_programs(image: Path) -> dict[str, list[Part]]:
    """The disc's programs other than the executable and the levels (read_wad_programs), or none
    for a disc the editor's reader of the later games does not know."""
    try:
        return read_wad_programs(image)[1]
    except (OSError, ValueError):
        return {}


def read_image(image: Path) -> tuple[bytes, dict[int, list[Part]]]:
    """The executable and every level's program from a disc image, through the editor's reader."""
    editor_modules()
    from disc import Disc, FormatError  # noqa: E402  (the editor's ISO and table-of-contents reader)
    from formats import overlay_sections, unpack  # noqa: E402
    from level import decoded  # noqa: E402
    levels: dict[int, list[Part]] = {}
    try:
        with Disc(image) as disc:
            disc.identify()
    except FormatError:
        return read_boot_executable(image), read_wad_levels(image)
    with Disc(image) as disc:
        exe = disc.executable()
        for info in disc.survey()["levels"]:
            r = info["ranges"]["data"]
            # The level data header's first pair is the overlay: read that much, not the level.
            offset, size = unpack("<ii", disc.sectors(r["lba"], 1), 0)
            if not size or offset < 0 or offset + size > r["sectors"] * 2048:
                continue
            parts = overlay_parts(decoded(disc.read(r["lba"] * 2048 + offset, size)))
            levels[int(info["id"])] = parts
    return exe, levels


def overlay_parts(overlay: bytes) -> list[Part]:
    """The parts of a level's program, as ParseBin's records hold them."""
    from formats import overlay_sections  # noqa: E402
    sections = overlay_sections(overlay)
    recs = sections["sections"]
    names = RECORD_NAMES if len(recs) == len(RECORD_NAMES) else [f"record{k}" for k in range(len(recs))]
    parts = []
    for name, rec in zip(names, recs):
        if rec["type"] != 1:
            continue
        body = overlay[rec["offset"]:rec["offset"] + rec["bytes"]]
        code = rec["address"] <= sections["entry_point"] < rec["address"] + rec["bytes"]
        parts.append(Part("text" if code else name, rec["address"], body, code))
    return parts


def read_wad_levels(image: Path) -> dict[int, list[Part]]:
    """Each level's program from Going Commando's or Up Your Arsenal's disc, through the editor's
    reader of the later games' discs. Deadlocked's port reads no level programs yet."""
    return read_wad_programs(image)[0]


# Up Your Arsenal's table of contents: the MISC wad's global header (0x48 bytes) at this offset, its
# sector at +4 and the front end's program (frontbin) at +0x20, a sector offset and count in it
# (rac3-decomp's tools/levels.py, read_core).
RAC3_MISC_HEADER = 0x648
RAC3_FRONTBIN = 0x20


def read_wad_programs(image: Path) -> tuple[dict[int, list[Part]], dict[str, list[Part]]]:
    """Each level's program by the level's number, and the disc's other programs by name (Up Your
    Arsenal's frontbin, the front end the boot program loads over its own when a level is left)."""
    editor_modules()
    from disc import Disc, FormatError  # noqa: E402
    from formats import unpack  # noqa: E402
    from level import decoded  # noqa: E402
    import wads  # noqa: E402
    levels: dict[int, list[Part]] = {}
    named: dict[str, list[Part]] = {}
    try:
        with Disc(image) as disc:
            version = wads.identify(disc)
            if version is None or version.game not in ("rac2", "rac3"):
                return levels, named
            wad = wads.WadDisc(disc, version)
            for entry in wad.levels():
                part = entry.get(wads.LEVEL)
                if part is None:
                    continue
                r = wads.level_ranges(part["header"], version)["data"]
                start = part["lba"] + r["sector"]
                offset, size = unpack("<ii", disc.sectors(start, 1), 0)
                if size > 0:
                    levels[int(entry["id"])] = overlay_parts(decoded(disc.read(start * 2048 + offset, size)))
            if version.game == "rac3":
                size, sector = unpack("<ii", wad.toc, RAC3_MISC_HEADER)
                if size == 0x48:
                    first, count = unpack("<ii", wad.toc, RAC3_MISC_HEADER + RAC3_FRONTBIN)
                    named["frontbin"] = overlay_parts(records_only(disc.sectors(sector + first, count)))
    except FormatError as e:
        print(f"hostgen: {image}: its level programs cannot be read: {e}", file=sys.stderr)
    return levels, named


def records_only(raw: bytes) -> bytes:
    """A program's records (address, size, type, entry; then the bytes) without the zeros that pad
    its last sector."""
    at = 0
    while at + 16 <= len(raw):
        address, size = struct.unpack_from("<2I", raw, at)
        if address == 0 and size == 0:
            break
        at += 16 + size
    return raw[:min(at, len(raw))]


def read_boot_executable(image: Path) -> bytes:
    """The executable SYSTEM.CNF boots, read from a disc image of a game the editor's reader does not
    know (Going Commando's), through the generic ISO 9660 reader: only that file, never a copy of
    the image."""
    from openrac_common import iso9660
    serial = iso9660.boot_serial(image)
    if serial is None:
        raise ValueError(f"{image}: not a PS2 disc image (no BOOT2 line in its SYSTEM.CNF)")
    try:
        return iso9660.read(image, serial)
    except FileNotFoundError:
        raise ValueError(f"{image}: its boot executable {serial} is not on the disc") from None


# ---- The instructions, as asm.Index has them ---------------------------------------------------

class RetailCode:
    """(program, address) -> (asm.Line, asm.Block), for every word of a program's code: the
    mapping asm.Index.code is, read from the retail bytes when first asked for."""

    def __init__(self, retail: Retail, starts: dict[int, dict[int, str]], low: int):
        self.retail = retail
        self.low = low
        self.starts = starts                      # program -> {address: name}
        self._sorted = {h: sorted(m) for h, m in starts.items()}
        self._blocks: dict[tuple[int, int], asm.Block] = {}
        self._pairs: dict[tuple[int, int], tuple[dict, dict]] = {}
        self._lines: dict[tuple[int, int], tuple[asm.Line, asm.Block]] = {}
        self.follow_far = False                   # _follow's `far`
        self.pair_moves = False                   # pairs: a register move carries a %hi along

    def __contains__(self, key) -> bool:
        return self.get(key) is not None

    def get(self, key, default=None):
        hit = self._lines.get(key)
        if hit is not None:
            return hit
        home, addr = key
        if addr & 3:
            return default
        part = next((p for p in self.retail.code_parts(home) if p.holds(addr)), None)
        if part is None:
            return default
        starts = self._sorted.get(home, [])
        i = bisect.bisect_right(starts, addr) - 1
        start = starts[i] if i >= 0 and starts[i] >= part.base else part.base
        end = starts[i + 1] if i + 1 < len(starts) else part.base + len(part.data)
        end = min(end, part.base + len(part.data))
        self._block(home, part, start, end)
        return self._lines.get(key, default)

    def _block(self, home: int, part: Part, start: int, end: int) -> None:
        if (home, start) in self._blocks:
            return
        name = self.starts.get(home, {}).get(start, f"code_{start:08X}")
        block = asm.Block(name, Path(f"<{self.retail.label}>"))
        words = [part.word(a) for a in range(start, end, 4)]
        # The pairs over the whole of the program's code at once: a catalogue's function boundary
        # may fall between a lui and its other half (a function it cut in two).
        key = (home, part.base)
        if key not in self._pairs:
            n = len(part.data) // 4
            self._pairs[key] = self.pairs(home, part.base, list(struct.unpack_from(f"<{n}I", part.data)))
        all_relocs, all_tables = self._pairs[key]
        relocs = {a: r for a, r in all_relocs.items() if start <= a < end}
        tables = {a: t for a, t in all_tables.items() if start <= a < end}
        for k, w in enumerate(words):
            a = start + 4 * k
            line = asm.Line(a, w, "", relocs.get(a))
            block.lines.append(line)
            self._lines[(home, a)] = (line, block)
        block.entries[name] = start
        for jr, table in tables.items():
            targets = []
            for k in range(1024):
                t = self.retail.word(home, table + 4 * k)
                if t is None or t & 3 or not start <= t < end:
                    break
                targets.append(t)
            if targets:
                block.tables[self._table_name(home, table)] = targets
        self._blocks[(home, start)] = block

    def _table_name(self, home: int, address: int) -> str:
        return f"jtbl_L{home:02d}_{address:08X}" if home >= 0 and address >= self.low else f"jtbl_{address:08X}"

    def _symbol(self, home: int, address: int, table: bool) -> tuple[str, int] | None:
        """The symbol a %hi/%lo pair names: the function (and offset) for a code address, the jump
        table, or the global the decompilations would name there."""
        if table:
            return self._table_name(home, address), 0
        # A function there: a level's own from `low` up, the executable's below it (or in its own
        # program). An address inside a function's code is data kept there (a table), as
        # spimdisasm names it.
        h = home if home < 0 or address >= self.low else -1
        if address in self.starts.get(h, {}):
            return self.starts[h][address], 0
        if not RAM[0] <= address < RAM[1]:
            return None
        if home >= 0 and address >= self.low:
            return f"D_L{home:02d}_{address:08X}", 0
        return f"D_{address:08X}", 0

    @staticmethod
    def table_scan(start: int, words: list[int]) -> list[tuple[int, int]]:
        """(jr address, table address) of the jump tables a run of code indexes: a jr through a
        register loaded by a lw from a lui-formed address after an addu with an index (as pairs
        finds them, without naming anything)."""
        hi: dict[int, int] = {}
        idx: dict[int, int] = {}
        loaded: dict[int, int] = {}
        out = []
        for k, w in enumerate(words):
            op, rs, rt, rd = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
            imm = w & 0xFFFF
            simm = imm - 0x10000 if imm & 0x8000 else imm
            if op == 0x0F:
                hi[rt] = imm << 16
                continue
            if op == 0x09 and rs in hi:                           # addiu %lo(table)
                hi[rt] = (hi[rs] + simm) & 0xFFFFFFFF
                continue
            if op == 0 and (w & 63) in (0x21, 0x2D) and rd:       # addu / daddu with an index
                if (rs in hi) != (rt in hi):
                    idx[rd] = hi[rs if rs in hi else rt]
                    continue
            if op == 0x23 and rs in idx:                          # lw
                loaded[rt] = (idx[rs] + simm) & 0xFFFFFFFF
                continue
            if op == 0 and (w & 63) == 0x08 and rs != 31 and rs in loaded:
                out.append((start + 4 * k, loaded[rs]))
            if op == 0 and (w & 63) == 0x08:
                hi.clear(); idx.clear(); loaded.clear()
        return out

    def pairs(self, home: int, start: int, words: list[int]):
        """The %hi/%lo relocations of a run of code's words, and its jump tables (jr address ->
        table address)."""
        from asmc import rw   # (asmc imports this module)
        hi: dict[int, tuple[int, int]] = {}       # register -> (high half, lui address)
        full: dict[int, int] = {}                 # register -> an address a pair completed in it
        idx: dict[int, tuple] = {}                # register -> (high half, lui address) or (address,) plus an index
        loaded: dict[int, int] = {}               # register -> the table a word was loaded from
        tables: dict[int, int] = {}
        uses: dict[int, list] = {}
        leave_after = None                        # the delay slot of a jr or j: nothing goes on past it
        insns = [asm.decode(w, start + 4 * k) for k, w in enumerate(words)]
        writes = [rw(i)[1] for i in insns]
        for k, w in enumerate(words):
            pc = start + 4 * k
            if leave_after is not None and pc > leave_after:
                hi.clear(); full.clear(); idx.clear(); loaded.clear()
                leave_after = None
            ins = insns[k]
            op = ins.op
            if op in ("jr", "j"):
                leave_after = pc + 4
            new = []
            if op == "lui":
                new.append((hi, ins.rt, ((w & 0xFFFF) << 16, pc)))
            elif op in LOADS_STORES or op in ("addiu", "daddiu", "ori"):
                imm = ins.imm if op != "ori" else (w & 0xFFFF)
                if ins.rs in hi:
                    value, at = hi[ins.rs]
                    s = (value | imm) if op == "ori" else (value + imm) & 0xFFFFFFFF
                    uses.setdefault(at, []).append(s)
                    if op in ("addiu", "daddiu", "ori"):
                        new.append((full, ins.rt, s))
                elif ins.rs in idx and op == "lw":
                    base = idx[ins.rs]
                    if len(base) == 2:              # lui, addu index, lw %lo(table)
                        s = (base[0] + imm) & 0xFFFFFFFF
                        uses.setdefault(base[1], []).append(s)
                    else:                           # lui, addiu %lo(table), addu index, lw 0
                        s = (base[0] + imm) & 0xFFFFFFFF
                    new.append((loaded, ins.rt, s))
            elif (self.pair_moves and op in ("addu", "daddu", "or") and ins.rd
                  and 0 in (ins.rs, ins.rt) and (ins.rs or ins.rt) in hi):
                # A move of a lui's register (hostgen.json asm.pair_through_moves): the copy is
                # completed later (Up Your Arsenal's func_L01_00462978 keeps `lui $s6, %hi(f)` and
                # forms f's address in $a2 after `daddu $a2, $s6, $zero`).
                new.append((hi, ins.rd, hi[ins.rs or ins.rt]))
            elif op in ("addu", "daddu") and ins.rd:
                for maps, wrap in ((hi, lambda v: v), (full, lambda v: (v,))):
                    if (ins.rs in maps) != (ins.rt in maps):
                        new.append((idx, ins.rd, wrap(maps[ins.rs if ins.rs in maps else ins.rt])))
                        break
            if op == "jr" and ins.rs != 31 and ins.rs in loaded:
                tables[pc] = loaded[ins.rs]
            if k > 0 and insns[k - 1].is_branch and insns[k - 1].likely:
                # A likely branch's delay slot runs only when it is taken: the straight run (its
                # fall-through) never sees what the slot does. A lui there is completed at the
                # branch's target, which the follow below finds.
                continue
            for r in writes[k]:
                for m in (hi, full, idx, loaded):
                    m.pop(r, None)
            for m, r, v in new:
                m[r] = v
        # A lui whose other half the straight run did not reach (it sits in a branch's delay slot,
        # the half at the branch's target): follow the branches a short way.
        for k, w in enumerate(words):
            if w >> 26 == 0x0F and start + 4 * k not in uses:
                s = self._follow(insns, writes, k, (w >> 16) & 31, (w & 0xFFFF) << 16, far=self.follow_far)
                if s is not None:
                    uses[start + 4 * k] = [s]
        relocs = {}
        table_addrs = set(tables.values())
        for at, items in uses.items():
            s = items[0]
            sym = self._symbol(home, s, s in table_addrs)
            if sym is not None:
                relocs[at] = ("hi", sym[0], sym[1])
        return relocs, tables


    @staticmethod
    def _follow(insns: list, writes: list, k: int, reg: int, value: int, limit: int = 48,
                far: bool = False) -> int | None:
        """The address the lui at insns[k] forms, along the code's branches: the first addiu, ori,
        load or store through `reg` before something writes it, a jr or j ends the way. `far`
        (hostgen.json asm.follow_saved_registers): a j goes on at its target, and a call keeps a
        register it must save, so a base set once for a whole function (rac2's level loop has
        `lui $s6, %hi(table)` and the %lo two calls later) is still a base."""
        start = insns[0].addr

        def formed(d):
            if d.rs == reg and (d.op in LOADS_STORES or d.op in ("addiu", "daddiu", "ori")):
                return (value | (d.word & 0xFFFF)) if d.op == "ori" else (value + d.imm) & 0xFFFFFFFF
            return None

        todo = [(k + 1, 0)]
        if k > 0:
            before = insns[k - 1]
            if before.is_branch and before.op not in ("jr", "jalr", "j", "jal"):
                if before.likely:
                    todo = []                  # a likely branch's slot runs only on the way taken
                todo.append(((before.target - start) // 4, 0))     # the lui is that branch's delay slot
        seen = set()
        while todo:
            i, n = todo.pop()
            if i in seen or not 0 <= i < len(insns) or n > limit:
                continue
            seen.add(i)
            ins = insns[i]
            s = formed(ins)
            if s is not None:
                return s
            if reg in writes[i]:
                continue
            if ins.op in ("jr", "j", "jal", "jalr"):
                if i + 1 < len(insns):         # its delay slot still runs
                    s = formed(insns[i + 1])
                    if s is not None:
                        return s
                    if far and reg not in writes[i + 1]:
                        if ins.op == "j":
                            todo.append(((ins.target - start) // 4, n + 1))
                        elif ins.op in ("jal", "jalr") and (16 <= reg <= 23 or reg == 30):
                            todo.append((i + 2, n + 1))
                continue
            if ins.is_branch:
                todo.append(((ins.target - start) // 4, n + 1))
                if ins.op not in ("beq",) or ins.rs != ins.rt:
                    todo.append((i + 1, n + 1))
                continue
            todo.append((i + 1, n + 1))
        return None


class CombinedCode:
    """The assembly files' instructions first (their text has the decompilation's own symbols),
    the retail bytes for everything else."""

    def __init__(self, files: dict, retail: RetailCode):
        self.files = files
        self.retail = retail

    def __contains__(self, key) -> bool:
        return self.get(key) is not None

    def get(self, key, default=None):
        hit = self.files.get(key)
        if hit is not None:
            return hit
        return self.retail.get(key, default)


def starts_of(program) -> dict[int, dict[int, str]]:
    """Every function hostgen knows, by program and address: the one with C (or the host's) where
    several names share a place.

    A level's six-digit name (hostgen.json "level_short_names": func_316150 in a level 2 file) is
    that level's address, never the executable's (Program.level_short_name), so it starts nothing
    in the executable unless the catalogue has a function there. Taken as a start, it cut the
    executable's function around it in two, and a jump table's targets past the cut were dropped:
    the Dynamo's update (func_00315E20, five states) lost states 3 and 4 to level 2's func_316150,
    so after one activation it returned at once every tick and never targeted again."""
    out: dict[int, dict[int, str]] = {}
    # hostgen.json "executable_extents": an executable address strictly inside a function the
    # decompilation's report places is no start (hostgen.read_extents).
    extents = getattr(program, "executable_extents", {}) or {}
    inside: set[int] = set()
    for start, size in extents.items():
        inside.update(range(start + 4, start + size, 4))
    inside -= set(extents)
    short = getattr(program, "level_short_names", None)
    starts = getattr(program, "executable_starts", set())
    # hostgen.json "level_extents": a level place strictly inside a function the catalogue places
    # and sizes there is no start for a name the C only declares. Up Your Arsenal's
    # func_L03_0044DC88 (left out for its assembly) declares func_L01_003242C8, rac2's name for
    # another routine, which fell 0x128 bytes into func_L01_003241A0: cut there, that function's
    # state switch kept one of its eleven cases, and every other state returned at once.
    extents: dict[int, list[tuple[int, int]]] = {}
    if getattr(program, "level_extents", False):
        sizes = getattr(program, "sizes", {})
        for name, places in getattr(program, "places", {}).items():
            size = sizes.get(name, 0)
            for ov, addr in places if size > 4 else ():
                extents.setdefault(ov, []).append((addr, addr + size))
        for ov in extents:
            extents[ov].sort()
    catalogued = {p for ps in getattr(program, "places", {}).values() for p in ps} if extents else set()

    def inside_level(ov: int, addr: int) -> bool:
        spans = extents.get(ov)
        if not spans or (ov, addr) in catalogued:
            return False
        i = bisect.bisect_right(spans, (addr, 0xFFFFFFFF)) - 1
        return i >= 0 and spans[i][0] < addr < spans[i][1]

    for sym in sorted(program.functions):
        m = short.match(sym) if short is not None else None
        level_only = m is not None and len(sym) == len("func_") + 6
        f = program.functions[sym] if extents else None
        declared_only = f is not None and f.unit is None and sym not in getattr(program, "host", ())
        for ov, addr in program.code_places(sym):
            if level_only and ov < 0 and addr not in starts:
                continue
            if ov < 0 and addr in inside:
                continue
            if ov >= 0 and declared_only and inside_level(ov, addr):
                continue
            have = out.setdefault(ov, {}).get(addr)
            if have is None or (program.functions.get(have) is not None and program.functions[have].unit is None
                                and program.functions[sym].unit is not None):
                out[ov][addr] = sym
    return out
