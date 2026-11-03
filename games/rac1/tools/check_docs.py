#!/usr/bin/env python3
"""
Fail if a Markdown link in the repository points to a file that does not exist.

  python3 tools/check_docs.py

Only relative links are checked. Web links are not fetched, so this needs no network.
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LINK = re.compile(r"\[[^\]]*\]\(([^)\s]+)\)")
# Vendored or generated Markdown: its links are not ours to fix.
SKIP = ("tools/ext/", "nonmatching/README.md")


def markdown_files():
    out = subprocess.run(["git", "ls-files", "*.md"], cwd=ROOT, capture_output=True, text=True, check=True).stdout
    return [p for p in out.splitlines() if not p.startswith(SKIP) and (ROOT / p).exists()]


def strip_code(text):
    """Links inside code are examples, not links."""
    text = re.sub(r"```.*?```", "", text, flags=re.S)
    return re.sub(r"`[^`\n]*`", "", text)


def broken_links(rel):
    path = ROOT / rel
    bad = []
    for target in LINK.findall(strip_code(path.read_text(errors="replace"))):
        if re.match(r"[a-z]+:", target) or target.startswith("#"):
            continue
        if not (path.parent / target.split("#")[0]).exists():
            bad.append(target)
    return bad


def main():
    files = markdown_files()
    bad = [(rel, target) for rel in files for target in broken_links(rel)]
    for rel, target in bad:
        print(f"{rel}: {target}")
    if bad:
        sys.exit(f"*** {len(bad)} broken link(s)")
    print(f"{len(files)} Markdown files: every relative link works")


if __name__ == "__main__":
    main()
