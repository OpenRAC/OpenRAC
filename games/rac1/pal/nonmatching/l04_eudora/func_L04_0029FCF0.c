/* NON_MATCHING func_L04_0029FCF0 -- src/overlays/l04_eudora/vendor_0029FCF0.c
 * Best so far: SIZE ours 460 / retail 468, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Advances a point along a closed polyline (list of 16-byte vertices after a count) by distance f, wrapping inde
 */
extern void func_001F9BF0(void *dst, void *a, void *b);
extern float func_001F9CB8(void *a);
extern void func_L00_001FF4B0(void *, void *, float);
extern float func_001F9C78(void *a, void *b);
extern void func_001F9BD8(void *, void *, void *);

// Advances a point along a closed polyline by a distance, returning the segment index it ends on.
int func_L04_0029FCF0(char *out, char *list, int idx, int dir, float f) {
    float A[4];
    float B[4];
    int n = *(int *)list;
    int k;
    char *p;
    float len, d;
    int neg = dir < 1;
    if (idx == n - 1) {
        k = 0;
        if (dir <= 0) k = n - 2;
    } else if (idx == 0) {
        k = 1;
        if (neg) k = n - 1;
    } else {
        k = idx + dir;
    }
    p = list + idx * 16 + 0x10;
    func_001F9BF0(A, list + k * 16 + 0x10, p);
    out += 0x10;
    len = func_001F9CB8(A);
    func_L00_001FF4B0(A, A, 1.0f);
    func_001F9BF0(B, out, p);
    d = func_001F9C78(B, A) + f;
    if (len < d) {
        idx = k;
        n = *(int *)list;
        if (idx == n - 1) {
            k = 0;
            if (dir <= 0) k = n - 2;
        } else if (idx == 0) {
            k = 1;
            if (neg) k = n - 1;
        } else {
            k = idx + dir;
        }
        func_001F9BF0(A, list + k * 16 + 0x10, list + idx * 16 + 0x10);
        d -= len;
        func_L00_001FF4B0(A, A, 1.0f);
    }
    func_L00_001FF4B0(A, A, d);
    func_001F9BD8(out, A, list + idx * 16 + 0x10);
    return idx;
}
