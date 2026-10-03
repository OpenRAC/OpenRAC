"""Report what this machine can do with this repository, and the next command to run.

Read-only: nothing is compiled, no proprietary instrument is executed, and no file is
written. It exists so a contributor can tell, in one command, which of three levels they
are at, and what a missing piece actually blocks:

  1. tooling only   the unit tests and the report export -- nothing proprietary needed
  2. + build        reconstruct assembly and gate a program -- SN ProDG 2.0
  3. + C            prove matching C -- SN ProDG 3.01 as well

It always ends with the single next command to type. Nothing here replaces the gates: a
green doctor is an environment statement, never evidence about a match.
"""
from __future__ import annotations

import argparse
import importlib.metadata
import json
import os
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
sys.path.insert(0, str(HERE))

MINIMUM_PYTHON = (3, 12)

# build.py refuses to run on other versions, and names the same three packages.
REQUIRED_PACKAGES = ("splat64", "spimdisasm", "rabbitizer")

# The instrument layout the other scripts already expect, verbatim:
# build.py uses the assembler and the linker, check_candidates.py uses all five.
ASSEMBLY_INSTRUMENTS = ("ee/bin/Ps2EeAs.exe", "ee/bin/ld.exe")
C_INSTRUMENTS = ("bin/ee-gcc2953.exe", "bin/ee-as.exe", "ee/bin/ld.exe",
                 "lib/gcc-lib/ee/2.95.3/cc1.exe", "lib/gcc-lib/ee/2.95.3/cpp.exe")


def pinned_versions(requirements: Path) -> dict:
    """Read `name==version` pins from requirements.txt; extras such as [mips] are dropped."""
    pins = {}
    if not requirements.is_file():
        return pins
    for line in requirements.read_text(encoding="utf-8").splitlines():
        line = line.split("#", 1)[0].strip()
        if "==" not in line:
            continue
        name, _, version = line.partition("==")
        pins[name.split("[", 1)[0].strip()] = version.strip()
    return pins


def installed_version(package: str) -> str | None:
    try:
        return importlib.metadata.version(package)
    except importlib.metadata.PackageNotFoundError:
        return None


def missing_instruments(root: Path, instruments: tuple) -> list:
    return [name for name in instruments if not (root / name).is_file()]


def inside_repository(path: Path) -> bool:
    resolved = path.resolve()
    return resolved == ROOT or ROOT in resolved.parents


def report_python(lines: list) -> bool:
    version = ".".join(str(part) for part in sys.version_info[:3])
    ok = sys.version_info[:2] >= MINIMUM_PYTHON
    wanted = ".".join(str(part) for part in MINIMUM_PYTHON)
    detail = "ok" if ok else f"below {wanted}; the pinned dependencies may refuse to install"
    lines.append(f"python            {version}  {detail}")
    return ok


def report_packages(lines: list, requirements: Path) -> tuple:
    """Returns (ok, missing) for the packages build.py refuses to run without."""
    pins = pinned_versions(requirements)
    ok, missing = True, []
    for package in REQUIRED_PACKAGES:
        wanted = pins.get(package, "?")
        found = installed_version(package)
        if found == wanted:
            lines.append(f"{package:<17} {found}  ok")
        else:
            shown = found or "not installed"
            lines.append(f"{package:<17} {shown}  needs {wanted} (pip install -r requirements.txt)")
            ok, missing = False, missing + [package]
    return ok, missing


def report_target(lines: list) -> bool:
    path = ROOT / "config" / "target.json"
    if not path.is_file():
        lines.append("config/target.json missing -- is this a complete checkout?")
        return False
    target = json.loads(path.read_text(encoding="utf-8"))
    lines.append(f"target            {target['game']} {target['region']} v{target['version']} "
                 f"({target['serial']}, {target['expected_levels']} levels)")
    lines.append(f"disc expected     {target['iso']['size']} bytes, sha256 {target['iso']['sha256']}")
    lines.append(f"boot expected     {target['boot']['size']} bytes, sha256 {target['boot']['sha256']}")
    return True


def report_disc(lines: list, iso: Path | None) -> bool:
    if iso is None:
        lines.append("disc image        not checked (pass --iso to verify your own copy)")
        return False
    if not iso.is_file():
        lines.append(f"disc image        {iso} not found")
        return False
    try:
        from setup import hashes, require_identity  # same verification path as setup.py
    except ImportError as error:
        lines.append(f"disc image        cannot import setup.py ({error})")
        return False
    try:
        require_identity(hashes(iso), json.loads(
            (ROOT / "config" / "target.json").read_text(encoding="utf-8"))["iso"], "ISO")
    except (OSError, ValueError) as error:
        lines.append(f"disc image        {iso}: {error} -- this is not the supported release")
        return False
    lines.append(f"disc image        {iso} matches the pinned USA v1.01 release")
    return True


def report_toolchain(lines: list, label: str, root: Path | None, instruments: tuple, needed: str) -> bool:
    if root is None:
        lines.append(f"{label:<17} not given ({needed})")
        return False
    missing = missing_instruments(root, instruments)
    if missing:
        lines.append(f"{label:<17} {root} is missing {', '.join(missing)}")
        return False
    lines.append(f"{label:<17} {root} has all {len(instruments)} instruments")
    return True


