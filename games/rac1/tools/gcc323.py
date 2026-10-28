#!/usr/bin/env python3
"""Build/run the published PS2DEV GCC 3.2.3 C frontend, without rewriting output.

  python tools/gcc323.py --setup
  python tools/gcc323.py -O2 -G2 -ffast-math -Iinclude -S input.c -o output.s

Linux builds run natively. Windows/macOS use the existing build container.
Only the C frontend is needed; the repository keeps its original assembler,
linker and libgcc. Sources and compiler binaries stay under ignored paths.
"""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path, PurePosixPath, PureWindowsPath
import shutil
import subprocess
import sys
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
PREFIX = ROOT / "toolchain/ps2dev-gcc323"
WORK = ROOT / "build-sn/gcc323-bootstrap"
SOURCE_URL = "https://ftp.gnu.org/gnu/gcc/gcc-3.2.3/gcc-3.2.3.tar.bz2"
SOURCE_SHA256 = "9229434cfdfb79d3183149ac09e1093be642d4be377cc491e75c23b856243fda"
PATCH_COMMIT = "33030016e8d4d4f95ec62b0e088361c3cbe9aca3"
PATCH_URL = ("https://raw.githubusercontent.com/ps2dev/ps2toolchain/" + PATCH_COMMIT
             + "/patches/gcc-3.2.3-PS2.patch")
PATCH_SHA256 = "0f563efed36319b8ad08c343aa324286c7c4038a239faa926826bde2e0afcfc7"
# Host flags only. Large-file interfaces also handle Windows bind-mount inodes
# when this frontend is built in the project's 32-bit Linux container.
HOST_CFLAGS = "-O2 -std=gnu89 -fno-strict-aliasing -fcommon -D_FILE_OFFSET_BITS=64"


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fetch(url: str, path: Path, expected: str) -> None:
    if not path.exists():
        path.parent.mkdir(parents=True, exist_ok=True)
        temporary = path.with_suffix(path.suffix + ".part")
        print("Downloading " + url, flush=True)
        with urllib.request.urlopen(url, timeout=60) as response, temporary.open("wb") as output:
            shutil.copyfileobj(response, output)
        if digest(temporary) != expected:
            raise ValueError("Checksum mismatch: " + str(temporary))
        temporary.replace(path)
    if digest(path) != expected:
        raise ValueError("Checksum mismatch: " + str(path))


def extract_source(archive: Path, destination: Path) -> None:
    """The pinned GNU archive contains regular files/directories, no links."""
    base = destination.resolve()
    with tarfile.open(archive, "r:bz2") as source:
        for member in source:
            name = PurePosixPath(member.name)
            if (name.is_absolute() or ".." in name.parts or "\\" in member.name
                    or not name.parts or name.parts[0] != "gcc-3.2.3"
                    or not (member.isfile() or member.isdir())):
                raise ValueError("Unexpected archive member: " + member.name)
            target = destination.joinpath(*name.parts)
            if not target.resolve().is_relative_to(base):
                raise ValueError("Archive path escapes destination: " + member.name)
            if member.isdir():
                target.mkdir(parents=True, exist_ok=True)
            else:
                target.parent.mkdir(parents=True, exist_ok=True)
                with source.extractfile(member) as content, target.open("wb") as output:
                    shutil.copyfileobj(content, output)
                target.chmod(member.mode & 0o777)


def run_logged(command: list[str], cwd: Path, log: Path, **kwargs) -> None:
    with log.open("w") as output:
        result = subprocess.run(command, cwd=cwd, stdout=output, stderr=subprocess.STDOUT, **kwargs)
    if result.returncode:
        raise RuntimeError("Compiler setup failed; see " + str(log))


