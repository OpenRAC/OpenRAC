#!/usr/bin/env python3
"""
Builds OpenRAC for Android: the launcher's APK with the games in it
(docs/port/ANDROID.md).

  python3 tools/build_android.py [GAME ...] [--release] [--keep-symbols] [--no-apk]

GAME is one of the port's games (port/game/<id>/hostgen.json): rac1-pal,
rac1-ntsc, rac1-pal1, rac1-jp and Going Commando's three discs (rac2-ntsc, rac2-pal,
rac2-ntsc2) when none is named. A game whose code is still partly in assembly (Going Commando: hostgen.json "asm" "interface") is built without
the disc, calling those functions through slots; the APK carries what the
device needs to build them from the player's disc when it is set up
(tools/asm_part.py): its kit, and Clang and lld (Zig's static AArch64 build,
downloaded at a fixed version and checked against its SHA-256). The steps:

  1. a Python for the launcher's tools: CPython for Android as Chaquopy
     packages it (MIT), from Maven Central at a fixed version, each file
     checked against its SHA-256, into build/android/
  2. the port with the NDK (port's android-arm64 preset): for each game
     libopenrac-GAME.so (the game's activity loads it) and
     libopenrac-GAME-cli.so (its windowless checks), SDL3, libadrenotools'
     hooks and libopenrac-python.so; and Enhanced graphics' probe baker,
     libopenrac-enhanced-baker.so, with Embree
     (the version and SHA-256 tools/build_port.py pins) built by the NDK into build/android/embree
     (--no-probe-baker leaves it out; a baker that does not build is left
     out with a warning, as on the desktops)
  3. those libraries, the NDK's C++ runtime and Python's, in the launcher's
     Android project (src-tauri/gen/android/app/src/main/jniLibs), and SDL's
     Java beside the game's activity (app/src/main/sdl-java)
  4. the copy of OpenRAC the launcher carries (tools/bundle_standalone.py
     --android) as the APK's assets/openrac.zip
  5. the APK: `yarn tauri android build` (a debug build, signed with the
     debug key so that it installs as it is; --release for an unsigned
     release build to sign yourself). Its libraries carry no debug
     information unless --keep-symbols is given.

It needs the Android SDK (ANDROID_HOME) with the NDK (ANDROID_NDK_HOME or
NDK_HOME, r27 or newer), a JDK 17 or newer, Rust with the
aarch64-linux-android target, Node with Corepack, CMake, Ninja, and a host
Clang for hostgen. --no-apk stops after step 4 (the Android Studio project in
launcher/src-tauri/gen/android is then ready to open). Standard library only.
"""

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tarfile
import urllib.request
import zipfile
from pathlib import Path

from openrac_common import digest, paths, proc

# Embree's pinned release and its source, as the desktops build it (tools/build_port.py).
import build_port as desktop

ROOT = paths.repo_root()
PORT = ROOT / "port"
LAUNCHER = ROOT / "launcher"
APP = LAUNCHER / "src-tauri/gen/android/app"
BUILD = ROOT / "build/android"
PORT_BUILD = PORT / "build/android-arm64"
DEFAULT_GAMES = ("rac1-pal", "rac1-ntsc", "rac1-pal1", "rac1-jp", "rac2-ntsc", "rac2-pal", "rac2-ntsc2")
# Clang and lld for the device, to build a game's part from the player's disc (tools/asm_part.py):
# Zig's official static AArch64 Linux build (MIT), one program with Clang and lld of LLVM 19.1.7
# in it ("zig clang", "zig ld.lld"), downloaded instead of built. The host's build of the same
# release checks the part's commands in the APK's build.
ZIG_VERSION = "0.14.1"
ZIG_FILES = {
    "aarch64-linux": "f7a654acc967864f7a050ddacfaa778c7504a0eca8d2b678839c21eea47c992b",
    "x86_64-linux": "24aeeec8af16c381934a6cd7d95c807a8cb2cf7df9fa40d359aa884195c4716c",
}
# The APK's library it is, as the launcher's tools find it (OPENRAC_NATIVE_LIBRARY_DIR).
PART_TOOL = "libopenrac-zig.so"
ANDROID_API = 29

