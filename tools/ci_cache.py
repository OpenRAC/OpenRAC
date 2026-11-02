# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Identify the installed CI toolchain before restoring CMake preparation: python tools/ci_cache.py.

Only identical workspaces, tools, runner images and system packages may share configure results.
Source/configuration identities and job variants are added by the port-build-cache action.
"""

import functools
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys


def fingerprint(identity):
    """Make a stable key without exposing package inventories or machine paths in the log."""
    return hashlib.sha256(json.dumps(identity, sort_keys=True).encode()).hexdigest()


def output(arguments):
    """Read a required tool's identity; failures must prevent restoring configure results."""
    return subprocess.check_output(arguments, text=True, encoding="utf-8", errors="replace").strip()


@functools.cache
def file_digest(path):
    """Hash an executable once even when clang and clang++ resolve to the same binary."""
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def tool_identity(program):
    """Include absolute paths because CMake stores them, as well as executable contents."""
    found = shutil.which(str(program))
    if not found:
        raise RuntimeError(f"CI preparation tool not found: {program}")
    path = Path(found).resolve()
    return {"path": str(Path(found).absolute()), "resolved": str(path),
            "version": output([str(path), "--version"]),
            "sha256": file_digest(str(path))}


def system_packages():
    """Invalidate Linux configure checks when installed compilers, headers or libraries change."""
    if shutil.which("dpkg-query"):
        text = output(["dpkg-query", "-W", "-f=${binary:Package}=${Version}\n"])
    elif shutil.which("rpm"):
        text = output(["rpm", "-qa", "--qf", "%{NAME}-%{VERSION}-%{RELEASE}.%{ARCH}\n"])
    else:
        raise RuntimeError("No system package inventory for this CI runner")
    return sorted(text.splitlines())


def installed_identity():
    """Describe the environment after package, Python and pinned Windows LLVM setup."""
    clang = os.environ.get("OPENRAC_CLANG") or shutil.which("clang")
    if not clang:
        raise RuntimeError("Clang must be installed before restoring CI preparation")
    cxx = str(Path(clang).with_name("clang++.exe" if os.name == "nt" else "clang++"))
    identity = {
        "workspace": str(Path.cwd().resolve()),
        "system": platform.system(), "machine": platform.machine(),
        "kernel": platform.release(),
        "tools": [tool_identity(tool) for tool in (clang, cxx, "cmake", "ninja")],
        "python": {"path": sys.executable, "version": sys.version},
        "environment": {key: os.environ.get(key, "") for key in (
            "ImageOS", "ImageVersion", "PROCESSOR_IDENTIFIER", "CC", "CXX", "CFLAGS", "CXXFLAGS", "CPPFLAGS",
            "LDFLAGS", "SDKROOT", "CMAKE_PREFIX_PATH", "INCLUDE", "LIB",
            "CMAKE_C_COMPILER_LAUNCHER", "CMAKE_CXX_COMPILER_LAUNCHER")},
    }
    if os.name == "nt":
        # The hosted image identifies Visual Studio and the Windows SDK used by vcvars64.bat.
        if not os.environ.get("ImageVersion"):
            raise RuntimeError("Hosted Windows image identity is required for configure caching")
    else:
        identity["packages"] = system_packages()
        # try_run probes can depend on the runner's supported instruction set.
        cpuinfo = Path("/proc/cpuinfo").read_text()
        identity["cpu_features"] = sorted(next(
            (line.split(":", 1)[1].split() for line in cpuinfo.splitlines()
             if line.startswith(("flags\t", "Features\t"))), []))
    return identity


def main():
    """Publish only a digest; actions/cache adds its job and configuration namespaces."""
    try:
        value = fingerprint(installed_identity())
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        # Missing identity information means a normal uncached build, never a broad restore.
        print(f"::warning::Skipping CI preparation cache: {error}")
        return
    if os.environ.get("GITHUB_OUTPUT"):
        with Path(os.environ["GITHUB_OUTPUT"]).open("a", encoding="utf-8") as stream:
            stream.write(f"fingerprint={value}\n")
    print(f"CI preparation toolchain: {value}")


if __name__ == "__main__":
    main()
