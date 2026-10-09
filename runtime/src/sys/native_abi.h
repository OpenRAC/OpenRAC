/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/**
 * The contract between the runtime and a library of host code compiled from a game's decompiled C.
 *
 * Such a library holds decompiled functions built for the host (runtime/port). Each one stands in
 * for the retail function at one guest address, on the same memory image: its arguments and its
 * result pass through the EE's registers as the console's calling convention has them, so it can
 * be called by retail code and can call retail code, and neither side knows.
 *
 * This file is C, because the library's generated code is C; the runtime includes it too.
 */

#ifndef OPENRAC_SYS_NATIVE_ABI_H
#define OPENRAC_SYS_NATIVE_ABI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** The version of this contract; a library built for another one is refused. */
#define OPENRAC_NATIVE_ABI 1

/**
 * What the runtime gives a library: the guest's memory, the EE's registers, and a way to call a
 * guest function. Every function takes `context` back as its first argument.
 */
typedef struct OpenracHost {
    /** `OPENRAC_NATIVE_ABI` of the runtime. */
    uint32_t abi;

    /** The runtime's own pointer, handed back on every call. */
    void* context;

    /** The base of the guest's 4 GB address space: a guest address is an offset from it. */
    uint8_t* space;

    /**
     * The stack pointer all host code shares, a guest address in main memory. The stack grows
     * down; the library's code keeps it itself.
     */
    uint32_t* stack_pointer;

    /** Reads general register `n` of the EE (its low 64 bits). */
    uint64_t (*get_r)(void* context, int n);

    /** Writes the low 64 bits of general register `n`; a 32-bit value is passed sign-extended. */
    void (*set_r)(void* context, int n, uint64_t value);

    /** Reads FPU register `n` as its 32 bits. */
    uint32_t (*get_f)(void* context, int n);

    /** Writes FPU register `n` from 32 bits. */
    void (*set_f)(void* context, int n, uint32_t bits);

    /**
     * Calls the guest function at `address` with the registers as they are and returns when it
     * returns. The function may be retail code or another function of a library.
     */
    void (*call)(void* context, uint32_t address);

    /**
     * Reads or writes a hardware register: an address in the range the console maps its devices
     * at, which is not memory. `bytes` is 1, 2, 4 or 8; the value is in the low bytes.
     */
    uint64_t (*hardware_read)(void* context, uint32_t address, uint32_t bytes);
    void (*hardware_write)(void* context, uint32_t address, uint32_t bytes, uint64_t value);

    /**
     * The console's float arithmetic, on bit patterns. It is not the host's: there is no infinity
     * and no not-a-number, results are cut towards zero, and a division by zero gives the largest
     * number. Host code computes with it so that it leaves what the retail code leaves.
     *
     * `op` is 0 to add, 1 to subtract, 2 to multiply, 3 to divide, 4 for the square root of `a`,
     * 5 to turn the integer `a` into a float, 6 to turn the float `a` into an integer.
     */
    uint32_t (*float_op)(uint32_t op, uint32_t a, uint32_t b);

    /** Orders two floats as the console does: the sign of the result is that of `a` less `b`. */
    int32_t (*float_compare)(uint32_t a, uint32_t b);

    /**
     * Gives the library guest-addressable memory for a module's own data (strings, tables).
     *
     * @return Its guest address; the memory is zero-filled and is never taken back.
     */
    uint32_t (*allocate)(void* context, uint32_t bytes, uint32_t alignment);
} OpenracHost;

/** One decompiled function of a library. */
typedef struct OpenracNativeFunction {
    /** The guest address of the retail function it stands in for. */
    uint32_t address;

    /** The retail function's size in bytes, and the CRC-32 of those bytes as the game has them. */
    uint32_t size;
    uint32_t crc;

    /** Its name in the decompilation, for reports. */
    const char* name;

    /** What it returns: 0 nothing, 1 an integer (register 2), 2 a float (FPU register 0). */
    uint32_t result;

    /** Runs it: arguments from the EE's registers, the result into them. */
    void (*entry)(void);
} OpenracNativeFunction;

/** A library: what it was built from and its functions. */
typedef struct OpenracNativeLibrary {
    /** `OPENRAC_NATIVE_ABI` the library was built for. */
    uint32_t abi;

    /** The game version it was built from (`rac1/pal`). */
    const char* game;

    /** How many functions there are, and the functions. */
    uint32_t count;
    const OpenracNativeFunction* functions;

    /** Called once before any function: the library keeps `host` and sets its modules up. */
    void (*start)(const OpenracHost* host);
} OpenracNativeLibrary;

/** The one symbol a library exports. */
const OpenracNativeLibrary* openrac_native_library(void);

#ifdef __cplusplus
}
#endif

#endif /* OPENRAC_SYS_NATIVE_ABI_H */
