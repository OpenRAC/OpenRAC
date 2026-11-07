#!/usr/bin/env python3
"""
Check compiled functions against retail more strictly than tools/audit_matches.py.

audit_matches.py masks the 16-bit immediate of every load, store and add that is
not $sp-based (tools/retail.py), so struct offsets, small constants and pointer
steps are not compared: `p += 4` on an `s32 *` (16 bytes) passes where retail
steps 4. This check uses the object's relocations instead of a mask:

- every word without a relocation must equal retail exactly;
- R_MIPS_26: the jal/j target must be the address the symbol's name gives;
- R_MIPS_HI16/LO16: retail's (hi << 16) + lo must be the symbol's address plus
  the addend, or, against the object's own .rodata/.data, retail must hold the
  same bytes at the address it forms;
- R_MIPS_GPREL16: retail's $gp offset must be the symbol's address plus the
  addend minus $gp (the value most gp-relative accesses agree on).

Symbols are resolved from their names (func_XXXXXXXX, D_XXXXXXXX, D_XXXXXXXX_).
An overlay function named after another level (func_L00_... called from L01)
sits elsewhere in this level, so such calls are noted, not checked. Overlay
functions need the unpacked overlays, like audit_matches.py ($OVERLAYS or
private/overlays).

Usage:
  venv/bin/python tools/strict_check.py               every func_ in build/obj/*.o
  venv/bin/python tools/strict_check.py -v            also list the functions that pass
  venv/bin/python tools/strict_check.py OBJ.o:FUNC... these (or @FILE, one per line)
Exits 1 when a function fails.
"""
import glob
import os
import re
import struct
import sys
from collections import Counter
from pathlib import Path

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

sys.path.insert(0, str(Path(__file__).resolve().parent))
import retail as mp  # noqa: E402

ROOT = mp.ROOT

FUNCS = {}
for l in (ROOT / "config" / "functions.tsv").read_text().splitlines():
    if l and not l.startswith("#"):
        n, a, s, _ = l.split("\t")
        FUNCS[n] = (int(a, 16), int(s, 16))
OVL_DIRS = {}
for l in (ROOT / "config" / "overlays.tsv").read_text().splitlines():
    if l and not l.startswith("#"):
        i, d, _, _ = l.split("\t")
        OVL_DIRS[i] = d
for l in (ROOT / "config" / "overlay_functions.tsv").read_text().splitlines():
    if l and not l.startswith("#"):
        n, a, sz, _, _ = l.split("\t")
        FUNCS[n] = (int(a, 16), int(sz, 16))
MAIN = mp.sections(ELFFile(open(ROOT / "baserom" / "SCUS_974.65.elf", "rb")))
_ovl = {}
NOTES = {}


def secs_for(name):
    m = re.match(r"func_(L\d\d)_", name)
    if not m:
        return MAIN
    lv = m.group(1)
    if lv not in _ovl:
        base = Path(os.environ.get("OVERLAYS") or ROOT / "private" / "overlays")
        path = base / "levels" / OVL_DIRS[lv] / "overlay.elf"
        _ovl[lv] = mp.sections(ELFFile(open(path, "rb")))
    return _ovl[lv]


def sym_addr(name):
    m = re.fullmatch(r"(?:func|D)_(?:L\d\d_)?([0-9A-Fa-f]+)_?", name)
    return int(m.group(1), 16) if m else None


def sext(v):
    return v - 0x10000 if v & 0x8000 else v


