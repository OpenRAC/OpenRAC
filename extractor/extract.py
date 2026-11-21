#!/usr/bin/env python3
"""Extract Ratchet & Clank levels from your own disc: PAL (SCES_509.16 v2.00) or NTSC-U
(SCUS_971.99 v1.00), told apart by the executable on it (disc.VERSIONS). Going Commando
(SCUS_972.68), Up Your Arsenal (SCUS_973.53, and its PAL disc SCES_524.56) and Deadlocked
(SCUS_974.65, and its PAL disc SCES_532.85, Ratchet: Gladiator) are read too, in early access (wads.VERSIONS).

  extract.py survey ISO                       disc layout and references, as JSON
  extract.py raw ISO OUT --level N            one level's sections, as stored and decoded
  extract.py port ISO OUT [--level N ...]     levels for the native port's viewer (port/viewer)

OUT must be a new directory under this repository's assets/ or build/,
both ignored by git. Output is built beside it and moved into place only
when complete. Never commit or share it: see LEGAL.md.

The level editor's projects and packs are made by OpenRAC Editor's tools, which read the disc
with this folder's modules (extractor_api.json).
"""

import argparse
from concurrent.futures import Executor, ThreadPoolExecutor, as_completed
import json
import os
from pathlib import Path
import pickle
import shutil
import struct
import subprocess
import sys
import tempfile

if sys.platform != "ios":
    from concurrent.futures import ProcessPoolExecutor

sys.path.insert(1, str(next(p for p in Path(__file__).resolve().parents if (p / "tools/openrac_common").is_dir()) / "tools"))
from openrac_common import background, files as file_ops  # noqa: E402

from disc import LEVEL_COUNT, Disc
from formats import FormatError
import errors
import progress_line
from spaceships import add_spaceships
from level import load_flight, load_level, load_title, load_wad_flight
from level import load_wad_level
from occlusion import write_stamp as write_pvs_stamp
from lighting import normal_table
from port import PortLevelWriter, levels_format
from util import sha256
import wads

ROOT = Path(__file__).resolve().parents[1]


def output_path(path: Path, anywhere: bool = False, replace: bool = False) -> Path:
    """A new directory inside assets/ or build/, after resolving symlinks. ANYWHERE: any new
    directory (an installed launcher has no checkout to write in: it names its own data folder).
    REPLACE: it may exist."""
    resolved = path.resolve()
    if not anywhere and not any(root in resolved.parents for root in (ROOT / "assets", ROOT / "build")):
        raise FormatError("output must be a new directory under this repository's assets/ or build/",
                          code="folder-not-allowed")
    if resolved.exists() and not replace:
        raise FormatError(f"{resolved} already exists; choose a new directory", code="folder-exists")
    return resolved


def export_kind(path: Path) -> str | None:
    """What a directory holds: None when it is missing or empty, "port" for levels extracted for
    the port (a manifest.json per level, or one beside), "godot" for the editor's Godot projects
    (project.godot, or one per level_NN), "other" for anything else."""
    if not path.is_dir() or not any(path.iterdir()):
        return None
    if (path / "manifest.json").is_file() or any(path.glob("level_*/manifest.json")):
        return "port"
    if (path / "project.godot").is_file() or any(path.glob("level_*/project.godot")):
        return "godot"
    return "other"


def ours(path: Path, kind: str) -> None:
    """Refuses to write over a directory that holds something other than an export of KIND."""
    found = export_kind(path)
    if found not in (None, kind):
        what = {"port": "levels extracted for the game", "godot": "Godot projects of the level editor",
                "other": "files that are not an export"}[found]
        raise FormatError(f"{path} holds {what}, not what this command writes; choose another folder",
                          code="folder-not-ours")


def say_output(path: Path) -> None:
    """The line on which the launcher reads the folder a command wrote (launcher/core/src/jobs.rs)."""
    print(f"openrac-output: {path}", flush=True)


