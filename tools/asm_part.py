#!/usr/bin/env python3
"""
The part of a game built from the player's disc (port/tools/hostgen/asmext.py), where the player
is: the functions the decompilation has only in assembly, translated from the disc's own programs
(port/tools/hostgen/asmdev.py) and compiled into a library the game loads before it starts
(port/game/common/program/asm_part.cpp). An Android launcher does it with the Python, Clang and lld
its APK carries; nothing of the disc is in the APK.

  python3 tools/asm_part.py kit GAME --build DIR --out KIT [--sysroot SYSROOT --resource-dir DIR]
  python3 tools/asm_part.py build GAME --disc IMAGE --out DIR [--kit KIT]
  python3 tools/asm_part.py check GAME --kit KIT --out DIR

kit (the APK's build, tools/build_android.py): what building the part needs besides the disc, in
KIT: hostgen's program before its assembly stage (DIR/games/GAME/gen/program.pickle, which a build
without the disc writes), the game's hostgen.json and interface, and every header the translations
include, copied from where the build compiled them, the system's (SYSROOT, Clang's RESOURCE-DIR)
among them.

build (the launcher's, once the disc is set up): DIR/libopenrac-GAME-part.so, where the game looks
for it (<data>/asm-part). Clang and lld are the APK's Zig (OPENRAC_NATIVE_LIBRARY_DIR:
libopenrac-zig.so) or OPENRAC_PART_ZIG, else OPENRAC_PART_CLANG and OPENRAC_PART_LLD, else clang
and ld.lld on PATH; the game's library is linked against so the part finds its functions.

check (the APK's build): a file that includes what the translations do, built with KIT as the
part is.
"""
from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
import threading
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
HOSTGEN = ROOT / "port" / "tools" / "hostgen"
# What every file of translations includes (asmc.file_head and PS2_LEAD).
INCLUDES = ("<math.h>", "<string.h>", '"openrac/ee.h"', '"openrac/ps2float.h"', '"game_protos.h"')
# The game's C flags (cmake/Games.cmake OPENRAC_GAME_C_FLAGS and the release build's), for a part
# that computes as the build's own translations did.
C_FLAGS = ["-O2", "-DNDEBUG", "-std=gnu11", "-fno-strict-aliasing", "-fwrapv", "-fsigned-char",
           "-ffp-contract=off", "-w", "-fPIC", "-DOPENRAC_ENHANCED_RENDERING=1", "-DOPENRAC_HAS_DISC=1",
           "-DOPENRAC_ASM_PART=1"]
ANDROID_TARGET = "aarch64-linux-android29"
KIT_FORMAT = 1
# Zig (Clang and lld in one program) as the APK carries it (tools/build_android.py PART_TOOL).
ZIG_LIBRARY = "libopenrac-zig.so"
# The scale of the count the build prints.
PROGRESS_TOTAL = 1000


def part_name(game: str) -> str:
    return f"libopenrac-{game}-part.so"


def kit(game: str, build: Path, out: Path, sysroot: Path | None, resource_dir: Path | None) -> None:
    gen = build / "games" / game / "gen"
    if not (gen / "program.pickle").is_file():
        sys.exit(f"{gen} has no program.pickle: build {game} without the disc first (hostgen --program-out)")
    if out.exists():
        shutil.rmtree(out)
    (out / "include").mkdir(parents=True)
    shutil.copy2(gen / "program.pickle", out / "program.pickle")
    config = ROOT / "port" / "game" / game
    shutil.copy2(config / "hostgen.json", out / "hostgen.json")
    cfg = json.loads((config / "hostgen.json").read_text())
    shutil.copy2(config / cfg["asm"]["interface"], out / cfg["asm"]["interface"])
    # Every header a translation reads, found by the compiler the build used, each under the
    # include folder it was found in: user headers first, then the system's.
    dirs = [gen, ROOT / "port/game" / game / "host", ROOT / "port/game/common/include", ROOT / "port/runtime/include",
            ROOT / "port"]
    system = [d for d in (resource_dir / "include" if resource_dir else None,
                          sysroot / "usr/include/aarch64-linux-android" if sysroot else None,
                          sysroot / "usr/include" if sysroot else None) if d]
    with tempfile.TemporaryDirectory() as tmp:
        probe = Path(tmp) / "probe.c"
        probe.write_text("".join(f"#include {i}\n" for i in INCLUDES))
        cmd = [os.environ.get("CC", "clang"), "-M", *C_FLAGS, *[f"-I{d}" for d in dirs], str(probe)]
        if sysroot:
            cmd[1:1] = [f"--target={ANDROID_TARGET}", f"--sysroot={sysroot}"]
        if resource_dir:
            cmd[1:1] = [f"-resource-dir={resource_dir}"]
        deps = subprocess.run(cmd, check=True, capture_output=True, text=True).stdout
    files = [Path(p) for p in deps.replace("\\\n", " ").split(":", 1)[1].split() if not p.endswith("probe.c")]
    for path in files:
        path = path.resolve()
        home = next((k for k, d in enumerate(dirs + system) if path.is_relative_to(d.resolve())), None)
        if home is None:
            if not system:
                continue  # a desktop's kit: the system's headers are the player's own
            sys.exit(f"{path} is in none of the include folders")
        rel = path.relative_to((dirs + system)[home].resolve())
        dest = out / "include" / str(home) / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, dest)
    (out / "kit.json").write_text(json.dumps({"format": KIT_FORMAT, "game": game, "includes": [
        [str(k), "system" if k >= len(dirs) else "user"] for k in range(len(dirs) + len(system))
        if (out / "include" / str(k)).is_dir()]}, indent=1) + "\n")
    print(f"{out}: the kit for {game}'s part ({len(files)} headers)")


