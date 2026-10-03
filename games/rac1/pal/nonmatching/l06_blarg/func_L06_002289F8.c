/* NON_MATCHING func_L06_002289F8 -- src/overlays/l06_blarg/help_00223630.c
 * Best so far: BYTES 6/616 (99.0% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Follow-camera easing: picks target values by mode byte at +0x20A4, adjusts by state, then moves three values w
 *   p4.c is 6 bytes off retail (same size): only the two loads before 'slt' are swapped (retail lw 0x198 then lw 0
 *   Idiom: separate locals b/b2/b3 each assigned D_0013E633 + 0xE1D keep the hi register shared; store order 0x228
 */
extern float func_001F9D48(float *, float *);
extern float func_00214D28(float *p, float target, float maxstep);
extern float func_L00_0025C918(float *p, float *v, float t, float u1, float u2, float eps);
extern float D_0015EE60 MACRO_ADDR;
extern float D_0015EE64 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern char D_0013E633[];

// Eases the follow-camera distance/height/tilt toward targets picked from the current mode.
void func_L06_002289F8(void) {
    char *b = D_0013E633 + 0xE1D;
    char *d;
    char *b2, *b3, *e;
    switch (*(unsigned char *)(b + 0x20A4)) {
    case 0:
        *(float *)(b + 0x228) = 0.8f;
        *(float *)(b + 0x22C) = 0.7f;
        *(float *)(b + 0x230) = 0.45f;
        break;
    case 1:
        *(float *)(b + 0x228) = 0.59999996f;
        *(float *)(b + 0x22C) = 0.45000002f;
        *(float *)(b + 0x230) = 0.3f;
        break;
    case 3:
        *(float *)(b + 0x228) = 0.8f;
        *(float *)(b + 0x22C) = 0.6f;
        *(float *)(b + 0x230) = 0.45f;
        break;
    }
    b2 = D_0013E633 + 0xE1D;
    if (*(int *)(b2 + 0x208C) == 4) {
        if (*(int *)(b2 + 0x420) < *(int *)(b2 + 0x198)) {
            if (*(short *)(b2 + 0x41E) == 0) *(float *)(b2 + 0x22C) = *(float *)(b2 + 0x434);
        }
    } else if (*(int *)(b2 + 0x2084) == 6) {
        *(float *)(b2 + 0x22C) = 0.5f;
    } else if (*(int *)(b2 + 0x2084) == 4) {
        *(float *)(b2 + 0x228) = 0.35000002f;
    } else if (*(int *)(b2 + 0x2084) == 0x7F) {
        *(float *)(b2 + 0x230) = 0.8f;
    }
    b3 = D_0013E633 + 0xE1D;
    if (*(unsigned char *)(b3 + 0x257) != 0 && *(int *)(b3 + 0x2094) != 0x12 &&
        *(int *)(b3 + 0x208C) != 0x11 && *(unsigned char *)(b3 + 0x12E4) == 0) {
        float dist = func_001F9D48((float *)(b3 + 0x210), (float *)(b3 + 0x80));
        if (!(*(float *)(b3 + 0x234) * 0.5f < dist)) return;
    }
    d = D_0013E633 + 0x103D;
    e = d - 0x220;
    func_00214D28((float *)d, *(float *)(e + 0x228), D_0015EE60 * 0.02f);
    func_00214D28((float *)(d + 4), *(float *)(e + 0x22C), D_0015EE60 * 0.02f);
    func_L00_0025C918((float *)(d + 0x14), (float *)(d + 0x18), *(float *)(e + 0x230),
                      D_0015EE64 * 0.02f, D_0015EE64 * 0.3f, D_0015EE6C * 4.0f);
}
