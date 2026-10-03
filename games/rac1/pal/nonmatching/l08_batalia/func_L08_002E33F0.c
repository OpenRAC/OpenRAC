/* NON_MATCHING func_L08_002E33F0 -- src/overlays/l08_batalia/vendor_002E0258.c
 * Best so far: SIZE ours 476 / retail 472, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Updates 33 attached part mobys (joint matrices), spins three parts, aims one at a point. Best p2.c (476 vs 472
 */
extern char D_0013E633[];
extern void func_L00_00250800(void *, int, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern float func_001FA748(float, float);
extern float func_L00_001FF860(float, float);
extern void func_L00_00251E30(void *);
extern short D_0015EE6C_g __asm__("D_0015EE6C");
extern short D_L08_00161CCC;

// Updates the attached part mobys and the three spinning parts of a moby.
void func_L08_002E33F0(char *moby) {
    float v0[4];
    float v10[4];
    float v20[4];
    char *data = *(char **)(moby + 0x78);
    char *p, *base;
    int i;
    char *q;
    base = data + 0x60;
    p = base + 0x1C;
    i = 0x20;
    do {
        char *m = *(char **)(p - 0xC);
        if (m != 0) {
            *(float *)(m + 0x48) = *(float *)(moby + 0x48);
            func_L00_00250800(m, *(int *)(p - 4), v0);
            func_L00_00250800(*(void **)(p - 8), *(int *)p, v10);
            func_001F9BF0(v20, v10, v0);
            func_001F9BD8(m + 0x10, m + 0x10, v20);
        }
        i--;
        p += 0x10;
    } while (i >= 0);
    if (*(char **)(data + 0xD0) != 0) {
        *(float *)(*(char **)(data + 0xD0) + 0x40) = func_001FA748(*(float *)(*(char **)(data + 0xD0) + 0x40), *(float *)&D_L08_00161CCC * 0.01745329f * *(float *)&D_0015EE6C_g);
    }
    if (*(char **)(data + 0xE0) != 0) {
        *(float *)(*(char **)(data + 0xE0) + 0x40) = func_001FA748(*(float *)(*(char **)(data + 0xE0) + 0x40), -(*(float *)&D_L08_00161CCC * 0.01745329f * *(float *)&D_0015EE6C_g));
    }
    if (*(char **)(data + 0xF0) != 0) {
        *(float *)(*(char **)(data + 0xF0) + 0x40) = func_001FA748(*(float *)(*(char **)(data + 0xF0) + 0x40), *(float *)&D_L08_00161CCC * 0.01745329f * *(float *)&D_0015EE6C_g);
    }
    q = *(char **)(data + 0x90);
    if (q != 0) {
        *(float *)(*(char **)(data + 0x90) + 0x48) = func_L00_001FF860(((float *)(D_0013E633 + 0xE1D))[0x20] - *(float *)(q + 0x10), ((float *)(D_0013E633 + 0xE1D))[0x21] - *(float *)(q + 0x14));
    }
    p = base + 0x10;
    i = 0x20;
    do {
        char *m = *(char **)p;
        if (m != 0) func_L00_00251E30(m);
        i--;
        p += 0x10;
    } while (i >= 0);
}
