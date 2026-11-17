#!/usr/bin/env python3
"""Name the labels of the computed gotos' tables (hostgen.json "jump_tables"), from a disc image.

Matching C reaches a jump table the compiler would have written for a switch through `goto *table[i]`,
with the table in the game's data: code addresses. The host's goto takes labels, not guest addresses,
so hostgen writes a switch over the labels each table holds (lower.py _indirect_goto), which this
table names. A developer runs it once with their own disc; what it writes is the labels' names, by
table and index, not the disc's bytes.

  python3 port/tools/hostgen/hostgen.py --game port/game/rac2-ntsc/hostgen.json \\
      --source games/rac2/decomp --out build/hg --program-out build/hg/program.pickle
  python3 port/tools/hostgen/jumptables.py --game port/game/rac2-ntsc/hostgen.json \\
      --program build/hg/program.pickle --report build/hg/report.json --image DISC.iso

hostgen's report lists each computed goto through a table it does not know yet, with the
function's labels in source order and what each one's code does first (a call, a goto, a return).
For each address of the table, the instructions there are read up to their first jal, j or jr $ra;
the addresses, in order, are aligned with the labels, in source order (matching C lays its code out
as its source), scoring where both lead to the same call. The result is merged into the file
hostgen.json's "jump_tables" names (jump_tables.json beside it) and each choice printed with how
well it agreed, for a look."""

from __future__ import annotations

import argparse
import json
import pickle
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import retail  # noqa: E402

_LEVEL = re.compile(r"_L(\d\d)_")
_ADDR = re.compile(r"([0-9A-Fa-f]{8})$")


def level_of(name: str) -> int:
    m = _LEVEL.search(name)
    return int(m.group(1)) if m else -1


def address_of(program, name: str) -> int | None:
    m = _ADDR.search(name)
    if _LEVEL.search(name) and m:
        return int(m.group(1), 16)  # a level's own data: where that level's program has it
    if name in program.symbols:
        return program.symbols[name]
    m = _ADDR.search(name)
    return int(m.group(1), 16) if m else None


def derived_address(derived: dict, name: str) -> tuple[int, int] | None:
    """(level, address) of a name the decompilation's build derives (derived_names.json), or None."""
    if name in derived.get("levels", {}):
        level, at = derived["levels"][name]
        return int(level), int(str(at), 16)
    if name in derived.get("values", {}):
        v = derived["values"][name]
        return -1, int(str(v[-1] if isinstance(v, list) else v), 16)
    return None


def function_place(program, name: str, level: int) -> tuple[int, int] | None:
    """The function's place in program LEVEL (where its table's addresses point), else its first."""
    places = list(program.code_places(name)) if hasattr(program, "code_places") else []
    for place in places:
        if place[0] == level:
            return place
    if places:
        return places[0]
    a = address_of(program, name)
    return (level_of(name), a) if a is not None else None


def size_at(program, place: tuple[int, int], name: str) -> int | None:
    """The catalogue's size of NAME, else the distance to the next function in its program."""
    if program.sizes.get(name):
        return program.sizes[name]
    level, at = place
    after = [a for (lv, a) in program.by_place if lv == level and a > at]
    return min(after) - at if after else None


def _li_v0(w: int) -> int | None:
    """The constant `li $v0, n` (addiu or ori from $zero) sets, else None."""
    if (w >> 21) & 0x1F or ((w >> 16) & 0x1F) != 2:
        return None
    if w >> 26 == 9:
        return (w & 0xFFFF) - 0x10000 if w & 0x8000 else w & 0xFFFF
    return w & 0xFFFF if w >> 26 == 13 else None


