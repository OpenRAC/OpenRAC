/* NON_MATCHING func_L09_00304EE0 -- src/overlays/l09_gaspar/vendor_002C2B08.c
 * Best so far: BYTES 14/476 (97.1% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Path-following moby update: collision query (func_L00_0025B4D0) switch cases 1/2 zero the timer, timer compare
 *   p1.c: instruction-identical except $s4/$s5 swapped (retail keeps arg3 in s5, arg2 in s4; ours the reverse). Lo
 *   An allocator tie on parameter register priority; unblock would need another use count/ordering for the two par
 */
extern char *func_L00_0025B478(void *, int, int);
extern int func_00120778(float);
extern int func_001E9730();
extern int func_L00_0025B4D0(void *, void *, void *, int, int *, float *, int, int);
extern void func_L00_002584A8(void *, int, int);
extern void func_L00_0025E4B0(void *m, short *p);
extern int func_001F9850(int);
extern void func_L00_0025E590(void *, void *);
extern char D_L09_00209280[];
extern char *D_L09_00160058 MACRO_ADDR;

// Updates a moby that follows a path: handles collision result, timer and state change.
void func_L09_00304EE0(char *moby, char *d, float *t) {
    float *tp = t;
    int n;
    char *m;
    char *o;
    int idx;
    if ((unsigned char)moby[0x20] != 3) {
        m = func_L00_0025B478(moby, 0x330000, 0);
        if (m) {
            func_001E9730(D_L09_00209280, *(short *)(moby + 0xA6), func_00120778(*(float *)(m + 0x2C)));
        }
        switch (func_L00_0025B4D0(moby, m, tp, 0, &n, 0, 0, 4)) {
        case 1:
        case 2:
            *(int *)tp = 0;
            break;
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
        }
        if (n >= 2) {
            if (*tp <= *(float *)(m + 0x2C)) {
                *(int *)tp = 0;
                *(unsigned short *)(moby + 0x34) &= 0xEFFF;
                idx = *(int *)(d + 0x70);
                if (idx != -1) {
                    o = D_L09_00160058 + (idx << 8);
                    if (o && (unsigned char)o[0x20] != 0xFE && (unsigned char)o[0x20] != 0xFD) {
                        if (*(short *)(o + 0xA6) == 0x494 || *(short *)(o + 0xA6) == 0x49D || *(short *)(o + 0xA6) == 0x4A0) {
                            o[0xBC] = 1;
                        }
                    }
                }
                func_L00_002584A8(moby, 0, -1);
                d[0x67] = 0x78;
                func_L00_0025E4B0(moby, (short *)(d + 0x60));
                moby[0x20] = 3;
            } else {
                *tp = *tp - *(float *)(m + 0x2C);
                ((unsigned char *)d)[0x67] = 0xFA;
                *(short *)(d + 0x26) = func_001F9850(0x3C);
                func_L00_0025E4B0(moby, (short *)(d + 0x60));
            }
            ((unsigned char *)moby)[0xA4] = 0xFF;
        } else {
            ((unsigned char *)moby)[0xA4] = 0xFF;
        }
    }
    func_L00_0025E590(moby, d + 0x60);
}
