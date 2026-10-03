/* NON_MATCHING func_L09_00305378 -- src/overlays/l09_gaspar/vendor_002C2B08.c
 * Best so far: BYTES 19/516 (96.3% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Damage handler for a moby (sibling of func_L11_00311318): query hit via func_L00_0025B4D0, jump-table switch (
 *   Best p1.c: every instruction right (516 bytes) except moby and the hit pointer p have their saved registers sw
 *   Would need a shape that changes the allocator's priority between moby and p.
 */
extern char D_L09_002092E0[];
extern short D_L09_00160058;
extern char *func_L00_0025B478(void *, int, int);
extern int func_00120778(float);
extern int func_001E9730();
extern int func_L00_0025B4D0(void *, void *, void *, int, void *, void *, int, int);
extern void func_L00_002584A8(void *, int, int);
extern int func_001F9850(int);

// Handles a ((unsigned char *)m0) taking damage: drains or resets a health value and switches state.
void func_L09_00305378(void *m0, void *m1, void *m2) {
    int cnt;
    unsigned char *a1 = m1;
    float *a2 = m2;
    char *p;
    unsigned char *e;
    if (((unsigned char *)m0)[0x20] != 3) {
        p = func_L00_0025B478(((unsigned char *)m0), 0x330000, 0);
        if (p != 0) {
            int t = *(short *)((unsigned char *)m0 + 0xA6);
            func_001E9730(D_L09_002092E0, t, func_00120778(*(float *)(p + 0x2C)));
        }
        switch (func_L00_0025B4D0(((unsigned char *)m0), p, a2, 0, &cnt, 0, 0, 4)) {
        case 0:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
            break;
        case 1:
        case 2:
            *a2 = 0.0f;
            break;
        }
        if (cnt >= 2) {
            if (*a2 <= *(float *)(p + 0x2C)) {
                *a2 = 0.0f;
                *(unsigned short *)((unsigned char *)m0 + 0x34) &= 0xEFFF;
                if (*(int *)(a1 + 0x60) != -1) {
                    e = *(unsigned char **)&D_L09_00160058 + (*(int *)(a1 + 0x60) << 8);
                    if (e != 0 && e[0x20] != 0xFE && e[0x20] != 0xFD) {
                        int s = *(short *)(e + 0xA6);
                        if (s == 0x494 || s == 0x49D || s == 0x4A0) e[0xBC] = 1;
                    }
                }
                if (*(int *)(a1 + 0x64) != -1) {
                    e = *(unsigned char **)&D_L09_00160058 + (*(int *)(a1 + 0x60) << 8);
                    if (e != 0 && e[0x20] != 0xFE && e[0x20] != 0xFD) {
                        int s = *(short *)(e + 0xA6);
                        if (s == 0x494 || s == 0x49D || s == 0x4A0) e[0xBC] = 1;
                    }
                }
                func_L00_002584A8(((unsigned char *)m0), 0, -1);
                ((unsigned char *)m0)[0x20] = 3;
            } else {
                *a2 = *a2 - *(float *)(p + 0x2C);
                *(short *)(a1 + 0x26) = func_001F9850(0x3C);
            }
            ((unsigned char *)m0)[0xA4] = 0xFF;
        } else {
            ((unsigned char *)m0)[0xA4] = 0xFF;
        }
    }
}
