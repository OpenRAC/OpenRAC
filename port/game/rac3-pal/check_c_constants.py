#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Checks the C rac3-pal takes from the decompilation against PAL's own instructions.

rac3-pal reads the decompilation's C (src/boot_elf, src/frontbin) and, for functions it still has
as assembly, the drafts in nonmatching/ (hostgen.json "candidates"). Many drafts are NTSC-U's C
with its names moved to PAL addresses, but a literal address, a $gp offset or a frame-rate
constant inside the code stays NTSC-U's. Three such slips cost a blank font (fixed palette
addresses 0x80 past PAL's), empty menu labels (a $gp fallback string through a register variable)
and a save icon written over all of memory (the allocator's buffer top at NTSC-U's 0x0022760C).

For every function the build takes from C, this compares what the C names with what PAL's
assembly of that function (the setup's asm/, splat's .s files, MMI .word lines included) does.
Strong hits (each one a reason to leave the candidate out, hostgen.json "candidates_left_out"):

- a literal of 0x1000 or more (or an address, 0x00100000-0x01FFFFFF or KSEG0) that is not an
  immediate, a shift, a constant built by lui+ori/addiu (or ori+dsll+ori), an address the code
  forms (lui/%lo, $gp-relative), or a word it loads from such an address;
- a function it calls (func_XXXXXXXX, or a func_X_hcN alias of one) that is not a jal/j target
  or a constant in PAL's code: the draft calls an NTSC-U address (`func_X + n` may point into one);
- a data symbol (D_XXXXXXXX, or a readable name from names.h) with no address formed at or up to
  0x400 past it;
- a $gp register variable (the host has no $gp);
- a frame-rate float (1/60, 60, 59.94, 30, 0.02, 50, 25) or a per-frame float PAL has scaled by
  1.2 that PAL's code lacks; PAL's divide by 6 (its (x*5+2)/6 timer scaling) missing from the C,
  or the C's 5/6 scaling missing from PAL's code;
- a draft nonmatching/README.md marks "differs" (it did not build to PAL's bytes), or a draft of
  one program read for the other program's function of the same address.

Weak hits are hints, printed for review but never failing the run: small literals (below 0x1000:
indices, case labels and offsets folded into a moved base), an NTSC-U-looking frame count whose
5/6 PAL's code has (8 and 16 are everywhere), and inline assembly hostgen skips. Matched C (PAL's
own bytes) is checked as well, as the check's own calibration (2 strong hits: two matched $gp
register variables), and never fails the run.

Run it after every decomp pin bump (README.md, "Checking the C against PAL"):

    python3 port/game/rac3-pal/check_c_constants.py --asm-source <a decomp checkout with asm/>

