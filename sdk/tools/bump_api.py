#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 the OpenRAC contributors
"""Change the SDK's version in every place that holds it, in one go.

    python3 sdk/tools/bump_api.py minor --note "what the new members do (their names)"
    python3 sdk/tools/bump_api.py patch
    python3 sdk/tools/bump_api.py --check

`minor` is for new members of openrac_api (or a change of what the table means): the interface's
minor goes up by one and the SDK's version becomes 1.<minor>.0. `patch` is for anything else a mod
sees, such as new hook points: only the SDK's last number goes up. The version lives in:

    sdk/include/openrac/openrac_mod.h          OPENRAC_MOD_API_MINOR, and its history (--note)
    sdk/VERSION                                MAJOR.MINOR.PATCH
    sdk/cmake/OpenRACModConfigVersion.cmake    PACKAGE_VERSION, MAJOR.MINOR
    launcher/core/src/plugins.rs               API_MINOR
    sdk/template/sdk.lock                      version and url (sha256 cleared: the release fills it)
    sdk/template-script/sdk.lock               the same

After a minor it runs gen_api_lock.py and gen_reference.py, whose outputs hold the minor too.
--check writes nothing and fails when the places do not agree. Stdlib only.
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import textwrap
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HEADER = Path("sdk/include/openrac/openrac_mod.h")
VERSION = Path("sdk/VERSION")
CMAKE = Path("sdk/cmake/OpenRACModConfigVersion.cmake")
LAUNCHER = Path("launcher/core/src/plugins.rs")
LOCKS = (Path("sdk/template/sdk.lock"), Path("sdk/template-script/sdk.lock"))
URL = "https://github.com/OpenRAC/OpenRAC/releases/download/sdk-v{v}/openrac-mod-sdk-{v}.zip"
HISTORY_START = "/* The interface's minor version"
HISTORY_WIDTH = 80


def read(root: Path, path: Path) -> str:
    return (root / path).read_text()


def found(pattern: str, text: str, where: Path) -> re.Match:
    m = re.search(pattern, text, re.M)
    if not m:
        raise SystemExit(f"{where}: no {pattern!r}")
    return m


def places(root: Path) -> dict[str, str]:
    """The version each place says."""
    out = {
        str(HEADER): "1." + found(r"^#define OPENRAC_MOD_API_MINOR (\d+)$", read(root, HEADER), HEADER).group(1),
        str(VERSION): read(root, VERSION).strip(),
        str(CMAKE): found(r'^set\(PACKAGE_VERSION "([\d.]+)"\)$', read(root, CMAKE), CMAKE).group(1),
        str(LAUNCHER): "1." + found(r"^pub const API_MINOR: u32 = (\d+);$", read(root, LAUNCHER), LAUNCHER).group(1),
    }
    for lock in LOCKS:
        data = json.loads(read(root, lock))
        out[str(lock)] = data["version"]
        out[f"{lock} url"] = data["url"]
    return out


def problems(root: Path) -> list[str]:
    p = places(root)
    version = p[str(VERSION)]
    if not re.fullmatch(r"1\.\d+\.\d+", version):
        return [f"{VERSION}: {version!r} is not 1.MINOR.PATCH"]
    api = version.rsplit(".", 1)[0]
    out = []
    for where in (HEADER, CMAKE, LAUNCHER):
        if p[str(where)] != api:
            out.append(f"{where} says {p[str(where)]}, {VERSION} says {version}")
    for lock in LOCKS:
        if p[str(lock)] != version:
            out.append(f"{lock} pins {p[str(lock)]}, {VERSION} says {version}")
        if p[f"{lock} url"] != URL.format(v=version):
            out.append(f"{lock}: its url is not the {version} release")
    return out


def add_history(header: str, minor: int, note: str) -> str:
    """Adds `1.<minor> <note>` at the end of the minor's history comment. Only the lines from the
    last "and 1.<minor - 1>" on are wrapped again, so the diff stays small."""
    start = header.index(HISTORY_START)
    end = header.index("*/", start)
    comment = header[start:end]
    m = None
    for m in re.finditer(rf"\band(?:\s|\*)+1\.{minor - 1}\b", comment):
        pass
    if m is None:
        raise SystemExit(f"{HEADER}: no 'and 1.{minor - 1}' in the minor's history")
    cut = comment.rfind("\n", 0, m.start()) + 1
    if cut == 0:
        raise SystemExit(f"{HEADER}: the minor's history is one line")
    tail = " ".join(" ".join(line.strip().lstrip("*").strip() for line in comment[cut:].splitlines()).split())
    if not tail.endswith("."):
        raise SystemExit(f"{HEADER}: the minor's history does not end with a full stop")
    # "... 1.27 Y, and 1.28 X." becomes "... 1.27 Y, 1.28 X, and 1.29 NOTE."
    tail = re.sub(rf"^(.*?)\band 1\.{minor - 1}\b", rf"\g<1>1.{minor - 1}", tail, count=1)
    tail = f"{tail[:-1]}, and 1.{minor} {note.strip().rstrip('.')}."
    lines = textwrap.wrap(tail, HISTORY_WIDTH, initial_indent=" * ", subsequent_indent=" * ",
                          break_long_words=False, break_on_hyphens=False)
    return header[:start] + comment[:cut] + "\n".join(lines) + "\n " + header[end:]


def bump(root: Path, kind: str, note: str | None) -> str:
    if problems(root):
        raise SystemExit("the places do not agree now; run --check and fix that first")
    major, minor, patch = (int(x) for x in read(root, VERSION).strip().split("."))
    if kind == "minor":
        if not note:
            raise SystemExit("a minor needs --note: what the new members do, for the header's history")
        minor, patch = minor + 1, 0
        header = read(root, HEADER)
        header = re.sub(r"^#define OPENRAC_MOD_API_MINOR \d+$", f"#define OPENRAC_MOD_API_MINOR {minor}", header,
                        flags=re.M)
        (root / HEADER).write_text(add_history(header, minor, note))
        cmake = read(root, CMAKE)
        (root / CMAKE).write_text(re.sub(r'^set\(PACKAGE_VERSION "[\d.]+"\)$',
                                         f'set(PACKAGE_VERSION "{major}.{minor}")', cmake, flags=re.M))
        rs = read(root, LAUNCHER)
        (root / LAUNCHER).write_text(re.sub(r"^pub const API_MINOR: u32 = \d+;$",
                                            f"pub const API_MINOR: u32 = {minor};", rs, flags=re.M))
    else:
        patch += 1
    version = f"{major}.{minor}.{patch}"
    (root / VERSION).write_text(version + "\n")
    for lock in LOCKS:
        text = read(root, lock)
        data = json.loads(text)
        data["version"] = version
        data["url"] = URL.format(v=version)
        data["sha256"] = ""
        (root / lock).write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n")
    return version


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("kind", nargs="?", choices=["minor", "patch"])
    ap.add_argument("--note", help="for a minor: what the new members do, with their names")
    ap.add_argument("--check", action="store_true", help="write nothing; fail when the places disagree")
    ap.add_argument("--no-regen", action="store_true", help="do not run gen_api_lock.py and gen_reference.py")
    ap.add_argument("--root", type=Path, default=ROOT, help=argparse.SUPPRESS)
    args = ap.parse_args(argv)
    if args.check or args.kind is None:
        found_problems = problems(args.root)
        for p in found_problems:
            print(f"error: {p}", file=sys.stderr)
        if not found_problems:
            print(f"SDK {read(args.root, VERSION).strip()} in every place")
        return 1 if found_problems else 0
    version = bump(args.root, args.kind, args.note)
    print(f"SDK {version}")
    if args.kind == "minor" and not args.no_regen:
        for tool in ("gen_api_lock.py", "gen_reference.py"):
            subprocess.run([sys.executable, str(args.root / "sdk" / "tools" / tool)], check=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
