#!/usr/bin/env python3
"""Keep native work recording and queue operations inside graph executors."""
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
rules = (
    (re.compile(r"\bvkCmd[A-Za-z0-9_]+\s*\("), "renderer/vulkan/commands/native_command_encoder.cpp"),
    (re.compile(r"\bvkQueue(?:Submit(?:2(?:KHR)?)?|PresentKHR|BindSparse)\s*\("),
     "renderer/vulkan/commands/graph_queue.cpp"),
)
failures = []
for directory in ("renderer", "enhanced", "game", "common", "platform", "viewer"):
    for path in (root / directory).rglob("*"):
        if path.suffix not in {".c", ".cpp", ".h", ".inc"}:
            continue
        relative = path.relative_to(root).as_posix()
        for line, source in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            for pattern, allowed in rules:
                if relative != allowed and pattern.search(source):
                    failures.append(f"{relative}:{line}: native operation belongs in {allowed}")
if failures:
    print("\n".join(failures), file=sys.stderr)
    sys.exit(1)
print("Native command and queue boundaries passed")
