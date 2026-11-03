#!/usr/bin/env python3
"""
Rename or move one source file, and everything that names its path.

  python3 tools/move_src.py src/core/00116FA0.c src/core/libc/stdio.c
  python3 tools/move_src.py --dry-run OLD NEW
  python3 tools/move_src.py --check        # every object and flag row names a real file

An executable object (src/core, src/game) may move into a subfolder of its own segment. An overlay
file may only be renamed inside its own folder. A move never changes what compiles: the object keeps
its line in the object list, so its link order, compiler and flags stay as they are.

The report keeps the old unit name (config/unit_aliases.tsv), so progress history survives.
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ALIASES = "config/unit_aliases.tsv"
OBJECT_LISTS = ["config/text.objects", "config/core_text.objects"]
FLAGS = "config/file_cflags.txt"
NAMES = "config/overlays/file_names.tsv"
# Text that names source paths. Generated reports are not here: their generators rewrite them.
REFS = ["config/*.txt", "config/*.objects", "config/*.tsv", "config/versions/*/map.tsv", "docs/**/*.md", "notes/**/*",
        "nonmatching/**/*", "tools/**/*.py", "tools/**/*.sh", "src/**/*.c", "include/**/*.h", "*.md",
        "Makefile.sn", "rac1.ld.sh"]
# The NTSC project's own paths, and tables generated from them, are not ours to rewrite.
SKIP = {"tools/move_src.py", "tools/tests/test_move_src.py", "config/modules.tsv", "config/names_ntsc.tsv",
        "config/names.tsv"}


def check_move(root, old, new):
    """Why this move is not allowed, or None."""
    if not (root / old).is_file():
        return f"{old} does not exist"
    if (root / new).exists():
        return f"{new} exists already"
    if not new.endswith(".c") or not re.fullmatch(r"[A-Za-z0-9_./-]+", new):
        return f"{new}: not a plain .c path"
    if old.startswith(("src/core/", "src/game/")):
        segment = old.split("/")[1]
        if not new.startswith(f"src/{segment}/"):
            return f"{new} leaves src/{segment}/: that changes the compile rule"
        return None
    if old.startswith("src/overlays/"):
        if Path(old).parent != Path(new).parent:
            return f"{new}: an overlay file stays in its own folder"
        return None
    return f"{old}: only src/core, src/game and src/overlays files can move"


def stem(path):
    """src/game/hud.c -> game/hud"""
    return path[len("src/"):-len(".c")]


def substitutions(old, new):
    """(regex, replacement) for each way the tree names this file."""
    a, b = stem(old), stem(new)
    subs = [
        (rf"\b{re.escape(old)}\b", new),
        (rf"\bbuild-sn/{re.escape(a)}\.o\b", f"build-sn/{b}.o"),
        # A version's own copies live in a folder named after the shared file.
        (rf"\bsrc/versions/([A-Za-z0-9_]+)/{re.escape(a)}(?=/)", rf"src/versions/\1/{b}"),
        # Its header (tools/tidy/move_protos.py), and the guard named after it.
        (rf'"{re.escape(a)}\.h"', f'"{b}.h"'),
        (rf"\b{re.sub(r'[^A-Za-z0-9]', '_', a).upper()}_H\b", f"{re.sub(r'[^A-Za-z0-9]', '_', b).upper()}_H"),
    ]
    if old.startswith("src/core/"):
        # Makefile.sn names a few core objects by their pattern stem.
        subs.append((rf'"\$\*" = {re.escape(a.split("/", 1)[1])}(?=[ ;\]])', f'"$*" = {b.split("/", 1)[1]}'))
    return [(re.compile(p), r) for p, r in subs]


def rewrite(text, subs):
    for pat, rep in subs:
        text = pat.sub(rep, text)
    return text


def update_refs(root, old, new, dry_run):
    subs = substitutions(old, new)
    changed = []
    seen = set()
    for pattern in REFS:
        for path in sorted(root.glob(pattern)):
            rel = path.relative_to(root).as_posix()
            if rel in seen or rel in SKIP or not path.is_file():
                continue
            seen.add(rel)
            try:
                text = path.read_text()
            except UnicodeDecodeError:
                continue
            after = rewrite(text, subs)
            if after != text:
                changed.append(rel)
                if not dry_run:
                    path.write_text(after)
    return changed


def update_names_table(root, old, new):
    """An overlay's table row gives the current name in its second column."""
    path = root / NAMES
    if not old.startswith("src/overlays/") or not path.exists():
        return
    lines = path.read_text().splitlines(keepends=True)
    for i, line in enumerate(lines):
        cols = line.rstrip("\n").split("\t")
        if line.startswith("#") or len(cols) < 2:
            continue
        if str(Path(cols[0]).parent / cols[1]) == old:
            cols[1] = Path(new).name
            lines[i] = "\t".join(cols) + "\n"
    path.write_text("".join(lines))


