/* sol-low, task v0270, USA v1.01 boot (one placement).
 * Rechecked with the upstream pinned compiler on 28 October 2026: the
 * prior sol-high 228-byte result below reproduces unchanged.
 * Explicit shared-return goto walk: 212 bytes, post-walk check eliminated.
 * Result-pointer/shared-return form: 232 bytes, extra saved s2 and 0x40 frame.
 * Neither improves the retained original source.
 * Best check: 228/228 bytes, 0/1 places, sdk -O2 -G2 -ffunction-sections
 * -fno-thread-jumps. The original public libgcc frame.c find_fde body below
 * now retains the post-walk null check and the complete retail binary search.
 * Only +0x20/+0x24 differ: beqz s0 targets the post-walk test at +0x5c
 * with a nop slot; retail targets the epilogue at +0xd0 with v0 = 0.
 * All other instructions/operands agree after ordinary symbol relocations.
 * This supersedes the earlier 216/228 measurement under the default SDK
 * profile; no new compiler or assembler behavior was introduced.
 *
 * Targeted controls: -fno-cse-follow-jumps and -fno-gcse still produce
 * 216 bytes and alter search layout/allocation. An explicit initial guard
 * with -fno-thread-jumps produces 236 bytes. An infinite walk with an
 * early null return produces 208 bytes and deletes the post-walk check.
 * Next: a source/profile retaining the post-walk check while threading only
 * the initial null exit; another ordinary loop spelling is insufficient.
 * Declarations required: common.h for u32/s32; object list and frame_init
 * are declared below. Keep this in an isolated unit if testing other flags.
 */
#include "common.h"

typedef struct
{
  u32 length;
  s32 CIE_delta;
  void *pc_begin;
  u32 pc_range;
} fde;

struct object
{
  void *pc_begin;
  void *pc_end;
  fde *fde_begin;
  fde **fde_array;
  u32 count;
  struct object *next;
};

extern struct object *D_00141838;

extern void func_00123A00(struct object *);

fde *func_00123B40(void *pc) {
    struct object *ob;
    u32 lo, hi;

    for (ob = D_00141838; ob; ob = ob->next) {
        if (ob->pc_begin == 0)
            func_00123A00(ob);
        if (pc >= ob->pc_begin && pc < ob->pc_end)
            break;
    }

    if (ob == 0)
        return 0;

    hi = ob->count;
    for (lo = 0; lo < hi;) {
        u32 i = (lo + hi) / 2;
        fde *f = ob->fde_array[i];

        if (pc < f->pc_begin)
            hi = i;
        else if (pc >= f->pc_begin + f->pc_range)
            lo = i + 1;
        else
            return f;
    }

    return 0;
}