def setup() -> None:
    manifest = PREFIX / "manifest.json"
    if manifest.exists():
        installed = json.loads(manifest.read_text())
        if (installed.get("source_sha256") == SOURCE_SHA256
                and installed.get("patch_sha256") == PATCH_SHA256
                and all((PREFIX / "bin" / n).is_file() for n in ("ee-gcc", "cc1", "cpp0"))):
            if subprocess.check_output(native_command(["-dumpversion"]), text=True).strip() != "3.2.3":
                raise RuntimeError("Installed frontend reports an unexpected version.")
            print("PS2DEV GCC 3.2.3 C frontend is ready.")
            return
        raise RuntimeError("Existing GCC 3.2.3 installation is incomplete or uses different inputs: " + str(PREFIX))
    for tool in ("gcc", "make", "patch"):
        if shutil.which(tool) is None:
            raise RuntimeError("Compiler setup needs " + tool + "; use the repository build container.")
    archive = PREFIX / "downloads/gcc-3.2.3.tar.bz2"
    patch = PREFIX / "downloads/gcc-3.2.3-PS2.patch"
    fetch(SOURCE_URL, archive, SOURCE_SHA256)
    fetch(PATCH_URL, patch, PATCH_SHA256)
    WORK.mkdir(parents=True, exist_ok=True)
    source = WORK / "source/gcc-3.2.3"
    stamp = WORK / "source/patched.ok"
    if not stamp.exists():
        if source.exists():
            raise RuntimeError("Interrupted source extraction/patch; inspect " + str(source))
        extract_source(archive, WORK / "source")
        with patch.open("r") as changes:
            run_logged(["patch", "--batch", "-p1"], source, WORK / "patch.log", stdin=changes)
        stamp.write_text(PATCH_SHA256 + "\n")
    if stamp.read_text().strip() != PATCH_SHA256:
        raise RuntimeError("Source patch stamp differs from the pinned release.")
    build = WORK / "build-ee"
    build.mkdir(exist_ok=True)
    if not (build / "Makefile").exists():
        env = dict(os.environ, CFLAGS=HOST_CFLAGS)
        run_logged([str(source / "configure"), "--prefix=" + str(PREFIX), "--target=ee",
                    "--enable-languages=c", "--with-newlib", "--without-headers",
                    "--disable-shared", "--disable-threads", "--disable-multilib", "--disable-nls"],
                   build, WORK / "configure.log", env=env)
    jobs = str(min(os.cpu_count() or 1, 4))
    print("Building the pinned C frontend (logs in " + str(WORK) + ")", flush=True)
    run_logged(["make", "-j" + jobs, "all-libiberty", "CFLAGS=" + HOST_CFLAGS, "MAKEINFO=true"],
               build, WORK / "libiberty.log")
    run_logged(["make", "-C", "gcc", "-j" + jobs, "cc1", "xgcc", "cpp0", "stmp-int-hdrs",
                "CFLAGS=" + HOST_CFLAGS, "MAKEINFO=true"], build, WORK / "compiler.log")
    binary = PREFIX / "bin"
    binary.mkdir(parents=True, exist_ok=True)
    for original, installed in (("xgcc", "ee-gcc"), ("cc1", "cc1"), ("cpp0", "cpp0")):
        shutil.copy2(build / "gcc" / original, binary / installed)
    shutil.copytree(build / "gcc/include", binary / "include", dirs_exist_ok=True)
    # Exercise the driver and preprocessor, not merely the existence of cc1.
    smoke = WORK / "smoke.c"
    smoke.write_text("int gcc323_smoke(int value) { return value + 1; }\n")
    run_logged([str(binary / "ee-gcc"), "-B" + str(binary) + "/", "-O2", "-S", str(smoke),
                "-o", str(WORK / "smoke.s")], ROOT, WORK / "smoke.log")
    manifest.write_text(json.dumps({"version": "3.2.3", "source_sha256": SOURCE_SHA256,
                                   "patch_sha256": PATCH_SHA256, "patch_commit": PATCH_COMMIT,
                                   "host_cflags": HOST_CFLAGS,
                                   "binaries": {n: digest(binary / n) for n in ("ee-gcc", "cc1", "cpp0")}},
                                  indent=2) + "\n")
    print("PS2DEV GCC 3.2.3 C frontend is ready.")


def container_argument(value: str, root) -> str:
    """Translate only filenames; preserve macro definitions and other flags."""
    if value.startswith("-I") and len(value) > 2:
        return "-I" + container_argument(value[2:], root)
    if value.startswith("-"):
        return value
    path_type = PureWindowsPath if root.drive else PurePosixPath
    path = path_type(value)
    if path.is_absolute():
        try:
            relative = path.relative_to(root)
            if ".." in relative.parts:
                raise ValueError("Parent traversal in compiler filename")
            return "/work/" + relative.as_posix()
        except ValueError as error:
            raise ValueError("Compiler filename is outside the checkout: " + value) from error
    if ".." in path.parts:
        raise ValueError("Parent traversal in compiler filename: " + value)
    return value.replace("\\", "/")


def in_container(arguments: list[str]) -> int:
    cli = os.environ.get("CONTAINER_CLI") or shutil.which("podman") or shutil.which("docker")
    if not cli:
        raise RuntimeError("GCC 3.2.3 on this host requires Docker/Podman; see docs/TOOLCHAIN.md.")
    image = os.environ.get("RAC1_BUILD_IMAGE")
    platform_args = []
    if not image:
        local = "localhost/rac1-build:latest" if "podman" in Path(cli).name else "rac1-build:latest"
        present = subprocess.run([cli, "image", "inspect", local], capture_output=True).returncode == 0
        if present:
            image = local
        else:
            image = "ghcr.io/lynder063/rac1-build:latest"
            platform_args = ["--platform", "linux/386"]
    # An explicitly selected/local image already chooses the host platform.
    # For the registry default, select the same 32-bit variant as run.sh.
    args = [container_argument(a, ROOT) for a in arguments]
    return subprocess.call([cli, "run", "--rm", "--init", *platform_args,
                            "-v", ROOT.as_posix() + ":/work", "-w", "/work", image,
                            "python", "tools/gcc323.py", *args])


def native_command(args: list[str]) -> list[str]:
    binary = PREFIX / "bin"
    if not (PREFIX / "manifest.json").exists() or not (binary / "ee-gcc").is_file():
        raise RuntimeError("Missing pinned C frontend; run: python tools/gcc323.py --setup")
    installed = json.loads((PREFIX / "manifest.json").read_text())
    if installed.get("source_sha256") != SOURCE_SHA256 or installed.get("patch_sha256") != PATCH_SHA256:
        raise RuntimeError("Installed frontend does not use the pinned source inputs.")
    for name in ("ee-gcc", "cc1", "cpp0"):
        if not (binary / name).is_file() or digest(binary / name) != installed.get("binaries", {}).get(name):
            raise RuntimeError("Installed frontend binary changed: " + name)
    return [str(binary / "ee-gcc"), "-B" + str(binary) + "/", *args]


def main() -> int:
    args = sys.argv[1:]
    if os.name == "nt" or sys.platform == "darwin":
        return in_container(args)
    if args == ["--setup"]:
        setup()
        return 0
    return subprocess.call(native_command(args))


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, RuntimeError) as error:
        raise SystemExit(str(error))