def load_aliases(root):
    """{current unit name: report unit name}"""
    path = root / ALIASES
    if not path.exists():
        return {}
    rows = (l.split("\t") for l in path.read_text().splitlines() if l and not l.startswith("#"))
    return {cur: first for cur, first in rows}


def save_alias(root, old, new):
    if not old.startswith(("src/core/", "src/game/")):
        return                              # a level is one report unit, whatever its files are called
    aliases = load_aliases(root)
    first = aliases.pop(stem(old), stem(old))
    if stem(new) != first:
        aliases[stem(new)] = first
    head = ("# Report unit names (tools/move_src.py): a moved object's current name, then the name the report\n"
            "# keeps for it, so progress history survives the move.\n")
    (root / ALIASES).write_text(head + "".join(f"{c}\t{f}\n" for c, f in sorted(aliases.items())))


def move_version_dirs(root, old, new):
    """Move src/versions/<K>/<old stem>/ to <new stem>/. Returns the folders moved."""
    moved = []
    versions = root / "src/versions"
    for version in sorted(p for p in versions.iterdir() if p.is_dir()) if versions.is_dir() else []:
        src, dst = version / stem(old), version / stem(new)
        if src.is_dir():
            # Through a temporary name: game/pause/ may have to move into game/pause/pause/.
            tmp = version / (stem(old).replace("/", "_") + ".moving")
            subprocess.run(["git", "mv", str(src), str(tmp)], cwd=root, check=True)
            dst.parent.mkdir(parents=True, exist_ok=True)
            subprocess.run(["git", "mv", str(tmp), str(dst)], cwd=root, check=True)
            moved.append(version.name)
    return moved


def move_header(root, old, new):
    """Move include/<old stem>.h with its source, if it has one."""
    src, dst = root / "include" / f"{stem(old)}.h", root / "include" / f"{stem(new)}.h"
    if src.exists():
        dst.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(["git", "mv", str(src), str(dst)], cwd=root, check=True)


def move(root, old, new, dry_run=False):
    why = check_move(root, old, new)
    if why:
        sys.exit(f"*** {why}")
    if dry_run:
        for rel in update_refs(root, old, new, True):
            print(f"would edit {rel}")
        return
    (root / new).parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(["git", "mv", old, new], cwd=root, check=True)
    versions = move_version_dirs(root, old, new)
    move_header(root, old, new)
    changed = update_refs(root, old, new, False)
    update_names_table(root, old, new)
    save_alias(root, old, new)
    print(f"{old} -> {new}: {len(changed)} files edited, version folders: {', '.join(versions) or 'none'}")


def check(root):
    """Every object and flag row must name a file that exists."""
    missing = []
    for rel in OBJECT_LISTS:
        for line in (root / rel).read_text().splitlines():
            m = re.match(r"build-sn/((?:core|game)/\S+)\.o\b", line)
            if m and not (root / f"src/{m.group(1)}.c").exists():
                missing.append(f"{rel}: src/{m.group(1)}.c")
    for line in (root / FLAGS).read_text().splitlines():
        if line and not line.startswith("#") and not (root / line.split()[0]).exists():
            missing.append(f"{FLAGS}: {line.split()[0]}")
    if missing:
        sys.exit("*** missing sources:\n" + "\n".join(missing))
    print("every object and flag row names a real file")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("old", nargs="?")
    ap.add_argument("new", nargs="?")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    if args.check:
        check(ROOT)
    elif args.old and args.new:
        move(ROOT, args.old, args.new, args.dry_run)
    else:
        ap.error("give OLD and NEW, or --check")


if __name__ == "__main__":
    main()
