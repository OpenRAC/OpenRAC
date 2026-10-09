// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <array>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "types.h"

namespace ps2 {

// A vector unit running microprograms: VU1 behind VIF1, or VU0 when the EE
// starts a microprogram on it. It is an interpreter that keeps the timing
// microprograms depend on: the upper and lower instruction of a pair see the
// same state; a float register written by one instruction can be read four
// cycles later, and an instruction that needs it sooner waits; flags appear
// four cycles after the instruction that set them; Q and P arrive when their
// dividers and function units finish; a branch tests the value an integer
// register had before the instruction just ahead of it, unless that
// instruction read flags into it or the branch had to wait; XGKICK sends its
// packet one instruction late. Arithmetic is the
// console's own (fp.h), on raw bit patterns.
class Vu {
public:
    struct Memory {
        u8* micro = nullptr;  // program memory, 8 bytes an instruction
        u32 micro_bytes = 0;  // a power of two: 16 KB for VU1, 4 KB for VU0
        u8* data = nullptr;   // data memory, 16 bytes a quadword
        u32 data_bytes = 0;
    };

    explicit Vu(Memory memory);

    void reset();

    // Run from instruction `address` until an instruction with the E bit has
    // finished (with the one after it), or `limit` instructions have run.
    // Returns how many ran. `resume` continues after the last stop.
    u64 run(u32 address, u64 limit = u64{1} << 26);
    u64 resume(u64 limit = u64{1} << 26);

    bool stopped() const { return !running_; }

    // Skip working out MAC and status flags that no instruction of the loaded
    // programs can read. What a program leaves behind when it ends is always
    // worked out. Safe for VU1, whose flags nothing outside it reads, and for
    // VU0 as long as the EE reads no status flags (`status_was_read`).
    bool skip_unread_flags = false;
    // Something outside read the status flags, whose remembering bits depend
    // on every instruction: nothing is skipped from now on.
    void status_was_read();
    // Program memory was written: forget what was worked out about it.
    void program_changed();

    // Running beside the EE, which is how VU0 is used: start a program, then
    // let it run a number of instructions at a time. `advance_to_sync` runs
    // until an instruction with the M bit (a point the program marks for the
    // EE to wait for) or the end.
    void start(u32 address);
    u64 advance(u64 instructions);
    u64 advance_to_sync(u64 limit);

    // One instruction given by the EE (a COP2 operation): it runs at once and
    // the EE waits for its result, so nothing is left in flight afterwards.
    void macro(u32 code);
    // The control registers the EE reads and writes with CFC2 and CTC2:
    // 0-15 the integer registers, 16 status, 17 MAC, 18 clip, 20 R, 21 I,
    // 22 Q, 26 TPC, 27 CMSAR0, 28 FBRST, 29 VPU-STAT, 31 CMSAR1.
    u32 control(unsigned reg) const;
    void set_control(unsigned reg, u32 value);

    // XGKICK: a GIF packet starts at this quadword of data memory.
    std::function<void(u32 quadword)> on_kick;
    // For looking into a program: called before each pair runs.
    std::function<void(u32 at, u32 upper, u32 lower)> on_step;
    // XTOP and XITOP read VIF registers.
    std::function<u32()> on_top, on_itop;

    // Registers. A float register is four raw 32-bit values, x first, so that
    // integers moved through them survive. VF0 is (0, 0, 0, 1) and VI0 is 0.
    std::array<std::array<u32, 4>, 32> vf{};
    std::array<u16, 16> vi{};
    std::array<u32, 4> acc{};
    u32 q = 0, p = 0, i = 0, r = 0;
    u32 mac = 0, status = 0, clip = 0;  // as instructions read them now
    u32 pc = 0;                         // in instructions

    float f(unsigned reg, unsigned field) const { return as_float(vf[reg][field]); }

    void set_f(unsigned reg, unsigned field, float value) { vf[reg][field] = as_u32(value); }

    u64 unknown_ops = 0;  // instruction words that decode to nothing

private:
    struct Flags {
        u32 mac = 0, status = 0, clip = 0;
        u64 at = 0;  // the cycle from which instructions see them
    };

    // The function for one instruction (see vu.cpp, at the end).
    using UpperRun = void (*)(Vu&, u32 code);
    using LowerRun = void (*)(Vu&, u32 code, u32 at);

