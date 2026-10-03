/* NON_MATCHING func_L03_00205E28 -- src/overlays/l03_kerwan/help_00205E28.c
 * Best so far: SIZE ours 508 / retail 512, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Searches a 32-byte-entry table (D_L03_0015F7EC, count D_L03_0015F7F0) for an entry whose radius/ray tests pass
 *   Remaining: retail reloads table base before the 0x14/0x10 field reads in the call setup (and after the func_00
 */
typedef struct {
    int pad[3];
    float r;
    int *p;
    void *q;
    int pad2[2];
} Ent;
extern int D_L03_0015F7F0 MACRO_ADDR;
extern Ent *D_L03_0015F7EC MACRO_ADDR;
extern float func_001F9D10(void *, void *);
extern int func_L00_0025EFC0(void *, void *, void *, void *, void *, void *, float, float, float);
extern float func_001F9D48(void *, void *);
extern float func_001F9B88(float);

/* searches the table for an entry near a point and reports it */
int func_L03_00205E28(void *arg0, int *arg1, int *arg2, int *arg3, float *arg4, void **arg5, int *arg6, int *arg7) {
    float vec[4];
    int o10;
    float o14;
    int i = 0;
    qcopy(vec, arg0);
    for (; i < D_L03_0015F7F0; i++) {
        if (arg6 != 0 && D_L03_0015F7EC[i].p == arg6) continue;
        if (arg7 != 0 && D_L03_0015F7EC[i].p != arg7) continue;
        if (*D_L03_0015F7EC[i].p == 0) continue;
        if (D_L03_0015F7EC[i].r < func_001F9D10(&D_L03_0015F7EC[i], vec)) continue;
        if (func_L00_0025EFC0(D_L03_0015F7EC[i].p, vec, arg2, &o10, &o14, D_L03_0015F7EC[i].q, 12.0f, 10.0f, 0.0f) == 0) continue;
        if (!(func_001F9D48(vec, arg2) < 1.7f)) continue;
        if (!(func_001F9B88(vec[2] - ((float *)arg2)[2]) < 1.5f)) continue;
        *arg1 = (int)D_L03_0015F7EC[i].p;
        *arg3 = o10;
        *arg4 = o14;
        *arg5 = D_L03_0015F7EC[i].q;
        return 1;
    }
    return 0;
}
