#!/usr/bin/env python3
"""
Fail if git tracks a file that looks like retail or build data.

  python3 tools/check_no_retail.py

It looks for disc and binary file types, and for any file holding NUL bytes. It cannot spot retail data
pasted as text, so reviewers still check for that (docs/REVIEWING.md).
"""
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BANNED = {".iso", ".elf", ".bin", ".o", ".a", ".irx", ".vag", ".wad", ".img", ".7z", ".zip", ".pss"}
BANNED_NAMES = ("SCES_", "SCUS_", "SCPS_")
# Images in the docs are not disc data.
ALLOWED_BINARY = {".png", ".jpg", ".jpeg", ".gif", ".ico", ".svg", ".webp"}


def tracked():
    out = subprocess.run(["git", "ls-files", "-z"], cwd=ROOT, capture_output=True, check=True).stdout
    return [p for p in out.decode().split("\0") if p]


def problem(rel):
    path = ROOT / rel
    if rel.startswith("tools/ext/") or not path.is_file():
        return None                         # vendored upstream tools
    if path.suffix.lower() in BANNED:
        return "banned file type"
    if path.name.startswith(BANNED_NAMES) and path.suffix not in {".ld", ".txt", ".md", ".tsv", ".yaml"}:
        return "named like a game executable"
    if path.suffix.lower() not in ALLOWED_BINARY and b"\0" in path.read_bytes()[:1 << 20]:
        return "holds NUL bytes"
    return None


def main():
    files = tracked()
    bad = [(rel, why) for rel in files if (why := problem(rel))]
    for rel, why in bad:
        print(f"{rel}: {why}")
    if bad:
        sys.exit(f"*** {len(bad)} file(s) look like retail or build data")
    print(f"{len(files)} tracked files: no retail or build data found")


if __name__ == "__main__":
    main()
