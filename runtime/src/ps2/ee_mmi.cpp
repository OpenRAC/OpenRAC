// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The EE's multimedia instructions: operations on the 128-bit registers as
// vectors of bytes, halfwords, words or doublewords, and the second multiply
// and divide unit (HI1, LO1).
#include <algorithm>
#include <limits>

#include "ee.h"

namespace ps2 {
namespace {

using Reg = Ee::Reg;

template <typename T>
inline T get(const Reg& r, unsigned n) {
    return load<T>(reinterpret_cast<const u8*>(&r) + n * sizeof(T));
}

template <typename T>
inline void put(Reg& r, unsigned n, T v) {
    store<T>(reinterpret_cast<u8*>(&r) + n * sizeof(T), v);
}

// One operation on every element of two registers.
template <typename T, typename F>
inline Reg each(const Reg& a, const Reg& b, F f) {
    Reg out;
    for (unsigned n = 0; n < 16 / sizeof(T); n++) {
        put<T>(out, n, static_cast<T>(f(get<T>(a, n), get<T>(b, n))));
    }
    return out;
}

template <typename T, typename Wide>
inline T saturate(Wide v) {
    return static_cast<T>(
        std::clamp<Wide>(v, std::numeric_limits<T>::min(), std::numeric_limits<T>::max())
    );
}

inline u64 sext32(u32 v) {
    return static_cast<u64>(static_cast<s64>(static_cast<s32>(v)));
}

// Interleave the lower (or upper) halves of two registers, t's element first.
template <typename T>
inline Reg interleave(const Reg& s, const Reg& t, bool upper) {
    Reg out;
    unsigned count = 8 / sizeof(T), base = upper ? count : 0;
    for (unsigned n = 0; n < count; n++) {
        put<T>(out, n * 2, get<T>(t, base + n));
        put<T>(out, n * 2 + 1, get<T>(s, base + n));
    }
    return out;
}

// Keep the even elements of t, then of s.
template <typename T>
inline Reg pack(const Reg& s, const Reg& t) {
    Reg out;
    unsigned count = 8 / sizeof(T);
    for (unsigned n = 0; n < count; n++) {
        put<T>(out, n, get<T>(t, n * 2));
        put<T>(out, count + n, get<T>(s, n * 2));
    }
    return out;
}

template <typename T, unsigned N>
inline Reg shuffle(const Reg& t, const unsigned (&order)[N]) {
    Reg out;
    for (unsigned n = 0; n < N; n++) {
        put<T>(out, n, get<T>(t, order[n]));
    }
    return out;
}

struct Division {
    u32 quotient, remainder;
};

inline Division divide(s32 n, s32 d) {
    if (d == 0) {
        return {n < 0 ? 1u : 0xFFFFFFFFu, static_cast<u32>(n)};
    }
    if (n == INT32_MIN && d == -1) {
        return {0x80000000u, 0};
    }
    return {static_cast<u32>(n / d), static_cast<u32>(n % d)};
}

inline Division divide_unsigned(u32 n, u32 d) {
    if (d == 0) {
        return {0xFFFFFFFFu, n};
    }
    return {n / d, n % d};
}

}  // namespace

void Ee::mmi(u32 op) {
    unsigned rs = (op >> 21) & 31, rt = (op >> 16) & 31, rd = (op >> 11) & 31,
             shift = (op >> 6) & 31;
    u32 s = static_cast<u32>(gpr[rs].lo), t = static_cast<u32>(gpr[rt].lo);
    const Reg T = gpr[rt];
    Reg out;
    bool write = false;

    switch (op & 0x3F) {
        case 0x00: {  // MADD
            s64 sum = static_cast<s64>((hi << 32) | (lo & 0xFFFFFFFFu))
                      + static_cast<s64>(static_cast<s32>(s)) * static_cast<s32>(t);
            lo = sext32(static_cast<u32>(sum));
            hi = sext32(static_cast<u32>(sum >> 32));
            set64(rd, lo);
            break;
        }
        case 0x01: {  // MADDU
            u64 sum = ((hi << 32) | (lo & 0xFFFFFFFFu)) + static_cast<u64>(s) * t;
            lo = sext32(static_cast<u32>(sum));
            hi = sext32(static_cast<u32>(sum >> 32));
            set64(rd, lo);
            break;
        }
        case 0x04: {  // PLZCW: leading bits equal to the sign, less one, for each low word
            auto count = [](u32 v) -> u32 {
                u32 n = 0;
                u32 sign = v >> 31;
                for (int bit = 30; bit >= 0 && ((v >> bit) & 1) == sign; bit--) {
                    n++;
                }
                return n;
            };
            set64(
                rd, count(s) | (static_cast<u64>(count(static_cast<u32>(gpr[rs].lo >> 32))) << 32)
            );
            break;
        }
        case 0x08: mmi0(op); break;
        case 0x09: mmi2(op); break;
        case 0x10: set64(rd, hi1); break;    // MFHI1
        case 0x11: hi1 = gpr[rs].lo; break;  // MTHI1
        case 0x12: set64(rd, lo1); break;    // MFLO1
        case 0x13: lo1 = gpr[rs].lo; break;  // MTLO1
        case 0x18: {                         // MULT1
            s64 product = static_cast<s64>(static_cast<s32>(s)) * static_cast<s32>(t);
            lo1 = sext32(static_cast<u32>(product));
            hi1 = sext32(static_cast<u32>(product >> 32));
            set64(rd, lo1);
            break;
        }
        case 0x19: {  // MULTU1
            u64 product = static_cast<u64>(s) * t;
            lo1 = sext32(static_cast<u32>(product));
            hi1 = sext32(static_cast<u32>(product >> 32));
            set64(rd, lo1);
            break;
        }
        case 0x1A: {  // DIV1
            Division d = divide(static_cast<s32>(s), static_cast<s32>(t));
            lo1 = sext32(d.quotient);
            hi1 = sext32(d.remainder);
            break;
        }
        case 0x1B: {  // DIVU1
            Division d = divide_unsigned(s, t);
            lo1 = sext32(d.quotient);
            hi1 = sext32(d.remainder);
            break;
        }
        case 0x20: {  // MADD1
            s64 sum = static_cast<s64>((hi1 << 32) | (lo1 & 0xFFFFFFFFu))
                      + static_cast<s64>(static_cast<s32>(s)) * static_cast<s32>(t);
            lo1 = sext32(static_cast<u32>(sum));
            hi1 = sext32(static_cast<u32>(sum >> 32));
            set64(rd, lo1);
            break;
        }
        case 0x21: {  // MADDU1
            u64 sum = ((hi1 << 32) | (lo1 & 0xFFFFFFFFu)) + static_cast<u64>(s) * t;
            lo1 = sext32(static_cast<u32>(sum));
            hi1 = sext32(static_cast<u32>(sum >> 32));
            set64(rd, lo1);
            break;
        }
        case 0x28: mmi1(op); break;
        case 0x29: mmi3(op); break;
        case 0x30: {  // PMFHL
            const Reg L{lo, lo1}, H{hi, hi1};
            write = true;
            switch (shift) {
                case 0:  // LW
                    for (unsigned n = 0; n < 2; n++) {
                        put<u32>(out, n * 2, get<u32>(L, n * 2));
                        put<u32>(out, n * 2 + 1, get<u32>(H, n * 2));
                    }
                    break;
                case 1:  // UW
                    for (unsigned n = 0; n < 2; n++) {
                        put<u32>(out, n * 2, get<u32>(L, n * 2 + 1));
                        put<u32>(out, n * 2 + 1, get<u32>(H, n * 2 + 1));
                    }
                    break;
                case 2:  // SLW: each 64-bit product saturated to a word
                    for (unsigned n = 0; n < 2; n++) {
                        s64 v = static_cast<s64>(
                            (static_cast<u64>(get<u32>(H, n * 2)) << 32) | get<u32>(L, n * 2)
                        );
                        put<u64>(out, n, sext32(static_cast<u32>(saturate<s32, s64>(v))));
                    }
                    break;
                case 3:  // LH
                    for (unsigned n = 0; n < 2; n++) {
                        put<u16>(out, n * 4 + 0, get<u16>(L, n * 4 + 0));
                        put<u16>(out, n * 4 + 1, get<u16>(L, n * 4 + 2));
                        put<u16>(out, n * 4 + 2, get<u16>(H, n * 4 + 0));
                        put<u16>(out, n * 4 + 3, get<u16>(H, n * 4 + 2));
                    }
                    break;
                case 4:  // SH: words saturated to halfwords
                    for (unsigned n = 0; n < 2; n++) {
                        put<s16>(out, n * 4 + 0, saturate<s16, s32>(get<s32>(L, n * 2 + 0)));
                        put<s16>(out, n * 4 + 1, saturate<s16, s32>(get<s32>(L, n * 2 + 1)));
                        put<s16>(out, n * 4 + 2, saturate<s16, s32>(get<s32>(H, n * 2 + 0)));
                        put<s16>(out, n * 4 + 3, saturate<s16, s32>(get<s32>(H, n * 2 + 1)));
                    }
                    break;
                default:
                    write = false;
                    not_known(op, pc - 4);
                    break;
            }
            break;
        }
        case 0x31: {  // PMTHL (LW)
            Reg L{lo, lo1}, H{hi, hi1};
            const Reg S = gpr[rs];
            put<u32>(L, 0, get<u32>(S, 0));
            put<u32>(H, 0, get<u32>(S, 1));
            put<u32>(L, 2, get<u32>(S, 2));
            put<u32>(H, 2, get<u32>(S, 3));
            lo = L.lo;
            lo1 = L.hi;
            hi = H.lo;
            hi1 = H.hi;
            break;
        }
        case 0x34:
            out = each<u16>(T, T, [&](u16 v, u16) { return v << (shift & 15); });
            write = true;
            break;  // PSLLH
        case 0x36:
            out = each<u16>(T, T, [&](u16 v, u16) { return v >> (shift & 15); });
            write = true;
            break;  // PSRLH
        case 0x37:
            out = each<s16>(T, T, [&](s16 v, s16) { return v >> (shift & 15); });
            write = true;
            break;  // PSRAH
        case 0x3C:
            out = each<u32>(T, T, [&](u32 v, u32) { return v << shift; });
            write = true;
            break;  // PSLLW
        case 0x3E:
            out = each<u32>(T, T, [&](u32 v, u32) { return v >> shift; });
            write = true;
            break;  // PSRLW
        case 0x3F:
            out = each<s32>(T, T, [&](s32 v, s32) { return v >> shift; });
            write = true;
            break;  // PSRAW
        default: not_known(op, pc - 4); break;
    }
    if (write && rd) {
        gpr[rd] = out;
    }
}

void Ee::mmi0(u32 op) {
    unsigned rd = (op >> 11) & 31;
    const Reg S = gpr[(op >> 21) & 31], T = gpr[(op >> 16) & 31];
    Reg out;
    switch ((op >> 6) & 31) {
        case 0x00: out = each<u32>(S, T, [](u32 a, u32 b) { return a + b; }); break;  // PADDW
        case 0x01: out = each<u32>(S, T, [](u32 a, u32 b) { return a - b; }); break;  // PSUBW
        case 0x02:
            out = each<s32>(S, T, [](s32 a, s32 b) { return a > b ? -1 : 0; });
            break;  // PCGTW
        case 0x03:
            out = each<s32>(S, T, [](s32 a, s32 b) { return std::max(a, b); });
            break;                                                                    // PMAXW
        case 0x04: out = each<u16>(S, T, [](u16 a, u16 b) { return a + b; }); break;  // PADDH
        case 0x05: out = each<u16>(S, T, [](u16 a, u16 b) { return a - b; }); break;  // PSUBH
        case 0x06:
            out = each<s16>(S, T, [](s16 a, s16 b) { return a > b ? -1 : 0; });
            break;  // PCGTH
        case 0x07:
            out = each<s16>(S, T, [](s16 a, s16 b) { return std::max(a, b); });
            break;                                                                          // PMAXH
        case 0x08: out = each<u8>(S, T, [](u8 a, u8 b) { return a + b; }); break;           // PADDB
        case 0x09: out = each<u8>(S, T, [](u8 a, u8 b) { return a - b; }); break;           // PSUBB
        case 0x0A: out = each<s8>(S, T, [](s8 a, s8 b) { return a > b ? -1 : 0; }); break;  // PCGTB
        case 0x10:
            out = each<s32>(S, T, [](s32 a, s32 b) { return saturate<s32, s64>(s64{a} + b); });
            break;  // PADDSW
        case 0x11:
            out = each<s32>(S, T, [](s32 a, s32 b) { return saturate<s32, s64>(s64{a} - b); });
            break;                                             // PSUBSW
        case 0x12: out = interleave<u32>(S, T, false); break;  // PEXTLW
        case 0x13: out = pack<u32>(S, T); break;               // PPACW
        case 0x14:
            out = each<s16>(S, T, [](s16 a, s16 b) { return saturate<s16, s32>(a + b); });
            break;  // PADDSH
        case 0x15:
            out = each<s16>(S, T, [](s16 a, s16 b) { return saturate<s16, s32>(a - b); });
            break;                                             // PSUBSH
        case 0x16: out = interleave<u16>(S, T, false); break;  // PEXTLH
        case 0x17: out = pack<u16>(S, T); break;               // PPACH
        case 0x18:
            out = each<s8>(S, T, [](s8 a, s8 b) { return saturate<s8, s32>(a + b); });
            break;  // PADDSB
        case 0x19:
            out = each<s8>(S, T, [](s8 a, s8 b) { return saturate<s8, s32>(a - b); });
            break;                                            // PSUBSB
        case 0x1A: out = interleave<u8>(S, T, false); break;  // PEXTLB
        case 0x1B: out = pack<u8>(S, T); break;               // PPACB
        case 0x1E:  // PEXT5: 1:5:5:5 to 8:8:8:8 in each word
            out = each<u32>(T, T, [](u32 c, u32) {
                return ((c & 0x1F) << 3) | ((c & 0x3E0) << 6) | ((c & 0x7C00) << 9)
                       | ((c & 0x8000) << 16);
            });
            break;
        case 0x1F:  // PPAC5: and back
            out = each<u32>(T, T, [](u32 c, u32) {
                return ((c >> 3) & 0x1F) | ((c >> 6) & 0x3E0) | ((c >> 9) & 0x7C00)
                       | ((c >> 16) & 0x8000);
            });
            break;
        default: not_known(op, pc - 4); return;
    }
    if (rd) {
        gpr[rd] = out;
    }
}

void Ee::mmi1(u32 op) {
    unsigned rd = (op >> 11) & 31;
    const Reg S = gpr[(op >> 21) & 31], T = gpr[(op >> 16) & 31];
    Reg out;
    switch ((op >> 6) & 31) {
        case 0x01:
            out = each<s32>(T, T, [](s32 a, s32) {
                return a == INT32_MIN ? INT32_MAX : (a < 0 ? -a : a);
            });
            break;  // PABSW
        case 0x02:
            out = each<u32>(S, T, [](u32 a, u32 b) { return a == b ? 0xFFFFFFFFu : 0u; });
            break;  // PCEQW
        case 0x03:
            out = each<s32>(S, T, [](s32 a, s32 b) { return std::min(a, b); });
            break;  // PMINW
        case 0x04:  // PADSBH: the low four halfwords subtract, the high four add
            for (unsigned n = 0; n < 8; n++) {
                u16 a = get<u16>(S, n), b = get<u16>(T, n);
                put<u16>(out, n, static_cast<u16>(n < 4 ? a - b : a + b));
            }
            break;
        case 0x05:
            out = each<s16>(T, T, [](s16 a, s16) {
                return a == INT16_MIN ? INT16_MAX : (a < 0 ? -a : a);
            });
            break;  // PABSH
        case 0x06:
            out = each<u16>(S, T, [](u16 a, u16 b) { return a == b ? 0xFFFF : 0; });
            break;  // PCEQH
        case 0x07:
            out = each<s16>(S, T, [](s16 a, s16 b) { return std::min(a, b); });
            break;  // PMINH
        case 0x0A:
            out = each<u8>(S, T, [](u8 a, u8 b) { return a == b ? 0xFF : 0; });
            break;  // PCEQB
        case 0x10:
            out = each<u32>(S, T, [](u32 a, u32 b) {
                return static_cast<u32>(std::min<u64>(u64{a} + b, 0xFFFFFFFFu));
            });
            break;  // PADDUW
        case 0x11:
            out = each<u32>(S, T, [](u32 a, u32 b) { return a > b ? a - b : 0; });
            break;                                            // PSUBUW
        case 0x12: out = interleave<u32>(S, T, true); break;  // PEXTUW
        case 0x14:
            out = each<u16>(S, T, [](u16 a, u16 b) { return std::min<u32>(u32{a} + b, 0xFFFF); });
            break;  // PADDUH
        case 0x15:
            out = each<u16>(S, T, [](u16 a, u16 b) { return a > b ? a - b : 0; });
            break;                                            // PSUBUH
        case 0x16: out = interleave<u16>(S, T, true); break;  // PEXTUH
        case 0x18:
            out = each<u8>(S, T, [](u8 a, u8 b) { return std::min<u32>(u32{a} + b, 0xFF); });
            break;  // PADDUB
        case 0x19:
            out = each<u8>(S, T, [](u8 a, u8 b) { return a > b ? a - b : 0; });
            break;                                           // PSUBUB
        case 0x1A: out = interleave<u8>(S, T, true); break;  // PEXTUB
        case 0x1B: {  // QFSRV: the 256-bit value rs:rt shifted right by the SA register's bytes
            unsigned bits_right = (sa & 0xF) * 8;
            if (bits_right == 0) {
                out = T;
            } else if (bits_right < 64) {
                out.lo = (T.lo >> bits_right) | (T.hi << (64 - bits_right));
                out.hi = (T.hi >> bits_right) | (S.lo << (64 - bits_right));
            } else {
                unsigned n = bits_right - 64;
                out.lo = n ? (T.hi >> n) | (S.lo << (64 - n)) : T.hi;
                out.hi = n ? (S.lo >> n) | (S.hi << (64 - n)) : S.lo;
            }
            break;
        }
        default: not_known(op, pc - 4); return;
    }
    if (rd) {
        gpr[rd] = out;
    }
}

void Ee::mmi2(u32 op) {
    unsigned rd = (op >> 11) & 31;
    const Reg S = gpr[(op >> 21) & 31], T = gpr[(op >> 16) & 31];
    Reg out;
    bool write = true;
    switch ((op >> 6) & 31) {
        case 0x00:    // PMADDW
        case 0x04: {  // PMSUBW
            bool subtract = ((op >> 6) & 31) == 0x04;
            u64* his[2] = {&hi, &hi1};
            u64* los[2] = {&lo, &lo1};
            for (unsigned n = 0; n < 2; n++) {
                s64 product = static_cast<s64>(get<s32>(S, n * 2)) * get<s32>(T, n * 2);
                s64 base = static_cast<s64>((*his[n] << 32) | (*los[n] & 0xFFFFFFFFu));
                s64 sum = subtract ? base - product : base + product;
                *los[n] = sext32(static_cast<u32>(sum));
                *his[n] = sext32(static_cast<u32>(sum >> 32));
                put<u64>(out, n, static_cast<u64>(sum));
            }
            break;
        }
        case 0x02:  // PSLLVW
            out.lo = sext32(get<u32>(T, 0) << (get<u32>(S, 0) & 31));
            out.hi = sext32(get<u32>(T, 2) << (get<u32>(S, 2) & 31));
            break;
        case 0x03:  // PSRLVW
            out.lo = sext32(get<u32>(T, 0) >> (get<u32>(S, 0) & 31));
            out.hi = sext32(get<u32>(T, 2) >> (get<u32>(S, 2) & 31));
            break;
        case 0x08: out = Reg{hi, hi1}; break;  // PMFHI
        case 0x09: out = Reg{lo, lo1}; break;  // PMFLO
        case 0x0A:                             // PINTH: t's low halfwords with s's high ones
            for (unsigned n = 0; n < 4; n++) {
                put<u16>(out, n * 2, get<u16>(T, n));
                put<u16>(out, n * 2 + 1, get<u16>(S, 4 + n));
            }
            break;
        case 0x0C: {  // PMULTW
            u64* his[2] = {&hi, &hi1};
            u64* los[2] = {&lo, &lo1};
            for (unsigned n = 0; n < 2; n++) {
                s64 product = static_cast<s64>(get<s32>(S, n * 2)) * get<s32>(T, n * 2);
                *los[n] = sext32(static_cast<u32>(product));
                *his[n] = sext32(static_cast<u32>(product >> 32));
                put<u64>(out, n, static_cast<u64>(product));
            }
            break;
        }
        case 0x0D: {  // PDIVW
            Division a = divide(get<s32>(S, 0), get<s32>(T, 0)),
                     b = divide(get<s32>(S, 2), get<s32>(T, 2));
            lo = sext32(a.quotient);
            hi = sext32(a.remainder);
            lo1 = sext32(b.quotient);
            hi1 = sext32(b.remainder);
            write = false;
            break;
        }
        case 0x0E:  // PCPYLD
            out.lo = T.lo;
            out.hi = S.lo;
            break;
        case 0x10:    // PMADDH
        case 0x14:    // PMSUBH
        case 0x1C: {  // PMULTH
            unsigned kind = (op >> 6) & 31;
            Reg L{lo, lo1}, H{hi, hi1};
            // Products 0, 1 go to LO's words, 2, 3 to HI's, 4, 5 to LO1's, 6, 7 to HI1's.
            for (unsigned n = 0; n < 8; n++) {
                s32 product = static_cast<s32>(get<s16>(S, n)) * get<s16>(T, n);
                Reg& where = (n & 2) ? H : L;
                unsigned word = (n & 4 ? 2 : 0) + (n & 1);
                s32 old = get<s32>(where, word);
                s32 value =
                    kind == 0x1C
                        ? product
                        : static_cast<s32>(
                              kind == 0x10 ? static_cast<u32>(old) + static_cast<u32>(product)
                                           : static_cast<u32>(old) - static_cast<u32>(product)
                          );
                put<s32>(where, word, value);
                if ((n & 1) == 0) {
                    put<s32>(out, n / 2, value);
                }
            }
            lo = L.lo;
            lo1 = L.hi;
            hi = H.lo;
            hi1 = H.hi;
            break;
        }
        case 0x11:    // PHMADH
        case 0x15: {  // PHMSBH
            bool subtract = ((op >> 6) & 31) == 0x15;
            for (unsigned n = 0; n < 4; n++) {
                s32 odd = static_cast<s32>(get<s16>(S, n * 2 + 1)) * get<s16>(T, n * 2 + 1);
                s32 even = static_cast<s32>(get<s16>(S, n * 2)) * get<s16>(T, n * 2);
                put<s32>(
                    out,
                    n,
                    static_cast<s32>(
                        subtract ? static_cast<u32>(odd) - static_cast<u32>(even)
                                 : static_cast<u32>(odd) + static_cast<u32>(even)
                    )
                );
            }
            lo = (lo & 0xFFFFFFFF00000000ull) | get<u32>(out, 0);
            hi = (hi & 0xFFFFFFFF00000000ull) | get<u32>(out, 1);
            lo1 = (lo1 & 0xFFFFFFFF00000000ull) | get<u32>(out, 2);
            hi1 = (hi1 & 0xFFFFFFFF00000000ull) | get<u32>(out, 3);
            break;
        }
        case 0x12: out = Reg{S.lo & T.lo, S.hi & T.hi}; break;  // PAND
        case 0x13: out = Reg{S.lo ^ T.lo, S.hi ^ T.hi}; break;  // PXOR
        case 0x1A: {                                            // PEXEH
            static constexpr unsigned order[8] = {2, 1, 0, 3, 6, 5, 4, 7};
            out = shuffle<u16>(T, order);
            break;
        }
        case 0x1B: {  // PREVH
            static constexpr unsigned order[8] = {3, 2, 1, 0, 7, 6, 5, 4};
            out = shuffle<u16>(T, order);
            break;
        }
        case 0x1D: {  // PDIVBW: every word of s by t's first halfword
            s32 d = get<s16>(T, 0);
            Reg L, H;
            for (unsigned n = 0; n < 4; n++) {
                Division q = divide(get<s32>(S, n), d);
                put<u32>(L, n, q.quotient);
                put<u32>(H, n, static_cast<u32>(static_cast<s32>(static_cast<s16>(q.remainder))));
            }
            lo = L.lo;
            lo1 = L.hi;
            hi = H.lo;
            hi1 = H.hi;
            write = false;
            break;
        }
        case 0x1E: {  // PEXEW
            static constexpr unsigned order[4] = {2, 1, 0, 3};
            out = shuffle<u32>(T, order);
            break;
        }
        case 0x1F: {  // PROT3W
            static constexpr unsigned order[4] = {1, 2, 0, 3};
            out = shuffle<u32>(T, order);
            break;
        }
        default: not_known(op, pc - 4); return;
    }
    if (write && rd) {
        gpr[rd] = out;
    }
}

void Ee::mmi3(u32 op) {
    unsigned rd = (op >> 11) & 31;
    const Reg S = gpr[(op >> 21) & 31], T = gpr[(op >> 16) & 31];
    Reg out;
    bool write = true;
    switch ((op >> 6) & 31) {
        case 0x00:    // PMADDUW
        case 0x0C: {  // PMULTUW
            bool add = ((op >> 6) & 31) == 0x00;
            u64* his[2] = {&hi, &hi1};
            u64* los[2] = {&lo, &lo1};
            for (unsigned n = 0; n < 2; n++) {
                u64 product = static_cast<u64>(get<u32>(S, n * 2)) * get<u32>(T, n * 2);
                if (add) {
                    product += (*his[n] << 32) | (*los[n] & 0xFFFFFFFFu);
                }
                *los[n] = sext32(static_cast<u32>(product));
                *his[n] = sext32(static_cast<u32>(product >> 32));
                put<u64>(out, n, product);
            }
            break;
        }
        case 0x03:  // PSRAVW
            out.lo = sext32(static_cast<u32>(get<s32>(T, 0) >> (get<u32>(S, 0) & 31)));
            out.hi = sext32(static_cast<u32>(get<s32>(T, 2) >> (get<u32>(S, 2) & 31)));
            break;
        case 0x08:  // PMTHI
            hi = S.lo;
            hi1 = S.hi;
            write = false;
            break;
        case 0x09:  // PMTLO
            lo = S.lo;
            lo1 = S.hi;
            write = false;
            break;
        case 0x0A:  // PINTEH: the even halfwords of t and s, interleaved
            for (unsigned n = 0; n < 4; n++) {
                put<u16>(out, n * 2, get<u16>(T, n * 2));
                put<u16>(out, n * 2 + 1, get<u16>(S, n * 2));
            }
            break;
        case 0x0D: {  // PDIVUW
            Division a = divide_unsigned(get<u32>(S, 0), get<u32>(T, 0)),
                     b = divide_unsigned(get<u32>(S, 2), get<u32>(T, 2));
            lo = sext32(a.quotient);
            hi = sext32(a.remainder);
            lo1 = sext32(b.quotient);
            hi1 = sext32(b.remainder);
            write = false;
            break;
        }
        case 0x0E:  // PCPYUD
            out.lo = S.hi;
            out.hi = T.hi;
            break;
        case 0x12: out = Reg{S.lo | T.lo, S.hi | T.hi}; break;        // POR
        case 0x13: out = Reg{~(S.lo | T.lo), ~(S.hi | T.hi)}; break;  // PNOR
        case 0x1A: {                                                  // PEXCH
            static constexpr unsigned order[8] = {0, 2, 1, 3, 4, 6, 5, 7};
            out = shuffle<u16>(T, order);
            break;
        }
        case 0x1B:  // PCPYH: the first halfword of each half, four times
            for (unsigned n = 0; n < 8; n++) {
                put<u16>(out, n, get<u16>(T, n & 4));
            }
            break;
        case 0x1E: {  // PEXCW
            static constexpr unsigned order[4] = {0, 2, 1, 3};
            out = shuffle<u32>(T, order);
            break;
        }
        default: not_known(op, pc - 4); return;
    }
    if (write && rd) {
        gpr[rd] = out;
    }
}

}  // namespace ps2