def asm_lead(r: retail.Retail, level: int, addr: int, names: dict, limit: int = 96) -> list:
    """What the code at ADDR does first: ["call", name, next name] (its first two calls), or ["return"]
    (["return", value] when it returns a constant), following unconditional branches (GCC's cross-jumping
    sends identical tails to one copy). Like _label_leads, the second call is read past any conditional
    branch, so it only adds to a match."""
    pc, calls, prev = addr, [], None
    for _ in range(limit):
        w = r.word(level, pc)
        if w is None:
            break
        op = w >> 26
        if op == 3:
            t = (pc & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
            calls.append(names.get((level, t)) or names.get((-1, t)) or f"0x{t:08X}")
            if len(calls) == 2:
                break
            pc, prev = pc + 4, None
            continue
        if op == 2:  # j
            pc, prev = (pc & 0xF0000000) | ((w & 0x3FFFFFF) << 2), None
            continue
        if op == 4 and ((w >> 16) & 0x3FF) == 0:  # b (beq $0, $0)
            off = w & 0xFFFF
            pc, prev = pc + 4 + ((off - 0x10000 if off & 0x8000 else off) << 2), None
            continue
        if op == 0 and (w & 0x3F) == 8:  # jr
            if calls:
                break
            slot = r.word(level, pc + 4)
            value = _li_v0(slot) if slot is not None else None
            if value is None and prev is not None:
                value = _li_v0(prev)
            return ["return"] if value is None else ["return", value]
        pc, prev = pc + 4, w
    return ["call", *calls] if calls else ["return"]


def _callee(name: str | None) -> object:
    """A callee as an address where its name says one (func_0032D238, func_32D238, 0x0032D238)."""
    m = re.search(r"(?:0x|_)([0-9A-Fa-f]{6,8})$", name or "")
    return int(m.group(1), 16) if m else name


def _same_call(x: str | None, y: str | None) -> bool | None:
    """Whether two callees are one function; None when Y is a derived name (not comparable)."""
    a, b = _callee(x), _callee(y)
    if isinstance(b, str) and b.startswith(("ref_", "r1_")):
        return None
    return a == b


def score(a: list, b: list) -> int:
    """How well the code at an address (A) and a label's (B) agree. The first call decides (3 or -1);
    a same second call adds 2 (a different one nothing, see asm_lead). Returns of two constants count
    like a first call."""
    if a[0] == "call" and b[0] == "call":
        same = _same_call(a[1], b[1])
        if same is None:
            return 0  # a call through a derived name: not comparable
        if not same:
            return -1
        return 3 + (2 if len(a) > 2 and len(b) > 2 and _same_call(a[2], b[2]) else 0)
    if a[0] == "return" and b[0] == "return" and len(a) > 1 and len(b) > 1:
        return 3 if a[1] == b[1] else -1
    return 1 if a[0] == b[0] else -1


def _named_index(label: str) -> int | None:
    m = re.search(r"(\d+)$", label)
    return int(m.group(1)) if m else None


def align(addresses: list[int], leads: dict[int, list], labels: list, indices: dict[int, set]) -> dict[int, str]:
    """Each address (ascending) to a label (source order), keeping their orders, best agreement: the same
    calls, a label named by one of the address's indices (c3, L1_3, rc2_case_3), and few labels passed over.
    Labels numbered from 1 (c1..c5 for indices 0..4) name index n - 1. A name only breaks a tie between
    leads: it weighs less than a call."""
    n, m = len(addresses), len(labels)
    numbers = [k for k in (_named_index(label) for label, _ in labels) if k is not None]
    base = 1 if numbers and min(numbers) == 1 else 0
    NEG = -10 ** 9

    def named(label: str) -> int | None:
        k = _named_index(label)
        return None if k is None else k - base
    best = [[NEG] * (m + 1) for _ in range(n + 1)]
    take = [[False] * (m + 1) for _ in range(n + 1)]
    for j in range(m + 1):
        best[0][j] = -j
    for i in range(1, n + 1):
        for j in range(1, m + 1):
            skip = best[i][j - 1] - 1
            a, (label, lead) = addresses[i - 1], labels[j - 1]
            use = best[i - 1][j - 1] + score(leads[a], lead) + (2 if named(label) in indices[a] else 0)
            if use >= skip:
                best[i][j], take[i][j] = use, True
            else:
                best[i][j] = skip
    out = {}
    i, j = n, m
    while i > 0 and j > 0:
        if take[i][j]:
            out[addresses[i - 1]] = labels[j - 1][0]
            i, j = i - 1, j - 1
        else:
            j -= 1
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--game", type=Path, required=True)
    ap.add_argument("--program", type=Path, required=True, help="hostgen's --program-out")
    ap.add_argument("--report", type=Path, required=True, help="hostgen's report.json")
    ap.add_argument("--image", type=Path, required=True, help="your disc image")
    args = ap.parse_args()

    cfg = json.loads(args.game.read_text())
    program = pickle.loads(args.program.read_bytes())["program"]
    program.index_places()
    names = {place: sym for place, sym in program.by_place.items()}
    low = (getattr(program, "relocate", None) or [0x15F000])[0]
    r = retail.Retail.from_image(args.image, low)
    report = json.loads(args.report.read_text())

    target = cfg.get("jump_tables", "tables/jump_tables.json")
    path = args.game.parent / (target if isinstance(target, str) else "tables/jump_tables.json")
    tables = json.loads(path.read_text()) if path.is_file() else {}
    derived_file = args.game.parent / "tables" / "derived_names.json"
    derived = json.loads(derived_file.read_text()) if derived_file.is_file() else {}
    for fn, gotos in sorted(report.get("computed_gotos", {}).items()):
        for g in gotos:
            name = g["table"]
            if name in tables:
                continue
            at, tlevel = address_of(program, name), level_of(name)
            if name.startswith(("ref_", "r1_")) and derived_address(derived, name):
                tlevel, at = derived_address(derived, name)
            if at is None:
                print(f"{fn}: {name} has no address; skipped", file=sys.stderr)
                continue
            place = function_place(program, fn, tlevel if tlevel >= 0 else level_of(fn))
            size = size_at(program, place, fn) if place is not None else None
            if place is None or not size:
                print(f"{fn}: no place or size; skipped", file=sys.stderr)
                continue
            level, start = place
            if tlevel < 0:
                tlevel = level
            words = []
            while True:
                w = r.word(tlevel, at + 4 * len(words))
                if w is None or not (start <= w < start + size):
                    break
                words.append(w)
            if not words:
                print(f"{fn}: {name} holds no address in the function; skipped", file=sys.stderr)
                continue
            distinct = sorted(set(words))
            leads = {a: asm_lead(r, level, a, names) for a in distinct}
            indices = {a: {i for i, w in enumerate(words) if w == a} for a in distinct}
            chosen = align(distinct, leads, g["labels"], indices)
            row = [chosen.get(w) for w in words]
            lead_of = dict((l[0], l[1]) for l in g["labels"])
            for i, (w, label) in enumerate(zip(words, row)):
                ok = label is not None and score(leads[w], lead_of.get(label)) > 0
                print(f"{fn} {name}[{i}] 0x{w:08X} {leads[w]} -> {label} {lead_of.get(label)}{'' if ok else '   CHECK'}")
            tables[name] = row
    path.write_text(json.dumps(dict(sorted(tables.items())), indent=1) + "\n")
    print(f"jumptables: {len(tables)} tables in {path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
