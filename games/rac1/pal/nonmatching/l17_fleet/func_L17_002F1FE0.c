/* NON_MATCHING func_L17_002F1FE0 -- src/overlays/l17_fleet/vendor_002F1558.c
 * Best so far: SIZE ours 1048 / retail 1040, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   - run13 p9 (probe: j-body calls removed): still no reduction, temp-form i+1 at loop top; so not size. Next: ex
 *   - run14 p10 (for with explicit int induction vars pc,pb,pd,off in the for-increment clause): temp-form again (
 *   - run15 p11 (while + explicit int ptrs in body): temp-form persists; w-local (float w = D_..D8 before data[0]=
 *   - run16 p13 (per-i element pointers): SIZE 1024; i still biv (temp-form), no reduction
 *   - run17 p14 (shared var i for A, m, q): SIZE 1064 worse
 *   - run18 p12 (p8 + w-local): SIZE 1016 vs 1040. BEST FILE. Prologue/first region and loop B / final loop match 
 *   STOP: loop A. Retail strength-reduces and eliminates i (four givs &cnt[i], &B[i], &D[i] (spilled), i*4 for E (
 *   Ours keeps i with the early addu t,i,1 / move i,t form and la+addu per use; tried index loop, i+=1, do-while, 
 */
extern float func_001FA888(int);
extern int func_001F4868(int);
extern void func_00234C98(int, long);
extern float func_001F9878(float);
extern float func_001FA748(float, float);
extern void func_L00_00251358(void *, void *, void *, void *);
extern float func_001F9B50(float);
extern float func_L00_00200210(float, float);
extern float func_001FA7D8(float x);
extern float func_001F9FA8(float);
extern void func_001F7868(void);
extern void func_L00_001FDE48(int, int, int, void *, int);
extern float D_0015EE6C MACRO_ADDR;
extern int D_L17_0015F6B0 MACRO_ADDR;
extern short D_L17_00162290;
extern short D_L17_00162298;
extern short D_L17_001622A0;
extern short D_L17_001622A8;
extern short D_L17_001622B0;
extern short D_L17_001622B8;
extern short D_L17_001622C8;
extern short D_L17_001622CC;
extern short D_L17_001622D0;
extern short D_L17_001622D4;
extern short D_L17_001622D8;
extern short D_L17_001622DC;
extern short D_L17_001622E0;
typedef struct { float v[2]; } P2;
typedef struct { P2 p[2]; } T4;
extern T4 D_L17_001622E8;

/* Draws the two sets of wavering points around the moby and queues them. */
void func_L17_002F1FE0(char *moby) {
    char *data = *(char **)(moby + 0x78);
    float scale, w;
    int r, g, b;
    int color;
    int cp = (int)&D_L17_00162290, bp = (int)&D_L17_00162298, dp = (int)&D_L17_001622A8, eo = 0;
    int i, j, k, m, q;

    scale = func_001FA888(D_L17_0015F6B0) * (*(float *)&D_L17_001622E0 * D_0015EE6C);
    func_00234C98(6, func_001F4868(*(int *)&D_L17_001622CC));
    func_00234C98(0x42, ((long)*(int *)&D_L17_001622C8 << 32) | 0x44);
    func_00234C98(8, 0);
    func_00234C98(0x14, ((long)0xFF90 << 32) | 0x260);
    w = *(float *)&D_L17_001622D8;
    *(float *)data = *(float *)(data + 4);
    *(float *)(data + 4) = func_001FA748(*(float *)(data + 4),
        360.0f / func_001F9878(w) * 0.017453292f * D_0015EE6C);
    func_L00_00251358(moby, &r, &g, &b);
    color = (*(int *)&D_L17_001622D0 << 24) | (b << 16) | (g << 8) | r;
    do {
        for (j = 0; j < *(int *)cp; j++) {
            float *p = (float *)(*(int *)((int)&D_L17_001622B0 + eo) + j * 12);
            float t = *(float *)&D_L17_001622DC *
                func_001F9FA8(func_001FA748(func_001FA7D8(func_L00_00200210(func_001F9B50(p[0] * p[0] + p[1] * p[1]), *(float *)&D_L17_001622D4) * 6.2831855f / *(float *)&D_L17_001622D4), *(float *)(data + 4)));
            *(float *)(j * 12 + *(int *)bp) = p[0] + *(float *)(moby + 0x10);
            *(float *)(j * 12 + *(int *)bp + 4) = p[1] + *(float *)(moby + 0x14);
            *(float *)(j * 12 + *(int *)bp + 8) = p[2] + *(float *)(moby + 0x18) + t;
            *(int *)(j * 4 + *(int *)dp) = color;
        }
        dp += 4;
        bp += 4;
        eo += 4;
    } while ((cp += 4) < (int)&D_L17_00162290 + 8);
    for (k = 0; k < 2; k++) {
        for (m = 0; m < 2; m++) {
            for (j = 0; j < ((int *)&D_L17_00162290)[m]; j++) {
                T4 tbl = D_L17_001622E8;
                *(float *)(j * 8 + ((int *)&D_L17_001622A0)[m]) = *(float *)(j * 8 + ((int *)&D_L17_001622B8)[m]) + func_L00_00200210(scale * tbl.p[k].v[0], 1.0f);
                *(float *)(j * 8 + ((int *)&D_L17_001622A0)[m] + 4) = *(float *)(j * 8 + ((int *)&D_L17_001622B8)[m] + 4) + func_L00_00200210(scale * tbl.p[k].v[1], 1.0f);
            }
        }
        func_001F7868();
        for (q = 0; q < 2; q++) {
            func_L00_001FDE48(((int *)&D_L17_00162290)[q], ((int *)&D_L17_00162298)[q], ((int *)&D_L17_001622A8)[q], (void *)((int *)&D_L17_001622A0)[q], 1);
        }
    }
}
