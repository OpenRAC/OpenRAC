// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

/**
 * Loading a library of host functions and running them in place of guest functions (see native.h).
 */

#include "sys/native.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>

#include "ps2/fp.h"

namespace sys {
namespace {

/** The most instructions the retail side of a checked call may take before the check gives up. */
constexpr u64 kCheckInstructions = 2'000'000;

/**
 * The CRC-32 of a run of bytes (the common one: polynomial 0xEDB88320, as zlib computes it).
 *
 * @param data The bytes.
 * @param bytes How many.
 * @return The checksum.
 */
u32 crc32(const u8* data, std::size_t bytes) {
    u32 crc = 0xFFFFFFFFu;

    for (std::size_t n = 0; n < bytes; n++) {
        crc ^= data[n];

        // One bit at a time; the functions are short and each is checked rarely.
        for (unsigned bit = 0; bit < 8; bit++) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1)));
        }
    }

    return ~crc;
}

}  // namespace

Native::Native(ps2::Ee& ee, ps2::GuestMemory& memory) : ee_(ee), memory_(memory) {
    stack_pointer_ = kStackTop;

    host_.abi = OPENRAC_NATIVE_ABI;
    host_.context = this;
    host_.space = memory_.space();
    host_.stack_pointer = &stack_pointer_;
    host_.get_r = get_r;
    host_.set_r = set_r;
    host_.get_f = get_f;
    host_.set_f = set_f;
    host_.call = call;
    host_.hardware_read = hardware_read;
    host_.hardware_write = hardware_write;
    host_.allocate = allocate;
    host_.float_op = float_op;
    host_.float_compare = float_compare;
}

bool Native::load(const std::string& path, std::string* error) {
    // Host code uses guest addresses as offsets from one base.
    if (!memory_.space()) {
        *error = "this host cannot map the guest's address space";
        return false;
    }

    void* library = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);

    // Not a library this host can load.
    if (!library) {
        *error = dlerror();
        return false;
    }

    using Entry = const OpenracNativeLibrary* (*)();
    Entry entry = reinterpret_cast<Entry>(dlsym(library, "openrac_native_library"));
    const OpenracNativeLibrary* found = entry ? entry() : nullptr;

    // Some other library, or one built for another version of the contract.
    if (!found || found->abi != OPENRAC_NATIVE_ABI) {
        *error = "it is not a library of host functions for this runtime";
        return false;
    }

    found->start(&host_);
    marks_.assign(ps2::GuestMemory::kRamBytes / 4, 0);

    for (u32 n = 0; n < found->count; n++) {
        const OpenracNativeFunction& function = found->functions[n];

        // Only code in main memory can be stood in for; the first function for an address wins.
        if (function.address >= ps2::GuestMemory::kRamBytes
            || by_address_.count(function.address)) {
            continue;
        }

        // A tool asked what is at this place of the table.
        if (name_at >= 0 && static_cast<u32>(name_at) == n) {
            std::fprintf(stderr, "native function %u is %s\n", n, function.name);
        }

        // Left out by the owner.
        if (n < first || n >= last || skip.count(function.name)) {
            continue;
        }

        by_address_[function.address] = functions_.size();
        Function added;

        added.entry = function;
        functions_.push_back(added);
        marks_[function.address >> 2] = 1;
    }

    ee_.native_marks = marks_.data();
    ee_.on_native = [this](u32 address) {
        return enter(address);
    };

    return true;
}

void Native::code_changed() {
    for (Function& function : functions_) {
        function.state = State::Unchecked;
    }

    bound_ = 0;
}

void Native::report(std::FILE* out, std::size_t most) const {
    std::vector<const Function*> busiest;
    std::size_t differing = 0;
    u64 compared = 0;
    u64 not_compared = 0;

    for (const Function& function : functions_) {
        compared += function.checked;
        not_compared += function.unchecked;
        differing += function.differs.empty() ? 0 : 1;

        // Never ran.
        if (function.calls == 0) {
            continue;
        }

        busiest.push_back(&function);
    }

    std::sort(busiest.begin(), busiest.end(), [](const Function* a, const Function* b) {
        return a->calls > b->calls;
    });

    std::fprintf(
        out,
        "host code: %zu functions in the library, %zu ran, %llu calls\n",
        functions_.size(),
        busiest.size(),
        static_cast<unsigned long long>(calls)
    );

    // What the comparison with the retail code found, when it was asked for.
    if (check) {
        std::fprintf(
            out,
            "  compared with the retail code: %llu calls, %zu functions differ; %llu calls reached "
            "outside memory and were not compared\n",
            static_cast<unsigned long long>(compared),
            differing,
            static_cast<unsigned long long>(not_compared)
        );

        for (const Function& function : functions_) {
            // The same as the retail code, or never compared.
            if (function.differs.empty()) {
                continue;
            }

            std::fprintf(
                out,
                "  differs  %08x  %s: %s\n",
                function.entry.address,
                function.entry.name,
                function.differs.c_str()
            );
        }
    }

    for (std::size_t n = 0; n < busiest.size() && n < most; n++) {
        std::fprintf(
            out,
            "  %10llu  %08x  %s\n",
            static_cast<unsigned long long>(busiest[n]->calls),
            busiest[n]->entry.address,
            busiest[n]->entry.name
        );
    }
}

