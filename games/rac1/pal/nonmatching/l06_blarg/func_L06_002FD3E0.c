/* NON_MATCHING func_L06_002FD3E0 -- src/overlays/l06_blarg/vendor_002B5990.c
 * Best so far: SIZE ours 1628 / retail 1664, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Level 6 blarg-class moby update (class 1054): spawns a child moby (func_0020D348) and steers it through a six-
 *   Still off: the case 4 dispatch (retail loads f20/f21/f23 from the gp block and the 10/20 constants in the stat
 *   Stopped for effort, not at a listed wall: 4 of 10 runs used. A next pass would start from the case 4 arm order
 */
extern short D_L06_00161FAC;
extern short D_L06_00161FB0;
extern short D_L06_00161FB4;
extern short D_L06_00161FB8;
extern short D_L06_00161FBC;
extern short D_L06_00161FC0;
extern short D_L06_00161FC4;
extern short D_L06_00161FC8;
extern short D_L06_00161FCC;
extern short D_L06_00161FD0;
extern short D_L06_00161FD4;
extern short D_L06_00161FD8;
extern short D_L06_00161FDC;
extern short D_L06_00161FE0;
extern short D_L06_00161FE4;
extern short D_L06_00161FE8;
extern short D_L06_00161FEC;
extern struct Moby *func_0020D348_m(int) __asm__("func_0020D348");
extern float AbsoluteFloat(float input) __asm__("func_001F9B88");
extern int func_001FA898_r(float) __asm__("func_001FA898");

