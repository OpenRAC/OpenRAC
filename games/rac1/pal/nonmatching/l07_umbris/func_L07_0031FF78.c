/* NON_MATCHING func_L07_0031FF78 -- src/overlays/l07_umbris/vendor_0031BDB8.c
 * Best so far: BYTES 7/564 (98.8% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_1552: spark spawner (state 0 init; state 1 waits for level flag 2, picks mob index, emits 10 sparks
 */
extern void func_L00_00264870(int);
extern int func_001F9850(int);
extern void func_L00_00250800(void *, int, void *);
extern float func_002140F8(float, float);
extern int func_L00_00258BC8(int, int);
extern int func_002140B0(int);
extern char *func_L00_0026DEA0_c(void *, int, void *, int, float, float, float, float) __asm__("func_L00_0026DEA0");
extern int D_L07_0015F6A8 MACRO_ADDR;
extern char D_L07_0016C960_c[] __asm__("D_L07_0016C960");
extern float D_L07_0015F660[] MACRO_ADDR;

// Runs the effect spawner: waits for the level flag, then emits sparks at four joints of a target moby.
void func_L07_0031FF78(char *m) {
    int state = *(unsigned char *)(m + 0x20);
    int a;
    int k;
    int t;
    int off;
    int i;
    int j;
    char *mob;
    char *S;
    float buf[4];
    switch (state) {
    case 0:
        *(unsigned char *)(m + 0x30) = 0xFF;
        m[0x20] = 1;
        break;
    case 1:
        a = D_L07_0015F6A8;
        if (a != 2) return;
        S = D_L07_0016C960_c;
        t = *(int *)(S + 0x30) - 3;
        if ((unsigned)t < 2) {
            int idx = a;
            if ((unsigned)t > (unsigned)state) idx = 0;
            off = idx * 4;
            func_L00_00264870(*(int *)(off + (S + 0x178)));
        }
        if (*(int *)(S + 0x30) != a) return;
        k = 7;
        if (*(int *)(S + 0x34) != func_001F9850(0xA8A)) k = 0;
        if (*(int *)(S + 0x34) == func_001F9850(0xACE)) k = 6;
        if (k != 0) {
            mob = ((char **)S)[0x5E + k];
            for (i = 0; i < 4; i++) {
                func_L00_00250800(mob, i, buf);
                for (j = 9; j >= 0; j--) {
                    float d = func_002140F8(20000.0f, 200000.0f);
                    int col = (func_L00_00258BC8(0x20, 0x80) << 24) | 0x7F7F7F;
                    int v = func_L00_00258BC8(0, 4);
                    char *p;
                    char *q;
                    if (func_002140B0(2) != 0) v = -v;
                    p = func_L00_0026DEA0_c(buf, v, D_L07_0015F660, col, 0.6f, 1.0f, 1.01f, d);
                    if (p != 0) {
                        q = p + 0x20;
                        *(short *)(p + 0xA) = func_001F9850(func_L00_00258BC8(0x3C, 0x5A));
                        *(int *)(q + 4) = 2;
                        *(char *)(q + 0xA) = col >> 24;
                        *(char *)(q + 0xB) = *(unsigned char *)(p + 0xA);
                    }
                }
            }
        }
        break;
    }
}