bool Native::enter(u32 address) {
    auto found = by_address_.find(address & (ps2::GuestMemory::kRamBytes - 1));

    // A mark with no function: not reached, the marks are made from the functions.
    if (found == by_address_.end()) {
        return false;
    }

    Function& function = functions_[found->second];

    // First time here since code changed: is this the function the host code was written from?
    if (function.state == State::Unchecked) {
        function.state = matches(function) ? State::Bound : State::Other;
        bound_ += function.state == State::Bound ? 1 : 0;
    }

    // Other code is at this address now.
    if (function.state != State::Bound) {
        return false;
    }

    function.calls++;
    calls++;

    // One of this function's first calls, and not inside another checked call: compare.
    if (function.checked + function.unchecked < check && !checking_) {
        check_call(function);
        return true;
    }

    function.entry.entry();

    return true;
}

void Native::take(Snapshot& to) {
    to.gpr = ee_.gpr;
    to.fpr = ee_.fpr;
    to.hi = ee_.hi;
    to.lo = ee_.lo;
    to.hi1 = ee_.hi1;
    to.lo1 = ee_.lo1;
    to.sa = ee_.sa;
    to.facc = ee_.facc;
    to.fcr31 = ee_.fcr31;
    to.ram.assign(memory_.ram(0), memory_.ram(0) + ps2::GuestMemory::kRamBytes);
    to.scratchpad
        .assign(memory_.scratchpad(0), memory_.scratchpad(0) + ps2::GuestMemory::kScratchpadBytes);
}

void Native::put_back(const Snapshot& from) {
    ee_.gpr = from.gpr;
    ee_.fpr = from.fpr;
    ee_.hi = from.hi;
    ee_.lo = from.lo;
    ee_.hi1 = from.hi1;
    ee_.lo1 = from.lo1;
    ee_.sa = from.sa;
    ee_.facc = from.facc;
    ee_.fcr31 = from.fcr31;
    std::copy(from.ram.begin(), from.ram.end(), memory_.ram(0));
    std::copy(from.scratchpad.begin(), from.scratchpad.end(), memory_.scratchpad(0));
}

void Native::check_call(Function& function) {
    u32 address = function.entry.address;
    u64 outside_before = outside;
    u64 event_before = ee_.event_at;

    checking_ = true;
    take(before_);

    /*
     * The retail function first, with the mark off so that the interpreter runs it, and with no
     * timed event inside it: an event would happen in one run and not in the other.
     */
    ee_.event_at = ~u64{0};
    marks_[address >> 2] = 0;

    ps2::Ee::Call call = ee_.begin_call(address);
    bool returned = ee_.run_call(kCheckInstructions);

    /*
     * It called a library function, touched a device, or is still running (waiting for an
     * interrupt, say). None of that can be done a second time: this is the call itself, let it
     * finish with events on again.
     */
    if (!returned || outside != outside_before) {
        ee_.event_at = event_before;

        // Still running: until it returns.
        if (!returned) {
            ee_.run_call(~u64{0});
        }

        ee_.end_call(call);
        marks_[address >> 2] = 1;
        function.unchecked++;
        checking_ = false;
        return;
    }

    ee_.end_call(call);
    marks_[address >> 2] = 1;
    take(retail_result_);

    // The same call again, by the host function.
    put_back(before_);
    function.entry.entry();
    take(host_result_);

    // What stays is what the retail code left.
    put_back(retail_result_);
    ee_.event_at = event_before;
    checking_ = false;
    function.checked++;

    char text[160] = "";
    u32 sp = static_cast<u32>(before_.gpr[29].lo) & (ps2::GuestMemory::kRamBytes - 1);

    // The result, by what the function returns.
    if (function.entry.result == 1 && host_result_.gpr[2].lo != retail_result_.gpr[2].lo) {
        std::snprintf(
            text,
            sizeof(text),
            "returns %llx, the retail code %llx",
            static_cast<unsigned long long>(host_result_.gpr[2].lo),
            static_cast<unsigned long long>(retail_result_.gpr[2].lo)
        );
    } else if (function.entry.result == 2 && host_result_.fpr[0] != retail_result_.fpr[0]) {
        std::snprintf(
            text,
            sizeof(text),
            "returns float bits %08x, the retail code %08x",
            host_result_.fpr[0],
            retail_result_.fpr[0]
        );
    }

    const u8* retail = retail_result_.ram.data();

    /*
     * Memory, but for the megabyte below the caller's stack pointer: the retail function's own
     * frame and its callees' are there, and host code keeps its locals elsewhere.
     */
    for (u32 at = 0; at < ps2::GuestMemory::kRamBytes && !text[0]; at += 8) {
        // The same eight bytes.
        if (std::memcmp(retail + at, host_result_.ram.data() + at, 8) == 0) {
            continue;
        }

        // Dead stack.
        if (at < sp && sp - at <= 0x100000) {
            continue;
        }

        // Host code's own data and stack, which the retail function never touches.
        if (at >= kDataBase && at < kStackTop) {
            continue;
        }

        u64 ours = 0;
        u64 theirs = 0;

        std::memcpy(&ours, host_result_.ram.data() + at, 8);
        std::memcpy(&theirs, retail + at, 8);
        std::snprintf(
            text,
            sizeof(text),
            "leaves %016llx at %08x, the retail code %016llx",
            static_cast<unsigned long long>(ours),
            at,
            static_cast<unsigned long long>(theirs)
        );
    }

    bool same_scratchpad = std::memcmp(
                               retail_result_.scratchpad.data(),
                               host_result_.scratchpad.data(),
                               ps2::GuestMemory::kScratchpadBytes
                           )
                           == 0;

    // The scratchpad, whole.
    if (!text[0] && !same_scratchpad) {
        std::snprintf(text, sizeof(text), "leaves the scratchpad different from the retail code");
    }

    // It differs: the interpreter runs this function from now on.
    if (text[0]) {
        function.differs = text;
        function.state = State::Other;
        bound_--;
    }
}

