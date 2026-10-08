// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "gs.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace ps2 {
namespace {

using namespace gsreg;

enum : u32 { kPoint, kLine, kLineStrip, kTriangle, kTriangleStrip, kTriangleFan, kSprite };

constexpr u32 kA = 0xFF000000u, kRgb = 0x00FFFFFFu;

inline u32 ch(u32 rgba, unsigned n) {
  return (rgba >> (n * 8)) & 0xFF;
}

inline u32 pack(u32 r, u32 g, u32 b, u32 a) {
  return r | (g << 8) | (b << 16) | (a << 24);
}

// Smallest pixel whose sample point is at or after a 12.4 coordinate.
inline s32 ceil16(s64 v) {
  return static_cast<s32>((v + 15) >> 4);
}

inline s32 sign_extend(u32 v, unsigned width) {
  u32 m = 1u << (width - 1);
  return static_cast<s32>((v ^ m) - m);
}

// The frame buffer formats in the 16-bit layout, the same test for the Z
// formats used as frame buffers.
inline bool is16(u32 psm) {
  return (psm & 0xF) == 2 || (psm & 0xF) == 0xA;
}

inline bool is24(u32 psm) {
  return (psm & 0xF) == 1;
}

inline bool alpha_passes(u32 test, u32 a, u32 ref) {
  switch (test) {
    case 0: return false;
    case 1: return true;
    case 2: return a < ref;
    case 3: return a <= ref;
    case 4: return a == ref;
    case 5: return a >= ref;
    case 6: return a > ref;
    default: return a != ref;
  }
}

inline u32 lerp_colour(u32 c0, u32 c1, float f) {
  u32 out = 0;
  for (unsigned n = 0; n < 4; n++) {
    float a = static_cast<float>(ch(c0, n)), b = static_cast<float>(ch(c1, n));
    out |= static_cast<u32>(a + (b - a) * f + 0.5f) << (n * 8);
  }
  return out;
}

}  // namespace

Gs::Gs() {
  reset();
}

void Gs::reset() {
  reg_.fill(0);
  priv_.fill(0);
  csr_ = imr_ = busdir_ = siglblid_ = 0;
  reg_[PRMODECONT] = 1;
  reg_[COLCLAMP] = 1;
  count_ = 0;
  fog_ = 0;
  clut_raw_.fill(0);
  clut_.fill(0);
  clut_psm_ = PSMCT32;
  clut_cbp_.fill(0);
  in_ = Transfer{};
  out_ = Transfer{};
}

void Gs::note(u32 what, const char* text) {
  if (todo & what) {
    return;
  }
  todo |= what;
  std::fprintf(stderr, "gs: not modelled yet: %s\n", text);
}

// --- registers -------------------------------------------------------------

void Gs::write(u8 reg, u64 data) {
  if (reg >= reg_.size()) {
    return;
  }
  switch (reg) {
    case PRIM:
      reg_[reg] = data;
      count_ = 0;
      break;
    case XYZF2:
    case XYZF3:
      fog_ = static_cast<u8>(bits(data, 56, 8));
      vertex(static_cast<u16>(bits(data, 0, 16)), static_cast<u16>(bits(data, 16, 16)),
             static_cast<u32>(bits(data, 32, 24)), reg == XYZF2);
      break;
    case XYZ2:
    case XYZ3:
      vertex(static_cast<u16>(bits(data, 0, 16)), static_cast<u16>(bits(data, 16, 16)),
             static_cast<u32>(data >> 32), reg == XYZ2);
      break;
    case FOG:
      reg_[reg] = data;
      fog_ = static_cast<u8>(bits(data, 56, 8));
      break;
    case TEX0_1:
    case TEX0_2:
      reg_[reg] = data;
      if (bits(reg_[TEX1_1 + (reg - TEX0_1)], 9, 1)) {
        note(gstodo::AUTO_MIP_ADDRESS, "automatic mipmap addresses (TEX1 MTBA)");
      }
      load_clut(data);
      break;
    case TEX2_1:
    case TEX2_2: {
      // TEX2 carries the format and colour table fields of TEX0 only.
      const u64 mask = (u64{0x3F} << 20) | (~u64{0} << 37);
      u64& tex0 = reg_[TEX0_1 + (reg - TEX2_1)];
      tex0 = (tex0 & ~mask) | (data & mask);
      reg_[reg] = data;
      load_clut(tex0);
      break;
    }
    case TEXA:
      reg_[reg] = data;
      rebuild_clut();
      break;
    case TRXDIR:
      reg_[reg] = data;
      start_transfer();
      break;
    case SIGNAL: {
      u32 id = static_cast<u32>(data), mask = static_cast<u32>(data >> 32);
      siglblid_ = (siglblid_ & ~u64{mask}) | (id & mask);
      csr_ |= 1;
      break;
    }
    case FINISH:
      csr_ |= 2;
      break;
    case LABEL: {
      u64 id = data & 0xFFFFFFFFu, mask = data >> 32;
      siglblid_ = (siglblid_ & ~(mask << 32)) | ((id & mask) << 32);
      break;
    }
    case SCANMSK:
      reg_[reg] = data;
      if (data & 3) {
        note(gstodo::SCANMASK, "scan line mask (SCANMSK)");
      }
      break;
    case DTHE:
      reg_[reg] = data;
      if (data & 1) {
        note(gstodo::DITHER, "dithering (DTHE)");
      }
      break;
    default:
      reg_[reg] = data;
      break;
  }
}

