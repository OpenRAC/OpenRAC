/* NON_MATCHING func_L16_002EAD58 -- src/overlays/l16_kalebo3/vendor_002E7C70.c
 * Best so far: SIZE ours 608 / retail 596, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L16_002EAD58: UpdateMoby_1948. State 0 builds a bounding sphere (center + max radius via func_001F9D10) f
 *   Best is p4.c (584 bytes vs retail 596, never reached equal size): control flow, cnt/ptr gp-rel handling and th
 *   Every wording that keeps them apart (struct array, float-indexed, char-cast, separate pointers) came out large
 */
extern float func_001F9D10(void *, void *);
extern void func_001F49B0(void *, void *);
extern void func_L16_002EAC18(void);
extern short D_L16_00162050;
extern short D_L16_00162058;
/* Builds a bounding sphere for each of two point sets on first update, then draws. */
void func_L16_002EAD58(char *m) {
    int i, j;
    float v[4];
    switch (*(unsigned char *)(m + 0x20)) {
    case 0:
        for (i = 0; i < 2; i++) {
            float maxx = -1024.0f, maxy = -1024.0f, maxz = -1024.0f;
            float minx = 1024.0f, miny = 1024.0f, minz = 1024.0f;
            float *box;
            for (j = 0; j < ((int *)&D_L16_00162050)[i]; j++) {
                float *p = (float *)(((char **)&D_L16_00162058)[i] + j * 12);
                if (maxx < p[0]) maxx = p[0];
                if (maxy < p[1]) maxy = p[1];
                if (maxz < p[2]) maxz = p[2];
                if (p[0] < minx) minx = p[0];
                if (p[1] < miny) miny = p[1];
                if (p[2] < minz) minz = p[2];
            }
            *(float *)(D_L16_001DF580 + i * 16) = (maxx + minx) * 0.5f;
            *(float *)(D_L16_001DF580 + i * 16 + 4) = (maxy + miny) * 0.5f;
            *(float *)(D_L16_001DF580 + i * 16 + 8) = (maxz + minz) * 0.5f;
            *(float *)(D_L16_001DF580 + i * 16 + 12) = 0.0f;
            box = (float *)(D_L16_001DF580 + i * 16);
            for (j = 0; j < ((int *)&D_L16_00162050)[i]; j++) {
                float *p = (float *)(((char **)&D_L16_00162058)[i] + j * 12);
                float r;
                v[0] = p[0];
                v[1] = p[1];
                v[2] = p[2];
                r = func_001F9D10(v, box);
                if (box[3] < r) box[3] = r;
            }
        }
        m[0x30] = 0xFF;
        m[0x20] = 1;
        *(short *)(m + 0x32) = 0xFF;
        break;
    case 1:
        func_001F49B0(func_L16_002EAC18, m);
        break;
    }
}
