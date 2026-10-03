/* NON_MATCHING func_L00_002B9730 -- src/overlays/shared/vendor_002B33E8.c
 * Best so far: SIZE ours 860 / retail 864, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002B9730 (shared, 864 bytes): walks a 15-entry list of child mobys at (m->data + 4); for each live en
 *   Not exact: best is p3.c (SIZE ours 852 / retail 864). Structure, frame size (0x160), register use and the stac
 *   Would unblock: combine p3 (array buf used directly, src a block local, pos an expression, q = D_0013E15A + 0x4
 *   Helper: build-sn/try/func_L00_002B9730/sdiff.py gives an instruction-level diff when try_func only prints SIZE
 *   Budget spent. Latest (p6/p7, SIZE 860/864): everything matches except i lives in $a1 (retail $a0) and the else
 */
extern char D_0013E15A[];
extern float D_0015EE6C MACRO_ADDR;
extern short D_L00_001615FC;
extern short D_L00_00161608;
extern short D_L00_0016160C;
extern short D_L00_00161610;
extern short D_L00_00161614;
extern short D_L00_00161618;
extern short D_L00_0016161C;
extern short D_L00_00161620;
extern short D_L00_00161624;
extern short D_L00_00161628;
extern short D_L00_0016162C;
extern void func_L00_001FF500(void *, void *, float);
extern float func_001FA888(int);
extern void func_L00_0025A8C0(char *arg, int a, int b, void *src, float scale);
extern void func_L00_001F2BE8(void *, int, void *, void *, float);
extern void func_L00_00258DB0(float *, float, float);
extern float func_002140F8(float, float);
extern int func_001FA8A8(int, int, float);
extern unsigned func_L00_0025D140(unsigned c, int mask);
extern float func_001F9878(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_001F9BD8(void *, void *, void *);
extern char *func_00219780(void *, void *, void *, int, int, int, int, int, int);

// Updates the effect moby's 15 slots: spawns and releases spark particles around live children.
void func_L00_002B9730(char *m) {
    float vec[4];
    char buf[0x30];
    char qv[16];
    float v2[4];
    float v3[4];
    char *base = *(char **)(m + 0x78) + 4;
    unsigned char *q = (unsigned char *)D_0013E15A + 0x4C6;
    int i;

    for (i = 0; i < 15; i++) {
        int off;
        char *o = *(char **)(base + i * 4);
        if (o == 0) {
            continue;
        }
        off = i * 4;
        if (*(unsigned char *)o == 0xC && o[1] >= 0) {
            float f20 = *(float *)(o + 0xC) / 210000.0f * 0.5f;
            char *src = o + 0x20;
            int type;
            func_L00_001FF500(vec, src, 1.0f);
            vec[2] = 1.0f;
            vec[3] = 5627.9248046875f;
            func_L00_0025A8C0(buf, (int)m, 0x10000, vec, func_001FA888(q[0x10]) + 1.0f);
            *(unsigned char *)(buf + 0x18) = 5;
            *(unsigned char *)(buf + 0x19) = 1;
            *(unsigned short *)(buf + 0x1A) = *(unsigned short *)(m + 0xA6);
            func_L00_001F2BE8((o + 0x10), 0, m, buf, f20);
            type = *(short *)(o + 0xA);
            if (type == 10) {
                int j;
                for (j = 0; j < *(int *)&D_L00_0016162C; j++) {
                    int c1;
                    int c2;
                    int c3;
                    int c4;
                    qcopy(qv, src);
                    func_L00_00258DB0(v2, 0.0f, *(float *)&D_L00_00161608 * D_0015EE6C);
                    *(float *)(qv + 0xC) = *(float *)&D_L00_0016160C;
                    v2[2] = D_0015EE6C * 4.0f;
                    v2[3] = *(float *)&D_L00_00161610;
                    c1 = func_001FA8A8(*(int *)&D_L00_00161614, *(int *)&D_L00_00161618, func_002140F8(0.0f, 1.0f));
                    func_L00_0025D140(c1, q[0x10]);
                    c2 = func_001FA8A8(*(int *)&D_L00_0016161C, *(int *)&D_L00_00161620, func_002140F8(0.0f, 1.0f));
                    func_L00_0025D140(c2, q[0x10]);
                    c3 = func_001FA898_r(func_001F9878(func_002140F8((float)*(int *)&D_L00_00161624, (float)*(int *)&D_L00_00161628)));
                    c4 = func_001FA898_r(func_001F9878(func_002140F8((float)*(int *)&D_L00_00161624, (float)*(int *)&D_L00_00161628)));
                    func_L00_00258DB0(v3, 0.0f, f20 + f20);
                    func_001F9BD8(v3, v3, (o + 0x10));
                    func_00219780(v3, qv, v2, c1, c2, 0xA, c3, c4, *(int *)&D_L00_001615FC);
                }
            } else if (type < 2) {
                *(int *)(base + i * 4) = 0;
            }
        } else {
            *(int *)(base + off) = 0;
        }
    }
}