/* Update for moby class 1054 on level 6: spawns a child moby and steers it through a six-state machine. */
void func_L06_002FD3E0(unsigned char *moby)
{
    char *data, *c;
    float vec[4], d1, d2, f0, f12, f13, f14, f15, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, k;
    int cnt, st, nb;

    data = *(char **)(moby + 0x78);
    switch (moby[0x20]) {
    case 0:
        qcopy(data + 0x10, moby + 0x10);
        qcopy(data + 0x20, moby + 0x40);
        qcopy(data + 0x30, moby + 0xE0);
        c = (char *)func_0020D348_m(0x41F);
        *(char **)(data + 0x40) = c;
        *(short *)(c + 0x32) = 0x40;
        c[0x31] = 1;
        *(s64 *)(c + 0x38) = *(s64 *)(moby + 0x38);
        *(short *)(*(char **)(data + 0x40) + 0x34) = *(unsigned short *)(moby + 0x34);
        qcopy(*(char **)(data + 0x40) + 0x10, moby + 0x10);
        qcopy(*(char **)(data + 0x40) + 0x40, moby + 0x40);
        if (*(int *)data >= 0) {
            moby[0x20] = 1;
            return;
        }
        st = *(int *)(data + 4);
        if (st == 2) {
            moby[0x20] = 2;
            return;
        }
        func_L00_001FF4B0(vec, data + 0x30, *(float *)&D_L06_00161FAC);
        func_001F9BD8(moby + 0x10, data + 0x10, vec);
        f12 = *(float *)&D_L06_00161FB0;
        func_L00_001FF4B0(vec, data + 0x30, -f12);
        func_001F9BD8(*(char **)(data + 0x40) + 0x10, data + 0x10, vec);
        func_L00_00251E30(*(char **)(data + 0x40));
        moby[0x20] = 5;
        return;
    case 1:
        if (func_00215570(D_0013E633 + 0xE9D, *(int *)data) == 0) return;
        st = *(int *)(data + 4);
        if (st == 1) {
            moby[0x20] = 4;
            *(int *)(data + 0x4C) = func_001F9850(0x14);
        } else if (moby[0x21] == 0xFF) {
            moby[0x20] = 3;
        } else {
            func_L01_0026F090(moby[0x21], 3);
        }
        func_0022ED80(0, 0, *(int *)(data + 0x40));
        return;
    case 3:
        d1 = func_001F9D10(moby + 0x10, data + 0x10);
        d2 = func_001F9D10(*(char **)(data + 0x40) + 0x10, data + 0x10);
        func_00214D88(&d1, (float *)(data + 0x8), *(float *)&D_L06_00161FAC, *(float *)&D_0015EE70 * 10.0f, *(float *)&D_0015EE70 * 20.0f, *(float *)&D_0015EE6C * 5.0f);
        func_00214D88(&d2, (float *)(data + 0x44), *(float *)&D_L06_00161FB0, *(float *)&D_0015EE70 * 10.0f, *(float *)&D_0015EE70 * 20.0f, *(float *)&D_0015EE6C * 5.0f);
        func_L00_001FF4B0(vec, data + 0x30, d1);
        func_001F9BD8(moby + 0x10, data + 0x10, vec);
        func_L00_001FF4B0(vec, data + 0x30, -d2);
        func_001F9BD8(*(char **)(data + 0x40) + 0x10, data + 0x10, vec);
        func_L00_00251E30(*(char **)(data + 0x40));
        k = 0.01f;
        f0 = AbsoluteFloat(d1 - *(float *)&D_L06_00161FAC);
        if (!(f0 < k)) return;
        f0 = AbsoluteFloat(d2 - *(float *)&D_L06_00161FB0);
        if (!(f0 < k)) return;
        moby[0x20] = 5;
        return;
    case 4:
        d1 = func_001F9D10(moby + 0x10, data + 0x10);
        d2 = func_001F9D10(*(char **)(data + 0x40) + 0x10, data + 0x10);
        st = moby[0xBC];
        nb = 1;
        if (st == 1) {
            nb = 2;
            f12 = (float)*(int *)&D_L06_00161FE4;
            f13 = (float)*(int *)&D_L06_00161FE8;
            f20 = d1;
            f21 = d2;
            f23 = *(float *)(*(char **)(data + 0x40) + 0x44);
            f22 = *(float *)(moby + 0x44);
            cnt = 0;
            goto l7e4;
        }
        if (st == 2) {
            f12 = (float)*(int *)&D_L06_00161FDC;
            f13 = (float)*(int *)&D_L06_00161FE0;
            f21 = *(float *)&D_L06_00161FB8;
            f22 = *(float *)&D_L06_00161FBC;
            f23 = *(float *)&D_L06_00161FC0;
            cnt = 1;
            goto l7e4;
        }
        f26 = 10.0f;
        f23 = *(float *)(moby + 0x44);
        f24 = 20.0f;
        f20 = *(float *)&D_L06_00161FB4;
        f22 = f23;
        f21 = *(float *)&D_L06_00161FB8;
        f29 = f26;
        f25 = 20.0f;
        f27 = f24;
        cnt = 30;
        goto l80c;
    l7e4:
        f26 = *(float *)&D_L06_00161FC4;
        f25 = *(float *)&D_L06_00161FC8;
        f24 = *(float *)&D_L06_00161FCC;
        f29 = *(float *)&D_L06_00161FD0;
        f28 = *(float *)&D_L06_00161FD4;
        f27 = *(float *)&D_L06_00161FD8;
        f0 = func_002140F8(f12, f13);
        cnt = func_001FA898_r(f0);
    l80c:
        func_00214D88(&d1, (float *)(data + 0x8), f20, f26 * *(float *)&D_0015EE70, f25 * *(float *)&D_0015EE70, f24 * *(float *)&D_0015EE6C);
        func_00214D88(&d2, (float *)(data + 0x44), f21, f26 * *(float *)&D_0015EE70, f25 * *(float *)&D_0015EE70, f24 * *(float *)&D_0015EE6C);
        k = 0.017453292f;
        f12 = f22;
        f22 = f27 * k;
        f21 = f29 * k;
        f20 = f28 * k;
        func_00214D88((float *)(moby + 0x44), (float *)(data + 0xC), f12, f21 * *(float *)&D_0015EE70, f20 * *(float *)&D_0015EE70, f22 * *(float *)&D_0015EE6C);
        func_00214D88((float *)(*(char **)(data + 0x40) + 0x44), (float *)(data + 0x48), f23, f21 * *(float *)&D_0015EE70, f20 * *(float *)&D_0015EE70, f22 * *(float *)&D_0015EE6C);
        func_L00_001FF4B0(vec, data + 0x10, d1);
        func_001F9BD8(moby + 0x10, data + 0x10, vec);
        func_L00_001FF4B0(vec, data + 0x10, -d2);
        func_001F9BD8(*(char **)(data + 0x40) + 0x10, data + 0x10, vec);
        func_L00_00251E30(*(char **)(data + 0x40));
        if (func_001F9908(data + 0x4C) == 0) return;
        moby[0xBC] = (unsigned char)nb;
        *(int *)(data + 0x4C) = func_001F9850(cnt);
        f0 = AbsoluteFloat(d1 - *(float *)&D_L06_00161FB4);
        if (!(f0 < 0.01f)) return;
        f0 = AbsoluteFloat(d2 - *(float *)&D_L06_00161FB8);
        if (!(f0 < 0.01f)) return;
        moby[0x20] = 5;
        return;
    case 5:
        if (*(float *)&D_L06_00161FEC == 0.0f) return;
        moby[0xBC] = 0;
        moby[0x20] = 1;
        qcopy(moby + 0x10, data + 0x10);
        qcopy(moby + 0x40, data + 0x20);
        qcopy(*(char **)(data + 0x40) + 0x10, moby + 0x10);
        qcopy(*(char **)(data + 0x40) + 0x40, moby + 0x40);
        *(float *)(data + 0x48) = 0.0f;
        *(float *)(data + 0x8) = 0.0f;
        *(float *)(data + 0xC) = 0.0f;
        *(float *)(data + 0x44) = 0.0f;
        return;
    default:
        return;
    }
}