# Chaquopy's CPython for Android (https://github.com/chaquo/chaquopy, MIT): the interpreter as a
# library with its modules, and its standard library, published on Maven Central.
PYTHON_VERSION = "3.13"
PYTHON_PACKAGE = "3.13.16-0"
PYTHON_MAVEN = "https://repo1.maven.org/maven2/com/chaquo/python/target"
PYTHON_FILES = {
    "arm64-v8a": "fae8cfd4d3910b6f978b22279fe820c0c6ba49d0ea20f7b106aa6130f1c8f355",
    "stdlib": "68e3f71e6e2f2a2febb22f2d3575657787b3dd94f07d2c6f3780461babbcbf58",
}
# libadrenotools' hook libraries, which it loads by path from the app's native library folder.
ADRENOTOOLS_HOOKS = ("hook_impl", "main_hook", "file_redirect_hook", "gsl_alloc_hook")
# The unified baker is named as a library so Android can execute it from the APK's native folder.
BAKE_TOOLS = ("openrac-enhanced-baker",)
EMBREE_ANDROID = BUILD / "embree"


def ndk_home() -> Path:
    for name in ("ANDROID_NDK_HOME", "NDK_HOME", "ANDROID_NDK_ROOT", "ANDROID_NDK_LATEST_HOME"):
        value = os.environ.get(name)
        if value and (Path(value) / "build/cmake/android.toolchain.cmake").is_file():
            return Path(value)
    sdk = os.environ.get("ANDROID_HOME") or os.environ.get("ANDROID_SDK_ROOT")
    if sdk:
        found = sorted((Path(sdk) / "ndk").glob("*/build/cmake/android.toolchain.cmake"))
        if found:
            return found[-1].parents[2]
    sys.exit("The Android NDK was not found: set ANDROID_NDK_HOME (or install it with the SDK's sdkmanager).")


def download(name: str, sha256: str, url: str | None = None) -> Path:
    file = url.rsplit("/", 1)[1] if url else f"target-{PYTHON_PACKAGE}-{name}.zip"
    dest = BUILD / "downloads" / file
    if not dest.is_file() or digest.sha256(dest) != sha256:
        dest.parent.mkdir(parents=True, exist_ok=True)
        url = url or f"{PYTHON_MAVEN}/{PYTHON_PACKAGE}/{file}"
        print(f"downloading {url}", flush=True)
        with urllib.request.urlopen(url) as response:
            data = response.read()
        got = hashlib.sha256(data).hexdigest()
        if got != sha256:
            sys.exit(f"{file}: its SHA-256 is {got}, not {sha256}")
        dest.write_bytes(data)
    return dest


def python() -> tuple[Path, Path]:
    """The Python package for the port's build (include/, jniLibs/) and the PYTHONHOME the launcher
    carries (lib/python3.N with its lib-dynload)."""
    target = BUILD / "python-target"
    home = BUILD / "python"
    stamp = BUILD / "python.stamp"
    if stamp.is_file() and stamp.read_text() == PYTHON_PACKAGE and target.is_dir() and home.is_dir():
        return target, home
    for path in (target, home):
        shutil.rmtree(path, ignore_errors=True)
    with zipfile.ZipFile(download("arm64-v8a", PYTHON_FILES["arm64-v8a"])) as z:
        z.extractall(target)
    lib = home / "lib" / f"python{PYTHON_VERSION}"
    with zipfile.ZipFile(download("stdlib", PYTHON_FILES["stdlib"])) as z:
        z.extractall(lib)
    shutil.copytree(target / "lib-dynload/arm64-v8a", lib / "lib-dynload")
    # What a player's tools never need, and the configuration of the other ABI.
    for name in ("test", "idlelib", "tkinter", "turtledemo", "ensurepip", "lib2to3", "pydoc_data",
                 f"config-{PYTHON_VERSION}-x86_64-linux-android",
                 "_sysconfigdata__android_x86_64-linux-android.py"):
        path = lib / name
        shutil.rmtree(path) if path.is_dir() else path.unlink(missing_ok=True)
    stamp.write_text(PYTHON_PACKAGE)
    return target, home


