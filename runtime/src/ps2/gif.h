// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <array>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "gs.h"
#include "types.h"

namespace ps2 {

// The GS interface: turns GIF packets into register writes and image data.
// A packet is a tag quadword followed by its data, in one of three layouts:
// PACKED (a quadword per register), REGLIST (64 bits per register) and IMAGE
// (raw pixels for a transfer). Each of the three paths (1: VU1's XGKICK,
// 2: VIF1's DIRECT, 3: the GIF DMA channel) keeps its own position, because a
// packet may arrive in several pieces.
//
// The GIF and the GS behind it can have a thread of their own (`start`):
// then `write` copies its data and returns, and the packets are taken apart
// and drawn on that thread, in the order given. Whatever else is to happen
// on the GS's side in that order goes through `run`, and whoever needs the
// GS itself waits with `sync` first.
class Gif {
 public:
  explicit Gif(Gs& gs) : gs_(gs) {}
  ~Gif();
  Gif(const Gif&) = delete;
  Gif& operator=(const Gif&) = delete;

  void reset();

  // Whole quadwords for one path (1, 2 or 3).
  void write(int path, const u8* data, std::size_t quadwords);

  // True when the path is between packets (the last tag seen had EOP and its
  // data is complete). Only without a thread, where `write` has done its work.
  bool idle(int path) const { return !paths_[path - 1].in_packet; }

  // How many quadwords the packet at `at` in a memory of `mask + 1`
  // quadwords has, up to the end of the data of its first tag with EOP,
  // wrapping at the memory's end. For XGKICK, which names only the start.
  static std::size_t packet_quadwords(const u8* memory, u32 at, u32 mask);

  // From now on the packets are handled on a thread of their own.
  void start();
  // Do this on the GS's side, after everything written so far.
  void run(std::function<void()> what);
  // Wait until everything written and asked has been done.
  void sync();

 private:
  struct Path {
    u64 regs = 0;
    u32 loops = 0;  // loops (or image quadwords) still to come
    u32 nreg = 0;
    u32 reg = 0;  // next register descriptor in the loop
    u32 flg = 0;
    bool eop = true;
    bool in_packet = false;
    float q = 1.0f;
  };

  void packed(Path& p, u64 lo, u64 hi);
  void advance(Path& p);
  void take(int path, const u8* data, std::size_t quadwords);

  Gs& gs_;
  std::array<Path, 3> paths_{};

  // Data copied for the thread: pieces of the paths, in order, and
  // optionally something to do after them.
  struct Piece {
    int path;
    std::size_t at, quadwords;
  };
  struct Chunk {
    std::vector<u8> data;
    std::vector<Piece> pieces;
    std::function<void()> then;
  };
  void hand_over();
  void loop();

  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable work_, done_;
  std::deque<Chunk> queue_;
  std::vector<Chunk> spare_;
  std::size_t queued_bytes_ = 0;
  bool busy_ = false, quit_ = false;
  Chunk open_;  // being filled by `write`
};

}  // namespace ps2