void Gs::write_privileged(u32 address, u64 data) {
  switch (address) {
    case gspriv::CSR:
      // Writing 1 to an event bit clears it; bit 9 resets the GS.
      if (data & 0x200) {
        reset();
      }
      csr_ &= ~(data & 0x1F);
      break;
    case gspriv::IMR:
      imr_ = data;
      break;
    case gspriv::BUSDIR:
      busdir_ = data;
      break;
    case gspriv::SIGLBLID:
      siglblid_ = data;
      break;
    default:
      if (address >= gspriv::PMODE && address <= gspriv::BGCOLOR) {
        priv_[(address >> 4) & 0xF] = data;
        if (address == gspriv::EXTWRITE && (data & 1)) {
          note(gstodo::FEEDBACK_WRITE, "feedback write (EXTWRITE)");
        }
      }
      break;
  }
}

u64 Gs::read_privileged(u32 address) const {
  switch (address) {
    case gspriv::CSR:
      // Revision 0x1B, id 0x55, FIFO empty (bit 14).
      return csr_ | (u64{0x1B} << 16) | (u64{0x55} << 24) | (u64{1} << 14);
    case gspriv::IMR:
      return imr_;
    case gspriv::BUSDIR:
      return busdir_;
    case gspriv::SIGLBLID:
      return siglblid_;
    default:
      if (address >= gspriv::PMODE && address <= gspriv::BGCOLOR) {
        return priv_[(address >> 4) & 0xF];
      }
      return 0;
  }
}

u32 Gs::prim_bits() const {
  // PRMODECONT chooses whether the attributes come from PRIM or PRMODE; the
  // primitive type always comes from PRIM.
  if (reg_[PRMODECONT] & 1) {
    return static_cast<u32>(reg_[PRIM]);
  }
  return static_cast<u32>((reg_[PRMODE] & ~u64{7}) | (reg_[PRIM] & 7));
}

Gs::Env Gs::environment() const {
  Env e;
  u32 prim = prim_bits();
  unsigned ctx = (prim >> 9) & 1;

  u64 frame = reg_[FRAME_1 + ctx];
  e.fbp = static_cast<u32>(bits(frame, 0, 9)) * 32;
  e.fbw = static_cast<u32>(bits(frame, 16, 6));
  e.fpsm = static_cast<u32>(bits(frame, 24, 6));
  e.fbmsk = static_cast<u32>(frame >> 32);

  u64 zbuf = reg_[ZBUF_1 + ctx];
  e.zbp = static_cast<u32>(bits(zbuf, 0, 9)) * 32;
  e.zpsm = 0x30 | static_cast<u32>(bits(zbuf, 24, 4));
  e.zmsk = bits(zbuf, 32, 1) != 0;
  e.zmax = is16(e.zpsm) ? 0xFFFFu : is24(e.zpsm) ? 0xFFFFFFu : 0xFFFFFFFFu;

  u64 offset = reg_[XYOFFSET_1 + ctx];
  e.ofx = static_cast<s32>(bits(offset, 0, 16));
  e.ofy = static_cast<s32>(bits(offset, 32, 16));

  u64 scissor = reg_[SCISSOR_1 + ctx];
  e.sx0 = static_cast<s32>(bits(scissor, 0, 11));
  e.sx1 = static_cast<s32>(bits(scissor, 16, 11));
  e.sy0 = static_cast<s32>(bits(scissor, 32, 11));
  e.sy1 = static_cast<s32>(bits(scissor, 48, 11));

  u64 test = reg_[TEST_1 + ctx];
  e.ate = bits(test, 0, 1) != 0;
  e.atst = static_cast<u32>(bits(test, 1, 3));
  e.aref = static_cast<u32>(bits(test, 4, 8));
  e.afail = static_cast<u32>(bits(test, 12, 2));
  e.date = bits(test, 14, 1) != 0;
  e.datm = bits(test, 15, 1) != 0;
  e.zte = bits(test, 16, 1) != 0;
  e.ztst = static_cast<u32>(bits(test, 17, 2));

  u64 alpha = reg_[ALPHA_1 + ctx];
  e.ba = static_cast<u32>(bits(alpha, 0, 2));
  e.bb = static_cast<u32>(bits(alpha, 2, 2));
  e.bc = static_cast<u32>(bits(alpha, 4, 2));
  e.bd = static_cast<u32>(bits(alpha, 6, 2));
  e.fix = static_cast<u32>(bits(alpha, 32, 8));
  e.pabe = (reg_[PABE] & 1) != 0;
  e.colclamp = (reg_[COLCLAMP] & 1) != 0;
  e.fba = (reg_[FBA_1 + ctx] & 1) != 0;

  e.iip = (prim >> 3) & 1;
  e.tme = (prim >> 4) & 1;
  e.fge = (prim >> 5) & 1;
  e.abe = (prim >> 6) & 1;
  e.fst = (prim >> 8) & 1;
  e.fogcol = static_cast<u32>(reg_[FOGCOL] & kRgb);

  if (e.tme) {
    Texture& t = e.tex;
    u64 tex0 = reg_[TEX0_1 + ctx];
    t.tbp[0] = static_cast<u32>(bits(tex0, 0, 14));
    t.tbw[0] = static_cast<u32>(bits(tex0, 14, 6));
    t.psm = static_cast<u32>(bits(tex0, 20, 6));
    t.tw = std::min<u32>(static_cast<u32>(bits(tex0, 26, 4)), 10);
    t.th = std::min<u32>(static_cast<u32>(bits(tex0, 30, 4)), 10);
    t.tcc = bits(tex0, 34, 1) != 0;
    t.tfx = static_cast<u32>(bits(tex0, 35, 2));
    t.csa = static_cast<u32>(bits(tex0, 56, 5));

    u64 mip1 = reg_[MIPTBP1_1 + ctx], mip2 = reg_[MIPTBP2_1 + ctx];
    for (unsigned n = 0; n < 3; n++) {
      t.tbp[1 + n] = static_cast<u32>(bits(mip1, n * 20, 14));
      t.tbw[1 + n] = static_cast<u32>(bits(mip1, n * 20 + 14, 6));
      t.tbp[4 + n] = static_cast<u32>(bits(mip2, n * 20, 14));
      t.tbw[4 + n] = static_cast<u32>(bits(mip2, n * 20 + 14, 6));
    }

    u64 clamp = reg_[CLAMP_1 + ctx];
    t.wms = static_cast<u32>(bits(clamp, 0, 2));
    t.wmt = static_cast<u32>(bits(clamp, 2, 2));
    t.minu = static_cast<u32>(bits(clamp, 4, 10));
    t.maxu = static_cast<u32>(bits(clamp, 14, 10));
    t.minv = static_cast<u32>(bits(clamp, 24, 10));
    t.maxv = static_cast<u32>(bits(clamp, 34, 10));

    u64 tex1 = reg_[TEX1_1 + ctx];
    t.lcm = bits(tex1, 0, 1) != 0;
    t.mxl = std::min<u32>(static_cast<u32>(bits(tex1, 2, 3)), 6);
    t.mmag = static_cast<u32>(bits(tex1, 5, 1));
    t.mmin = static_cast<u32>(bits(tex1, 6, 3));
    t.l = static_cast<u32>(bits(tex1, 19, 2));
    t.k = static_cast<float>(sign_extend(static_cast<u32>(bits(tex1, 32, 12)), 12)) / 16.0f;
  }
  if ((prim >> 7) & 1) {
    const_cast<Gs*>(this)->note(gstodo::ANTIALIAS, "edge antialiasing (PRIM AA1)");
  }
  return e;
}

