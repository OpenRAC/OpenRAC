/* NON_MATCHING func_L01_002B9E68 -- src/overlays/shared/vendor_002B90A8.c
 * Best so far: BYTES 11/740 (98.5% of the bytes match), checked 2026-10-07.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   mini16 a02: clips four grid-cell corners and selects draw mode. Best p6.c or p7.c BYTES 11/740, both full clip
 *   Stopped budget 8/8. Only seven homogeneous/depth-coordinate stores differ in order at +a4..+140; reverse chain
 */
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9EE8(void *, void *, void *);
extern int func_001F9B20(void *);
extern void func_L01_00263338(int, int);
extern float D_L01_001CAF80[];
extern char D_L01_001672C0[];
typedef struct { float corners[4][4]; float local[4], projected[4]; float offsets[4][4]; } Bounds01;

/* clips the four corners of a grid cell before submitting its draw mode */
void func_L01_002B9E68(float *pos, int cell, int mode) {
    Bounds01 box;
    int i, clip, depth;
    if (mode) {
        float width = D_L01_001CAF80[2];
        float height = D_L01_001CAF80[3];
        float dx = -width * 0.5f;
        float dy = -height * 0.5f;
        float x0 = dx * (float)(cell & 3);
        float y0 = dy * (float)(cell >> 2);
        float x1 = x0 + dx;
        float y1 = y0 + dy;
        float x = pos[0] + width;
        float y = pos[1] + height;
        box.offsets[0][0] = x0; box.offsets[0][1] = y0;
        box.offsets[1][0] = x1; box.offsets[1][1] = y0;
        box.offsets[2][0] = x0; box.offsets[2][1] = y1;
        box.offsets[3][0] = x1; box.offsets[3][1] = y1;
        box.corners[0][0] = x + x0; box.corners[0][1] = y + y0;
        box.corners[1][0] = x + x1; box.corners[1][1] = y + y0;
        box.corners[2][0] = x + x0; box.corners[2][1] = y + y1;
        box.corners[3][0] = x + x1; box.corners[3][1] = y + y1;
        box.corners[3][3] = box.corners[2][3] = box.corners[1][3] = box.corners[0][3] = 1.0f;
        box.corners[3][2] = box.corners[2][2] = box.corners[1][2] = box.corners[0][2] = pos[2];
        depth = 0;
        clip = 0;
        for (i = 0; i < 4; i++) {
            func_001F9BF0(box.local, box.corners[i], D_L01_001672C0);
            func_001F9C30(box.projected, box.local, 1024.0f);
            func_001F9C30(box.local, box.local, 1024.0f);
            func_001F9EE8(box.projected, box.projected, D_L01_001672C0 - 0x80);
            func_001F9EE8(box.local, box.local, D_L01_001672C0 - 0x40);
            clip = (clip << 8) | func_001F9B20(box.projected);
            depth = (depth << 8) | func_001F9B20(box.local);
        }
        if (mode == 1) {
            if ((depth & 0x01010101) == 0x01010101 || (depth & 0x02020202) == 0x02020202 ||
                (depth & 0x04040404) == 0x04040404 || (depth & 0x08080808) == 0x08080808 ||
                (depth & 0x20202020) == 0x20202020) return;
            if (clip == 0) mode = 0;
        } else {
            if ((depth & 0x01010101) == 0x01010101 || (depth & 0x02020202) == 0x02020202 ||
                (depth & 0x04040404) == 0x04040404 || (depth & 0x08080808) == 0x08080808 ||
                (depth & 0x20202020) == 0x20202020) return;
            mode = 0;
        }
    }
    {
        int offset = (cell & 3) * 4 + (cell >> 2) * 0x44;
        if (mode) mode = 14;
        else mode = 12;
        func_L01_00263338(mode, offset);
    }
}
