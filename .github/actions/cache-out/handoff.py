# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Hand a cache from develop's merge queue to develop (.github/actions/cache-out).

pack OUT: put the files that KEY's PATHS name (as actions/cache would find them) in OUT/<name>/,
as cache.tar with absolute names and manifest.json, and print name= and dir= for $GITHUB_OUTPUT.
unpack DIR SLOTS: extract each artifact under DIR to where it came from and print key_N and path_N
(N from 1 to SLOTS) for $GITHUB_OUTPUT, for cache-handoff.yml's save steps.
"""

import glob
import hashlib
import json
import os
from pathlib import Path
import re
import sys
import tarfile


def expand(pattern):
    """The files PATTERN matches, as actions/cache's globbing would: a directory means its tree."""
    found = set()
    for match in glob.glob(os.path.expanduser(pattern), recursive=True):
        path = Path(match).absolute()
        if path.is_dir():
            found.update(p for p in path.rglob("*") if p.is_file() or p.is_symlink())
        elif path.exists() or path.is_symlink():
            found.add(path)
    return found


def matched(paths):
    """The files PATHS (one pattern per line, "!" to leave out) name."""
    included, excluded = set(), set()
    for line in paths.splitlines():
        line = line.strip()
        if line:
            (excluded if line.startswith("!") else included).update(expand(line.lstrip("!")))
    return sorted(included - excluded)


def output(lines):
    with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as stream:
        stream.write("".join(lines))


def multiline(name, value):
    delimiter = "EOF_" + hashlib.sha1(value.encode()).hexdigest()
    return f"{name}<<{delimiter}\n{value}\n{delimiter}\n"


def pack(out):
    key, paths = os.environ["KEY"], os.environ["PATHS"]
    files = matched(paths)
    if not files:
        print(f"cache {key}: no files")
        return
    # Readable, and unique per key even when two long keys share their start.
    name = f"cache-{re.sub(r'[^A-Za-z0-9._-]', '_', key)[:150]}-{hashlib.sha1(key.encode()).hexdigest()[:10]}"
    folder = Path(out) / name
    folder.mkdir(parents=True, exist_ok=True)
    with tarfile.open(folder / "cache.tar", "w") as archive:
        for path in files:
            archive.add(path, arcname=str(path).lstrip("/"), recursive=False)
    (folder / "manifest.json").write_text(json.dumps({"key": key, "path": paths}), encoding="utf-8")
    print(f"cache {key}: {len(files)} files")
    output([f"name={name}\n", f"dir={folder}\n"])


def unpack(root, slots):
    # One artifact downloaded by name lands in ROOT itself, several in a folder each.
    manifests = sorted([*Path(root).glob("manifest.json"), *Path(root).glob("*/manifest.json")])
    if len(manifests) > slots:
        print(f"::warning::{len(manifests)} caches, only the first {slots} are saved")
    lines = []
    for number, manifest in enumerate(manifests[:slots], 1):
        data = json.loads(manifest.read_text(encoding="utf-8"))
        with tarfile.open(manifest.parent / "cache.tar") as archive:
            archive.extractall("/", filter="tar")
        (manifest.parent / "cache.tar").unlink()
        print(f"cache {data['key']}")
        lines += [multiline(f"key_{number}", data["key"]), multiline(f"path_{number}", data["path"])]
    output(lines)


if __name__ == "__main__":
    if sys.argv[1] == "pack":
        pack(sys.argv[2])
    else:
        unpack(sys.argv[2], int(sys.argv[3]))