// --- primitive assembly ----------------------------------------------------

void Gs::vertex(u16 x, u16 y, u32 z, bool draw) {
  Vertex& v = queue_[count_++];
  v.x = x;
  v.y = y;
  v.z = z;
  u64 rgbaq = reg_[RGBAQ];
  v.rgba = static_cast<u32>(rgbaq);
  v.q = as_float(static_cast<u32>(rgbaq >> 32));
  u64 st = reg_[ST];
  v.s = as_float(static_cast<u32>(st));
  v.t = as_float(static_cast<u32>(st >> 32));
  u64 uv = reg_[UV];
  v.u = static_cast<u16>(bits(uv, 0, 14));
  v.v = static_cast<u16>(bits(uv, 16, 14));
  v.fog = fog_;

  // A vertex written through XYZ3 or XYZF3 moves the queue along without
  // drawing, which is how strips skip a triangle.
  auto with_env = [&](auto&& fn) {
    if (draw) {
      Env e = environment();
      stats.primitives++;
      fn(e);
    }
  };

  switch (reg_[PRIM] & 7) {
    case kPoint:
      with_env([&](const Env& e) { draw_point(e, queue_[0]); });
      count_ = 0;
      break;
    case kLine:
      if (count_ == 2) {
        with_env([&](const Env& e) { draw_line(e, queue_[0], queue_[1]); });
        count_ = 0;
      }
      break;
    case kLineStrip:
      if (count_ == 2) {
        with_env([&](const Env& e) { draw_line(e, queue_[0], queue_[1]); });
        queue_[0] = queue_[1];
        count_ = 1;
      }
      break;
    case kTriangle:
      if (count_ == 3) {
        with_env([&](const Env& e) { draw_triangle(e, queue_[0], queue_[1], queue_[2]); });
        count_ = 0;
      }
      break;
    case kTriangleStrip:
      if (count_ == 3) {
        with_env([&](const Env& e) { draw_triangle(e, queue_[0], queue_[1], queue_[2]); });
        queue_[0] = queue_[1];
        queue_[1] = queue_[2];
        count_ = 2;
      }
      break;
    case kTriangleFan:
      if (count_ == 3) {
        with_env([&](const Env& e) { draw_triangle(e, queue_[0], queue_[1], queue_[2]); });
        queue_[1] = queue_[2];
        count_ = 2;
      }
      break;
    case kSprite:
      if (count_ == 2) {
        with_env([&](const Env& e) { draw_sprite(e, queue_[0], queue_[1]); });
        count_ = 0;
      }
      break;
    default:
      count_ = 0;
      break;
  }
}

// --- rasterisers -----------------------------------------------------------
//
// Sample points are at whole pixel coordinates. A sprite covers the pixels
// from its first corner up to, not including, its second; a triangle does
// not draw its right and bottom edges.

void Gs::draw_point(const Env& e, const Vertex& a) {
  s32 x = (static_cast<s32>(a.x) - e.ofx) >> 4;
  s32 y = (static_cast<s32>(a.y) - e.ofy) >> 4;
  if (x < e.sx0 || x > e.sx1 || y < e.sy0 || y > e.sy1) {
    return;
  }
  float u = e.fst ? a.u / 16.0f : a.s / a.q * static_cast<float>(1u << e.tex.tw);
  float v = e.fst ? a.v / 16.0f : a.t / a.q * static_cast<float>(1u << e.tex.th);
  pixel(e, x, y, std::min(a.z, e.zmax), shade(e, a.rgba, u, v, e.tex.k, a.fog));
}

