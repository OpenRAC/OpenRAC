/* Per-frame hero-side update (called from func_L00_002076E8). When the packed word at
   0x22F8 is in use (0x22F4 set), its three low bytes are clamped and written back through
   func_L00_00251328. Then, for the current state, a timer test may set a flag that runs
   func_L00_00251328 with the table values from func_L00_00251358 and func_L00_002078E0.
   Finally, when the table at 0x10B8 selects an entry, func_L00_00207948 runs on the 0x1090 object. */
extern unsigned char D_0013E15A_u[] __asm__("D_0013E15A");
extern void func_L00_00251328(void *obj, int a, int b, int c);
extern void func_L00_00251358(void *obj, int *a, int *b, int *c);

void func_L00_00207CC0(void) {
    char *g = (char *)D_0013F450;
    int c16 = 0;

    if (*(int *)(g + 0x22F4) != 0) {
        int s = *(int *)(g + 0x22F8);
        int b16 = ((s >> 16) & 0xFF) - 7;
        int b17 = ((s >> 8) & 0xFF) - 7;
        int b19 = (s & 0xFF) - 7;

        if (!(-1 < b16)) {
            b16 = 0;
        }
        if (!(-1 < b17)) {
            b17 = 0;
        }
        if (!(-1 < b19)) {
            b19 = 0;
        }
        func_L00_00251328(*(char **)(g + 0x2080), b19, b17, b16);
        *(int *)(g + 0x22F8) = (s & (int)0xFF000000u) | (b16 << 16) | (b17 << 8) | b19;
    }

    {
        int st = *(int *)(g + 0x2084);
        if (st == 0x80 || st == 0x82) {
            /* the call's return value is in $2 when the compare runs */
            int r = func_001F9850(0x1E);
            c16 = (*(int *)(g + 0x198) < r);
        }
    }

    if (D_0015EE84_m == 0xF || D_0015EE84_m == 0x11) {
        if (*(int *)(g + 0x2084) == 0x76) {
            int r = func_001F9850(0x14);
            if (*(int *)(g + 0x198) < r) {
                c16 = 1;
            }
        }
    }

    if (c16 != 0) {
        int t0 = 0;
        int t4 = 0;
        int t8 = 0;
        int x = *(int *)(g + 0x198);
        int t = (-1 < x) ? x : x + 3;
        int rem = x - ((t >> 2) << 2);

        func_L00_00251358(*(char **)(g + 0x2080), &t0, &t4, &t8);
        if (rem < 3) {
            t0 = 0;
            t4 = 0;
            t8 = 0;
        } else {
            t0 = 0x90;
            t4 = 0x90;
            t8 = 0xF0;
        }
        func_L00_00251328(*(char **)(g + 0x2080), t0, t4, t8);
        func_L00_002078E0();
    }

    {
        int idx = *(int *)(g + 0x10B8);
        if (idx >= 0 && D_0013E15A_u[0x4C6 + idx] != 0) {
            void *p = *(void **)(g + 0x1090);
            if (p != 0) {
                func_L00_00207948(p, *(char **)(g + 0x2080));
            }
        }
    }
}
