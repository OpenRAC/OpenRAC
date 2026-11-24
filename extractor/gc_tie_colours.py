"""Going Commando's tie vertex colours: what the game's PatchTieVertexLighting (boot 0x2F3E78)
leaves in its vertex colour cache for an instance, so the port can draw each tie vertex in it.

The gameplay file's header word 0x94 points at one entry per tie instance, {s16 instance, u16 n,
n u16}, ended by instance -1. An entry is the instance's colour slots: a word (base RGB in its low
three bytes, a shift in its top byte), then one 5:5:5:1 colour per slot that is shifted right by
the shift and added to the base, saturating. The slots are lit by the instance's directional light
set (VU0, float32 rounded toward zero as the VU does), slot 511 is the class's flat colour, and the
class's remap table for a LOD spreads the slots over the vertices in each packet's order: copies,
averages of two (also weighted 3:1), of three (2:1:1) and of four, and zeros. Packet j's vertex k
takes colour k of the packet's run, which starts on a multiple of four (EmitTieDrawPackets,
0x2F21B0, unpacks the packet's byte 10 colours from the cache).

Checked against retail's cache in level 3 (fcheck states, test only): byte for byte for every
instance without point lights. Point lights (a level's dynamic ones) are not applied; a blended
light set (select & 0xFF00) takes its first set.
"""
from __future__ import annotations

import math
import struct

SLOT_511 = 511


def _t32(x: float) -> float:
    """float32, rounded toward zero (the VU's rounding)."""
    if x == 0:
        return 0.0
    f = struct.unpack("<f", struct.pack("<f", x))[0]
    if abs(f) > abs(x):
        f = struct.unpack("<f", struct.pack("<I", struct.unpack("<I", struct.pack("<f", f))[0] - 1))[0]
    return f


def _mul(a: float, b: float) -> float:
    return _t32(_t32(a) * _t32(b))


def _add(a: float, b: float) -> float:
    return _t32(a + b)


# The executable's table of (cos, sin) of k turns / 256 that a slot's normal code indexes.
_TRIG = [(struct.unpack("<f", struct.pack("<f", math.cos(k * 2 * math.pi / 256)))[0],
          struct.unpack("<f", struct.pack("<f", math.sin(k * 2 * math.pi / 256)))[0]) for k in range(256)]


def colour_entries(gameplay: bytes) -> dict[int, bytes]:
    """Instance index -> its colour entry (header word and slots), from header word 0x94."""
    at, = struct.unpack_from("<I", gameplay, 0x94)
    result = {}
    if not at:
        return result
    while at + 4 <= len(gameplay):
        index, count = struct.unpack_from("<hH", gameplay, at)
        if index < 0:
            break
        result[index] = gameplay[at + 4:at + 4 + 2 * count]
        at += 4 + 2 * count
    return result


def decode_slots(entry: bytes, count: int) -> list[tuple[int, int, int, int]]:
    """The entry's COUNT slot colours, RGBA bytes (0x80 = 1.0), before lighting."""
    word, = struct.unpack_from("<I", entry, 0)
    base = (word & 0xFF, (word >> 8) & 0xFF, (word >> 16) & 0xFF, 0x80)
    shift = word >> 24
    slots = []
    for i in range(count):
        h, = struct.unpack_from("<H", entry, 4 + 2 * i)
        fields = ((h & 31) << 3, ((h >> 5) & 31) << 3, ((h >> 10) & 31) << 3, (h >> 15) << 7)
        slots.append(tuple(min(255, b + (v >> shift)) for b, v in zip(base, fields)))
    return slots


