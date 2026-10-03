/* NON_MATCHING func_L01_002F5AE8 -- src/overlays/l01_novalis/vendor_002BA898.c
 * Best so far: BYTES 12/588 (98.0% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   CrankSliderUpdate: state 0 inits, state 1 slides the moby between two points by the linked crank moby's angle 
 *   p3.c is 12/588 bytes off (6 instructions): only register choice at the D_L01_00160058 table lookup: retail hol
 *   Needs a wording that changes the allocation order of the index and base temporaries (idx local did not).
 */
extern int func_L00_0028EB98(void *, int);
extern void func_L00_0028EBF0(int);
extern void func_0022ED80(int, int, int);
extern int func_0022ED80_2FA458(int, int, void *) __asm__("func_0022ED80");
extern char *D_L01_00160058_m __asm__("D_L01_00160058");
extern char D_0013E633[];

// Crank slider update: slides the moby between two points by the linked crank's angle and manages its sound handle.
void func_L01_002F5AE8(char *m) {
    char *d = *(char **)(m + 0x78);
    int idx;
    switch (*(unsigned char *)(m + 0x20)) {
    case 0:
        *(float *)(d + 4) = *(float *)(m + 0x10);
        *(float *)(d + 8) = *(float *)(m + 0x14);
        *(int *)(d + 0x14) = -1;
        *(int *)(d + 0x18) = 0;
        *(unsigned char *)(m + 0x20) = 1;
        break;
    case 1:
        if ((idx = *(int *)d) != -1 && *(float *)(d + 0xC) != 0.0f && *(float *)(d + 0x10) != 0.0f) {
            char *o = D_L01_00160058_m + (idx << 8);
            if (*(short *)(o + 0xA6) == 0x118) {
                char *e = *(char **)(o + 0x78);
                *(float *)(m + 0x10) = *(float *)(d + 4) + (*(float *)(d + 0xC) - *(float *)(d + 4)) * *(float *)e;
                *(float *)(m + 0x14) = *(float *)(d + 8) + (*(float *)(d + 0x10) - *(float *)(d + 8)) * *(float *)e;
                if (*(float *)e != *(float *)(d + 0x18)) {
                    if (*(float *)e == 1.0f || *(float *)e == 0.0f) {
                        if (func_L00_0028EB98(m, *(int *)(d + 0x14)) != 0) {
                            int t = *(int *)(d + 0x14);
                            if (t != -1) {
                                char *p = D_0013E633 + 0x1D + t * 0x70;
                                if (*(char **)(p + 0x88) == m && *(unsigned char *)(p + 0x74) != 0) {
                                    func_L00_0028EBF0(t);
                                }
                            }
                            *(int *)(d + 0x14) = -1;
                        }
                        func_0022ED80(1, 0, (int)m);
                    } else {
                        if (func_L00_0028EB98(m, *(int *)(d + 0x14)) == 0) {
                            *(int *)(d + 0x14) = func_0022ED80_2FA458(0, 4, m);
                        }
                    }
                } else {
                    if (func_L00_0028EB98(m, *(int *)(d + 0x14)) != 0) {
                        int t = *(int *)(d + 0x14);
                        if (t != -1) {
                            char *p = D_0013E633 + 0x1D + t * 0x70;
                            if (*(char **)(p + 0x88) == m && *(unsigned char *)(p + 0x74) != 0) {
                                func_L00_0028EBF0(t);
                            }
                        }
                        *(int *)(d + 0x14) = -1;
                    }
                }
                *(float *)(d + 0x18) = *(float *)e;
            }
        }
        break;
    }
}
