#!/usr/bin/env python3
"""
Give data blocks the type the code already reads them as.

  python3 tools/tidy/data_view_types.py            # dry run: show what fits
  python3 tools/tidy/data_view_types.py --write

If every declaration of a block in the code says the same type, the block takes it: `extern f32 fog[];` makes it
`f32 fog[]`, and a struct from a shared header works the same way. A `char` view says nothing about the shape, so it does
not count. A type stays out if the block's words do not fit it, because then the code's view is not the whole story.
Types have no bytes, but run tools/gate_loop.py afterwards.
"""
import re
import subprocess
import sys
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tools/tidy"))
import data_in_c as d  # noqa: E402
import data_types as dt  # noqa: E402

DECLARATION = re.compile(r"^\s*extern\s+((?:const\s+|volatile\s+)*(?:struct\s+|unsigned\s+)?[A-Za-z_]\w*)\s+(\w+)\s*(\[[^\]]*\])?[^;\n]*;", re.M)
SCALARS = {"f32": ("f", 4), "float": ("f", 4), "s32": ("i", 4), "int": ("i", 4), "unsigned int": ("i", 4),
           "s16": ("i", 2), "short": ("i", 2), "u16": ("i", 2), "unsigned short": ("i", 2)}
SAYS_NOTHING = {"char", "unsigned char", "signed char", "u8", "s8", "void", "long", "double", "s64", "u64", "u32"}


def views():
    """{symbol: Counter of (type)}, across the code and headers."""
    out = subprocess.run(["git", "ls-files", "src/*.c", "include/*.h"], cwd=ROOT, capture_output=True, text=True).stdout
    found = defaultdict(Counter)
    for rel in out.split():
        if rel.startswith(("src/data/", "include/names", "src/libgcc", "src/versions/")):
            continue
        text = re.sub(r"/\*.*?\*/|//[^\n]*", " ", (ROOT / rel).read_text(errors="replace"), flags=re.S)
        for m in DECLARATION.finditer(text):
            found[m.group(2)][re.sub(r"\b(?:const|volatile)\s+", "", m.group(1)).strip()] += 1
    return found


def chosen_type(counter):
    """The one type the views agree on, ignoring those that say nothing about the shape."""
    useful = {t for t in counter if t not in SAYS_NOTHING}
    return useful.pop() if len(useful) == 1 else None


def header_of(typ, headers):
    bare = typ.replace("struct ", "")
    for rel, text in headers.items():
        if re.search(rf"\bstruct\s+{bare}\b\s*\{{", text) or re.search(rf"\}}\s*{bare}\s*;", text):
            return rel
    return None


def layout(typ, headers, cache):
    """(header, size, fields) of a type, or None."""
    if typ in cache:
        return cache[typ]
    if typ in SCALARS:
        kind, size = SCALARS[typ]
        result = ("", size, [(0, kind, size)])
    else:
        header = header_of(typ, headers)
        try:
            result = dt.learn(f"{typ}:{header}") if header else None
        except (ValueError, KeyError):
            result = None
    cache[typ] = result
    return result


def main():
    write = "--write" in sys.argv[1:]
    found = views()
    names = d.readable_names()
    blocks = d.all_blocks()
    headers = {p.relative_to(ROOT / "include").as_posix(): re.sub(r"/\*.*?\*/|//[^\n]*", " ", p.read_text(errors="replace"), flags=re.S)
               for p in (ROOT / "include").rglob("*.h") if "names" not in p.parts and p.name != "names.h"}
    types, cache = dt.load(), {}
    pairs, refused = [], Counter()
    for label, element, _ in d.rows():
        if element != "u32":
            continue
        counter = found.get(label, Counter()) + found.get(names.get(label, ""), Counter())
        typ = chosen_type(counter)
        if not typ:
            continue
        learned = layout(typ, headers, cache)
        if not learned:
            refused["no layout"] += 1
            continue
        try:
            words = d.parse(blocks[label])[1]
            if not d.alignment_allows(label, len(words), learned[1]):
                raise ValueError("not on 8 bytes")
            d.render_typed(words, learned[2], learned[1])
        except ValueError:
            refused["does not fit"] += 1
            continue
        types[typ] = learned
        pairs.append(f"{label}={typ}")
    print(f"{len(pairs)} blocks fit; {dict(refused)}")
    print(Counter(p.split("=")[1] for p in pairs).most_common(14))
    if write:
        dt.save(types)
        d.set_types(pairs)


if __name__ == "__main__":
    main()
