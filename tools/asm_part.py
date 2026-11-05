#!/usr/bin/env python3
"""
The part of a game built from the player's disc (port/tools/hostgen/asmext.py), where the player
is: the functions the decompilation has only in assembly, translated from the disc's own programs
(port/tools/hostgen/asmdev.py) and compiled into a library the game loads before it starts
(port/game/common/program/asm_part.cpp). An Android or a Windows launcher does it with the Python,
Clang and lld it carries; nothing of the disc is in the launcher.

  python3 tools/asm_part.py kit GAME --build DIR --out KIT [--sysroot SYSROOT --resource-dir DIR]
                                                           [--windows-zig ZIG]
  python3 tools/asm_part.py build GAME --disc IMAGE --out DIR [--kit KIT]
  python3 tools/asm_part.py check GAME --kit KIT --out DIR
  python3 tools/asm_part.py exports GAME --disc IMAGE --kit KIT --out DIR

kit (the launcher's build: tools/build_android.py, or CI's on Windows): what building the part needs besides the disc, in
KIT: hostgen's program before its assembly stage (DIR/games/GAME/gen/program.pickle, which a build
without the disc writes), the game's hostgen.json and interface, and every header the translations
include, copied from where the build compiled them, the system's (SYSROOT, Clang's RESOURCE-DIR)
among them. --windows-zig names an unpacked Zig for Windows: the system's headers are the free ones
it carries (mingw-w64's and Clang's), so nothing of Microsoft's is in the kit.

build (the launcher's, once the disc is set up): DIR/libopenrac-GAME-part.so (.dll on Windows),
where the game looks for it (<data>/asm-part). Clang and lld are the APK's Zig
(OPENRAC_NATIVE_LIBRARY_DIR: libopenrac-zig.so), the installer's (tools/part-zig/zig.exe) or
OPENRAC_PART_ZIG, else OPENRAC_PART_CLANG and OPENRAC_PART_LLD, else clang and ld.lld on PATH; the
game's library is linked against so the part finds its functions. A Windows program is not linked
against: the kit's asm_part_exports.def says what it exports, and the import library is made here.

check (the launcher's build): a file that includes what the translations do, built with KIT as the
part is.

exports (a maintainer's, with the disc, when a sync changes what the part calls, as for
asm_interface.json): port/game/GAME/asm_part_exports.def, from the part compiled for Windows. DIR
holds the work. Names only: nothing of the disc is in it.
"""
from __future__ import annotations

import argparse
import json
import os
import shutil
import struct
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
WINDOWS_TARGET = "x86_64-windows-gnu"
KIT_FORMAT = 1
# Zig (Clang and lld in one program) as the APK carries it (tools/build_android.py PART_TOOL).
ZIG_LIBRARY = "libopenrac-zig.so"
# And as a Windows installer carries it (tools/bundle_standalone.py).
ZIG_PROGRAM = ROOT / "tools" / "part-zig" / "zig.exe"
# What the Windows program exports for the part (tools/asm_part.py exports), beside its hostgen.json.
EXPORTS = "asm_part_exports.def"
# The scale of the count the build prints.
PROGRESS_TOTAL = 1000


def part_name(game: str, windows: bool = False) -> str:
    return f"libopenrac-{game}-part.{'dll' if windows else 'so'}"


def kit(game: str, build: Path, out: Path, sysroot: Path | None, resource_dir: Path | None,
        windows_zig: Path | None = None) -> None:
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
    if windows_zig and (config / EXPORTS).is_file():
        shutil.copy2(config / EXPORTS, out / EXPORTS)
    # Every header a translation reads, found by the compiler the build used, each under the
    # include folder it was found in: user headers first, then the system's.
    dirs = [gen, ROOT / "port/game" / game / "host", ROOT / "port/game/common/include", ROOT / "port/runtime/include",
            ROOT / "port"]
    system = [d for d in (resource_dir / "include" if resource_dir else None,
                          sysroot / "usr/include/aarch64-linux-android" if sysroot else None,
                          sysroot / "usr/include" if sysroot else None) if d]
    if windows_zig:
        system = [windows_zig / "lib/include", windows_zig / "lib/libc/include/any-windows-any"]
    with tempfile.TemporaryDirectory() as tmp:
        probe = Path(tmp) / "probe.c"
        probe.write_text("".join(f"#include {i}\n" for i in INCLUDES))
        cmd = [os.environ.get("CC", "clang"), "-M", *C_FLAGS, *[f"-I{d}" for d in dirs], str(probe)]
        if windows_zig:
            cmd[:1] = [str(windows_zig / "zig.exe"), "clang", f"--target={WINDOWS_TARGET}", "-nostdinc",
                       *[f"-isystem{d}" for d in system]]
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
    (out / "kit.json").write_text(json.dumps({"format": KIT_FORMAT, "game": game,
                                              "target": "windows" if windows_zig else "android", "includes": [
        [str(k), "system" if k >= len(dirs) else "user"] for k in range(len(dirs) + len(system))
        if (out / "include" / str(k)).is_dir()]}, indent=1) + "\n")
    print(f"{out}: the kit for {game}'s part ({len(files)} headers)")


