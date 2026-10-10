/* NON_MATCHING func_L00_0020D3A0 -- src/overlays/shared/help_0020CDF0.c
 * Best so far: SIZE ours 584 / retail 588, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Claimed by a16 after reaching N=8; not attempted. Free for another worker.
 *   Searches a 32-byte-entry table (D_L00_0015F7EC, count D_L00_0015F7F0) for an entry near a position and reports
 *   p4.c is closest (584 vs 588): retail spills three out pointers (a1, a4, a5) to 0x18/0x1C/0x20($sp) and loads t
 *   before the qcopy); ours spills two and keeps one in an s-reg and tests the count later. Needs a source shape t
 */
extern float func_001F9D10(void *, void *);
extern int func_L00_0025EFC0(void *, void *, void *, void *, void *, int, float, float, float);
extern float func_001F9D48(void *, void *);
extern float func_001F9B88(float);
typedef struct {
    int pad0[3];
    float r;
    int *p;
    int q;
    int pad1[2];
} Ent;
extern int D_L00_0015F7F0 MACRO_ADDR;
extern Ent *D_L00_0015F7EC MACRO_ADDR;
extern char D_0013E633[];

// Searches the table for an entry near the position; on a hit reports it through the out pointers.
int func_L00_0020D3A0(void *pos, void *oA, void *p2_, void *oB, void *oC, void *oD, int f1, int f2) {
    int *outA = oA;
    float *p2 = p2_;
    int *outB = oB;
    float *outC = oC;
    int *outD = oD;
    float vec[4];
    int x;
    float y;
    int i = 0;
    qcopy(vec, pos);
    for (; i < D_L00_0015F7F0; i++) {
        if (f1 != 0 && (int)D_L00_0015F7EC[i].p == f1) continue;
        if (f2 != 0 && (int)D_L00_0015F7EC[i].p != f2) continue;
        if (*D_L00_0015F7EC[i].p == 0) continue;
        if (func_001F9D10(&D_L00_0015F7EC[i], vec) > D_L00_0015F7EC[i].r) continue;
        if (func_L00_0025EFC0(D_L00_0015F7EC[i].p, vec, p2, &x, &y, D_L00_0015F7EC[i].q, 12.0f, 10.0f, 0.0f) != 0) {
            float lim = 0.9f;
            float hlim = 1.5f;
            float dd, hd;
            char *g = D_0013E633 + 0xE1D;
            if (*(unsigned int *)(g + 0x208C) < 2) lim = 0.3f;
            if (*(short *)(g + 0x1CA) != 0) {
                hlim = 10.0f;
                lim = lim + 0.5f;
            }
            dd = func_001F9D48(vec, p2);
            hd = func_001F9B88(vec[2] - p2[2]);
            if (dd < lim && hd < hlim) {
                *outA = (int)D_L00_0015F7EC[i].p;
                *outB = x;
                *outC = y;
                *outD = D_L00_0015F7EC[i].q;
                return 1;
            }
        }
    }
    return 0;
}
