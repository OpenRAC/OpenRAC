// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <cstdarg>
#include <cstdio>
#include <string>

#include "types.h"

// A disassembler for vector unit instruction pairs, for reading a program
// while working on the interpreter. Output is for the terminal; listings of
// a game's programs are game code and are not kept in the repository.
namespace ps2::vudis {

inline std::string fields(u32 dest) {
    std::string s;
    if (dest & 8) {
        s += 'x';
    }
    if (dest & 4) {
        s += 'y';
    }
    if (dest & 2) {
        s += 'z';
    }
    if (dest & 1) {
        s += 'w';
    }
    return s;
}

inline std::string text(const char* format, ...) __attribute__((format(printf, 1, 2)));

inline std::string text(const char* format, ...) {
    char buffer[96];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    return buffer;
}

inline std::string upper(u32 code) {
    static const char* const bc = "xyzw";
    u32 dest = (code >> 21) & 0xF, ft = (code >> 16) & 31, fs = (code >> 11) & 31,
        fd = (code >> 6) & 31, fn = code & 0x3F;
    std::string d = fields(dest);
    auto three = [&](const char* name) {
        return text("%s.%s vf%u, vf%u, vf%u", name, d.c_str(), fd, fs, ft);
    };
    auto with_bc = [&](const char* name) {
        return text("%s%c.%s vf%u, vf%u, vf%u", name, bc[fn & 3], d.c_str(), fd, fs, ft);
    };
    auto with = [&](const char* name) {
        return text("%s.%s vf%u, vf%u", name, d.c_str(), fd, fs);
    };
    static const char* const group[7] = {"add", "sub", "madd", "msub", "max", "mini", "mul"};
    if (fn < 0x1C) {
        return with_bc(group[fn >> 2]);
    }
    switch (fn) {
        case 0x1C: return with("mulq");
        case 0x1D: return with("maxi");
        case 0x1E: return with("muli");
        case 0x1F: return with("minii");
        case 0x20: return with("addq");
        case 0x21: return with("maddq");
        case 0x22: return with("addi");
        case 0x23: return with("maddi");
        case 0x24: return with("subq");
        case 0x25: return with("msubq");
        case 0x26: return with("subi");
        case 0x27: return with("msubi");
        case 0x28: return three("add");
        case 0x29: return three("madd");
        case 0x2A: return three("mul");
        case 0x2B: return three("max");
        case 0x2C: return three("sub");
        case 0x2D: return three("msub");
        case 0x2E: return three("opmsub");
        case 0x2F: return three("mini");
        default: break;
    }
    if (fn < 0x3C) {
        return text("?upper %08x", code);
    }
    u32 index = (((code >> 6) & 0x1F) << 2) | (code & 3);
    auto acc3 = [&](const char* name) {
        return text("%s.%s acc, vf%u, vf%u", name, d.c_str(), fs, ft);
    };
    auto acc_bc = [&](const char* name) {
        return text("%s%c.%s acc, vf%u, vf%u", name, bc[index & 3], d.c_str(), fs, ft);
    };
    auto acc1 = [&](const char* name) {
        return text("%s.%s acc, vf%u", name, d.c_str(), fs);
    };
    auto convert = [&](const char* name) {
        return text("%s.%s vf%u, vf%u", name, d.c_str(), ft, fs);
    };
    static const char* const agroup[4] = {"adda", "suba", "madda", "msuba"};
    static const char* const scale[4] = {"0", "4", "12", "15"};
    if (index < 0x10) {
        return acc_bc(agroup[index >> 2]);
    }
    if (index < 0x14) {
        return convert((std::string("itof") + scale[index & 3]).c_str());
    }
    if (index < 0x18) {
        return convert((std::string("ftoi") + scale[index & 3]).c_str());
    }
    if (index < 0x1C) {
        return acc_bc("mula");
    }
    switch (index) {
        case 0x1C: return acc1("mulaq");
        case 0x1D: return convert("abs");
        case 0x1E: return acc1("mulai");
        case 0x1F: return text("clip vf%u, vf%u", fs, ft);
        case 0x20: return acc1("addaq");
        case 0x21: return acc1("maddaq");
        case 0x22: return acc1("addai");
        case 0x23: return acc1("maddai");
        case 0x24: return acc1("subaq");
        case 0x25: return acc1("msubaq");
        case 0x26: return acc1("subai");
        case 0x27: return acc1("msubai");
        case 0x28: return acc3("adda");
        case 0x29: return acc3("madda");
        case 0x2A: return acc3("mula");
        case 0x2C: return acc3("suba");
        case 0x2D: return acc3("msuba");
        case 0x2E: return acc3("opmula");
        case 0x2F: return "nop";
        default: return text("?upper %08x", code);
    }
}

inline std::string lower(u32 code, u32 at) {
    static const char* const bc = "xyzw";
    u32 op = code >> 25, dest = (code >> 21) & 0xF, it = (code >> 16) & 31, is = (code >> 11) & 31,
        id = (code >> 6) & 31;
    s32 imm11 = static_cast<s32>(((code & 0x7FF) ^ 0x400) - 0x400);
    u32 imm15 = ((code >> 10) & 0x7800) | (code & 0x7FF),
        imm12 = ((code >> 10) & 0x800) | (code & 0x7FF);
    u32 target = at + 1 + static_cast<u32>(imm11);
    std::string d = fields(dest);
    switch (op) {
        case 0x00: return text("lq.%s vf%u, %d(vi%u)", d.c_str(), it, imm11, is & 15);
        case 0x01: return text("sq.%s vf%u, %d(vi%u)", d.c_str(), is, imm11, it & 15);
        case 0x04: return text("ilw.%s vi%u, %d(vi%u)", d.c_str(), it & 15, imm11, is & 15);
        case 0x05: return text("isw.%s vi%u, %d(vi%u)", d.c_str(), it & 15, imm11, is & 15);
        case 0x08: return text("iaddiu vi%u, vi%u, %u", it & 15, is & 15, imm15);
        case 0x09: return text("isubiu vi%u, vi%u, %u", it & 15, is & 15, imm15);
        case 0x10: return text("fceq 0x%06x", code & 0xFFFFFF);
        case 0x11: return text("fcset 0x%06x", code & 0xFFFFFF);
        case 0x12: return text("fcand 0x%06x", code & 0xFFFFFF);
        case 0x13: return text("fcor 0x%06x", code & 0xFFFFFF);
        case 0x14: return text("fseq vi%u, 0x%03x", it & 15, imm12);
        case 0x15: return text("fsset 0x%03x", imm12);
        case 0x16: return text("fsand vi%u, 0x%03x", it & 15, imm12);
        case 0x17: return text("fsor vi%u, 0x%03x", it & 15, imm12);
        case 0x18: return text("fmeq vi%u, vi%u", it & 15, is & 15);
        case 0x1A: return text("fmand vi%u, vi%u", it & 15, is & 15);
        case 0x1B: return text("fmor vi%u, vi%u", it & 15, is & 15);
        case 0x1C: return text("fcget vi%u", it & 15);
        case 0x20: return text("b %u", target);
        case 0x21: return text("bal vi%u, %u", it & 15, target);
        case 0x24: return text("jr vi%u", is & 15);
        case 0x25: return text("jalr vi%u, vi%u", it & 15, is & 15);
        case 0x28: return text("ibeq vi%u, vi%u, %u", it & 15, is & 15, target);
        case 0x29: return text("ibne vi%u, vi%u, %u", it & 15, is & 15, target);
        case 0x2C: return text("ibltz vi%u, %u", is & 15, target);
        case 0x2D: return text("ibgtz vi%u, %u", is & 15, target);
        case 0x2E: return text("iblez vi%u, %u", is & 15, target);
        case 0x2F: return text("ibgez vi%u, %u", is & 15, target);
        case 0x40: break;
        default: return text("?lower %08x", code);
    }
    u32 fn = code & 0x3F;
    if (fn < 0x3C) {
        switch (fn) {
            case 0x30: return text("iadd vi%u, vi%u, vi%u", id & 15, is & 15, it & 15);
            case 0x31: return text("isub vi%u, vi%u, vi%u", id & 15, is & 15, it & 15);
            case 0x32:
                return text(
                    "iaddi vi%u, vi%u, %d", it & 15, is & 15, static_cast<s32>((id ^ 0x10) - 0x10)
                );
            case 0x34: return text("iand vi%u, vi%u, vi%u", id & 15, is & 15, it & 15);
            case 0x35: return text("ior vi%u, vi%u, vi%u", id & 15, is & 15, it & 15);
            default: return text("?lower %08x", code);
        }
    }
    u32 fsf = (code >> 21) & 3, ftf = (code >> 23) & 3;
    switch ((((code >> 6) & 0x1F) << 2) | (code & 3)) {
        case 0x30:
            return dest == 0 && it == 0 && is == 0 ? std::string("nop")
                                                   : text("move.%s vf%u, vf%u", d.c_str(), it, is);
        case 0x31: return text("mr32.%s vf%u, vf%u", d.c_str(), it, is);
        case 0x34: return text("lqi.%s vf%u, (vi%u++)", d.c_str(), it, is & 15);
        case 0x35: return text("sqi.%s vf%u, (vi%u++)", d.c_str(), is, it & 15);
        case 0x36: return text("lqd.%s vf%u, (--vi%u)", d.c_str(), it, is & 15);
        case 0x37: return text("sqd.%s vf%u, (--vi%u)", d.c_str(), is, it & 15);
        case 0x38: return text("div q, vf%u%c, vf%u%c", is, bc[fsf], it, bc[ftf]);
        case 0x39: return text("sqrt q, vf%u%c", it, bc[ftf]);
        case 0x3A: return text("rsqrt q, vf%u%c, vf%u%c", is, bc[fsf], it, bc[ftf]);
        case 0x3B: return "waitq";
        case 0x3C: return text("mtir vi%u, vf%u%c", it & 15, is, bc[fsf]);
        case 0x3D: return text("mfir.%s vf%u, vi%u", d.c_str(), it, is & 15);
        case 0x3E: return text("ilwr.%s vi%u, (vi%u)", d.c_str(), it & 15, is & 15);
        case 0x3F: return text("iswr.%s vi%u, (vi%u)", d.c_str(), it & 15, is & 15);
        case 0x40: return text("rnext.%s vf%u, r", d.c_str(), it);
        case 0x41: return text("rget.%s vf%u, r", d.c_str(), it);
        case 0x42: return text("rinit r, vf%u%c", is, bc[fsf]);
        case 0x43: return text("rxor r, vf%u%c", is, bc[fsf]);
        case 0x64: return text("mfp.%s vf%u, p", d.c_str(), it);
        case 0x68: return text("xtop vi%u", it & 15);
        case 0x69: return text("xitop vi%u", it & 15);
        case 0x6C: return text("xgkick vi%u", is & 15);
        case 0x70: return text("esadd p, vf%u", is);
        case 0x71: return text("ersadd p, vf%u", is);
        case 0x72: return text("eleng p, vf%u", is);
        case 0x73: return text("erleng p, vf%u", is);
        case 0x74: return text("eatanxy p, vf%u", is);
        case 0x75: return text("eatanxz p, vf%u", is);
        case 0x76: return text("esum p, vf%u", is);
        case 0x78: return text("esqrt p, vf%u%c", is, bc[fsf]);
        case 0x79: return text("ersqrt p, vf%u%c", is, bc[fsf]);
        case 0x7A: return text("ercpr p, vf%u%c", is, bc[fsf]);
        case 0x7B: return "waitp";
        case 0x7C: return text("esin p, vf%u%c", is, bc[fsf]);
        case 0x7D: return text("eatan p, vf%u%c", is, bc[fsf]);
        case 0x7E: return text("eexp p, vf%u%c", is, bc[fsf]);
        default: return text("?lower %08x", code);
    }
}

// One pair as a line: the address, the lower instruction (or the number for
// I), the upper one and its end-of-program mark.
inline std::string pair(u32 at, u32 upper_word, u32 lower_word) {
    std::string low = (upper_word & 0x80000000u)
                          ? text("loi %g", static_cast<double>(as_float(lower_word)))
                          : lower(lower_word, at);
    return text(
        "%4u: %-28s | %s%s%s",
        at,
        low.c_str(),
        upper(upper_word).c_str(),
        (upper_word & 0x40000000u) ? "  [E]" : "",
        (upper_word & 0x20000000u) ? "  [M]" : ""
    );
}

}  // namespace ps2::vudis
