// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// openrac-gsdemo: draws a scene of its own through the same path a game's
// frame takes. It writes a display list into guest memory as a VIF1 DMA
// chain (CNT and REF tags carrying VIF codes, GIF packets sent with DIRECT,
// a texture and its colour table sent as image data, a VU1 microprogram and
// the vertices it transforms), runs the chain through the DMA walker, VIF1,
// VU1, the GIF and the software GS, and shows what the GS's display circuit
// reads. Nothing here comes from a game: the microprogram is written below.

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <string>
#include <vector>

#include "ps2/dma.h"
#include "ps2/graphics.h"
#include "ps2/memory.h"
#include "ps2/vu_asm.h"

#ifndef OPENRAC_NO_WINDOW
#include "host/window.h"
#endif

using namespace ps2;

namespace {

constexpr int kWidth = 512, kHeight = 448;
constexpr u32 kFramePage = 0;  // 512 x 448 x 32 bits is 112 pages
constexpr u32 kZPage = 112;
constexpr u32 kTextureBlock = 224 * 32;  // a 64 x 64 8-bit texture: 16 blocks
constexpr u32 kClutBlock = kTextureBlock + 16;
constexpr u32 kOffsetX = 2048 - kWidth / 2, kOffsetY = 2048 - kHeight / 2;

constexpr u32 kListAddress = 0x00400000;
constexpr u32 kUploadAddress = 0x00500000;
constexpr u32 kProgramAddress = 0x00520000;

// VIF codes.
constexpr u32 kVifNop = 0, kVifStcycl = 0x01000000, kVifMscal = 0x14000000, kVifMpg = 0x4A000000,
              kVifDirect = 0x50000000, kVifUnpackV4_32 = 0x6C000000;

// PRIM: the primitive type and its attribute bits.
constexpr u32 kTriangle = 3, kTriangleStrip = 4, kSprite = 6;
constexpr u32 kGouraud = 1 << 3, kTextured = 1 << 4, kFogged = 1 << 5, kBlended = 1 << 6,
              kUv = 1 << 8;

// A display list in guest memory, written the way a game writes one.
class List {
public:
    List(GuestMemory& memory, u32 address) : memory_(memory), at_(address) {}

    u32 address() const { return at_; }

    void quad(u64 lo, u64 hi) {
        memory_.write<u64>(at_, lo);
        memory_.write<u64>(at_ + 8, hi);
        at_ += 16;
    }

    // A CNT tag whose two spare words are VIF codes: NOP, then DIRECT for the
    // quadwords that follow. The count is filled in by end_direct().
    void begin_direct() {
        open_ = at_;
        quad(0, 0);
    }

    void end_direct(bool irq = false) {
        u32 quadwords = (at_ - open_) / 16 - 1;
        u64 tag = quadwords | (u64{dmatag::CNT} << 28) | (u64{irq} << 31);
        u64 vif = u64{kVifDirect | quadwords} << 32;
        memory_.write<u64>(open_, tag);
        memory_.write<u64>(open_ + 8, vif);
    }

    // A REF tag: the quadwords are elsewhere in memory.
    void ref_direct(u32 address, u32 quadwords) {
        quad(
            quadwords | (u64{dmatag::REF} << 28) | (u64{address} << 32),
            u64{kVifDirect | quadwords} << 32
        );
    }

    void end() { quad(u64{dmatag::END} << 28, 0); }

    // A CNT tag with any two VIF codes in its spare words, for data that is
    // not a GIF packet. The count is filled in by end_data().
    void begin_data(u32 vif0, u32 vif1) {
        open_ = at_;
        quad(0, vif0 | (u64{vif1} << 32));
    }

    void end_data() {
        u32 quadwords = (at_ - open_) / 16 - 1;
        memory_.write<u64>(open_, quadwords | (u64{dmatag::CNT} << 28));
    }

    // A REF tag with any two VIF codes.
    void ref(u32 address, u32 quadwords, u32 vif0, u32 vif1) {
        quad(quadwords | (u64{dmatag::REF} << 28) | (u64{address} << 32), vif0 | (u64{vif1} << 32));
    }

