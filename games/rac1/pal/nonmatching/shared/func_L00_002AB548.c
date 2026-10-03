/* NON_MATCHING func_L00_002AB548 -- src/overlays/shared/vendor_002A5138.c
 * Best so far: SIZE ours 968 / retail 964, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Projectile-path trace: from a0 steps w/B with gravity until func_001F9908(&x) reports done or the hit test pas
 *   Best p4.c: 968 vs 964 bytes; left: else-branch (o[0x20]!=0) retail does 'lh x; daddu pB; lw d2[0x50]; beqz; sw
 */
extern float func_L00_001FF860(float, float);
extern float func_001FA748(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9BD8(void *, void *, void *);
extern char *func_L00_0025D390(void *);
extern int func_001F9850(int);
extern int func_L00_001EFFF0(void *, void *, int, int, int);
extern int func_L00_001F3958(void);
extern int func_001F9908(int *arg0);
extern void func_001F49B0(void (*)(void), void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern void func_L00_002AAEF0(void);
extern int D_L00_0015F6A8 MACRO_ADDR;
extern int D_L00_00173F40[];
extern float D_0015EE70 MACRO_ADDR;
extern void *D_L00_00173F58;
extern float D_L00_00173F80[];
extern char D_0013E633[];

// Traces a projectile path from a0 step by step and resolves where it lands.
int func_L00_002AB548(char *a0, char *o, int mode) {
    float w[4];
    float B[4];
    float C[4];
    float box[4];
    int x;
    char *base = D_0013E633 + 0xE1D;
    int i;
    float *pB;
    char *tmp;
    if (*(int *)(base + 0x2084) == 0x72) return 0;
    if (D_L00_0015F6A8 != 0) return 0;
    qcopy(C, o + 0x10);
    if (*(unsigned char *)(o + 0x20) == 0) {
        w[0] = func_001F9F90(func_001FA748(-0.36196801f, func_L00_001FF860(*(float *)(base + 0x670), *(float *)(base + 0x674)))) * 0.859039f;
        w[1] = func_001F9FA8(func_001FA748(-0.36196801f, func_L00_001FF860(*(float *)(base + 0x670), *(float *)(base + 0x674)))) * 0.859039f;
        *(int *)&w[2] = 0;
        func_001F9BD8(w, w, base + 0x80);
        w[2] = w[2] + 0.49082f;
        qcopy(B, a0);
        pB = B;
        if (*(int *)(base + 0x2FC) != 0 && func_L00_0025D390((void *)*(int *)(base + 0x2FC)) != 0 && mode != 0) {
            func_001F9BD8(pB, pB, base + 0x100);
        }
        x = func_001F9850(0x78);
    } else {
        tmp = *(char **)(*(char **)(a0 + 0x30) + 0x78);
        qcopy(w, o + 0x10);
        qcopy(B, a0);
        pB = B;
        x = *(short *)(a0 + 0x34);
        if (*(int *)(tmp + 0x50) != 0) *(short *)(a0 + 0x36) = 1;
    }
    {
        char *base2 = D_0013E633 + 0xE1D;
        char *b2 = *(char **)(base2 + 0x2080);
        *(u128 *)box = *(u128 *)(b2 + 0x10);
        box[2] = w[2];
        if (func_L00_001EFFF0(box, w, 0x10, (int)b2, 0) != 0) {
            if (func_L00_001F3958() != 0 && D_L00_00173F40[7] > 0) {
                qcopy(a0 + 0x10, (char *)D_L00_00173F40 + 0x20);
            }
        }
    }
    while (func_001F9908(&x) == 0) {
        qcopy(C, w);
        i = 9;
        do {
            func_001F9BD8(w, w, pB);
            pB[2] = pB[2] - D_0015EE70 * 9.8f;
        } while (--i >= 0);
        if (func_L00_001EFFF0(C, w, 0x12, (int)o, 0) == 0) continue;
        if (func_L00_001F3958() == 0 && 0.0f < pB[2]) continue;
        if (D_L00_00173F40[7] > 0) {
            qcopy(a0 + 0x10, (char *)D_L00_00173F40 + 0x20);
            break;
        }
    }
    if (x != 0) {
        if (mode == 0) {
            if (D_L00_00173F58 == 0) return 0;
            if (func_L00_0025D390(D_L00_00173F58) == 0) return 0;
        }
        if (mode != 2 || D_L00_00173F58 == 0 || func_L00_0025D390(D_L00_00173F58) == 0) {
            func_001F49B0(func_L00_002AAEF0, o);
        }
        qcopy(a0 + 0x20, D_L00_00173F80);
        func_001F9BF0(C, a0 + 0x10, &D_L00_00166EC0);
        func_001F9C30(C, C, 0.95f);
        func_001F9BD8(a0 + 0x10, C, &D_L00_00166EC0);
        return 1;
    }
    return 0;
}
