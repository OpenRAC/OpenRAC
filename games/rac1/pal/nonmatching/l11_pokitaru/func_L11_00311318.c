/* NON_MATCHING func_L11_00311318 -- src/overlays/l11_pokitaru/vendor_002CC828.c
 * Best so far: BYTES 14/472 (97.0% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Damage handler for a moby: queries hit result (jump table switch 0..11, cases 1,2 zero the health float), then
 *   Best p4.c: whole body right (472 bytes) but the two saved registers for a1/a2 (s20/s21) are swapped (retail a2
 *   Would need a different use pattern that makes a2 more attractive to the allocator.
 */
extern char D_L11_0021B820[];
extern char *func_L00_0025B478(void *, int, int);
extern int func_00120778(float);
extern int func_001E9730();
extern int func_L00_0025B4D0(void *, void *, void *, int, void *, void *, int, int);
extern void func_L00_0025E4B0(void *m, short *p);
extern void func_00213DE0(void *, int, int, int);
extern int func_001F9850(int);
extern void func_L00_0025E590(void *, void *);

// Handles a moby taking damage: drains or resets a health value and switches animation and state.
void func_L11_00311318(void *m0, void *m1, void *m2) {
    unsigned char *a1 = m1;
    float *a2 = m2;
    int cnt;
    unsigned char *moby = m0;
    char *p;
    unsigned char *child;
    if (moby[0x20] != 4) {
        p = func_L00_0025B478(moby, 0x330000, 0);
        if (p != 0) {
            int t = *(short *)(moby + 0xA6);
            func_001E9730(D_L11_0021B820, t, func_00120778(*(float *)(p + 0x2C)));
        }
        switch (func_L00_0025B4D0(moby, p, a2, 0, &cnt, 0, 0, 4)) {
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
                *(unsigned short *)(moby + 0x34) &= 0xEFFF;
                a1[0x67] = 0x78;
                func_L00_0025E4B0(moby, (short *)(a1 + 0x60));
                if (moby[0x53] != 2) func_00213DE0(moby, 2, 0, 3);
                child = *(unsigned char **)(a1 + 0x70);
                if (child != 0 && child[0x20] != 0xFE && child[0x20] != 0xFD) {
                    child[0x20] = 3;
                }
                moby[0x20] = 3;
            } else {
                *a2 = *a2 - *(float *)(p + 0x2C);
                a1[0x67] = 0xFA;
                *(short *)(a1 + 0x26) = func_001F9850(0x3C);
                func_L00_0025E4B0(moby, (short *)(a1 + 0x60));
                if (moby[0x53] != 4) func_00213DE0(moby, 4, 0, 3);
            }
            moby[0xA4] = 0xFF;
        } else {
            moby[0xA4] = 0xFF;
        }
    }
    func_L00_0025E590(moby, a1 + 0x60);
}
