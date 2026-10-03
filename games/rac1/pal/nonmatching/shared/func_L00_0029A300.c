/* NON_MATCHING func_L00_0029A300 -- src/overlays/shared/tieproc_00299108.c
 * Best so far: SIZE ours 1220 / retail 1228, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * Cannot land as written (#define in a candidate): rewrite that in plain C first.
 * What the last attempts found:
 *   func_L00_0029A300 (cutscene-mode update: camera-track pacing, per-moby pose blend, frame steps): budget spent 
 *   Differences: (1) retail hoists `lui` of D_L00_001610E8 / D_L00_001610F8 into $23/$30 before the loop and on th
 *   (2) retail's loop is not strength-reduced (`sll; addu $2,$19,$2; lw 0x178($2)`, base is a copy `daddu $19,$16,
 *   Idioms found: statement-expression pointers ({ char *p = SYM; *(T*)(p+off); }) reproduce retail's hi-once / ad
 *   Would unblock: the source form that makes gcc keep hi(E8)/hi(F8) in $23/$30 and leave the loop giv unreduced (
 */
#include "common.h"

extern char D_L00_0016C158[];
extern char D_L00_001610B8[];
extern char D_L00_001610C8[];
extern char D_L00_001610D8[];
extern char D_L00_001610E8[];
extern char D_L00_001610F8[];
extern char D_L00_0016C960[];
extern unsigned char D_0013A5E0[];
extern float D_L00_0015F4FC_m __asm__("D_L00_0015F4FC") MACRO_ADDR;
extern int D_0015EF20 MACRO_ADDR;
extern int D_0015EFD8 MACRO_ADDR;
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern short D_0015EFA0;
extern int D_0015EE80 MACRO_ADDR;

extern void func_L00_00299108(void);
extern void func_L00_0023EC00(void);
extern void func_00213C78(void);
extern void func_0022EF68(void);
extern void func_001E9768(void *, int);
extern void func_L00_002076E8(void);
extern void func_00218A80(void);
extern int func_00216960(void);
extern int func_001F9850(int);
extern void func_L00_00299E70(void);
extern void func_00217748(int);
extern void func_00204FC0();
extern void func_L00_00245B88(int);
extern unsigned char func_L00_0029A158(void);
extern void func_0020D6D0(void *);
extern float func_001FA888(int);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_00251E30(void *);
extern void func_00214550(char *);
extern void func_L00_002353B8(char *);
extern void func_L00_00207A48(char *);
extern void func_0022DD68(void);
extern void func_00202260(void);
extern void func_0020DE20(void);
extern void func_L00_00299148(void);

#define FL ({ char *cf = D_L00_0016C158; *(int *)(cf + 8); })
#define PADP(T, o) ({ char *pp = (char *)D_0013A5E0 + 0x2460; *(T *)(pp + (o)); })
#define PAD (*(int *)(D_0013A5E0 + 0x2604))

/* Per-frame cutscene update: pace the camera track, drive the cutscene mobys and run the frame steps. */
void func_L00_0029A300(void) {
    char *d, *d2, *dd;
    int r, lim, i, s21, t;
    char *m, *base;
    float v[4];
    float w[4];

    if ((FL & 2) || ((FL & 0x10) && (PAD & 0x200))) {
        func_L00_00299108();
    }
    if (!(FL & 0x10) || (PAD & 0x220)) {
        func_L00_0023EC00();
    }
    if ((FL & 2) || ((FL & 0x10) && (PAD & 0x200))) {
        func_00213C78();
        func_0022EF68();
    }
    func_001E9768(D_L00_001610B8, 3);
    if ((FL & 1) || ((FL & 0x10) && (PAD & 0x200))) {
        func_L00_002076E8();
    }
    func_001E9768(D_L00_001610C8, 7);
    if ((FL & 4) || ((FL & 0x10) && (PAD & 0x200))) {
        func_00218A80();
    }
    func_001E9768(D_L00_001610D8, 5);
    d = D_L00_0016C960;
    {
        float f = D_L00_0015F4FC_m - 0.34f;
        *(int *)(d + 0x34) += 1;
        *(int *)(d + 0x38) += 1;
        D_L00_0015F4FC_m = f;
        if (f < 0.0f) {
            D_L00_0015F4FC_m = 0.0f;
        }
    }
    if (*(int *)(d + 0x34) >= *(short *)(d + 0x46)) {
        func_00216960();
    }
    r = *(int *)(d + 0x34) >= *(short *)(d + 0x40);
    if (!(*(int *)(d + 0x34) < func_001F9850(0x12)) && D_L00_0015F4FC_m == 0.0f) {
        if ((*(int *)&D_0015EFA0 != 0 || D_0015EF20 != 0 || D_0015EFD8 != 0 || D_0015EE84_m <= 0) &&
            (PADP(int, 0x1A4) & 0x800)) {
            r = 1;
        } else if ((PADP(long, 0x1A0) & 0x8000000000FL) == 0x8000000000FL) {
            r = 1;
        }
    }
    d2 = D_L00_0016C960;
    if (r) {
        func_L00_00299E70();
    } else {
        lim = 0x60;
        if (D_0015EE80) {
            lim = 0x50;
        }
        if (*(int *)(d2 + 0x38) >= lim) {
            func_00217748(1);
            *(int *)(d2 + 0x3C) += 1;
            func_00204FC0();
            func_L00_00245B88(*(int *)(d2 + 0x3C) + 1);
        }
        s21 = func_L00_0029A158();
        for (i = 0; i < *(short *)(d2 + 0x44); i++) {
            dd = d2;
            m = *(char **)(dd + (i << 2) + 0x178);
            t = *(int *)(dd + 0x38) >> 1;
            m[0x51] = t + 1;
            m[0x50] = t;
            func_0020D6D0(m);
            *(float *)(m + 0x54) = func_001FA888(*(int *)(dd + 0x38) & 1) * 0.5f;
            if (s21 && (*(int *)(dd + 0x38) & 1)) {
                *(float *)(m + 0x54) = 1.0f;
            }
            base = *(char **)(m + 0x78);
            func_001F9C30(v, base + (((unsigned char)m[0x50]) << 4), 1.0f - *(float *)(m + 0x54));
            func_001F9C30(w, base + (((unsigned char)m[0x51]) << 4), *(float *)(m + 0x54));
            func_001F9BD8(m + 0x10, v, w);
            m[0x71] = 0xFF;
            func_L00_00251E30(m);
            if (*(unsigned char *)(m + 0x7F)) {
                func_00214550(m);
            }
            if (*(short *)(m + 0xA6) == 0) {
                func_L00_002353B8(m);
            }
            if (*(short *)(m + 0xA6) == 0xA || *(short *)(m + 0xA6) == 0x1A3 || *(short *)(m + 0xA6) == 0x555) {
                func_L00_00207A48(m);
            }
        }
    }
    func_001E9768(D_L00_001610E8, 8);
    func_0022DD68();
    func_001E9768(D_L00_001610F8, 6);
    func_00202260();
    func_0020DE20();
    if ((FL & 2) || ((FL & 0x10) && (PAD & 0x200))) {
        func_L00_00299148();
    }
}
