/* func_0034FFC0: Opus 5.5 xhigh, task v1751. RAC1's vblankHandler plus an extra DMA upload.
 * Result: 464/464 bytes, 28 of 28 places in USA v1.01 with the local CpuEnableInt below, as a
 * unit of its own including common.h (not in the PAL or v2.00 catalogues). Not landed only because
 * a game unit may not define its own asm helper: retail ends in sync; ei at 00350088/0035008C and
 * 0035016C/00350170, and include/common.h has CpuSync but no CpuEnableInt.
 * To land: add CpuEnableInt to include/common.h (RAC1 include/common.h has this exact line), delete
 * the local copy, and put this file in src/game/0034CEA0-vblank.c in place of the marker.
 * Only that own-unit form was measured: 0034CEA0.c does not include common.h and has its own s32
 * typedef. Profile: -O2 -G0 -ffunction-sections.
 */
#include "common.h"

static __inline__ void CpuEnableInt(void) { __asm__ __volatile__("ei"); }
extern char *D_001B23CC[] __attribute__((sda));
extern int D_001A7BB8 __attribute__((sda));
extern int D_001B23C8[] __attribute__((sda));
extern void func_002B6480(int, int);
extern int func_00126F38(int);
extern int func_001244B8(int, int);
extern void func_00127220(int, int);
extern int *func_00351F38(char *);
extern volatile int D_001AE7D0[] __attribute__((sda));
extern volatile int D_001AE7D4[] __attribute__((sda));
extern volatile int D_001AE7D8[] __attribute__((sda));
extern volatile int D_001AE7DC[] __attribute__((sda));
extern volatile int D_001AE7E0[] __attribute__((sda));

int func_0034FFC0(void) {
    int h = func_00126F38(2);
    int *buf;

    D_001AE7DC[0] = D_001A7BB8 ? D_001AE7DC[0] ^ 1 : (int)((*(volatile unsigned long long *)0x12001000 >> 13) & 1);
    if (D_001AE7D0[0] != 0) {
        D_001AE7D4[0] = D_001AE7D4[0] + 1;
        D_001AE7E0[0] = func_001244B8(1, 0);
        if (D_001AE7E0[0] == 0) {
            buf = func_00351F38(D_001B23CC[0] + 0xD9168);
            if (buf == 0) {
                CpuSync();
                CpuEnableInt();
                return 0;
            }
            if (D_001AE7DC[0] == 0 && buf[0] == 2) {
                int off;
                func_001244B8(0, 0);
                off = (buf[1] << 14) + 0x1C81C0;
                func_002B6480(D_001B23C8[0] + off, 0);
                func_00127220(h, (int)buf + 0x40);
                buf[0] = 1;
            } else if (D_001AE7DC[0] == 1) {
                int st = buf[0];
                if (st == 1) {
                    int off;
                    func_001244B8(0, 0);
                    off = (buf[1] << 14) + 0x1C81C0;
                    func_002B6480(D_001B23C8[0] + off, 0);
                    func_00127220(h, (int)buf + 0x13880);
                    buf[0] = 0;
                    D_001AE7D8[0] = st;
                }
            }
        }
    }
    CpuSync();
    CpuEnableInt();
    return 0;
}
