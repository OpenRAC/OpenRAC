/* NON_MATCHING func_L00_0026F510 -- src/overlays/shared/partupd_0026A130.c
 * Best so far: BYTES 38/1292 (97.1% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Particle update (follows a parent chain with a 64-bit flag test, steers toward a target, fades alpha). Budget 
 *   Structure, loop (do/while with hoisted constants, `ksq = k*k` taken before the BF0 call), stack layout and tai
 *   Also a wall: retail reads D_0015EE64 via $gp in one arm and lui/%lo in the other; one symbol cannot do both (a
 */
extern void func_001F9BC0(void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern float func_001F9CB8(void *);
extern float func_001F9B88(float);
extern int func_L00_001F10E0(float, void *, int, void *);
extern void func_001F9EE8(void *, void *, void *);
extern float func_L00_001FF860(float, float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_L00_001FF610(void *, void *, void *);
extern void func_L00_002688A8(void *);
extern short D_0015EE64_s __asm__("D_0015EE64");
extern char D_L00_00173F70[];
extern char D_L00_00173F80[];
extern char D_L00_00166EC0[];

/* updates a trailing particle: follows its parent chain, steers toward a target, fades out */
void func_L00_0026F510(char *m) {
    char *p = m + 0x20;
    char *mp;
    float v0[4];
    float v1[4];
    float v2[4];
    float v3[4];
    if ((*(unsigned short *)(p + 0x1E) ^ 1) & 1) {
        int n = *(int *)(p + 0x14);
        char *q = 0;
        float k = 2.0f;
        float f;
        float d;
        if (n) q = (char *)n + 0x20;
        func_001F9BC0(v2);
        f = 0.15f;
        mp = m + 0x10;
        if (*(short *)(p + 0x1C) == 0 && n != 0 && !(*(long *)(q + 0x18) & 0x1FFFF00000000L)) {
            do {
                float ksq = k * k;
                float t;
                func_001F9BF0(v0, (char *)n + 0x10, mp);
                t = 0.2f / ksq;
                k += 1.0f;
                func_L00_001FF4B0(v0, v0, t);
                f *= 0.999f;
                func_001F9BD8(v2, v2, v0);
                n = *(int *)(q + 0x14);
                if (n) q = (char *)n + 0x20;
            } while (n && !(*(long *)(q + 0x18) & 0x1FFFF00000000L));
        }
        f = 0.15f - f + 1.07f;
        if (q == 0 || *(short *)(q + 0x1C) == 0) {
            func_001F9BD8(p, p, v2);
        } else {
            func_001F9C30(p, p, 0.5f);
        }
        if (*(int *)(p + 0x14)) {
            func_001F9C30(p, p, f);
            d = *(float *)&D_0015EE64_s;
        } else {
            func_001F9C30(p, p, 0.97f);
            d = *(float *)0x15EE64;
        }
        *(float *)(p + 8) -= d * 0.02f;
        func_001F9BD8(v0, mp, p);
    } else {
        qcopy(v0, m + 0x10);
        mp = m + 0x10;
    }
    if (*(int *)(p + 0x10)) {
        float g;
        func_001F9BF0(v2, v0, *(char **)(p + 0x10) + 0x10);
        g = func_001F9CB8(v2);
        if (0.1f < func_001F9B88(g - 0.15f)) {
            func_L00_001FF4B0(v2, v2, 0.15f);
            func_001F9BD8(v0, v2, *(char **)(p + 0x10) + 0x10);
        }
        if (func_L00_001F10E0(0.15f, v0, 0, 0)) {
            if (*(short *)(p + 0x1C) == 0) {
                qcopy(v0, D_L00_00173F70);
            } else {
                qcopy(v0, mp);
            }
            *(short *)(p + 0x1C) = 1;
        } else {
            *(short *)(p + 0x1C) = 0;
        }
        func_001F9BF0(v2, *(char **)(p + 0x10) + 0x10, D_L00_00166EC0);
        func_001F9EE8(v2, v2, D_L00_00166EC0 - 0x80);
        func_001F9C30(v2, v2, 1.0f / v2[3]);
        func_001F9BF0(v3, v0, D_L00_00166EC0);
        func_001F9EE8(v3, v3, D_L00_00166EC0 - 0x80);
        func_001F9C30(v3, v3, 1.0f / v3[3]);
        func_001F9BF0(v2, v2, v3);
        m[8] = func_001FA898_r(func_L00_001FF860(v2[0], v2[1]) * 128.0f / 3.14159274f) - 0x20;
        if (*(short *)(p + 0x1C)) {
            func_L00_001FF610(p, p, D_L00_00173F80);
            func_001F9C30(p, p, 0.95f);
        } else {
            func_001F9BF0(v1, v0, mp);
            func_001F9BD8(p, p, v1);
            func_001F9C30(p, p, 0.5f);
        }
        func_001F9BF0(v1, v0, mp);
        func_001F9BD8(p, p, v1);
        func_001F9C30(p, p, 0.5f);
    }
    qcopy(mp, v0);
    if (*(unsigned short *)(p + 0x1E) & 2) {
        unsigned c = *(unsigned *)(m + 4);
        int t = (c >> 24) - 2;
        if (t > 0) {
            *(unsigned *)(m + 4) = (c & 0xFFFFFF) | (t << 24);
        } else if (*(int *)(p + 0x14) == 0) {
            char *w = *(char **)(p + 0x10);
            if (w) *(int *)(w + 0x34) = 0;
            func_L00_002688A8(m);
        }
    }
}