void Gs::draw_line(const Env& e, const Vertex& a, const Vertex& b) {
  s32 x0 = static_cast<s32>(a.x) - e.ofx, y0 = static_cast<s32>(a.y) - e.ofy;
  s32 x1 = static_cast<s32>(b.x) - e.ofx, y1 = static_cast<s32>(b.y) - e.ofy;
  s32 steps = (std::max(std::abs(x1 - x0), std::abs(y1 - y0)) + 15) >> 4;
  if (steps <= 0) {
    return;
  }
  float tw = static_cast<float>(1u << e.tex.tw), th = static_cast<float>(1u << e.tex.th);
  for (s32 i = 0; i < steps; i++) {
    float f = static_cast<float>(i) / static_cast<float>(steps);
    s32 x = (x0 + static_cast<s32>(std::lround(static_cast<float>(x1 - x0) * f)) + 8) >> 4;
    s32 y = (y0 + static_cast<s32>(std::lround(static_cast<float>(y1 - y0) * f)) + 8) >> 4;
    if (x < e.sx0 || x > e.sx1 || y < e.sy0 || y > e.sy1) {
      continue;
    }
    u32 colour = e.iip ? lerp_colour(a.rgba, b.rgba, f) : b.rgba;
    double z = static_cast<double>(a.z) + (static_cast<double>(b.z) - a.z) * f;
    float u, v;
    if (e.fst) {
      u = (a.u + (b.u - a.u) * f) / 16.0f;
      v = (a.v + (b.v - a.v) * f) / 16.0f;
    } else {
      float q = a.q + (b.q - a.q) * f;
      u = (a.s + (b.s - a.s) * f) / q * tw;
      v = (a.t + (b.t - a.t) * f) / q * th;
    }
    u32 fog = static_cast<u32>(a.fog + (b.fog - a.fog) * f + 0.5f);
    pixel(e, x, y, std::min(static_cast<u32>(z + 0.5), e.zmax), shade(e, colour, u, v, e.tex.k, fog));
  }
}

void Gs::draw_sprite(const Env& e, const Vertex& a, const Vertex& b) {
  s32 x0 = static_cast<s32>(a.x) - e.ofx, y0 = static_cast<s32>(a.y) - e.ofy;
  s32 x1 = static_cast<s32>(b.x) - e.ofx, y1 = static_cast<s32>(b.y) - e.ofy;
  float u0, v0, u1, v1;
  if (e.fst) {
    u0 = a.u / 16.0f;
    v0 = a.v / 16.0f;
    u1 = b.u / 16.0f;
    v1 = b.v / 16.0f;
  } else {
    // Both corners are divided by the second vertex's Q.
    float tw = static_cast<float>(1u << e.tex.tw), th = static_cast<float>(1u << e.tex.th);
    u0 = a.s / b.q * tw;
    v0 = a.t / b.q * th;
    u1 = b.s / b.q * tw;
    v1 = b.t / b.q * th;
  }
  if (x0 > x1) {
    std::swap(x0, x1);
    std::swap(u0, u1);
  }
  if (y0 > y1) {
    std::swap(y0, y1);
    std::swap(v0, v1);
  }
  if (x0 == x1 || y0 == y1) {
    return;
  }
  s32 px0 = std::max(ceil16(x0), e.sx0), px1 = std::min(ceil16(x1) - 1, e.sx1);
  s32 py0 = std::max(ceil16(y0), e.sy0), py1 = std::min(ceil16(y1) - 1, e.sy1);
  float du = (u1 - u0) / static_cast<float>(x1 - x0), dv = (v1 - v0) / static_cast<float>(y1 - y0);
  u32 z = std::min(b.z, e.zmax);
  for (s32 y = py0; y <= py1; y++) {
    float v = v0 + static_cast<float>(y * 16 - y0) * dv;
    for (s32 x = px0; x <= px1; x++) {
      float u = u0 + static_cast<float>(x * 16 - x0) * du;
      pixel(e, x, y, z, shade(e, b.rgba, u, v, e.tex.k, b.fog));
    }
  }
}

