/* NON_MATCHING func_L12_002C0940 -- src/overlays/l12_hoven/vendor_002C0310.c
 * Best so far: SIZE ours 572 / retail 580, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Grid-neighbour scan: for entry a (stride 0x1190, 17 entries at D_L12_001CC1C0) sets bytes 0xC..0x13 to 0xFF, t
 *   Budget spent (p8 is the best, same size 580, BYTES ~140 differ). Control flow and all stores match. Difference
 *   Unblock: find a form where the in-loop entry address is not merged with the pre-loop one, and an ordering that
 */
extern float func_001F9B88(float);
extern char D_L12_001CC1C0[];

/* For each other entry i, record in entry a the index of the neighbour at each grid offset. */
void func_L12_002C0940(int a) {
    char *base = D_L12_001CC1C0;
    char *q = base;
    int i;
    ((char *)(base + (unsigned)a * 0x1190))[0x13] = 0xFF;
    ((char *)(base + (unsigned)a * 0x1190))[0xC] = 0xFF;
    ((char *)(base + (unsigned)a * 0x1190))[0xE] = 0xFF;
    ((char *)(base + (unsigned)a * 0x1190))[0xD] = 0xFF;
    ((char *)(base + (unsigned)a * 0x1190))[0xF] = 0xFF;
    ((char *)(base + (unsigned)a * 0x1190))[0x10] = 0xFF;
    ((char *)(base + (unsigned)a * 0x1190))[0x11] = 0xFF;
    ((char *)(base + (unsigned)a * 0x1190))[0x12] = 0xFF;
    i = 0;
    do {
        if (i != a) {
            char *e = base + a * 0x1190;
            float dx = *(float *)e - *(float *)q;
            float dy = *(float *)(e + 4) - *(float *)(q + 4);
            if (func_001F9B88(dx) < 0.1f) {
                dx = 16.0f;
                if (func_001F9B88(dy - dx) < 0.1f) e[0xC] = i;
                if (func_001F9B88(dy + dx) < 0.1f) e[0xE] = i;
            } else if (func_001F9B88(dy) < 0.1f) {
                dy = 16.0f;
                if (func_001F9B88(dx - dy) < 0.1f) e[0xD] = i;
                if (func_001F9B88(dx + dy) < 0.1f) e[0xF] = i;
            } else {
                float s = 16.0f;
                if (func_001F9B88(dx + s) < 0.1f) {
                    if (func_001F9B88(dy - s) < 0.1f) e[0x11] = i;
                    if (func_001F9B88(dy + s) < 0.1f) e[0x13] = i;
                } else if (func_001F9B88(dx - s) < 0.1f) {
                    if (func_001F9B88(dy - s) < 0.1f) e[0x10] = i;
                    if (func_001F9B88(dy + s) < 0.1f) e[0x12] = i;
                }
            }
        }
        i++;
        q += 0x1190;
    } while (i < 17);
}