def light_slots(slots, cls: bytes, matrix: list[float], light_set) -> list[tuple[int, int, int, int]]:
    """The slots lit by LIGHT_SET (colour A with its back factor in w, direction A, colour B,
    direction B; floats) for an instance placed by MATRIX (column-major, as the gameplay file's).
    A slot's normal is its code at class +0x34's 8-byte records (+6): (cos a cos e, sin a cos e,
    sin e). The light directions go into the instance's space through its unit axes."""
    c0, l0, c1, l1 = light_set

    def unit(v):
        length = _add(_add(_mul(v[0], v[0]), _mul(v[1], v[1])), _mul(v[2], v[2]))
        if length <= 0:
            return [0.0, 0.0, 0.0]
        q = _t32(1 / math.sqrt(length))
        return [_mul(x, q) for x in v[:3]]

    axes = []
    for j in range(3):
        a = matrix[4 * j:4 * j + 3]
        length = math.sqrt(sum(x * x for x in a))
        w = _t32(1 / length) if length else 0.0
        axes.append([_mul(x, w) for x in a])

    def into(light):
        return [-1.0 * _add(_add(_mul(r[0], light[0]), _mul(r[1], light[1])), _mul(r[2], light[2]))
                for r in axes]

    r0, r1 = into(unit(l0)), into(unit(l1))
    records, = struct.unpack_from("<I", cls, 0x34)
    ceiling = _t32(65537.9)
    out = []
    for i, colour in enumerate(slots):
        code, = struct.unpack_from("<H", cls, records + 8 * i + 6)
        lo, hi = _TRIG[code & 0xFF], _TRIG[code >> 8]
        n = (_mul(lo[0], hi[0]), _mul(lo[1], hi[0]), hi[1])
        d0 = _add(_add(_mul(n[0], r0[0]), _mul(n[1], r0[1])), _mul(n[2], r0[2]))
        d1 = _add(_add(_mul(n[0], r1[0]), _mul(n[1], r1[1])), _mul(n[2], r1[2]))
        d0 = max(d0, _mul(d0, c0[3]))
        d1 = max(d1, _mul(d1, c1[3]))
        lit = []
        for k in range(3):
            a = _add(65536.0, colour[k] / 128.0)
            a = _add(a, _mul(c0[k], d0))
            a = _add(a, _mul(c1[k], d1))
            a = min(a, ceiling)
            lit.append(max(0, min(255, int(round((a - 65536.0) * 128)))))
        out.append((lit[0], lit[1], lit[2], colour[3]))
    return out


def _remap(cls: bytes, lod: int, slots) -> list[tuple[int, int, int, int]]:
    """The class's LOD remap table applied to the 512 slots: the colour cache's run for the LOD."""
    u16 = lambda o: struct.unpack_from("<H", cls, o)[0]
    at = struct.unpack_from("<I", cls, 0x34)[0] + u16(0x2A + 2 * lod)
    sizes, at = cls[at:at + 32], at + 32
    out = []
    for size in sizes:
        if size == 0:
            break
        chunk, at = cls[at:at + size * 16], at + size * 16
        lengths = struct.unpack_from("<8H", chunk, 0)
        buf = [(0, 0, 0, 0)] * (lengths[6] * 4)
        q = 16

        def put(dst, value):
            buf[dst] = value

        for j in range(q, q + lengths[0], 4):
            src, dst = struct.unpack_from("<HH", chunk, j)
            put(dst >> 2, slots[src >> 2])
        q += lengths[0]
        for j in range(q, q + lengths[1], 4):
            w, = struct.unpack_from("<I", chunk, j)
            a, b = slots[(w >> 23) & 0x1FF], slots[(w >> 14) & 0x1FF]
            put((w & 0x3FFC) >> 2, tuple((x + y) >> 1 for x, y in zip(a, b)))
        q += lengths[1]
        for j in range(q, q + lengths[2], 4):
            w, = struct.unpack_from("<I", chunk, j)
            a, b = slots[(w >> 14) & 0x1FF], slots[(w >> 23) & 0x1FF]
            put((w & 0x3FFC) >> 2, tuple((3 * x + y) >> 2 for x, y in zip(a, b)))
        q += lengths[2]
        for j in range(q, q + lengths[3], 8):
            a, b, c, dst = struct.unpack_from("<4H", chunk, j)
            a, b, c = slots[a >> 2], slots[b >> 2], slots[c >> 2]
            put(dst >> 2, tuple((2 * x + y + z) >> 2 for x, y, z in zip(a, b, c)))
        q += lengths[3]
        for j in range(q, q + lengths[4], 8):
            w, = struct.unpack_from("<I", chunk, j)
            fourth, dst = struct.unpack_from("<HH", chunk, j + 4)
            four = [slots[(w >> 2) & 0x1FF], slots[(w >> 11) & 0x1FF], slots[(w >> 20) & 0x1FF], slots[fourth >> 2]]
            put(dst >> 2, tuple(sum(v) >> 2 for v in zip(*four)))
        q += lengths[4]
        for j in range(q, q + lengths[5], 2):
            dst, = struct.unpack_from("<H", chunk, j)
            put(dst >> 2, (0, 0, 0, 0))
        out += buf
    return out