void Gs::draw_triangle(const Env& e, const Vertex& a, const Vertex& b, const Vertex& c) {
  const Vertex* v[3] = {&a, &b, &c};
  s64 x[3], y[3];
  for (int i = 0; i < 3; i++) {
    x[i] = static_cast<s32>(v[i]->x) - e.ofx;
    y[i] = static_cast<s32>(v[i]->y) - e.ofy;
  }
  s64 area = (x[1] - x[0]) * (y[2] - y[0]) - (y[1] - y[0]) * (x[2] - x[0]);
  if (area == 0) {
    return;
  }
  if (area < 0) {
    std::swap(v[1], v[2]);
    std::swap(x[1], x[2]);
    std::swap(y[1], y[2]);
    area = -area;
  }

  s32 px0 = std::max(ceil16(std::min({x[0], x[1], x[2]})), e.sx0);
  s32 px1 = std::min(static_cast<s32>(std::max({x[0], x[1], x[2]}) >> 4), e.sx1);
  s32 py0 = std::max(ceil16(std::min({y[0], y[1], y[2]})), e.sy0);
  s32 py1 = std::min(static_cast<s32>(std::max({y[0], y[1], y[2]}) >> 4), e.sy1);
  if (px0 > px1 || py0 > py1) {
    return;
  }

  // Edge i runs from vertex i+1 to vertex i+2 and its function is the
  // weight of vertex i. With the area positive the inside is where all
  // three are positive; a sample exactly on an edge belongs to the triangle
  // only if the edge is a top or a left one.
  s64 ex[3], ey[3], bias[3], row[3];
  for (int i = 0; i < 3; i++) {
    int p = (i + 1) % 3, n = (i + 2) % 3;
    ex[i] = x[n] - x[p];
    ey[i] = y[n] - y[p];
    bool top_left = ey[i] < 0 || (ey[i] == 0 && ex[i] > 0);
    bias[i] = top_left ? 0 : 1;
    row[i] = ex[i] * (static_cast<s64>(py0) * 16 - y[p]) - ey[i] * (static_cast<s64>(px0) * 16 - x[p]);
  }

  double inv_area = 1.0 / static_cast<double>(area);
  float tw = static_cast<float>(1u << e.tex.tw), th = static_cast<float>(1u << e.tex.th);
  double z[3] = {static_cast<double>(v[0]->z), static_cast<double>(v[1]->z), static_cast<double>(v[2]->z)};
  bool flat_z = v[0]->z == v[1]->z && v[1]->z == v[2]->z;

  for (s32 py = py0; py <= py1; py++) {
    s64 w[3] = {row[0], row[1], row[2]};
    for (s32 px = px0; px <= px1; px++) {
      if (w[0] >= bias[0] && w[1] >= bias[1] && w[2] >= bias[2]) {
        double l0 = static_cast<double>(w[0]) * inv_area;
        double l1 = static_cast<double>(w[1]) * inv_area;
        double l2 = static_cast<double>(w[2]) * inv_area;

        u32 zv = flat_z ? v[0]->z
                        : static_cast<u32>(std::clamp(l0 * z[0] + l1 * z[1] + l2 * z[2] + 0.5, 0.0, 4294967295.0));

        u32 colour = c.rgba;
        if (e.iip) {
          colour = 0;
          for (unsigned n = 0; n < 4; n++) {
            double value = l0 * ch(v[0]->rgba, n) + l1 * ch(v[1]->rgba, n) + l2 * ch(v[2]->rgba, n);
            colour |= std::min<u32>(static_cast<u32>(value + 0.5), 255) << (n * 8);
          }
        }

        float tu = 0, tv = 0, lod = e.tex.k;
        if (e.tme) {
          if (e.fst) {
            tu = static_cast<float>(l0 * v[0]->u + l1 * v[1]->u + l2 * v[2]->u) / 16.0f;
            tv = static_cast<float>(l0 * v[0]->v + l1 * v[1]->v + l2 * v[2]->v) / 16.0f;
          } else {
            double s = l0 * v[0]->s + l1 * v[1]->s + l2 * v[2]->s;
            double t = l0 * v[0]->t + l1 * v[1]->t + l2 * v[2]->t;
            double q = l0 * v[0]->q + l1 * v[1]->q + l2 * v[2]->q;
            tu = static_cast<float>(s / q) * tw;
            tv = static_cast<float>(t / q) * th;
            if (!e.tex.lcm) {
              lod = static_cast<float>(-std::log2(std::fabs(q))) * static_cast<float>(1u << e.tex.l) + e.tex.k;
            }
          }
        }

        u32 fog = static_cast<u32>(l0 * v[0]->fog + l1 * v[1]->fog + l2 * v[2]->fog + 0.5);
        pixel(e, px, py, std::min(zv, e.zmax), shade(e, colour, tu, tv, lod, fog));
      }
      for (int i = 0; i < 3; i++) {
        w[i] -= ey[i] * 16;
      }
    }
    for (int i = 0; i < 3; i++) {
      row[i] += ex[i] * 16;
    }
  }
}

// --- texturing ---------------------------------------------------------------

u32 Gs::expand16(u16 c) const {
  u64 texa = reg_[TEXA];
  u32 a;
  if (c & 0x8000) {
    a = static_cast<u32>(bits(texa, 32, 8));
  } else if (bits(texa, 15, 1) && c == 0) {
    a = 0;
  } else {
    a = static_cast<u32>(bits(texa, 0, 8));
  }
  return pack((c & 0x1F) << 3, ((c >> 5) & 0x1F) << 3, ((c >> 10) & 0x1F) << 3, a);
}

u32 Gs::texel(const Texture& t, u32 level, s32 iu, s32 iv) const {
  s32 w = std::max(1, (1 << t.tw) >> level), h = std::max(1, (1 << t.th) >> level);
  auto wrap = [level](s32 c, s32 size, u32 mode, u32 lo, u32 hi) -> s32 {
    switch (mode) {
      case 0: return c & (size - 1);
      case 1: return std::clamp(c, 0, size - 1);
      case 2: return std::clamp(c, static_cast<s32>(lo >> level), static_cast<s32>(hi >> level));
      default: return (c & static_cast<s32>(lo)) | static_cast<s32>(hi);
    }
  };
  iu = wrap(iu, w, t.wms, t.minu, t.maxu);
  iv = wrap(iv, h, t.wmt, t.minv, t.maxv);

  u32 psm = t.psm >= 0x30 ? (t.psm & 0xF) : t.psm;  // a Z buffer read as a texture
  u32 raw = memory.read(t.psm, t.tbp[level], t.tbw[level], static_cast<u32>(iu), static_cast<u32>(iv));
  switch (psm) {
    case PSMCT32:
      return raw;
    case PSMCT24: {
      u64 texa = reg_[TEXA];
      u32 a = (bits(texa, 15, 1) && raw == 0) ? 0 : static_cast<u32>(bits(texa, 0, 8));
      return raw | (a << 24);
    }
    case PSMCT16:
    case PSMCT16S:
      return expand16(static_cast<u16>(raw));
    default:
      return clut_[(t.csa * 16 + raw) & 0xFF];
  }
}

u32 Gs::sample_level(const Texture& t, u32 level, float u, float v, bool linear) const {
  float scale = 1.0f / static_cast<float>(1u << level);
  u *= scale;
  v *= scale;
  if (!linear) {
    return texel(t, level, static_cast<s32>(std::floor(u)), static_cast<s32>(std::floor(v)));
  }
  u -= 0.5f;
  v -= 0.5f;
  float fu0 = std::floor(u), fv0 = std::floor(v);
  s32 iu = static_cast<s32>(fu0), iv = static_cast<s32>(fv0);
  float fu = u - fu0, fv = v - fv0;
  u32 top = lerp_colour(texel(t, level, iu, iv), texel(t, level, iu + 1, iv), fu);
  u32 bottom = lerp_colour(texel(t, level, iu, iv + 1), texel(t, level, iu + 1, iv + 1), fu);
  return lerp_colour(top, bottom, fv);
}

