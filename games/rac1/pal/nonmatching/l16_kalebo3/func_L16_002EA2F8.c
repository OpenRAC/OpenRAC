/* NON_MATCHING func_L16_002EA2F8 -- src/overlays/l16_kalebo3/vendor_002E7C70.c
 * Best so far: SIZE ours 592 / retail 584, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * Cannot land as written (#define in a candidate): rewrite that in plain C first.
 * What the last attempts found:
 *   UpdateMoby_1943: state 0 builds a bounding sphere (centre = (max+min)*0.5, radius = max func_001F9D10 distance
 *   Best p4.c: size 584 matches, BYTES 227/584. Differences: retail's i lives in $a0 (ours $a1) and the loop-carri
 *   Would unblock: the source form that leaves two un-CSE'd address expressions for the same cell, and a variable 
 */
extern char _gp;
#define CNT ((int *)(&_gp - 0x4D50))
#define PTS ((float **)(&_gp - 0x4D40))
extern float func_001F9D10(void *, void *);
extern void func_001F49B0(void *, void *);
extern void func_L16_002EA1B8(void);

/* Builds a bounding sphere (centre and radius) for each of three point sets, then queues the draw callback. */
void func_L16_002EA2F8(unsigned char *moby) {
    int i;
    int j;
    switch (moby[0x20]) {
    case 0:
        for (i = 0; i < 3; i++) {
            float maxx = -1024.0f, maxy = -1024.0f, maxz = -1024.0f;
            float minx = 1024.0f, miny = 1024.0f, minz = 1024.0f;
            float *s = &D_L16_001DC8D0[i * 4];
            for (j = 0; j < CNT[i]; j++) {
                float *p = PTS[i] + j * 3;
                if (maxx < p[0]) maxx = p[0];
                if (maxy < p[1]) maxy = p[1];
                if (maxz < p[2]) maxz = p[2];
                if (p[0] < minx) minx = p[0];
                if (p[1] < miny) miny = p[1];
                if (p[2] < minz) minz = p[2];
            }
            s[0] = (maxx + minx) * 0.5f;
            s[1] = (maxy + miny) * 0.5f;
            s[2] = (maxz + minz) * 0.5f;
            s[3] = 0.0f;
            s = &D_L16_001DC8D0[i * 4];
            for (j = 0; j < CNT[i]; j++) {
                float v[3];
                float d;
                float *p = PTS[i] + j * 3;
                v[0] = p[0];
                v[1] = p[1];
                v[2] = p[2];
                d = func_001F9D10(v, s);
                if (s[3] < d) s[3] = d;
            }
        }
        moby[0x20] = 1;
        break;
    case 1:
        func_001F49B0((void *)func_L16_002EA1B8, moby);
        break;
    }
}
