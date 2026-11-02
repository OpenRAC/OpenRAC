#include "include_asm.h"

extern volatile int D_001AD520 __attribute__((sda));
extern int D_001B22B8 __attribute__((sda));
extern int D_001B22B4_1 __attribute__((sda));
extern int D_001B22B4;
extern int D_001B22B0;
extern int D_001B22A0[];
extern int D_001B22A8;
extern int func_00126F38(unsigned int);
extern void func_0011AEA0(int);
extern void func_00127220(void *, int);
extern void func_002FC3C0(unsigned long);
extern void func_002FC220(void);
extern void func_002F0FA0(void);

/* Terminates and submits the DMA chain, advancing the buffer when full. */
void func_002FC280(void)
{
    int flags;
    int used;
    int ready = 0;
    char *packet;

    flags = D_001AD520 | 0x1F;
    used = D_001B22A8 - D_001B22A0[D_001B22B4];
    D_001AD520 = flags;
    if (used > D_001B22B0) {
        if (used < D_001B22B8)
            D_001B22B8 = used;
        ready = 1;
    }
    if (!ready || D_001B22B4_1 == 0) {
        *(int *)(D_001B22A8 + 0) = 0x70000000;
        *(int *)(D_001B22A8 + 4) = 0;
        *(int *)(D_001B22A8 + 8) = 0;
        *(int *)(D_001B22A8 + 12) = 0;
        packet = (char *)func_00126F38(1);
        *(int *)packet |= 0xC0;
        func_0011AEA0(0);
        func_00127220(packet, D_001B22A0[D_001B22B4]);
        if (ready) {
            func_002FC3C0(1);
            func_002FC220();
        }
    } else {
        D_001AD520 = 0;
        func_002F0FA0();
    }
}
