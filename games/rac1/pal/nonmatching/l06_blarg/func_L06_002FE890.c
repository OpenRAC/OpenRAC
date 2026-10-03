/* NON_MATCHING func_L06_002FE890 -- src/overlays/l06_blarg/vendor_002FE5D0.c
 * Best so far: SIZE ours 748 / retail 740, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Draws 58 billboard quads (two 0x90-byte Quad structs on stack: pos[4][4], col[4], st[4][2], long gif[4]), clip
 *   Best is p4.c (748 vs 740 bytes): remaining diffs are loop counter i (retail keeps i in $s1 with spill of i+1 a
 *   and inner-loop sched/pointer increment order (retail increments $s1/$fp together after the st stores, count de
 */
typedef int u128 __attribute__((mode(TI)));
extern int func_001F4868(int);
extern void func_001FA190(void *);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9EE8(void *, void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9EC0(void *, void *, void *);
extern float func_001F9C78(void *, void *);
extern void func_L00_001FD1D8(void *, void *, int);
extern char D_L06_00167500[];
extern char D_L06_00167640[];
extern char D_L06_001F08B0[];
extern char D_L06_001EFF30[];
extern char D_L06_001EFA00[];
extern char D_L06_001EF660[];
extern char D_L06_001EF080[];
extern short D_L06_00162030;
extern short D_L06_00162034;
extern short D_L06_00162020;
extern short D_L06_00162010;
extern short D_L06_0016200C;
extern short D_L06_00162014;
extern short D_L06_00162018;
extern short D_L06_0016201C;
extern short D_L06_00162024;
extern short D_L06_00162028;
extern short D_L06_0016202C;

typedef struct {
    float pos[4][4];
    int col[4];
    float st[4][2];
    long gif[4];
} Quad;

// draws the 58 billboard quads of a sprite table, clipped against the camera
void func_L06_002FE890(void)
{
    Quad a;
    Quad b;
    float m[20];
    float t1[4];
    float t2[4];
    float *mat;
    float *v1;
    float *v2;
    Quad *pb;
    int i = 0;
    float f21 = *(float *)&D_L06_00162034 * *(float *)(D_L06_00167500 + 0x154);
    float f20 = *(float *)&D_L06_00162030 * *(float *)(D_L06_00167500 + 0x158);
    long r2, r3;

    a.gif[1] = func_001F4868(*(int *)&D_L06_00162020);
    r2 = func_001F4868(0x2A);
    r3 = (long)*(int *)&D_L06_0016200C | ((long)*(int *)&D_L06_00162010 << 2)
        | ((long)*(int *)&D_L06_00162014 << 4) | ((long)*(int *)&D_L06_00162018 << 6)
        | ((long)*(int *)&D_L06_0016201C << 32);
    mat = m;
    b.gif[1] = r2;
    a.gif[3] = r3;
    b.gif[3] = r3;
    a.gif[2] = 0xFF9000000260L;
    b.gif[2] = 0xFF9000000260L;
    b.gif[0] = 0;
    a.gif[0] = 0;
    func_001FA190(mat);
    v1 = t1;
    v2 = t2;
    a.col[0] = *(int *)&D_L06_00162024;
    b.col[0] = *(int *)&D_L06_00162028;
    a.col[3] = *(int *)&D_L06_00162024;
    a.col[2] = *(int *)&D_L06_00162024;
    a.col[1] = *(int *)&D_L06_00162024;
    b.col[3] = *(int *)&D_L06_00162028;
    b.col[2] = *(int *)&D_L06_00162028;
    b.col[1] = *(int *)&D_L06_00162028;
    do {
        int o = i * 16;
        int j;
        short *idx;
        char *tu;
        char *tv;
        i++;
        func_001F9C30(v1, D_L06_001F08B0 + o, *(float *)&D_L06_0016202C);
        func_001F9EE8(v1, v1, m);
        func_001F9BF0(v2, v1, D_L06_00167640);
        func_L00_001FF4B0(v2, v2, 1.0f);
        func_001F9EC0(v1, D_L06_001EFF30 + o, mat);
        if (!(func_001F9C78(v2, v1) > 0.0f)) {
            pb = &b;
            tu = D_L06_001EFA00;
            tv = D_L06_001EFA00 + 4;
            idx = (short *)(D_L06_001EF660 + o);
            for (j = 0; j < 4; j++) {
                short ia = idx[0];
                short ib = idx[1];
                float u, v;
                func_001F9C30(a.pos[j], D_L06_001EF080 + ia * 16, *(float *)&D_L06_0016202C);
                *(u128 *)pb->pos[j] = *(u128 *)a.pos[j];
                u = *(float *)(tu + ib * 8);
                v = *(float *)(tv + ib * 8);
                a.st[j][0] = u + f20;
                a.st[j][1] = v + f21;
                b.st[j][0] = u;
                b.st[j][1] = v;
                idx += 2;
            }
            func_L00_001FD1D8(&a, m, 0);
            func_L00_001FD1D8(pb, m, 0);
        }
    } while (i < 0x3A);
}
