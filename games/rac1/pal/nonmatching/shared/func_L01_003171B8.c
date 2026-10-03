/* NON_MATCHING func_L01_003171B8 -- src/overlays/shared/vendor_002F7700.c
 * Best so far: SIZE ours 600 / retail 588, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Claimed by a18 after its N=8 was reached; no attempt was made. Still free for another worker (budget untouched
 *   Camera activation (ActivateCamera_3): copies camera pose from D_L01_00167304 object, fills path segment length
 *   Best p3.c (432/588 bytes matching by offset; logic right). Left: ours uses 7 saved regs, retail 6: retail shar
 */
extern float func_001F9D10(void *, void *);
extern int func_L00_0025EFC0(void *, void *, void *, int *, float *, int, float, float, float);
extern void func_001F9BF0(void *dst, void *a, void *b);
extern float func_001F9CB8(void *a);
extern void func_L00_001FF4B0(void *, void *, float);
extern float func_001F9C78(void *a, void *b);
extern void func_L01_00316270(void *);
extern char *D_L01_0015F050 MACRO_ADDR;
extern char *D_L01_00167304 MACRO_ADDR;
extern char *D_L01_001B0C30[];
extern char D_0013E633[];

// Camera activation: copies the current camera pose, precomputes path segment lengths and checks the path progress.
void func_L01_003171B8(char *cam) {
    char *table = D_L01_0015F050;
    char *src = D_L01_00167304;
    char *d = *(char **)(table + (*(short *)(cam + 0x84) << 5) + 0x1C);
    char *path;
    int idx;
    float fl;
    float v00[4];
    float v10[4];
    float v20[4];
    int i;
    int k;
    qcopy(cam + 0x30, src + 0x30);
    qcopy(cam, src);
    qcopy(cam + 0x10, src + 0x10);
    qcopy(cam + 0x20, src + 0x20);
    qcopy(cam + 0x40, cam);
    if (*(int *)(d + 0x20) >= 0) {
        path = D_L01_001B0C30[*(int *)(d + 0x20)];
        if (*(int *)path > 0) {
            i = 0;
            do {
                int j = i + 1;
                *(float *)(path + i * 16 + 0x1C) =
                    func_001F9D10(path + i * 16 + 0x10, path + (j % *(int *)path) * 16 + 0x10);
                i = j;
            } while (i < *(int *)path);
        }
    }
    *(short *)(d + 0x36) = 0;
    if (*(int *)(d + 0x30) == 3) {
        path = D_L01_001B0C30[*(int *)(d + 0x20)];
        idx = 0;
        fl = 0.0f;
        if (func_L00_0025EFC0(path, D_0013E633 + 0xE9D, v00, &idx, &fl, 0, 20.0f, 1.0f, 0.0f)) {
            k = idx + 1;
            if (k >= *(int *)path) {
                k = *(int *)path - 1;
            }
            func_001F9BF0(v10, path + k * 16 + 0x10, path + idx * 16 + 0x10);
            if (func_001F9CB8(v10) != 0.0f && k != idx) {
                func_L00_001FF4B0(v10, v10, 1.0f);
                func_001F9BF0(v20, cam + 0x30, D_0013E633 + 0xE9D);
                if (func_001F9C78(v20, v10) > 0.0f) {
                    *(short *)(d + 0x36) = 1;
                }
            }
        }
    }
    func_L01_00316270(cam);
}
