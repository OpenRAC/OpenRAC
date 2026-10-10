/* NON_MATCHING func_L06_002F3D40 -- src/overlays/shared/vendor_002D9548.c
 * Best so far: SIZE ours 784 / retail 788, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern char *D_L06_001AC500[];
extern int func_L00_0025A208(int *, int, int, int);
extern int func_L00_0025A2F0(int *, int, int, int);

/* Picks the nearest grind/target point for m within 12 units at about its height (classes 0x3B1,
 * 0x400, 0x515, 0x516; inactive 0x400/0x516 and unlinked 0x515 skipped), from its sector's moby list or
 * the global list, into d->174, and marks the search done. */
void func_L06_002F3D40(void *mv) {
    char *m = mv;
    int it[4];
    char *d = *(char **)(m + 0x78);
    float best = 12.0f;
    *(int *)(d + 0x174) = 0;
    if (*(int *)(d + 0x30C) != -1) {
        func_L00_0025A208(it, *(int *)(d + 0x30C), 0, 0);
        while (it[0] != 0) {
            short c = *(short *)(it[0] + 0xA6);
            if (c == 0x3B1 || c == 0x400 || c == 0x515 || c == 0x516) {
                char *pos = m + 0x10;
                if (!(best < func_001F9D48(pos, (char *)it[0] + 0x10))
                    && !(0.2f < func_001F9B88(*(float *)(m + 0x18) - *(float *)(it[0] + 0x18)))
                    && !(1.5f < func_001F9B88(*(float *)(m + 0x18) - *(float *)(it[0] + 0x18)))) {
                    if ((*(short *)(it[0] + 0xA6) == 0x400 || *(short *)(it[0] + 0xA6) == 0x516)
                        && ((unsigned char *)it[0])[0x20] == 2) {
                    } else if (*(short *)(it[0] + 0xA6) == 0x515 && **(int **)(it[0] + 0x78) == -1) {
                    } else {
                        best = func_001F9D48(pos, (char *)it[0] + 0x10);
                        *(int *)(d + 0x174) = it[0];
                    }
                }
            }
            func_L00_0025A2F0(it, it[0], 0, 0);
        }
    } else {
        char **lp;
        char *o;
        for (lp = D_L06_001AC500; (o = *lp) != 0; lp++) {
            short c = *(short *)(o + 0xA6);
            if (c == 0x3B1 || c == 0x400 || c == 0x515 || c == 0x516) {
                char *pos = m + 0x10;
                char *op = o + 0x10;
                if (!(best < func_001F9D48(pos, op))
                    && !(0.2f < func_001F9B88(*(float *)(m + 0x18) - *(float *)(o + 0x18)))
                    && !(1.5f < func_001F9B88(*(float *)(m + 0x18) - *(float *)(o + 0x18)))) {
                    if ((*(short *)(o + 0xA6) == 0x400 || *(short *)(o + 0xA6) == 0x516) && ((unsigned char *)o)[0x20] == 2) {
                    } else if (*(short *)(o + 0xA6) == 0x515 && **(int **)(o + 0x78) == -1) {
                    } else {
                        best = func_001F9D48(pos, op);
                        *(char **)(d + 0x174) = o;
                    }
                }
            }
        }
    }
    *(short *)(d + 0x19A) = 1;
}
