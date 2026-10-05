"""
R5900 code helpers shared by OpenRAC's tools: finding function boundaries in a
block of code, and fingerprints that say when two functions are the same code
at different addresses. Standard library only.

Lifted from games/rac1/pal/tools/overlays.py (rac1-decomp, MIT), where the
same rules pair 99.7% of the US build's code with the PAL build's. Nothing
here knows about one game.
"""
import hashlib
import struct
from pathlib import Path

# Opcodes whose 16-bit immediate is an offset from a base register (loads, stores, cache, lwc1...).
MEM = {0x1A, 0x1B, 0x1E, 0x1F, *range(0x20, 0x30), 0x31, 0x36, 0x37, 0x39, 0x3E, 0x3F}


def words(b: bytes) -> tuple:
    return struct.unpack(f"<{len(b) // 4}I", b[:len(b) // 4 * 4])


def trim(b: bytes) -> bytes:
    """B without the zero padding after it."""
    while b.endswith(b"\0\0\0\0"):
        b = b[:-4]
    return b


def mask(w: int) -> int:
    """One instruction with every link-dependent field cleared, and with them
    every constant and struct offset: jump targets, `lui` immediates, and the
    16-bit immediate of anything not relative to $sp."""
    op, rs = w >> 26, (w >> 21) & 31
    if op in (2, 3):                                           # j, jal: target
        return w & 0xFC000000
    if op == 0x0F or rs == 28 and op >= 8:                     # lui, $gp-relative
        return w & 0xFFFF0000
    if (op in MEM or op in (0x09, 0x0D, 0x19)) and rs != 29:   # %lo halves and other non-stack offsets
        return w & 0xFFFF0000
    return w


def shape(b: bytes) -> str:
    """The loose fingerprint: equal for two functions with the same instructions
    whatever their addresses, constants and struct offsets. Two functions that
    differ only in a number share it, so it finds relatives, not copies."""
    return hashlib.sha1(struct.pack(f"<{len(trim(b)) // 4}I", *map(mask, words(trim(b))))).hexdigest()[:16]


def identity(b: bytes) -> bytes:
    """One function's words with only its address fields masked: what two
    copies must share to be the same function.

    A register is address-derived when a `lui` loads it with the top half of a
    RAM address, or when it is copied or computed from one that is; immediates
    on those (and on $gp) are %lo halves and stay masked. An immediate on
    $zero, a small offset on any other register and an `ori` are the code's own
    and are compared. Large offsets on other registers stay masked: an address
    half can reach one through a spill."""
    w = words(trim(b))
    tainted, grew = set(), True
    while grew:
        grew = False
        for x in w:
            op, rs, rt, rd = x >> 26, (x >> 21) & 31, (x >> 16) & 31, (x >> 11) & 31
            new = None
            if op == 0x0F and 0x10 <= x & 0xFFFF < 0x200:
                new = rt
            elif op in (0x09, 0x19) and rs in tainted:
                new = rt
            elif op == 0 and x & 0x3F in (0x21, 0x2D, 0x25) and (rs in tainted or rt in tainted):
                new = rd
            if new and new not in tainted:
                tainted.add(new)
                grew = True
    out = []
    for x in w:
        op, rs, imm = x >> 26, (x >> 21) & 31, x & 0xFFFF
        if op in (2, 3):
            x &= 0xFC000000
        elif op == 0x0F:
            if 0x10 <= imm < 0x200:
                x &= 0xFFFF0000
        elif rs == 28 and op >= 8:
            x &= 0xFFFF0000
        elif (op in MEM or op in (0x09, 0x0D, 0x19)) and rs != 29:
            small = imm < 0x1000 or imm >= 0xF000
            if rs in tainted or (rs != 0 and op != 0x0D and not small):
                x &= 0xFFFF0000
        out.append(x)
    return struct.pack(f"<{len(out)}I", *out)


def fingerprint(b: bytes) -> str:
    """The strict fingerprint: equal only for the same function at another
    address (identity above)."""
    return hashlib.sha1(identity(b)).hexdigest()[:16]


def ends_in_jump(w: int) -> bool:
    """jr, j or b (beq $0, $0): the last instruction of a function whose delay
    slot follows it."""
    return (w & 0xFC1FFFFF) == 0x00000008 or w >> 26 == 2 or w >> 16 == 0x1000


def code_size(b: bytes) -> int:
    """The function's size: its bytes without the padding after it, but with a
    nop in the delay slot of its final jump, which trim() takes for padding."""
    t = trim(b)
    if len(t) < len(b) and t and ends_in_jump(words(t[-4:])[0]):
        return len(t) + 4
    return len(t)


def split(text: bytes, base: int, extra=()) -> list[tuple[int, int]]:
    """(offset, size) of each function in TEXT loaded at BASE: starts at call
    targets, after returns, after tail calls followed by a frame opener, at a
    frame opener after padding, and at EXTRA word indices. The size includes
    the padding up to the next function; code_size() takes it off."""
    w = words(text)
    starts = {0, *extra}
    for i, x in enumerate(w):
        if x >> 26 == 3:
            t = ((x & 0x3FFFFFF) << 2) | (base & 0xF0000000)
            if base <= t < base + len(text):
                starts.add((t - base) // 4)
        if (x == 0x03E00008 or x >> 26 == 2) and i + 2 < len(w):
            j = i + 2
            while j < len(w) and w[j] == 0:
                j += 1
            if x == 0x03E00008 or (j < len(w) and w[j] >> 16 == 0x27BD and w[j] & 0x8000):
                starts.add(j)
        if x >> 16 == 0x27BD and x & 0x8000 and i and w[i - 1] == 0:
            starts.add(i)
    s = sorted(x for x in starts if x < len(w))
    return [(a * 4, (b - a) * 4) for a, b in zip(s, s[1:] + [len(w)])]


def elf_sections(path: Path) -> dict[str, tuple[int, bytes, int]]:
    """name -> (address, bytes, flags) of the sections an ELF file stores (not .bss)."""
    b = Path(path).read_bytes()
    if b[:4] != b"\x7fELF":
        raise ValueError(f"{path}: not an ELF file")
    shoff, = struct.unpack_from("<I", b, 0x20)
    entsize, count, names_at = struct.unpack_from("<3H", b, 0x2E)
    if not count:
        return {}
    strtab = struct.unpack_from("<6I", b, shoff + names_at * entsize)[4]
    out = {}
    for i in range(count):
        name, kind, flags, addr, off, size = struct.unpack_from("<6I", b, shoff + i * entsize)
        if kind == 1 and size:                                  # SHT_PROGBITS
            out[b[strtab + name:b.index(b"\0", strtab + name)].decode()] = (addr, b[off:off + size], flags)
    return out
