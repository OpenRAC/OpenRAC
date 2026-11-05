# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Decide whether a pull request can omit native and launcher builds."""

import os
from pathlib import Path
import subprocess


def docs_only(base):
    """Classify the effective PR merge diff. Missing or ambiguous evidence requires full CI."""
    try:
        subprocess.run(["git", "rev-parse", "--verify", "HEAD^2"],
                       check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        parent = subprocess.check_output(["git", "rev-parse", "HEAD^1"]).strip()
        if parent != base.encode():
            return False
        diff = subprocess.check_output(
            ["git", "diff", "--no-renames", "--name-only", "-z", "HEAD^1", "HEAD", "--"])
    except (OSError, subprocess.CalledProcessError):
        return False

    paths = diff.split(b"\0")[:-1]
    # The native env test checks DEBUGGING.md against the environment registry.
    return bool(paths) and all(
        path.startswith(b"docs/") and path.endswith(b".md")
        and path != b"docs/port/DEBUGGING.md" for path in paths
    )


if __name__ == "__main__":
    result = "true" if docs_only(os.environ.get("PR_BASE", "")) else "false"
    with Path(os.environ["GITHUB_OUTPUT"]).open("a", encoding="utf-8") as output:
        print(f"docs_only={result}", file=output)