    void floats(float x, float y, float z, float w) {
        quad(as_u32(x) | (u64{as_u32(y)} << 32), as_u32(z) | (u64{as_u32(w)} << 32));
    }

    void words(u32 x, u32 y, u32 z, u32 w) { quad(x | (u64{y} << 32), z | (u64{w} << 32)); }

    void gif_tag(
        u32 loops, bool eop, u32 flg, std::initializer_list<u8> regs, bool pre = false, u32 prim = 0
    ) {
        u64 lo = loops | (u64{eop} << 15) | (u64{pre} << 46) | (u64{prim} << 47) | (u64{flg} << 58)
                 | (u64{regs.size() & 0xF} << 60);
        u64 hi = 0;
        unsigned n = 0;
        for (u8 r : regs) {
            hi |= u64{r} << (n++ * 4);
        }
        quad(lo, hi);
    }

    void ad(u8 reg, u64 value) { quad(value, reg); }

    // The packed forms of the vertex registers.
    void rgbaq(u32 r, u32 g, u32 b, u32 a) { quad(r | (u64{g} << 32), b | (u64{a} << 32)); }

    void st(float s, float t, float q) { quad(as_u32(s) | (u64{as_u32(t)} << 32), as_u32(q)); }

    void uv(float u, float v) { quad(fixed(u) | (u64{fixed(v)} << 32), 0); }

    void xyz(float x, float y, u32 z) { quad(screen_x(x) | (u64{screen_y(y)} << 32), z); }

    void xyzf(float x, float y, u32 z, u32 fog) {
        quad(screen_x(x) | (u64{screen_y(y)} << 32), (u64{z & 0xFFFFFF} << 4) | (u64{fog} << 36));
    }

private:
    static u32 fixed(float v) { return static_cast<u32>(std::lround(v * 16.0f)); }

    // Coordinates are given from the centre of the screen, which sits at the
    // centre of the GS coordinate space (2048, 2048); XYOFFSET then puts the
    // buffer's top left corner at (0, 0).
    static u32 screen_x(float x) {
        return static_cast<u32>(std::lround((x + 2048.0f) * 16.0f)) & 0xFFFF;
    }

    static u32 screen_y(float y) {
        return static_cast<u32>(std::lround((y + 2048.0f) * 16.0f)) & 0xFFFF;
    }

