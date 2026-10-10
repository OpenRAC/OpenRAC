/* NON_MATCHING func_L01_002FABE8 -- src/overlays/l01_novalis/vendor_002FABE8.c
 * Best so far: SIZE ours 984 / retail 992, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Rock (class 704) update on level 01: two rounds of 200 and 40 iterations of random vector sets from moby+0x48 
 *   Left: retail keeps the hi part of D_L01_00161CE0 in $s6 across the whole function and forms $v1 = $s6 + 0x1CE0
 *   Unblock: a form of the second loop's base pointer that gcc keeps as an addiu value, and the saved-register ord
 */
extern float func_002140F8(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern int func_L00_00258BC8(int, int);
extern int func_001F9850(int);
extern void func_L00_0026DD70(void *, void *, int, int, float, int);
extern int func_002140B0(int);
extern void func_L01_002F9908(void *, void *, unsigned int, int, float, float, float, float, int);
extern void func_L00_002584A8(void *, int, int);
extern void func_0020D678(void *);
extern int func_0022ED80(int, int, int);
extern void *func_L00_0025B478_FB158(void *, int, int);
extern float D_0015EE6C MACRO_ADDR;
extern char D_L01_00161CE0[];
typedef int u128x __attribute__((mode(TI)));
struct u64un { unsigned long long v; } __attribute__((packed));


/* Rock formation update: builds two random vector sets from the moby's velocity, emits sparks and a shower. */
void func_L01_002FABE8(char *moby) {
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
    func_0022ED80(0, 0x10, (int)moby);
    src = D_L01_00161CE0;

    i = 0xC7;
    do {
        *(u128x *)A = 0;
        A[0] = func_002140F8(-1.0f, 1.0f);
        i--;
        A[1] = func_002140F8(-1.0f, 1.0f);
        A[2] = func_002140F8(-1.0f, 1.0f);
        *(u128x *)B = 0;
        *(u128x *)T = *(u128x *)A;
        func_002140F8(-1.0f, 1.0f);
        r = func_001F9F90(*(float *)(moby + 0x48));
        B[0] = r * r;
        func_002140F8(-0.3f, 0.3f);
        s = func_001F9FA8(*(float *)(moby + 0x48));
        B[1] = s * s;
        B[2] = func_002140F8(0.0f, 2.0f);
        *(u128x *)A = *(u128x *)B;
        func_001F9BD8(A, A, moby + 0x10);
        f0 = func_002140F8(0.5f, 1.5f);
        func_L00_001FF4B0(T, T, f0 * D_0015EE6C);
        f20 = func_002140F8(0.5f, 1.0f) * 210000.0f;
        res = func_L00_00258BC8(30, 90);
        k = func_001F9850(res);
        func_L00_0026DD70(A, T, 0x5F787878, 0x181818, f20, k);
    } while (i >= 0);

    i = 0x27;
    do {
        q = src;
        ((struct u64un *)T)->v = ((struct u64un *)q)->v;
        *(unsigned int *)(T + 2) = *(unsigned int *)(q + 8);
        *(u128x *)B = 0;
        B[0] = func_002140F8(-1.0f, 1.0f);
        i--;
        B[1] = func_002140F8(-1.0f, 1.0f);
        B[2] = func_002140F8(-1.0f, 1.0f);
        *(u128x *)C = 0;
        *(u128x *)A = *(u128x *)B;
        func_002140F8(-1.2f, 1.2f);
        r = func_001F9F90(*(float *)(moby + 0x48));
        C[0] = r * r;
        func_002140F8(-0.4f, 0.4f);
        s = func_001F9FA8(*(float *)(moby + 0x48));
        C[1] = s * s;
        C[2] = func_002140F8(0.5f, 2.0f);
        *(u128x *)B = *(u128x *)C;
        func_001F9BD8(B, B, moby + 0x10);
        f0 = func_002140F8(1.0f, 5.0f);
        func_L00_001FF4B0(A, A, f0 * D_0015EE6C);
        idx = func_002140B0(3);
        f20 = func_002140F8(0.04f, 0.09f);
        res = func_L00_00258BC8(60, 180);
        func_L01_002F9908(B, A, *(unsigned int *)((char *)T + (idx << 2)), res, f20, 1.0f, 1.0f, 0.75f, 0);
    } while (i >= 0);

    func_L00_002584A8(moby, 0, -1);
    func_0020D678(moby);
}
