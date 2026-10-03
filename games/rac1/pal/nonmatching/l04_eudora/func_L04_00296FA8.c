/* NON_MATCHING func_L04_00296FA8 -- src/overlays/l04_eudora/vuchain_00293490.c
 * Best so far: SIZE ours 508 / retail 500, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Initialises a vuchain moby's segment data (6-byte args to func_0020DB98, per-segment loop, then picks a base f
 *   Best is p5.c (SIZE 508 vs 500): ours has a loop-entry guard (retail is a do-while, sp+0x20 address computed af
 */
extern void func_0020DB98(char *arg0, int arg1, void *arg2, char *arg3);
extern void func_0020DAF8(char *arg0, int arg1, char *arg2);
extern float func_L00_001FF860(float, float);
extern void func_0020D960(char *arg0, int arg1, unsigned char *arg2);
extern float func_0020D830(char *);
extern float func_001FA888(int);
extern float func_L04_00242868(int a, unsigned int b);
extern float func_001F9B88(float);
extern int func_L04_00293990_f(char *arg0, char *arg1, float f) __asm__("func_L04_00293990");
extern int D_L04_001CABB8[];

/* sets up a vuchain moby's segments from its data */
void func_L04_00296FA8(char *arg0, char *arg1) {
    int args[6];
    float tmp[16];
    int p;
    char *a;
    char *b;
    float f;
    char *tp = (char *)tmp;
    int lim = (int)arg1 + 0x160;
    int i;
    int *q;
    int *t;
    args[0] = (unsigned char)arg1[0xF0];
    args[1] = (unsigned char)arg1[0xF1];
    args[2] = (unsigned char)arg1[0x1A0];
    args[3] = (unsigned char)arg1[0x1A1];
    args[4] = (unsigned char)arg1[0xB4];
    args[5] = (unsigned char)arg1[0xB5];
    func_0020DB98(arg0, 6, args, arg1);
    a = arg1 + 0x160;
    b = arg1 + 0x120;
    for (p = (int)arg1; p < lim; p += 0xB0) {
        func_0020DAF8(arg0, *(unsigned char *)(p + 0xF0), tp);
        *(int *)(p + 0x104) = 0;
        *(int *)(p + 0xFC) = 0;
        *(float *)(p + 0xF8) = func_L00_001FF860(tmp[4], tmp[5]);
        if (*(unsigned char *)(p + 0x161) == 0)
            func_0020D960(arg0, *(unsigned char *)(p + 0xF3), (unsigned char *)a);
        if (*(unsigned char *)(p + 0x121) == 0)
            func_0020D960(arg0, *(unsigned char *)(p + 0xF2), (unsigned char *)b);
        b += 0xB0;
        a += 0xB0;
    }
    if ((unsigned char)arg0[0x52] != 0xFF) {
        f = func_0020D830(arg0);
    } else {
        f = func_001FA888((unsigned char)arg0[0x51]);
        f += func_L04_00242868((unsigned char)arg0[0x22], (unsigned char)arg0[0x53]);
    }
    t = D_L04_001CABB8;
    arg1[0xB6] = 5;
    *(int *)(arg1 + 0xC0) = 0;
    arg1[0xB7] = 0;
    q = (int *)(arg1 + 0x70);
    for (i = 12; i >= 0; i--) {
        int *e = (int *)*q;
        if (*e == (unsigned char)arg0[0x53]) {
            *(float *)(arg1 + 0xC0) = func_001F9B88(*(float *)(arg0 + 0x58) * ((float *)e)[2]);
            arg1[0xB7] = *(unsigned char *)t;
        }
        t++;
        q++;
    }
    arg1[0xB6] = func_L04_00293990_f(arg0, arg1, f);
    *(short *)(arg1 + 0xEC) = 0;
    *(int *)(arg1 + 0xB8) = 0;
    *(int *)(arg1 + 0xBC) = 0;
    *(int *)(arg1 + 0xC8) = 0;
    *(int *)(arg1 + 0xE8) = 0;
    *(int *)(arg1 + 0xD8) = 0;
    *(int *)(arg1 + 0xE4) = 0;
}