    GuestMemory& memory_;
    u32 at_;
    u32 open_ = 0;
};

u64 tex0(u32 tbp, u32 tbw, u32 psm, u32 tw, u32 th, bool tcc, u32 tfx, u32 cbp, u32 cpsm, u32 cld) {
    return tbp | (u64{tbw} << 14) | (u64{psm} << 20) | (u64{tw} << 26) | (u64{th} << 30)
           | (u64{tcc} << 34) | (u64{tfx} << 35) | (u64{cbp} << 37) | (u64{cpsm} << 51)
           | (u64{cld} << 61);
}

// One host to local transfer, as a packet of its own: four register writes,
// then the pixels as image data. Returns its length in quadwords.
u32 write_upload(
    GuestMemory& memory,
    u32 address,
    u32 block,
    u32 width_units,
    u32 psm,
    u32 width,
    u32 height,
    const std::vector<u8>& pixels
) {
    List list(memory, address);
    list.gif_tag(4, false, 0, {0xE});
    list.ad(gsreg::BITBLTBUF, (u64{block} << 32) | (u64{width_units} << 48) | (u64{psm} << 56));
    list.ad(gsreg::TRXPOS, 0);
    list.ad(gsreg::TRXREG, width | (u64{height} << 32));
    list.ad(gsreg::TRXDIR, 0);
    u32 quadwords = static_cast<u32>((pixels.size() + 15) / 16);
    list.gif_tag(quadwords, true, 2, {});
    std::memcpy(memory.ram(list.address()), pixels.data(), pixels.size());
    return (list.address() - address) / 16 + quadwords;
}

struct Uploads {
    u32 texture_address = 0, texture_quadwords = 0;
    u32 clut_address = 0, clut_quadwords = 0;
    u32 program_address = 0, program_quadwords = 0;
};

// Where the microprogram finds its input, from the address XTOP gives it,
// and where it builds its packet.
constexpr u32 kVuMatrix = 0, kVuTag = 4, kVuCount = 5, kVuVertices = 6, kVuOutput = 512;

// The microprogram: for each vertex (texture coordinates, colour, position)
// multiply the position by a 4 x 4 matrix, divide by W, turn X and Y into
// the GS's fixed point and Z into an integer, and write the three quadwords
// of a packed GIF vertex. Then kick the packet.
vuasm::Program vertex_program() {
    using namespace vuasm;
    Program p;
    p.lo(xtop(1));
    p.lo(lq(XYZW, 1, kVuMatrix + 0, 1));
    p.lo(lq(XYZW, 2, kVuMatrix + 1, 1));
    p.lo(lq(XYZW, 3, kVuMatrix + 2, 1));
    p.lo(lq(XYZW, 4, kVuMatrix + 3, 1));
    p.lo(lq(XYZW, 10, kVuTag, 1));
    p.lo(ilw(X, 3, kVuCount, 1));
    p.lo(iaddiu(2, 1, kVuOutput));
    p.lo(sq(XYZW, 10, 0, 2));
    p.lo(iaddiu(5, 2, 1));            // where the next vertex is written
    p.lo(iaddiu(4, 1, kVuVertices));  // where the next vertex is read
    u32 loop = p.here();
    p.lo(lq(XYZW, 20, 2, 4));                            // position
    p.add(mulabc(XYZW, 1, 20, 0), lq(XYZW, 21, 0, 4));   // ACC = column 0 * x; texture coordinates
    p.add(maddabc(XYZW, 2, 20, 1), lq(XYZW, 22, 1, 4));  // + column 1 * y; colour
    p.hi(maddabc(XYZW, 3, 20, 2));                       // + column 2 * z
    p.hi(maddbc(XYZW, 23, 4, 20, 3));                    // + column 3 * w
    p.lo(div(0, 3, 23, 3));                              // Q = 1 / W
    p.lo(waitq());
    p.hi(mulq(XYZ, 23, 23));  // the perspective divide
    p.hi(mulq(XYZ, 21, 21));  // S/W, T/W, 1/W
    p.add(ftoi4(XY, 24, 23), sq(XYZW, 22, 1, 5));
    p.add(ftoi0(Z, 24, 23), sq(XYZ, 21, 0, 5));
    p.lo(iaddiu(4, 4, 3));
    p.lo(iaddi(3, 3, -1));
    p.lo(sq(XYZ, 24, 2, 5));
    p.lo(ibne(3, 0, static_cast<s32>(loop) - static_cast<s32>(p.here() + 1)));
    p.lo(iaddiu(5, 5, 3));  // in the branch's delay slot
    p.lo(xgkick(2));
    p.add(nop() | E, lnop());
    p.hi(nop());
    return p;
}

// A 64 x 64 texture of 8-bit indices (tiles with a ring in each) and its
// table of 256 colours, laid out as the GS expects a table to be stored.
Uploads prepare_uploads(GuestMemory& memory) {
    std::vector<u8> texels(64 * 64);
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            int tile = ((x >> 4) + (y >> 4)) & 1;
            float dx = static_cast<float>((x & 15) - 8) + 0.5f,
                  dy = static_cast<float>((y & 15) - 8) + 0.5f;
            float ring = std::fabs(std::sqrt(dx * dx + dy * dy) - 5.0f);
            int shade = ring < 1.5f ? 15 : static_cast<int>(std::min(14.0f, ring * 2.0f));
            texels[static_cast<std::size_t>(y * 64 + x)] =
                static_cast<u8>(tile * 16 + shade + ((x >> 5) ^ (y >> 5)) * 32);
        }
    }
    std::vector<u8> table(256 * 4);
    for (u32 i = 0; i < 256; i++) {
        u32 shade = i & 15, tile = (i >> 4) & 1, tint = (i >> 5) & 1;
        u32 r = tile ? 40 + shade * 12 : 200 - shade * 8;
        u32 g = tint ? 60 + shade * 10 : 150 - shade * 4;
        u32 b = tile ? 220 - shade * 6 : 50 + shade * 9;
        u32 a = shade == 15 ? 0x40 : 0x80;
        u32 position = (i & 0xE7) | ((i & 0x08) << 1) | ((i & 0x10) >> 1);
        store<u32>(&table[position * 4], r | (g << 8) | (b << 16) | (a << 24));
    }

    Uploads u;
    u.texture_address = kUploadAddress;
    u.texture_quadwords =
        write_upload(memory, u.texture_address, kTextureBlock, 1, PSMT8, 64, 64, texels);
    u.clut_address = kUploadAddress + 0x10000;
    u.clut_quadwords = write_upload(memory, u.clut_address, kClutBlock, 1, PSMCT32, 16, 16, table);

    // The microprogram as a game stores one: the VIF code that loads it, then
    // the program, as whole quadwords ready to be referenced from a list.
    vuasm::Program program = vertex_program();
    std::vector<u32> blob =
        {kVifNop, kVifNop, kVifNop, kVifMpg | (static_cast<u32>(program.words().size() / 2) << 16)};
    blob.insert(blob.end(), program.words().begin(), program.words().end());
    blob.resize((blob.size() + 3) & ~std::size_t{3}, kVifNop);
    std::memcpy(memory.ram(kProgramAddress), blob.data(), blob.size() * 4);
    u.program_address = kProgramAddress;
    u.program_quadwords = static_cast<u32>(blob.size() / 4);
    return u;
}