def tools() -> tuple[list[str], list[str]]:
    """The commands that compile and link: OPENRAC_PART_ZIG's Zig ("zig clang", "zig ld.lld"), else
    the APK's (OPENRAC_NATIVE_LIBRARY_DIR/libopenrac-zig.so), else OPENRAC_PART_CLANG and
    OPENRAC_PART_LLD, else clang and ld.lld on PATH."""
    zig = os.environ.get("OPENRAC_PART_ZIG")
    libs = os.environ.get("OPENRAC_NATIVE_LIBRARY_DIR")
    if not zig and libs and (Path(libs) / ZIG_LIBRARY).is_file():
        zig = str(Path(libs) / ZIG_LIBRARY)
    if zig:
        return [zig, "clang"], [zig, "ld.lld"]
    found = []
    for env, name in (("OPENRAC_PART_CLANG", "clang"), ("OPENRAC_PART_LLD", "ld.lld")):
        path = os.environ.get(env) or shutil.which(name)
        if not path:
            sys.exit(f"no {name} to build the part with ({env})")
        found.append(path)
    return [found[0]], [found[1], "-flavor", "gnu"]


def read_kit(kit_dir: Path, game: str) -> dict:
    meta = json.loads((kit_dir / "kit.json").read_text())
    if meta.get("format") != KIT_FORMAT or meta.get("game") != game:
        sys.exit(f"{kit_dir} is not a kit for {game}")
    return meta


def compile_and_link(game: str, sources: list[Path], part: Path, kit_dir: Path, meta: dict,
                     sysroot: Path | None, jobs: int, game_library: Path | None) -> None:
    """SOURCES compiled with the kit's headers and linked into PART, against GAME_LIBRARY."""
    clang, lld = tools()
    flags = ["--driver-mode=gcc", *C_FLAGS]
    for home, kind in meta["includes"]:
        flags += ["-isystem" if kind == "system" else "-I", str(kit_dir / "include" / home)]
    lib = kit_dir / "lib"
    android = lib.is_dir()
    if android or sysroot:
        flags += [f"--target={ANDROID_TARGET}", "-nostdinc"]
    print(f"compiling {len(sources)} files", flush=True)
    compiled = [0]
    lock = threading.Lock()

    def compile_one(c: Path) -> Path:
        o = c.with_suffix(".o")
        subprocess.run([*clang, *flags, "-c", str(c), "-o", str(o)], check=True)
        with lock:
            compiled[0] += 1
            at = 850 + 140 * compiled[0] // len(sources)
            print(f"[{at}/{PROGRESS_TOTAL}] compiled {c.name}", flush=True)
        return o

    with ThreadPoolExecutor(max(1, jobs)) as ex:
        objects = [str(o) for o in ex.map(compile_one, sources)]
    if game_library is not None:
        # The game's library, so the part finds the game's functions in it when it is loaded.
        objects.append(str(game_library))
    if android:
        objects = [str(lib / "crtbegin_so.o"), *objects, *map(str, sorted(lib.glob("*.a"))),
                   str(lib / "libc.so"), str(lib / "libm.so"), str(lib / "crtend_so.o")]
    tmp = part.with_name(part.name + ".tmp")
    print(f"[990/{PROGRESS_TOTAL}] linking {part.name}", flush=True)
    subprocess.run([*lld, "-shared", "-z", "max-page-size=16384", "-o", str(tmp), f"-soname={part.name}",
                    *objects], check=True)
    tmp.replace(part)


