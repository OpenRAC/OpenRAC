#ifndef COMMON_H
#define COMMON_H

#include "include_asm.h"
#include "names.h"   /* readable names for func_/D_ symbols (docs/NAMES.md) */

#include "types.h"
#include "macros.h"

/*
 * Copies one 16-byte quadword from src to dst through $2, the way retail's
 * own source did: an inline-asm copy shaped like libvu0's sceVu0CopyVector
 * (which uses $6). Each address goes into its own register and is read at
 * offset 0, which no C copy reproduces: a long long or aligned-struct copy
 * folds the offset into lq/sq. First matched on func_001ECC10 (camera.c).
 *
 * This is the one sanctioned inline asm. Candidates call it; they never
 * write asm inside a function themselves (tools/integrate.py refuses that).
 */
static __inline__ void qcopy(void *dst, void *src) {
    __asm__ __volatile__("lq $2,0x0(%1)\n\tsq $2,0x0(%0)" : : "r"(dst), "r"(src) : "$2", "memory");
}

/*
 * The same copy without the "memory" clobber. Retail has both behaviours.
 * Where a value read before a copy is read again after it, the hero's
 * state-setting function (func_L02_0022BE40 and its level versions) reloads
 * it, which is qcopy(); the hero's update function keeps it in its register
 * across the copy (case 107 of func_L05_00256148 and func_L16_00227818),
 * which is this one. Dropping the clobber from qcopy() itself leaves eleven
 * state-setting functions 8 bytes short, so the two are separate. Use this
 * one only where retail keeps a value live across the copy.
 */
static __inline__ void qcopy_nc(void *dst, void *src) {
    __asm__ __volatile__("lq $2,0x0(%1)\n\tsq $2,0x0(%0)" : : "r"(dst), "r"(src) : "$2");
}

/*
 * Clears one 16-byte quadword with `sq $0`, the other store retail's own
 * source had as inline asm: this compiler's `sq` takes a register, so C
 * zeroing a 128-bit value always comes out `por $2,$0,$0` + `sq $2`, while
 * retail has `sq $0` in code that is otherwise the compiler's. No clobbers:
 * with a "memory" clobber the compiler reloads the base address around it.
 * Identified by Lombyte's project, the US decompilation (its include/qzero.h).
 */
static __inline__ void qzero(void *p) {
    __asm__ __volatile__("sq $0,0x0(%0)" : : "r"(p));
}

/*
 * Privileged/ordering CPU instructions the retail C emitted through the SDK's
 * own inline helpers. Kept here as the only place for them, like qcopy();
 * functions call these, never write the asm themselves.
 */
static __inline__ void CpuSync(void) {
    __asm__ __volatile__("sync");
}
static __inline__ void CpuSyncP(void) {
    __asm__ __volatile__("sync.p");
}
static __inline__ void CpuEnableInt(void) {
    __asm__ __volatile__("ei");
}
static __inline__ void CpuDisableInt(void) {
    __asm__ __volatile__("di");
}

#endif /* COMMON_H */
