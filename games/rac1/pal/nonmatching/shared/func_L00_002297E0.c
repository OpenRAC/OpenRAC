/* NON_MATCHING func_L00_002297E0 -- src/overlays/shared/help_002297E0.c
 * Best so far: SIZE ours 912 / retail 920, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Picks a weighted-random eligible entry from the 5-entry (0x70 byte) table at D_L00_00179990 (conditions per sl
 *   p1.c (cand[20], qcopy for the two vector copies, plain &D[i] indexing) matches the structure and saved registe
 *   Probable wall: a per-function flag (-mno-split-addresses, the base address as one `la` pseudo so combine can f
 */
typedef struct { char pad0[0x44]; int f44; char pad48[8]; int f50; char pad54[4]; float f58; char pad5c[4]; int f60; char pad64[0xC]; } Ent;
typedef struct { float f[4]; } __attribute__((aligned(16))) V4s;
extern Ent D_L00_00179990[] NOT_SDA;
extern int D_L00_00179BB0 MACRO_ADDR;
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern char D_0013E633[];
extern int func_L00_0020DB30(int);
extern float func_002140F8(float, float);
extern int func_L00_001EFFF0(void *, void *, int, int, int);
extern int func_001F9850(int);
extern float func_001F9878(float);

// Picks a random eligible entry: gathers the candidates and selects by weighted roll.
int func_L00_002297E0(int *out) {
    char *g = D_0013E633 + 0xE1D;
    int cand[20];
    int n = 0;
    int i, j;
    float f20, f21;
    V4s va, vb;
    if (*(short *)(g + 0x1E8) != 0) {
        return 0;
    }
    if (*(int *)(g + 0x22A8) == 1) {
        *out = 4;
        if (D_L00_00179BB0 != 0) return 0;
        if ((*(int *)(g + 0xA98) & 2) == 0) return 0;
        if (func_002140F8(0.0f, 1.0f) < 0.33f) return 1;
        return 0;
    }
    for (i = 0; i < 5; i++) {
        Ent *e = &D_L00_00179990[i];
        if (e->f60 != 0) continue;
        if (e->f58 == 0.0f) continue;
        {
            int a = func_L00_0020DB30(0);
            if (e->f50 != -1 && e->f50 != a) continue;
        }
        if (e->f44 != 0 && *(int *)(g + 0x2FC) != 0) continue;
        if (i == 3) {
            if (D_0015EE84_m != 0xC) continue;
            if (func_L00_0020DB30(0) == 0x17) continue;
            qcopy(&va, g + 0xD0);
            qcopy(&vb, &va);
            vb.f[2] = vb.f[2] + 8.0f;
            if (func_L00_001EFFF0(&va, &vb, 2, *(int *)(g + 0x2080), 0) != 0) continue;
        }
        if (i == 4) {
            char *h = D_0013E633 + 0xE1D;
            if (*(int *)(h + 0x2094) != 1) continue;
            if (func_001F9850(0x32) < *(int *)(h + 0x198)) continue;
            if (*(int *)(h + 0x22A8) >= 2) continue;
        }
        cand[n] = i;
        n++;
    }
    if (n == 0) return 0;
    f20 = 0.0f;
    for (j = 0; j < n; j++) {
        f20 += 1.0f / (float)(int)(func_001F9878(D_L00_00179990[cand[j]].f58) * 60.0f);
    }
    f21 = func_002140F8(0.0f, 1.0f);
    if (f21 < f20) {
        for (j = 0; j < n; j++) {
            *out = cand[j];
            f20 -= 1.0f / (float)(int)(func_001F9878(D_L00_00179990[cand[j]].f58) * 60.0f);
            if (f20 < f21) return 1;
        }
        return 1;
    }
    return 0;
}