def embree_android_options(ndk: Path) -> dict[str, str]:
    """How Embree is configured for Android: the port's preset's target (arm64-v8a, API 28, the
    shared C++ runtime) through the NDK's toolchain file, and build_port's Embree options for an
    ARM machine (Embree's own NEON)."""
    return {
        "CMAKE_TOOLCHAIN_FILE": (ndk / "build/cmake/android.toolchain.cmake").as_posix(),
        "ANDROID_ABI": "arm64-v8a",
        "ANDROID_PLATFORM": "android-28",
        "ANDROID_STL": "c++_shared",
        "CMAKE_BUILD_TYPE": "Release",
        **desktop.embree_options("aarch64"),
    }


def build_embree(ndk: Path) -> Path:
    """Embree built for Android and installed in EMBREE_ANDROID (once for its version, options
    and NDK); its embree_DIR."""
    install = EMBREE_ANDROID / "install"
    stamp_file = EMBREE_ANDROID / "openrac-embree.json"
    options = embree_android_options(ndk)
    stamp = {"version": desktop.EMBREE_VERSION, "sha256": desktop.EMBREE_SHA256, "options": options}
    try:
        if json.loads(stamp_file.read_text(encoding="utf-8")) == stamp:
            return desktop.embree_config(install)
    except (OSError, ValueError, desktop.BuildError):
        pass
    # The source build_port fetched and checked, shared with the desktop's build.
    source = desktop.fetch_embree(desktop.EMBREE_DIR)
    if os.name == "nt":
        # Embree takes "/" for a full path when its target is not Windows, which fails on a Windows
        # host. The install prefix gives the same relative path.
        package = source / "common/cmake/package.cmake"
        package.write_bytes(package.read_bytes().replace(
            b'"/${EMBREE_CMAKECONFIG_DIR}" "/"',
            b'"${CMAKE_INSTALL_PREFIX}/${EMBREE_CMAKECONFIG_DIR}" "${CMAKE_INSTALL_PREFIX}"'))
    work = EMBREE_ANDROID / "build"
    for folder in (work, install):
        shutil.rmtree(folder, ignore_errors=True)
    env = dict(os.environ, ANDROID_NDK_HOME=str(ndk))
    proc.run(["cmake", "-S", source, "-B", work, "-G", "Ninja", f"-DCMAKE_INSTALL_PREFIX={install.as_posix()}",
              *(f"-D{key}={value}" for key, value in options.items())], cwd=ROOT, env=env, echo=True)
    proc.run(["cmake", "--build", work], cwd=ROOT, env=env, echo=True)
    proc.run(["cmake", "--install", work], cwd=ROOT, env=env, echo=True)
    notices = install / "licenses" / "embree"
    notices.mkdir(parents=True, exist_ok=True)
    for name in ("LICENSE.txt", "third-party-programs.txt"):
        shutil.copy2(source / name, notices / name)
    stamp_file.write_text(json.dumps(stamp, indent=2) + "\n", encoding="utf-8")
    return desktop.embree_config(install)


def baker_defines(ndk: Path) -> list[str]:
    """The port's cache entries for the probe baker, or none (and a warning) when Embree does not
    build for Android."""
    try:
        return ["-DOPENRAC_PROBE_BAKER=ON", f"-Dembree_DIR={build_embree(ndk).as_posix()}"]
    except (OSError, ValueError, subprocess.CalledProcessError, desktop.BuildError) as e:
        print(f"warning: the probe baker is left out: {e}", file=sys.stderr, flush=True)
        return ["-DOPENRAC_PROBE_BAKER=OFF"]