struct Point {
    float x, y, z;
};

// A pinhole camera at the origin looking down +z, onto the 512 x 448 buffer
// (whose pixels are wider than tall on a 4:3 screen).
struct Projected {
    float x, y, q;
    u32 z;
};

Projected project(Point p) {
    float q = 1.0f / p.z;
    Projected out;
    out.x = p.x * q * 300.0f;
    out.y = -p.y * q * 262.0f;
    out.q = q;
    out.z = static_cast<u32>(std::min(1.0f, q) * 16777215.0f);
    return out;
}

void build_frame(GuestMemory& memory, const Uploads& uploads, int frame) {
    List list(memory, kListAddress);
    float time = static_cast<float>(frame) / 60.0f;

    // Drawing environment: one 32-bit colour buffer, a 24-bit Z buffer.
    list.begin_direct();
    list.gif_tag(11, true, 0, {0xE});
    list.ad(gsreg::PRMODECONT, 1);
    list.ad(gsreg::FRAME_1, kFramePage | (u64{kWidth / 64} << 16) | (u64{PSMCT32} << 24));
    list.ad(gsreg::ZBUF_1, kZPage | (u64{1} << 24));
    list.ad(gsreg::XYOFFSET_1, (kOffsetX * 16) | (u64{kOffsetY * 16} << 32));
    list.ad(gsreg::SCISSOR_1, (u64{kWidth - 1} << 16) | (u64{kHeight - 1} << 48));
    list.ad(gsreg::TEST_1, (u64{1} << 16) | (u64{1} << 17));  // depth test on, always pass
    list.ad(gsreg::COLCLAMP, 1);
    list.ad(gsreg::TEXA, 0x80 | (u64{0x80} << 32));
    list.ad(gsreg::FOGCOL, 0x30 | (0x48 << 8) | (0x70 << 16));
    list.ad(gsreg::ALPHA_1, 0x44);  // (Cs - Cd) * As + Cd
    list.ad(gsreg::CLAMP_1, 0);
    list.end_direct();

    // The texture and its table go up once, by reference to packets built
    // outside the list.
    if (frame == 0) {
        list.ref_direct(uploads.texture_address, uploads.texture_quadwords);
        list.ref_direct(uploads.clut_address, uploads.clut_quadwords);
    }

    // Clear colour and depth with one sprite: sky above, a darker band below.
    list.begin_direct();
    list.gif_tag(2, true, 0, {gsreg::RGBAQ, gsreg::XYZ2, gsreg::XYZ2}, true, kSprite);
    list.rgbaq(0x30, 0x48, 0x70, 0x80);
    list.xyz(-256, -224, 0);
    list.xyz(256, 0, 0);
    list.rgbaq(0x10, 0x18, 0x28, 0x80);
    list.xyz(-256, 0, 0);
    list.xyz(256, 224, 0);
    list.end_direct();

    // A textured floor running to the horizon: perspective-correct ST with Q,
    // bilinear filtering, fog by distance, depth tested.
    list.begin_direct();
    list.gif_tag(3, false, 0, {0xE});
    list.ad(gsreg::TEST_1, (u64{1} << 16) | (u64{2} << 17));  // pass when nearer or equal
    list.ad(gsreg::TEX1_1, (u64{1} << 5) | (u64{1} << 6));    // linear both ways
    list.ad(gsreg::TEX0_1, tex0(kTextureBlock, 1, PSMT8, 6, 6, true, 0, kClutBlock, PSMCT32, 1));
    list.gif_tag(
        4,
        true,
        0,
        {gsreg::ST, gsreg::RGBAQ, gsreg::XYZF2},
        true,
        kTriangleStrip | kGouraud | kTextured | kFogged
    );
    const float scroll = time * 0.5f;
    const Point floor[4] = {{-6, -1, 1.2f}, {6, -1, 1.2f}, {-6, -1, 14}, {6, -1, 14}};
    for (const Point& p : floor) {
        Projected v = project(p);
        list.st(p.x * 0.5f * v.q, (p.z * 0.5f + scroll) * v.q, v.q);
        list.rgbaq(0x80, 0x80, 0x80, 0x80);
        u32 fog = static_cast<u32>(std::max(0.0f, 255.0f - (p.z - 1.2f) * 19.0f));
        list.xyzf(v.x, v.y, v.z, fog);
    }
    list.end_direct();

    // A cube drawn by VU1. The list loads the microprogram once (by reference,
    // as the games do), then each frame unpacks a matrix, a GIF tag and 36
    // vertices into VU1's memory and starts the program, which transforms the
    // vertices and kicks the packet itself.
    if (frame == 0) {
        list.ref(uploads.program_address, uploads.program_quadwords, kVifNop, kVifNop);
    }
    {
        const float a = time * 0.9f, b = time * 0.6f;
        const float ca = std::cos(a), sa = std::sin(a), cb = std::cos(b), sb = std::sin(b);
        // Rotation about X then Y, and the cube's place in front of the camera.
        const float rot[3][3] = {{ca, sa * sb, sa * cb}, {0, cb, -sb}, {-sa, ca * sb, ca * cb}};
        const float place[3] = {-1.7f, 0.25f, 5.0f};
        const u32 vertices = 36;
        list.begin_data(
            kVifStcycl | 0x0101, kVifUnpackV4_32 | ((kVuVertices + vertices * 3) << 16) | 0x8000
        );
        // The matrix, a column a quadword. It takes a point straight to GS
        // coordinates times W: scaled for the screen, with W times the centre of
        // the coordinate space added so that the divide leaves the centre there.
        for (int c = 0; c < 3; c++) {
            list.floats(
                300.0f * rot[0][c] + 2048.0f * rot[2][c],
                -262.0f * rot[1][c] + 2048.0f * rot[2][c],
                0.0f,
                rot[2][c]
            );
        }
        list.floats(
            300.0f * place[0] + 2048.0f * place[2],
            -262.0f * place[1] + 2048.0f * place[2],
            16777215.0f,
            place[2]
        );
        // The tag the program copies to the head of its packet.
        list.gif_tag(
            vertices,
            true,
            0,
            {gsreg::ST, gsreg::RGBAQ, gsreg::XYZ2},
            true,
            kTriangle | kGouraud | kTextured
        );
        list.words(vertices, 0, 0, 0);
        for (int face = 0; face < 6; face++) {
            // A face: an axis, a side, and the two axes that span it.
            int axis = face / 2, u_axis = (axis + 1) % 3, v_axis = (axis + 2) % 3;
            float side = (face & 1) ? 1.0f : -1.0f;
            const int corner[6][2] = {{0, 0}, {1, 0}, {0, 1}, {1, 0}, {1, 1}, {0, 1}};
            for (const auto& c : corner) {
                float position[3];
                position[axis] = side * 0.8f;
                position[u_axis] = (static_cast<float>(c[0]) * 2.0f - 1.0f) * 0.8f;
                position[v_axis] = (static_cast<float>(c[1]) * 2.0f - 1.0f) * 0.8f;
                list.floats(static_cast<float>(c[0]), static_cast<float>(c[1]), 1.0f, 0.0f);
                u32 light =
                    0x58 + static_cast<u32>(face) * 0x0C + static_cast<u32>(c[0] + c[1]) * 0x10;
                list.words(light, light, light, 0x80);
                list.floats(position[0], position[1], position[2], 1.0f);
            }
        }
        list.end_data();
        list.begin_data(kVifMscal | 0, kVifNop);
        list.end_data();
    }

    // Two Gouraud triangles turning through each other: the Z buffer decides
    // every pixel along the line where they cross.
    list.begin_direct();
    list.gif_tag(6, true, 0, {gsreg::RGBAQ, gsreg::XYZ2}, true, kTriangle | kGouraud);
    const u32 colours[3][3] = {{0xFF, 0x30, 0x30}, {0x30, 0xFF, 0x30}, {0x30, 0x50, 0xFF}};
    for (int t = 0; t < 2; t++) {
        float turn = time * (t == 0 ? 1.1f : -0.8f) + static_cast<float>(t) * 1.3f;
        for (int i = 0; i < 3; i++) {
            float around = static_cast<float>(i) * 2.0943951f;
            Point p = {std::sin(around) * 1.1f, std::cos(around) * 1.1f + 0.3f, 0};
            Point r = {1.6f + p.x * std::cos(turn), p.y, 4.6f + p.x * std::sin(turn)};
            Projected v = project(r);
            const u32* c = colours[(i + t) % 3];
            list.rgbaq(c[0], c[1], c[2], 0x80);
            list.xyz(v.x, v.y, v.z);
        }
    }
    list.end_direct();

    // A head-up layer: a translucent panel, then the texture one texel per
    // pixel through UV coordinates, nearest filtering, table alpha blended.
    list.begin_direct();
    list.gif_tag(2, false, 0, {0xE});
    list.ad(gsreg::TEST_1, (u64{1} << 16) | (u64{1} << 17));
    list.ad(gsreg::TEX1_1, 0);
    list.gif_tag(1, false, 0, {gsreg::RGBAQ, gsreg::XYZ2, gsreg::XYZ2}, true, kSprite | kBlended);
    list.rgbaq(0x00, 0x00, 0x00, 0x50);
    list.xyz(-244, -212, 0);
    list.xyz(-244 + 80, -212 + 80, 0);
    list.gif_tag(
        1,
        true,
        0,
        {gsreg::RGBAQ, gsreg::UV, gsreg::XYZ2, gsreg::UV, gsreg::XYZ2},
        true,
        kSprite | kTextured | kBlended | kUv
    );
    list.rgbaq(0x80, 0x80, 0x80, 0x80);
    list.uv(0.5f, 0.5f);
    list.xyz(-236, -204, 0);
    list.uv(64.5f, 64.5f);
    list.xyz(-236 + 64, -204 + 64, 0);
    list.end_direct();

    list.end();
}

