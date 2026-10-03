/* NON_MATCHING func_L00_002901A8 -- src/overlays/shared/space_0028FB78.c
 * Best so far: SIZE ours 232 / retail 240, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Camera-follow update: clears D_0013E130+0x2A, sets +0x2C=1, then if D_0015EE84 is in range and its per-mode fl
 *   Best is p3.c/p2.c: branch layout and the second half match; the only difference is the address of D_0013E130. 
 *   Would unblock: a wording that stops CSE from sharing the low part between the first part and the body, while s
 */
extern void func_L00_0028FCA0(int);
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern unsigned char D_0013DE4B NOT_SDA;
extern unsigned char D_0013D4F1[] NOT_SDA;
extern char D_0013E130_a[] __asm__("D_0013E130") NOT_SDA;
extern int D_L00_0015F6A8 MACRO_ADDR;
extern float D_L00_0015F4FC_f __asm__("D_L00_0015F4FC") MACRO_ADDR;
extern void func_002348B8(void);

/* Per-frame check of the camera-follow state: reset the flag when it does not apply, else restart it. */
void func_L00_002901A8(void) {
    char *g = D_0013E130_a;
    int v;
    *(short *)(g + 0x2C) = 1;
    if (*(short *)(g + 0x2A) != 0) {
        *(short *)(g + 0x2A) = 0;
    }
    v = D_0015EE84_m;
    if ((v == 1 && D_0013DE4B == 0) || v == 0 || (v == 0xE && D_0013D4F1[7] == 0) || v >= 0x14) {
        D_L00_0015F6A8 = 0;
    } else {
        int a;
        *(short *)(D_0013E130_a + 0x24) = -1;
        D_L00_0015F6A8 = 6;
        D_L00_0015F4FC_f = 1.0f;
        *(int *)(D_0013E130_a + 0x20) = 0;
        func_002348B8();
        a = D_L00_0016128C - 0x60000;
        D_L00_0016C960[0x16] = D_L00_00173F00[1] + a;
        D_L00_0016C960[0x17] = D_L00_00173F00[2] + a;
        D_L00_0016128C = a;
        func_L00_0028FCA0(8);
    }
}
