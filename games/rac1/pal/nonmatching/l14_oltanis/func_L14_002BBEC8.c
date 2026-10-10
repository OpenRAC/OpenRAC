/* NON_MATCHING func_L14_002BBEC8 -- src/overlays/l14_oltanis/vendor_002ACCC0.c
 * Best so far: SIZE ours 1688 / retail 1684, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Oltanis moby update: clamps the moby's position and two floats, then either picks a spark/beam from a 32-byte 
 *   Left: retail keeps five floats live across the loop (f20-f24 saved in the prologue); ours saves none of them a
 *   Unblock: a form that keeps the 0.01/0.5/1.0 values in named floats across the loop so the compiler saves them.
 */
extern int func_001F9938(void *);
extern void func_001F9BD8(void *, void *, void *);
extern float func_001F9B88(float);
extern int func_L00_001F10E0(float, void *, int, void *);
extern void func_001F9BC0(void *);
extern int func_L00_0028EF68(int i, int a1, int v, int k);
extern void func_L00_0025F4A8(void *, void *, void *, float, float, int, int, int, float, float, float, int, float, float, int, int, int, int);
extern void func_L14_002BC728(char *moby);
extern float func_001FA748(float, float);
extern void func_L00_00250800(void *, int, void *);
extern float func_L00_00258C80(float lo, float hi);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern int func_001F9850(int);
extern int func_L00_00258BC8(int, int);
extern void func_L00_0026DA50(void *, void *, int, int, int, int, float);
extern int func_002140B0(int);
extern float func_002140F8(float, float);
extern char *func_L00_0026DEA0(void *, int, void *, int, float, float, float, float);
extern int func_L00_0023F0D0(float *, float, float, float, float, float);
extern float D_0015EE60 MACRO_ADDR;
extern float D_0015EE64 MACRO_ADDR;
extern float D_L14_0015F660[] MACRO_ADDR;
extern char D_L14_00180AC0[];
extern char D_0013E633[];
extern short D_L14_001616A8;
extern short D_L14_001616A4;
extern short D_L14_001616A0;

