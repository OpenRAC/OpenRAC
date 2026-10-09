// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "gif.h"

#include <algorithm>

#include "fp_quad.h"

namespace ps2 {
namespace {

enum : u32 {
    kPacked = 0,
    kReglist = 1,
    kImage = 2,
    kImage2 = 3
};

}  // namespace

void Gif::reset() {
    paths_ = {};
}

void Gif::advance(Path& p) {
    if (++p.reg == p.nreg) {
        p.reg = 0;
        p.loops--;
    }
}

void Gif::packed(Path& p, u64 lo, u64 hi) {
    u32 desc = static_cast<u32>((p.regs >> (p.reg * 4)) & 0xF);
    switch (desc) {
        case gsreg::PRIM: gs_.write(gsreg::PRIM, lo & 0x7FF); break;
        case gsreg::RGBAQ: {
            // One colour component per word; Q is the one the last ST carried.
            u64 rgba = (lo & 0xFF) | ((lo >> 32 & 0xFF) << 8) | ((hi & 0xFF) << 16)
                       | ((hi >> 32 & 0xFF) << 24);
            gs_.write(gsreg::RGBAQ, rgba | (static_cast<u64>(as_u32(p.q)) << 32));
            break;
        }
        case gsreg::ST:
            gs_.write(gsreg::ST, lo);
            p.q = as_float(static_cast<u32>(hi));
            break;
        case gsreg::UV: gs_.write(gsreg::UV, (lo & 0x3FFF) | ((lo >> 32 & 0x3FFF) << 16)); break;
        case gsreg::XYZF2: {
            u64 xy = (lo & 0xFFFF) | ((lo >> 32 & 0xFFFF) << 16);
            u64 z = (hi >> 4) & 0xFFFFFF, f = (hi >> 36) & 0xFF;
            bool no_kick = (hi >> 47) & 1;
            gs_.write(no_kick ? gsreg::XYZF3 : gsreg::XYZF2, xy | (z << 32) | (f << 56));
            break;
        }
        case gsreg::XYZ2: {
            u64 xy = (lo & 0xFFFF) | ((lo >> 32 & 0xFFFF) << 16);
            bool no_kick = (hi >> 47) & 1;
            gs_.write(no_kick ? gsreg::XYZ3 : gsreg::XYZ2, xy | ((hi & 0xFFFFFFFFu) << 32));
            break;
        }
        case gsreg::FOG: gs_.write(gsreg::FOG, ((hi >> 36) & 0xFF) << 56); break;
        case 0xE:  // A+D: any register, by address
            gs_.write(static_cast<u8>(hi & 0xFF), lo);
            break;
        case 0xF: break;
        default: gs_.write(static_cast<u8>(desc), lo); break;
    }
}

namespace {
// How much is copied before the thread is given it, and how far ahead of the
// thread the writer may get.
constexpr std::size_t kChunkBytes = 64 * 1024, kMostQueuedBytes = 16 * 1024 * 1024;
}  // namespace

Gif::~Gif() {
    if (thread_.joinable()) {
        hand_over();
        {
            std::lock_guard lock(mutex_);
            quit_ = true;
        }
        work_.notify_all();
        thread_.join();
    }
}

void Gif::start() {
    if (!thread_.joinable()) {
        thread_ = std::thread([this] { loop(); });
    }
}

std::size_t Gif::packet_quadwords(const u8* memory, u32 at, u32 mask) {
    std::size_t total = 0;
    while (total <= mask) {
        u64 lo = load<u64>(memory + ((at + total) & mask) * 16);
        total++;
        u64 loops = bits(lo, 0, 15), nreg = bits(lo, 60, 4);
        if (nreg == 0) {
            nreg = 16;
        }
        switch (bits(lo, 58, 2)) {
            case kPacked: total += loops * nreg; break;
            case kReglist: total += (loops * nreg + 1) / 2; break;
            default: total += loops; break;
        }
        if (bits(lo, 15, 1)) {
            break;
        }
    }
    return std::min<std::size_t>(total, std::size_t{mask} + 1);
}

void Gif::write(int path, const u8* data, std::size_t quadwords) {
    if (!thread_.joinable()) {
        take(path, data, quadwords);
        return;
    }
    if (quadwords == 0) {
        return;
    }
    std::size_t at = open_.data.size();
    open_.data.insert(open_.data.end(), data, data + quadwords * 16);
    if (!open_.pieces.empty() && open_.pieces.back().path == path) {
        open_.pieces.back().quadwords += quadwords;
    } else {
        open_.pieces.push_back(Piece{path, at, quadwords});
    }
    if (open_.data.size() >= kChunkBytes) {
        hand_over();
    }
}

void Gif::run(std::function<void()> what) {
    if (!thread_.joinable()) {
        what();
        return;
    }
    open_.then = std::move(what);
    hand_over();
}

void Gif::hand_over() {
    if (open_.data.empty() && !open_.then) {
        return;
    }
    {
        std::unique_lock lock(mutex_);
        done_.wait(lock, [this] { return queued_bytes_ < kMostQueuedBytes; });
        queued_bytes_ += open_.data.size();
        queue_.push_back(std::move(open_));
        if (!spare_.empty()) {
            open_ = std::move(spare_.back());
            spare_.pop_back();
        } else {
            open_ = Chunk{};
        }
    }
    work_.notify_one();
}

void Gif::sync() {
    if (!thread_.joinable()) {
        return;
    }
    hand_over();
    std::unique_lock lock(mutex_);
    done_.wait(lock, [this] { return queue_.empty() && !busy_; });
}

void Gif::loop() {
    std::unique_lock lock(mutex_);
    for (;;) {
        work_.wait(lock, [this] { return quit_ || !queue_.empty(); });
        if (queue_.empty()) {
            return;
        }
        Chunk chunk = std::move(queue_.front());
        queue_.pop_front();
        busy_ = true;
        lock.unlock();

        for (const Piece& piece : chunk.pieces) {
            take(piece.path, chunk.data.data() + piece.at, piece.quadwords);
        }
        if (chunk.then) {
            chunk.then();
            chunk.then = nullptr;
        }

        lock.lock();
        busy_ = false;
        queued_bytes_ -= chunk.data.size();
        chunk.data.clear();
        chunk.pieces.clear();
        if (spare_.size() < 16) {
            spare_.push_back(std::move(chunk));
        }
        done_.notify_all();
    }
}

void Gif::take(int path, const u8* data, std::size_t quadwords) {
    fp::want_nearest();  // the GS computes as the host does; a vector unit may have been running
    Path& p = paths_[path - 1];
    while (quadwords) {
        if (p.loops == 0) {
            // A tag.
            u64 lo = load<u64>(data), hi = load<u64>(data + 8);
            data += 16;
            quadwords--;
            p.loops = static_cast<u32>(bits(lo, 0, 15));
            p.eop = bits(lo, 15, 1) != 0;
            p.flg = static_cast<u32>(bits(lo, 58, 2));
            p.nreg = static_cast<u32>(bits(lo, 60, 4));
            if (p.nreg == 0) {
                p.nreg = 16;
            }
            p.regs = hi;
            p.reg = 0;
            p.q = 1.0f;
            if (p.flg == kPacked && bits(lo, 46, 1)) {
                gs_.write(gsreg::PRIM, bits(lo, 47, 11));
            }
            p.in_packet = !(p.loops == 0 && p.eop);
            continue;
        }

        switch (p.flg) {
            case kPacked:
                packed(p, load<u64>(data), load<u64>(data + 8));
                advance(p);
                data += 16;
                quadwords--;
                break;
            case kReglist:
                for (int half = 0; half < 2 && p.loops; half++) {
                    u32 desc = static_cast<u32>((p.regs >> (p.reg * 4)) & 0xF);
                    if (desc < 0xE) {
                        gs_.write(static_cast<u8>(desc), load<u64>(data + half * 8));
                    }
                    advance(p);
                }
                data += 16;
                quadwords--;
                break;
            default: {
                std::size_t n = std::min<std::size_t>(p.loops, quadwords);
                gs_.transfer_in(data, n * 16);
                p.loops -= static_cast<u32>(n);
                data += n * 16;
                quadwords -= n;
                break;
            }
        }
        if (p.loops == 0) {
            p.in_packet = !p.eop;
        }
    }
}

}  // namespace ps2
