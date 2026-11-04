#!/usr/bin/env python3
"""
Drop the address suffix from local type names.

  python3 tools/tidy/drop_type_suffixes.py            # dry run: show counts
  python3 tools/tidy/drop_type_suffixes.py --write
  python3 tools/tidy/drop_type_suffixes.py --list

`Vars_A7078` becomes `Vars` when the short name is free: not a word in the file or its version fragments, not in any
header, and not a name in include/names/. A type name has no bytes. If two types in a file would get one name, only the
first does.
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import merge_types as mt  # noqa: E402
import prune_externs as pe  # noqa: E402
import prune_types as pt  # noqa: E402

ROOT = pe.ROOT
# Each suffix has a digit: `Decade` is all hex letters, but it is a word.
SUFFIXED = re.compile(r"^([A-Za-z_]\w*?[A-Za-z])(?:_(?=[0-9A-Fa-f]*[0-9])[0-9A-Fa-f]{5,6})+$")
PROVENANCE = re.compile(r"Adapted from|Lombyte|GPL|\(c\)|NTSC decomp", re.I)


def short_name(name):
    """The name without its address suffix, or None if it has none or nothing readable is left."""
    m = SUFFIXED.match(name)
    if not m or len(m.group(1)) < 2:
        return None
    return m.group(1)


def plan(text, fragments, taken):
    """{old name: new name} for this file. taken holds words the whole tree reserves."""
    words = set(pe.WORD.findall(pe.without_comments(text + "\n" + fragments)))
    renames = {}
    for _, _, names in pt.find_types(text):
        for name in sorted(names):
            new = short_name(name)
            if new and new not in words and new not in taken and new not in renames.values():
                renames[name] = new
    return renames


def rename_all(text, renames):
    """Rename in code, and in comments up to any provenance note."""
    pattern = re.compile(r"\b(" + "|".join(map(re.escape, sorted(renames, key=len, reverse=True))) + r")\b")

    def comment(m):
        if m.group(0)[0] != "/":
            return m.group(0)
        start = PROVENANCE.search(m.group(0))
        at = start.start() if start else len(m.group(0))
        return pattern.sub(lambda w: renames[w.group(1)], m.group(0)[:at]) + m.group(0)[at:]

    return pe.COMMENT_OR_STRING.sub(comment, mt.rename_code(text, renames))


def reserved():
    """Words in every header, and every name in the table."""
    words = set()
    for path in (ROOT / "include").rglob("*.h"):
        words |= set(pe.WORD.findall(path.read_text(errors="replace")))
    return words


def main():
    write, listing = "--write" in sys.argv[1:], "--list" in sys.argv[1:]
    taken = reserved()
    total = files = 0
    for rel in pe.sources(["src"]):
        if rel.startswith("src/data/"):
            continue
        path = ROOT / rel
        text = path.read_text(errors="surrogateescape")
        renames = plan(text, pe.fragments_text(rel), taken)
        if not renames:
            continue
        total += len(renames)
        files += 1
        if listing:
            for old, new in sorted(renames.items()):
                print(f"{rel}: {old} -> {new}")
        if write:
            path.write_text(rename_all(text, renames), errors="surrogateescape")
            for fragment in mt.fragment_paths(rel):
                old = fragment.read_text(errors="surrogateescape")
                new = rename_all(old, renames)
                if new != old:
                    fragment.write_text(new, errors="surrogateescape")
    print(f"{'renamed' if write else 'would rename'} {total} types in {files} files")


if __name__ == "__main__":
    main()