bool Native::matches(const Function& function) {
    u32 address = function.entry.address;
    u32 size = function.entry.size;

    // A function that would run past the end of memory is not there.
    if (size == 0 || address + size > ps2::GuestMemory::kRamBytes) {
        return false;
    }

    return crc32(memory_.ram(address), size) == function.entry.crc;
}

uint64_t Native::get_r(void* context, int n) {
    return static_cast<Native*>(context)->ee_.gpr[static_cast<std::size_t>(n) & 31].lo;
}

void Native::set_r(void* context, int n, uint64_t value) {
    static_cast<Native*>(context)->ee_.gpr[static_cast<std::size_t>(n) & 31].lo = value;
}

uint32_t Native::get_f(void* context, int n) {
    return static_cast<Native*>(context)->ee_.fpr[static_cast<std::size_t>(n) & 31];
}

void Native::set_f(void* context, int n, uint32_t bits) {
    static_cast<Native*>(context)->ee_.fpr[static_cast<std::size_t>(n) & 31] = bits;
}

void Native::call(void* context, uint32_t address) {
    Native* self = static_cast<Native*>(context);

    // Another host function, or the interpreter. Either way the registers carry the call.
    if (!self->enter(address)) {
        self->ee_.run_function(address);
    }
}

uint64_t Native::hardware_read(void* context, uint32_t address, uint32_t bytes) {
    ps2::Ee& ee = static_cast<Native*>(context)->ee_;

    // By width, through the core, which asks the machine's register model.
    switch (bytes) {
        case 1:
            return ee.read8(address);

        case 2:
            return ee.read16(address);

        case 4:
            return ee.read32(address);

        default:
            // 8 bytes: the widest a host function reads at once.
            return ee.read64(address);
    }
}

void Native::hardware_write(void* context, uint32_t address, uint32_t bytes, uint64_t value) {
    ps2::Ee& ee = static_cast<Native*>(context)->ee_;

    // By width, through the core, which tells the machine's register model.
    switch (bytes) {
        case 1:
            ee.write8(address, static_cast<u8>(value));
            break;

        case 2:
            ee.write16(address, static_cast<ps2::u16>(value));
            break;

        case 4:
            ee.write32(address, static_cast<u32>(value));
            break;

        default:
            // 8 bytes: the widest a host function writes at once.
            ee.write64(address, value);
            break;
    }
}

uint32_t Native::float_op(uint32_t op, uint32_t a, uint32_t b) {
    // The model reports overflow and the like; host code has nowhere to keep that.
    u32 unused = 0;

    switch (op) {
        case 0:
            return ps2::fp::add(a, b, unused);

        case 1:
            return ps2::fp::sub(a, b, unused);

        case 2:
            return ps2::fp::mul(a, b, unused);

        case 3:
            return ps2::fp::div(a, b, unused);

        case 4:
            return ps2::fp::sqrt(a, unused);

        case 5:
            // The integer `a` as a float.
            return ps2::fp::from_int(static_cast<ps2::s32>(a));

        default:
            // 6: the float `a` as an integer, cut towards zero and kept in range.
            return static_cast<u32>(ps2::fp::to_int(a));
    }
}

int32_t Native::float_compare(uint32_t a, uint32_t b) {
    ps2::s64 left = ps2::fp::key(a);
    ps2::s64 right = ps2::fp::key(b);

    // Below, equal, above.
    if (left < right) {
        return -1;
    }

    return left > right ? 1 : 0;
}

uint32_t Native::allocate(void* context, uint32_t bytes, uint32_t alignment) {
    Native* self = static_cast<Native*>(context);
    u32 align = std::max<u32>(alignment, 16);
    u32 at = (self->next_data_ + align - 1) & ~(align - 1);

    // The data area is full: the decompilations keep next to no data of their own, so a bug.
    if (at > kDataEnd || bytes > kDataEnd - at) {
        std::fprintf(stderr, "openrac: no room for host code's data\n");
        std::abort();
    }

    self->next_data_ = at + bytes;

    return at;
}

}  // namespace sys