def build_port(games: list[str], ndk: Path, python_target: Path, baker: bool = True) -> bool:
    """The port's libraries for GAMES; whether the probe baker is among them."""
    env = dict(os.environ, ANDROID_NDK_HOME=str(ndk))
    defines = baker_defines(ndk) if baker else ["-DOPENRAC_PROBE_BAKER=OFF"]
    proc.run(["cmake", "--preset", "android-arm64", f"-DOPENRAC_ANDROID_PYTHON={python_target}", *defines],
             cwd=PORT, env=env, echo=True)
    targets = [t for g in games for t in (f"openrac-{g}", f"openrac-{g}-cli")]
    targets += ["openrac-python", "SDL3-shared", *ADRENOTOOLS_HOOKS]
    with_baker = "-DOPENRAC_PROBE_BAKER=ON" in defines
    if with_baker:
        targets.extend(BAKE_TOOLS)
    proc.run(["cmake", "--build", PORT_BUILD, "--target", *targets], cwd=PORT, env=env, echo=True)
    return with_baker


def has_part(game: str) -> bool:
    """Whether GAME's code is still partly in assembly, built on the device from the disc."""
    cfg = json.loads((PORT / "game" / game / "hostgen.json").read_text())
    return bool(cfg.get("asm", {}).get("interface"))


def ndk_toolchain(ndk: Path) -> Path:
    found = sorted(ndk.glob("toolchains/llvm/prebuilt/*/bin/clang"))
    if not found:
        sys.exit(f"{ndk} has no prebuilt Clang")
    return found[0].parent.parent


def zig(platform: str = "aarch64-linux") -> Path:
    """Zig for PLATFORM (ZIG_FILES), unpacked under build/android/zig: its program, the Clang
    headers it carries (lib/include) and the licenses (Zig's; LLVM's, the one libc++ carries)."""
    name = f"zig-{platform}-{ZIG_VERSION}"
    home = BUILD / "zig" / name
    if not (home / "zig").is_file():
        archive = download(name, ZIG_FILES[platform], f"https://ziglang.org/download/{ZIG_VERSION}/{name}.tar.xz")
        shutil.rmtree(home, ignore_errors=True)
        with tarfile.open(archive) as tar:
            keep = {f"{name}/zig", f"{name}/LICENSE", f"{name}/lib/libcxx/LICENSE.TXT"}
            members = [m for m in tar.getmembers() if m.name in keep or m.name.startswith(f"{name}/lib/include/")]
            tar.extractall(home.parent, members=members, filter="data")
    return home


def host_zig() -> Path | None:
    """The host's Zig of the same release, to check the part's commands; None where Zig has no
    build in ZIG_FILES for it."""
    platform = f"{os.uname().machine}-linux" if sys.platform.startswith("linux") else ""
    return zig(platform) / "zig" if platform in ZIG_FILES else None


def part_kit(game: str, ndk: Path, tools: Path) -> Path:
    """What the device needs besides the disc to build GAME's part (tools/asm_part.py kit), checked
    with the host's Zig where there is one."""
    toolchain = ndk_toolchain(ndk)
    sysroot = toolchain / "sysroot"
    libs = sysroot / "usr/lib/aarch64-linux-android" / str(ANDROID_API)
    builtins = sorted(toolchain.glob("lib/clang/*/lib/linux/libclang_rt.builtins-aarch64-android.a"))
    if not builtins:
        sys.exit(f"{toolchain} has no libclang_rt.builtins for aarch64")
    out = BUILD / "asm-part" / game
    env = dict(os.environ, CC=str(toolchain / "bin/clang"))
    proc.run([sys.executable, ROOT / "tools/asm_part.py", "kit", game, "--build", PORT_BUILD, "--out", out,
              "--sysroot", sysroot, "--resource-dir", tools / "lib",
              "--libs", *[libs / n for n in ("crtbegin_so.o", "crtend_so.o", "libc.so", "libm.so")], builtins[-1]],
             cwd=ROOT, env=env, echo=True)
    host = host_zig()
    if host is not None:
        proc.run([sys.executable, ROOT / "tools/asm_part.py", "check", game, "--kit", out, "--out", BUILD / "asm-check"],
                 cwd=ROOT, env=dict(os.environ, OPENRAC_PART_ZIG=str(host)), echo=True)
    return out