bool write_ppm(const std::string& path, const Image& image) {
    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) {
        return false;
    }
    std::fprintf(f, "P6\n%d %d\n255\n", image.width, image.height);
    for (u32 p : image.pixels) {
        u8 rgb[3] = {static_cast<u8>(p), static_cast<u8>(p >> 8), static_cast<u8>(p >> 16)};
        std::fwrite(rgb, 1, 3, f);
    }
    std::fclose(f);
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    int frames = -1;
    bool headless = false;
    std::string ppm;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--frames" && i + 1 < argc) {
            frames = std::atoi(argv[++i]);
        } else if (arg == "--ppm" && i + 1 < argc) {
            ppm = argv[++i];
        } else if (arg == "--headless") {
            headless = true;
        } else {
            std::fprintf(stderr, "usage: openrac-gsdemo [--frames N] [--headless] [--ppm FILE]\n");
            return 2;
        }
    }
#ifdef OPENRAC_NO_WINDOW
    headless = true;
#endif
    if (headless && frames < 0) {
        frames = 1;
    }

    GuestMemory memory;
    Graphics graphics;
    Gs& gs = graphics.gs;
    Vif1& vif = graphics.vif;
    DmaChannel channel;
    Uploads uploads = prepare_uploads(memory);

    // One read circuit showing the colour buffer: 512 pixels across 2,560
    // video clock units, 448 lines.
    gs.write_privileged(gspriv::PMODE, 1);
    gs.write_privileged(
        gspriv::DISPFB1, kFramePage | (u64{kWidth / 64} << 9) | (u64{PSMCT32} << 15)
    );
    gs.write_privileged(
        gspriv::DISPLAY1, (u64{4} << 23) | (u64{kWidth * 5 - 1} << 32) | (u64{kHeight - 1} << 44)
    );

