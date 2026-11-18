# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Tool discovery and Windows dependency setup for tools/build_port.py.

Imports on every desktop platform; Windows registry access happens only when needed.
"""
import os
import shutil
import subprocess
from pathlib import Path

WINDOWS = os.name == "nt"

NO_VISUAL_STUDIO = (
    "Visual Studio's C++ tools were not found. Install Visual Studio (Community is free) or its "
    'Build Tools with the "Desktop development with C++" workload, then build again.'
)
NO_CLANG = (
    "Clang was not found. Install LLVM (winget install LLVM.LLVM) or Visual Studio's "
    '"C++ Clang tools for Windows", then build again.'
    if WINDOWS
    else "Clang was not found. Install it (apt install clang, or Xcode's command line tools), then build again."
)
NO_NINJA = (
    'Ninja was not found. Install it (winget install Ninja-build.Ninja) or Visual Studio\'s "C++ CMake '
    'tools for Windows", then build again.'
    if WINDOWS
    else "Ninja was not found. Install it (apt install ninja-build, or brew install ninja), then build again."
)
NO_CMAKE = "CMake 3.25 or newer was not found. Install it (https://cmake.org/download/), then build again."
BUILD_TOOLS = "Microsoft.VisualStudio.2022.BuildTools"
CPP_OPTIONS = (
    "--add", "Microsoft.VisualStudio.Workload.VCTools", "--includeRecommended", "--passive", "--norestart",
)


class BuildError(Exception):
    """Why the build cannot start, worded for whoever reads the launcher's Tasks."""


class MissingTool(BuildError):
    """A missing build tool that WinGet can install on Windows."""

    def __init__(self, message: str, package: str):
        super().__init__(message)
        self.package = package


def say(text: str) -> None:
    print(text, flush=True)


def parse_environment(text: str) -> dict[str, str]:
    """What cmd.exe's `set` prints, as an environment. Names are upper-cased, as Windows ignores
    their case; the drive entries (=C:=...) are left out."""
    env = {}
    for line in text.splitlines():
        key, sep, value = line.partition("=")
        if sep and key:
            env[key.upper()] = value
    return env


def visual_studio_installer() -> Path:
    base = os.environ.get("PROGRAMFILES(X86)") or r"C:\Program Files (x86)"
    return Path(base) / "Microsoft Visual Studio" / "Installer"


def query_visual_studio(*filters: str) -> Path | None:
    """The latest installation matching vswhere's filters; absence differs from a failed query."""
    vswhere = visual_studio_installer() / "vswhere.exe"
    if not vswhere.is_file():
        return None
    try:
        found = subprocess.run([str(vswhere), "-latest", *filters, "-property", "installationPath"],
                               capture_output=True, text=True)
    except OSError as error:
        raise BuildError(f"Could not inspect Visual Studio: {error}") from error
    if found.returncode != 0:
        raise BuildError(f"Could not inspect Visual Studio (vswhere exited with {found.returncode}).")
    lines = found.stdout.strip().splitlines()
    return Path(lines[0]) if lines else None


def find_visual_studio() -> Path:
    """The newest Visual Studio (any edition, Build Tools too) with the C++ x64 tools."""
    found = query_visual_studio("-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64")
    if found is None:
        raise MissingTool(NO_VISUAL_STUDIO, BUILD_TOOLS)
    return found


def msvc_environment(visual_studio: Path) -> dict[str, str]:
    """The environment vcvars64.bat sets up, read back with `set` (UTF-16, cmd's /u, so any
    character in a path comes through). Failures identify setup or readback and include the
    script's output and toolchain availability, without dumping unrelated environment values."""
    bat = visual_studio / "VC" / "Auxiliary" / "Build" / "vcvars64.bat"
    if not bat.is_file():
        raise BuildError(f"{bat} is missing: repair Visual Studio's C++ tools, then build again.")
    setup_complete = "OPENRAC_VCVARS_COMPLETE"
    # Write the marker before starting the environment dump so its failure can be identified.
    command = f'cmd /d /s /c "call "{bat}" 1>&2 && (1>&2 echo {setup_complete}& cmd /d /u /c set)"'
    try:
        done = subprocess.run(command, capture_output=True)
    except OSError as error:
        raise BuildError(f"Could not start cmd.exe to run {bat}\n{error}") from error
    env = parse_environment(done.stdout.decode("utf-16-le", errors="replace"))
    if done.returncode == 0 and env.get("INCLUDE"):
        return env

    diagnostics = done.stderr.decode("oem" if WINDOWS else "utf-8", errors="replace")
    setup_output, marker, dump_output = diagnostics.partition(setup_complete + "\r\n")
    if not marker:
        failure = "vcvars64.bat did not complete successfully"
    elif done.returncode != 0 or not env:
        failure = "cmd.exe could not read back the configured environment"
    else:
        failure = "INCLUDE is missing from the configured environment"

    summary = [
        f"Visual Studio environment setup failed: {failure}.",
        f"Visual Studio: {visual_studio}",
        f"Setup script: {bat}",
        f"Exit code: {done.returncode}",
    ]
    if env:
        summary.append(f"MSVC toolset: {env.get('VCTOOLSVERSION') or 'unset'}")
        summary.append(f"Windows SDK: {env.get('WINDOWSSDKVERSION') or 'unset'}")
        for name in ("INCLUDE", "LIB", "LIBPATH"):
            summary.append(f"{name}: {'set' if env.get(name) else 'unset'}")
        for tool in ("clang.exe", "cl.exe", "link.exe", "rc.exe", "llvm-rc.exe"):
            path = find_program(tool, env.get("PATH"))
            summary.append(f"{tool} on configured PATH: {path or 'not found'}")
    else:
        summary.append("Toolchain environment: unavailable")
    summary.extend(("", "vcvars64.bat output:", setup_output.strip() or "(no output)"))
    if dump_output.strip():
        summary.extend(("", "Environment dump diagnostics:", dump_output.strip()))
    raise BuildError("\n".join(summary))


