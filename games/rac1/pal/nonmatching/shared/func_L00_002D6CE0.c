/* NON_MATCHING func_L00_002D6CE0 -- src/overlays/shared/vendor_002D1168.c
 * Best so far: BYTES 7/340 (97.9% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002D6CE0: UpdateMoby state-1 handler: writes moby[0xB0]+2 into two per-slot tables (D_L00_001BA960, D
 *   Best candidate p6.c: 7 of 340 bytes differ, all one register swap: retail keeps the 0xFF constant in $a2 and t
 *   Would need a source shape that gives the byte local lower allocation priority than the 0xFF constant; tried in
 */
extern int func_0022ED80(int, int, int);
extern char D_L00_001BA960[];
extern char D_L00_001BB5C0[];
extern unsigned char D_L00_0015FD48[];
extern char *D_L00_001B0830[];
extern short D_0015EE84;
extern char D_0014171B[];

// Marks a newly activated moby's slot in two tables, then resets the matching entries whose float field equals the moby's value.
void func_L00_002D6CE0(char *m) {
    char *d = *(char **)(m + 0x78);
    int i;
    if (*(unsigned char *)(m + 0x20) == 1) {
        int b;
        int s;
        int s2;
        char *t;
        unsigned char c;
        s = *(short *)(m + 0xB2);
        t = D_L00_001BA960 - (-s);
        t[0x454] = *(unsigned char *)(m + 0xB0) + 2;
        b = *(unsigned char *)(m + 0xB0);
        c = b;
        if (!(b != 0xFF && (D_L00_0015FD48[c] == 0xFF || *(unsigned char *)(D_0014171B + 0xAA35 + (c + (*(int *)&D_0015EE84 << 4))) != 0xFF))) {
            s2 = *(short *)(m + 0xB2);
            t = D_L00_001BB5C0 - (-s2);
            t[0x454] = b + 2;
        }
        *(unsigned char *)(m + 0x20) = 2;
        *(unsigned char *)(m + 0xBC) = 1;
        *(int *)(m + 0x90) = 0x80208020;
        func_0022ED80(0, 0, (int)m);
        if (*(int *)d != -1) {
            for (i = 0; i < *(int *)D_L00_001B0830[*(int *)d]; i++) {
                char *e = D_L00_001B0830[*(int *)d] + i * 16;
                if (*(float *)(e + 0x1C) == *(float *)(d + 4)) *(int *)(e + 0x1C) = 0;
            }
        }
    }
}