It runs hostgen (C only, no assembly stage, ~20 s) to learn which functions are taken from C and
which candidates were used, or reads an existing hostgen output (--gen). It honours
"candidates_left_out", "candidate_folders_left_out", "candidate_scopes", "from_assembly" and the
library table's host rows. Exit status 1 when a candidate has a strong hit; --json writes every
finding.
"""

from __future__ import annotations

import argparse
import json
import math
import re
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
GP = 0x001DC8B0

# Literals the check never reports: they cannot be addresses or version constants, and the
# compiler folds them into instruction choices (shifts, sign tests, increments).
TRIVIAL = set(range(-1, 9)) | {16, 32, 64, 128, 255, 256, 0xFFFF, 0xFFFFFFFF}


# --- PAL's assembly ------------------------------------------------------------------------------

_LINE = re.compile(r"/\*\s*[0-9A-F]+\s+([0-9A-F]{8})\s+([0-9A-F]{8})\s*\*/|^\s*\.word\s+0x([0-9A-Fa-f]{8})\s*/\*", re.M)


def read_words(path: Path) -> list[tuple[int, int]]:
    """(address, instruction word) for each instruction in a splat .s file (the MMI ones splat
    writes as .word with the instruction in a comment, lq/sq among them, included)."""
    out = []
    pc = 0
    for m in _LINE.finditer(path.read_text(errors="replace")):
        if m.group(1):
            pc = int(m.group(1), 16)
            out.append((pc, int.from_bytes(bytes.fromhex(m.group(2)), "little")))
        else:
            pc += 4
            out.append((pc, int(m.group(3), 16)))
    return out


def sext16(v: int) -> int:
    return v - 0x10000 if v & 0x8000 else v


class Asm:
    """What one function's instructions use: immediates, constants, addresses, call targets."""

    def __init__(self, words: list[tuple[int, int]], memory: "Memory"):
        self.imm: set[int] = set()
        self.const: set[int] = set()
        self.addr: set[int] = set()
        self.calls: set[int] = set()
        self.shifts: set[int] = set()
        self.loaded: set[int] = set()
        self.has_mult = False
        self.divides_by_6 = False
        six: set[int] = set()  # registers last set to 6 (addiu/ori $r, $0, 6)
        # Every lui value each register held anywhere in the function: the scan is linear, and a
        # lui in one branch's delay slot is used after a label further down.
        lui: dict[int, set[int]] = {}
        for pc, w in words:
            op = w >> 26
            rs, rt, imm = (w >> 21) & 31, (w >> 16) & 31, w & 0xFFFF
            if op in (2, 3):  # j, jal
                self.calls.add(((pc + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2))
                continue
            if op == 0:  # SPECIAL
                fn = w & 63
                if fn in (0, 2, 3, 0x38, 0x3A, 0x3B, 0x3C, 0x3E, 0x3F):
                    sa = (w >> 6) & 31
                    if fn >= 0x3C:
                        sa += 32
                    self.shifts.add(sa)
                if fn in (0x18, 0x19, 0x1A, 0x1B):
                    self.has_mult = True
                    if fn in (0x1A, 0x1B) and rt in six:
                        self.divides_by_6 = True
                six.discard((w >> 11) & 31)
                continue
            if op == 0x1C and (w & 63) in (0x18, 0x19, 0x1A, 0x1B, 0x00, 0x01):  # MMI mult/madd/div1
                self.has_mult = True
                if (w & 63) in (0x1A, 0x1B) and rt in six:
                    self.divides_by_6 = True
                continue
            if op in (0x09, 0x0D) and rs == 0 and imm == 6:
                six.add(rt)
            elif 0x08 <= op <= 0x0F or 0x18 <= op <= 0x19 or 0x20 <= op <= 0x27 or op == 0x37:
                six.discard(rt)
            if op == 0x0F:  # lui
                v = imm << 16
                self.imm.add(v)
                self.const.add(v)
                lui.setdefault(rt, set()).add(v)
                continue
            if op in (0x04, 0x05, 0x06, 0x07, 0x14, 0x15, 0x16, 0x17, 0x01, 0x10, 0x11, 0x12):
                continue  # branches and coprocessor moves: their immediates are offsets
            s, z = sext16(imm), imm
            self.imm.update((s, z))
            bases = {GP} if rs == 28 else lui.get(rs, set())
            for base in list(bases):
                a = (base | z) if op == 0x0D else (base + s) & 0xFFFFFFFF
                self.addr.add(a)
                self.const.add(a)
                if 0x1E <= op <= 0x3F:  # a load or store at that address: the words there
                    for off in (0, 4, 8, 12):
                        v = memory.word(a + off, 4)
                        if v is not None:
                            self.loaded.add(v)
                if op in (0x08, 0x09, 0x0D, 0x18, 0x19) and rt == rs:
                    lui.setdefault(rt, set()).add(a)

    def explains_int(self, v: int) -> bool:
        u = v & 0xFFFFFFFF
        cands = {v, u, -v, (-v) & 0xFFFFFFFF, v - 1, v + 1, (~v) & 0xFFFF, (~v) & 0xFFFFFFFF, sext16(v & 0xFFFF) if v < 0x10000 else v}
        pool = self.imm | self.const | self.addr | self.loaded
        if cands & pool or (u - 1) & 0xFFFFFFFF in pool or (u + 1) & 0xFFFFFFFF in pool:
            return True
        if v > 0 and v & (v - 1) == 0 and int(math.log2(v)) in self.shifts:
            return True
        # A value built in two halves (lui/ori, ori + dsll + ori) or an immediate shifted into place.
        if u >= 0x10000 and ((u >> 16) << 16 in self.const or (u >> 16) in self.imm or ((u + 0x8000) >> 16) << 16 in self.const) \
                and ((u & 0xFFFF) in self.imm or sext16(u & 0xFFFF) in self.imm or (u & 0xFFFF) == 0):
            return True
        if u >= 0x10000 and any((u >> k) in self.imm and (u >> k) << k == u for k in self.shifts if k):
            return True
        # The low half of a value the code builds with lui + addiu/ori, or its high half.
        if v < 0x10000 and any((c & 0xFFFF) == v or (c >> 16) == v for c in self.const):
            return True
        # A constant multiply done as shifts and adds/subs (x*5 = (x<<2)+x, x*7 = (x<<3)-x, ...).
        if 0 < v < 0x10000 and any((1 << a) + (1 << b) == v or (1 << a) - (1 << b) == v
                                   for a in self.shifts | {0} for b in self.shifts | {0}):
            return True
        # A divide by a constant as a multiply by its reciprocal.
        if 1 < v < 0x10000 and self.has_mult:
            magic = {(1 << (32 + k)) // v + 1 for k in range(0, 8)} | {(1 << (32 + k)) // v for k in range(0, 8)}
            if magic & {c & 0xFFFFFFFF for c in self.const}:
                return True
        return False

    def explains_float(self, bits: int) -> bool:
        pool = self.const | self.loaded | self.imm
        if bits in pool or bits ^ 0x80000000 in pool:  # the C's literal may carry a minus sign
            return True
        return (bits & 0xFFFF) == 0 and bits in self.const

    def explains_symbol(self, a: int, window: int = 0x400) -> bool:
        if any(a <= x < a + window for x in self.addr | self.const):
            return True
        return ((a + 0x8000) >> 16) << 16 in self.const and sext16(a & 0xFFFF) in self.imm

    def explains_call(self, a: int, window: int = 1) -> bool:
        """A call or a pointer to the function at a; with a window, a pointer into it
        (`(u8 *)func_X + 0x38`)."""
        return any(a <= x < a + window for x in self.calls | self.const | self.addr | self.loaded)


class Memory:
    """The programs' loaded bytes (ELF PT_LOAD segments), to read the words the code loads."""

    def __init__(self, elves: list[Path]):
        self.segments: list[tuple[int, bytes]] = []
        for p in elves:
            if p.is_file():
                self._load(p.read_bytes())

    def _load(self, data: bytes) -> None:
        if data[:4] != b"\x7fELF":
            return
        phoff, = struct.unpack_from("<I", data, 0x1C)
        phentsize, phnum = struct.unpack_from("<HH", data, 0x2A)
        for i in range(phnum):
            ptype, off, vaddr, _paddr, filesz = struct.unpack_from("<IIIII", data, phoff + i * phentsize)
            if ptype == 1 and filesz:
                self.segments.append((vaddr, data[off:off + filesz]))

    def word(self, a: int, size: int) -> int | None:
        for base, blob in self.segments:
            if base <= a and a + size <= base + len(blob):
                return int.from_bytes(blob[a - base:a - base + size], "little")
        return None


# --- the C -----------------------------------------------------------------------------------

_STRIP = re.compile(r'"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'|/\*.*?\*/|//[^\n]*', re.S)
_FLOAT = re.compile(r"(?<![\w.])((?:\d+\.\d*|\.\d+)(?:[eE][+-]?\d+)?|\d+[eE][+-]?\d+)[fFlL]?(?![\w.])")
_INT = re.compile(r"(?<![\w.])(0[xX][0-9A-Fa-f]+|\d+)[uUlL]*(?![\w.])")
_SYM = re.compile(r"\b(D_|func_)([0-9A-Fa-f]{6,8})(?:_\w+)?\b")
_IDENT = re.compile(r"\b[A-Za-z_]\w*\b")
_GPREG = re.compile(r'__asm__\s*\(\s*"\$?(gp|28)"\s*\)|register[^;]*asm\s*\(\s*"\$?(gp|28)"')
_ASM_STMT = re.compile(r'__asm__\s*(?:volatile|__volatile__)?\s*\(\s*"((?:\\.|[^"\\])*)"')


def strip(text: str) -> str:
    return _STRIP.sub(lambda m: re.sub(r"[^\n]", " ", m.group(0)), text)


def functions(text: str) -> dict[str, tuple[str, str]]:
    """Top-level function definitions: name -> (body as written, body stripped of comments
    and strings)."""
    clean = strip(text)
    out = {}
    depth = 0
    i = 0
    n = len(clean)
    while i < n:
        c = clean[i]
        if c == "{":
            if depth == 0:
                j = i - 1
                while j >= 0 and clean[j].isspace():
                    j -= 1
                if j >= 0 and clean[j] == ")":
                    # the matching "(" of the parameter list, then the name before it
                    d, k = 0, j
                    while k >= 0:
                        if clean[k] == ")":
                            d += 1
                        elif clean[k] == "(":
                            d -= 1
                            if d == 0:
                                break
                        k -= 1
                    m = re.search(r"([A-Za-z_]\w*)\s*$", clean[:k])
                    start = i
                    d2, e = 0, i
                    while e < n:
                        if clean[e] == "{":
                            d2 += 1
                        elif clean[e] == "}":
                            d2 -= 1
                            if d2 == 0:
                                break
                        e += 1
                    if m and m.group(1) not in ("if", "while", "for", "switch", "sizeof", "__attribute__"):
                        line_start = clean.rfind("\n", 0, m.start()) + 1
                        out[m.group(1)] = (text[line_start:e + 1], clean[line_start:e + 1])
                    i = e + 1
                    continue
            depth += 1
        elif c == "}":
            depth = max(0, depth - 1)
        i += 1
    return out


def read_aliases(decomp: Path) -> dict[str, int]:
    """Readable names -> address, from the decompilation's names headers."""
    out = {}
    for h in ("include/names.h", "include/bytematch_names.h"):
        p = decomp / h
        if p.is_file():
            for m in re.finditer(r"#define\s+(\w+)\s+(?:D_|func_)([0-9A-Fa-f]{6,8})\b", p.read_text(errors="replace")):
                out[m.group(1)] = int(m.group(2), 16)
    return out


def float_bits(text: str) -> int:
    return struct.unpack("<I", struct.pack("<f", float(text)))[0]


FRAME_RATE = {"1/60": 1 / 60, "60": 60.0, "59.94": 59.94, "30": 30.0, "0.02": 0.02, "50": 50.0, "25": 25.0}


def check_function(name: str, body: str, clean: str, asm: Asm, aliases: dict[str, int],
                   local: set[str] = frozenset()) -> list[dict]:
    hits = []
    own = int(name[5:13], 16) if re.match(r"func_[0-9A-F]{8}", name) else -1
    seen = set()

    def hit(kind: str, what: str, strength: str) -> None:
        if (kind, what) not in seen:
            seen.add((kind, what))
            hits.append({"kind": kind, "what": what, "strength": strength})

    if _GPREG.search(body):
        hit("gp", "a register variable bound to $gp (the host has none)", "strong")
    for m in _ASM_STMT.finditer(body):
        code = m.group(1).strip()
        if code and not code.startswith(".extern") and not re.fullmatch(r"\$?\w+", code):
            hit("inline-asm", f'__asm__("{code[:60]}")', "weak")
    # Only the function's own statements: the header with its name has no literals worth checking.
    head_end = clean.find("{")
    stmts = clean[head_end:]
    # Literals inside a symbol's suffix or name never reach here: _INT needs a non-word before it.
    for m in _FLOAT.finditer(stmts):
        v = float(m.group(1))
        bits = float_bits(m.group(1))
        if v in (0.0, 1.0, -1.0, 0.5, 2.0):
            continue
        if not asm.explains_float(bits):
            scaled = [k for k in (1.2, 1 / 1.2, 1.44, 1 / 1.44) if asm.explains_float(float_bits(repr(v * k)))]
            if scaled:
                hit("float", f"{m.group(0)}: a per-frame value PAL's code has scaled by {scaled[0]:.4g} (50 Hz)", "strong")
                continue
            rate = [k for k, r in FRAME_RATE.items() if abs(v - r) < 1e-4 * max(1, r)]
            hit("float", f"{m.group(0)} (0x{bits:08X}){' frame-rate value ' + rate[0] if rate else ''}",
                "strong" if rate else "weak")
    for m in _INT.finditer(stmts):
        tok = m.group(1)
        v = int(tok, 16) if tok.lower().startswith("0x") else int(tok)
        if v in TRIVIAL:
            continue
        if asm.explains_int(v):
            continue
        pal = {(v * 5 + 2) // 6, v * 5 // 6, round(v * 5 / 6)} - {v}
        if 6 <= v <= 3600 and pal & asm.imm:
            hit("timer", f"{tok}: an NTSC-U frame count where PAL's code has {sorted(pal & asm.imm)[0]} (5/6 of it)",
                "weak")  # matched C has many of these too (an 8 or a 16 is everywhere): a hint only
        elif 0x00100000 <= v <= 0x01FFFFFF or 0x80000000 <= v <= 0x9FFFFFFF:
            hit("address", f"{tok} (a literal address PAL's code does not form)", "strong")
        elif v >= 0x1000:
            # An offset or a constant that big is kept as written; smaller ones are often folded
            # into a moved base register (0x330 from a base of 0x170), so they are weak.
            hit("int", f"{tok}", "strong" if v < 0x00100000 else "weak")
        else:
            hit("int", f"{tok}", "weak")
    # PAL scales NTSC-U's 60 Hz timers to 50 Hz as (x*5+2)/6 (a multiply by the reciprocal of 6):
    # C and assembly must agree on it.
    c_scales = bool(re.search(r"[/%]\s*6\b", stmts))
    a_scales = asm.divides_by_6 or (asm.has_mult and any((c & 0xFFFFFFFF) in (0x2AAAAAAB, 0xAAAAAAAB)
                                                         for c in asm.const))
    if a_scales and not c_scales:
        hit("timer", "PAL's code divides by 6 (its 50 Hz timer scaling, (x*5+2)/6); the C does not", "strong")
    elif c_scales and not a_scales and re.search(r"\*\s*5\b", stmts):
        hit("timer", "the C scales by 5/6; PAL's code does not", "strong")
    # Symbols: data and functions, by address.
    for m in _SYM.finditer(stmts):
        a = int(m.group(2), 16)
        word = m.group(0)
        if word == name or word.startswith(name + "_") or a == own:
            continue
        if word != m.group(1) + m.group(2) and word in local:
            continue  # a helper copy the file defines itself (func_X_hcN)
        if m.group(1) == "func_":
            plus = re.match(r"\s*\+", stmts[m.end():])
            if not asm.explains_call(a, 0x100 if plus else 1):
                hit("call", f"{word} (not a call or constant in PAL's code)", "strong")
        else:
            if not asm.explains_symbol(a):
                hit("data", f"{word} (PAL's code forms no address at or after it)", "strong")
    for m in _IDENT.finditer(stmts):
        a = aliases.get(m.group(0))
        if a is not None and not (asm.explains_symbol(a) or asm.explains_call(a, 0x100)):
            hit("data", f"{m.group(0)} = 0x{a:08X} (PAL's code forms no address at or after it)", "strong")
    return hits


# --- which functions the build takes from C --------------------------------------------------

def run_hostgen(game: Path, decomp: Path, out: Path, jobs: int) -> Path:
    cmd = [sys.executable, str(REPO / "port/tools/hostgen/hostgen.py"), "--game", str(game), "--source", str(decomp),
           "--out", str(out), "--quiet", "--allow-unreadable", "--asm", "off", "--jobs", str(jobs)]
    subprocess.run(cmd, check=True)
    return out


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--game", type=Path, default=HERE / "hostgen.json")
    ap.add_argument("--decomp", type=Path, default=REPO / "games/rac3/decomp", help="the decompilation at its pin")
    ap.add_argument("--asm-source", type=Path, help="a decompilation checkout whose setup wrote asm/ and "
                    "baserom/*.elf (default: --decomp)")
    ap.add_argument("--gen", type=Path, help="an existing hostgen output (report.json, prep/) to read, "
                    "instead of running hostgen")
    ap.add_argument("--json", type=Path, help="write every finding here")
    ap.add_argument("--jobs", type=int, default=6, help="hostgen's parallel jobs (default 6)")
    ap.add_argument("--matched", action="store_true", help="also list hits in matched C")
    args = ap.parse_args(argv)
    decomp = args.decomp.resolve()
    asm_src = (args.asm_source or args.decomp).resolve()
    cfg = json.loads(args.game.read_text())
    if args.gen:
        return check(args, cfg, decomp, asm_src, args.gen)
    with tempfile.TemporaryDirectory(prefix="rac3-check-") as tmp:
        run_hostgen(args.game, decomp, Path(tmp), args.jobs)
        return check(args, cfg, decomp, asm_src, Path(tmp))


def check(args: argparse.Namespace, cfg: dict, decomp: Path, asm_src: Path, gen: Path) -> int:
    """The check over one hostgen output (report.json and its prepared sources, prep/)."""
    report = json.loads((gen / "report.json").read_text())
    candidates_used = set(report.get("candidates_used", []))
    stubbed = set(report.get("stubbed", {}))
    left_out = set(cfg.get("candidates_left_out", {}))
    from_asm = set(cfg.get("from_assembly", []))
    hosted = set()
    lib = args.game.parent / cfg.get("libraries", "libraries.tsv")
    if lib.is_file():
        for line in lib.read_text().splitlines():
            row = line.split("\t")
            if len(row) >= 4 and row[3] == "host":
                hosted.add(row[0])
    # Each candidate's file, and why its draft was not carried (nonmatching/README.md's table).
    cand_file: dict[str, str] = {}
    table = decomp / cfg.get("candidates", "nonmatching/functional.tsv")
    if table.is_file():
        for line in table.read_text().splitlines():
            if line.strip() and not line.startswith("#"):
                n, _, f = line.partition("\t")
                cand_file[n.strip()] = f.strip()
    why: dict[tuple[str, str], str] = {}
    readme = decomp / "nonmatching/README.md"
    if readme.is_file():
        for m in re.finditer(r"^\| `(\w+)` \| (\w+) \| [^|]*\| ([^|]*)\|", readme.read_text(), re.M):
            why[(m.group(1), m.group(2))] = m.group(3).strip()
    scopes = cfg.get("candidate_scopes", {})
    memory = Memory([asm_src / "baserom/boot_elf.elf", asm_src / "baserom/frontbin.elf"])
    aliases = read_aliases(decomp)
    asm_dirs = {"boot_elf": asm_src / "asm/boot_elf/nonmatchings", "frontbin": asm_src / "asm/nonmatchings"}
    asm_files = {prog: {p.stem: p for p in d.rglob("*.s")} for prog, d in asm_dirs.items()}
    if not any(asm_files.values()):
        print(f"no assembly under {asm_src}/asm: run the decompilation's setup there, or give --asm-source",
              file=sys.stderr)
        return 2
    results = []
    checked = {"candidate": 0, "matched": 0}
    no_asm = []
    for prog in ("boot_elf", "frontbin"):
        root = gen / "prep/src" / prog
        suffix = "__frontbin" if prog == "frontbin" else ""
        for path in sorted(root.rglob("*.c")):
            rel = path.relative_to(gen / "prep")
            if any(str(rel).startswith(s) for s in cfg.get("skip", [])):
                continue
            original = decomp / rel
            orig_defs = functions(original.read_text(errors="replace")) if original.is_file() else {}
            defs = functions(path.read_text(errors="replace"))
            for name, (body, clean) in defs.items():
                full = name + suffix
                if name in from_asm or full in from_asm or full in stubbed or name in hosted:
                    continue
                origin = "matched" if name in orig_defs else "candidate"
                extra = []
                if origin == "candidate":
                    if name not in candidates_used or name in left_out:
                        continue
                    f = cand_file.get(name, "")
                    folder = str(Path(f).parent)
                    if folder in cfg.get("candidate_folders_left_out", {}):
                        continue  # the whole folder is left out: its functions come from assembly
                    if folder in scopes and scopes[folder] != f"src/{prog}":
                        continue  # hostgen.json "candidate_scopes" keeps it out of this program
                    draft_prog = Path(f).parent.name
                    if draft_prog in ("boot_elf", "frontbin") and draft_prog != prog:
                        extra.append({"kind": "program", "strength": "strong",
                                      "what": f"{f} is {draft_prog}'s draft, read for {prog}'s function of the "
                                              f"same address (another routine)"})
                    reason = why.get((name, draft_prog), "")
                    if reason.startswith("differs"):
                        extra.append({"kind": "draft", "strength": "strong",
                                      "what": f"the NTSC-U draft's code is not PAL's ({reason})"})
                asm_path = asm_files[prog].get(name)
                if asm_path is None:
                    no_asm.append(full)
                    continue
                checked[origin] += 1
                asm = Asm(read_words(asm_path), memory)
                hits = extra + check_function(name, body, clean, asm, aliases, set(defs))
                if hits:
                    results.append({"function": full, "program": prog, "origin": origin, "file": str(rel), "hits": hits})
    strong = [r for r in results if any(h["strength"] == "strong" for h in r["hits"])]
    for r in results:
        if r["origin"] == "matched" and not args.matched:
            continue
        mark = "STRONG" if r in strong else "weak"
        print(f"{r['function']} [{r['origin']}, {mark}] {r['file']}")
        for h in r["hits"]:
            print(f"    {h['strength']:6} {h['kind']:10} {h['what']}")
    cand_strong = [r for r in strong if r["origin"] == "candidate"]
    print(f"\nchecked {checked['candidate']} candidate and {checked['matched']} matched functions "
          f"({len(no_asm)} without a .s file); candidates with strong hits: {len(cand_strong)}, "
          f"with weak hits only: {sum(1 for r in results if r['origin'] == 'candidate' and r not in strong)}; "
          f"matched with strong hits (the check's own noise): "
          f"{sum(1 for r in strong if r['origin'] == 'matched')}")
    if args.json:
        args.json.write_text(json.dumps({"checked": checked, "no_asm": no_asm, "results": results}, indent=1))
    return 1 if cand_strong else 0


if __name__ == "__main__":
    sys.exit(main())
