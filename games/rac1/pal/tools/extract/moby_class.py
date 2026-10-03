"""Moby classes: each class blob's high-LOD mesh in its bind pose, and its
skeleton and animations (moby_anim.py).

The core index's moby class table (+0x18) has 32-byte entries: the core
offset of the class blob (0 for classes without one), the class number,
two unknown words and 16 texture slots into the moby texture table. The
blob layout follows ReRAC's format notes and loader (ISC;
docs/formats/moby_rac1.md and crates/rc-formats/src/moby.rs), which
derive it from Wrench's moby reader and the game's own code. The PAL
disc has the same layout.

A class mesh is split into packets, each a VIF list (texture coordinates,
an index stream and optional GS texture blocks) and a vertex table. The
vertex cache, the VU0 matrix slots and the current texture carry over
from one packet to the next within a LOD list.
"""

from dataclasses import dataclass, field
import math
import struct

from formats import FormatError, Texture, span, unpack
from mesh import Mesh
from moby_anim import Skeleton, class_sequences, ratchet_sequences, skeleton

HEADER_SIZE = 0x48
# UNPACK commands: V2-16 with the write mask (texture coordinates), V4-8
# (index stream) and V4-32 (GS texture blocks), and their payload bytes per
# element. Addresses are relative to VIF1 TOPS (flag 0x8000).
ST, INDICES, GS_BLOCKS = 0x75, 0x6e, 0x6c
ELEMENT_SIZES = {ST: 4, INDICES: 4, GS_BLOCKS: 16}
ST_ADDRESS, INDEX_ADDRESS = 0x80c2, 0x812d
NO_STORE = 0xf4  # VU0 address meaning "do not keep this matrix".
# A GS texture block whose TEX0 word is -1: the game binds an 8x8 texture of
# 0x80 texels instead, so the faces show plain vertex colour. The mesh uses
# this key and the writer pairs it with GREY.
UNTEXTURED = ("moby", -1)
GREY = Texture(8, 8, bytes(64), bytes((0x80,) * 4) + bytes(1020))


@dataclass
class MobyClass:
    """A class's scale, its high-LOD mesh in model units (packed / 1024)
    and, for an animated class, its skeleton.

    Drawn size is model units times the class scale times the instance's
    own scale. skins[i] lists the (joint, weight) pairs of mesh vertex i,
    weights in 256ths summing to 256; the mesh carries them only when the
    class has a skeleton.
    """
    scale: float
    mesh: Mesh | None
    joint_count: int = 0
    skins: list[tuple[tuple[int, int], ...]] = field(default_factory=list)
    skeleton: Skeleton | None = None


def normal(azimuth: int, elevation: int) -> tuple[float, float, float]:
    """Spherical angles in 256ths of a turn, as ReRAC decodes them from VU0 code."""
    a, e = azimuth * math.pi / 128, elevation * math.pi / 128
    return (math.cos(a) * math.cos(e), math.sin(a) * math.cos(e), math.sin(e))


def unpacks(data: bytes) -> list[tuple[int, int, int, bytes]]:
    """(command, immediate, count, payload) of each UNPACK in a VIF list; NOPs are skipped."""
    result, pos = [], 0
    while pos < len(data):
        word, = unpack("<I", data, pos)
        command, count, immediate = word >> 24, (word >> 16) & 0xff or 256, word & 0xffff
        pos += 4
        if command == 0:
            continue
        if command not in ELEMENT_SIZES:
            raise FormatError(f"unsupported moby VIF command {command:#x}")
        size = ELEMENT_SIZES[command] * count
        result.append((command, immediate, count, span(data, pos, size)))
        pos += size
    return result


class Slots:
    """VU0 matrix slots (ReRAC moby_rac1.md §2.10): each holds a joint or a blend of joints."""

    def __init__(self):
        self.slots: dict[int, tuple[tuple[int, int], ...]] = {}

    def store(self, address: int, skin: tuple) -> None:
        if address % 4:
            raise FormatError("unaligned moby VU0 address")
        self.slots[address] = skin

    def load(self, address: int) -> tuple:
        if address % 4 or address >= NO_STORE or address not in self.slots:
            raise FormatError("moby vertex loads an unwritten or invalid VU0 slot")
        return self.slots[address]

    def joint(self, address: int) -> int:
        skin = self.load(address)
        if len(skin) != 1:
            raise FormatError("moby blend of a blend")
        return skin[0][0]

    def vertex(self, kind: int, r: bytes) -> tuple:
        """Bytes 0-7 of a two-way (0), three-way (1) or single (2) vertex. Stores come first."""
        if kind == 0:
            self.store(r[6], ((r[1] >> 1, 256),))
            skin = ((self.joint(r[2]), r[4]), (self.joint(r[3]), r[5]))
        elif kind == 1:
            skin = ((self.joint(r[2]), r[4]), (self.joint(r[3]), r[5]), (self.joint(r[1] & 0xfe), r[6]))
        else:
            self.store(r[3], ((r[1] >> 1, 256),))
            return self.load(r[2])
        if sum(w for _, w in skin) != 256:
            raise FormatError("moby blend weights do not sum to 256")
        self.store(r[7], skin)
        return skin


