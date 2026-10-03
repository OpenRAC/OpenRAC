/* NON_MATCHING func_L03_002DBAD8 -- src/overlays/l03_kerwan/vendor_002CB280.c
 * Best so far: SIZE ours 344 / retail 352, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby-style helper: if D_L03_0015F6A8==2 and D_0015EEB0[0]: sets D_L03_0015F680 to -0.2f / +0.2f by two r
 *   Best p6.c (non-MACRO float store): 45/352 bytes. Retail stores the two floats with `lui $at` (MACRO_ADDR form,
 *   but tools/ps2eeas_nops.py then refuses ("object has 0 mtc1 uses; source has 2 -- refusing to guess": li.s foll
 *   Other finds: q[1] is unsigned (lbu), store order 0x28,0x20,0x24 = source order 0x20,0x24,0x28. The three uses 
 */
extern void func_0020D960(char *, int, void *);
extern int D_L03_0015F6A8 MACRO_ADDR;
extern unsigned char D_0015EEB0[] MACRO_ADDR;
extern char D_L03_0016C9E0[];
extern float D_L03_0015F680;
extern char D_L03_0017C5C0[];
extern short D_L03_00161C34;

/* Nudges a scroll value by the current section of the level and attaches a manipulator to matching mobys. */
void func_L03_002DBAD8(char *m) {
    if (D_L03_0015F6A8 == 2 && D_0015EEB0[0] != 0) {
        char *s = D_L03_0016C9E0;
        int i;
        if (*(int *)(s + 0x30) == 2) {
            int v = *(int *)(s + 0x34);
            if (v >= 0x1EA) {
                if (v < 0x266) D_L03_0015F680 = -0.2f;
            }
        }
        s = D_L03_0016C9E0;
        if (*(int *)(s + 0x30) == 2) {
            int v = *(int *)(s + 0x34);
            if (v >= 0x2EE) {
                if (v < 0x321) D_L03_0015F680 = 0.2f;
            }
        }
        s = D_L03_0016C9E0;
        for (i = 0; i < *(short *)(s + 0x44); i++) {
            char *o = *(char **)(s + 0x178 + i * 4);
            if (*(short *)(o + 0xA6) == *(short *)(m + 0xA6)) {
                char *q = D_L03_0017C5C0;
                if (q[1] == 0) {
                    float f;
                    func_0020D960(o, 0, q);
                    f = *(float *)&D_L03_00161C34;
                    *(float *)(q + 0x28) = f;
                    *(float *)(q + 0x20) = f;
                    *(float *)(q + 0x24) = f;
                }
            }
        }
    }
}
