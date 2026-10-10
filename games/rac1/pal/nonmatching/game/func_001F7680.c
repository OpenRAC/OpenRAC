extern s32 D_0015EF84 MACRO_ADDR;
extern s32 D_0013E604[];
extern void func_00234E80(void);

/* func_001F7680(src): writes the packet chain for the blocks from src on. The
   block count is D_0013E604[0]; each pass takes up to 128 blocks (c), writes
   a GIF tag set (registers 0x50-0x53, with x = D_0015EF84 >> 8 in the first
   register set, +8 per block) and a DMA reference tag of c * 128 quadwords,
   then moves src on by c * 2048 bytes. D_00161000 is the packet write
   pointer; it ends 0x80 bytes past each pass's start. Then func_00234E80. */
void func_001F7680(int a0) {
    unsigned int src = (unsigned int)a0;
    unsigned int x = (unsigned int)(D_0015EF84 >> 8);
    int rem = D_0013E604[0];

    do {
        int c = (rem >= 0 && rem <= 0x80) ? rem : 0x80;
        unsigned int n = (unsigned int)c;
        unsigned int *p = (unsigned int *)D_00161000;

        p[0] = 0x10000006;
        p[1] = 0;
        p[2] = 0;
        p[3] = 0x50000006;
        *(u64 *)(p + 4) = 0x4000000000000001UL;
        *(u64 *)(p + 6) = 0x0EEEEEEEUL;
        *(u64 *)(p + 8) = ((u64)x << 32) | (1UL << 51);
        *(u64 *)(p + 10) = 0x50;
        *(u64 *)(p + 12) = 0;
        *(u64 *)(p + 14) = 0x51;
        *(u64 *)(p + 16) = ((u64)n << 32) | 0x200;
        *(u64 *)(p + 18) = 0x52;
        *(u64 *)(p + 20) = 0;
        *(u64 *)(p + 22) = 0x53;
        *(u64 *)(p + 24) = ((u64)(n << 7)) | (1UL << 59) | 0x8000;
        *(u64 *)(p + 26) = 0;
        p[28] = 0x30000000 | (n << 7);
        p[29] = src;
        p[30] = 0;
        p[31] = 0x50000000 | (n << 7);
        D_00161000 = (int *)(p + 32);

        src += n << 11;
        x += n * 8;
        rem -= 0x80;
    } while (rem > 0);

    func_00234E80();
}
