/* NON_MATCHING func_L13_0030A1C8 -- src/overlays/l13_gemlik/vendor_002EBD00.c
 * Best so far: SIZE ours 1648 / retail 1632, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Level 13 moby update: effect state machine on moby[0x20] with a 12-way jump table on a value from func_0025B4D
 *   Best by size: p3.c (1648 vs 1632, 16 over). Cases 3-10 before 1/2 and the f20 branch sense match retail's layo
 *   Unblock: the saved-register set (regalloc.py on p3.c) and the 0x1A4/0x164 store shape around func_00215570.
 */
extern void func_L13_0030A9B0(void);
extern float func_002140F8(float, float);
extern float func_001F9878(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_001F9908(void *);
extern char *func_L00_0025B478(void *, int, int);
extern int func_L00_0025B4D0(void *, void *, void *, int, int *, float *, int, int);
extern int func_001F9850(int);
extern float func_L00_001FF860(float, float);
extern void func_L00_0025D5B0(void *, void *, float, int, int, int);
extern void func_L00_002584A8(void *, int, int);
extern void func_L00_0025E4B0(void *m, short *p);
extern void func_L00_0025E590(void *, void *);
extern int func_L00_00260FB0(void *, void *, float, int, int, void *, int);
extern float func_001F9D48(void *, void *);
extern float func_001F9B88(float);
extern int func_00215570(void *, int);
extern float func_00214358(void *, int, float);
extern int func_L00_001F3958(void);
extern void func_L00_0025F4A8(void *, void *, void *, float, float, int, int, int, float, float, float, int, float, float, int, int, int, int);
extern void func_0020D678(void *);
extern float func_001F9CE8(void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern char *D_L13_001B0AB0[];
extern char D_L13_00160700[] MACRO_ADDR;
extern char D_L13_001741C0[];
extern char D_0013E633[];
extern short D_L13_00161EE8;
extern short D_L13_00161EF0;
extern short D_L13_00161EF4;
extern short D_L13_00161EF8;
extern short D_L13_00161EFC;

/* Level 13 moby update: effect state machine that steers the moby's height and clamps its position to the level bounds. */
void func_L13_0030A1C8(char *moby)
{
    char *data = *(char **)(moby + 0x78);
    float sp34;
    int sp30;
    char *r19;
    int sel;
    float fz;
    float v20[4];
    float P70[4];

    *(float *)(moby + 0x2C) = *(float *)(*(char **)(moby + 0x24) + 0x24) * *(float *)&D_L13_00161EE8;
    func_L13_0030A9B0();

    if (*(int *)(data + 0x38) != 0) {
        int t = func_001FA898_r(func_001F9878(func_002140F8(180.0f, 240.0f)));
        *(int *)(data + 0x184) = t;
        *(int *)(data + 0x38) = 0;
    }

    fz = 0.0f;
    sp34 = 0.0f;
    func_001F9908(data + 0x184);
    r19 = func_L00_0025B478(moby, 0x330000, 0);
    sel = func_L00_0025B4D0(moby, r19, data + 0x20, 0, &sp30, &sp34, 0, 4);

    if (sp30 != 1 && *(unsigned char *)(moby + 0x20) != 0x10) {
        float d;

        if (r19 != 0 && *(char **)(r19 + 0x20) != 0 && *(short *)(*(char **)(r19 + 0x20) + 0xA6) == 0x4EE) {
            sp34 = fz;
        }
        d = *(float *)(data + 0x20) - sp34;
        *(float *)(data + 0x20) = d;
        if (d <= 0.0f) sel = 1;

        *(int *)(data + 0x80) = 0x2CD;
        *(float *)(data + 0x88) = 0.7f;

        switch (sel) {
        case 0:
            break;
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
            {
                float f0;
                *(unsigned char *)(moby + 0x20) = 0xF;
                func_001F9850(0x3C);
                *(short *)(data + 0x26) = 0xF;
                *(float *)(data + 0xB0) = 8.0f;
                *(float *)(data + 0x7C) = *(float *)&D_L13_00161EF4 * D_0015EE6C;
                *(float *)(data + 0xB4) = 15.0f;
                *(float *)(data + 0x78) = *(float *)&D_L13_00161EF0 * D_0015EE6C;
                f0 = func_L00_001FF860(*(float *)(r19 + 0x10), *(float *)(r19 + 0x14));
                func_L00_0025D5B0(moby, data + 0x60, f0, 7, 1, 0);
                *(unsigned char *)(data + 0x117) = 0xFA;
            }
            break;
        case 1:
        case 2:
            {
                float f0;
                *(unsigned char *)(moby + 0x20) = 0x10;
                *(short *)(moby + 0x34) = *(unsigned short *)(moby + 0x34) & 0xEFFF;
                *(float *)(data + 0xB0) = 9.0f;
                *(float *)(data + 0xB4) = 17.0f;
                *(float *)(data + 0x7C) = *(float *)&D_L13_00161EFC * D_0015EE6C;
                *(float *)(data + 0x78) = *(float *)&D_L13_00161EF8 * D_0015EE6C;
                f0 = func_L00_001FF860(*(float *)(r19 + 0x10), *(float *)(r19 + 0x14));
                func_L00_0025D5B0(moby, data + 0x60, f0, 8, 1, 0);
                *(unsigned char *)(data + 0x117) = 0xFA;
                func_L00_002584A8(moby, 0, -1);
            }
            break;
        case 11:
            break;
        default:
            break;
        }

        func_L00_0025E4B0(moby, (short *)(data + 0x110));
        *(unsigned char *)(moby + 0xA4) = 0xFF;
        func_L00_0025E590(moby, data + 0x110);
    } else {
        *(unsigned char *)(moby + 0xA4) = 0xFF;
        func_L00_0025E590(moby, data + 0x110);
    }

    if (*(int *)(data + 0x184) != 0) {
        *(float *)(data + 0x190) = *(float *)(data + 0x1A0);
    } else {
        *(float *)(data + 0x190) = *(float *)(data + 0x19C);
    }
    {
        char *st = D_L13_001B0AB0[*(int *)(data + 0x180)];
        int r2 = func_L00_00260FB0(moby, data + 0x120, *(float *)(data + 0x190), 0, 0, st + 0x10, *(int *)st);
        if (r2 != 2) {
            float f0 = func_001F9D48(data + 0x170, data + 0x120);
            int r5;
            if (*(float *)(data + 0x190) < f0) {
                *(int *)(data + 0x164) = 2;
            } else {
                float f = func_001F9B88(*(float *)(moby + 0x18) - *(float *)(data + 0x128));
                if (3.0f < f) *(int *)(data + 0x164) = 2;
            }
            r5 = *(int *)(data + 0x1A4);
            if (r5 == -1) {
                *(int *)(data + 0x1A4) = -1;
            } else {
                if (func_00215570(data + 0x120, r5) == 0) {
                    *(int *)(data + 0x164) = 2;
                } else {
                    *(int *)(data + 0x1A4) = -1;
                }
            }
        }
    }

    if (*(int *)(data + 0x160) == 0) {
        *(int *)(data + 0x160) = *(int *)(D_0013E633 + 0x2E9D);
    }
    {
        float f20 = func_00214358(moby + 0x10, 0, 0.5f);
        if (f20 != 0.0f) {
            switch (func_L00_001F3958()) {
            case 0:
            case 1:
            case 3:
            case 11:
            case 13:
                *(short *)(moby + 0x34) = *(unsigned short *)(moby + 0x34) & 0xEFFF;
                func_L00_002584A8(moby, 0, -1);
                func_L00_0025F4A8(moby, D_L13_00160700, 0, 0.0f, 0.0f, 10, 3, 0x10, 4.0f, 2.0f, 9.0f, -1, 1.0f, 15.0f, 1, 1, -1, 0);
                func_0020D678(moby);
                return;
            default:
                break;
            }
            if (*(unsigned char *)(moby + 0x20) < 0xF) {
                float f0 = func_001F9CE8(D_L13_001741C0 + 0x40);
                float f1;
                f0 = func_L00_001FF860(*(float *)(D_L13_001741C0 + 0x48), f0);
                if (0.69813168f < f0) {
                    func_L00_001FF4B0(v20, D_L13_001741C0 + 0x40, 0.05f);
                    func_001F9BD8(moby + 0x10, moby + 0x10, v20);
                }
                f1 = *(float *)(moby + 0x18);
                if (f20 < f1) {
                    float g = *(float *)&D_0015EE70 * 9.8f;
                    float h = *(float *)(data + 0x68) - g;
                    *(float *)(data + 0x68) = h;
                    f0 = *(float *)(moby + 0x18) + h;
                    *(float *)(moby + 0x18) = f0;
                    if (f0 < f20) {
                        *(int *)(data + 0x68) = 0;
                        *(float *)(moby + 0x18) = f20;
                    }
                } else if (f1 < f20) {
                    *(float *)(moby + 0x18) = f1 + 0.05f;
                }
            }
        } else {
            if (*(unsigned char *)(moby + 0x20) < 0xF) {
                float f0 = *(float *)&D_0015EE70 * 9.8f;
                float f1 = *(float *)(data + 0x68) - f0;
                *(float *)(data + 0x68) = f1;
                *(float *)(moby + 0x18) = *(float *)(moby + 0x18) + f1;
            }
        }
    }

    {
        float f0 = *(float *)(moby + 0x10);
        if (f0 < 2.0f || 1021.0f < f0 || *(float *)(moby + 0x14) < 2.0f || 1021.0f < *(float *)(moby + 0x14)
            || *(float *)(moby + 0x18) < 2.0f || 1021.0f < *(float *)(moby + 0x18)) {
            func_0020D678(moby);
        }
    }
}
