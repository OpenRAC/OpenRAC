/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The contract between a game's translated C and the port's runtime.
 *
 * The port keeps the game's memory where the game expects it: one 4 GB
 * reservation whose offsets are the PlayStation 2's own addresses. A pointer
 * in the game is a 32-bit offset into it (gaddr), the way OpenGOAL keeps
 * Jak's pointers ("OpenGOAL offsets", docs/port/DESIGN.md). Structures keep
 * their console layout, globals stay at their retail addresses, and an
 * address the game computes (an absolute address, a global plus an offset,
 * an uncached alias) means what it meant on the console.
 *
 * port/tools/hostgen writes C against this header: every pointer becomes a
 * gaddr, every access through one goes through G(), and a call through a
 * function pointer goes through GFN(). Hand-written host code uses the same
 * macros. This header is C (C11 with the GNU extensions both GCC and Clang
 * have), so the generated code is compiled as C, and C++ includes it too.
 */
#ifndef OPENRAC_GUEST_H
#define OPENRAC_GUEST_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* An address in game memory: what a pointer is in the game. */
typedef uint32_t gaddr;

/* The game's 128-bit integer (gcc's mode(TI)): quadword copies and MMI. */
__extension__ typedef __int128 openrac_s128;
__extension__ typedef unsigned __int128 openrac_u128;

/* ---- The memory map, in game addresses ----
 *
 * Main RAM and its two uncached aliases share one set of pages, as on the
 * console. The scratchpad is the EE's 16 KB of fast memory. The register
 * pages are plain memory: the port never models the hardware behind them;
 * where the game hands work to the hardware (a DMA send, a vsync wait), the
 * runtime's replacement of that library call takes over. The port's own
 * region holds what translated code needs that the console kept elsewhere
 * (locals whose address is taken, string literals). Everything else in the
 * reservation is inaccessible, so a stray access stops with its address. */
#define GUEST_RAM 0x00000000u
#define GUEST_RAM_SIZE 0x02000000u /* 32 MB */
#define GUEST_RAM_UNCACHED 0x20000000u
#define GUEST_RAM_UNCACHED_ACCEL 0x30000000u
#define GUEST_EE_REGS 0x10000000u
#define GUEST_EE_REGS_SIZE 0x00010000u
#define GUEST_VU_MEM 0x11000000u
#define GUEST_VU_MEM_SIZE 0x00010000u
#define GUEST_GS_REGS 0x12000000u
#define GUEST_GS_REGS_SIZE 0x00002000u
#define GUEST_SCRATCHPAD 0x70000000u
#define GUEST_SCRATCHPAD_SIZE 0x00004000u /* 16 KB */
#define GUEST_PORT 0xE0000000u
#define GUEST_PORT_SIZE 0x01000000u /* 16 MB */
#define GUEST_STACK_TOP (GUEST_PORT + 0x00100000u)
#define GUEST_STACK_BOTTOM GUEST_PORT
#define GUEST_STRINGS (GUEST_PORT + 0x00100000u)
#define GUEST_STRINGS_SIZE 0x00100000u
#define GUEST_STATICS (GUEST_PORT + 0x00200000u)
#define GUEST_STATICS_SIZE 0x00100000u

/* ---- Addresses ---- */

/* Where game memory starts in the host's address space. Set once by the
 * runtime before any game code runs. */
extern uint8_t* openrac_guest_base;

/* The host pointer for a game address, and an lvalue of type T there. */
#define G(a) ((void*)(openrac_guest_base + (gaddr)(a)))
#define GREF(T, a) (*(T*)G(a))

/* The game address of a host pointer into game memory. */
#define GADDR(p) ((gaddr)((const uint8_t*)(p) - openrac_guest_base))

/* The host compiler does not know that RAM and its uncached aliases are the
 * same memory: a write through one alias and a read through another in the
 * same function may be reordered or folded. Code that does that needs
 * GBARRIER() between the two (the game itself uses the aliases for what it
 * hands to the hardware, which the port reads after the call). */
#define GBARRIER() __asm__ __volatile__("" : : : "memory")

/* ---- Locals whose address is taken ----
 *
 * On the console such a local lives on the stack in game memory, and the
 * game passes its address around like any other pointer. Translated code
 * gives the function a frame on the game stack (GUEST_STACK_*): GFRAME(n)
 * reserves n bytes, aligned to 16, and gives them back when the function
 * returns, by whatever path. */
extern gaddr openrac_guest_sp;
gaddr openrac_guest_frame(uint32_t size);

