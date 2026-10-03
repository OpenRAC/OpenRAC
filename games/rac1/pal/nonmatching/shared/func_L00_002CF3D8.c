/* NON_MATCHING func_L00_002CF3D8 -- src/overlays/shared/vendor_002C96D0.c
 * Best so far: BYTES 1/708 (99.9% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Finds the nearest eligible moby (class types 5/7/8, not in a 9-entry exclusion list, within a per-type range) 
 *   Best candidate p2.c (p3.c same bytes): 1 byte differs. The 4D6 arm's `b` lands on the `nop` before the shared 
 *   Keys: no local for m+0x10 (write `m + 0x10` at each use), switch arms each with `if (lim < dist) continue;`. I
 */
extern int D_L00_001600A4 MACRO_ADDR;
extern float func_001F9D10(void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_L00_001EFFF0(void *, void *, int, int, int);

/* finds the nearest eligible moby to a point, skipping a list of nine */
char *func_L00_002CF3D8(char *a, int *excl) {
    char *result = 0;
    char *m;
    float best = 1e9f;
    float v[4];
    for (m = (char *)D_L00_001600A4; m != 0; m = *(char **)(m + 0x28)) {
        char *cls = *(char **)(m + 0x24);
        int t = 0;
        float dist;
        char *h;
        char *x;
        if (cls != 0) t = *(unsigned char *)(cls + 0x46);
        if (m == 0 || *(unsigned char *)(m + 0x20) == 0xFE) continue;
        if (*(unsigned char *)(m + 0x20) == 0xFD) continue;
        if (t != 5 && t != 7 && t != 8) continue;
        if (excl != 0) {
            int found = 0;
            int i;
            for (i = 0; i < 9; i++) {
                if (excl[i] == (int)m) {
                    found = 1;
                    break;
                }
            }
            if (found) continue;
        }
        if (t == 5 && (*(unsigned short *)(m + 0x34) & 0x1000) == 0) continue;
        if (*(unsigned char *)(m + 0x31) == 0) {
            short c = *(short *)(m + 0xA6);
            if (c != 0x350 && c != 0x31) continue;
        }
        dist = func_001F9D10(a, m + 0x10);
        if (*(short *)(m + 0xA6) == 0x4D6) {
            func_L00_001FF4B0(v, *(char **)(m + 0x78), dist);
            func_001F9BD8(v, v, m + 0x10);
            if (dist < func_001F9D10(a, v)) continue;
        } else {
            float lim;
            switch (t) {
            case 5:
                lim = 4.0f;
                break;
            case 7:
                lim = 5.3f;
                break;
            case 8:
                lim = 6.0f;
                break;
            default:
                goto skip;
            }
            if (lim < dist) continue;
        }
    skip:
        if (!(dist < best)) continue;
        h = func_L00_002DCD40(m);
        if (h != 0 && *(short *)(h + 0x68) > 0) continue;
        x = (char *)func_L00_0025D390(m);
        qcopy(v, m + 0x10);
        if (x != 0) v[2] = v[2] + (*(float *)(x + 0x10) + 0.05f);
        if (func_L00_001EFFF0(a, v, 6, (int)m, 0) == 0) {
            best = dist;
            result = m;
        }
    }
    return result;
}
