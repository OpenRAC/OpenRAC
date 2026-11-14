#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Makes the test's pictures (port/tests/pack_pictures_test.cpp) again.

Needs Pillow, zstandard (pip) and Basis Universal's basisu tool (release 2.50):
    python3 make.py /path/to/basisu
"""
import struct
import subprocess
import sys
from pathlib import Path

import zstandard
from PIL import Image

HERE = Path(__file__).resolve().parent
COLOURS = [(255, 0, 0, 255), (0, 255, 0, 255), (0, 0, 255, 255), (255, 255, 255, 0)]


def quadrants() -> Image.Image:
    image = Image.new("RGBA", (8, 8))
    for y in range(8):
        for x in range(8):
            image.putpixel((x, y), COLOURS[(y >= 4) * 2 + (x >= 4)])
    return image


def rgba8_zstd(image: Image.Image) -> bytes:
    """A KTX2 of one RGBA8 level (VK_FORMAT_R8G8B8A8_UNORM), Zstandard-compressed. No DFD: the
    port reads the format from the header."""
    level = zstandard.ZstdCompressor(level=19).compress(image.tobytes())
    identifier = bytes([0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A])
    header = struct.pack("<9I4I2Q", 37, 1, 8, 8, 0, 0, 1, 1, 2, 0, 0, 0, 0, 0, 0)
    index_end = len(identifier) + len(header) + 24
    index = struct.pack("<3Q", index_end, len(level), 8 * 8 * 4)
    return identifier + header + index + level


def main() -> None:
    image = quadrants()
    image.save(HERE / "quadrants.png")
    (HERE / "quadrants-rgba8-zstd.ktx2").write_bytes(rgba8_zstd(image))
    basisu = sys.argv[1]
    runs = {
        "quadrants-uastc.ktx2": ["-uastc", "-ktx2_no_zstandard"],
        "quadrants-uastc-zstd.ktx2": ["-uastc", "-ktx2_zstandard_level", "9"],
        "quadrants-etc1s.ktx2": [],
    }
    for name, options in runs.items():
        subprocess.run(
            [basisu, "-ktx2", *options, "-file", str(HERE / "quadrants.png"), "-output_file", str(HERE / name)],
            check=True,
        )


if __name__ == "__main__":
    main()
