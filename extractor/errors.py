"""What a tool says when it stops, for the launcher: one machine-readable line, then the message.

    openrac-error: project-outdated-gameplay
    extract: level 00: this PAL project was made before ...

The launcher reads the code (launcher/src/lib/toolErrors.ts maps each one to a sentence in a
player's words and the step to take); the message goes to the log, behind "Show details". A
code the launcher does not know gets its generic sentence, so a new code never shows a raw
message. test_errors.py checks that every code raised here is one the launcher maps.
"""

import errno
import struct
import sys

from formats import FormatError

# The marker the launcher looks for (launcher/src/lib/toolErrors.ts, ERROR_LINE).
LINE = "openrac-error: "


def code_of(exc: BaseException) -> str:
    """The code for EXC: a FormatError's own, else one for the kind of failure."""
    if isinstance(exc, FormatError):
        return exc.code or "disc-data"
    if isinstance(exc, struct.error):
        return "disc-data"
    if isinstance(exc, OSError):
        if exc.errno == errno.ENOSPC:
            return "disk-full"
        if exc.errno in (errno.EACCES, errno.EPERM, errno.EBUSY, errno.ETXTBSY):
            return "file-locked"
        if exc.errno == errno.ENOENT:
            return "file-missing"
        return "file-error"
    return "internal"


def say(exc: BaseException, stream=None, code: str | None = None) -> str:
    """Prints EXC's code line (CODE, else code_of's; to STREAM, stderr by default) and returns it."""
    code = code or code_of(exc)
    print(f"{LINE}{code}", file=stream or sys.stderr, flush=True)
    return code