class ListState:
    """What carries over between the packets of one LOD list."""

    def __init__(self):
        self.cache: dict[int, tuple] = {}
        self.slots = Slots()
        self.texture: int | None = None


def packet(data: bytes, entry: bytes, state: ListState, mesh: Mesh, remap: bytes,
           skins: list) -> None:
    """Add one regular packet's vertices and drawn triangles to mesh."""
    list_offset, list_size, _, table, data_size, positions_qwc, colours_qwc, transfer = unpack("<IHHIBBBB", entry)
    commands = unpacks(span(data, list_offset, list_size * 16))
    if [c[0] for c in commands] not in ([ST, INDICES], [ST, INDICES, GS_BLOCKS]):
        raise FormatError("unsupported moby VIF program")
    (_, st_at, st_count, st), (_, index_at, index_count, index_data) = commands[:2]
    if (st_at, index_at) != (ST_ADDRESS, INDEX_ADDRESS) or index_data[3]:
        raise FormatError("unexpected moby VIF addresses")
    secrets, textures = [index_data[2]], []
    if len(commands) == 3:
        _, blocks_at, count, blocks = commands[2]
        if count % 4 or blocks_at != INDEX_ADDRESS + index_count or index_data[1] != index_count:
            raise FormatError("misplaced moby GS texture blocks")
        for b in range(count // 4):
            textures.append(unpack("<i", blocks, b * 64 + 32)[0])  # TEX0 data, patched at load time.
            secrets.append(blocks[b * 16 + 12])  # Extra indices ride in successive qwords.
    if positions_qwc != (transfer * 6 + 15) // 16 or colours_qwc != (transfer + 3) // 4:
        raise FormatError("moby packet entry sizes disagree")

    transfers, two, three, single, duplicates, total, vertex_offset, multipliers = unpack("<8I", data, table)
    in_file = two + three + single
    if total != in_file + duplicates or total != transfer:
        raise FormatError("moby vertex counts disagree")
    pairs = span(data, table + 0x20, transfers * 2)
    at = table + 0x20 + transfers * 2
    at += 2 if at % 4 else 0
    at += 4 if at % 8 else 0
    copies = struct.unpack(f"<{duplicates}H", span(data, at, duplicates * 2))
    epilogue = (multipliers - vertex_offset) // 16 - in_file if multipliers >= vertex_offset else -1
    if not 1 <= epilogue <= 6 or (multipliers - vertex_offset) % 16:
        raise FormatError("invalid moby epilogue vertex count")
    if data_size * 16 - multipliers != (transfer * 4 + 15) // 16 * 16:
        raise FormatError("moby colour multipliers do not fit the vertex data")
    records = span(data, table + vertex_offset, (in_file + epilogue) * 16)
    # Each record carries the 9-bit cache ID of the vertex seven places earlier;
    # the last ones continue in the six halfwords at +4 of the final record.
    ids = [unpack("<H", records, i * 16)[0] & 0x1ff for i in range(7, in_file + epilogue)]
    ids += [h & 0x1ff for h in unpack("<6H", records, len(records) - 12)]
    if len(ids) < in_file:
        raise FormatError("moby packet lacks vertex IDs")

    for i in range(transfers):
        state.slots.store(pairs[i * 2 + 1], ((pairs[i * 2], 256),))
    vertices = []
    for i in range(in_file):
        record = records[i * 16:(i + 1) * 16]
        kind = 0 if i < two else 1 if i < two + three else 2
        skin = state.slots.vertex(kind, record)
        x, y, z = unpack("<3h", record, 10)
        vertices.append(((x / 1024, y / 1024, z / 1024), normal(record[8], record[9]), skin))
    for i, vertex in enumerate(vertices):
        state.cache[ids[i]] = vertex
    for copy in copies:
        if copy & 0x7f or (copy >> 7) not in state.cache:
            raise FormatError("moby duplicate vertex refers to an empty cache slot")
        vertices.append(state.cache[copy >> 7])
    if st_count < len(vertices):
        raise FormatError("moby packet has fewer texture coordinates than vertices")

    base = len(mesh.positions)
    for i, (position, n, skin) in enumerate(vertices):
        s, t = unpack("<2h", st, i * 4)
        mesh.positions.append(position)
        mesh.uvs.append((s / 4096, t / 4096))
        mesh.normals.append(n)
        skins.append(skin)

    # Index stream: 1-based, bit 7 suppresses the drawing kick. A 0 switches
    # to the next GS block's texture and pushes the next extra index; an
    # extra index of 0 ends the packet. The last three pushes (1, 1, 1)
    # flush the VU1 pipeline and are never drawn.
    pushes, secret, block = [], 0, 0
    for byte in index_data[4:]:
        if byte == 0:
            if secret >= len(secrets):
                raise FormatError("moby index stream runs out of extra indices")
            byte, secret = secrets[secret], secret + 1
            if byte == 0:
                break
            if block >= len(textures):
                raise FormatError("moby texture switch without a GS block")
            state.texture, block = textures[block], block + 1
            byte |= 0x80
        if not 0 < byte & 0x7f <= len(vertices):
            raise FormatError("moby vertex index out of range")
        pushes.append(((byte & 0x7f) - 1, byte & 0x80 == 0, state.texture))
    else:
        raise FormatError("moby index stream is not terminated")
    if block != len(textures) or [p[0] for p in pushes[-3:]] != [0, 0, 0]:
        raise FormatError("moby packet does not end with its flush trailer")
    pushes = pushes[:-3]
    for n in range(2, len(pushes)):
        index, kick, texture = pushes[n]
        if not kick:
            continue
        if texture is None or texture < -1 or texture >= 16:
            raise FormatError(f"invalid moby texture index {texture}")
        if texture >= 0 and remap[texture] == 255:
            raise FormatError("moby texture slot has no texture")
        key = UNTEXTURED if texture < 0 else ("moby", remap[texture])
        a, b, c = (base + pushes[n - 2][0], base + pushes[n - 1][0], base + index)
        mesh.add_face(key, orient(mesh, a, b, c))


def orient(mesh: Mesh, a: int, b: int, c: int) -> tuple[int, int, int]:
    """Strip order keeps no consistent winding; face the stored vertex normals."""
    pa, pb, pc = (mesh.positions[v] for v in (a, b, c))
    ab = [pb[j] - pa[j] for j in range(3)]
    ac = [pc[j] - pa[j] for j in range(3)]
    cross = (ab[1] * ac[2] - ab[2] * ac[1], ab[2] * ac[0] - ab[0] * ac[2], ab[0] * ac[1] - ab[1] * ac[0])
    dot = sum(cross[j] * (mesh.normals[a][j] + mesh.normals[b][j] + mesh.normals[c][j]) for j in range(3))
    return (c, b, a) if dot < 0 else (a, b, c)


def moby_class(data: bytes, remap: bytes, name: str, sequences: list | None = None) -> MobyClass:
    """Decode a class blob's header, high-LOD packets (low LOD and metal are
    not read), skeleton and sequences: the class's own unless sequences
    lists others, as moby_anim.skeleton takes them."""
    header = span(data, 0, HEADER_SIZE)
    packet_table, = unpack("<i", header)
    high, low, metal, metal_begin, joint_count = header[4:9]
    scale, = unpack("<f", header, 0x24)
    if not math.isfinite(scale) or scale <= 0:
        raise FormatError("invalid moby class scale")
    if not packet_table or not high:
        return MobyClass(scale, None, joint_count)
    if metal and metal_begin != high + low:
        raise FormatError("unexpected moby metal packet position")
    mesh, skins, state = Mesh(name, normals=[]), [], ListState()
    for i in range(high):
        try:
            packet(data, span(data, packet_table + i * 16, 16), state, mesh, remap, skins)
        except FormatError as exc:
            raise FormatError(f"packet {i}: {exc}") from exc
    if not mesh.triangles:
        raise FormatError("moby class has no drawn triangles")
    for skin in skins:
        if any(j >= max(joint_count, 1) for j, _ in skin):
            raise FormatError("moby vertex uses a joint past the class's joint count")
    bones = skeleton(data, class_sequences(data) if sequences is None else sequences)
    if bones is not None:
        mesh.skins = skins
    return MobyClass(scale, mesh, joint_count, skins, bones)


def moby_classes(level) -> dict[int, MobyClass | None]:
    """Class number -> decoded class, or None for a class without a blob,
    for every entry of the core index's moby class table. Ratchet (class 0)
    takes the level's ratchet sequences."""
    result = {}
    for entry in level.table(0x18, 32):
        start, class_id = unpack("<ii", entry)
        if class_id in result:
            raise FormatError(f"duplicate moby class {class_id}")
        if not start:
            result[class_id] = None
            continue
        try:
            result[class_id] = moby_class(level.block(start), entry[16:32], f"moby_{class_id}",
                                          ratchet_sequences(level) if class_id == 0 else None)
        except FormatError as exc:
            raise FormatError(f"moby class {class_id}: {exc}") from exc
    return result
