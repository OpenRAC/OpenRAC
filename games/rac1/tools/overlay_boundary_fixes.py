#!/usr/bin/env python3
"""Verify and repair PAL v2 overlay boundaries, including four joins of executable code.

These are catalogue corrections, not C matches or new assembly classifications.
The existing overlay_remnants rule must accept every retained remnant.
Run after regenerating the catalogue, then regenerate the PAL version table and
dispatch roles. No other region's catalogue is changed.

  python tools/overlay_boundary_fixes.py --check
  python tools/overlay_boundary_fixes.py --write
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

import overlays as ov
import overlay_remnants as rem

OLD = "func_L00_001EB1A4"
NEW = "func_L00_001EB1A8"
FILL = "func_L06_00216EE4"
SPLIT = "func_L08_002DB438"
EMPTY = "func_001E9768"
CAMERA = "func_L00_002EDB58"
ZERO = "func_001E97C8"
CAMERA_PLACES = [(0, 0x2EDB58), (1, 0x319ED8), (8, 0x3169C8)]
OLD_PLACES = list(enumerate((
    0x1EB1A4, 0x20CD24, 0x1FBFA4, 0x1E4A24, 0x1DDA24,
    0x219DA4, 0x202D24, 0x212EA4, 0x1FDB24, 0x20A3A4,
    0x1DE724, 0x21C924, 0x20DB24, 0x1F6AA4, 0x1FE324,
    0x1E47A4, 0x1E9924, 0x1E8224, 0x1F3DA4,
)))
FILL_PLACES = [(6, 0x216EE4), (8, 0x2590B4), (11, 0x2749DC), (14, 0x25713C)]

# Two more joins whose first bytes equal another function, so the catalogue cut them.
# FLAG: level 0 0x261448 (48 bytes, all 19 levels) = 16-byte prefix (func_L00_0025D390's
# second place) + func_L00_00261458 + the 8-byte return-zero func_001E97C8 tail.
FLAG = "func_L00_00261448"
FLAG_MID = "func_L00_00261458"
FLAG_PREFIX = "func_L00_0025D390"
FLAG_FP = "7820f76139466b04"
FLAG_MID_PLACES = [(0, 0x261458), (1, 0x276290), (2, 0x2633C0), (3, 0x24FAB0), (4, 0x2540B0),
                   (5, 0x28B4D8), (6, 0x26FFB8), (7, 0x289A28), (8, 0x26BAE8), (9, 0x280EC8),
                   (10, 0x253EC8), (11, 0x286988), (12, 0x279E40), (13, 0x26D400),
                   (14, 0x2697D8), (15, 0x250C90), (16, 0x25CCB8), (17, 0x25BD80), (18, 0x263728)]
FLAG_PLACES = [(lv, a - 16) for lv, a in FLAG_MID_PLACES]
# ORXON: level 10 0x2C0348 (408 bytes) = the 48-byte piece shared with func_L03_002DDDF8
# + func_L10_002C0378/0388/03EC/04C4.
ORXON = "func_L10_002C0348"
ORXON_TWIN = "func_L03_002DDDF8"
ORXON_FP = "6a87258ea69ecc03"
ORXON_PIECES = (("func_L10_002C0378", 16, "870e434a010e83ef"),
                ("func_L10_002C0388", 100, "0b59c0f14ae6fc20"),
                ("func_L10_002C03EC", 216, "28a51f5330d5f425"),
                ("func_L10_002C04C4", 28, "481517892b423ed5"))
ORXON_PLACE = (10, 0x2C0348)

MATCHED_MERGES = Path(__file__).resolve().parent.parent / "config/overlays/matched_boundaries.json"


def matched_merges():
    """Only the four joins independently checked EXACT in all 19 levels."""
    return json.loads(MATCHED_MERGES.read_text())


def require(condition, message):
    if not condition:
        raise ValueError(message)


def verify_matched_merges():
    exe = {name: ov.fingerprint(body) for name, _, _, body in ov.exe_functions()}
    for spec in matched_merges():
        require(exe.get(spec["exe"]) == spec["fingerprint"],
                f"{spec['head']}: not the same code as executable function {spec['exe']}")
        require([p[0] for p in spec["places"]] == list(range(19)),
                f"{spec['head']}: expected exactly one audited placement in every level")
        size, offset = spec["size"], spec["prefix_size"]
        for level, address, digest in spec["places"]:
            address = int(address, 16)
            image = rem.level_image(level)
            blob = struct.pack("<" + "I" * (size // 4),
                               *(image.word(at) for at in range(address, address + size, 4)))
            require(hashlib.sha256(blob).hexdigest() == digest,
                    f"{spec['head']} L{level:02d}: audited retail code bytes changed")
            require(ov.fingerprint(blob) == spec["fingerprint"] and
                    ov.fingerprint(blob[:offset]) == spec["prefix_fingerprint"] and
                    ov.fingerprint(blob[offset:]) == spec["tail_fingerprint"],
                    f"{spec['head']} L{level:02d}: prefix/tail identity changed")


def restore_matched_merges(by_name):
    """Absorb only the 19 audited prefix occurrences, retaining every other copy.

    Each joined range is the same code as an executable function, so, as the
    catalogue does for any such function, its places go under that function
    (kind exe) and its C is the executable's."""
    for spec in matched_merges():
        head, tail, prefix, name = (spec[k] for k in ("head", "tail", "prefix", "exe"))
        size, offset = spec["size"], spec["prefix_size"]
        places = [(level, int(address, 16)) for level, address, _ in spec["places"]]
        expected = (name, "exe", size, spec["fingerprint"], places)
        require((name in by_name) != (tail in by_name), f"{head}: expected split or merged entry, not both")
        pro = by_name[prefix]
        require(pro[1:4] == ("shared", offset, spec["prefix_fingerprint"]),
                f"{head}: shared prefix shape changed")
        targets = set(places)
        if tail in by_name:
            old = by_name[tail]
            require(old[1:4] == ("shared", size - offset, spec["tail_fingerprint"]) and
                    old[4] == [(level, address + offset) for level, address in places],
                    f"{head}: split tail differs from the audited range")
            require(targets <= set(pro[4]), f"{head}: missing audited prefix occurrence")
            keep = [place for place in pro[4] if place not in targets]
            require(len({level for level, _ in keep}) == 19,
                    f"{head}: removing absorbed prefixes would lose an independent copy")
            by_name[prefix] = (*pro[:4], keep)
            del by_name[tail]
            by_name[name] = expected
        else:
            require(by_name[name] == expected, f"{head}: merged row differs from audited code")
            require(not targets.intersection(pro[4]), f"{head}: absorbed prefix still overlaps the C function")