def analyse(obj, fname):
    """-> (problems, gp samples [(symbol address + addend, retail offset, word)])"""
    elf = ELFFile(open(obj, "rb"))
    text = elf.get_section_by_name(".text")
    syms = list(elf.get_section_by_name(".symtab").iter_symbols())
    fs = [s for s in syms if s.name == fname and s["st_info"]["type"] == "STT_FUNC"]
    if not fs:
        return ["no symbol"], []
    start, size = fs[0]["st_value"], fs[0]["st_size"]
    code = text.data()[start:start + size]
    addr, rsize = FUNCS[fname]
    secs = secs_for(fname)
    r = mp.read(secs, addr, rsize)
    if len(code) < len(r) and not any(r[len(code):]):
        r = r[:len(code)]
    if len(code) != len(r):
        return ["size %d vs %d" % (len(code), len(r))], []
    words = [w for (w,) in struct.iter_unpack("<I", code)]
    rwords = [w for (w,) in struct.iter_unpack("<I", r)]
    sections = list(elf.iter_sections())
    relocs = {}
    for sec in sections:
        if isinstance(sec, RelocationSection) and sec.name in (".rel.text", ".rela.text"):
            for rel in sec.iter_relocations():
                off = rel["r_offset"]
                if start <= off < start + size:
                    relocs.setdefault((off - start) // 4, []).append(rel)

    def target(rel):
        s = syms[rel["r_info_sym"]]
        a = sym_addr(s.name)
        if a is not None:
            return "abs", a, s.name
        if s.name == fname:
            return "abs", addr, s.name
        if isinstance(s["st_shndx"], int):
            value = s["st_value"] if s["st_info"]["type"] != "STT_SECTION" else 0
            return "local", (sections[s["st_shndx"]], value), s.name or sections[s["st_shndx"]].name
        return "unnamed", None, s.name

    problems, gp = [], []
    notes = NOTES.setdefault((obj, fname), [])
    his = {}
    for i, (w, rw) in enumerate(zip(words, rwords)):
        rs = relocs.get(i)
        if not rs:
            if w != rw:
                problems.append("word %d: %08X vs retail %08X" % (i, w, rw))
            continue
        rel = rs[0]
        t = rel["r_info_type"]
        kind, a, sname = target(rel)
        pc = addr + 4 * i
        if t == 4:  # R_MIPS_26
            if (w >> 26) != (rw >> 26):
                problems.append("word %d: opcode differs" % i)
                continue
            add = (w & 0x3FFFFFF) << 2
            rt = ((rw & 0x3FFFFFF) << 2) | (pc & 0xF0000000)
            if kind == "local" and a[0].name == ".text":
                exp = addr - start + a[1] + add
            elif kind == "abs":
                exp = a + add
            else:
                problems.append("word %d: jump to %s %s" % (i, kind, sname))
                continue
            other = re.match(r"func_(L\d\d)_", sname)
            mine = re.match(r"func_(L\d\d)_", fname)
            if other and (not mine or other.group(1) != mine.group(1)):
                # An overlay function is named after the first level it occurs in; in this
                # level it sits elsewhere. Not checkable by name: noted, not counted as a failure.
                notes.append("word %d: call to %s (other level)" % (i, sname))
                continue
            if exp != rt:
                problems.append("word %d: jal %s = %X vs retail %X" % (i, sname, exp, rt))
        elif t in (5, 6):  # HI16 / LO16
            if (w & 0xFFFF0000) != (rw & 0xFFFF0000):
                problems.append("word %d: hi/lo instruction differs" % i)
                continue
            key = rel["r_info_sym"]
            if t == 5:
                his.setdefault(key, []).append((w & 0xFFFF, rw & 0xFFFF))
                continue
            if not his.get(key):
                problems.append("word %d: LO16 without HI16" % i)
                continue
            lo, rlo = sext(w & 0xFFFF), sext(rw & 0xFFFF)
            for hi, rhi in his[key][-1:]:
                add = ((hi << 16) + lo) & 0xFFFFFFFF
                rt = ((rhi << 16) + rlo) & 0xFFFFFFFF
                if kind == "abs":
                    if (a + add) & 0xFFFFFFFF != rt:
                        problems.append("word %d: %s+%X = %X vs retail %X" % (i, sname, add, (a + add) & 0xFFFFFFFF, rt))
                elif kind == "local":
                    sec, value = a
                    if sec["sh_type"] == "SHT_NOBITS":
                        problems.append("word %d: local bss %s" % (i, sec.name))
                        continue
                    data = sec.data()
                    off = value + add
                    n = 8 if off + 8 <= len(data) else 4
                    want = data[off:off + n]
                    got = mp.read(secs, rt, n)
                    if got != want:
                        problems.append("word %d: %s+%X %s vs retail @%X %s" % (i, sec.name, off, want.hex(), rt, got.hex() if got else None))
                else:
                    problems.append("word %d: unnamed symbol %s" % (i, sname))
        elif t == 7:  # GPREL16
            if (w & 0xFFFF0000) != (rw & 0xFFFF0000):
                problems.append("word %d: gp-relative instruction differs" % i)
                continue
            if kind != "abs":
                problems.append("word %d: gp-relative to %s %s" % (i, kind, sname))
                continue
            gp.append((a + sext(w & 0xFFFF), sext(rw & 0xFFFF), i))
        else:
            problems.append("word %d: relocation type %d" % (i, t))
    return problems, gp


def all_functions():
    """OBJ:FUNC for every func_ defined in build/obj/*.o."""
    out = []
    for path in sorted(glob.glob(str(ROOT / "build" / "obj" / "*.o"))):
        with open(path, "rb") as f:
            for s in ELFFile(f).get_section_by_name(".symtab").iter_symbols():
                if s["st_info"]["type"] == "STT_FUNC" and s.name in FUNCS:
                    out.append("%s:%s" % (path, s.name))
    return out


def main():
    verbose = "-v" in sys.argv[1:]
    args = []
    for a in sys.argv[1:]:
        if a == "-v":
            continue
        args += Path(a[1:]).read_text().split() if a.startswith("@") else [a]
    items = [a.rsplit(":", 1) for a in (args or all_functions())]
    res = {}
    votes = Counter()
    for obj, fn in items:
        try:
            p, g = analyse(obj, fn)
        except Exception as e:  # noqa: BLE001
            p, g = ["error %r" % e], []
        res[(obj, fn)] = (p, g)
        for exp, off, _ in g:
            votes[exp - off] += 1
    gpv = votes.most_common(1)[0][0] if votes else None
    failed = 0
    for (obj, fn), (p, g) in res.items():
        p = list(p)
        for exp, off, i in g:
            if exp - off != gpv:
                p.append("word %d: gp offset %d, expected %d" % (i, off, exp - gpv))
        failed += bool(p)
        if p:
            print("FAIL	%s	%s" % (fn, "; ".join(p[:4]) + (" ..." if len(p) > 4 else "")))
        elif verbose:
            notes = NOTES.get((obj, fn))
            print("PASS	%s%s" % (fn, "	(%d calls into other levels not checked)" % len(notes) if notes else ""))
    print("%d functions, %d pass, %d fail" % (len(res), len(res) - failed, failed))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
