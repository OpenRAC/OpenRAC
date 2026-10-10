/* NON_MATCHING func_L13_002D0298 -- src/overlays/l13_gemlik/vendor_002C2638.c
 * Best so far: SIZE ours 1320 / retail 1328, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Forcefield tower update (moby class 170): a switch on moby[0x20] (0 = start, 1 = wait/linger, 2 = active), wit
 *   Differences left: the state-1 "v != 0" block gets a movz (`run448` flag) and an extra `addiu $s1,1` where reta
 *   Unblock: a way to get the zero store and the 16-byte copy through `por`/`sq $v0` without a typedef clash (the 
 */
typedef int u128z_2D0298 __attribute__((mode(TI)));
extern int D_0015EE84 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern int D_L13_00184828[];
extern char D_0014171B[];
extern void func_0020D678(void *);
extern void func_L00_00250800(void *, int, void *);
extern char *func_L13_0030D468(char *, void *, float);
extern int func_L00_0028EB98(void *, int);
extern int func_0022ED80_s(int, int, int) __asm__("func_0022ED80");
extern int func_L13_002994D0(int, const void *);
extern int func_002140B0(int);
extern int func_001160D8(void);
extern float func_002140F8(float, float);
extern int func_001F9850(int);
extern void func_L00_0026AA10_C4748(float *, unsigned char, unsigned char, unsigned char, float, float, int) __asm__("func_L00_0026AA10");
extern int func_00215B18(char *, float);
extern void func_L13_002CFF90(char *, char *, float *);
extern void func_L00_0025F4A8_s(void *, void *, void *, float, float, int, int, int, float, float, float, float, int, float, int, int, int, int) __asm__("func_L00_0025F4A8");
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);

/* Forcefield tower update (moby class 170): state machine over its pieces. */
void func_L13_002D0298(char *moby) {
    char *d = *(char **)(moby + 0x78);
    float vec[4];
    float blk[4];
    float acc[4];
    int state = *(unsigned char *)(moby + 0x20);

    switch (state) {
    case 0: {
        int a = *(unsigned char *)(moby + 0x30);
        int b = *(short *)(moby + 0x32);
        unsigned short h;
        int w, rnd;
        *(unsigned char *)(moby + 0xBC) = 0;
        *(unsigned char *)(moby + 0x20) = 1;
        if (a < b) *(unsigned char *)(moby + 0x30) = *(unsigned char *)(moby + 0x32);
        rnd = D_0015EE84;
        *(int *)(d + 0x70) = -1;
        h = *(unsigned short *)(moby + 0xB2);
        w = *(int *)((char *)D_0014171B + 0xAB75 + ((short)h >> 5) * 4 + (rnd << 8));
        if ((w >> (h & 0x1F)) & 1) {
            if (*(int *)(d + 0x74) != -1) D_L13_00184828[*(int *)(d + 0x74)] = 1;
            func_0020D678(moby);
        } else {
            func_L00_00250800(moby, 0, vec);
            vec[2] = vec[2] + 1.1f;
            func_L13_0030D468(moby, vec, 2.8f);
            *(unsigned short *)(moby + 0x34) &= 0xEFFF;
        }
    break;
    }
    case 1: {
        int t = *(int *)(d + 0x74);
        int v;
        int run448 = 1;
        if (t != -1) D_L13_00184828[t] = 0;
        if (*(int *)(d + 0x70) != -1 && func_L00_0028EB98(moby, *(int *)(d + 0x70))) {
            v = *(unsigned char *)(moby + 0x31);
        } else {
            if (*(int *)(d + 0x70) == -1) {
                int r = func_0022ED80_s(0, 4, (int)moby);
                *(int *)(d + 0x70) = r;
                if (r == -1) run448 = 0;
            }
            if (run448) {
                qzero(vec);
                vec[2] = vec[2] + 8.7f;
                func_L13_002994D0(*(int *)(d + 0x70), vec);
            }
            v = *(unsigned char *)(moby + 0x31);
        }
        if (v != 0 && func_002140B0(9) == 0) {
            int s17 = func_001160D8();
            int s16, s18, e, m;
            float f20;
            s17 = (s17 + 0x30) & 0x3F;
            s16 = func_001160D8();
            s16 = (s16 + 0x20) & 0x3F;
            s18 = func_001160D8() & 0x2F;
            func_L00_00250800(moby, 0, vec);
            vec[2] = vec[2] + 0.5f;
            f20 = func_002140F8(900000.0f, 1800000.0f);
            e = func_001160D8();
            m = func_001F9850(e % 40 + 40);
            func_L00_0026AA10_C4748(vec, s17, s16, s18, f20, 0.0f, m);
        }
        func_L13_002CFF90(moby, d, (float *)(d + 0x20));
    break;
    }
    case 2: {
        float f = D_0015EE6C;
        float s;
        int c;
        int k;
        *(u128z_2D0298 *)blk = 0;
        f = f * 8.0f;
        s = 1000.0f;
        c = *(unsigned char *)(moby + 0xBC);
        blk[2] = f;
        *(u128z_2D0298 *)vec = *(u128z_2D0298 *)blk;
        if (c == 1) s = 17.0f;
        else if (c < 2) {
            if (c == 0) s = 8.5f;
        } else if (c == 2) s = 17.0f;
        if (s < 100.0f) {
            if (func_00215B18(moby, s)) {
                func_L00_00250800(moby, *(unsigned char *)(moby + 0xBC) + 1, blk);
                func_L00_0025F4A8_s(moby, vec, blk, 0.0f, 0.0f, 10, 3, 16, 4.0f, 2.0f, 0.0f, 1.0f, -1, 20.0f, 1, 1, -1, 0);
                *(unsigned char *)(moby + 0xBC) = *(unsigned char *)(moby + 0xBC) + 1;
            }
        }
        if (*(unsigned char *)(moby + 0x70) & 2) {
            func_L00_0025F4A8_s(moby, vec, 0, 0.0f, 0.0f, 10, 3, 16, 4.0f, 2.0f, 0.0f, 1.0f, -1, 20.0f, 1, 1, -1, 0);
            func_L00_00250800(moby, 3, blk);
            func_001F9BF0(blk, blk, moby + 0x10);
            k = 4;
            do {
                float r;
                k--;
                r = func_002140F8(0.0f, 3.0f);
                func_001F9C30(acc, blk, r);
                func_001F9BD8(acc, acc, moby + 0x10);
                func_L13_002CF960(moby, acc, 1);
            } while (k >= 0);
            if (*(int *)(d + 0x74) != -1) D_L13_00184828[*(int *)(d + 0x74)] = 1;
            func_0020D678(moby);
        }
    }
    break;
    }
}