#ifndef OPENRAC_NO_WINDOW
    host::Window window;
    if (!headless && !window.open("OpenRAC software GS", 960, 720)) {
        return 1;
    }
#endif

    Image image;
    double total_ms = 0;
    int frame = 0;
    for (; frames < 0 || frame < frames; frame++) {
#ifndef OPENRAC_NO_WINDOW
        if (!headless && !window.pump()) {
            break;
        }
#endif
        auto start = std::chrono::steady_clock::now();
        build_frame(memory, uploads, frame);
        channel.tadr = kListAddress;
        channel.chcr = 0x1C5;  // from memory, chain mode, tag transfer and tag interrupts on, start
        DmaStop stop = run_source_chain(memory, channel, [&](const u8* data, std::size_t bytes) {
            vif.write(data, bytes);
        });
        if (stop != DmaStop::End) {
            std::fprintf(stderr, "the list did not end\n");
            return 1;
        }
        if (!gs.display(image)) {
            std::fprintf(stderr, "no display circuit is on\n");
            return 1;
        }
        total_ms +=
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start)
                .count();
#ifndef OPENRAC_NO_WINDOW
        if (!headless) {
            window.present(image, 4.0f / 3.0f);
        }
#endif
    }

    if (frame > 0) {
        std::printf(
            "%d frames, %.2f ms each to build, draw and read back; %llu primitives, %llu pixels, "
            "%llu transfers, "
            "%llu VU1 instructions\n",
            frame,
            total_ms / frame,
            static_cast<unsigned long long>(gs.stats.primitives),
            static_cast<unsigned long long>(gs.stats.pixels),
            static_cast<unsigned long long>(gs.stats.transfers),
            static_cast<unsigned long long>(graphics.vu1_instructions)
        );
        if (vif.unknown_codes || graphics.vu1.unknown_ops) {
            std::fprintf(
                stderr,
                "unknown: %llu VIF codes, %llu VU instructions\n",
                static_cast<unsigned long long>(vif.unknown_codes),
                static_cast<unsigned long long>(graphics.vu1.unknown_ops)
            );
        }
    }
    if (!ppm.empty() && !write_ppm(ppm, image)) {
        std::fprintf(stderr, "cannot write %s\n", ppm.c_str());
        return 1;
    }
    return 0;
}
