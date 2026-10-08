/* NON_MATCHING func_L07_0030E128 -- src/overlays/l07_umbris/vendor_002CE470.c
 * Best so far: SIZE ours 572 / retail 580, checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Sets the 8 neighbour-link bytes (0xC..0x13) of cell idx (stride 0x1190, 0x24 cells at D_L07_001DD100) to 0xFF,
 *   Best p6/p3: size 580 matches, 59-61 bytes differ. Only real difference: retail recomputes idx*0x1190 (mult wit
 *   Would unblock: a source form where the loop's cell-idx address is not treated as loop invariant; p4/p5 (if (i 
 *   x03 (q30): p11.c is the closest (572 vs 580): me computed through a second alias of D_L07_001DD100 (extern cha
 */
extern char D_L07_001DD100[];
extern char D_me[] __asm__("D_L07_001DD100");
extern float func_001F9B88(float);

/* Marks each of the 0x24 grid cells' neighbour links relative to cell idx. */
void func_L07_0030E128(int idx) {
    unsigned char *me = (unsigned char *)D_me + idx * 0x1190;
    int i;
    char *base;
    me[0x13] = 0xFF;
    me[0xC] = 0xFF;
    me[0xE] = 0xFF;
    me[0xD] = 0xFF;
    me[0xF] = 0xFF;
    me[0x10] = 0xFF;
    me[0x11] = 0xFF;
    me[0x12] = 0xFF;
    base = D_L07_001DD100;
    for (i = 0; i < 0x24; i++) {
        float *o = (float *)(base + i * 0x1190);
        float *m;
        float dx, dy, ox, oy;
        if (idx != i) {
        ox = o[0];
        oy = o[1];
        m = (float *)(base + idx * 0x1190);
        dx = m[0] - ox;
        dy = m[1] - oy;
        if (func_001F9B88(dx) < 0.1f) {
            if (func_001F9B88(dy - 16.0f) < 0.1f) me[0xC] = i;
            if (func_001F9B88(dy + 16.0f) < 0.1f) me[0xE] = i;
        } else if (func_001F9B88(dy) < 0.1f) {
            if (func_001F9B88(dx - 16.0f) < 0.1f) me[0xD] = i;
            if (func_001F9B88(dx + 16.0f) < 0.1f) me[0xF] = i;
        } else if (func_001F9B88(dx + 16.0f) < 0.1f) {
            if (func_001F9B88(dy - 16.0f) < 0.1f) me[0x11] = i;
            if (func_001F9B88(dy + 16.0f) < 0.1f) me[0x13] = i;
        } else if (func_001F9B88(dx - 16.0f) < 0.1f) {
            if (func_001F9B88(dy - 16.0f) < 0.1f) me[0x10] = i;
            if (func_001F9B88(dy + 16.0f) < 0.1f) me[0x12] = i;
        }
        }
    }
}
