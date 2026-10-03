/* NON_MATCHING func_L00_0029A940 -- src/overlays/shared/tieproc_00299108.c
 * Best so far: SIZE ours 496 / retail 504, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_0029A940 (StartPssMovie): sets loader globals, then uploads two runs of image strips (0xD/0xE 128x128
 *   Best candidate p6.c/p3.c (SIZE 496 vs 504): the loop counters, movn selection and copy of D_L00_0015F50C into 
 *   Ours puts `sw D_L00_00161118` in the `beq n1,0` delay slot (check_macro_slots turns it gp-relative, 1 insn ins
 */
#include "common.h"
extern char D_0013E633[];
extern void func_00118D80(int);
extern void func_0022EFE8(void);
extern void func_00216D88(void);
extern void func_001F4E08(int);
extern void func_L00_00203FB8(void);
extern int func_00122630(void *, short, short, short, short, short, short, short);
extern int func_00122958(void *, void *);
extern int func_00120858(int, unsigned short);
extern int func_001E9730();
extern void func_001F9978(void);
extern int D_L00_00161118 MACRO_ADDR;
extern int D_L00_00161110 MACRO_ADDR;
extern int D_L00_0016110C MACRO_ADDR;
extern int D_L00_00161108 MACRO_ADDR;
extern int D_L00_00161114_m __asm__("D_L00_00161114") MACRO_ADDR;
extern int D_0015EE80 MACRO_ADDR;
extern int D_L00_0015F6A8 MACRO_ADDR;
extern int D_0015EF88;
extern short D_0015EF78_g __asm__("D_0015EF78");
extern short D_L00_0015F6BC_g __asm__("D_L00_0015F6BC");
extern int D_L00_0015F50C;
extern int D_L00_00173F04;
extern char D_L00_001E9800[];

/* Start a movie: set up the loader globals and upload the two image strips. */
void func_L00_0029A940(int arg0, int arg1, int arg2) {
    int buf[24];
    int n1, n2, s16, s17, i, t;
    D_0013E633[0x88] |= 8;
    func_00118D80(0);
    func_0022EFE8();
    func_00216D88();
    D_L00_00161108 = arg0;
    D_L00_00161110 = arg2;
    D_L00_0016110C = arg1;
    *(int *)&D_L00_0015F6BC_g = 1;
    func_001F4E08(4);
    func_L00_00203FB8();
    t = D_L00_0015F50C;
    D_L00_0015F6A8 = 1;
    D_L00_00161114_m = D_L00_00173F04;
    D_L00_00161118 = t;
    n1 = 0xD;
    n2 = 0x11;
    if (D_0015EE80 != 0) {
        n1 = 0xE;
        n2 = 0xE;
    }
    s17 = t;
    s16 = D_0015EF88;
    for (i = 0; i < n1; i++) {
        func_00122630(buf, s16 >> 8, 2, 1, 0, 0, 0x80, 0x80);
        s16 += 0x10000;
        func_00118D80(0);
        func_00122958(buf, (void *)s17);
        s17 += 0xC000;
        func_00120858(0, 0);
    }
    s16 = *(int *)&D_0015EF78_g;
    for (i = 0; i < n2; i++) {
        func_00122630(buf, s16 >> 8, 1, 0, 0, 0, 0x40, 0x40);
        s16 += 0x4000;
        func_00118D80(0);
        func_00122958(buf, (void *)s17);
        s17 += 0x4000;
        func_00120858(0, 0);
    }
    if (s16 > 0x400000) {
        func_001E9730(D_L00_001E9800, s16 - 0x400000);
        func_001F9978();
    }
}
