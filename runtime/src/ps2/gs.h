// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <array>
#include <vector>

#include "gs_memory.h"
#include "types.h"

namespace ps2 {

// General register addresses, as they appear in A+D data and in the REGS
// field of a GIF tag.
namespace gsreg {
enum : u8 {
  PRIM = 0x00,
  RGBAQ = 0x01,
  ST = 0x02,
  UV = 0x03,
  XYZF2 = 0x04,
  XYZ2 = 0x05,
  TEX0_1 = 0x06,
  TEX0_2 = 0x07,
  CLAMP_1 = 0x08,
  CLAMP_2 = 0x09,
  FOG = 0x0A,
  XYZF3 = 0x0C,
  XYZ3 = 0x0D,
  TEX1_1 = 0x14,
  TEX1_2 = 0x15,
  TEX2_1 = 0x16,
  TEX2_2 = 0x17,
  XYOFFSET_1 = 0x18,
  XYOFFSET_2 = 0x19,
  PRMODECONT = 0x1A,
  PRMODE = 0x1B,
  TEXCLUT = 0x1C,
  SCANMSK = 0x22,
  MIPTBP1_1 = 0x34,
  MIPTBP1_2 = 0x35,
  MIPTBP2_1 = 0x36,
  MIPTBP2_2 = 0x37,
  TEXA = 0x3B,
  FOGCOL = 0x3D,
  TEXFLUSH = 0x3F,
  SCISSOR_1 = 0x40,
  SCISSOR_2 = 0x41,
  ALPHA_1 = 0x42,
  ALPHA_2 = 0x43,
  DIMX = 0x44,
  DTHE = 0x45,
  COLCLAMP = 0x46,
  TEST_1 = 0x47,
  TEST_2 = 0x48,
  PABE = 0x49,
  FBA_1 = 0x4A,
  FBA_2 = 0x4B,
  FRAME_1 = 0x4C,
  FRAME_2 = 0x4D,
  ZBUF_1 = 0x4E,
  ZBUF_2 = 0x4F,
  BITBLTBUF = 0x50,
  TRXPOS = 0x51,
  TRXREG = 0x52,
  TRXDIR = 0x53,
  HWREG = 0x54,
  SIGNAL = 0x60,
  FINISH = 0x61,
  LABEL = 0x62,
};
}  // namespace gsreg

// Privileged registers, by their address on the EE bus.
namespace gspriv {
enum : u32 {
  PMODE = 0x12000000,
  SMODE1 = 0x12000010,
  SMODE2 = 0x12000020,
  SRFSH = 0x12000030,
  SYNCH1 = 0x12000040,
  SYNCH2 = 0x12000050,
  SYNCV = 0x12000060,
  DISPFB1 = 0x12000070,
  DISPLAY1 = 0x12000080,
  DISPFB2 = 0x12000090,
  DISPLAY2 = 0x120000A0,
  EXTBUF = 0x120000B0,
  EXTDATA = 0x120000C0,
  EXTWRITE = 0x120000D0,
  BGCOLOR = 0x120000E0,
  CSR = 0x12001000,
  IMR = 0x12001010,
  BUSDIR = 0x12001040,
  SIGLBLID = 0x12001080,
};
}  // namespace gspriv

// Things the model met and does not do yet. Each is reported once.
namespace gstodo {
enum : u32 {
  ANTIALIAS = 1u << 0,
  DITHER = 1u << 1,
  SCANMASK = 1u << 2,
  AUTO_MIP_ADDRESS = 1u << 3,
  CLUT_CSM2_WIDE = 1u << 4,
  FEEDBACK_WRITE = 1u << 5,
};
}  // namespace gstodo

// An image for the host: 8-bit RGBA, rows top to bottom.
struct Image {
  int width = 0;
  int height = 0;
  std::vector<u32> pixels;  // R in the low byte
};

// The Graphics Synthesizer as the games see it: registers, local memory and
// what a drawing kick does to that memory. This is a software model: every
// pixel is computed here and stored in GS memory in the frame buffer's own
// format, so textures drawn into, read back or reused as targets behave as
// on the machine.
class Gs {
 public:
  Gs();
  void reset();

  // A write to a general register (from a GIF packet).
  void write(u8 reg, u64 data);

  // Privileged registers (written by the EE directly).
  void write_privileged(u32 address, u64 data);
  u64 read_privileged(u32 address) const;

  // Image data of a host to local transfer (GIF IMAGE mode), in any chunking.
  void transfer_in(const u8* data, std::size_t bytes);
  // Image data of a local to host transfer. Returns the bytes produced.
  std::size_t transfer_out(u8* data, std::size_t bytes);

