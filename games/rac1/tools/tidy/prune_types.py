#!/usr/bin/env python3
"""
Delete file-scope struct and union types that nothing in the file uses.

  python3 tools/tidy/prune_types.py src/game            # dry run: show counts
  python3 tools/tidy/prune_types.py src/game --write
  python3 tools/tidy/prune_types.py src/game --list     # show what would go

A type counts as used if its name or tag appears anywhere else in the file's code, in its version fragments
(src/versions/<K>/<file>/), or in a macro body in include/. Types that only dead types use go too.

Kept: a struct that also defines a variable, and one with a provenance note inside. A type has no bytes of its
own, but run tools/check_all.sh after anyway.
"""
import collections
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import prune_externs as pe  # noqa: E402

ROOT = pe.ROOT
BLOCK = re.compile(r"^(typedef\s+)?(?:struct|union)\b[^;{()]*\{", re.M)
ATTRIBUTE = re.compile(r"__attribute__\s*\(\(.*?\)\)", re.S)


def find_types(text):
    """[(start, end, names)] of each file-scope type definition that defines only types."""
    masked = pe.mask(text)
    found = []
    for m in BLOCK.finditer(masked):
        i, depth = m.end(), 1
        while i < len(masked) and depth:
            depth += (masked[i] == "{") - (masked[i] == "}")
            i += 1
        end = masked.find(";", i)
        if end < 0:
            continue
        tail = ATTRIBUTE.sub(" ", masked[i:end]).strip()
        if not m.group(1) and tail:
            continue                        # struct { ... } variable;
        names = set(pe.WORD.findall(tail))
        tag = re.search(r"(?:struct|union)\s+(\w+)", masked[m.start():m.end()])
        if tag:
            names.add(tag.group(1))
        if names:
            found.append((m.start(), end + 1, names))
    return found


def dead_types(text, fragments, macro_words):
    """The find_types entries nothing uses, directly or through a live type."""
    alive = find_types(text)
    everything = list(alive)
    # Everything that is not a type definition counts as a use. So do live types, below.
    rest, last = [], 0
    for start, end, _ in everything:
        rest.append(text[last:start])
        last = end
    rest.append(text[last:])
    used = set(pe.WORD.findall(pe.without_comments("".join(rest) + "\n" + fragments))) | macro_words
    while True:
        words = collections.Counter()
        for start, end, _ in alive:
            words.update(pe.WORD.findall(pe.without_comments(text[start:end])))
        keep = []
        for block in alive:
            own = collections.Counter(pe.WORD.findall(pe.without_comments(text[block[0]:block[1]])))
            others = words - own
            if block[2] & used or any(others[name] for name in block[2]):
                keep.append(block)
        if len(keep) == len(alive):
            return [b for b in everything if b not in alive]
        alive = keep


def prune(text, fragments, macro_words):
    """(new text, removed names, kept for provenance)."""
    remove, removed, provenance = [], [], 0
    for start, end, names in dead_types(text, fragments, macro_words):
        stop = pe.trailing_comment_end(text, end)
        if pe.PROVENANCE.search(text[start:stop]):
            provenance += 1
            continue
        remove.append((start, stop))
        removed.append(sorted(names))
    if not remove:
        return text, [], provenance
    parts, last = [], 0
    for start, stop in remove:
        parts.append(text[last:start])
        last = stop
    parts.append(text[last:])
    return re.sub(r"\n{3,}", "\n\n", "".join(parts)), removed, provenance


def main():
    args = sys.argv[1:]
    write, listing = "--write" in args, "--list" in args
    paths = [a for a in args if not a.startswith("--")]
    if not paths:
        sys.exit(__doc__)
    macro_words = pe.header_macro_words()
    total = kept = 0
    for rel in pe.sources(paths):
        path = ROOT / rel
        text = path.read_text(errors="surrogateescape")
        new, removed, provenance = prune(text, pe.fragments_text(rel), macro_words)
        total += len(removed)
        kept += provenance
        if listing:
            for names in removed:
                print(f"{rel}: {' '.join(names)}")
        if write and new != text:
            path.write_text(new, errors="surrogateescape")
    print(f"{'removed' if write else 'would remove'} {total} types; kept {kept} with a provenance note")


if __name__ == "__main__":
    main()
