#!/usr/bin/env python3
"""
Give overlay source files descriptive names (config/overlays/file_names.tsv).

  python3 tools/overlay_rename.py          # rename every row not yet applied
  python3 tools/overlay_rename.py --check  # fail if a row is not applied

tools/overlay_src.py names a file <unit>_<first address>.c, after the
executable unit before it, which says nothing about what the file holds.
Each row of file_names.tsv maps that generated path to a descriptive name
in the same directory. Renaming moves the file (git mv), records the old
name and the start address in the file's first comment, and rewrites the
path in config/file_cflags.txt, config/versions/*/map.tsv and docs/. A version's
own copies (src/versions/<K>/overlays/<dir>/<old stem>/func_*.c, tools/version_src.py) move
to the new stem's directory with it.
overlay_src.py reads the same table, so a re-run writes to the new name.
A file's name never reaches the compiled bytes: an overlay function is
placed by the address in its own symbol.
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TABLE = ROOT / "config/overlays/file_names.tsv"
REFS = ["config/file_cflags.txt", "config/versions/*/map.tsv", "docs/**/*.md"]
OLD_RE = re.compile(r"^(.+)_([0-9A-F]{8})\.c$")


def load():
    """[(old path, new path)] relative to ROOT, from file_names.tsv."""
    rows = []
    if not TABLE.exists():
        return rows
    for line in TABLE.read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        old, new = line.split("\t")[:2]
        rows.append((old, str(Path(old).parent / new)))
    return rows


def renamed(path):
    """The descriptive path for a generated overlay path (as a ROOT-relative str), or the path itself."""
    return dict(load()).get(path, path)


def note(text, old_name):
    """text with the old name and start address in its first comment."""
    m = OLD_RE.match(old_name)
    extra = f"Starts at 0x{m.group(2)} (was {old_name})."
    first, _, rest = text.partition("\n")
    if first.startswith("/*") and first.endswith("*/") and extra not in first:
        return f"{first[:-2].rstrip()} {extra} */\n{rest}"
    if extra in text.split("\n", 2)[0]:
        return text
    return f"/* {extra} */\n{text}"


def main():
    check = "--check" in sys.argv[1:]
    rows = load()
    news = [n for _, n in rows]
    dup = {n for n in news if news.count(n) > 1}
    if dup:
        sys.exit("file_names.tsv: same new name twice: " + ", ".join(sorted(dup)))
    todo = [(o, n) for o, n in rows if (ROOT / o).exists()]
    vdirs = [(old, new) for o, n in rows for k in sorted(p.name for p in (ROOT / "src/versions").iterdir())
             for old, new in [(f"src/versions/{k}/{o[4:-2]}", f"src/versions/{k}/{n[4:-2]}")]
             if (ROOT / old).is_dir()]
    if check:
        if todo or vdirs:
            sys.exit(f"{len(todo) + len(vdirs)} rows not applied, e.g. {(todo or vdirs)[0][0]}")
        print(f"{len(rows)} overlay files renamed")
        return
    for old, new in todo:
        if (ROOT / new).exists():
            sys.exit(f"{new} exists already")
        subprocess.run(["git", "mv", old, new], cwd=ROOT, check=True)
        p = ROOT / new
        p.write_text(note(p.read_text(), Path(old).name))
    for old, new in vdirs:
        (ROOT / new).parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(["git", "mv", old, new], cwd=ROOT, check=True)
    subs = dict(todo) | dict(vdirs)
    if subs:
        pat = re.compile("|".join(re.escape(o) for o in sorted(subs, key=len, reverse=True)))
        for glob in REFS:
            for p in sorted(ROOT.glob(glob)):
                t = p.read_text()
                t2 = pat.sub(lambda m: subs[m.group(0)], t)
                if t2 != t:
                    p.write_text(t2)
    print(f"renamed {len(todo)} files and {len(vdirs)} version directories")


if __name__ == "__main__":
    main()