u32 Gs::sample(const Texture& t, float u, float v, float lod) const {
  if (lod <= 0.0f) {
    return sample_level(t, 0, u, v, t.mmag != 0);
  }
  bool linear = t.mmin == 1 || t.mmin >= 4;
  if (t.mxl == 0 || t.mmin < 2) {
    return sample_level(t, 0, u, v, linear);
  }
  float top = static_cast<float>(t.mxl);
  if (t.mmin == 2 || t.mmin == 4) {
    return sample_level(t, static_cast<u32>(std::min(lod + 0.5f, top)), u, v, linear);
  }
  float l = std::min(lod, top);
  u32 l0 = static_cast<u32>(l), l1 = std::min(l0 + 1, t.mxl);
  return lerp_colour(sample_level(t, l0, u, v, linear), sample_level(t, l1, u, v, linear), l - static_cast<float>(l0));
}

u32 Gs::shade(const Env& e, u32 rgba, float u, float v, float lod, u32 fog) const {
  u32 r = ch(rgba, 0), g = ch(rgba, 1), b = ch(rgba, 2), a = ch(rgba, 3);
  if (e.tme) {
    u32 t = sample(e.tex, u, v, lod);
    u32 tr = ch(t, 0), tg = ch(t, 1), tb = ch(t, 2), ta = ch(t, 3);
    auto mod = [](u32 x, u32 y) { return std::min<u32>((x * y) >> 7, 255); };
    switch (e.tex.tfx) {
      case 0:  // modulate
        r = mod(tr, r);
        g = mod(tg, g);
        b = mod(tb, b);
        if (e.tex.tcc) {
          a = mod(ta, a);
        }
        break;
      case 1:  // decal
        r = tr;
        g = tg;
        b = tb;
        if (e.tex.tcc) {
          a = ta;
        }
        break;
      default:  // highlight, highlight 2
        r = std::min<u32>(mod(tr, r) + a, 255);
        g = std::min<u32>(mod(tg, g) + a, 255);
        b = std::min<u32>(mod(tb, b) + a, 255);
        if (e.tex.tcc) {
          a = e.tex.tfx == 2 ? std::min<u32>(ta + a, 255) : ta;
        }
        break;
    }
  }
  if (e.fge) {
    auto mix = [fog](u32 c, u32 f) { return static_cast<u32>(static_cast<s32>(f) + (((static_cast<s32>(c) - static_cast<s32>(f)) * static_cast<s32>(fog)) >> 8)); };
    r = mix(r, ch(e.fogcol, 0));
    g = mix(g, ch(e.fogcol, 1));
    b = mix(b, ch(e.fogcol, 2));
  }
  return pack(r, g, b, a);
}

// --- colour lookup table -----------------------------------------------------
//
// The GS keeps its own copy of the table and loads it when TEX0 or TEX2 is
// written, as the CLD field says. Later changes to the memory the table came
// from do not reach a primitive until the table is loaded again.

void Gs::load_clut(u64 tex0) {
  u32 psm = static_cast<u32>(bits(tex0, 20, 6));
  u32 cbp = static_cast<u32>(bits(tex0, 37, 14));
  u32 cpsm = static_cast<u32>(bits(tex0, 51, 4));
  bool csm2 = bits(tex0, 55, 1) != 0;
  u32 csa = static_cast<u32>(bits(tex0, 56, 5));
  u32 cld = static_cast<u32>(bits(tex0, 61, 3));

  switch (cld) {
    case 1:
      break;
    case 2:
    case 3:
      clut_cbp_[cld - 2] = cbp;
      break;
    case 4:
    case 5:
      if (clut_cbp_[cld - 4] == cbp) {
        return;
      }
      clut_cbp_[cld - 4] = cbp;
      break;
    default:
      return;
  }

  u32 entries;
  if (psm == PSMT8 || psm == PSMT8H) {
    entries = 256;
  } else if (psm == PSMT4 || psm == PSMT4HL || psm == PSMT4HH) {
    entries = 16;
  } else {
    return;
  }

  for (u32 i = 0; i < entries; i++) {
    u32 raw;
    if (csm2) {
      u64 texclut = reg_[TEXCLUT];
      u32 x = static_cast<u32>(bits(texclut, 6, 6)) * 16 + i;
      raw = memory.read(PSMCT16, cbp, static_cast<u32>(bits(texclut, 0, 6)), x, static_cast<u32>(bits(texclut, 12, 10)));
    } else if (entries == 256) {
      // Entries 8-15 and 16-23 of every 32 are stored the other way round.
      u32 p = (i & 0xE7) | ((i & 0x08) << 1) | ((i & 0x10) >> 1);
      raw = memory.read(cpsm, cbp, 1, p & 15, p >> 4);
    } else {
      raw = memory.read(cpsm, cbp, 1, i & 7, i >> 3);
    }
    clut_raw_[(csa * 16 + i) & 0xFF] = raw;
  }
  clut_psm_ = csm2 ? static_cast<u32>(PSMCT16) : cpsm;
  rebuild_clut();
}

void Gs::rebuild_clut() {
  for (u32 i = 0; i < 256; i++) {
    clut_[i] = clut_psm_ == PSMCT32 ? clut_raw_[i] : expand16(static_cast<u16>(clut_raw_[i]));
  }
}

// --- pixel pipeline ----------------------------------------------------------

u32 Gs::frame_read(const Env& e, s32 x, s32 y) const {
  u32 v = memory.read(e.fpsm, e.fbp, e.fbw, static_cast<u32>(x), static_cast<u32>(y));
  if (is16(e.fpsm)) {
    return pack((v & 0x1F) << 3, ((v >> 5) & 0x1F) << 3, ((v >> 10) & 0x1F) << 3, (v & 0x8000) ? 0x80 : 0);
  }
  if (is24(e.fpsm)) {
    return v | 0x80000000u;
  }
  return v;
}

