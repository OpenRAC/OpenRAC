/* NON_MATCHING func_L13_0030B940 -- src/overlays/l13_gemlik/vendor_002EBD00.c
 * Best so far: BYTES 83/580 (85.7% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Marks cell n's 8 neighbour slots (+0xC..+0x13) with the index of each other cell exactly one grid step away (c
 *   Best p2.c: logic and size look right (580 expected). Remaining diff: retail recomputes n*0x1190 (mult) inside 
 *   ours CSEs it with the pre-loop row pointer and hoists it (different register use in the prologue, no mult in l
 */
extern char D_L13_001D9E80[];
extern float func_001F9B88(float);

/* Links grid cell n to its eight neighbours that lie one cell away. */
void func_L13_0030B940(int n) {
    int i;
    unsigned char *row = (unsigned char *)D_L13_001D9E80 + n * 0x1190;
    unsigned char *o;
    unsigned char *e;
    float dx, dz;
    row[0xC] = 0xFF;
    row[0xE] = 0xFF;
    row[0xD] = 0xFF;
    row[0xF] = 0xFF;
    row[0x10] = 0xFF;
    row[0x11] = 0xFF;
    row[0x12] = 0xFF;
    row[0x13] = 0xFF;
    for (i = 0; i < 0x16; i++) {
        if (i != n) {
            e = (unsigned char *)D_L13_001D9E80 + n * 0x1190 + (i - i);
            o = (unsigned char *)D_L13_001D9E80 + i * 0x1190;
            dx = *(float *)e - *(float *)o;
            dz = *(float *)(e + 4) - *(float *)(o + 4);
            if (func_001F9B88(dx) < 0.1f) {
                if (func_001F9B88(dz - 16.0f) < 0.1f) e[0xC] = i;
                if (func_001F9B88(dz + 16.0f) < 0.1f) e[0xE] = i;
            } else if (func_001F9B88(dz) < 0.1f) {
                if (func_001F9B88(dx - 16.0f) < 0.1f) e[0xD] = i;
                if (func_001F9B88(dx + 16.0f) < 0.1f) e[0xF] = i;
            } else if (func_001F9B88(dx + 16.0f) < 0.1f) {
                if (func_001F9B88(dz - 16.0f) < 0.1f) e[0x11] = i;
                if (func_001F9B88(dz + 16.0f) < 0.1f) e[0x13] = i;
            } else if (func_001F9B88(dx - 16.0f) < 0.1f) {
                if (func_001F9B88(dz - 16.0f) < 0.1f) e[0x10] = i;
                if (func_001F9B88(dz + 16.0f) < 0.1f) e[0x12] = i;
            }
        }
    }
}
