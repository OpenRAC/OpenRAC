/* NON_MATCHING func_L09_002EBD30 -- src/overlays/l09_gaspar/vendor_002C2B08.c
 * Best so far: BYTES 6/732 (99.2% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
typedef struct {
    int count;
    int pad[3];
    float e[1][4];
} Path_2ebd30;
extern Path_2ebd30 *D_L09_001B0930_p[] __asm__("D_L09_001B0930");
extern void func_L00_00251328(void *, int, int, int);
extern void func_001F9CA0(void *, void *, void *);

/* spawns a debris moby from the owner and sends it off sideways from its path with a random spread */
char *func_L09_002EBD30(char *owner) {
    float a[4];
    float b[4];
    char *moby = func_0020D348(func_002140B0(4) + 0x107);
    if (moby != 0) {
        char *pos = moby + 0x10;
        char *od = *(char **)(owner + 0x78);
        char *d = *(char **)(moby + 0x78);
        qcopy(pos, owner + 0x10);
        (*(int *)(od + 0x18))++;
        *(char **)(d + 0x50) = owner;
        *(int *)(d + 0x10) = *(int *)(od + 0x10);
        *(int *)(d + 0x48) = 1;
        *(int *)(d + 0x14) = 0;
        moby[0xBC] = 1;
        moby[0x31] = 1;
        ((unsigned char *)moby)[0x30] = 0xFF;
        *(short *)(moby + 0x32) = 0xFF;
        func_L00_00251328(moby, 0x80, 0x80, 0x80);
        moby[0x23] = 0;
        *(float *)(moby + 0x2C) *= func_002140F8(0.4f, 0.85f);
        *(float *)(d + 0x40) = func_002140F8(-0.0052359877f, 0.0052359877f);
        *(float *)(d + 0x44) = func_002140F8(-0.0052359877f, 0.0052359877f);
        if (*(int *)(d + 0x10) >= 0) {
            Path_2ebd30 *p = D_L09_001B0930_p[*(int *)(d + 0x10)];
            float r = func_002140F8(-30.0f, 30.0f);
            func_001F9BF0(a, p->e[p->count - 1], p->e[p->count - 2]);
            func_001F9BF0(b, p->e[1], p->e[0]);
            func_001F9CA0(b, b, a);
            b[2] = 0.0f;
            func_L00_001FF4B0(d, b, r);
            func_001F9BF0(a, p->e[p->count - 1], p->e[0]);
            func_001F9CA0(b, b, a);
            func_L00_001FF4B0(b, b, func_002140F8(0.0f, 5.0f));
            func_001F9BD8(d, d, b);
            qcopy(d + 0x20, p->e[0]);
            qcopy(d + 0x30, od + 0x30);
            func_001F9BD8(pos, p->e[0], d);
            func_001F9BD8(pos, pos, D_L09_00166FC0);
            func_001F9BF0(pos, pos, d + 0x30);
            {
                float g = D_0015EE6C;
                *(float *)(d + 0xC) = r / 20.0f + 10.0f;
                *(float *)(moby + 0x2C) *= r / 100.0f + 1.0f;
                *(float *)(d + 0xC) *= g;
            }
        }
        func_L00_00251E30(moby);
    }
    return moby;
}
