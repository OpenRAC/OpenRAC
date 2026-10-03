/* NON_MATCHING func_L07_00320CD0 -- src/overlays/l07_umbris/vendor_0031BDB8.c
 * Best so far: BYTES 1/476 (99.8% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_1789: state0 init, state1 positions 3 point pairs. Only diff: addu operand order (idx,g vs g,idx) a
 */
extern void func_L00_00250800(void *, int, void *);
extern void func_001F9BF0(void *dst, void *a, void *b);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_L00_00264BE8(void *, void *, void *, float, float);
extern int D_L07_0015F6A8 MACRO_ADDR;
extern char D_L07_0016C960[];
extern short D_L07_00161D68;
extern short D_L07_00161D70;
extern short D_L07_00161D6C;

/* UpdateMoby_1789: spawn state then position three points. */
void func_L07_00320CD0(unsigned char *moby) {
    switch (moby[0x20]) {
    case 0:
        moby[0x30] = 0xFF;
        moby[0x20] = 1;
        break;
    case 1:
        if (D_L07_0015F6A8 == 2) {
            char *g = D_L07_0016C960;
            int s = *(int *)(g + 0x30);
            if (s == 1 || s == 2) {
                float a0[4], a1[4], a2[4], b0[4], b1[4], b2[4], c0[4], c1[4], c2[4];
                int idx = 0;
                char *o;
                if (s == 1) idx = 3;
                if (s == 2) idx = 4;
                o = *(char **)(g + (0x178 + idx * 4));
                func_L00_00250800(o, 0, a0);
                func_L00_00250800(o, 3, b0);
                func_001F9BF0(c0, a0, b0);
                func_L00_001FF4B0(c0, c0, *(float *)&D_L07_00161D68);
                func_L00_00264BE8(a0, a0, c0, *(float *)&D_L07_00161D6C, *(float *)&D_L07_00161D70);
                func_L00_00250800(o, 1, a1);
                func_L00_00250800(o, 4, b1);
                func_001F9BF0(c1, a1, b1);
                func_L00_001FF4B0(c1, c1, *(float *)&D_L07_00161D68);
                func_L00_00264BE8(a1, a1, c1, *(float *)&D_L07_00161D6C, *(float *)&D_L07_00161D70);
                func_L00_00250800(o, 2, a2);
                func_L00_00250800(o, 5, b2);
                func_001F9BF0(c2, a2, b2);
                func_L00_001FF4B0(c2, c2, *(float *)&D_L07_00161D68);
                func_L00_00264BE8(a2, a2, c2, *(float *)&D_L07_00161D6C, *(float *)&D_L07_00161D70);
            }
        }
        break;
    }
}
