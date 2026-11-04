#!/usr/bin/env python3
"""Builds a game for a player, from an installed launcher's copy of OpenRAC.

  python tools/build_game.py rac2-ntsc --disc IMAGE --out DIR

A game that still needs code from the player's disc cannot be shipped built, so the installer
carries no program for it. This fetches the source of the commit the installer was built from,
builds it with tools/build_port.py from the player's own disc, and copies the program to DIR. The
copy is read-only, so all of this happens under DIR. The compilers are the player's:
tools/build_port.py says what to install when one is missing.
"""
from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
STAMP = ROOT / "port" / "build" / "release" / "openrac-build.json"
ARCHIVE = "https://github.com/OpenRAC/OpenRAC/archive/{commit}.tar.gz"
PRESET = "standalone"
# Under DIR, and short: Windows limits a path's length, and the build's own are long.
SOURCE = "src"
DONE = ".openrac-source"


def carried_commit(stamp: Path = STAMP) -> str:
    """The commit this copy was built from, which the source has to match."""
    try:
        commit = json.loads(stamp.read_text(encoding="utf-8")).get("commit")
    except (OSError, ValueError, AttributeError):
        commit = None
    if not isinstance(commit, str) or not commit:
        sys.exit(f"this copy of OpenRAC does not say which commit it is ({stamp})")
    return commit


def fetch_source(url: str, commit: str, out: Path) -> Path:
    """The source of COMMIT under OUT, downloaded unless the last run left it whole."""
    source = out / SOURCE
    done = source / DONE
    if done.is_file() and done.read_text(encoding="utf-8").strip() == commit:
        print(f"source of {commit[:8]}: already here", flush=True)
        return source
    if source.exists():
        shutil.rmtree(source)
    source.mkdir(parents=True)
    archive = "source.tar.gz"
    print(f"downloading the source of {commit[:8]}", flush=True)
    subprocess.run(["curl", "-sSfL", "-o", archive, url], cwd=source, check=True)
    # From inside the folder: on Windows tar reads "D:\..." as a host named D.
    subprocess.run(["tar", "-xzf", archive, "--strip-components=1"], cwd=source, check=True)
    (source / archive).unlink()
    done.write_text(commit + "\n", encoding="utf-8")
    return source


def copy_program(build: Path, game: str, out: Path) -> Path:
    """The built program into OUT: openrac-GAME, openrac-GAME.exe, or on macOS its app bundle."""
    name = f"openrac-{game}"
    apps = build / "apps" / name
    if any(apps.glob("*.app")):
        shutil.copytree(apps, out / "apps" / name, symlinks=True, dirs_exist_ok=True)
        return out / "apps" / name
    for path in (build / f"{name}.exe", build / name):
        if path.is_file():
            shutil.copy2(path, out / path.name)
            return out / path.name
    sys.exit(f"the build made no {name} in {build}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Builds a game from your disc, from an installed launcher.")
    parser.add_argument("game", help="the port game, such as rac2-ntsc")
    parser.add_argument("--disc", type=Path, required=True, help="the image of your disc")
    parser.add_argument("--out", type=Path, required=True, help="where the source, the build and the program go")
    parser.add_argument("--source-url", help="where the source archive is, if not on GitHub")
    parser.add_argument("--preset", default=PRESET, help=f"the CMake preset; default {PRESET}")
    args = parser.parse_args(argv)
    if not args.disc.is_file():
        sys.exit(f"the disc image {args.disc} is not there")
    args.out.mkdir(parents=True, exist_ok=True)
    out = args.out.resolve()

    commit = carried_commit()
    source = fetch_source(args.source_url or ARCHIVE.format(commit=commit), commit, out)
    subprocess.run([sys.executable, "tools/build_port.py", args.game, "--disc", str(args.disc.resolve()),
                    "--preset", args.preset], cwd=source, check=True)
    program = copy_program(source / "port" / "build" / args.preset, args.game, out)
    print(f"built {program}", flush=True)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except FileNotFoundError as e:
        sys.exit(f"{e.filename} was not found: it is needed to fetch the source")
    except subprocess.CalledProcessError as e:
        sys.exit(e.returncode)
