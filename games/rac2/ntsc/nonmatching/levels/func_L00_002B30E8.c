/* GPT-6.1 Sol (high), v0775; default game -O2 -G0.
 * Best check: 328/348 bytes, 0/27 places.
 * Slot indices are unsigned bytes, including both final global increments.
 * Preserve independent row/column scaling of the per-player counter and the
 * initial two counter values before their stores. Global base-address
 * lifetimes, repeated slot reads, and scalar scheduling still differ.
 * A multidimensional spelling reaches 320 bytes with the retail 16-byte frame,
 * but not its address allocation; -mno-split-addresses and a declared volatile
 * slot do not resolve this. This retained draft has no volatile fields.
 * Required declarations are included below.
 */
#include "common.h"
extern int D_001A79F0;
extern int D_001A73D8;
extern int D_001A73E0[];
extern int D_0013A148[];
extern unsigned char D_0019B2E8[];
extern int D_001A24A8[];
extern int D_L00_001B19C8;
extern unsigned char D_00189E20_1[];
extern void func_L00_002F45B8(void);
extern void func_0027D330(int);

void func_L00_002B30E8(void)
{
    char *state;
    int player;
    int slot;
    int id;
    int threshold;
    int index;
    int address;

    { int *counter=&D_0013A148[D_001A79F0];
      int total=D_001A73D8;
      int count=*counter;
      D_001A73D8=total+1;
      *counter=count+1;
    }
    func_L00_002F45B8();
    state=(char *)D_00189E20_1;
    slot = (unsigned char)D_00189E20_1[0x22C6];
    if (slot == 0xFF)
        return;
    id = *(int *)(state + 0x2294);
    if (id == 0x74 || id == 0x79)
        threshold = 0x708;
    else
        threshold = 0x384;
    if (D_L00_001B19C8 >= threshold) {
        player=D_001A79F0;
        slot=(unsigned char)D_00189E20_1[0x22c6];
        address=player*16+slot;
        if ((D_0019B2E8[address]&0x80)==0) {
            index=slot&0xf;
            (*(int (*)[16])(D_001A24A8+player*16))[index]++;
        }
    }
    slot=(unsigned char)D_00189E20_1[0x22c6];
    D_001A73E0[slot & 0xF]++;
    slot=(unsigned char)D_00189E20_1[0x22c6];
    D_001A73E0[slot]++;
    func_0027D330(0x10);
    state[0x22C1] = 1;
}
