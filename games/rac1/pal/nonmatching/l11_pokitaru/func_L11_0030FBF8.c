/* NON_MATCHING func_L11_0030FBF8 -- src/overlays/l11_pokitaru/vendor_002CC828.c
 * Best so far: SIZE ours 572 / retail 580, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L11_0030FBF8(a): marks the 8 link bytes (+0xC..+0x13) of 0x1190-byte cell `a` as 0xFF, then for every oth
 *   records i in the link slot matching the +-16 offset on x or y (four-way if/else chain on |dx|,|dy|,|dx+16|,|dx
 *   Structure and the stores' order are right (p3/p5/p6 match the prologue store order and all branches). Differen
 *   Retail recomputes `a * 0x1190` (mult in the delay slot of the i==a test, constant 0x1190 hoisted into $21) and
 *   ours CSEs the prologue's mult into the loop (cse_around_loop) and keeps it in a saved reg plus a strength-redu
 *   Every wording tried (char* base, Cell[] index, if/continue, do-while) merges the two computations. Would need 
 *   which the loop's record address is not seen as equal to the prologue's (unknown).
 */
extern float func_001F9B88(float);

/* Links grid cell a to the neighbours one step away (16 units) in each direction. */
void func_L11_0030FBF8(int a) {
    int i;
    char *base = D_L11_001DAB40;
    char *p;
    char *r = base + a * 0x1190;
    r[0x13] = 0xFF;
    r[0xC] = 0xFF;
    r[0xE] = 0xFF;
    r[0xD] = 0xFF;
    r[0xF] = 0xFF;
    r[0x10] = 0xFF;
    r[0x11] = 0xFF;
    r[0x12] = 0xFF;
    p = base;
    for (i = 0; i < 20; i++, p += 0x1190) {
        float dx;
        float dy;
        if (i == a) continue;
        r = base + a * 0x1190;
        dx = *(float *)r - *(float *)p;
        dy = *(float *)(r + 4) - *(float *)(p + 4);
        if (func_001F9B88(dx) < 0.1f) {
            if (func_001F9B88(dy - 16.0f) < 0.1f) r[0xC] = i;
            if (func_001F9B88(dy + 16.0f) < 0.1f) r[0xE] = i;
        } else if (func_001F9B88(dy) < 0.1f) {
            if (func_001F9B88(dx - 16.0f) < 0.1f) r[0xD] = i;
            if (func_001F9B88(dx + 16.0f) < 0.1f) r[0xF] = i;
        } else if (func_001F9B88(dx + 16.0f) < 0.1f) {
            if (func_001F9B88(dy - 16.0f) < 0.1f) r[0x11] = i;
            if (func_001F9B88(dy + 16.0f) < 0.1f) r[0x13] = i;
        } else if (func_001F9B88(dx - 16.0f) < 0.1f) {
            if (func_001F9B88(dy - 16.0f) < 0.1f) r[0x10] = i;
            if (func_001F9B88(dy + 16.0f) < 0.1f) r[0x12] = i;
        }
    }
}