void Gs::frame_write(const Env& e, s32 x, s32 y, u32 rgba, u32 mask) {
  u32 ux = static_cast<u32>(x), uy = static_cast<u32>(y);
  if (is16(e.fpsm)) {
    auto to16 = [](u32 c) { return ((c >> 3) & 0x1F) | (((c >> 11) & 0x1F) << 5) | (((c >> 19) & 0x1F) << 10) | ((c >> 16) & 0x8000); };
    u32 m = to16(mask), v = to16(rgba);
    u32 old = m == 0xFFFF ? 0 : memory.read(e.fpsm, e.fbp, e.fbw, ux, uy);
    memory.write(e.fpsm, e.fbp, e.fbw, ux, uy, (old & ~m) | (v & m));
    return;
  }
  if (is24(e.fpsm)) {
    mask &= kRgb;
  }
  u32 old = mask == 0xFFFFFFFFu ? 0 : memory.read(e.fpsm, e.fbp, e.fbw, ux, uy);
  memory.write(e.fpsm, e.fbp, e.fbw, ux, uy, (old & ~mask) | (rgba & mask));
}

void Gs::pixel(const Env& e, s32 x, s32 y, u32 z, u32 rgba) {
  u32 sa = rgba >> 24;
  bool write_rgb = true, write_a = true, write_z = !e.zmsk;

  if (e.ate && !alpha_passes(e.atst, sa, e.aref)) {
    switch (e.afail) {
      case 0:  // keep
        return;
      case 1:  // frame buffer only
        write_z = false;
        break;
      case 2:  // Z buffer only
        write_rgb = write_a = false;
        break;
      default:  // colour only; without an alpha channel it is "frame buffer only"
        write_z = false;
        write_a = !((e.fpsm & 0xF) == 0);
        break;
    }
  }

  bool need_dest = e.date || e.abe;
  u32 dest = need_dest ? frame_read(e, x, y) : 0;
  if (e.date && !is24(e.fpsm) && ((dest >> 31) != 0) != e.datm) {
    return;
  }

  if (e.zte) {
    if (e.ztst == 0) {
      return;
    }
    if (e.ztst >= 2) {
      u32 stored = memory.read(e.zpsm, e.zbp, e.fbw, static_cast<u32>(x), static_cast<u32>(y));
      if (e.ztst == 2 ? z < stored : z <= stored) {
        return;
      }
    }
  }

  stats.pixels++;

  if (write_rgb || write_a) {
    u32 out = rgba;
    if (e.abe && !(e.pabe && !(sa & 0x80))) {
      u32 da = dest >> 24;
      s32 factor = e.bc == 0 ? static_cast<s32>(sa) : e.bc == 1 ? static_cast<s32>(da) : static_cast<s32>(e.fix);
      out &= kA;
      for (unsigned n = 0; n < 3; n++) {
        s32 cs = static_cast<s32>(ch(rgba, n)), cd = static_cast<s32>(ch(dest, n));
        s32 a = e.ba == 0 ? cs : e.ba == 1 ? cd : 0;
        s32 b = e.bb == 0 ? cs : e.bb == 1 ? cd : 0;
        s32 d = e.bd == 0 ? cs : e.bd == 1 ? cd : 0;
        s32 c = (((a - b) * factor) >> 7) + d;
        c = e.colclamp ? std::clamp(c, 0, 255) : (c & 0xFF);
        out |= static_cast<u32>(c) << (n * 8);
      }
    }
    if (e.fba) {
      out |= 0x80000000u;
    }
    u32 mask = ~e.fbmsk & ((write_rgb ? kRgb : 0) | (write_a ? kA : 0));
    if (mask) {
      frame_write(e, x, y, out, mask);
    }
  }

  if (write_z) {
    memory.write(e.zpsm, e.zbp, e.fbw, static_cast<u32>(x), static_cast<u32>(y), z);
  }
}

// --- transfers ---------------------------------------------------------------

void Gs::start_transfer() {
  u32 dir = static_cast<u32>(reg_[TRXDIR] & 3);
  u64 size = reg_[TRXREG];
  u32 width = static_cast<u32>(bits(size, 0, 12)), height = static_cast<u32>(bits(size, 32, 12));
  in_ = Transfer{};
  out_ = Transfer{};
  if (width == 0 || height == 0) {
    return;
  }
  stats.transfers++;
  if (dir == 0) {
    in_.active = true;
    in_.width = width;
    in_.height = height;
  } else if (dir == 1) {
    // Local to host: produce the whole image now, hand it out as it is asked for.
    u64 buffer = reg_[BITBLTBUF], position = reg_[TRXPOS];
    u32 bp = static_cast<u32>(bits(buffer, 0, 14)), bw = static_cast<u32>(bits(buffer, 16, 6));
    u32 psm = static_cast<u32>(bits(buffer, 24, 6));
    u32 sx = static_cast<u32>(bits(position, 0, 11)), sy = static_cast<u32>(bits(position, 16, 11));
    unsigned bpp = transfer_bits(psm);
    u64 acc = 0;
    unsigned have = 0;
    for (u32 y = 0; y < height; y++) {
      for (u32 x = 0; x < width; x++) {
        acc |= static_cast<u64>(memory.read(psm, bp, bw, (sx + x) & 2047, (sy + y) & 2047)) << have;
        have += bpp;
        while (have >= 8) {
          out_.pending.push_back(static_cast<u8>(acc));
          acc >>= 8;
          have -= 8;
        }
      }
    }
    if (have) {
      out_.pending.push_back(static_cast<u8>(acc));
    }
    out_.pending.resize((out_.pending.size() + 15) & ~std::size_t{15});
    out_.active = true;
  } else if (dir == 2) {
    copy_local();
  }
}

