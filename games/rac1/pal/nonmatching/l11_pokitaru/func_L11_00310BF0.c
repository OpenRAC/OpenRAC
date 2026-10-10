/* NON_MATCHING func_L11_00310BF0 -- src/overlays/l11_pokitaru/vendor_002CC828.c
 * Best so far: SIZE ours 1644 / retail 1648, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Ammo/jet-fighter crate update: homes on the player, lifts and spins, then a pickup state machine (switch on mo
 *   Stopped at run 7 of 10 (best: p6.c, size 1644 vs 1648, 4 bytes short; p5.c is the same bytes with the 0xFE/0xF
 */
extern char *D_L11_001748D8;
extern char D_0013E633[];
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern void func_L00_00250800(void *, int, void *);
extern int func_L00_001F10E0(float, void *, int, void *);
extern void func_L00_0025F4A8(void *, void *, void *, float, float, int, int, int, float, float, float, float, int, float, int, int, int, int);
extern float func_001F9D10(void *, void *);
extern float func_001F9D48(void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern void func_0022ED80(int, int, int);
extern int func_L11_00310B28(char *owner, char *data);
extern float func_001FA748(float, float);
extern float func_001F9F90(float);
extern float func_001F9B50(float);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_0020D678(void *);
extern void func_L00_0025B040(unsigned char *, float);

/* Update for an ammo or jet-fighter crate moby: homes on the player, lifts and spins, then runs its pickup state machine. */
void func_L11_00310BF0(char *moby) {
    float vec[4];
    char *d;
    char *p21;
    char *p16;
    char *p17;
    char *p6;
    char *p;
    char *q;
    int flag = 0;
    float f20;
    float f21;
    int r;
    int r2;
    float fr1;
    float fr2;
    float fr3;
    float fl;
    d = *(char **)(moby + 0x78);
    {
        char *k = D_0013E633 + 0xE1D;
        p21 = *(char **)(k + 0x15F0);
    }
    f21 = 100000.0f;
    f20 = f21;
    func_L00_00250800(moby, 0, vec);
    r2 = func_L00_001F10E0(0.55f, vec, 0, moby);
    if (r2 != 0) {
        if (D_L11_001748D8 != 0 && (*(short *)(D_L11_001748D8 + 0xA6) == 0x4DA || *(short *)(D_L11_001748D8 + 0xA6) == 0x45 || *(short *)(D_L11_001748D8 + 0xA6) == 0x563)) {
            flag = 1;
        } else {
            func_L00_0025F4A8(moby, d + 0x10, vec, 0.0f, 0.0f, 3, 2, 5, 0.0f, 0.0f, 9.0f, 1.0f, -1, 0.0f, 0, 1, -1, 0);
            moby[0x20] = 5;
        }
    }
    if (*(unsigned char *)(moby + 0x20) < 4) {
        {
            char *k = D_0013E633 + 0xE1D;
            if (*(int *)(k + 0x2084) == 0x32 && p21 != 0) {
                if (*(unsigned char *)(p21 + 0x20) != 0xFE) {
                    if (*(unsigned char *)(p21 + 0x20) != 0xFD) {
                        if (*(short *)(p21 + 0xA6) == 0x4DA || *(short *)(p21 + 0xA6) == 0x45 || *(short *)(p21 + 0xA6) == 0x563) {
                            p17 = p21 + 0x10;
                            p16 = moby + 0x10;
                            f21 = func_001F9D10(p17, p16);
                            func_001F9D48(p17, p16);
                            func_001F9BD8(vec, p16, d);
                            func_001F9C30(vec, vec, 0.5f);
                            f20 = func_001F9D48(p17, vec);
                        }
                    }
                }
            }
        }
        if (flag != 0 || f21 < 4.0f || f20 < 4.0f) {
            *(int *)(moby + 0x94) = 0;
            p = *(char **)(d + 0x34);
            if (p != 0) *(int *)(p + 0x94) = 0;
            switch (*(short *)(moby + 0xA6)) {
            default:
                moby[0x20] = 4;
                break;
            case 0x4C2: {
                char *k = D_0013E633 + 0xE1D;
                *(unsigned char *)(k + 0x15F6) = *(unsigned char *)(k + 0x15F6) + 5;
                p6 = *(char **)(k + 0x15F0);
                if (p6 != 0) {
                    if (*(unsigned char *)(p6 + 0x20) != 0xFE) {
                        if (*(unsigned char *)(p6 + 0x20) != 0xFD) func_0022ED80(4, 0, (int)p6);
                    }
                }
                {
                    char *k2 = D_0013E633 + 0xE1D;
                    if (*(unsigned char *)(k2 + 0x15F7) < *(unsigned char *)(k2 + 0x15F6)) *(unsigned char *)(k2 + 0x15F6) = *(unsigned char *)(k2 + 0x15F7);
                }
                moby[0x20] = 4;
                break;
            }
            case 0x4C4: {
                char *k = D_0013E633 + 0xE1D;
                *(float *)(k + 0x15FC) = *(float *)(k + 0x15FC) + *(float *)(k + 0x1600) * 0.25f;
                p6 = *(char **)(k + 0x15F0);
                if (p6 != 0) {
                    if (*(unsigned char *)(p6 + 0x20) != 0xFE) {
                        if (*(unsigned char *)(p6 + 0x20) != 0xFD) func_0022ED80(5, 0, (int)p6);
                    }
                }
                {
                    char *k2 = D_0013E633 + 0xE1D;
                    if (*(float *)(k2 + 0x1600) < *(float *)(k2 + 0x15FC)) *(float *)(k2 + 0x15FC) = *(float *)(k2 + 0x1600);
                }
                moby[0x20] = 4;
                break;
            }
            }
        }
    }
    switch (*(unsigned char *)(moby + 0x20)) {
    case 0:
        *(int *)(d + 0x34) = 0;
        func_L11_00310B28(moby, d);
        *(int *)(d + 0x30) = 0;
        moby[0x20] = 1;
        break;
    case 1:
        p16 = d + 0x10;
        func_001F9C30(p16, p16, 0.9f);
        *(float *)(d + 0x18) = *(float *)(d + 0x18) - D_0015EE70 * 9.8f;
        func_001F9BD8(d, d, p16);
        func_001F9BD8(moby + 0x10, d, d + 0x20);
        r = func_L11_00310B28(moby, d);
        if (r != 0) {
            p = *(char **)(d + 0x34);
            if (*(unsigned char *)(p + 0x52) == 1) {
                func_001F9C30(p16, p16, 0.8f);
                moby[0x20] = 2;
            }
        }
        break;
    case 2:
        p16 = d + 0x10;
        p17 = d + 0x10;
        q = moby + 0x10;
        f20 = 0.8f;
        fr1 = func_001FA748(*(float *)(moby + 0x48), D_0015EE6C * 0.034906585f);
        *(float *)(moby + 0x48) = fr1;
        fr2 = func_001FA748(*(float *)(d + 0x30), D_0015EE6C * 1.5707964f);
        *(float *)(d + 0x30) = fr2;
        fr3 = func_001F9F90(fr2);
        *(float *)(moby + 0x44) = fr3 * 0.7853982f;
        func_001F9C30(p16, p16, f20);
        *(float *)(d + 0x18) = *(float *)(d + 0x18) - D_0015EE70 * 9.8f;
        if (f21 < 40.0f) {
            if (*(unsigned char *)(moby + 0x31) != 0) {
                fl = func_001F9B50(f21);
                f20 = f20 / fl;
                func_001F9BF0(vec, p21 + 0x10, q);
                func_L00_001FF4B0(vec, vec, f20);
                func_001F9BD8(p17, p17, vec);
            }
        }
        func_001F9BD8(d, d, p17);
        func_001F9BD8(q, d, d + 0x20);
        r = func_L11_00310B28(moby, d);
        if (r != 0) {
            p = *(char **)(d + 0x34);
            if (*(unsigned char *)(p + 0x20) == 2) moby[0x20] = 2;
        }
        break;
    case 3:
        *(float *)(d + 0x18) = *(float *)(d + 0x18) - D_0015EE70 * 9.8f;
        func_001F9BD8(moby + 0x10, moby + 0x10, d + 0x10);
        break;
    case 4:
        moby[0x20] = 5;
        break;
    case 5:
        func_0020D678(moby);
        return;
    default:
        break;
    }
    if (*(float *)(moby + 0x18) < 20.0f) {
        func_0020D678(moby);
        return;
    }
    if (*(float *)(moby + 0x10) < 20.0f) *(float *)(moby + 0x10) = 20.0f;
    if (1003.0f < *(float *)(moby + 0x10)) *(float *)(moby + 0x10) = 1003.0f;
    if (*(float *)(moby + 0x14) < 20.0f) *(float *)(moby + 0x14) = 20.0f;
    if (1003.0f < *(float *)(moby + 0x14)) *(float *)(moby + 0x14) = 1003.0f;
    if (*(float *)(moby + 0x18) < 20.0f) *(float *)(moby + 0x18) = 20.0f;
    if (1003.0f < *(float *)(moby + 0x18)) *(float *)(moby + 0x18) = 1003.0f;
    func_L00_0025B040(moby, 0.5f);
}