static inline void openrac_guest_frame_end(gaddr* saved) {
    openrac_guest_sp = *saved;
}

#define GFRAME(name, size)                                                                         \
    gaddr name##_saved_sp __attribute__((cleanup(openrac_guest_frame_end))) = openrac_guest_sp;    \
    const gaddr name = openrac_guest_frame(size)

/* A static local whose address the game takes: storage in game memory for
 * the life of the program, zeroed (the port's statics region). */
gaddr openrac_guest_static(uint32_t size);

/* ---- String literals ----
 *
 * A literal in the game's C is data in game memory on the console. GSTR
 * copies it into the port's string region once, at its first use, and gives
 * its address. */
gaddr openrac_guest_string(const char* s, size_t size);
#define GSTR(lit)                                                                                  \
    __extension__({                                                                                \
        static gaddr gstr_cached_;                                                                 \
        if (gstr_cached_ == 0) {                                                                   \
            gstr_cached_ = openrac_guest_string((lit), sizeof(lit));                               \
        }                                                                                          \
        gstr_cached_;                                                                              \
    })

/* ---- Functions ----
 *
 * Function pointers in the game are code addresses: in its data (tables of
 * update functions, vtables) and in what it computes. GFN looks the address
 * up in the functions the build registered (port/runtime/guest.cpp) and
 * gives a host function pointer of type T. A level's program is loaded at
 * the same addresses as every other level's, so a code address means the
 * function of the level that is loaded (openrac_guest_set_overlay). */
typedef void (*openrac_host_fn)(void);

typedef struct openrac_fn_entry {
    gaddr address;
    openrac_host_fn fn;
    const char* name;
} openrac_fn_entry;

/* Overlay -1 is the executable; 0..n are the level programs. */
#define OPENRAC_OVERLAY_EXE (-1)
void openrac_guest_register(int overlay, const openrac_fn_entry* entries, size_t count);
void openrac_guest_set_overlay(int overlay);
int openrac_guest_overlay(void);
openrac_host_fn openrac_guest_function(gaddr address);
const char* openrac_guest_function_name(gaddr address);
#define GFN(T, a) ((T)openrac_guest_function(a))

/* Which overlay is loaded, asked at each call through a code address when
 * set: the game knows (rac1 keeps its current level in a global). */
void openrac_guest_set_overlay_source(int (*current)(void));

/* ---- Per-level relocation ----
 *
 * A level's program carries its own copy of much of the executable's code,
 * whose globals sit at other addresses (rac1: each level's copy of the engine).
 * The port has one host function for both, written with the executable's
 * addresses, so an executable function's global address goes through
 * OPENRAC_DATA and a function address it takes through OPENRAC_CODE: with a
 * level loaded, an address in the relocated range becomes that level's. The
 * tables are built when a level's code is first used, by pairing the address
 * operands of each function's executable code with its copy in the level's
 * code (the two differ only in addresses), so nothing of the game is stored. */
void openrac_guest_set_relocation(gaddr low, gaddr high, gaddr gp, const uint8_t* exe_code,
                                  gaddr exe_base, uint32_t exe_size);
extern gaddr openrac_relocate_low, openrac_relocate_high;
gaddr openrac_relocate_data(gaddr address);
gaddr openrac_relocate_code(gaddr address);
static inline gaddr OPENRAC_DATA(gaddr a) {
    return a - openrac_relocate_low < openrac_relocate_high - openrac_relocate_low
               ? openrac_relocate_data(a)
               : a;
}
static inline gaddr OPENRAC_CODE(gaddr a) {
    return openrac_relocate_high != 0 ? openrac_relocate_code(a) : a;
}

/* A function the game calls that has no C in the port yet: still assembly in
 * the decompilation, or a library the port has not replaced. Logged once per
 * function and counted (openrac_guest_report_missing at exit). While the
 * port is being brought up the program stops at the first one instead
 * (openrac_guest_stop_on_missing), so the log ends with what to do next. */
void openrac_guest_missing(const char* name);
void openrac_guest_report_missing(void);
void openrac_guest_stop_on_missing(int stop);

/* ---- What the game's own inline assembly did ----
 *
 * The decompilation keeps three quadword helpers as inline assembly
 * (games/rac1/pal/include/common.h): qcopy, qcopy_nc and qzero. */
static inline void openrac_qcopy(gaddr dst, gaddr src) {
    memmove(G(dst), G(src), 16);
}

static inline void openrac_qzero(gaddr p) {
    memset(G(p), 0, 16);
}

#ifdef __cplusplus
}
#endif

#endif /* OPENRAC_GUEST_H */