def adopt_flat_export(out: Path, version: str, kind: str = "port") -> bool:
    """What was written before each version had a folder of its own sat straight in OUT's parent:
    the port's levels (assets/port/rac1, <data>/port), the Godot projects (assets/levels/rac1) and
    the level packer's output (assets/packed/rac1). When OUT does not exist yet and the parent holds
    that of VERSION, its level folders and the files beside them move into OUT, so the new layout
    starts from them (a Godot project keeps its edits) instead of leaving them beside it. Only PAL
    had the flat layout; the port's levels also say their version in their manifests. KIND is
    "port", "godot" or "packed". True when something moved."""
    parent = out.parent
    if out.exists() or not parent.is_dir():
        return False
    marker = {"port": "manifest.json", "godot": "project.godot", "packed": "gameplay_pal.wad"}[kind]
    levels = [d for d in sorted(parent.glob("level_*")) if d.is_dir() and (d / marker).is_file()]
    if not levels:
        return False
    if kind == "port":
        try:
            found = json.loads((levels[-1] / marker).read_text(encoding="utf-8")).get("version", "pal")
        except (OSError, ValueError):
            return False
    else:
        found = "pal"
    if found != version:
        return False
    out.mkdir(parents=True)
    files = ("verified", "patches.tsv", "packed.json", "changes.txt")
    for item in sorted(parent.iterdir()):
        if (item.is_dir() and item.name.startswith("level_")) or (item.is_file() and item.name.startswith(files)):
            file_ops.rename(item, out / item.name)
    return True


def publish(path: Path, build, anywhere: bool = False, replace: bool = False) -> Path:
    """Run build(directory) in a temporary sibling, then rename it to path. REPLACE: a directory
    already there gives way to the new one once that is whole (never before: a failed run leaves
    the old one as it was)."""
    dest = output_path(path, anywhere, replace)
    dest.parent.mkdir(parents=True, exist_ok=True)
    temporary = Path(tempfile.mkdtemp(prefix=f".{dest.name}-", dir=dest.parent))
    old = None
    try:
        build(temporary)
        if dest.exists():
            old = Path(tempfile.mkdtemp(prefix=f".{dest.name}-old-", dir=dest.parent))
            backup = old / dest.name
            file_ops.rename(dest, backup)
            try:
                file_ops.rename(temporary, dest)
            except BaseException:
                # The completed build is not published. Put the previous directory back;
                # if the filesystem also refuses that rename, leave the backup recoverable.
                try:
                    if not dest.exists() and backup.exists():
                        file_ops.rename(backup, dest)
                except OSError:
                    pass
                raise
            shutil.rmtree(old)
        else:
            file_ops.rename(temporary, dest)
    finally:
        if temporary.exists():
            shutil.rmtree(temporary)
        if old is not None and old.exists() and not (old / dest.name).exists():
            shutil.rmtree(old)
    return dest


def default_jobs() -> int:
    """How many levels are written at once: all but a few of the CPU's threads
    (background.cpu_budget), so the computer stays usable, unless OPENRAC_EXTRACT_JOBS says
    otherwise: a number, or "all" for every thread."""
    wanted = os.environ.get("OPENRAC_EXTRACT_JOBS", "").strip().lower()
    if wanted == "all":
        return os.cpu_count() or 1
    if wanted.isdigit() and int(wanted) > 0:
        return int(wanted)
    return background.cpu_budget()


def worker_pool(jobs: int, script: Path | None = None) -> Executor:
    """JOBS processes for the levels. Where Python has no process pools (Android's CPython has no
    sem_open, so no multiprocessing), each level is written by a Python of its own instead
    (SubprocessPool)."""
    if sys.platform == "ios":
        return ThreadPoolExecutor(1)
    try:
        return ProcessPoolExecutor(jobs)
    except (ImportError, NotImplementedError, OSError):
        return SubprocessPool(jobs, script)


# What SubprocessPool may run: the level exports below. Every function given to worker_pool's
# submit must be here (test_extract checks it), or Android fails where a desktop works.
WORKER_FUNCTIONS = ("export_port_level", "export_port_title", "export_port_flight", "export_port_wad_level",
                    "export_port_wad_flight")
# A phone's memory, not its cores, limits how many levels are written at once.
SUBPROCESS_JOBS = 4


