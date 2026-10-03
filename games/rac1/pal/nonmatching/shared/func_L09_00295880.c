/* NON_MATCHING func_L09_00295880 -- src/overlays/shared/partupd_00295880.c
 * Best so far: SIZE ours 456 / retail 464, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Walks a point along a 16-byte-node path by distance t, returns the node index.
 *   After p2 only two moves differ: retail copies idx*16 into $s6 after the first func_001F9BF0 call and
 *   an extra daddu $a1,$sp (second block). Tried off2 copy and w pointer local; no change. Needs reg-alloc reshuff
 */
extern void func_001F9BF0(void *dst, void *a, void *b);
extern float func_001F9CB8(void *a);
extern void func_L00_001FF4B0(void *, void *, float);
extern float func_001F9C78(void *a, void *b);
extern void func_001F9BD8(void *, void *, void *);

// Walks a point along a path of 16-byte nodes by a distance, writing the new position.
int func_L09_00295880(char *obj, int *path, void *out, int idx, int dir, float t)
{
    float v[4];
    float w[4];
    int next;
    float len;
    char *cur;
    int off;
    int off2;

    if (idx == path[0] - 1) {
        next = 0;
        if (dir <= 0) next = path[0] - 2;
    } else if (idx == 0) {
        next = path[0] - 1;
        if (dir > 0) next = 1;
    } else {
        next = idx + dir;
    }
    off = idx * 16;
    cur = (char *)path + (off + 0x10);
    func_001F9BF0(v, (char *)path + (next * 16 + 0x10), cur);
    off2 = off;
    len = func_001F9CB8(v);
    func_L00_001FF4B0(v, v, 1.0f);
    func_001F9BF0(w, obj + 0x10, cur);
    t = t + func_001F9C78(w, v);
    if (len < t) {
        idx = next;
        if (idx == path[0] - 1) {
            next = 0;
            if (dir <= 0) next = path[0] - 2;
        } else if (idx == 0) {
            next = path[0] - 1;
            if (dir > 0) next = 1;
        } else {
            next = idx + dir;
        }
        t -= len;
        off = idx * 16;
        func_001F9BF0(v, (char *)path + (next * 16 + 0x10), (char *)path + (off + 0x10));
        func_L00_001FF4B0(v, v, 1.0f);
        off2 = off;
    }
    func_L00_001FF4B0(v, v, t);
    func_001F9BD8(out, v, (char *)path + (off2 + 0x10));
    return idx;
}
