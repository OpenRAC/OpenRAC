#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""How far two runs' shots (OPENRAC_SHOT) differ, shot by shot, with Pillow.

    shot_diff.py A B

Each PNG in folder A is compared with the one of the same name in B: the pixels whose largest
channel difference is over 0, and over 8, and the largest difference. A shot missing from B is
counted as missing, so a run that stopped early shows.
"""

import sys
from pathlib import Path

from PIL import Image, ImageChops


def main() -> int:
    a, b = Path(sys.argv[1]), Path(sys.argv[2])
    total = big = worst = missing = 0
    for shot in sorted(a.glob("*.png")):
        other = b / shot.name
        if not other.exists():
            missing += 1
            continue
        bands = ImageChops.difference(Image.open(shot).convert("RGB"), Image.open(other).convert("RGB")).split()
        largest = ImageChops.lighter(ImageChops.lighter(bands[0], bands[1]), bands[2])
        h = largest.histogram()
        differ, over = sum(h[1:]), sum(h[9:])
        top = max((i for i, n in enumerate(h) if n), default=0)
        print(f"{shot.name}: {differ} pixels differ, {over} by more than 8, by up to {top}")
        total, big, worst = total + differ, big + over, max(worst, top)
    print(f"{a} against {b}: {total} pixels differ, {big} by more than 8, by up to {worst}; {missing} shots missing")
    return 0


if __name__ == "__main__":
    sys.exit(main())
