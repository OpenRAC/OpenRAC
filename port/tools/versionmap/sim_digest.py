#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""What two rac1 discs' levels must agree on to be played together, measured on the discs.

    python3 port/tools/versionmap/sim_digest.py DISC [DISC ...]

Reads each disc image in place and prints each level's sim digest (port/game/rac1/host/sim_digest.h,
the game's own reading of a level's moby records, on the same 60 Hz copy the port loads), with a
digest of the whole records beside it: two discs with the second number different and the first the
same differ only in what each game keeps to itself. Exits 1 when two discs' sim digests differ on a
level. Writes nothing.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "editor"))
import disc as discs  # noqa: E402
import level as levels  # noqa: E402

RECORD = 0x78
LEVELS = 19
KEYS = (0, 0x20)
PLACE = (0x30, 0x48)


def mix(value, data):
    for byte in data:
        value = ((value ^ byte) * 16777619) & 0xFFFFFFFF
    return value


def digest(records):
    value = mix(2166136261, struct.pack("<i", len(records)))
    for r in records:
        value = mix(value, r[KEYS[0] : KEYS[1]])
        value = mix(value, r[PLACE[0] : PLACE[1]])
    return value or 1


def whole(records):
    value = 2166136261
    for r in records:
        value = mix(value, r)
    return value


def read_level(disc, toc, number):
    info = disc.level_header(toc, number)
    where = info["ranges"]["gameplay_ntsc"]
    gameplay = levels.decoded(disc.sectors(where["lba"], where["sectors"]))
    start = struct.unpack_from("<i", gameplay, 0x44)[0]
    count = struct.unpack_from("<i", gameplay, start)[0]
    return [gameplay[start + 0x10 + i * RECORD : start + 0x10 + (i + 1) * RECORD] for i in range(count)]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("discs", nargs="+", type=Path)
    args = parser.parse_args()
    table = []
    for path in args.discs:
        with discs.Disc(path) as disc:
            version, _ = disc.identify()
            toc = disc.read(version.toc_lba * discs.SECTOR, version.toc_size)
            table.append([read_level(disc, toc, n) for n in range(LEVELS)])
    failed = 0
    for n in range(LEVELS):
        sim = [digest(t[n]) for t in table]
        full = [whole(t[n]) for t in table]
        same = len(set(sim)) == 1
        failed += 0 if same else 1
        print(
            f"level {n:2d} records {'/'.join(str(len(t[n])) for t in table):>11}"
            f" sim {' '.join(f'{d:08x}' for d in sim)} whole {' '.join(f'{d:08x}' for d in full)}"
            f"{'' if same else '  DIFFERS'}{'  (records differ: allowed)' if same and len(set(full)) > 1 else ''}"
        )
    print(f"{LEVELS - failed} of {LEVELS} levels agree")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