def clang_candidates(env: dict[str, str], visual_studio: Path | None) -> list[Path]:
    """Where Clang is looked for on Windows, in order."""
    out = []
    if env.get("OPENRAC_CLANG"):
        out.append(Path(env["OPENRAC_CLANG"]))
    for base in (env.get("PROGRAMFILES"), env.get("PROGRAMW6432")):
        if base:
            out.append(Path(base) / "LLVM" / "bin" / "clang.exe")
    if env.get("USERPROFILE"):
        out.append(Path(env["USERPROFILE"]) / "scoop" / "apps" / "llvm" / "current" / "bin" / "clang.exe")
    if visual_studio is not None:
        out.append(visual_studio / "VC" / "Tools" / "Llvm" / "x64" / "bin" / "clang.exe")
    on_path = shutil.which("clang", path=env.get("PATH"))
    if on_path:
        out.append(Path(on_path))
    return out


def find_clang(env: dict[str, str], visual_studio: Path | None) -> tuple[Path, Path]:
    """Clang and Clang++ (beside it), for a new build folder on Windows."""
    for clang in clang_candidates(env, visual_studio):
        if clang.is_file():
            cxx = clang.with_name("clang++.exe")
            return clang, cxx if cxx.is_file() else clang
    raise MissingTool(NO_CLANG, "LLVM.LLVM")


def find_program(name: str, *paths: str | None) -> Path | None:
    for path in paths:
        if path:
            found = shutil.which(name, path=path)
            if found:
                return Path(found)
    return None


def find_cmake(cache: dict[str, str], *paths: str | None) -> Path:
    """The CMake that configured the build folder, else the first on these PATHs."""
    cached = cache.get("CMAKE_COMMAND")
    if cached and Path(cached).is_file():
        return Path(cached)
    found = find_program("cmake", *paths)
    if found is None:
        raise MissingTool(NO_CMAKE, "Kitware.CMake")
    return found


def windows_path() -> str:
    """Include newly installed tools without restarting the launcher or changing Windows' PATH."""
    import winreg

    paths = [os.environ.get("PATH", "")]
    for hive, key in (
        (winreg.HKEY_LOCAL_MACHINE, r"SYSTEM\CurrentControlSet\Control\Session Manager\Environment"),
        (winreg.HKEY_CURRENT_USER, r"Environment"),
    ):
        try:
            with winreg.OpenKey(hive, key) as environment:
                value, _ = winreg.QueryValueEx(environment, "Path")
                paths.append(os.path.expandvars(value))
        except OSError:
            pass
    local = os.environ.get("LOCALAPPDATA")
    if local:
        paths.extend(str(Path(local) / "Microsoft" / folder) for folder in ("WindowsApps", "WinGet/Links"))
    return os.pathsep.join(paths)


def working_winget() -> Path | None:
    """Check the executable too: an App Installer alias can exist without a working client."""
    winget = find_program("winget", windows_path())
    if winget is not None:
        try:
            result = subprocess.run([str(winget), "--version"], capture_output=True, text=True, timeout=15)
            if result.returncode == 0 and result.stdout.strip():
                return winget
        except (OSError, subprocess.TimeoutExpired):
            pass
    return None


