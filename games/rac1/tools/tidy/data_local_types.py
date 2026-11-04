#!/usr/bin/env python3
"""
Give data blocks the struct type their code already uses, when that type is local to a C file.

  python3 tools/tidy/data_local_types.py            # dry run: show what fits
  python3 tools/tidy/data_local_types.py --write

A data file is compiled on its own, so it cannot see a struct defined in src/game/ui/pause.c. If a block's owner defines
its type at file scope, the struct moves to include/structs/data/<file>.h, with the local structs it needs, and both files
include that. A block moves only if the words fit the type's fields. Types have no bytes, but run tools/gate_loop.py after.
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tools/tidy"))
import data_in_c as d  # noqa: E402
import data_types as dt  # noqa: E402
import prune_externs as pe  # noqa: E402

SCALARS = {"char", "int", "short", "u8", "u16", "u32", "s32", "s16", "s8", "long", "float", "f32", "unsigned", "void",
           "double", "u64", "s64"}
EXTERN = r"^\s*extern\s+((?:const\s+)?(?:struct\s+)?[A-Za-z_]\w*)\s+{name}\s*(?:\[[^\]]*\])?\s*(?:MACRO_ADDR|NOT_SDA)?\s*;"


def masked(text):
    return pe.COMMENT_OR_STRING.sub(lambda m: re.sub(r"[^\n]", " ", m.group(0)), text)


def closing_brace(mask, start):
    depth = 0
    for at in range(start, len(mask)):
        if mask[at] == "{":
            depth += 1
        elif mask[at] == "}":
            depth -= 1
            if not depth:
                return at
    return None


def definitions(text):
    """{type spelling: (start, end)} of the structs at file scope. A name defined twice is left out."""
    mask = masked(text)
    found, seen = {}, set()
    for m in re.finditer(r"^(typedef\s+)?struct(?:\s+(\w+))?\s*\{", mask, re.M):
        end = closing_brace(mask, m.end() - 1)
        if end is None:
            continue
        tail = re.match(r"\s*(?:__attribute__\s*\(\(.*?\)\)\s*)?(\w+)?\s*(?:__attribute__\s*\(\(.*?\)\)\s*)?;\n?", mask[end + 1:])
        if not tail:
            continue
        stop = end + 1 + tail.end()
        names = []
        if m.group(1) and tail.group(1):
            names.append(tail.group(1))
        if m.group(2):
            names.append(f"struct {m.group(2)}")
        for name in names:
            if name in seen:
                found.pop(name, None)
            elif not name.startswith("struct ") or m.group(2):
                found[name] = (m.start(), stop)
            seen.add(name)
    return found


def with_needs(name, text, defs):
    """The type and the local types its body names, those first, as (start, end) spans in order."""
    order, stack = [], [name]
    while stack:
        current = stack.pop()
        span = defs.get(current)
        if span is None or span in order:
            continue
        order.append(span)
        body = masked(text[span[0]:span[1]])
        for word in set(re.findall(r"\b[A-Za-z_]\w*\b", body)):
            for key in (word, f"struct {word}"):
                if key in defs and defs[key] != span:
                    stack.append(key)
    return sorted(order)


def header_for(owner):
    return f"structs/data/{owner[len('src/'):-len('.c')].replace('/', '_')}.h"


def candidates():
    """[(label, type, owner file)] for u32 blocks whose owner defines the type the code declares."""
    names = d.readable_names()
    out = []
    for label, element, source in d.rows():
        if element != "u32" or source.startswith("src/data/shared/"):
            continue
        owner = "src/" + source[len("src/data/"):]
        path = ROOT / owner
        if not path.exists():
            continue
        text = path.read_text(errors="surrogateescape")
        for symbol in {label, names.get(label, label)}:
            m = re.search(EXTERN.format(name=symbol), masked(text), re.M)
            if m and m.group(1) not in SCALARS:
                spelled = m.group(1).replace("const ", "")
                if spelled in definitions(text):
                    out.append((label, spelled, owner))
                break
    return out


def header_text(owner, parts):
    guard = "STRUCTS_DATA_" + re.sub(r"\W", "_", owner[len("src/"):-len(".c")]).upper() + "_H"
    return f'#ifndef {guard}\n#define {guard}\n\n#include "common.h"\n\n' + "\n".join(parts) + f"\n#endif /* {guard} */\n"


def main():
    write = "--write" in sys.argv[1:]
    blocks = d.all_blocks()
    by_owner = {}
    for label, typ, owner in candidates():
        by_owner.setdefault(owner, []).append((label, typ))
    moved = refused = 0
    pairs = []
    for owner, wanted in sorted(by_owner.items()):
        path = ROOT / owner
        original = path.read_text(errors="surrogateescape")
        defs = definitions(original)
        header = header_for(owner)
        target = ROOT / "include" / header
        if target.exists():
            print(f"{owner}: {header} is already here, so this file stays as it is")
            continue
        spans, fitting = [], []
        target.parent.mkdir(parents=True, exist_ok=True)
        for label, typ in wanted:
            needed = with_needs(typ, original, defs)
            trial = sorted(set(spans) | set(needed))
            target.write_text(header_text(owner, [original[a:b] for a, b in trial]))
            try:
                learned = dt.learn(f"{typ}:{header}")
                d.render_typed(d.parse(blocks[label])[1], learned[2], learned[1])
            except (ValueError, KeyError) as problem:
                print(f"{label} {typ}: stays: {str(problem)[:70]}")
                refused += 1
                continue
            spans = trial
            fitting.append((label, typ))
        if not fitting:
            target.unlink()
            continue
        target.write_text(header_text(owner, [original[a:b] for a, b in spans]))
        moved += len(fitting)
        print(f"{owner}: {len(fitting)} blocks, {len(spans)} structs")
        if not write:
            target.unlink()
            continue
        new, last = [], 0
        for a, b in spans:
            new += [original[last:a]]
            last = b
        new.append(original[last:])
        text = "".join(new)
        at = d.after_first_includes(text) if hasattr(d, "after_first_includes") else re.search(r'^#include "common.h"\n', text, re.M).end()
        path.write_text(text[:at] + f'#include "{header}"\n' + text[at:], errors="surrogateescape")
        learned_types = dt.load()
        for label, typ in fitting:
            learned_types[typ] = dt.learn(f"{typ}:{header}")
            pairs.append(f"{label}={typ}")
        dt.save(learned_types)
    if write and pairs:
        d.set_types(pairs)
    print(f"{'moved' if write else 'would move'}: {moved} blocks fit, {refused} do not")


if __name__ == "__main__":
    main()
