// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <array>
#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
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
//
// Primitives can be drawn as they arrive, or (set_threads) gathered and
// drawn by other threads while more are gathered, each thread taking bands
// of scan lines. The result is the same memory either way: a band's
// primitives are drawn in their order, and the gathered ones are drawn
// before anything reads or writes memory they use.
class Gs {
 public:
  Gs();
  ~Gs();
  void reset();

  // How many threads draw. 0: each primitive is drawn at once, by the caller.
  // 1: gathered, and drawn by the caller. More: gathered, and drawn in the
  // background by that many threads.
  void set_threads(unsigned threads);
  // Have everything gathered so far drawn. For whatever reads `memory` or
  // `stats` from outside.
  void finish();

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
  bool display(Image& out);
  // A rectangle of any buffer as RGBA, for tools and tests.
  Image snapshot(u32 fbp, u32 fbw, u32 psm, int width, int height);

  // For tools: called for every primitive drawn, with a line describing the
  // state it is drawn with (target, scissor, texture, tests, blending) and
  // its bounding box in the target.
  std::function<void(const std::string& state, int x0, int y0, int x1, int y1, float lod0, float lod1)> on_primitive;

  GsMemory memory;

  struct Stats {
    u64 primitives = 0;
    u64 pixels = 0;
    u64 transfers = 0;
    u64 texture_decodes = 0, texels_decoded = 0;
    u64 flushes = 0;
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

  // A set of pages of GS memory (512 of 8 KB).
  struct Pages {
    std::array<u64, 8> bits{};
    void set(u32 page) { bits[(page >> 6) & 7] |= u64{1} << (page & 63); }
    void add(const Pages& o) {
      for (unsigned n = 0; n < 8; n++) bits[n] |= o.bits[n];
    }
    bool intersects(const Pages& o) const {
      u64 any = 0;
      for (unsigned n = 0; n < 8; n++) any |= bits[n] & o.bits[n];
      return any != 0;
    }
    void clear() { bits = {}; }
  };
  // The pages a rectangle of a buffer lies in.
  static void add_pages(Pages& pages, u32 psm, u32 bp, u32 bw, s32 x0, s32 y0, s32 x1, s32 y1);

  // The colour table as it was at some moment, for the primitives given then.
  struct ClutCopy {
    std::array<u32, 256> colours;
    u64 hash = 0;  // of all 256 entries, once something has asked
    bool hashed = false;
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
    // Worked out once a primitive:
    const GsMemory::Layout* layout = nullptr;
    unsigned kind = 0;         // how a stored texel becomes a colour (see gs.cpp)
    bool lod_per_pixel = false;  // the level or the filter depends on each pixel's Q
    u32 ta0 = 0, ta1 = 0;
    bool aem = false;
    const u32* clut = nullptr;  // the colour table as it was when the primitive was given
    ClutCopy* clut_source = nullptr;
    // Each level as plain colours, taken when a primitive first needs it
    // (null: the level is read from GS memory texel by texel).
    std::array<const u32*, 7> decoded{};
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
    const GsMemory::Layout* flayout = nullptr;
    const GsMemory::Layout* zlayout = nullptr;
    bool f16 = false, f24 = false, z16 = false, z24 = false;
    u64 target = 0;      // which buffers it draws to
    // About the texture's levels, filled in as primitives need them:
    u32 looked_at = 0, in_place = 0;     // levels found; levels read from GS memory
    std::array<Pages, 7> level_pages{};  // the pages each level lies in
    u32 last_need = 0;                   // the levels the last primitive needed,
    Pages decoded_pages, in_place_pages; // their pages: of those decoded, of those read in place
  };

  // A primitive waiting to be drawn.
  struct Queued {
    const Env* env;
    u8 kind;
    Vertex v[3];
  };

  // Primitives gathered to be drawn together, with everything they refer
  // to. Once handed over to be drawn, nothing in it changes.
  struct Batch {
    std::vector<Queued> primitives;
    std::array<std::vector<u32>, 128> bands;  // by 16 scan lines: the primitives that reach each band
    std::vector<u16> used_bands;
    std::deque<Env> envs;
    std::deque<ClutCopy> cluts;
    std::vector<std::shared_ptr<std::vector<u32>>> textures;  // the decoded levels its states use
    void clear();
  };

  struct Transfer {
    bool active = false, changed = false;
    u32 dir = 3;
    u32 x = 0, y = 0;  // pixels done in the current row, rows done
    u32 width = 0, height = 0;
    std::vector<u8> pending;
  };

  void note(u32 what, const char* text);

