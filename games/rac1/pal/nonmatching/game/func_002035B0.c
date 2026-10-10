/*
 * BuildMobyAdGif: writes the four 64-bit words of one moby ad GIF packet at out.
 * rec holds signed 16-bit fields at +0x4 (the first two lookups via
 * func_001F9968), +0x6, +0x8, +0xA, +0xC and +0xE, offset by the global
 * D_0015EF8C >> 8. The words are at out+0x00, +0x10, +0x20 and +0x30; with
 * a6 == -1 the last two are 0x9980-based and 0, with a6 < -1 they come from
 * the 16-byte table D_0019E7C0 (or D_0019E7D8 for a6 == -3). Otherwise
 * the words hold a6 << 24 and a5 << 2 and a4 (low word).
 */
extern int D_0015EF8C NOT_SDA;
extern int func_001F9968(int);
extern long D_0019E7C0[];
extern long D_0019E7D8[];

void func_002035B0(unsigned char *out, unsigned char *rec, int a2, int a3, int a4, int a5, int a6) {
    int u;
    int t;
    int W;
    int H;
    int F4;
    int F6;
    int K;
    int c8;
    int c11;
    int c12;
    int s8;
    u64 d0;
    u64 d1;
    u64 d2;
    u64 d3;
    u64 x;
    long *tp;

    u = *(unsigned short *)(rec + 4);
    t = (int)((unsigned int)u << 16);
    W = t >> 23;
    H = t >> 22;
    if (!(0 < W)) {
        W = 1;
    }
    F4 = func_001F9968(t >> 16);
    if (!(0 < H)) {
        H = 1;
    }
    F6 = func_001F9968(*(short *)(rec + 6));

    K = D_0015EF8C >> 8;
    c12 = *(short *)(rec + 0xE) + K;
    c11 = *(short *)(rec + 0xA) + K;
    c8 = *(short *)(rec + 0xC) + K;
    s8 = *(short *)(rec + 8);

    if (a6 >= 0) {
        d0 = ((u64)(long)a2 << 32)
           | ((u64)(long)(s8 - 1) << 2)
           | (((u64)(long)a3 << 6) | 0x20UL);
        d1 = (u64)(long)a4
           | ((u64)(long)a5 << 2)
           | ((u64)(long)a6 << 24);
        d2 = ((u64)(long)H << 14)
           | ((u64)(long)F4 << 26)
           | 0x01300000UL
           | ((u64)(long)F6 << 30)
           | ((u64)(long)c11 << 37)
           | (1UL << 34)
           | (1UL << 63);
        d3 = ((u64)(long)W << 14)
           | ((u64)(long)c8 << 20)
           | ((u64)(long)c12 << 40)
           | (1UL << 34)
           | (1UL << 54);
    } else if (a6 == -1) {
        d0 = ((u64)(long)a2 << 32)
           | ((u64)(long)a3 << 6)
           | 0x20UL;
        d1 = 5UL;
        x = (1UL << 44) | 0x9980UL;
        x = (x << 19) | 0x7FFBUL;
        d2 = x;
        d3 = 0UL;
    } else {
        tp = (a6 == -3) ? D_0019E7D8 : D_0019E7C0;
        d0 = ((u64)(long)a2 << 32)
           | ((u64)(long)a3 << 6)
           | 0x20UL;
        d1 = 5UL;
        d2 = (u64)tp[0];
        d3 = (u64)tp[2];
    }

    *(u64 *)(out + 0x00) = d0;
    *(u64 *)(out + 0x10) = d1;
    *(u64 *)(out + 0x20) = d2;
    *(u64 *)(out + 0x30) = d3;
}