def collect_libraries(games: list[str], ndk: Path, python_target: Path, keep_symbols: bool,
                      baker: bool = False, tools: Path | None = None) -> list[str]:
    dest = APP / "src/main/jniLibs/arm64-v8a"
    dest.mkdir(parents=True, exist_ok=True)
    # What an earlier run put here; the launcher's own library is Tauri's to place.
    for old in dest.glob("*.so"):
        if old.name != "libopenrac_launcher_lib.so":
            old.unlink()
    wanted = [PORT_BUILD / f"libopenrac-{g}.so" for g in games]
    wanted += [PORT_BUILD / f"libopenrac-{g}-cli.so" for g in games]
    wanted.append(PORT_BUILD / "libopenrac-python.so")
    if baker:
        wanted.extend(PORT_BUILD / f"lib{tool}.so" for tool in BAKE_TOOLS)
    for name in ("SDL3", *ADRENOTOOLS_HOOKS):
        found = sorted(PORT_BUILD.rglob(f"lib{name}.so"))
        if not found:
            sys.exit(f"the port's build has no lib{name}.so")
        wanted.append(found[0])
    runtime = sorted(ndk.glob("toolchains/llvm/prebuilt/*/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so"))
    if not runtime:
        sys.exit(f"{ndk} has no libc++_shared.so for aarch64")
    wanted.append(runtime[0])
    wanted += sorted((python_target / "jniLibs/arm64-v8a").glob("*.so"))
    # llvm-strip.exe on a Windows host
    strip = sorted(ndk.glob("toolchains/llvm/prebuilt/*/bin/llvm-strip")) + \
        sorted(ndk.glob("toolchains/llvm/prebuilt/*/bin/llvm-strip.exe"))
    names = []
    for path in wanted:
        if not path.is_file():
            sys.exit(f"{path} was not built")
        shutil.copy2(path, dest / path.name, follow_symlinks=True)
        # Gradle strips them again; this way the APK carries no debug info even when it cannot.
        if strip and not keep_symbols:
            proc.run([strip[0], "--strip-debug", dest / path.name], cwd=ROOT, echo=True)
        names.append(path.name)
    # Zig (Clang and lld) under a library's name, so Android lets it run from the APK's folder;
    # its release is stripped already.
    if tools is not None:
        shutil.copy2(tools / "zig", dest / PART_TOOL)
        names.append(PART_TOOL)
    # SDL's activity and its helpers, as the SDL the port was built with has them.
    sdl_java = sorted(PORT_BUILD.rglob("android-project/app/src/main/java/org/libsdl"))
    if not sdl_java:
        sys.exit("the port's build has no SDL Java sources (android-project/app/src/main/java)")
    java = APP / "src/main/sdl-java"
    shutil.rmtree(java, ignore_errors=True)
    shutil.copytree(sdl_java[0], java / "org/libsdl")
    return names