  // Primitive assembly.
  void vertex(u16 x, u16 y, u32 z, bool draw);
  // Rows outside clip0..clip1 are left to whoever has that band.
  void draw_point(const Env& e, const Vertex& a, s32 clip0, s32 clip1);
  void draw_line(const Env& e, const Vertex& a, const Vertex& b, s32 clip0, s32 clip1);
  void draw_triangle(const Env& e, const Vertex& a, const Vertex& b, const Vertex& c, s32 clip0, s32 clip1);
  void draw_sprite(const Env& e, const Vertex& a, const Vertex& b, s32 clip0, s32 clip1);
  void draw(const Queued& q, s32 clip0, s32 clip1);
  void submit(unsigned kind, unsigned count);
  void render(const Batch& batch);
  void render_band(const Batch& batch, unsigned band);
  Env environment();
  void ensure_env();
  void report(const Env& e, unsigned count) const;
  u32 prim_bits() const;

  // Per pixel.
  void pixel(const Env& e, const GsMemory::Row& frow, const GsMemory::Row& zrow, s32 x, u32 z, u32 rgba);
  u32 shade(const Env& e, u32 rgba, float u, float v, float lod, u32 fog) const;
  u32 sample(const Texture& t, float u, float v, float lod) const;
  u32 sample_level(const Texture& t, u32 level, float u, float v, bool linear) const;
  u32 texel(const Texture& t, u32 level, s32 iu, s32 iv, const u32* decoded) const;
  u32 frame_read(const Env& e, const GsMemory::Row& row, s32 x) const;
  void frame_write(const Env& e, const GsMemory::Row& row, s32 x, u32 rgba, u32 mask);

  // Decoded textures. A level is decoded once and kept until something
  // writes to the pages it lies in, the colour table it used changes, or
  // TEXA changes under a format that needs it.
  struct CachedTexture {
    std::shared_ptr<std::vector<u32>> texels;
    Pages pages;
    u64 stamp = 0;
  };
  std::shared_ptr<std::vector<u32>> cached_level(const Texture& t, u32 level);
  // Which levels of its texture the primitive in the queue can read.
  u32 levels_needed(const Env& e, unsigned count) const;
  // Have a current state with those levels found: decoded copies, or in place.
  void prepare_levels(u32 need);
  bool in_place_reach(const Env& e, unsigned count, u32 need, Pages& pages) const;
  // Note a write to pages: decoded copies of them are stale from now on.
  void stamp(const Pages& pages);
  // A level is known by where and how it is stored, and by what turns its
  // texels into colours: the table entries it uses, or TEXA.
  struct TextureKey {
    u64 place = 0, colours = 0;
    bool operator==(const TextureKey&) const = default;
  };
  struct TextureKeyHash {
    std::size_t operator()(const TextureKey& k) const { return static_cast<std::size_t>(k.place * 0x9E3779B97F4A7C15ull ^ k.colours); }
  };
  std::unordered_map<TextureKey, CachedTexture, TextureKeyHash> texture_cache_;
  std::array<u64, 512> page_stamp_{};
  u64 clock_ = 1;

  // Colour lookup table.
  void load_clut(u64 tex0);
  void rebuild_clut();
  static u32 expand16(u16 c, u32 ta0, u32 ta1, bool aem);

  // Primitives are gathered into a batch; a full batch is drawn by a thread
  // of its own while the next is gathered, its bands shared out among the
  // pool. A batch being drawn touches only the pages it draws to and the
  // textures it reads in place, so whoever gathers may read and write every
  // other page meanwhile, and waits before touching those.
  class Pool;
  class Raster;
  std::unique_ptr<Pool> pool_;
  unsigned threads_ = 0;
  std::unique_ptr<Batch> batch_;         // the one being gathered
  Pages pending_write_, pending_read_;   // what it will write, and read in place
  Pages pending_colour_, pending_depth_; // what it will write, by which side
  Pages inflight_write_, inflight_read_; // the same for batches handed over and maybe not drawn yet
  u64 pending_target_ = 0;
  Env* env_ = nullptr;
  bool env_dirty_ = true, clut_copy_dirty_ = true;
  std::atomic<u64> pixels_{0};
  // Hand the gathered batch over (or draw it here, with no thread for that).
  // The current state is gone afterwards: `ensure_env` makes the next.
  void flush();
  void wait_for_drawing();
  // Before reading or writing pages of GS memory outside drawing.
  void before_read(const Pages& pages);
  void before_write(const Pages& pages);
  std::unique_ptr<Raster> raster_;  // last: it stops before what it uses goes

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
  Pages in_pages_;
};

}  // namespace ps2