def vertex_colours(entry: bytes, cls: bytes, matrix: list[float] | None = None, light_set=None,
                   lod: int = 0) -> list[int]:
    """One RGBA word (little-endian bytes, 0x80 = 1.0) per vertex of the class's LOD, packets in
    order and each packet's vertices in its own order (ties.tie_mesh's colour index)."""
    count, = struct.unpack_from("<H", cls, 0x38)
    slots = decode_slots(entry, count)
    if light_set is not None and matrix is not None:
        slots = light_slots(slots, cls, matrix, light_set)
    slots = list(slots) + [(0, 0, 0, 0)] * (512 - len(slots))
    if struct.unpack_from("<H", cls, 0x3C)[0] & 0x10:
        slots[SLOT_511] = tuple(cls[0x4C:0x50])
    cache = _remap(cls, lod, slots)
    table, = struct.unpack_from("<I", cls, 4 * lod)
    result, at = [], 0
    for p in range(cls[0x0C + lod]):
        n = cls[table + 16 * p + 10]
        result += [r | g << 8 | b << 16 | a << 24 for r, g, b, a in cache[at:at + n]]
        at += (n + 3) // 4 * 4
    return result


# What each vertex's colour is made of, for relighting an instance at run time (the port relights a
# tie the game moves, as the game does: the mover marks it and PatchTieVertexLighting lights its
# slots again by its matrix then). One (kind, a, b, c, d) per vertex in vertex_colours' order:
# kind 0 none (zero), 1 slot a, 2 (a + b) >> 1, 3 (3a + b) >> 2, 4 (2a + b + c) >> 2,
# 5 (a + b + c + d) >> 2, each channel.
ZERO, COPY, HALF, THREE_ONE, TWO_ONE_ONE, QUARTER = range(6)


