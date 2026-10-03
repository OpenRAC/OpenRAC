/* NON_MATCHING func_L00_0026F248 -- src/overlays/shared/partupd_0026A130.c
 * Best so far: SIZE ours 704 / retail 712, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   PartType29Update: per-frame update of an owner-following particle (fade, bob limit, tint, orientation blend). 
 *   Retail picks the bounce threshold 0.3f/0.15f in two separate arms (li.s, b, nop / li.s) joined at one c.lt; gc
 *   Every wording of the if/else/ternary gives that hoisted form; inline ternary inside the compare duplicates the
 */
extern int func_001F9938(void *);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_L00_002688A8(void *);
extern void func_L00_00250800(void *, int, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern float D_0015EE60 MACRO_ADDR;
extern unsigned char *D_L00_001B2474;
extern char D_L00_00166EC0[];

/* Updates a particle that follows its owner: fades, bobs and tints, then blends its orientation. */
void func_L00_0026F248(char *m) {
    char *p;
    if (m == 0) return;
    p = m + 0x20;
    if (func_001F9938(m + 0xA) || *(int *)(m + 0x20) == 0 || *(char *)(*(int *)(m + 0x20) + 0x20) < 0) {
        func_L00_002688A8(m);
        return;
    }
    *(short *)(p + 0xC) = (*(unsigned short *)(p + 0xC) + 1) & 0xF;
    if (*(float *)(m + 0xC) < 159600.0f) {
        *(float *)(m + 0xC) += D_0015EE60 * 3989.9998f;
        if (*(float *)(m + 0xC) > 159600.0f) *(float *)(m + 0xC) = 159600.0f;
    } else if (*(float *)(m + 0xC) > 159600.0f) {
        *(float *)(m + 0xC) -= D_0015EE60 * 15959.999f;
        if (*(float *)(m + 0xC) < 159600.0f) *(float *)(m + 0xC) = 159600.0f;
    }
    *(float *)(p + 8) += D_0015EE60 * 0.001f;
    m[8] = func_001FA898_r(*(float *)(p + 8) * 255.0f);
    m[2] = D_L00_001B2474[*(short *)(p + 0xC)];
    if (*(float *)(m + 0xC) <= 159600.0f || *(float *)(p + 0x10) > 0.0f) {
        *(float *)(p + 4) += *(float *)(p + 0x10);
    }
    {
        float thr;
        float pos;
        if (*(float *)(m + 0xC) <= 159600.0f) {
            thr = 0.3f;
            pos = *(float *)(p + 4);
        } else {
            thr = 0.15f;
            pos = *(float *)(p + 4);
        }
        if (thr < pos) {
            *(float *)(p + 0x10) = -*(float *)(p + 0x10);
            *(float *)(p + 4) += *(float *)(p + 0x10);
        } else if (pos < 0.0f) {
            func_L00_002688A8(m);
            return;
        }
    }
    {
        int r = func_001FA898_r(*(float *)(p + 4) * 256.0f);
        *(int *)(m + 4) = (*(int *)(m + 4) & 0xFFFFFF) + (r << 24);
    }
    *(float *)(p + 0x14) += (1.0f - *(float *)(p + 0x14)) * (D_0015EE60 * 0.175f);
    if (*(short *)(p + 0xE) != -1) {
        char v[0x20];
        func_L00_00250800(*(void **)p, *(short *)(p + 0xE), v);
        func_001F9BF0(v + 0x10, D_L00_00166EC0, v);
        func_L00_001FF4B0(v + 0x10, v + 0x10, *(float *)(p + 0x14));
        func_001F9BD8(m + 0x10, v, v + 0x10);
    }
}
