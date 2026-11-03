#!/usr/bin/env python3
"""
Counts of things the refactor removes. A count may go down, but never up.

  python3 tools/ratchets.py --check     # fail if a count went up
  python3 tools/ratchets.py --update    # save lower counts (config/ratchets.json)

The counts are regex searches of src/, not a parser: they measure a trend. Generated code (libgcc) and
version fragments and src/data/ are left out.
"""
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SAVED = ROOT / "config/ratchets.json"
COMMENT = re.compile(r"/\*.*?\*/|//[^\n]*", re.S)

# name: (where to search, regex). "code" has comments removed, "comments" is only comments.
METRICS = {
    "raw_offset_casts": ("code", r"\*\s*\(\s*(?:volatile\s+)?\w+(?:\s+\w+)?\s*\*+\s*\)\s*\(\s*\(?[\w>.-]+\)?\s*\+\s*0x"),
    "float_int_casts": ("code", r"\*\s*\(\s*(?:float|f32)\s*\*\s*\)\s*&"),
    "hex_compares": ("code", r"[!=]=\s*0x[0-9A-Fa-f]+"),
    "oclass_literals": ("code", r"oclass\s*[!=]=\s*(?:0x)?[0-9A-Fa-f]+"),
    "extern_lines": ("code", r"^\s*extern\b"),
    "local_struct_typedefs": ("code", r"^\s*typedef\s+struct\b"),
    "asm_aliases": ("code", r"\bextern\b[^;]*?__asm__\s*\("),
    "address_names_in_code": ("code", r"\b(?:func|D|FUN|Data)_(?:L\d+_)?[0-9A-Fa-f]{5,}\b"),
    "include_asm": ("code", r"^\s*INCLUDE_ASM\s*\("),
    "ids_in_comments": ("comments", r"\bMoby_\d+\b|\bclass\s+(?:0x[0-9A-Fa-f]+|\d+)\b"),
    "address_names_in_comments": ("comments", r"\b(?:func|D|FUN|Data)_(?:L\d+_)?[0-9A-Fa-f]{5,}\b"),
    "offsets_in_comments": ("comments", r"\+\s*0x[0-9A-Fa-f]+"),
    "worker_notes_in_comments": ("comments", r"\b(?:regalloc|permuter|functional:)|\bmatch(?:es|ed)? (?:now|retail)\b"),
    "numbered_names": ("code", r"\b(?:Moby|Class)\d{2,}\w*\b|\b\w*_class_[0-9a-f]+(?:_\w*)?\b(?<!_class_data)"),
    "fakematch_tags": ("comments", r"FAKEMATCH:"),
    "long_lines": ("raw", r"^.{121,}$"),
    "tab_lines": ("raw", r"^.*\t.*$"),
}


def sources():
    # Untracked files count too, so a check before `git add` agrees with CI. src/data/ has to name its address labels.
    args = ["git", "ls-files", "-co", "--exclude-standard", "src/*.c"]
    out = subprocess.run(args, cwd=ROOT, capture_output=True, text=True, check=True).stdout
    return [ROOT / f for f in out.split() if not f.startswith(("src/versions/", "src/libgcc/", "src/data/"))]


def measure():
    code_parts, comment_parts, raw_parts = [], [], []
    for path in sources():
        if not path.exists():
            continue
        text = path.read_text(errors="replace")
        raw_parts.append(text)
        code_parts.append(COMMENT.sub(" ", text))
        comment_parts.extend(COMMENT.findall(text))
    text_of = {"code": "\n".join(code_parts), "comments": "\n".join(comment_parts), "raw": "\n".join(raw_parts)}
    return {name: len(re.findall(rx, text_of[where], re.M)) for name, (where, rx) in METRICS.items()}


def saved():
    return json.loads(SAVED.read_text()) if SAVED.exists() else {}


def save(counts):
    SAVED.write_text(json.dumps(counts, indent=2, sort_keys=True) + "\n")


def main():
    mode = (sys.argv[1:] or ["--check"])[0]
    now, was = measure(), saved()
    if mode == "--update":
        worse = {k: (was[k], v) for k, v in now.items() if k in was and v > was[k]}
        if worse and "--force" not in sys.argv:
            sys.exit(f"*** counts went up, not saving: {worse}")
        save(now)
        print(f"saved {len(now)} counts")
    elif mode == "--check":
        worse = {k: (was.get(k), v) for k, v in now.items() if k not in was or v > was[k]}
        for name, (before, value) in sorted(worse.items()):
            print(f"{name}: {before} -> {value}")
        if worse:
            sys.exit(f"*** {len(worse)} count(s) went up. Fix them, or explain in the PR why the count must grow.")
        lower = sum(1 for k, v in now.items() if v < was[k])
        print(f"{len(now)} counts checked{f', {lower} lower: run --update' if lower else ''}")
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