def vertex_slot_refs(cls: bytes, lod: int = 0) -> list[tuple[int, int, int, int, int]]:
    """The remap of _remap, kept as slot references instead of colours, laid out per vertex as
    vertex_colours lays out the colours."""
    u16 = lambda o: struct.unpack_from("<H", cls, o)[0]
    at = struct.unpack_from("<I", cls, 0x34)[0] + u16(0x2A + 2 * lod)
    sizes, at = cls[at:at + 32], at + 32
    cache = []
    for size in sizes:
        if size == 0:
            break
        chunk, at = cls[at:at + size * 16], at + size * 16
        lengths = struct.unpack_from("<8H", chunk, 0)
        buf = [(ZERO, 0, 0, 0, 0)] * (lengths[6] * 4)
        q = 16
        for j in range(q, q + lengths[0], 4):
            src, dst = struct.unpack_from("<HH", chunk, j)
            buf[dst >> 2] = (COPY, src >> 2, 0, 0, 0)
        q += lengths[0]
        for j in range(q, q + lengths[1], 4):
            w, = struct.unpack_from("<I", chunk, j)
            buf[(w & 0x3FFC) >> 2] = (HALF, (w >> 23) & 0x1FF, (w >> 14) & 0x1FF, 0, 0)
        q += lengths[1]
        for j in range(q, q + lengths[2], 4):
            w, = struct.unpack_from("<I", chunk, j)
            buf[(w & 0x3FFC) >> 2] = (THREE_ONE, (w >> 14) & 0x1FF, (w >> 23) & 0x1FF, 0, 0)
        q += lengths[2]
        for j in range(q, q + lengths[3], 8):
            a, b, c, dst = struct.unpack_from("<4H", chunk, j)
            buf[dst >> 2] = (TWO_ONE_ONE, a >> 2, b >> 2, c >> 2, 0)
        q += lengths[3]
        for j in range(q, q + lengths[4], 8):
            w, = struct.unpack_from("<I", chunk, j)
            fourth, dst = struct.unpack_from("<HH", chunk, j + 4)
            buf[dst >> 2] = (QUARTER, (w >> 2) & 0x1FF, (w >> 11) & 0x1FF, (w >> 20) & 0x1FF, fourth >> 2)
        q += lengths[4]
        for j in range(q, q + lengths[5], 2):
            dst, = struct.unpack_from("<H", chunk, j)
            buf[dst >> 2] = (ZERO, 0, 0, 0, 0)
        cache += buf
    table, = struct.unpack_from("<I", cls, 4 * lod)
    result, at = [], 0
    for p in range(cls[0x0C + lod]):
        n = cls[table + 16 * p + 10]
        result += cache[at:at + n]
        at += (n + 3) // 4 * 4
    return result


def slot_normal_codes(cls: bytes) -> list[int]:
    """Each slot's normal code (class +0x34's 8-byte records, +6): azimuth | elevation << 8."""
    count, = struct.unpack_from("<H", cls, 0x38)
    records, = struct.unpack_from("<I", cls, 0x34)
    return [struct.unpack_from("<H", cls, records + 8 * i + 6)[0] for i in range(count)]


def slot_positions(cls: bytes) -> list[int]:
    """Each slot's position, s16 x, y, z in the class's units (class +0x34's 8-byte records): the
    point lights' pass places it by the instance's matrix times the class scale (+0x40) / 1024."""
    count, = struct.unpack_from("<H", cls, 0x38)
    records, = struct.unpack_from("<I", cls, 0x34)
    return [v for i in range(count) for v in struct.unpack_from("<3h", cls, records + 8 * i)]


def class_scale(cls: bytes) -> float:
    """The class's scale (+0x40), which the game keeps in its instances' matrices' last word."""
    return struct.unpack_from("<f", cls, 0x40)[0]


def flat_colour(cls: bytes) -> int | None:
    """Slot 511's colour, the class's flat colour (RGBA word), when the class has one."""
    if struct.unpack_from("<H", cls, 0x3C)[0] & 0x10:
        return struct.unpack_from("<I", cls, 0x4C)[0]
    return None


def mix(refs, slots) -> list[int]:
    """vertex_colours' words from vertex_slot_refs and the 512 lit slots (a check of the refs)."""
    out = []
    for kind, a, b, c, d in refs:
        if kind == ZERO:
            v = (0, 0, 0, 0)
        elif kind == COPY:
            v = slots[a]
        elif kind == HALF:
            v = tuple((x + y) >> 1 for x, y in zip(slots[a], slots[b]))
        elif kind == THREE_ONE:
            v = tuple((3 * x + y) >> 2 for x, y in zip(slots[a], slots[b]))
        elif kind == TWO_ONE_ONE:
            v = tuple((2 * x + y + z) >> 2 for x, y, z in zip(slots[a], slots[b], slots[c]))
        else:
            v = tuple(sum(t) >> 2 for t in zip(slots[a], slots[b], slots[c], slots[d]))
        out.append(v[0] | v[1] << 8 | v[2] << 16 | v[3] << 24)
    return out
