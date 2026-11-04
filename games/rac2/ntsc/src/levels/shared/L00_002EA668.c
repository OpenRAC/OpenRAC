/* Claims a slot (the given one, or the last free one of 32), counts it and fills it in.
   Own unit: -frac2-gcse-unsplit-edges lets PRE hoist the table's high half above the
   first branch, as retail has it. */
#include "include_asm.h"
#include "common.h"

typedef struct {
    char pad_00[0x10];
    int f10;
    int f14;
    int f18;
    int f1C;
    unsigned short f20;
    unsigned short f22;
    unsigned short f24;
    unsigned short f26;
    char pad_28[8];
} Slot_r002EA668;

extern Slot_r002EA668 D_L00_001BCC90[];
extern int D_L00_001B1998 MACRO_ADDR;

int func_L00_002EA668(int index, unsigned short a1, unsigned short a2, unsigned short a3,
                      unsigned short a4, int a5, int a6) {
    Slot_r002EA668 *p;
    int i;
    if (index < 0) {
        for (i = 0; i < 0x20; i++) {
            if ((D_L00_001BCC90[i].f1C & 5) == 0)
                index = i;
        }
        if (index < 0)
            return -1;
    }
    p = &D_L00_001BCC90[index];
    D_L00_001B1998++;
    p->f20 = a1;
    p->f22 = a2;
    p->f24 = a3;
    p->f26 = a4;
    p->f14 = a5;
    p->f1C = 5;
    p->f10 = a6;
    return index;
}
