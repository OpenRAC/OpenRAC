/* NON_MATCHING func_L05_002F9BA0 -- src/overlays/shared/vendor_002CF2C0.c
 * Best so far: SIZE ours 932 / retail 920, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Hoverboard moby update (class 439, levels 05 and 16): state 0 steers toward the rider and may switch to state 
 *   Left: one saved register too many (retail saves $16-$20; ours also saves $21, the v10 address pointer, because
 *   Budget: 8 runs spent (p0 to p7). regalloc.py failed to produce its dump for this candidate, so the priorities 
 */
extern f32 func_001F9D10_1edff8(void *, void *) __asm__("func_001F9D10");
extern void func_L05_0024D680(int, int);
extern int func_001F9850(int);
extern int func_L00_00203F20(int a, int b);
extern void func_001F9BF0(void *, void *, void *);
extern float func_001F9CB8(void *);
extern void func_L00_00250800(void *, int, void *);
extern float func_00214158(void);
extern f32 func_001F9F90(f32);
extern void func_L00_001FF4B0(void *, void *, float);
extern float func_001F9FA8(float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_L00_00258BC8(int, int);
extern void func_L00_0026A7F8(void *, void *, int, int, int, int, int, int);
extern void func_001F9C30(void *, void *, float);
extern void func_L00_0026DA50(void *, void *, int, int, int, int, float);
extern int D_L05_0015F6B0 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern int D_L05_0015F788 MACRO_ADDR;
extern int D_L05_001619E8;
extern int D_L05_001619CC;
extern int D_L05_001619D0;
extern char D_0013E633[];
extern unsigned char D_0013D355[];
extern unsigned char D_0014171B[];

// Hoverboard moby update (class 439, levels 05 and 16): steers toward its rider, then runs the vector and sound setup.
void func_L05_002F9BA0(unsigned char *moby) {
    char *pos;
    char *data;
    char *g;
    char *gq;
    char v0[16];
    char v10[16];
    char v20[16];
    char v30[16];
    char v40[16];
    float f1;
    int one = 1;

    pos = (char *)moby + 0x10;
    data = *(char **)(moby + 0x78);
    if (moby[0x20] == 0) {
        g = D_0013E633 + 0xE9D;
        if (func_001F9D10_1edff8(g, pos) < 5.5f) {
            if (D_L05_0015F6B0 >= 4) {
                gq = D_0013E633 + 0xE1D;
                if (*(int *)(gq + 0x208C) != 0x15 && *(int *)(gq + 0x208C) != 0x16) {
                    *(unsigned char **)(gq + 0x86C) = moby;
                    func_L05_0024D680(0x6B, 1);
                    if (*(int *)(data + 0x50) == 0) {
                        qcopy(pos, data + 0x10);
                    }
                    moby[0x30] = 0xFF;
                    moby[0x20] = one;
                }
            }
        }
    } else if (moby[0x20] == one) {
        g = D_0013E633 + 0xE1D;
        if (*(int *)(data + 0x50) == 0) {
            if (D_0013D355[0x13B] != 0) {
                if (*(unsigned short *)(D_0014171B + 0x31D) == 0) {
                    if (D_L05_0015F788 != 0) {
                        int ret = func_001F9850(0x4B0);
                        if (ret < *(int *)(g + 0x19C)) {
                            if (*(unsigned short *)(D_0014171B + 0x645) == 0) {
                                func_L00_00203F20(0x138F, 0x5F);
                            }
                        }
                    }
                }
            }
        }
        if (*(int *)(g + 0x208C) != 0x16) {
            moby[0x30] = 0x10;
            if (*(int *)(data + 0x50) != 0) {
                moby[0x20] = 0;
            } else {
                qcopy(pos, data + 0x10);
                moby[0x20] = 0;
            }
        }
    }

    func_001F9BF0(v0, pos, data);
    qcopy(data, pos);
    f1 = func_001F9CB8(v0);
    if (moby[0xBC] != 2 && f1 < 2.0f) {
        float f20;
        float f21;
        func_L00_00250800(moby, D_L05_0015F6B0 & 1, v10);
        f21 = func_00214158();
        f20 = D_0015EE6C * 0.25f;
        func_L00_001FF4B0(v20, moby + 0xE0, f20 * func_001F9F90(f21));
        func_L00_001FF4B0(v30, moby + 0xD0, f20 * func_001F9FA8(f21));
        func_001F9BD8(v20, v20, v0);
        func_001F9BD8(v20, v20, v30);
        qcopy(v20, v0);
        *(float *)(v20 + 0xC) = (float)D_L05_001619E8;
        if (moby[0xBC] == 0) {
            int a;
            int b;
            int c;
            a = func_001F9850(10);
            b = func_001F9850(13);
            c = func_L00_00258BC8(a, b);
            func_L00_0026A7F8(v10, v20, D_L05_001619CC, D_L05_001619D0, c, 15, 15, 1);
        } else {
            int a;
            int b;
            int c;
            func_001F9C30(v40, v0, 0.92f);
            a = func_001F9850(0xF);
            b = func_001F9850(0x12);
            c = func_L00_00258BC8(a, b);
            func_L00_0026A7F8(v10, v40, 0x5032F0D2, D_L05_001619D0, c, 0x1E, 0x14, 1);
            a = func_001F9850(8);
            b = func_001F9850(0x11);
            c = func_L00_00258BC8(a, b);
            func_L00_0026DA50(v10, v20, D_L05_001619CC, D_L05_001619D0, c, 1, 15000.0f);
        }
    }
    moby[0xBC] = 0;
}
