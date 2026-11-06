#!/usr/bin/env python3
"""Builds a game for a player, from an installed launcher's copy of OpenRAC.

  python tools/build_game.py rac2-ntsc --disc IMAGE --out DIR

A game that still needs code from the player's disc cannot be shipped built, so the installer
carries no program for it. This fetches the source of the commit the installer was built from,
builds it with tools/build_port.py from the player's own disc, and copies the program to DIR. The
copy is read-only. Long Windows installation paths build in the user's temporary folder;
the finished program still goes into DIR. The compilers are the player's:
tools/build_port.py says what to install when one is missing.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import stat
import subprocess
import sys
import tarfile
import tempfile
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
STAMP = ROOT / "port" / "build" / "release" / "openrac-build.json"
ARCHIVE = "https://github.com/{repository}/archive/{commit}.tar.gz"
# Where the source is when the copy does not say which repository built it.
REPOSITORY = "OpenRAC/OpenRAC"
# owner/name as GitHub allows them, and nothing that climbs out of the URL.
REPOSITORY_NAME = r"[A-Za-z0-9][A-Za-z0-9-]*/[A-Za-z0-9_][A-Za-z0-9._-]*"
PRESET = "standalone"
# Under DIR, and short: Windows limits a path's length, and the build's own are long.
SOURCE = "src"
DONE = ".openrac-source"
PROGRAMS = "program"
# The longest path a build makes under DIR, and the most Windows takes without long paths turned on.
LONGEST_BUILD_PATH = 181
WINDOWS_PATH_LIMIT = 259


def build_directory(out: Path) -> Path:
    """Keep dependency paths below MAX_PATH without changing the player's installation."""
    if sys.platform != "win32" or len(str(out)) + LONGEST_BUILD_PATH <= WINDOWS_PATH_LIMIT:
        return out
    # Stable across retries, separate for each installed program. A failed build can resume.
    key = hashlib.sha256(os.path.normcase(str(out)).encode("utf-8")).hexdigest()[:12]
    directory = Path(tempfile.gettempdir()).resolve() / f"openrac-{key}"
    if len(str(directory)) + LONGEST_BUILD_PATH > WINDOWS_PATH_LIMIT:
        sys.exit("the temporary folder's path is too long to build OpenRAC; set TEMP and TMP to a "
                 "short writable folder and restart the launcher (the installation can stay where it is)")
    directory.mkdir(parents=True, exist_ok=True)
    print(f"building in {directory} to keep Windows paths short", flush=True)
    return directory


def read_stamp(stamp: Path) -> dict:
    try:
        data = json.loads(stamp.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return {}
    return data if isinstance(data, dict) else {}


def carried_commit(stamp: Path = STAMP) -> str:
    """The commit this copy was built from, which the source has to match."""
    commit = read_stamp(stamp).get("commit")
    if not isinstance(commit, str) or not commit:
        sys.exit(f"this copy of OpenRAC does not say which commit it is ({stamp})")
    return commit


def carried_repository(stamp: Path = STAMP) -> str:
    """The GitHub repository this copy was built in (a fork's commits are only there)."""
    repository = read_stamp(stamp).get("repository")
    if isinstance(repository, str) and re.fullmatch(REPOSITORY_NAME, repository):
        return repository
    return REPOSITORY


def without_top_folder(member: tarfile.TarInfo, path: str) -> tarfile.TarInfo | None:
    """A GitHub archive keeps everything in one folder, named after the commit."""
    name = member.name.partition("/")[2]
    return tarfile.data_filter(member.replace(name=name, deep=False), path) if name else None


def use_the_systems_libraries() -> None:
    """The compilers and git are the system's, and need its libraries. A launcher started from Steam
    hands down Steam's, and with them the system's curl stopped on a symbol its libcurl lacked."""
    if sys.platform.startswith("linux"):
        for name in ("LD_LIBRARY_PATH", "LD_PRELOAD"):
            os.environ.pop(name, None)


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
    archive = source / "source.tar.gz"
    print(f"downloading the source of {commit[:8]}", flush=True)
    # Python's own: not every Windows PC has curl and tar.
    with urllib.request.urlopen(url, timeout=120) as response, archive.open("wb") as file:
        shutil.copyfileobj(response, file)
    with tarfile.open(archive) as tar:
        tar.extractall(source, filter=without_top_folder)
    archive.unlink()
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


def remove_tree(path: Path) -> None:
    """Deletes what it can of PATH: the game is built by now, so what stays never stops it. Windows
    refuses to delete a read-only file, and git makes its objects read-only (_deps/*-src/.git)."""
    def writable_then_again(function, name, _error):
        try:
            os.chmod(name, stat.S_IWRITE)
            function(name)
        except OSError as e:
            print(f"could not delete {name}: {e}", flush=True)

    if sys.version_info >= (3, 12):
        shutil.rmtree(path, onexc=writable_then_again)
    else:
        shutil.rmtree(path, onerror=writable_then_again)


def clean_up(source: Path, out: Path) -> None:
    """After a build that worked: its source and build, and the builds of older launchers. Only
    under {data}/program, whose folders are all this script's."""
    remove_tree(source)
    if out.parent.name != PROGRAMS:
        return
    for other in out.parent.iterdir():
        if other.is_dir() and other != out:
            remove_tree(other)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Builds a game from your disc, from an installed launcher.")
    parser.add_argument("game", help="the port game, such as rac2-ntsc")
    parser.add_argument("--disc", type=Path, required=True, help="the image of your disc")
    parser.add_argument("--out", type=Path, required=True, help="where the finished program goes")
    parser.add_argument("--source-url", help="where the source archive is, if not on GitHub")
    parser.add_argument("--preset", default=PRESET, help=f"the CMake preset; default {PRESET}")
    args = parser.parse_args(argv)
    use_the_systems_libraries()
    if not args.disc.is_file():
        sys.exit(f"the disc image {args.disc} is not there")
    args.out.mkdir(parents=True, exist_ok=True)
    out = args.out.resolve()

    commit = carried_commit()
    url = args.source_url or ARCHIVE.format(repository=carried_repository(), commit=commit)
    source = fetch_source(url, commit, build_directory(out))
    subprocess.run([sys.executable, "tools/build_port.py", args.game, "--disc", str(args.disc.resolve()),
                    "--preset", args.preset], cwd=source, check=True)
    program = copy_program(source / "port" / "build" / args.preset, args.game, out)
    clean_up(source, out)
    print(f"built {program}", flush=True)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except subprocess.CalledProcessError as e:
        sys.exit(e.returncode)
    except (OSError, tarfile.TarError) as e:
        sys.exit(f"the build stopped: {e}")