class SubprocessPool(Executor):
    """A process pool without multiprocessing: each call runs `SCRIPT --worker` (this file by
    default, or the level editor's tools, which call worker() with their own exports) in the same
    Python, its function's name and arguments pickled in and its result pickled out, up to
    SUBPROCESS_JOBS at a time."""

    def __init__(self, jobs: int, script: Path | None = None):
        self._threads = ThreadPoolExecutor(max(1, min(jobs, SUBPROCESS_JOBS)))
        self.script = script or Path(__file__).resolve()

    def submit(self, fn, /, *args, **kwargs):
        return self._threads.submit(run_worker, fn.__name__, args, kwargs, self.script)

    def shutdown(self, wait: bool = True, *, cancel_futures: bool = False) -> None:
        self._threads.shutdown(wait, cancel_futures=cancel_futures)


def run_worker(name: str, args: tuple, kwargs: dict, script: Path | None = None):
    done = subprocess.run([sys.executable, str(script or Path(__file__).resolve()), "--worker"],
                          input=pickle.dumps((name, args, kwargs)), capture_output=True, check=False)
    sys.stderr.write(done.stderr.decode(errors="replace"))
    if done.returncode:
        raise RuntimeError(f"{name} failed in its own process (exit status {done.returncode})")
    return pickle.loads(done.stdout)


def worker(functions: dict | None = None) -> None:
    """One call for SubprocessPool: (name, args, kwargs) on stdin, the result on stdout. FUNCTIONS:
    the exports it may run, by name (this file's WORKER_FUNCTIONS by default)."""
    functions = functions if functions is not None else {name: globals()[name] for name in WORKER_FUNCTIONS}
    name, args, kwargs = pickle.load(sys.stdin.buffer)
    if name not in functions:
        sys.exit(f"{name} is not one of the level exports")
    stdout = sys.stdout
    sys.stdout = sys.stderr  # what the export prints must not mix with its result
    try:
        value = functions[name](*args, **kwargs)
    finally:
        sys.stdout = stdout
    result = stdout.buffer
    result.write(pickle.dumps(value))
    result.flush()


def open_later(iso: Path) -> tuple[Disc, wads.WadDisc]:
    disc = Disc(iso)
    return disc, wads.WadDisc(disc, wads.identify(disc))


def export_port_wad_level(iso: Path, number: int, out: Path, lod: int) -> dict:
    """One later game's level in the port's layout (for the overrides packs and the viewer)."""
    disc, source = open_later(iso)
    with disc:
        return PortLevelWriter(out, load_wad_level(source, source.level(number)), source.normals()).write(lod)


def export_port_wad_flight(iso: Path, number: int, out: Path, lod: int) -> dict:
    """A Going Commando flight to a planet as a level of its own (level.load_wad_flight)."""
    disc, source = open_later(iso)
    with disc:
        return PortLevelWriter(out, load_wad_flight(source, number), source.normals()).write(lod)


def normals(disc: Disc) -> list[tuple[int, int]]:
    """The executable's (cos, sin) table, where the disc's version keeps it (lighting.py)."""
    return normal_table(disc.executable(), disc.version.normal_table)


def export_port_level(iso: Path, info: dict, out: Path, lod: int) -> dict:
    """One level for the port's viewer; runs in its own process."""
    with Disc(iso) as disc:
        level = load_level(disc, info)
        level.extra_moby_classes = add_spaceships(disc, disc.survey(), level)
        return PortLevelWriter(out, level, normals(disc)).write(lod)


def export_port_title(iso: Path, survey: dict, out: Path, lod: int) -> dict:
    """The title world as level 99 for the port's viewer (level.load_title)."""
    with Disc(iso) as disc:
        return PortLevelWriter(out, load_title(disc, survey), normals(disc)).write(lod)


def export_port_flight(iso: Path, survey: dict, out: Path, lod: int) -> dict:
    """The flight between planets' sky as level 98 for the port's viewer (level.load_flight)."""
    with Disc(iso) as disc:
        return PortLevelWriter(out, load_flight(disc, survey), normals(disc)).write(lod)


def levels_stamp_name(game: str = "rac1") -> str:
    return f"levels-{levels_format(game)}.json"