  // The start of a vertical blank: the event bit, and which field follows.
  void vblank(bool odd_field) { csr_ = (csr_ & ~u64{0x2000}) | 0x8 | (odd_field ? 0x2000 : 0); }

  // What the display circuits show, or false when no circuit is enabled.
  bool display(Image& out) const;
  // A rectangle of any buffer as RGBA, for tools and tests.
  Image snapshot(u32 fbp, u32 fbw, u32 psm, int width, int height) const;

  GsMemory memory;

  struct Stats {
    u64 primitives = 0;
    u64 pixels = 0;
    u64 transfers = 0;
  } stats;
  u32 todo = 0;  // gstodo bits met so far

 private:
  struct Vertex {
    u16 x = 0, y = 0;  // 12.4, before the context's offset
    u32 z = 0;
    u32 rgba = 0;
    float s = 0, t = 0, q = 1;
    u16 u = 0, v = 0;  // 10.4
    u8 fog = 0;
  };

  struct Texture {
    u32 psm = 0;
    std::array<u32, 7> tbp{};
    std::array<u32, 7> tbw{};
    u32 tw = 0, th = 0;
    bool tcc = false;
    u32 tfx = 0;
    u32 csa = 0;
    u32 wms = 0, wmt = 0, minu = 0, maxu = 0, minv = 0, maxv = 0;
    bool lcm = false;
    u32 mxl = 0, mmag = 0, mmin = 0, l = 0;
    float k = 0;
  };

  // Everything a primitive needs, decoded from the registers of its context.
  struct Env {
    u32 fbp = 0, fbw = 0, fpsm = 0, fbmsk = 0;
    u32 zbp = 0, zpsm = 0, zmax = 0;
    bool zmsk = false;
    s32 ofx = 0, ofy = 0;
    s32 sx0 = 0, sx1 = 0, sy0 = 0, sy1 = 0;
    bool ate = false, date = false, datm = false, zte = false;
    u32 atst = 0, aref = 0, afail = 0, ztst = 0;
    bool abe = false, pabe = false, colclamp = true, fba = false;
    u32 ba = 0, bb = 0, bc = 0, bd = 0, fix = 0;
    bool iip = false, tme = false, fge = false, fst = false;
    u32 fogcol = 0;
    Texture tex;
  };

  struct Transfer {
    bool active = false;
    u32 dir = 3;
    u32 x = 0, y = 0;  // pixels done in the current row, rows done
    u32 width = 0, height = 0;
    std::vector<u8> pending;
  };

  void note(u32 what, const char* text);

  // Primitive assembly.
  void vertex(u16 x, u16 y, u32 z, bool draw);
  void draw_point(const Env& e, const Vertex& a);
  void draw_line(const Env& e, const Vertex& a, const Vertex& b);
  void draw_triangle(const Env& e, const Vertex& a, const Vertex& b, const Vertex& c);
  void draw_sprite(const Env& e, const Vertex& a, const Vertex& b);
  Env environment() const;
  u32 prim_bits() const;

  // Per pixel.
  void pixel(const Env& e, s32 x, s32 y, u32 z, u32 rgba);
  u32 shade(const Env& e, u32 rgba, float u, float v, float lod, u32 fog) const;
  u32 sample(const Texture& t, float u, float v, float lod) const;
  u32 sample_level(const Texture& t, u32 level, float u, float v, bool linear) const;
  u32 texel(const Texture& t, u32 level, s32 iu, s32 iv) const;
  u32 frame_read(const Env& e, s32 x, s32 y) const;
  void frame_write(const Env& e, s32 x, s32 y, u32 rgba, u32 mask);

  // Colour lookup table.
  void load_clut(u64 tex0);
  void rebuild_clut();
  u32 expand16(u16 c) const;

  // Transfers.
  void start_transfer();
  void copy_local();

  std::array<u64, 0x80> reg_{};
  std::array<u64, 0x12> priv_{};  // 0x00..0xE0 by (address >> 4), then CSR, IMR, BUSDIR, SIGLBLID
  u64 csr_ = 0, imr_ = 0, busdir_ = 0, siglblid_ = 0;

  std::array<Vertex, 3> queue_{};
  int count_ = 0;
  u8 fog_ = 0;

  std::array<u32, 256> clut_raw_{};
  std::array<u32, 256> clut_{};
  u32 clut_psm_ = PSMCT32;
  std::array<u32, 2> clut_cbp_{};

  Transfer in_;
  Transfer out_;
};

}  // namespace ps2