/* Oltanis moby class 81 update: clamp and ease a point, then pick a spark or beam from the list. */
void func_L14_002BBEC8(char *moby) {
    char *data;
    char *p;
    char *pc0;
    char *pe0;
    char *q16;
    char *r17;
    float a20[4];
    float a30[4];
    float a40[4];
    float a50[4];
    float a60[4];
    float f20;
    float f21;
    float f22;
    float f23;
    float f24;
    float r;
    int i;
    int r6;
    int r2;
    int r4;
    int r1;
    int col;

    data = *(char **)(moby + 0x78);
    p = moby + 0x10;
    if (func_001F9938(data + 0x10) != 0) {
        goto l190;
    }
    func_001F9BD8(p, p, data);
    if (*(float *)&D_L14_001616A8 * D_0015EE60 < func_001F9B88(*(float *)data)) {
        *(float *)data = *(float *)data * ((*(float *)&D_L14_001616A4 - 1.0f) * D_0015EE60 + 1.0f);
    }
    if (*(float *)&D_L14_001616A8 * D_0015EE60 < func_001F9B88(*(float *)(data + 4))) {
        *(float *)(data + 4) = *(float *)(data + 4) * ((*(float *)&D_L14_001616A4 - 1.0f) * D_0015EE60 + 1.0f);
    }
    if (0.0f < *(float *)(data + 8)) {
        *(float *)(data + 8) = *(float *)(data + 8) * ((*(float *)&D_L14_001616A4 - 1.0f) * D_0015EE60 + 1.0f);
    }
    *(float *)(data + 8) = *(float *)(data + 8) - D_0015EE64 * *(float *)&D_L14_001616A0;

    f21 = 2.0f;
    if (*(float *)(moby + 0x10) < f21 || 1020.0f < *(float *)(moby + 0x10)) {
        goto l190;
    }
    if (*(float *)(moby + 0x14) < f21 || 1020.0f < *(float *)(moby + 0x14)) {
        goto l190;
    }
    if (*(float *)(moby + 0x18) < f21 || 1020.0f < *(float *)(moby + 0x18)) {
        goto l190;
    }
    if (func_L00_001F10E0(0.75f, p, 0, 0) != 0) {
        goto l108;
    }
    f20 = 10.0f;
    if (func_001F9B88(*(float *)(moby + 0x10) - *(float *)((D_0013E633 + 0xE1D) + 0x80)) < f20) {
        if (func_001F9B88(*(float *)(moby + 0x14) - *(float *)((D_0013E633 + 0xE1D) + 0x84)) < f20) {
            goto l108;
        }
    }

    /* far from the target: ease the rest and pick a new point */
    *(float *)(moby + 0x40) = func_001FA748(*(float *)(moby + 0x40), *(float *)(data + 0x14));
    pe0 = moby + 0xE0;
    *(float *)(moby + 0x44) = func_001FA748(*(float *)(moby + 0x44), *(float *)(data + 0x18));
    f21 = 0.01f;
    f24 = f21;
    f20 = 0.5f;
    f23 = f20;
    f22 = 1.0f;
    *(float *)(moby + 0x48) = func_001FA748(*(float *)(moby + 0x48), *(float *)(data + 0x1C));
    func_L00_00250800(moby, 0, a20);
    pc0 = moby + 0xC0;
    r = func_L00_00258C80(f21, f20);
    func_001F9C30(a30, pc0, r);
    func_001F9BD8(a40, a20, a30);
    func_001F9BF0(a50, a20, p);
    func_L00_001FF4B0(a50, a50, 0.05f);
    r1 = func_001F9850(10);
    r2 = func_001F9850(20);
    r4 = func_L00_00258BC8(r1, r2);
    func_L00_0026DA50(a40, a50, 0x4F007FFF, 0x1FFFFFFF, r4, 1, 40000.0f);

    for (i = 0; i < 3; i++) {
        if (i != 2) {
            r = func_L00_00258C80(f24, f23);
            func_001F9C30(a30, pc0, r);
            func_001F9BD8(a40, p, a30);
            r = func_L00_00258C80(f24, f23);
            func_001F9C30(a30, moby + 0xD0, r);
            func_001F9BD8(a40, p, a30);
        }
        r6 = func_002140B0(6);
        r2 = func_002140B0(2);
        f20 = func_002140F8(20000.0f, 100000.0f);
        if (r2 != 0) {
            r6 = -r6;
        }
        r4 = func_L00_00258BC8(0x10, 0xFF);
        col = 0x60000000 | (r4 << 8) | (r4 << 16) | r4;
        r17 = func_L00_0026DEA0(a40, r6, D_L14_0015F660, col, 0.05f, f22, f22, f20);
        if (r17 != 0) {
            r17[3] = 0x44;
            *(short *)(r17 + 0xA) = func_001F9850(60);
            q16 = r17 + 0x20;
            q16[0xA] = 0x60;
            *(int *)(q16 + 4) = 2;
            q16[0xB] = r17[0xA];
        }
    }

    func_001F9BF0(a60, a20, p);
    f20 = f22;
    func_001F9C30(a60, a60, 2.5f);
    func_001F9BD8(a40, p, a60);
    if (func_002140B0(2) != 0) {
        func_001F9C30(a60, pe0, -1.0f);
    } else {
        qcopy(a60, pe0);
    }
    func_001F9BD8(a40, a40, a60);
    if (*(short *)(data + 0x12) == -1) {
        *(short *)(data + 0x12) = func_L00_0023F0D0(a40, 3.0f, 0.0f, 1.0f, 0.8f, 0.2f);
    } else {
        char *e = D_L14_00180AC0 + (*(short *)(data + 0x12) << 5);
        qcopy(e + 0x10, a40);
        *(float *)(e + 8) = 0.2f;
        *(float *)(e + 0x1C) = 3.0f;
        *(float *)e = f20;
        *(float *)(e + 4) = 0.8f;
    }
    return;

l108:
    func_001F9BC0(a20);
    func_L00_0028EF68(1, 0, (int)moby, *(short *)(*(char **)(data + 0x20) + 0xA6));
    func_L00_0025F4A8(moby, a20, p, 2.0f, 4.0f, 5, 2, 4, 1.0f, 0.5f, 9.0f, -1, 1.0f, 5.0f, 0, 2, -1, 0);
l190:
    func_L14_002BC728(moby);
}
