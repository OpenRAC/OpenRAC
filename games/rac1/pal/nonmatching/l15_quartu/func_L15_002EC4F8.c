/* NON_MATCHING func_L15_002EC4F8 -- src/overlays/l15_quartu/vendor_0029C1D0.c
 * Best so far: SIZE ours 840 / retail 848, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Draws a ring of 0x44 sprite points around a moby (vec3 positions, packed colour, vec2 offsets), then submits t
 *   Best candidate p7.c: SIZE 840 vs 848, everything else in shape. Retail keeps (out array + 4) in a register ($3
 *   and spills the `&t[1]` pointer to the stack; ours rematerialises lui/addiu D_L15_001DE4D0+4 inside the loop (g
 *   off + (int)oy, oy = out + 1 before the loop, oy = D + 1 at the top). Loop 2 and the k loop already match in sh
 *   Would unblock: a spelling that stops cse folding the second base into a constant, so the loop-invariant is hoi
 */
extern float func_001FA888(int);
extern int func_001F4868(int);
extern void func_00234C98(int, long);
extern float func_001F9878(float);
extern float func_001FA748(float, float);
extern void func_L00_00251358(void *, int *, int *, int *);
extern float func_001F9B50(float);
extern float func_L00_00200210(float, float);
extern float func_001FA7D8(float x);
extern float func_001F9FA8(float);
extern void func_001F7868(void);
extern void func_L00_001FDE48(int, int, int, void *, int);
extern int D_L15_0015F6B0 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern float D_L15_001DE4D0[];
extern float D_L15_001DDE70[];
extern int D_L15_001DEA20[];
extern float D_L15_001DE1A0[];
extern float D_L15_001DE800[];
extern float D_L15_001621F0[];
extern short D_L15_001621EC;
extern short D_L15_001621D8;
extern short D_L15_001621D4;
extern short D_L15_001621E4;
extern short D_L15_001621DC;
extern short D_L15_001621E0;
extern short D_L15_001621E8;
extern short D_L15_001621A0;
extern short D_L15_001621C0;
extern short D_L15_001621C8;
extern short D_L15_001621D0;
typedef struct { float f[4]; } V4;

/* Draws the level's animated ring of sprites around a moby. */
void func_L15_002EC4F8(char *moby) {
    char *data = *(char **)(moby + 0x78);
    float scale;
    float w;
    int a, b, c;
    int color;
    float *q;
    int i, k;
    unsigned j;
    float *in = D_L15_001DDE70;
    float *out = D_L15_001DE4D0;
    float *oy;
    int *cp = D_L15_001DEA20;
    int off = 0;
    float t[4];
    scale = func_001FA888(D_L15_0015F6B0) * (*(float *)&D_L15_001621EC * D_0015EE6C);
    func_00234C98(6, func_001F4868(*(int *)&D_L15_001621D8));
    func_00234C98(0x42, ((long)*(int *)&D_L15_001621D4 << 32) | 0x44);
    func_00234C98(8, 0);
    func_00234C98(0x14, 0xFF9000000260L);
    *(float *)data = *(float *)(data + 4);
    w = 360.0f / func_001F9878(*(float *)&D_L15_001621E4) * 0.017453292f * D_0015EE6C;
    *(float *)(data + 4) = func_001FA748(*(float *)(data + 4), w);
    func_L00_00251358(moby, &a, &b, &c);
    q = t + 1;
    color = (*(int *)&D_L15_001621DC << 24) | (c << 16) | (b << 8) | a;
    oy = out + 1;
    for (j = 0; j < 0x44; j++) {
        float r, s;
        r = func_001F9B50(in[0] * in[0] + in[1] * in[1]);
        r = func_L00_00200210(r, *(float *)&D_L15_001621E0);
        r = func_001FA7D8(r * 6.2831855f / *(float *)&D_L15_001621E0);
        r = func_001FA748(r, *(float *)(data + 4));
        s = func_001F9FA8(r);
        out[0] = in[0] + *(float *)(moby + 0x10);
        *(float *)(off + (int)oy) = in[1] + *(float *)(moby + 0x14);
        out[2] = in[2] + *(float *)(moby + 0x18) + *(float *)&D_L15_001621E8 * s;
        *cp++ = color;
        in += 3;
        out += 3;
        off += 12;
    }
    i = 0;
    do {
        for (j = 0; j < 0x44; j++) {
            *(V4 *)t = *(V4 *)D_L15_001621F0;
            D_L15_001DE800[j * 2] = D_L15_001DE1A0[j * 2] + func_L00_00200210(scale * t[i * 2], 1.0f);
            D_L15_001DE800[j * 2 + 1] = D_L15_001DE1A0[j * 2 + 1] + func_L00_00200210(scale * q[i * 2], 1.0f);
        }
        func_001F7868();
        for (k = 0; k >= 0; k--) {
            func_L00_001FDE48(((int *)&D_L15_001621A0)[-k], ((int *)&D_L15_001621C0)[-k], ((int *)&D_L15_001621D0)[-k], (int *)&D_L15_001621C8 + -k, 1);
        }
        i++;
    } while (i < 2);
}
