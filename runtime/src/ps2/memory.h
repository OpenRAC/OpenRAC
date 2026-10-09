// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

/**
 * The memory a game program sees: main memory and the scratchpad.
 *
 * Every address a program uses is turned into a pointer to host memory by masking, so an address
 * the program got wrong lands somewhere inside the two buffers and never outside them.
 * Memory-mapped registers and the other segments of the address space are not modelled here.
 *
 * Sources: the EE's memory sizes as publicly documented.
 */

#pragma once

#include <vector>

#include "types.h"

namespace ps2 {

/**
 * The memory a game program sees: 32 MB of main memory and the 16 KB scratchpad. Every address the
 * game stores is an offset into one of them.
 */
class GuestMemory {
public:
    /** Size of main memory in bytes: 32 MB (documented). */
    static constexpr u32 kRamBytes = 32 * 1024 * 1024;

    /** Size of the scratchpad in bytes: 16 KB (documented). */
    static constexpr u32 kScratchpadBytes = 16 * 1024;

    /** Allocates both memories, zero-filled. */
    GuestMemory() : ram_(kRamBytes), scratchpad_(kScratchpadBytes) {}

    /**
     * Turns a DMA address into a host pointer.
     *
     * Bit 31 selects the scratchpad (the SPR flag of a tag or of MADR), otherwise the address is a
     * main memory address in any segment. A scratchpad address is rounded down to a quadword.
     *
     * @param address The address as it sits in a tag or in MADR, SPR flag included.
     * @return A pointer into the scratchpad or into main memory.
     */
    u8* dma(u32 address) {
        // SPR flag: the address is an offset into the scratchpad.
        if (address & 0x80000000u) {
            return scratchpad_.data() + (address & (kScratchpadBytes - 1) & ~0xFu);
        }
        return ram_.data() + (address & (kRamBytes - 1));
    }

    /**
     * Turns a main memory address into a host pointer.
     *
     * @param address An address in any segment; the bits above the memory size are dropped.
     * @return A pointer into main memory.
     */
    u8* ram(u32 address) { return ram_.data() + (address & (kRamBytes - 1)); }

    /**
     * Turns a scratchpad offset into a host pointer.
     *
     * @param offset Byte offset in the scratchpad; the bits above its size are dropped.
     * @return A pointer into the scratchpad.
     */
    u8* scratchpad(u32 offset) { return scratchpad_.data() + (offset & (kScratchpadBytes - 1)); }

    /**
     * Reads a value from main memory.
     *
     * @tparam T The type to read, usually an integer.
     * @param address A main memory address; it need not be aligned.
     * @return The value at that address.
     */
    template <typename T>
    T read(u32 address) {
        return load<T>(ram(address));
    }

    /**
     * Writes a value to main memory.
     *
     * @tparam T The type to write, usually an integer.
     * @param address A main memory address; it need not be aligned.
     * @param value The value to store.
     */
    template <typename T>
    void write(u32 address, T value) {
        store<T>(ram(address), value);
    }

private:
    /** Main memory. */
    std::vector<u8> ram_;

    /** The scratchpad. */
    std::vector<u8> scratchpad_;
};

}  // namespace ps2
