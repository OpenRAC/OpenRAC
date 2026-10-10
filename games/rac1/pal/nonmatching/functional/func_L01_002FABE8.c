extern float func_002140F8_2FABE8(float, float) __asm__("func_002140F8");
extern float func_001F9F90_2FABE8(float) __asm__("func_001F9F90");
extern float func_001F9FA8_2FABE8(float) __asm__("func_001F9FA8");
extern void func_001F9BD8_2FABE8(void *, void *, void *) __asm__("func_001F9BD8");
extern void func_L00_001FF4B0_2FABE8(void *, void *, float) __asm__("func_L00_001FF4B0");
extern int func_L00_00258BC8_2FABE8(int, int) __asm__("func_L00_00258BC8");
extern int func_001F9850_2FABE8(int) __asm__("func_001F9850");
extern void func_L00_0026DD70_2FABE8(void *, void *, int, int, float, int) __asm__("func_L00_0026DD70");
extern int func_002140B0_2FABE8(int) __asm__("func_002140B0");
extern void func_L01_002F9908_2FABE8(void *, void *, unsigned int, int, float, float, float, float, int) __asm__("func_L01_002F9908");
extern void func_L00_002584A8_2FABE8(void *, int, int) __asm__("func_L00_002584A8");
extern void func_0020D678_2FABE8(void *) __asm__("func_0020D678");
extern int func_0022ED80_2FABE8(int, int, void *) __asm__("func_0022ED80");
extern void *func_L00_0025B478_FB158(void *, int, int) __asm__("func_L00_0025B478");
extern float D_0015EE6C_2FABE8 __asm__("D_0015EE6C") MACRO_ADDR;
extern char D_L01_00161CE0_2FABE8[] __asm__("D_L01_00161CE0");
typedef int u128x __attribute__((mode(TI)));
struct u64un { unsigned long long v; } __attribute__((packed));


/* func_L01_002FABE8 -- src/overlays/l01_novalis/vendor_002FABE8.c (functional C for the port, not a match)
 * Breakable rock (class 704, Novalis): when hit (func_L00_0025B478, damage > 0) it plays its sound,
 * throws 200 debris puffs (func_L00_0026DD70) and 40 sparks (func_L01_002F9908, colours from
 * D_L01_00161CE0) in random directions biased along its yaw, sets its death bits and deletes itself.
 * From the staged near miss (nonmatching/l01_novalis/func_L01_002FABE8.c); fixed: the yaw-biased
 * component is a random times cos/sin of the yaw (the near miss squared the cos/sin and dropped
 * the random), the hit-slot byte is unsigned, the hit query's symbol.
 * equiv: DIFFERENT by shape only: the spark loop's colour-table load is rotated into the loop's
 * delay slot (one more copy of the +8 load); every call, argument and float operation matches. */
void func_L01_002FABE8(unsigned char *moby) {
    float T[4];
    float A[4];
    float B[4];
    float C[4];
    char *p;
    char *src;
    char *q;
    float f20;
    float r;
    float s;
    float f0;
    int i;
    int k;
    int res;
    int idx;

    p = (char *)func_L00_0025B478_FB158(moby, 0x10000, 0);
    moby[0xA4] = 0xFF;
    if (p == 0 || !(0.0f < *(float *)(p + 0x2C))) {
        return;
    }
    func_0022ED80_2FABE8(0, 0x10, moby);
    src = D_L01_00161CE0_2FABE8;

    i = 0xC7;
    do {
        *(u128x *)A = 0;
        A[0] = func_002140F8_2FABE8(-1.0f, 1.0f);
        i--;
        A[1] = func_002140F8_2FABE8(-1.0f, 1.0f);
        A[2] = func_002140F8_2FABE8(-1.0f, 1.0f);
        *(u128x *)B = 0;
        *(u128x *)T = *(u128x *)A;
        r = func_002140F8_2FABE8(-1.0f, 1.0f);
        B[0] = r * func_001F9F90_2FABE8(*(float *)(moby + 0x48));
        s = func_002140F8_2FABE8(-0.3f, 0.3f);
        B[1] = s * func_001F9FA8_2FABE8(*(float *)(moby + 0x48));
        B[2] = func_002140F8_2FABE8(0.0f, 2.0f);
        *(u128x *)A = *(u128x *)B;
        func_001F9BD8_2FABE8(A, A, moby + 0x10);
        f0 = func_002140F8_2FABE8(0.5f, 1.5f);
        func_L00_001FF4B0_2FABE8(T, T, f0 * D_0015EE6C_2FABE8);
        f20 = func_002140F8_2FABE8(0.5f, 1.0f) * 210000.0f;
        res = func_L00_00258BC8_2FABE8(30, 90);
        k = func_001F9850_2FABE8(res);
        func_L00_0026DD70_2FABE8(A, T, 0x5F787878, 0x181818, f20, k);
    } while (i >= 0);

    i = 0x27;
    do {
        q = src;
        ((struct u64un *)T)->v = ((struct u64un *)q)->v;
        *(unsigned int *)(T + 2) = *(unsigned int *)(q + 8);
        *(u128x *)B = 0;
        B[0] = func_002140F8_2FABE8(-1.0f, 1.0f);
        i--;
        B[1] = func_002140F8_2FABE8(-1.0f, 1.0f);
        B[2] = func_002140F8_2FABE8(-1.0f, 1.0f);
        *(u128x *)C = 0;
        *(u128x *)A = *(u128x *)B;
        r = func_002140F8_2FABE8(-1.2f, 1.2f);
        C[0] = r * func_001F9F90_2FABE8(*(float *)(moby + 0x48));
        s = func_002140F8_2FABE8(-0.4f, 0.4f);
        C[1] = s * func_001F9FA8_2FABE8(*(float *)(moby + 0x48));
        C[2] = func_002140F8_2FABE8(0.5f, 2.0f);
        *(u128x *)B = *(u128x *)C;
        func_001F9BD8_2FABE8(B, B, moby + 0x10);
        f0 = func_002140F8_2FABE8(1.0f, 5.0f);
        func_L00_001FF4B0_2FABE8(A, A, f0 * D_0015EE6C_2FABE8);
        idx = func_002140B0_2FABE8(3);
        f20 = func_002140F8_2FABE8(0.04f, 0.09f);
        res = func_L00_00258BC8_2FABE8(60, 180);
        func_L01_002F9908_2FABE8(B, A, *(unsigned int *)((char *)T + (idx << 2)), res, f20, 1.0f, 1.0f, 0.75f, 0);
    } while (i >= 0);

    func_L00_002584A8_2FABE8(moby, 0, -1);
    func_0020D678_2FABE8(moby);
}
