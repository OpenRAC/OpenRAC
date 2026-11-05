"""Windows prerequisites for a player's build; installation is explicit, never part of a check."""
from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys

import build_port


LLVM = "LLVM.LLVM"
VS = "Microsoft.VisualStudio.2022.BuildTools"
PACKAGES = {
    LLVM: ("LLVM (Clang compiler)", "compiles the game's C and C++ code",
           "Windows Settings > Apps > Installed apps > LLVM > Uninstall"),
    VS: ("Microsoft Visual Studio 2022 Build Tools",
         "MSVC C++ libraries, Windows SDK, CMake and Ninja (Desktop development with C++)",
         "Visual Studio Installer > Build Tools 2022 > More > Uninstall; use Modify to remove individual components"),
}


def missing() -> list[str]:
    if sys.platform != "win32":
        return []
    required = []
    studio = None
    env = dict(os.environ)
    try:
        studio = build_port.find_visual_studio()
        env = build_port.msvc_environment(studio)
        if not env.get("WINDOWSSDKDIR"):
            required.append(VS)
        build_port.find_cmake({}, os.environ.get("PATH"), env.get("PATH"))
        if build_port.find_program("ninja", env.get("PATH")) is None:
            required.append(VS)
    except build_port.BuildError:
        required.append(VS)
    try:
        build_port.find_clang(env, studio)
    except build_port.BuildError:
        required.append(LLVM)
    return list(dict.fromkeys(required))


def description(packages: list[str]) -> str:
    if not packages:
        return ""
    rows = ["OpenRAC needs these tools to build this game on your PC:"]
    for package in packages:
        name, reason, removal = PACKAGES[package]
        rows += [f"\n{name}\nWhy: {reason}.\nRemove later: {removal}."]
    rows += ["\nOnly the listed missing prerequisites will be installed. Existing tools are kept. "
             "The installers may request administrator access and several GB of disk space. "
             "Removing these tools later prevents rebuilding games and may affect other development software."]
    return "\n".join(rows)


def install(packages: list[str]) -> None:
    winget = shutil.which("winget")
    if winget is None:
        raise build_port.BuildError("Windows App Installer (winget) is missing. Install the listed tools "
                                    "from visualstudio.microsoft.com/downloads and releases.llvm.org, then retry.")
    for package in packages:
        if package not in PACKAGES:
            raise build_port.BuildError(f"unknown build prerequisite: {package}")
        command = [winget, "install", "--id", package, "--exact", "--source", "winget", "--disable-interactivity"]
        if package == VS:
            # The bootstrapper adds the workload to an existing Build Tools installation too.
            command += ["--force", "--override", "--passive --wait --norestart --add "
                        "Microsoft.VisualStudio.Workload.VCTools --includeRecommended"]
        print(f"Installing {PACKAGES[package][0]}", flush=True)
        result = subprocess.run(command, check=False)
        if result.returncode != 0:
            raise build_port.BuildError(f"the installer for {PACKAGES[package][0]} stopped "
                                        f"with code {result.returncode}; the game was not built")
    remaining = missing()
    if remaining:
        raise build_port.BuildError("The tools are not ready yet. Finish the installers, restart the "
                                    "launcher (or Windows if requested), and try again.\n" + description(remaining))


def ensure(install_tools: bool = False) -> None:
    packages = missing()
    if not packages:
        return
    print(description(packages), flush=True)
    if not install_tools:
        raise build_port.BuildError("install the listed tools before building; no source was downloaded")
    install(packages)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", required=True)
    parser.parse_args()
    print(json.dumps(description(missing())))


if __name__ == "__main__":
    main()