def tools() -> tuple[list[str], list[str]]:
    """The commands that compile and link: OPENRAC_PART_ZIG's Zig ("zig clang", "zig ld.lld"), else
    the APK's (OPENRAC_NATIVE_LIBRARY_DIR/libopenrac-zig.so), the installer's (ZIG_PROGRAM), else
    OPENRAC_PART_CLANG and OPENRAC_PART_LLD, else clang and ld.lld on PATH."""
    zig = os.environ.get("OPENRAC_PART_ZIG")
    libs = os.environ.get("OPENRAC_NATIVE_LIBRARY_DIR")
    if not zig and libs and (Path(libs) / ZIG_LIBRARY).is_file():
        zig = str(Path(libs) / ZIG_LIBRARY)
    if not zig and ZIG_PROGRAM.is_file():
        zig = str(ZIG_PROGRAM)
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


def is_windows(meta: dict) -> bool:
    return meta.get("target") == "windows"


def compile_all(sources: list[Path], kit_dir: Path, meta: dict, sysroot: Path | None, jobs: int) -> list[str]:
    """SOURCES compiled with the kit's headers; the objects."""
    clang = tools()[0]
    flags = ["--driver-mode=gcc", *C_FLAGS]
    for home, kind in meta["includes"]:
        flags += ["-isystem" if kind == "system" else "-I", str(kit_dir / "include" / home)]
    lib = kit_dir / "lib"
    android = lib.is_dir()
    if android or sysroot:
        flags += [f"--target={ANDROID_TARGET}", "-nostdinc"]
    if is_windows(meta):
        # Every Windows library is position independent; the flag is for the others.
        flags = [f for f in flags if f != "-fPIC"] + [f"--target={WINDOWS_TARGET}", "-nostdinc"]
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
        return [str(o) for o in ex.map(compile_one, sources)]


def link_windows(game: str, objects: list[str], tmp: Path, kit_dir: Path) -> None:
    """OBJECTS linked into the DLL TMP, with Zig's lld. What they call of the program comes through
    an import library made from the kit's list; nothing else comes in, not even a C library: the
    translations call none, and a name that would need one stops the link."""
    zig = tools()[0][0]
    imports = tmp.with_name(f"openrac-{game}.lib")
    subprocess.run([zig, "dlltool", "-m", "i386:x86-64", "-D", f"openrac-{game}.exe",
                    "-d", str(kit_dir / EXPORTS), "-l", str(imports)], check=True)
    subprocess.run([zig, "lld-link", "/dll", "/noentry", "/nodefaultlib", "/machine:x64", "/noimplib",
                    f"/out:{tmp}", *objects, str(imports)], check=True)
    imports.unlink(missing_ok=True)


def compile_and_link(game: str, sources: list[Path], part: Path, kit_dir: Path, meta: dict,
                     sysroot: Path | None, jobs: int, game_library: Path | None) -> None:
    """SOURCES compiled with the kit's headers and linked into PART, against GAME_LIBRARY."""
    objects = compile_all(sources, kit_dir, meta, sysroot, jobs)
    lib = kit_dir / "lib"
    android = lib.is_dir()
    if game_library is not None:
        # The game's library, so the part finds the game's functions in it when it is loaded.
        objects.append(str(game_library))
    if android:
        objects = [str(lib / "crtbegin_so.o"), *objects, *map(str, sorted(lib.glob("*.a"))),
                   str(lib / "libc.so"), str(lib / "libm.so"), str(lib / "crtend_so.o")]
    tmp = part.with_name(part.name + ".tmp")
    print(f"[990/{PROGRESS_TOTAL}] linking {part.name}", flush=True)
    if is_windows(meta):
        link_windows(game, objects, tmp, kit_dir)
    else:
        subprocess.run([*tools()[1], "-shared", "-z", "max-page-size=16384", "-o", str(tmp),
                        f"-soname={part.name}", *objects], check=True)
    tmp.replace(part)


