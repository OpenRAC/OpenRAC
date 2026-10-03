/* NON_MATCHING func_L04_002E2DF0 -- src/overlays/l04_eudora/vendor_002CB800.c
 * Best so far: SIZE ours 532 / retail 528, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Eudora moby update: spins yaw, 4-state switch (wait, approach hero <3.0, trigger exit/level change, finish).
 *   p1.c is closest (56/528 bytes): only the first D_L04_0015F6A8==2 compare is scheduled after the swc1 (retail l
 *   and in case 2 register choices (li 0xB4 into $v0, lui $at store) differ. Retail reuses the switch's constant 2
 */
extern float func_001FA748(float, float);
extern void func_L00_002D80A0(char *);
extern void func_0020D678(void *);
extern void func_L04_002E30E0(void *);
extern float func_001F9D48(void *, void *);
extern void func_L00_00299B68(int);
extern void func_L00_00264DB8(int, int);
extern void func_L00_002618D8(int, int);
extern void func_L00_00286128(void *a0, void *a1);
extern int func_001E9730();
extern int func_0020BFC8(int, int);
extern float D_0015EE6C MACRO_ADDR;
extern char D_0013E633[];
extern unsigned char D_0013D5CA[] NOT_SDA;
extern unsigned char D_0013D5EB[] __attribute__((section(".data")));
extern int D_L04_0015F6A8;
extern int D_L04_0015F6A8_b __asm__("D_L04_0015F6A8");
extern int D_L04_0015F720;
extern char *D_L04_00160058;
extern char D_L04_001DCA60[];

// Eudora moby update: spins, then waits for the hero and triggers a level exit.
void func_L04_002E2DF0(char *m) {
    char *d = *(char **)(m + 0x78);
    *(float *)(m + 0x48) = func_001FA748(*(float *)(m + 0x48), D_0015EE6C * 1.5707964f);
    if (D_L04_0015F6A8 != 2) {
        if (*(unsigned short *)(m + 0x34) & 1) {
            *(unsigned short *)(m + 0x34) = *(unsigned short *)(m + 0x34) & 0xFFBE;
        }
    } else {
        *(unsigned short *)(m + 0x34) = *(unsigned short *)(m + 0x34) | 0x41;
    }
    switch (*(unsigned char *)(m + 0x20)) {
    case 0:
        func_L00_002D80A0(m);
        if (D_0013D5EB[0xE] != 0) {
            func_0020D678(m);
        } else {
            m[0x20] = 1;
            *(float *)(m + 0x18) = *(float *)(m + 0x18) + 1.0f;
        }
        break;
    case 1:
        func_L04_002E30E0(m);
        if (func_001F9D48(m + 0x10, D_0013E633 + 0xE9D) < 3.0f) {
            *(unsigned short *)(m + 0x34) = *(unsigned short *)(m + 0x34) | 0x41;
            func_L00_00299B68(2);
            m[0x20] = 2;
        }
        break;
    case 2:
        if (D_L04_0015F6A8_b != 2) {
            if (D_0013D5CA[7] == 0) {
                func_L00_00264DB8(0xFA7, -1);
            } else {
                func_L00_00264DB8(0x53E4, -1);
            }
            D_L04_0015F720 = 0xB4;
            func_L00_002618D8(9, 1);
            if (*(int *)(d + 4) != -1) {
                char *p = D_L04_00160058 + *(int *)(d + 4) * 256;
                func_L00_00286128(p + 0x10, p + 0x40);
            } else {
                func_001E9730(D_L04_001DCA60, *(short *)(m + 0xB2));
            }
            m[0x20] = 3;
            func_0020BFC8(0, -1);
        }
        break;
    case 3:
        func_0020D678(m);
        break;
    }
}
