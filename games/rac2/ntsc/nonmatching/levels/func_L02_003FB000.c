/* func_L02_003FB000 (288 bytes, 1 place, 02:003FB000) - near miss, measured 21 October 2026.

   In a -G8 unit of its own (src/levels/02_maktar_nebula/L02_003FB000-g8.c, no cflags line) this
   C is 284 bytes against retail's 288, 0 of 1 places: every instruction is retail's (both $gp
   loads included), and the only missing word is the `nop` at +0x114 that aligns the shared
   return (`jr $ra` at +0x118, the target of the two early returns) to 8 bytes.
   In src/levels/02_maktar_nebula/L02_003F5430.c (-G0) the same C is 288 bytes but 3 words off:
   no nop, and D_L02_001B1214 comes out `lui` + `lhu %lo` (patch 0020: outside a delay slot,
   gas only uses $gp for a symbol whose .extern size it has seen, and -G0 emits none).

   Why the nop is missing: final.c (shorten_branches) gives LOOP_ALIGN to the first CODE_LABEL
   after a NOTE_INSN_LOOP_BEG, and nothing else gives an 8-byte label alignment here. Retail's
   return label must therefore follow loop notes with no label in between, i.e. a loop after
   the last store whose body compiled to nothing.
   - Diagnosis in scratch only: the same C with an empty `do { } while (0);` after the last
     store is retail's 72 words exactly. That spelling is rejected (empty loop).
   - A wrapper around the LAST store only gives retail's bytes: `do { <the 0x11E store> }
     while (0);` or `for (;;) { <the 0x11E store>; return; }` - each measured in the -G8 unit,
     `tools/check.py`: 1 of 1 places match. Not landed: such a wrapper is there only to steer
     the alignment (the merged precedents are func_00124818's do/while(0) barrier and
     func_00122760's `while` whose body returns); the lead decides whether it is admissible.
   - A wrapper around the three tail statements (`do { ... } while (0)`, or
     `for (;;) { ...; return; }`) also aligns the label, but then gcse's PRE hoist of the %hi
     of D_00189E20 above the inner loop stays (without it cse2 folds the copy back into the
     tail; with it the pseudo is global and gets $t8): 280 bytes.
   - `while (entry != 0) { ...; return; }` around the whole walk: the outer LOOP_BEG's first
     label is the inner loop head, so the return label stays 4-aligned (and the %hi is
     hoisted the same way): 280 bytes.
   What would unblock it: a plain-C construct that leaves loop notes after the tail without
   wrapping it (the original was probably an empty debug/assert macro). */

#include "include_asm.h"

extern char *D_L02_001B359C __attribute__((sda));
extern unsigned int D_L02_001FC060[];
extern unsigned short D_L02_001B1214 __attribute__((sda));
extern char D_00189E20_c[];

void func_L02_003FB000(char *a0) {
    char *state = *(char **)(a0 + 0x68);
    unsigned int index = *(unsigned int *)(state + 0x40);
    unsigned int entry;
    short *p;
    char *tbl;
    char *rec;
    char *g;

    *(int *)(state + 0x60) = 0;
    *(float *)(state + 0x64) = 0;
    if (index == 0xFF)
        return;
    entry = D_L02_001FC060[index];
    if (entry == 0)
        return;
    p = (short *)entry;
    tbl = D_L02_001B359C;
    do {
        rec = tbl + ((*(unsigned short *)p & 0x7FFF) << 8);
        if (rec == 0)
            continue;
        if (*(unsigned char *)(rec + 0x20) == 0xFE)
            continue;
        if (*(unsigned char *)(rec + 0x20) == 0xFD)
            continue;
        if (*(unsigned char *)(rec + 0x20) == 1)
            continue;
        {
            short type = *(short *)(rec + 0xAA);
            if (type == 0x9CA || type == 0xB22 || type == 0xC3B || type == 0xC3E || type == 0x9F8 || type == 0xA44) {
                *(int *)(state + 0x60) += 1;
                *(float *)(state + 0x64) += *(float *)(*(char **)(rec + 0x68) + 0x20);
            }
        }
    } while (*p++ >= 0);
    g = D_00189E20_c;
    *(float *)(state + 0x68) = (float)*(int *)(g + 0x24CC);
    *(short *)(state + 0x11C) = 0;
    *(unsigned short *)(state + 0x11E) = D_L02_001B1214;
}