def retail_blob(level, address, size):
    image = rem.level_image(level)
    return struct.pack("<" + "I" * (size // 4), *(image.word(at) for at in range(address, address + size, 4)))


def verify_flag_and_orxon():
    for level, address in FLAG_PLACES:
        require(ov.fingerprint(retail_blob(level, address, 48)) == FLAG_FP,
                f"L{level:02d} {address:08X}: flagged-word function bytes changed")
    level, address = ORXON_PLACE
    whole = retail_blob(level, address, 408)
    require(ov.fingerprint(whole) == ORXON_FP, "L10 002C0348: function bytes changed")
    require(whole[:48] == retail_blob(3, 0x2DDDF8, 48), "L10 002C0348: 48-byte piece differs from L03 002DDDF8")
    at = 48
    for _, size, fp in ORXON_PIECES:
        require(ov.fingerprint(whole[at:at + size]) == fp, "L10 002C0348: piece boundaries changed")
        at += size
    require(at == 408, "L10 002C0348: pieces do not add up")


def merge_flag(by_name):
    """Rejoin 16-byte prefix + func_L00_00261458 + return-zero tail into one function."""
    if FLAG in by_name:
        require(by_name[FLAG] == (FLAG, "shared", 48, FLAG_FP, FLAG_PLACES) and FLAG_MID not in by_name,
                f"{FLAG}: merged row differs from audited code")
        return
    mid = by_name.pop(FLAG_MID)
    require(mid[1:5] == ("shared", 24, "077ea5b6ecdb23c4", FLAG_MID_PLACES), f"{FLAG_MID}: unexpected row")
    pre, zero = by_name[FLAG_PREFIX], by_name[ZERO]
    require(set(FLAG_PLACES) <= set(pre[4]), f"{FLAG_PREFIX}: missing audited prefix occurrence")
    tails = {(lv, a + 40) for lv, a in FLAG_PLACES}
    require(tails <= set(zero[4]), f"{ZERO}: missing audited tail occurrence")
    by_name[FLAG_PREFIX] = (*pre[:4], [p for p in pre[4] if p not in set(FLAG_PLACES)])
    by_name[ZERO] = (*zero[:4], [p for p in zero[4] if p not in tails])
    by_name[FLAG] = (FLAG, "shared", 48, FLAG_FP, FLAG_PLACES)


def merge_orxon(by_name):
    """Make the four level-10 pieces and their shared 48-byte start one function."""
    if ORXON in by_name:
        require(by_name[ORXON] == (ORXON, "level", 408, ORXON_FP, [ORXON_PLACE]) and
                not any(n in by_name for n, _, _ in ORXON_PIECES), f"{ORXON}: merged row differs")
        return
    at = ORXON_PLACE[1] + 48
    for name, size, fp in ORXON_PIECES:
        row = by_name.pop(name)
        require(row[1:5] == ("level", size, fp, [(10, at)]), f"{name}: unexpected row")
        at += size
    twin = by_name[ORXON_TWIN]
    require(twin[1:4] == ("shared", 48, "1bc047e59fb89dca") and twin[4] == [(3, 0x2DDDF8), ORXON_PLACE],
            f"{ORXON_TWIN}: unexpected row")
    by_name[ORXON_TWIN] = (ORXON_TWIN, "level", 48, twin[3], [(3, 0x2DDDF8)])
    by_name[ORXON] = (ORXON, "level", 408, ORXON_FP, [ORXON_PLACE])


def verify_retail():
    """Check 27 assembly boundaries and 76 placements of the four joins of executable code."""
    verify_matched_merges()
    verify_flag_and_orxon()
    for level, address in OLD_PLACES:
        image = rem.level_image(level)
        expected = (rem.JR_RA, 0x27BD00C0, 0, 0x27BD0020, 0, 0x27BD0130, 0)
        actual = tuple(image.word(at) for at in range(address - 16, address + 12, 4))
        require(actual == expected, f"L{level:02d} {address:08X}: changed predecessor/remnant bytes")
        require(all(at not in image.reached for at in range(address - 4, address + 12, 4)),
                f"L{level:02d} {address:08X}: a remnant/alignment word is referenced")
        require(image.verdict(address + 4, 4) is None,
                f"L{level:02d} {address + 4:08X}: corrected entry fails the existing remnant rule")
    for level, address in FILL_PLACES:
        image = rem.level_image(level)
        require(address % 8 == 4 and image.word(address) == rem.FILL,
                f"L{level:02d} {address:08X}: not the expected four-byte alignment fill")
        require((image.word(address - 8), image.word(address - 4)) == (rem.JR_RA, 0),
                f"L{level:02d} {address:08X}: preceding function does not return")
        require(address not in image.reached,
                f"L{level:02d} {address:08X}: alignment fill is referenced")
    image = rem.level_image(8)
    require(tuple(image.word(at) for at in range(0x2DB430, 0x2DB448, 4)) ==
            (rem.JR_RA, 0x27BD00D0, 0x27BD0050, 0, rem.JR_RA, 0),
            "L08 002DB438: changed epilogue/remnant/empty function sequence")
    require(image.verdict(0x2DB438, 4) is None,
            "L08 002DB438: corrected entry fails the existing remnant rule")
    require(0x2DB440 in image.reached, "L08 empty function is no longer referenced")
    require((0x2DB440, "UpdateMoby_324") in ov.vtbl_roles(8),
            "L08 dispatch table no longer identifies the empty UpdateMoby_324")

    for level, address in CAMERA_PLACES:
        image = rem.level_image(level)
        expected = (rem.JR_RA, 0x27BD0020, 0, 0xAC60000C, 0, 0xAC400000, 0, rem.JR_RA, 0x0000102D)
        actual = tuple(image.word(at) for at in range(address - 12, address + 24, 4))
        require(actual == expected, f"L{level:02d} {address:08X}: changed camera remnant/return-zero bytes")
        require(all(at not in image.reached for at in range(address, address + 16, 4)),
                f"L{level:02d} {address:08X}: a camera remnant/alignment word is referenced")
        require(image.verdict(address, 12) is None,
                f"L{level:02d} {address:08X}: camera remnants fail the existing rule")
        require((address + 16, "InitCamera_7") in ov.vtbl_roles(level),
                f"L{level:02d} camera table no longer identifies the return-zero InitCamera_7")


def corrected_rows(rows):
    """Return corrected (name, kind, size, fingerprint, places) rows, idempotently."""
    rows = [(n, k, s, fp, list(ps)) for n, k, s, fp, ps in rows]
    by_name = {r[0]: r for r in rows}
    require(len(by_name) == len(rows), "duplicate catalogue names")
    require((OLD in by_name) != (NEW in by_name), "expected one old or corrected shared remnant")
    if OLD in by_name:
        row = by_name.pop(OLD)
        require(row[1:4] == ("shared", 8, "bcb738dd2b9b07d0") and row[4] == OLD_PLACES,
                "shared boundary input differs from the audited PAL v2 catalogue")
    elif NEW in by_name:
        row = by_name[NEW]
        require(row[1] == "shared" and row[2] == 4 and
                row[4] == [(lv, a + 4) for lv, a in OLD_PLACES], "unexpected corrected shared remnant")
    by_name[NEW] = (NEW, "shared", 4, ov.fingerprint(bytes.fromhex("3001bd27")),
                    [(lv, a + 4) for lv, a in OLD_PLACES])
    if FILL in by_name:
        row = by_name.pop(FILL)
        require(row[1:4] == ("shared", 4, "1124cc07fd6404e8") and row[4] == FILL_PLACES,
                "fill input differs from the audited PAL v2 catalogue")
    row = by_name[SPLIT]
    require(row[1] == "level" and row[2] in (4, 16) and row[4] == [(8, 0x2DB438)],
            "unexpected L08 boundary entry")
    if row[2] == 16:
        require(row[3] == "375552645e17b893", "L08 original fingerprint changed")
    by_name[SPLIT] = (SPLIT, "level", 4, ov.fingerprint(bytes.fromhex("5000bd27")), [(8, 0x2DB438)])
    row = by_name[EMPTY]
    require(row[1:4] == ("exe", 4, "bc6e15da5a72ab85"), "empty-function canonical entry changed")
    by_name[EMPTY] = (*row[:4], sorted(set(row[4]) | {(8, 0x2DB440)}))
    row = by_name[CAMERA]
    require(row[1] == "shared" and row[2] in (12, 24) and row[4] == CAMERA_PLACES,
            "unexpected camera boundary entry")
    if row[2] == 24:
        require(row[3] == "121e8a0cd10e4c7b", "camera original fingerprint changed")
    by_name[CAMERA] = (CAMERA, "shared", 12,
                       ov.fingerprint(bytes.fromhex("0c0060ac00000000000040ac")), CAMERA_PLACES)
    row = by_name[ZERO]
    require(row[1:4] == ("exe", 8, "0a5385cf4a659d30"), "return-zero canonical entry changed")
    by_name[ZERO] = (*row[:4], sorted(set(row[4]) | {(lv, a + 16) for lv, a in CAMERA_PLACES}))
    restore_matched_merges(by_name)
    merge_flag(by_name)
    merge_orxon(by_name)
    return sorted(by_name.values(), key=lambda r: (r[4][0][0], r[4][0][1]))


def load(path):
    out = []
    for line in path.read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        n, k, s, fp, _levels, ps = line.split("\t")
        out.append((n, k, int(s), fp, [(int(p[:2]), int(p[3:], 16)) for p in ps.split(",")]))
    return out


def serialize(rows):
    lines = ["# name\tkind\tsize\tfingerprint\tlevels\tplaces (level:address)"]
    for n, k, s, fp, ps in rows:
        lines.append(f"{n}\t{k}\t{s}\t{fp}\t{len({lv for lv, _ in ps})}\t" +
                     ",".join(f"{lv:02d}:{a:08X}" for lv, a in ps))
    return "\n".join(lines) + "\n"


def measures(rows):
    return {"catalogue_entries": len(rows),
            "unique_non_exe_bytes": sum(s for _, k, s, _, _ in rows if k != "exe"),
            "catalogue_placements": sum(len(ps) for _, _, _, _, ps in rows),
            "catalogue_placement_bytes": sum(s * len(ps) for _, _, s, _, ps in rows)}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--catalogue", type=Path, default=ov.CATALOGUE)
    group = ap.add_mutually_exclusive_group(required=True)
    group.add_argument("--check", action="store_true")
    group.add_argument("--write", action="store_true")
    args = ap.parse_args()
    verify_retail()
    before = load(args.catalogue)
    after = corrected_rows(before)
    b, a = measures(before), measures(after)
    print("Before:", b)
    print("After: ", a)
    print("Delta: ", {k: a[k] - b[k] for k in b})
    if args.check:
        require(before == after, "catalogue boundary corrections have not been applied")
        print("PASS: corrected table and 27 assembly boundaries and 76 placements of executable code verified; original remnant rule unchanged")
    else:
        args.catalogue.write_text(serialize(after))
        gone = {n for n, _, _ in ORXON_PIECES} | {FLAG_MID}
        keep = [line for line in ov.VARIANTS.read_text().splitlines(True)
                if line.split("\t")[0] not in gone and line.split("\t")[1] not in gone]
        ov.VARIANTS.write_text("".join(keep))
        print(f"Wrote {args.catalogue}; no C function has been added by this repair")


if __name__ == "__main__":
    main()
