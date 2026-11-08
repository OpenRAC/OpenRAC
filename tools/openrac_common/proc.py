# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""
Running other programs: a command, and git in a given checkout.

  from openrac_common import proc
  proc.run(["cmake", "--build", build], cwd=port, echo=True)   # fails loudly, shows the command
  head = proc.git(checkout, "rev-parse", "HEAD")                # stdout, stripped
  dirty = proc.git(decomp, "status", "--porcelain", check=False)

run is written from tools/build_android.py's run (echo the command, fail on a non-zero exit)
and port/tools/hostgen/clangast.py:76 (capture text, replacing what does not decode); git from
tools/sources.py's git (check, optionally bytes) and tools/fcheck_gate.py's (no check). Both
take paths as well as strings in a command and pass every argument as a string. Nothing here
goes through a shell, so no argument is ever split or expanded. Every git call in tools/*.py
goes through git; a command needing what run does not offer (input, a timeout, output
to a file, a shell) keeps subprocess.

OPENRAC_BUILD_LOG=brief (CI) shortens what an echoed command prints (brief_lines): Ninja's line
for each compile becomes a progress line every tenth of the build, and CMake's "-- " status lines
are left out. Everything else, the compilers' warnings and errors and Ninja's FAILED, stays.
"""
import os
import re
import subprocess
import sys
from collections.abc import Mapping, Sequence
from pathlib import Path

Argument = str | Path | int


def run(cmd: Sequence[Argument], *, cwd: Path | str | None = None, env: Mapping[str, str] | None = None,
        echo: bool = False, check: bool = True, capture: bool = False,
        brief: bool = False) -> subprocess.CompletedProcess:
    """Run CMD (no shell) in CWD with ENV (default: this process's), and return how it ended.

    ECHO prints "+ CMD" first and flushes, so the command shows in a log before its own output.
    CHECK raises subprocess.CalledProcessError on a non-zero exit. CAPTURE keeps stdout and
    stderr as text on the result instead of letting them through, replacing bytes that are not
    UTF-8 rather than failing on them (a compiler's message may quote any byte of a source).
    An echoed or BRIEF command's output is shortened under OPENRAC_BUILD_LOG=brief (brief_lines)."""
    args = [str(arg) for arg in cmd]
    if echo:
        print("+", " ".join(args), flush=True)
    if capture:
        return subprocess.run(args, cwd=cwd, env=env, check=check, capture_output=True, text=True,
                              encoding="utf-8", errors="replace")
    if (echo or brief) and brief_log():
        code = run_brief(args, cwd=cwd, env=env)
        if check and code != 0:
            raise subprocess.CalledProcessError(code, args)
        return subprocess.CompletedProcess(args, code)
    return subprocess.run(args, cwd=cwd, env=env, check=check)


def brief_log() -> bool:
    """Whether OPENRAC_BUILD_LOG=brief asks for short build output (CI)."""
    return os.environ.get("OPENRAC_BUILD_LOG", "").strip().lower() == "brief"


# Ninja's status line for one step ("[12/3456] Building C object ..."), and CMake's own status.
NINJA_STEP = re.compile(r"^\[(\d+)/(\d+)\] (.*)$")
QUIET_STEP = re.compile(r"^(?:Building (?:C|CXX|RC|ASM)\w* object|Scanning dependencies)")


def brief_lines(lines, parts: int = 10):
    """LINES without the steps a build log does not need: Ninja's compile lines (a progress line
    "[ 40%] 1234/3085 steps" each tenth instead) and CMake's "-- " status. Other steps (hostgen,
    links, custom commands), warnings, errors and Ninja's FAILED are kept."""
    shown = -1
    left_out = 0
    for line in lines:
        text = line.rstrip("\r\n")
        step = NINJA_STEP.match(text)
        if step and QUIET_STEP.match(step.group(3)):
            done, total = int(step.group(1)), max(int(step.group(2)), 1)
            tenth = done * parts // total
            if tenth != shown:
                shown = tenth
                yield f"[{done * 100 // total:3d}%] {done}/{total} steps"
            continue
        if text.startswith("-- "):
            left_out += 1
            continue
        yield text
    if left_out:
        yield f"({left_out} CMake status lines left out: OPENRAC_BUILD_LOG=brief)"


def run_brief(args: Sequence[str], *, cwd: Path | str | None = None,
              env: Mapping[str, str] | None = None) -> int:
    """Run ARGS with its output (stdout and stderr together) through brief_lines; its exit code."""
    with subprocess.Popen(args, cwd=cwd, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          text=True, encoding="utf-8", errors="replace") as child:
        for text in brief_lines(child.stdout):
            print(text, flush=True)
        return child.wait()


def git(repo: Path | str, *args: Argument, check: bool = True, binary: bool = False) -> str | bytes:
    """git's standard output for `git -C REPO ARGS...`: as bytes when BINARY (git archive, git
    show of a file), else as UTF-8 text with the surrounding whitespace stripped. CHECK raises
    subprocess.CalledProcessError when git fails; without it, a failure gives what git printed,
    usually nothing. git's error output is captured and not shown."""
    done = subprocess.run(["git", "-C", str(repo), *(str(arg) for arg in args)], capture_output=True,
                          check=check)
    return done.stdout if binary else done.stdout.decode().strip()