def translate(disc: Path, out: Path, kit_dir: Path) -> Path:
    """The functions in assembly, translated from DISC into OUT/work/src; that folder."""
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
    return src


def build(game: str, disc: Path, out: Path, kit_dir: Path, sysroot: Path | None, jobs: int) -> Path:
    meta = read_kit(kit_dir, game)
    src = translate(disc, out, kit_dir)
    work = src.parent
    libs = os.environ.get("OPENRAC_NATIVE_LIBRARY_DIR")
    part = out / part_name(game, is_windows(meta))
    compile_and_link(game, sorted(src.glob("*.c")), part, kit_dir, meta, sysroot, jobs,
                     Path(libs) / f"libopenrac-{game}.so" if libs else None)
    shutil.rmtree(work, ignore_errors=True)
    print(f"built {part}", flush=True)
    return part


def external_symbols(objects: list[str]) -> list[str]:
    """What COFF OBJECTS use and do not define, sorted; data as `__imp_name`, as they read it."""
    used, defined = set(), set()
    for path in objects:
        data = Path(path).read_bytes()
        table, count = struct.unpack_from("<II", data, 8)
        strings = table + count * 18
        k = 0
        while k < count:
            entry = data[table + k * 18:table + (k + 1) * 18]
            value, section, _, storage, aux = struct.unpack("<IhHBB", entry[8:])
            if entry[:4] == bytes(4):
                at = strings + struct.unpack("<I", entry[4:8])[0]
                name = data[at:data.index(0, at)].decode()
            else:
                name = entry[:8].rstrip(b"\0").decode()
            if storage == 2:  # IMAGE_SYM_CLASS_EXTERNAL
                if section > 0:
                    defined.add(name)
                elif section == 0 and value == 0:
                    used.add(name)
            k += 1 + aux
    return sorted(used - defined)


def exports_text(symbols: list[str]) -> str:
    """The module definition that exports SYMBOLS: functions by name, data (`__imp_name`) as DATA."""
    rows = sorted((s.removeprefix("__imp_"), s.startswith("__imp_")) for s in symbols)
    return "EXPORTS\n" + "".join(f"  {name}{' DATA' if data else ''}\n" for name, data in rows)


def exports(game: str, disc: Path, out: Path, kit_dir: Path, jobs: int) -> Path:
    """Writes what GAME's Windows program has to export for its part (EXPORTS)."""
    meta = read_kit(kit_dir, game)
    if not is_windows(meta):
        sys.exit(f"{kit_dir} is not a Windows kit (--windows-zig): only a Windows program names its exports")
    src = translate(disc, out, kit_dir)
    objects = compile_all(sorted(src.glob("*.c")), kit_dir, meta, None, jobs)
    path = ROOT / "port" / "game" / game / EXPORTS
    path.write_text(exports_text(external_symbols(objects)), encoding="utf-8", newline="\n")
    shutil.rmtree(src.parent, ignore_errors=True)
    print(f"{path}: written", flush=True)
    return path


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
    k.add_argument("--windows-zig", type=Path, help="an unpacked Zig for Windows, for a Windows kit")
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
    e = sub.add_parser("exports")
    e.add_argument("game")
    e.add_argument("--disc", type=Path, required=True)
    e.add_argument("--kit", type=Path, required=True)
    e.add_argument("--out", type=Path, required=True)
    e.add_argument("--jobs", type=int, default=min(4, os.cpu_count() or 2))
    args = ap.parse_args(argv)
    if args.command == "check":
        check(args.game, args.kit.resolve(), args.out.resolve())
        return 0
    if args.command == "kit":
        kit(args.game, args.build, args.out, args.sysroot, args.resource_dir, args.windows_zig)
        if args.libs:
            (args.out / "lib").mkdir(exist_ok=True)
            for p in args.libs:
                shutil.copy2(p, args.out / "lib" / p.name)
        return 0
    if not args.disc.is_file():
        sys.exit(f"the disc image {args.disc} is not there")
    args.out.mkdir(parents=True, exist_ok=True)
    if args.command == "exports":
        exports(args.game, args.disc.resolve(), args.out.resolve(), args.kit.resolve(), args.jobs)
        return 0
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