def ensure_winget() -> Path:
    """Try registration, then Microsoft's current-user repair once, checking after each step."""
    winget = working_winget()
    if winget is not None:
        return winget
    # Microsoft's documented registration and Microsoft.WinGet.Client repair routes:
    # https://learn.microsoft.com/en-us/windows/package-manager/winget/
    steps = (
        ("Registering Windows App Installer",
         "Add-AppxPackage -RegisterByFamilyName -MainPackage Microsoft.DesktopAppInstaller_8wekyb3d8bbwe"),
        ("Installing or repairing WinGet for your Windows account",
         "[Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol "
         "-bor [Net.SecurityProtocolType]::Tls12; "
         "Install-PackageProvider -Name NuGet -MinimumVersion 2.8.5.201 -Scope CurrentUser -Force | Out-Null; "
         "Install-Module -Name Microsoft.WinGet.Client -Scope CurrentUser -Repository PSGallery "
         "-Force -AllowClobber -Confirm:$false; "
         "Import-Module Microsoft.WinGet.Client; Repair-WinGetPackageManager -Force -Latest"),
    )
    recovery = ("Install or update Microsoft's App Installer: https://apps.microsoft.com/detail/9nblggh4nns1 "
                "then retry the build, or install the missing build tools yourself.")
    for message, script in steps:
        say(message + "...")
        try:
            # Execution policy applies only to this child process; Windows policies are unchanged.
            status = subprocess.run([
                "powershell.exe", "-NoProfile", "-NonInteractive", "-ExecutionPolicy", "RemoteSigned",
                "-Command", "$ErrorActionPreference = 'Stop'; " + script,
            ]).returncode
        except OSError as error:
            raise BuildError(f"Could not start WinGet setup: {error}\n{recovery}") from error
        winget = working_winget()
        if winget is not None:
            return winget
    raise BuildError(f"WinGet setup finished with exit code {status}, but WinGet is still unavailable. "
                     f"See the setup output above.\n{recovery}")


def modify_build_tools() -> bool:
    """Add C++ to an existing VS 2022 Build Tools instance; WinGet install cannot do that."""
    found = query_visual_studio("-products", "Microsoft.VisualStudio.Product.BuildTools", "-version", "[17.0,18.0)")
    if found is None:
        return False
    setup = visual_studio_installer() / "setup.exe"
    if not setup.is_file():
        raise BuildError(f"{setup} is missing. Repair the Visual Studio Installer, then build again.")
    say(f"Adding C++ tools to {found}. Windows may ask for administrator permission.")
    env = dict(os.environ)
    env["OPENRAC_VS_INSTALLER"] = str(setup)
    env["OPENRAC_VS_ARGUMENTS"] = subprocess.list2cmdline([
        "modify", "--installPath", str(found), *CPP_OPTIONS,
    ])
    # setup.exe needs elevation and does not accept the bootstrapper's --wait switch.
    # Keep paths as data, and wait for the elevated installer before rechecking the tools.
    script = ("$ErrorActionPreference = 'Stop'; "
              "$process = Start-Process -FilePath $env:OPENRAC_VS_INSTALLER "
              "-ArgumentList $env:OPENRAC_VS_ARGUMENTS -Verb RunAs -WindowStyle Hidden -Wait -PassThru; "
              "exit $process.ExitCode")
    try:
        status = subprocess.run(["powershell.exe", "-NoProfile", "-NonInteractive", "-Command", script],
                                env=env).returncode
    except OSError as error:
        raise BuildError(f"Could not start the Visual Studio Installer: {error}") from error
    if status not in (0, 3010):  # 3010: installed successfully, a restart may be needed.
        raise BuildError(f"Could not add the C++ tools (installer exited with {status}). "
                         "See its output above, then retry the build.")
    return True


def install_windows_tool(missing: MissingTool) -> None:
    """Install one missing tool, streaming progress into the build's log. Never reboot."""
    if missing.package == BUILD_TOOLS and modify_build_tools():
        return
    try:
        winget = ensure_winget()
    except BuildError as error:
        raise BuildError(f"{error}\n{missing}") from error
    command = [str(winget), "install", "--exact", "--id", missing.package, "--source", "winget",
               "--silent", "--accept-package-agreements", "--accept-source-agreements", "--disable-interactivity"]
    if missing.package == BUILD_TOOLS:
        command += ["--override", subprocess.list2cmdline([*CPP_OPTIONS, "--wait"])]
    say(f"Installing {missing.package} with WinGet. Windows may ask for administrator permission.")
    try:
        status = subprocess.run(command).returncode
    except OSError as error:
        raise BuildError(f"Could not start WinGet: {error}\n{missing}") from error
    if status != 0:
        raise BuildError(f"WinGet could not install {missing.package} (exit code {status}). "
                         f"See its output above; retry the build after resolving it.\n{missing}") from missing
