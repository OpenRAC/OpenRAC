"""Readers for the tests: a .glb's JSON and binary chunk, and one accessor's floats."""

import json
import struct


def read_glb(data: bytes) -> tuple[dict, bytes]:
    magic, version, length = struct.unpack_from("<3I", data)
    assert (magic, version, length) == (0x46546C67, 2, len(data))
    size, kind = struct.unpack_from("<I4s", data, 12)
    assert kind == b"JSON" and size % 4 == 0
    doc = json.loads(data[20:20 + size])
    binary_size, kind = struct.unpack_from("<I4s", data, 20 + size)
    assert kind == b"BIN\0" and binary_size == len(data) - 28 - size
    return doc, data[28 + size:]


def attribute(doc: dict, binary: bytes, index: int) -> list[tuple]:
    accessor = doc["accessors"][index]
    view = doc["bufferViews"][accessor["bufferView"]]
    width = int(accessor["type"][3])
    return list(struct.iter_unpack(f"<{width}f", binary[view["byteOffset"]:view["byteOffset"] + view["byteLength"]]))