def port(iso: Path, survey: dict, levels: list[int], lod: int, jobs: int, out: Path, game: str = "rac1") -> None:
    """OUT/level_NN/ per level (port.py), written in parallel. A later game's levels
    have no title world to add; Going Commando's flights are its own."""
    out.mkdir(parents=True, exist_ok=True)
    with worker_pool(min(jobs, len(levels))) as pool:
        if game == "rac1":
            running = [pool.submit(export_port_level, iso, survey["levels"][i], out, lod) for i in levels]
            running.append(pool.submit(export_port_title, iso, survey, out, lod))
            running.append(pool.submit(export_port_flight, iso, survey, out, lod))
        else:
            running = [pool.submit(export_port_wad_level, iso, i, out, lod) for i in levels]
            if game == "rac2":
                running += [pool.submit(export_port_wad_flight, iso, i, out, lod)
                            for i in range(wads.RAC2_NTSC.flight_count)]
        progress_line.say("read", 0, len(running))
        try:
            for count, done in enumerate(as_completed(running), 1):
                stats = done.result()
                print(f"level {stats['level']:02}: {stats['mesh_instances']} meshes placed, "
                      f"{stats['triangles']} triangles, {stats['textures']} textures", flush=True)
                progress_line.say("read", count, len(running), stats.get("level"))
        except BaseException:
            pool.shutdown(cancel_futures=True)
            raise
    share_identical_files(out)
    if game == "rac1":
        # port.py wrote each level's visibility: the launcher has nothing to add (occlusion.py).
        write_pvs_stamp(out)
    # Last: every level is whole. The launcher counts the levels as ready by this stamp, named
    # after the export format, so a format bump reads them again before they are checked.
    (out / levels_stamp_name(game)).write_text(json.dumps({"format": levels_format(game)}) + "\n", encoding="utf-8", newline="\n")


# The folders of a port level whose files many levels have alike (a moby class, a texture) and
# nothing writes again once the levels are read: they are written once and linked.
SHARED_FOLDERS = ("mobys", "ties", "shrubs", "textures")


def share_identical_files(out: Path) -> int:
    """Links the files that levels in OUT have alike (files.link_identical): Going Commando's levels
    have 0.4 GB of them, Ratchet & Clank's 0.1 GB. Bytes freed."""
    paths = sorted(path for folder in SHARED_FOLDERS for path in out.glob(f"*/{folder}/**/*"))
    freed = file_ops.link_identical(paths)
    if freed:
        print(f"{freed >> 20} MB of files levels have alike written once", flush=True)
    return freed


def raw_later(source: wads.WadDisc, level_id: int, out: Path) -> None:
    """raw() for a later game's level: its sections as stored and decoded."""
    level = load_wad_level(source, source.level(level_id))
    files = {"level_header.bin": level.header, "core_index.bin": level.index, "core_data.bin": level.core,
             "gameplay.bin": level.gameplay}
    if level.art is not None:
        files["art_instances.bin"] = level.art
    files.update({f"stored/{name}.bin": data for name, data in level.stored.items()})
    manifest = {"schema": 1, "game": source.version.game, "level": level_id, "overlay": level.overlay, "files": []}
    for name, data in files.items():
        (out / name).parent.mkdir(parents=True, exist_ok=True)
        (out / name).write_bytes(data)
        manifest["files"].append({"path": name, "bytes": len(data), "sha256": sha256(data)})
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8", newline="\n")


def later_main(args, parser, disc: Disc, version: wads.Version) -> None:
    """The commands for Going Commando's, Up Your Arsenal's and Deadlocked's discs."""
    source = wads.WadDisc(disc, version)
    survey = source.survey()
    numbers = [level["id"] for level in survey["levels"] if "ranges" in level]
    if args.command == "survey":
        json.dump(survey, sys.stdout, indent=2)
        print()
        return
    wanted = sorted(set(args.level or numbers))
    unknown = [n for n in wanted if n not in numbers]
    if unknown:
        raise FormatError(f"{version.game} has no level {unknown[0]}; its levels are {', '.join(map(str, numbers))}",
                          code="no-such-level")
    if args.command == "port":
        ours(output_path(args.out, args.anywhere, replace=True), "port")
        dest = publish(args.out, lambda out: port(args.iso, survey, wanted, args.terrain_lod, max(1, args.jobs), out,
                                                  version.game), args.anywhere, replace=True)
        # Going Commando's port draws them (its Play, developer tools on); the other later games' do not yet.
        state = "playable" if version.game == "rac2" else "early access, not playable yet"
        print(f"Levels in the port's layout ({state}): {dest}")
        say_output(dest)
    else:
        dest = publish(args.out, lambda out: raw_later(source, args.level[0], out))
        print(f"Level {args.level[0]} sections: {dest}")