def report_wrench(lines: list, wrench: Path | None) -> bool:
    if wrench is None:
        lines.append("wrench            not given (needed to unpack the 27 level overlays)")
        return False
    if not wrench.is_file():
        lines.append(f"wrench            {wrench} not found")
        return False
    lines.append(f"wrench            {wrench}")
    return True


def report_runtime(lines: list, runtime: Path | None) -> bool:
    if runtime is None:
        lines.append("runtime           not given (--runtime, a working directory outside this repository)")
        return False
    if inside_repository(runtime):
        lines.append(f"runtime           {runtime} is inside the repository -- choose a directory outside it")
        return False
    if runtime.exists() and not os.access(runtime, os.W_OK):
        lines.append(f"runtime           {runtime} is not writable")
        return False
    lines.append(f"runtime           {runtime} {'(writable)' if runtime.exists() else '(will be created)'}")
    return True


def report_manifest(lines: list, runtime: Path | None) -> Path | None:
    """A previous setup.py run leaves <runtime>/latest.json -> manifest. Then the disc does not
    have to be verified again: re-hashing 3.8 GB is not a useful thing to ask of a contributor."""
    if runtime is None:
        return None
    latest = runtime / "latest.json"
    if not latest.is_file():
        lines.append("manifest          none yet (produced by scripts/setup.py)")
        return None
    try:
        manifest = Path(json.loads(latest.read_text(encoding="utf-8"))["manifest"])
    except (OSError, KeyError, json.JSONDecodeError) as error:
        lines.append(f"manifest          {latest} is unreadable ({error})")
        return None
    if not manifest.is_file():
        lines.append(f"manifest          {latest} points at a missing manifest ({manifest})")
        return None
    lines.append(f"manifest          {manifest} (a previous setup.py run; the disc need not be re-verified)")
    return manifest


def doctor(argv: list | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Report what this machine can do with this repository, and the next command to run")
    parser.add_argument("--iso", type=Path, help="your own disc image, verified against config/target.json")
    parser.add_argument("--toolchain", type=Path, help="SN ProDG 2.0 EE toolchain (assembly and link)")
    parser.add_argument("--c-toolchain", type=Path, help="SN ProDG 3.01 EE toolchain (C candidates)")
    parser.add_argument("--wrench", type=Path, help="wrenchbuild, to unpack the level overlays")
    parser.add_argument("--runtime", type=Path, help="working directory outside this repository")
    args = parser.parse_args(argv)

    lines = ["RAC2 environment", "-----------------"]
    python_ok = report_python(lines)
    packages_ok, _ = report_packages(lines, ROOT / "requirements.txt")
    target_ok = report_target(lines)
    disc_ok = report_disc(lines, args.iso)
    wrench_ok = report_wrench(lines, args.wrench)
    runtime_ok = report_runtime(lines, args.runtime)
    manifest = report_manifest(lines, args.runtime)
    assembly_ok = report_toolchain(lines, "ProDG 2.0", args.toolchain, ASSEMBLY_INSTRUMENTS,
                                   "needed to reconstruct assembly")
    c_ok = report_toolchain(lines, "ProDG 3.01", args.c_toolchain, C_INSTRUMENTS,
                            "needed to prove C candidates")

    lines.append("")
    tooling = python_ok and target_ok
    if not tooling:
        lines.append("VERDICT: this checkout is incomplete -- the tooling cannot run yet.")
        lines.append("Next:   fix the lines marked above, then run this command again.")
        print("\n".join(lines))
        return 2

    prepared = manifest is not None or (disc_ok and wrench_ok and runtime_ok)
    build_ok = prepared and runtime_ok and assembly_ok
    c_ready = build_ok and c_ok

    lines.append("VERDICT")
    lines.append("  tooling (tests + report export)  yes -- needs nothing proprietary")
    lines.append(f"  build (assembly reconstruction)  {'yes' if build_ok else 'not yet'}")
    lines.append(f"  C candidates (byte proofs)       {'yes' if c_ready else 'not yet'}")
    lines.append("")
    lines.append("NEXT COMMAND")
    if not packages_ok:
        lines.append("  pip install -r requirements.txt")
    elif not prepared or not runtime_ok:
        lines.append(f"  python scripts/setup.py --iso <disc.iso> --runtime {args.runtime or '<runtime>'} "
                     f"--wrench <wrenchbuild.exe>")
    elif not assembly_ok:
        lines.append("  python -m unittest discover -s tests -v"
                     "    # assembly build needs a ProDG 2.0 toolchain (--toolchain)")
    elif not c_ok:
        where = manifest or Path(str(args.runtime or "<runtime>")) / "latest.json"
        lines.append(f"  python scripts/build.py --manifest {where} "
                     f"--toolchain {args.toolchain} --all-levels")
    else:
        # The reference sits next to the manifest setup.py wrote; print the real path when known.
        reference = (manifest.parent / "reference" / "boot.elf") if manifest else Path("<runtime>/runs/<id>/reference/boot.elf")
        lines.append(f"  python scripts/check_candidates.py --reference {reference} "
                     f"--toolchain {args.c_toolchain} --runtime {args.runtime or '<runtime>'}")

    print("\n".join(lines))
    return 0


def main() -> int:
    try:
        return doctor()
    except (OSError, ValueError, KeyError, json.JSONDecodeError) as error:
        print(f"Doctor failed: {error}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