def bundle(python_home: Path, baker: bool = False, games: list[str] | None = None,
           kits: list[Path] | None = None, tools: Path | None = None) -> Path:
    out = BUILD / "openrac"
    proc.run([sys.executable, ROOT / "tools/bundle_standalone.py", out, "--android", "--python", python_home,
              "--android-games", ",".join(games or DEFAULT_GAMES),
              *[a for k in kits or [] for a in ("--asm-part-kit", k)]],
             cwd=ROOT, echo=True)
    # Runtime HDR decoders also need their notices when the baker is omitted.
    if (PORT_BUILD / "licenses").is_dir():
        shutil.copytree(PORT_BUILD / "licenses", out / "port/build/release/licenses", dirs_exist_ok=True)
    if baker:
        # The license of what the baker has built in, beside where the launcher finds it, as a
        # desktop's copy carries it (tools/bundle_standalone.py).
        shutil.copytree(EMBREE_ANDROID / "install/licenses", out / "port/build/release/licenses", dirs_exist_ok=True)
    if tools is not None:
        # Zig's license and LLVM's, whose Clang and lld are in libopenrac-zig.so.
        notices = out / "port/build/release/licenses/zig"
        notices.mkdir(parents=True, exist_ok=True)
        shutil.copy2(tools / "LICENSE", notices / "LICENSE")
        shutil.copy2(tools / "lib/libcxx/LICENSE.TXT", notices / "LLVM-LICENSE.TXT")
    assets = APP / "src/main/assets"
    assets.mkdir(parents=True, exist_ok=True)
    archive = assets / "openrac.zip"
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for path in sorted(out.rglob("*")):
            if path.is_file():
                z.write(path, path.relative_to(out).as_posix())
    print(f"{archive}: {archive.stat().st_size / 1e6:.0f} MB")
    return archive


def build_apk(ndk: Path, release: bool, keep_symbols: bool) -> None:
    env = dict(os.environ, NDK_HOME=str(ndk), ANDROID_NDK_HOME=str(ndk))
    if keep_symbols:
        env["OPENRAC_ANDROID_KEEP_SYMBOLS"] = "1"  # app/build.gradle.kts
    else:
        # The launcher's own library, which a debug build compiles with debug information.
        env.setdefault("CARGO_PROFILE_DEV_DEBUG", "0")
    yarn = shutil.which("yarn") or "yarn"
    proc.run([yarn, "install", "--immutable"], cwd=LAUNCHER, env=env, echo=True)
    cmd = [yarn, "tauri", "android", "build", "--apk", "--target", "aarch64"]
    if not release:
        cmd.append("--debug")
    proc.run(cmd, cwd=LAUNCHER, env=env, echo=True)
    for apk in sorted((APP / "build/outputs/apk").rglob("*.apk")):
        print(f"APK: {apk} ({apk.stat().st_size / 1e6:.0f} MB)")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("games", nargs="*", default=list(DEFAULT_GAMES))
    parser.add_argument("--release", action="store_true", help="an unsigned release APK (default: debug)")
    parser.add_argument("--keep-symbols", action="store_true", help="keep the libraries' debug information")
    parser.add_argument("--no-apk", action="store_true", help="stop before Gradle (steps 1 to 4)")
    parser.add_argument("--no-probe-baker", action="store_true",
                        help="leave Enhanced graphics' probe baker (and Embree) out")
    args = parser.parse_args()
    for game in args.games:
        if not (PORT / "game" / game / "hostgen.json").is_file():
            sys.exit(f"{game} is not one of the port's games (port/game/<id>/hostgen.json)")
    ndk = ndk_home()
    python_target, python_home = python()
    baker = build_port(args.games, ndk, python_target, baker=not args.no_probe_baker)
    parts = [g for g in args.games if has_part(g)]
    tools = zig() if parts else None
    names = collect_libraries(args.games, ndk, python_target, args.keep_symbols, baker, tools)
    print("native libraries:", ", ".join(names))
    kits = [part_kit(g, ndk, tools) for g in parts]
    bundle(python_home, baker, args.games, kits, tools)
    if not args.no_apk:
        build_apk(ndk, args.release, args.keep_symbols)
    return 0


if __name__ == "__main__":
    sys.exit(main())
