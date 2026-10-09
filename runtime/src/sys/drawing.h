// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

#include "ps2/graphics.h"

namespace sys {

using ps2::u32;
using ps2::u64;
using ps2::u8;

// The drawing path (VIF1, VU1, the GIF, the GS) on a thread of its own.
//
// On the console those run beside the EE: a program hands the DMA controller
// a display list and carries on with the next frame while the list is drawn.
// Here the EE's side copies what a channel sends as it sends it, and this
// thread takes the copies in order. Nothing on the drawing side reads the
// EE's memory, so the two never touch the same data; whatever the EE's side
// needs from the drawing side (a register read, pixels sent back, the
// picture) waits until everything given so far has been done.
//
// Until `start` is called everything runs at once on the caller's thread,
// which is also how it runs when the thread is not wanted.
class Drawing {
public:
    explicit Drawing(ps2::Graphics& graphics) : graphics_(graphics) {}

    ~Drawing();
    Drawing(const Drawing&) = delete;
    Drawing& operator=(const Drawing&) = delete;

    void start();

    bool threaded() const { return thread_.joinable(); }

    // --- what the EE's side gives, in order ---
    // Data a DMA channel or a FIFO write sends: to VIF1, or to the GIF on path 3.
    // Pieces gather until `send`.
    void vif(const u8* data, std::size_t bytes) {
        if (vif_copy) {
            vif_copy->insert(vif_copy->end(), data, data + bytes);
        }
        gather(kVif, data, bytes);
    }

    void gif(const u8* data, std::size_t bytes) { gather(kGif, data, bytes); }

    void send();
    // A write to one of the GS's privileged registers.
    void privileged(u32 address, u64 value);
    // The start of a vertical blank.
    void vblank(bool odd_field);
    // Take the picture the display circuits show now. At most two of these
    // wait at a time: with a third, the caller waits, so the EE's side never
    // runs far ahead of what is drawn.
    void present();

    // For tools: when set, everything given to VIF1 is also appended here.
    std::vector<u8>* vif_copy = nullptr;

    // --- what the EE's side asks ---
    // Wait until everything given has been done.
    void sync();
    // The latest picture taken by `present`. False if there is none yet or the
    // display was off.
    bool picture(ps2::Image& out);

private:
    enum Kind : u8 {
        kVif,
        kGif,
        kPrivileged,
        kVblank,
        kPresent
    };

    struct Command {
        Kind kind = kVif;
        u32 address = 0;
        u64 value = 0;
        std::vector<u8> data;
    };

    void gather(Kind kind, const u8* data, std::size_t bytes);
    void give(Command&& command);
    void run(Command& command);
    void loop();

    ps2::Graphics& graphics_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable work_, done_;
    std::deque<Command> queue_;
    std::vector<std::vector<u8>> spare_;  // buffers to use again
    bool busy_ = false, quit_ = false;
    unsigned presents_waiting_ = 0;

    Command open_;  // what `vif` and `gif` have gathered since the last `send`
    bool open_used_ = false;

    std::mutex picture_mutex_;
    ps2::Image picture_, spare_picture_;
    bool picture_shown_ = false;
};

}  // namespace sys
