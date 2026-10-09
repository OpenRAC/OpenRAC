// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

/**
 * Host code in place of guest functions: loading a library built from a game's decompiled C and
 * running its functions where the retail program would run its own.
 *
 * This is how the port grows out of the interpreter (docs/DESIGN.md, route C): every decompiled
 * function that is compiled for the host and proven against the retail one takes that much away
 * from what is interpreted. The library is built on the user's machine by runtime/port; the
 * runtime ships no game code.
 */

#pragma once

#include <array>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "ps2/ee.h"
#include "ps2/memory.h"
#include "sys/native_abi.h"

namespace sys {

using ps2::u32;
using ps2::u64;
using ps2::u8;

/**
 * A loaded library of host functions and which guest addresses they stand in for right now.
 *
 * A function is bound to its address only while the guest's memory there holds the retail code
 * the function was written from: the bytes are checked against the library's checksum the first
 * time the address is reached, and again after the program says it changed code (a level
 * loading). So a function of another level, or of another version of the game, is never run.
 *
 * Only the thread that runs the EE uses it.
 */
class Native {
public:
    /**
     * Where host code keeps its own data and its stack: in the first megabyte of main memory,
     * which is the kernel's on the console and holds nothing here.
     *
     * They are in main memory, not beside it, because a function's local buffer is as good an
     * address as any to the game: it hands such buffers to the DMA controller and masks their
     * addresses like any other (seen in game code: a texture upload from a structure on the
     * stack). Data goes up from `kDataBase`; the stack goes down from `kStackTop` to
     * `kDataEnd`. Interrupt handlers have their own stack above (`Machine::kHandlerStack`).
     */
    static constexpr u32 kDataBase = 0x00080000;
    static constexpr u32 kDataEnd = 0x00090000;
    static constexpr u32 kStackTop = 0x000F0000;

    /**
     * Makes an empty set over a core and its memory.
     *
     * @param ee The core whose registers the functions use; it must outlive this object.
     * @param memory The guest's memory; it must outlive this object.
     */
    Native(ps2::Ee& ee, ps2::GuestMemory& memory);

    /**
     * Loads a library and makes its functions candidates for their addresses.
     *
     * @param path The library file.
     * @param[out] error Why it failed, when it did.
     * @return True if it is loaded.
     */
    bool load(const std::string& path, std::string* error);

    /** Forgets which functions are bound: each is checked again when its address is reached. */
    void code_changed();

    /** How many functions the library has, and how many are bound to their addresses now. */
    std::size_t functions() const { return functions_.size(); }

    std::size_t bound() const { return bound_; }

    /** How many times host code ran in place of a guest function. */
    u64 calls = 0;

    /**
     * Which of the library's functions are used, set before `load`: those whose place in the
     * library's table is from `first` up to but not including `last`, less those named in `skip`.
     * For finding the function that misbehaves by halving.
     */
    std::size_t first = 0;
    std::size_t last = ~std::size_t{0};
    std::set<std::string> skip;

    /** For that search: when not negative, `load` prints the name at this place of the table. */
    long name_at = -1;

    /**
     * How many of each function's first calls are checked against the retail code; 0 for none.
     *
     * A checked call runs the retail function in the interpreter, puts the machine back as it
     * was, runs the host function and compares what the two left in memory and in the result
     * register. A function that differs is handed back to the interpreter for good and listed
     * by `report`. The retail result is the one that stays.
     */
    unsigned check = 0;

    /**
     * Counted up by the owner whenever a guest function reaches outside the guest's memory: a
     * replaced library function, a kernel call, a device register. A call that did cannot be
     * run twice, so it is not checked.
     */
    u64 outside = 0;

    /**
     * Writes one line for each function that ran: how often, its address and its name.
     *
     * @param out Where to write.
     * @param most How many lines at most, the busiest first.
     */
    void report(std::FILE* out, std::size_t most) const;

private:
    /** What is known about one function's binding. */
    enum class State : u8 {
        Unchecked,  // Not compared with the guest's memory since code last changed.
        Bound,      // The guest's memory holds the retail function: host code runs.
        Other,      // Something else is there: the interpreter runs it.
    };

    /** One function of the library. */
    struct Function {
        OpenracNativeFunction entry;  // The library's record.
        State state = State::Unchecked;
        u64 calls = 0;           // Times it ran.
        unsigned checked = 0;    // Calls compared with the retail code so far.
        unsigned unchecked = 0;  // Calls that could not be compared: they reached outside.
        std::string differs;     // How it differed from the retail code; empty if it never did.
    };

    /** Everything a function may change that is compared or put back: registers and memory. */
    struct Snapshot {
        std::array<ps2::Ee::Reg, 32> gpr;  // The general registers.
        std::array<u32, 32> fpr;           // The FPU registers.
        u64 hi, lo, hi1, lo1;              // The multiply results.
        u32 sa, facc, fcr31;               // SA and the FPU's accumulator and flags.
        std::vector<u8> ram;               // Main memory.
        std::vector<u8> scratchpad;        // The scratchpad.
    };

    /**
     * Runs the host function for an address, if it is bound.
     *
     * @param address The address the program counter reached.
     * @return True if host code ran.
     */
    bool enter(u32 address);

    /**
     * Runs one call both ways and compares (see `check`).
     *
     * @param function The function; it is bound and its mark is set.
     */
    void check_call(Function& function);

    /**
     * Copies the registers and memories.
     *
     * @param[out] to Where to.
     */
    void take(Snapshot& to);

    /**
     * Puts registers and memories back from a copy.
     *
     * @param from The copy.
     */
    void put_back(const Snapshot& from);

    /**
     * Compares the guest's memory at a function's address with the code it stands in for.
     *
     * @param function The function.
     * @return True if the checksum of the bytes there is the library's.
     */
    bool matches(const Function& function);

    /** The callbacks the library is given; `context` is this object. */
    static uint64_t get_r(void* context, int n);
    static void set_r(void* context, int n, uint64_t value);
    static uint32_t get_f(void* context, int n);
    static void set_f(void* context, int n, uint32_t bits);
    static void call(void* context, uint32_t address);
    static uint64_t hardware_read(void* context, uint32_t address, uint32_t bytes);
    static void hardware_write(void* context, uint32_t address, uint32_t bytes, uint64_t value);
    static uint32_t allocate(void* context, uint32_t bytes, uint32_t alignment);
    static uint32_t float_op(uint32_t op, uint32_t a, uint32_t b);
    static int32_t float_compare(uint32_t a, uint32_t b);

    ps2::Ee& ee_;                                      // The core.
    ps2::GuestMemory& memory_;                         // The guest's memory.
    OpenracHost host_{};                               // What the library was given.
    std::vector<Function> functions_;                  // The library's functions.
    std::unordered_map<u32, std::size_t> by_address_;  // Index into `functions_` by guest address.
    std::vector<u8> marks_;      // One byte a word of main memory, for the core.
    std::size_t bound_ = 0;      // Functions in state `Bound`.
    u32 stack_pointer_ = 0;      // The host code's shared stack pointer.
    u32 next_data_ = kDataBase;  // Where the next module's data goes.
    bool checking_ = false;      // A call is being compared; calls inside it are not.
    Snapshot before_;            // The machine before a checked call.
    Snapshot host_result_;       // What the host function left.
    Snapshot retail_result_;     // What the retail function left.
};

}  // namespace sys