void Gs::transfer_in(const u8* data, std::size_t bytes) {
  if (!in_.active) {
    return;
  }
  u64 buffer = reg_[BITBLTBUF], position = reg_[TRXPOS];
  u32 bp = static_cast<u32>(bits(buffer, 32, 14)), bw = static_cast<u32>(bits(buffer, 48, 6));
  u32 psm = static_cast<u32>(bits(buffer, 56, 6));
  u32 dx = static_cast<u32>(bits(position, 32, 11)), dy = static_cast<u32>(bits(position, 48, 11));
  unsigned bpp = transfer_bits(psm);

  auto put = [&](u32 value) {
    memory.write(psm, bp, bw, (dx + in_.x) & 2047, (dy + in_.y) & 2047, value);
    if (++in_.x == in_.width) {
      in_.x = 0;
      if (++in_.y == in_.height) {
        in_.active = false;
      }
    }
  };

  in_.pending.insert(in_.pending.end(), data, data + bytes);
  std::size_t used = 0;
  const std::vector<u8>& p = in_.pending;
  if (bpp == 4) {
    while (used < p.size() && in_.active) {
      put(p[used] & 0xF);
      if (in_.active) {
        put(p[used] >> 4);
      }
      used++;
    }
  } else {
    unsigned step = bpp / 8;
    while (used + step <= p.size() && in_.active) {
      u32 value = 0;
      for (unsigned n = 0; n < step; n++) {
        value |= static_cast<u32>(p[used + n]) << (n * 8);
      }
      put(value);
      used += step;
    }
  }
  if (in_.active) {
    in_.pending.erase(in_.pending.begin(), in_.pending.begin() + static_cast<std::ptrdiff_t>(used));
  } else {
    in_.pending.clear();
  }
}

std::size_t Gs::transfer_out(u8* data, std::size_t bytes) {
  if (!out_.active) {
    return 0;
  }
  std::size_t n = std::min(bytes, out_.pending.size() - out_.x);
  std::memcpy(data, out_.pending.data() + out_.x, n);
  out_.x += static_cast<u32>(n);
  if (out_.x == out_.pending.size()) {
    out_ = Transfer{};
  }
  return n;
}

void Gs::copy_local() {
  u64 buffer = reg_[BITBLTBUF], position = reg_[TRXPOS], size = reg_[TRXREG];
  u32 sbp = static_cast<u32>(bits(buffer, 0, 14)), sbw = static_cast<u32>(bits(buffer, 16, 6));
  u32 spsm = static_cast<u32>(bits(buffer, 24, 6));
  u32 dbp = static_cast<u32>(bits(buffer, 32, 14)), dbw = static_cast<u32>(bits(buffer, 48, 6));
  u32 dpsm = static_cast<u32>(bits(buffer, 56, 6));
  u32 sx = static_cast<u32>(bits(position, 0, 11)), sy = static_cast<u32>(bits(position, 16, 11));
  u32 dx = static_cast<u32>(bits(position, 32, 11)), dy = static_cast<u32>(bits(position, 48, 11));
  u32 order = static_cast<u32>(bits(position, 59, 2));  // bit 0: rows bottom up, bit 1: right to left
  u32 width = static_cast<u32>(bits(size, 0, 12)), height = static_cast<u32>(bits(size, 32, 12));
  for (u32 j = 0; j < height; j++) {
    u32 y = (order & 1) ? height - 1 - j : j;
    for (u32 i = 0; i < width; i++) {
      u32 x = (order & 2) ? width - 1 - i : i;
      u32 value = memory.read(spsm, sbp, sbw, (sx + x) & 2047, (sy + y) & 2047);
      memory.write(dpsm, dbp, dbw, (dx + x) & 2047, (dy + y) & 2047, value);
    }
  }
}

// --- output ------------------------------------------------------------------

Image Gs::snapshot(u32 bp, u32 bw, u32 psm, int width, int height) const {
  Image image;
  image.width = width;
  image.height = height;
  image.pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      u32 v = memory.read(psm, bp, bw, static_cast<u32>(x), static_cast<u32>(y));
      if (is16(psm)) {
        v = pack((v & 0x1F) << 3, ((v >> 5) & 0x1F) << 3, ((v >> 10) & 0x1F) << 3, 0);
      }
      image.pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)] = v | kA;
    }
  }
  return image;
}

bool Gs::display(Image& out) const {
  u64 pmode = priv_[0];
  int circuit = (pmode & 1) ? 0 : (pmode & 2) ? 1 : -1;
  if (circuit < 0) {
    return false;
  }
  u64 fb = priv_[circuit == 0 ? 0x7 : 0x9], disp = priv_[circuit == 0 ? 0x8 : 0xA];
  u32 bp = static_cast<u32>(bits(fb, 0, 9)) * 32, bw = static_cast<u32>(bits(fb, 9, 6));
  u32 psm = static_cast<u32>(bits(fb, 15, 5));
  u32 dbx = static_cast<u32>(bits(fb, 32, 11)), dby = static_cast<u32>(bits(fb, 43, 11));
  int width = static_cast<int>((bits(disp, 32, 12) + 1) / (bits(disp, 23, 4) + 1));
  int height = static_cast<int>((bits(disp, 44, 11) + 1) / (bits(disp, 27, 2) + 1));
  if (bw == 0 || width <= 0 || height <= 0) {
    return false;
  }
  width = std::min(width, static_cast<int>(bw * 64));
  out.width = width;
  out.height = height;
  out.pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      u32 v = memory.read(psm, bp, bw, (dbx + static_cast<u32>(x)) & 2047, (dby + static_cast<u32>(y)) & 2047);
      if (is16(psm)) {
        v = pack((v & 0x1F) << 3, ((v >> 5) & 0x1F) << 3, ((v >> 10) & 0x1F) << 3, 0);
      }
      out.pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)] = v | kA;
    }
  }
  return true;
}

}  // namespace ps2
