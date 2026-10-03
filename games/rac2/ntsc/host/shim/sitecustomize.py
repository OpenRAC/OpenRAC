"""Run the RAC2 pipeline (written for Windows + WSL) on macOS or Linux, unchanged.

Python imports this module at start-up when its directory is on PYTHONPATH.
It changes how three things are executed and nothing else; every hash,
comparison and gate in the project's scripts runs exactly as written.

* A program whose name ends in ".exe" (Ps2EeAs.exe, ld.exe) is started through
  RAC2_EXE_RUNNER, default "wine". "{cwd}" in the runner is replaced by the
  working directory of the call, e.g.
      RAC2_EXE_RUNNER="docker exec -w {cwd} rac2-wine wine"
* "wsl.exe -d <distro> -e bash -lc <script>" (scripts/wsl_chain.py) runs
  <script> through RAC2_LINUX_RUNNER, default "bash -c", e.g.
      RAC2_LINUX_RUNNER="docker exec rac2-gnu bash -c"
  RAC2_WSL_TOOLS and RAC2_WSL_TMP keep their meaning (paths seen by that shell).
* wsl_chain.wsl_path() returns the POSIX path itself instead of /mnt/<drive>/...,
  because the Linux side sees the same paths as this process.

The files the scripts hash (Ps2EeAs.exe, ld.exe, cc1, cpp, as) are the real
instruments; only the launcher changes.
"""
from __future__ import annotations

import importlib.abc
import importlib.machinery
import os
import shlex
import subprocess
import sys
from pathlib import Path

_EXE_RUNNER = os.environ.get("RAC2_EXE_RUNNER", "wine")
_LINUX_RUNNER = os.environ.get("RAC2_LINUX_RUNNER", "bash -c")
_ORIGINAL_POPEN = subprocess.Popen


def _rewrite(args, cwd):
    if isinstance(args, (str, bytes, os.PathLike)) or not args:
        return args
    args = [os.fsdecode(a) if isinstance(a, (bytes, os.PathLike)) else a for a in args]
    program = os.path.basename(args[0]).lower()
    if program == "wsl.exe":
        # wsl.exe -d DISTRO -e bash -lc SCRIPT
        if len(args) != 7 or args[1] != "-d" or args[3:6] != ["-e", "bash", "-lc"]:
            raise ValueError(f"Unexpected WSL invocation: {args[:6]}")
        return shlex.split(_LINUX_RUNNER) + [args[6]]
    if program.endswith(".exe"):
        directory = os.fspath(cwd) if cwd is not None else os.getcwd()
        runner = [part.replace("{cwd}", directory) for part in shlex.split(_EXE_RUNNER)]
        return runner + args
    return args


class _Popen(_ORIGINAL_POPEN):
    def __init__(self, args, *positional, **keywords):
        super().__init__(_rewrite(args, keywords.get("cwd")), *positional, **keywords)


subprocess.Popen = _Popen


class _WslChainFinder(importlib.abc.MetaPathFinder):
    def find_spec(self, name, path, target=None):
        if name != "wsl_chain":
            return None
        spec = importlib.machinery.PathFinder.find_spec(name, path)
        if spec is None or spec.loader is None:
            return spec
        original = spec.loader.exec_module

        def exec_module(module):
            original(module)
            module.wsl_path = lambda value: str(Path(value).resolve())

        spec.loader.exec_module = exec_module
        return spec


sys.meta_path.insert(0, _WslChainFinder())
