"""How far along a tool is, for the launcher: one machine-readable line per step of work.

    openrac-progress: read 3/18 4
    openrac-progress: project

The step, then how many of its items are done out of how many, then the level the newest one
was (a level's number). A step without a count says only its name, and the launcher shows a bar
without an end. Levels are often read in parallel, so the count goes up as each one finishes,
in whatever order they do. The launcher reads these lines (launcher/src/lib/progress.ts,
PROGRESS_LINE) and names each step in a player's words; they never reach the job's "Show details".
test_progress_line.py checks the format and that every step said here is one the launcher names.
"""

import re
import sys
from collections.abc import Callable, Iterable, Iterator
from typing import TypeVar

# The marker the launcher looks for (launcher/src/lib/progress.ts, PROGRESS_LINE).
LINE = "openrac-progress: "

# The steps, and what each counts.
STEPS = {
    "read": "levels read from the disc (Godot projects, the port's copy)",
    "project": "Godot projects written, one per level",
    "carry": "levels whose edits were carried onto a fresh copy",
    "pack": "levels packed",
    "visibility": "levels whose visibility was added for the port",
    "textures": "levels whose textures a pack was shown or hidden in",
}

PATTERN = re.compile(r"^(?P<step>[a-z]+)(?: (?P<done>\d+)/(?P<total>\d+)(?: (?P<level>\d+))?)?$")


def text(step: str, done: int | None = None, total: int | None = None, level: int | None = None) -> str:
    """The line for STEP: DONE of TOTAL, the newest at LEVEL; the step alone without a count."""
    if step not in STEPS:
        raise ValueError(f"unknown progress step {step!r}")
    if done is None or total is None:
        return f"{LINE}{step}"
    if not 0 <= done <= total:
        raise ValueError(f"progress {done}/{total} is out of range")
    return f"{LINE}{step} {done}/{total}" + ("" if level is None else f" {level}")


def say(step: str, done: int | None = None, total: int | None = None, level: int | None = None,
        stream=None) -> str:
    """Prints STEP's line (to STREAM, stdout by default) and returns it."""
    line = text(step, done, total, level)
    print(line, file=stream or sys.stdout, flush=True)
    return line


def parse(line: str) -> dict | None:
    """A progress line read back ({"step", "done", "total", "level"}), or None for any other line."""
    if not line.startswith(LINE):
        return None
    found = PATTERN.match(line[len(LINE):].strip())
    if found is None or found["step"] not in STEPS:
        return None
    number = lambda key: None if found[key] is None else int(found[key])  # noqa: E731
    return {"step": found["step"], "done": number("done"), "total": number("total"), "level": number("level")}


T = TypeVar("T")


def counted(step: str, items: Iterable[T], level: Callable[[T], int | None] = lambda item: None,
            stream=None) -> Iterator[T]:
    """ITEMS one by one, saying STEP's 0 of all first and each count as an item's work ends (the
    loop asks for the next). A loop left by an error says no more."""
    items = list(items)
    say(step, 0, len(items), stream=stream)
    for done, item in enumerate(items, 1):
        yield item
        say(step, done, len(items), level(item), stream=stream)