    // What a pair reads that can make it wait, and how it is run, worked out
    // once per pair.
    struct Needs {
        UpperRun upper_run = nullptr;
        LowerRun lower_run = nullptr;
        u32 up = 0, low = 0;
        bool known = false;
        u8 count = 0;
        u8 reg[4] = {0, 0, 0, 0}, mask[4] = {0, 0, 0, 0};
        u8 wait = 0;               // 1: the divider must be free, 2: the function unit must be done
        bool flags_wanted = true;  // something can read the flags its upper instruction sets
        bool upper_nop = false, lower_nop = false;
        // The lower instruction may read or write the float register the upper
        // one writes, so the two have to be kept apart with care.
        bool together = true;
        // None of the three pairs before it in memory writes a float register it
        // reads: coming to it in a straight line, it never waits for one.
        bool no_wait = false;
    };
    enum class Op {
        Add,
        Sub,
        Mul,
        Madd,
        Msub
    };
    enum class From {
        Ft,
        Bc,
        Q,
        I
    };

    void step();
    void upper(u32 code);
    void lower(u32 code, u32 at);
    void upper_special(u32 code);
    void lower_special(u32 code);
    void upper_body(u32 code, u32 fn);
    void upper_special_body(u32 code, u32 fn);
    void lower_body(u32 code, u32 at, u32 op);
    void lower_special_body(u32 code, u32 fn, u32 index);
    template <unsigned Slot>
    static void upper_as(Vu& vu, u32 code);
    template <unsigned Slot>
    static void lower_as(Vu& vu, u32 code, u32 at);
    template <std::size_t... N>
    static constexpr std::array<UpperRun, sizeof...(N)> upper_runs(std::index_sequence<N...>);
    template <std::size_t... N>
    static constexpr std::array<LowerRun, sizeof...(N)> lower_runs(std::index_sequence<N...>);
    static const std::array<UpperRun, 192> kUpperRuns;
    static const std::array<LowerRun, 320> kLowerRuns;

    template <Op op, From from, bool to_acc>
    void arith(u32 code);
    void min_max(u32 code, From from, bool max);
    u32 operand(u32 code, From from, unsigned field) const;
    u32 result(u32 value, u32 problems, unsigned field, u32& flags) const;
    void post_flags(u32 mac_bits);
    void post();
    void settle();
    void finish_q();
    void fire_kick();

    void write_vf(unsigned reg, u32 mask, const std::array<u32, 4>& value);
    void about_to_write_vf(unsigned reg, u32 mask);
    void write_vi(unsigned reg, u16 value);
    void write_vi_from_flags(unsigned reg, u16 value);
    u16 branch_vi(unsigned reg) const;
    void branch(u32 target);
    void start_q(u32 value, unsigned latency, u32 divide_flags);
    void work_out(Needs& needs, u32 up, u32 low) const;
    bool never_waits(const Needs& needs, u32 at) const;
    bool flags_can_be_read(u32 at) const;
    void look_at_programs();
    void start_p(double value, unsigned latency);

    u8* quad(u32 address) { return memory_.data + ((address * 16) & (memory_.data_bytes - 1)); }

    Memory memory_;
    u32 pc_mask_ = 0;
    bool running_ = false, sync_point_ = false;

    // The upper instruction's register write, held back while the lower one runs.
    bool in_upper_ = false;
    unsigned upper_reg_ = 0;
    std::array<u32, 4> upper_old_{};

    // Time, in cycles: one an instruction, more when an instruction waits.
    u64 cycle_ = 0;
    bool timed_ = false;  // a microprogram is running (the EE's own instructions are not timed)
    // The cycle from which each field of each float register can be read.
    std::array<std::array<u64, 4>, 32> readable_{};
    std::array<u64, 32> register_ready_{};  // the latest of a register's four

    // What has been worked out about the pairs of program memory, kept for
    // each content the memory has had: the games swap a few programs in and
    // out all the time, and each comes back as it was.
    struct Image {
        std::vector<u8> micro;
        std::vector<Needs> needs;
        bool looked_at = false, sticky_readers = true;
    };

    std::unordered_map<u64, std::unique_ptr<Image>> images_;
    Image* image_ = nullptr;
    Needs* needs_ = nullptr;      // of the image in use
    bool program_dirty_ = true;   // program memory may differ from the image in use
    bool sticky_readers_ = true;  // of the image in use
    void choose_image();
    bool flags_wanted_ = true;  // for the instruction being run

    std::array<Flags, 8> flag_pipe_{};
    unsigned flag_first_ = 0, flag_count_ = 0;
    u32 mac_latest_ = 0, status_latest_ = 0, clip_latest_ = 0;  // the newest values, visible or not

    u32 q_next_ = 0, q_flags_ = 0, p_next_ = 0;
    u64 q_at_ = 0, p_at_ = 0;  // the cycle the result arrives, 0 when none is on its way

    unsigned branch_in_ = 0, stop_in_ = 0, kick_in_ = 0;
    u64 straight_ = 0;  // pairs run since the program was started or a branch was taken
    u32 branch_target_ = 0, kick_address_ = 0;

    unsigned backup_reg_ = 0, backup_ttl_ = 0;
    u16 backup_value_ = 0;
    u32 cmsar0_ = 0, fbrst_ = 0;
};

}  // namespace ps2