def raw(disc: Disc, survey: dict, level_id: int, out: Path) -> None:
    level = load_level(disc, survey["levels"][level_id])
    files = {"level_header.bin": level.header, "core_index.bin": level.index,
             "core_data.bin": level.core, f"{disc.version.gameplay}.bin": level.gameplay}
    files.update({f"stored/{name}.bin": data for name, data in level.stored.items()})
    manifest = {"schema": 1, "level": survey["levels"][level_id], "overlay": level.overlay, "files": []}
    for name, data in files.items():
        (out / name).parent.mkdir(parents=True, exist_ok=True)
        (out / name).write_bytes(data)
        manifest["files"].append({"path": name, "bytes": len(data), "sha256": sha256(data)})
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8", newline="\n")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("survey").add_argument("iso", type=Path)
    native = commands.add_parser("port")
    native.add_argument("iso", type=Path)
    native.add_argument("out", type=Path)
    native.add_argument("--level", type=int, action="append",
                        help="a level to export (repeatable; default: all)")
    native.add_argument("--jobs", type=int, default=default_jobs(),
                        help="levels to export at once (default: all but a few CPU threads, or OPENRAC_EXTRACT_JOBS)")
    native.add_argument("--terrain-lod", type=int, choices=(0, 2), default=0,
                        help="terrain detail: 0 finest (default), 2 coarsest")
    native.add_argument("--replace", action="store_true",
                        help="kept for older launchers: an earlier export in OUT is always replaced once the new "
                             "one is complete")
    native.add_argument("--anywhere", action="store_true",
                        help="OUT may be outside this repository (the installed launcher's data folder)")
    dump = commands.add_parser("raw")
    dump.add_argument("iso", type=Path)
    dump.add_argument("out", type=Path)
    dump.add_argument("--level", type=int, required=True, action="append")
    args = parser.parse_args()
    if args.command == "port":
        # Whatever the player does comes first; the levels' processes have the rest of the CPU.
        background.lower_priority()
    try:
        with Disc(args.iso) as disc:
            later = wads.identify(disc)
            if later is not None:
                later_main(args, parser, disc, later)
                return
            # Ratchet & Clank: its 19 levels, numbered 0 to 18.
            if getattr(args, "level", None) and any(n not in range(LEVEL_COUNT) for n in args.level):
                raise FormatError(f"Ratchet & Clank's levels are 0 to {LEVEL_COUNT - 1}", code="no-such-level")
            survey = disc.survey()
            if args.command == "survey":
                json.dump(survey, sys.stdout, indent=2)
                print()
            elif args.command == "port":
                levels = sorted(set(args.level or range(LEVEL_COUNT)))
                if adopt_flat_export(args.out.resolve(), disc.version.key):
                    print(f"moved the levels extracted before into {args.out.resolve()}")
                # The levels for the port are made from the disc alone: a folder of them is replaced
                # by the new export once that is whole (an earlier export, of any format).
                ours(output_path(args.out, args.anywhere, replace=True), "port")
                dest = publish(args.out, lambda out: port(args.iso, survey, levels, args.terrain_lod, max(1, args.jobs), out),
                               args.anywhere, replace=True)
                print(f"Levels for the port's viewer: {dest}")
                say_output(dest)
            else:
                dest = publish(args.out, lambda out: raw(disc, survey, args.level[0], out))
                print(f"Level {args.level[0]} sections: {dest}")
    except (OSError, FormatError, struct.error) as exc:
        errors.say(exc)
        parser.exit(1, f"extract: {exc}\n")
    except Exception:
        # Not one of the cases above: the launcher says its generic sentence, the log has the trace.
        errors.say(Exception())
        raise


if __name__ == "__main__":
    if sys.argv[1:] == ["--worker"]:
        worker()
    else:
        main()