def build(game: str, disc: Path, out: Path, kit_dir: Path, sysroot: Path | None, jobs: int) -> Path:
    meta = read_kit(kit_dir, game)
    work = out / "work"
    shutil.rmtree(work, ignore_errors=True)
    src = work / "src"
    sys.path.insert(0, str(HOSTGEN))
    import asmc  # noqa: E402  (hostgen's, pure Python)
    import asmdev  # noqa: E402
    print("translating the functions in assembly from your disc", flush=True)
    # How far along, as the launcher reads a count ("[done/total]", launcher/src/lib/progress.ts):
    # finding the functions in the disc's programs is most of the time, compiling the rest.
    shown = [-1]

    def progress(step: str, done: int, total: int) -> None:
        start, share = {"find": (0, 800), "translate": (800, 50)}[step]
        at = start + share * done // max(1, total)
        if at > shown[0]:
            shown[0] = at
            print(f"[{at}/{PROGRESS_TOTAL}] {'finding' if step == 'find' else 'translating'} "
                  f"the functions in assembly ({done}/{total})", flush=True)
    asmc.PROGRESS = progress
    code = asmdev.main(["--program", str(kit_dir / "program.pickle"), "--game", str(kit_dir / "hostgen.json"),
                        "--retail-image", str(disc), "--out", str(src)])
    if code:
        sys.exit(code)
    libs = os.environ.get("OPENRAC_NATIVE_LIBRARY_DIR")
    part = out / part_name(game)
    compile_and_link(game, sorted(src.glob("*.c")), part, kit_dir, meta, sysroot, jobs,
                     Path(libs) / f"libopenrac-{game}.so" if libs else None)
    shutil.rmtree(work, ignore_errors=True)
    print(f"built {part}", flush=True)
    return part


def check(game: str, kit_dir: Path, out: Path) -> None:
    """The kit and the tools work together: a file that includes what a translation does,
    compiled and linked as the part is (the APK's build runs it with the host's Zig)."""
    meta = read_kit(kit_dir, game)
    shutil.rmtree(out, ignore_errors=True)
    out.mkdir(parents=True)
    probe = out / "probe.c"
    probe.write_text("".join(f"#include {i}\n" for i in INCLUDES)
                     + "int openrac_asm_register(void) { return (int)sqrtf(4.0f); }\n")
    compile_and_link(game, [probe], out / "probe.so", kit_dir, meta, None, 1, None)
    print(f"{kit_dir}: the kit builds with {' '.join(tools()[0])}", flush=True)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="command", required=True)
    k = sub.add_parser("kit")
    k.add_argument("game")
    k.add_argument("--build", type=Path, required=True, help="the port's build folder")
    k.add_argument("--out", type=Path, required=True)
    k.add_argument("--sysroot", type=Path, help="the NDK's sysroot, for an Android kit")
    k.add_argument("--resource-dir", type=Path, help="Clang's resource folder (its own headers)")
    k.add_argument("--libs", type=Path, nargs="*", default=[],
                   help="what the link needs besides the game (crtbegin_so.o, crtend_so.o, libc.so, libm.so, builtins)")
    b = sub.add_parser("build")
    b.add_argument("game")
    b.add_argument("--disc", type=Path, required=True)
    b.add_argument("--out", type=Path, required=True)
    b.add_argument("--kit", type=Path, help="default: port/asm-part/GAME in this copy")
    b.add_argument("--sysroot", type=Path)
    # A phone's memory holds a few compilers at once (each takes up to about 160 MB).
    b.add_argument("--jobs", type=int, default=min(4, os.cpu_count() or 2))
    c = sub.add_parser("check")
    c.add_argument("game")
    c.add_argument("--kit", type=Path, required=True)
    c.add_argument("--out", type=Path, required=True)
    args = ap.parse_args(argv)
    if args.command == "check":
        check(args.game, args.kit.resolve(), args.out.resolve())
        return 0
    if args.command == "kit":
        kit(args.game, args.build, args.out, args.sysroot, args.resource_dir)
        if args.libs:
            (args.out / "lib").mkdir(exist_ok=True)
            for p in args.libs:
                shutil.copy2(p, args.out / "lib" / p.name)
        return 0
    if not args.disc.is_file():
        sys.exit(f"the disc image {args.disc} is not there")
    args.out.mkdir(parents=True, exist_ok=True)
    build(args.game, args.disc.resolve(), args.out.resolve(),
          (args.kit or ROOT / "port" / "asm-part" / args.game).resolve(), args.sysroot, args.jobs)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except subprocess.CalledProcessError as e:
        # The compiler printed its errors above; this line says which step stopped.
        cmd = e.cmd if isinstance(e.cmd, str) else " ".join(map(str, e.cmd))
        print(f"error: {cmd[:300]} stopped with code {e.returncode}", file=sys.stderr, flush=True)
        sys.exit(e.returncode or 1)
