/* NON_MATCHING func_L16_002EB5A0 -- src/overlays/l16_kalebo3/vendor_002E7C70.c
 * Best so far: SIZE ours 584 / retail 596, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   First candidate p0.c adapts the sibling `func_L16_002EA828` bounds code to three table entries. It compiles to
 */
extern int D_L16_001620C8[] MACRO_ADDR;
extern float *D_L16_001620D8[] MACRO_ADDR;
extern float D_L16_001E5660[];
extern float func_001F9D10(void *, void *);
extern void func_001F49B0(void *, void *);
extern void func_L16_002EA1B8(void);

void func_L16_002EB5A0(unsigned char *m) {
    int i, j;
    float maxx, maxy, maxz, minx, miny, minz;
    float p[4];
    float *out;
    float *verts;
    int count;
    switch (m[0x20]) {
    case 0:
        for (i = 0; i < 3; i++) {
            count = D_L16_001620C8[i];
            verts = D_L16_001620D8[i];
            out = D_L16_001E5660 + i * 4;
            maxx = maxy = maxz = -1024.0f;
            minx = miny = minz = 1024.0f;
            for (j = 0; j < count; j++) {
                float x = verts[j * 3 + 0];
                float y = verts[j * 3 + 1];
                float z = verts[j * 3 + 2];
                if (x > maxx) maxx = x;
                if (y > maxy) maxy = y;
                if (z > maxz) maxz = z;
                if (x < minx) minx = x;
                if (y < miny) miny = y;
                if (z < minz) minz = z;
            }
            out[3] = 0.0f;
            out[0] = (maxx + minx) * 0.5f;
            out[1] = (maxy + miny) * 0.5f;
            out[2] = (maxz + minz) * 0.5f;
            for (j = 0; j < D_L16_001620C8[i]; j++) {
                float dist;
                p[0] = D_L16_001620D8[i][j * 3 + 0];
                p[1] = D_L16_001620D8[i][j * 3 + 1];
                p[2] = D_L16_001620D8[i][j * 3 + 2];
                dist = func_001F9D10(p, out);
                if (dist > out[3]) out[3] = dist;
            }
        }
        m[0x30] = 0xFF;
        m[0x20] = 1;
        *(short *)(m + 0x32) = 0xFF;
        break;
    case 1:
        func_001F49B0(func_L16_002EA1B8, m);
        break;
    }
}
