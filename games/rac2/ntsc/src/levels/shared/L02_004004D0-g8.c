typedef int TI128 __attribute__((mode(TI)));
typedef unsigned char u8;
typedef unsigned short u16;
typedef int s32;
typedef short s16;

extern s32 D_L02_001A9018;
extern u16 **D_L02_001A9014;
extern char *D_L02_001B3C1C;
extern char D_L02_001B1510[8] __attribute__((sda));
extern char D_L02_001BA500[];

extern void func_00283098(void *, void *, void *);
extern void func_00282CC0(void *, void *, void *);
extern void func_00282CF0(void *, void *, void *);
extern void func_00282F20(void *, void *, float);
extern void func_00282D30(void *, void *, float);
extern float func_00282DB0(void *, void *);
extern int func_L00_0031C998(int, int, float);
extern void func_L00_00325AE8(void *, int, float, float);

void func_L02_004004D0(char *a0) {
    char *state = *(char **)(a0 + 0x68);
    s32 idx = *(s32 *)(state + 0x104);
    u16 *q;
    u16 *p;
    char *e;
    float v0[4];
    float v1[4];
    float v2[4];
    float v3[4];
    float d;

    if (idx == -1)
        return;
    if (idx >= D_L02_001A9018)
        return;
    q = D_L02_001A9014[idx];
    if (q == 0)
        return;
    p = q;
    do {
        e = D_L02_001B3C1C + ((*p & 0x7FFF) << 6);
        *(TI128 *)v0 = *(TI128 *)(e + 0x30);
        func_00283098(v1, D_L02_001B1510, e);
        func_00282CC0(v0, v0, v1);
        func_00282CF0(v2, D_L02_001BA500, v0);
        func_00282F20(v2, v2, 1.0f);
        func_00282D30(v3, e + 0x20, -1.0f);
        d = func_00282DB0(v2, v3);
        if (d > 0.5f) {
            d = d - 0.5f;
            d = d + d;
            func_L00_00325AE8(v0, func_L00_0031C998(0xFF8020, 0x88FF8020, d * d), 3.0f, 1.0f);
        }
    } while ((s16)*p++ >= 0);
}
