extern char D_0013E633[];
extern char D_0013A5E0[];
extern int func_L00_00233CC0(void);
extern void func_L00_002180F8(void);
extern float func_L00_00211EE8(void);
extern float func_001F9CE8(void *);
extern void func_001FA190(void *);
extern void func_001FA218(void *, void *);
extern void func_001FA4A0(void *, void *);
extern void func_L00_00221A98(void);
extern void func_L00_00221D50(void);

/* Per-frame hero motion step: fills the stick-derived direction at +0x1D20,
   pushes the two 32-entry rings (+0x21A8/+0x21AC and +0x21B0/+0x21B4) and the
   16-byte block at +0x80 into +0x1B00, then runs the matrix and camera steps. */
void func_L00_00211D28(void) {
    char *s = D_0013E633 + 0xE1D;
    char *p = D_0013A5E0 + 0x2460;
    int b, idx, next, t, cnt, idx2, next2;
    float f;
    int *dst, *src;

    *(unsigned char *)(s + 0x20B3) = (unsigned char)func_L00_00233CC0();
    *(float *)(s + 0x229C) = func_L00_00211EE8();
    *(int *)(s + 0x240) = 0;

    *(float *)(s + 0x1D20) = *(float *)(p + 0x108);
    *(float *)(s + 0x1D24) = *(float *)(p + 0x10C);
    f = func_001F9CE8(s + 0x1D20);
    if (f < 0.25f) {
        b = *(int *)(p + 0x1B0);
        *(float *)(s + 0x1D20) = (float)(((b >> 13) & 1) - ((b >> 15) & 1));
        *(float *)(s + 0x1D24) = (float)(((b >> 14) & 1) - ((b >> 12) & 1));
    }

    func_L00_002180F8();
    idx = *(int *)(s + 0x21A8);
    *(float *)(s + 0x2128 + idx * 4) = *(float *)(s + 0x98);

    idx = *(int *)(s + 0x21A8);
    next = idx + 1;
    t = idx + 0x20;
    if (-1 < next) {
        t = next;
    }
    t = (t >> 5) << 5;
    *(int *)(s + 0x21A8) = next - t;
    cnt = *(int *)(s + 0x21AC) + 1;
    *(int *)(s + 0x21AC) = cnt;
    if (cnt >= 33) {
        *(int *)(s + 0x21AC) = 0x20;
    }

    idx2 = *(int *)(s + 0x21B0);
    dst = (int *)(s + 0x1B00 + idx2 * 16);
    src = (int *)(s + 0x80);
    *(u128_0CDF0 *)dst = *(u128_0CDF0 *)src;
    next2 = idx2 + 1;
    t = idx2 + 0x20;
    if (-1 < next2) {
        t = next2;
    }
    t = (t >> 5) << 5;
    *(int *)(s + 0x21B0) = next2 - t;
    cnt = *(int *)(s + 0x21B4) + 1;
    *(int *)(s + 0x21B4) = cnt;
    if (cnt >= 33) {
        *(int *)(s + 0x21B4) = 0x20;
    }

    func_001FA218(s, s + 0x90);
    func_001FA190(s + 0x40);
    func_001FA4A0(s + 0x40, s);
    func_L00_00211A38();
    func_L00_00221A98();
    func_L00_00221D50();
}
